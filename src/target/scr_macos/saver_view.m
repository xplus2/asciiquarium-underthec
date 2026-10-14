#import <ScreenSaver/ScreenSaver.h>

#include <string.h>
#include <time.h>

#import "saver.h"
#include "../../app.h"
#include "../../color.h"

#define RUN_MAX 512
#define MIN_FONT_PX 10
#define MIN_FONT_PX_PREVIEW 5

/* enum color, bold */
static const unsigned char palette_rgb[9][2][3] = {
  {{0xc0, 0xc0, 0xc0}, {0xff, 0xff, 0xff}}, {{0x00, 0x00, 0x00}, {0x80, 0x80, 0x80}},
  {{0xc0, 0x00, 0x00}, {0xff, 0x55, 0x55}}, {{0x00, 0xc0, 0x00}, {0x55, 0xff, 0x55}},
  {{0xc0, 0xc0, 0x00}, {0xff, 0xff, 0x55}}, {{0x00, 0x00, 0xc0}, {0x55, 0x55, 0xff}},
  {{0xc0, 0x00, 0xc0}, {0xff, 0x55, 0xff}}, {{0x00, 0xc0, 0xc0}, {0x55, 0xff, 0xff}},
  {{0xc0, 0xc0, 0xc0}, {0xff, 0xff, 0xff}},
};

static double now_seconds(void) {
  return (double)clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1e9;
}

@interface UnderTheCView : ScreenSaverView {
  struct app _app;
  bool _started;
  CGFloat _cellW;
  CGFloat _cellH;
  CGFloat _offX;
  CGFloat _offY;
  NSArray<NSColor *> *_colors;
  NSArray<NSDictionary *> *_attrs;
  UnderTheCConfig *_config;
  CGContextRef _backing;
  NSGraphicsContext *_backingGfx;
}
@end

@implementation UnderTheCView

- (instancetype)initWithFrame:(NSRect)frame isPreview:(BOOL)isPreview {
  self = [super initWithFrame:frame isPreview:isPreview];
  if (self != nil) [self setAnimationTimeInterval:1.0 / 24.0];
  return self;
}

- (void)dealloc {
  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
  if (_started) app_free(&_app);
  if (_backing != NULL) CGContextRelease(_backing);
}

/* macOS 14+ never calls stopAnimation */
- (void)willStop:(NSNotification *)note {
  (void)note;
  if ([self isAnimating]) [self stopAnimation];
}

- (BOOL)isFlipped {
  return YES;
}

- (BOOL)hasConfigureSheet {
  return YES;
}

- (NSWindow *)configureSheet {
  _config = [[UnderTheCConfig alloc] init];
  return _config.window;
}

- (void)setFrameSize:(NSSize)size {
  [super setFrameSize:size];
  [self layoutCells];
}

- (void)layoutCells {
  NSSize size = self.bounds.size;
  if (size.width <= 0 || size.height <= 0) return;
  CGFloat px = floor(size.height / 45.0);
  CGFloat min_px = [self isPreview] ? MIN_FONT_PX_PREVIEW : MIN_FONT_PX;
  if (px < min_px) px = min_px;
  NSFont *fonts[2];
  fonts[0] = [NSFont fontWithName:@"Menlo-Regular" size:px];
  if (fonts[0] == nil) fonts[0] = [NSFont userFixedPitchFontOfSize:px];
  fonts[1] = [NSFont fontWithName:@"Menlo-Bold" size:px];
  if (fonts[1] == nil) fonts[1] = fonts[0];

  NSMutableArray<NSColor *> *colors = [NSMutableArray arrayWithCapacity:18];
  NSMutableArray<NSDictionary *> *attrs = [NSMutableArray arrayWithCapacity:18];
  for (int c = 0; c < 9; c++) {
    for (int b = 0; b < 2; b++) {
      NSColor *color = [NSColor colorWithSRGBRed:palette_rgb[c][b][0] / 255.0
                                           green:palette_rgb[c][b][1] / 255.0
                                            blue:palette_rgb[c][b][2] / 255.0
                                           alpha:1.0];
      [colors addObject:color];
      [attrs addObject:@{NSFontAttributeName : fonts[b], NSForegroundColorAttributeName : color}];
    }
  }
  _colors = colors;
  _attrs = attrs;

  _cellW = [@"M" sizeWithAttributes:@{NSFontAttributeName : fonts[0]}].width;
  _cellH = ceil(fonts[0].ascender - fonts[0].descender + fonts[0].leading);
  int cols = (int)(size.width / _cellW);
  int rows = (int)(size.height / _cellH);
  if (cols < 1) cols = 1;
  if (rows < 1) rows = 1;
  _offX = floor((size.width - cols * _cellW) / 2);
  _offY = floor((size.height - rows * _cellH) / 2);
  if (_started) app_resize(&_app, cols, rows);
  [self resetBacking:size];
  [self redrawAll];
}

- (void)resetBacking:(NSSize)size {
  if (_backing != NULL) CGContextRelease(_backing);
  _backing = NULL;
  _backingGfx = nil;
  NSSize px = [self convertSizeToBacking:size];
  CGFloat scale = size.width > 0 ? px.width / size.width : 1.0;
  CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  _backing = CGBitmapContextCreate(NULL, (size_t)px.width, (size_t)px.height, 8, 0, space, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
  CGColorSpaceRelease(space);
  if (_backing == NULL) return;
  CGContextTranslateCTM(_backing, 0, px.height);
  CGContextScaleCTM(_backing, scale, -scale);
  _backingGfx = [NSGraphicsContext graphicsContextWithCGContext:_backing flipped:YES];
}

- (void)redrawAll {
  if (_backing == NULL) return;
  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext:_backingGfx];
  [[NSColor blackColor] setFill];
  NSRectFill(self.bounds);
  if (_started && _attrs != nil && _app.canvas.cells != NULL && _app.canvas.width > 0) {
    struct canvas *c = &_app.canvas;
    canvas_diff_invalidate(c);
    if (canvas_diff(c))
      for (int row = 0; row < c->height; row++) [self drawSpan:row lo:0 hi:c->width - 1];
  }
  [NSGraphicsContext restoreGraphicsState];
  [self setNeedsDisplay:YES];
}

- (void)startAnimation {
  [super startAnimation];
  [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                      selector:@selector(willStop:)
                                                          name:@"com.apple.screensaver.willstop"
                                                        object:nil];
  if (_started) return;
  struct config cfg;
  config_init(&cfg);
  char err[256];
  if (!saver_config_load(&cfg, err, sizeof err) || !config_check(&cfg, err, sizeof err)) {
    NSLog(@"underthec: %s", err);
    config_free(&cfg);
    config_init(&cfg);
  }
  config_start(&cfg, &_app, now_seconds());
  _started = true;
  [self setAnimationTimeInterval:1.0 / (double)cfg.fps];
  config_free(&cfg);
  [self layoutCells];
}

- (void)stopAnimation {
  [super stopAnimation];
  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];
  if (!_started) return;
  app_free(&_app);
  _started = false;
}

- (void)animateOneFrame {
  if (!_started) return;
  app_frame(&_app, now_seconds());
  if (_backing == NULL || _attrs == nil) return;
  struct canvas *c = &_app.canvas;
  if (!canvas_diff(c)) return;
  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext:_backingGfx];
  for (int row = 0; row < c->height; row++)
    if (c->row_hi[row] >= c->row_lo[row]) [self drawSpan:row lo:c->row_lo[row] hi:c->row_hi[row]];
  [NSGraphicsContext restoreGraphicsState];
  [self setNeedsDisplay:YES];
}

- (NSColor *)colorFor:(int)col bold:(bool)bold {
  return _colors[(NSUInteger)col * 2 + (bold ? 1 : 0)];
}

/* runs of same fg attr in cols lo..hi */
- (void)drawRow:(int)row lo:(int)lo hi:(int)hi {
  const struct canvas *c = &_app.canvas;
  const struct cell *line = &c->cells[(size_t)row * (size_t)c->width];
  CGFloat y = _offY + row * _cellH;
  char text[RUN_MAX];
  int n = 0;
  CGFloat run_x = _offX;
  const struct cell *run_attr = NULL;
  for (int col = lo; col <= hi + 1; col++) {
    const struct cell *cell = col <= hi ? &line[col] : NULL;
    if (cell != NULL && cell->cont) continue;
    size_t len = cell != NULL ? strlen(cell->glyph) : 0;
    bool same = cell != NULL && run_attr != NULL && run_attr->col == cell->col && run_attr->bold == cell->bold && n + (int)len <= RUN_MAX;
    if (same) {
      memcpy(text + n, cell->glyph, len);
      n += (int)len;
      continue;
    }
    if (run_attr != NULL) {
      NSString *s = [[NSString alloc] initWithBytes:text length:(NSUInteger)n encoding:NSUTF8StringEncoding];
      [s drawAtPoint:NSMakePoint(run_x, y) withAttributes:_attrs[(NSUInteger)run_attr->col * 2 + (run_attr->bold ? 1 : 0)]];
    }
    n = 0;
    run_attr = cell;
    if (cell == NULL) break;
    run_x = _offX + col * _cellW;
    memcpy(text, cell->glyph, len);
    n = (int)len;
  }
}

- (void)drawSpan:(int)row lo:(int)lo hi:(int)hi {
  const struct canvas *c = &_app.canvas;
  const struct cell *line = &c->cells[(size_t)row * (size_t)c->width];
  CGFloat y = _offY + row * _cellH;
  [[NSColor blackColor] setFill];
  NSRectFill(NSMakeRect(_offX + lo * _cellW, y, (hi - lo + 1) * _cellW, _cellH));
  for (int col = lo; col <= hi; col++) {
    if (line[col].bg == COL_DEFAULT) continue;
    [[self colorFor:line[col].bg bold:line[col].bg_bold] setFill];
    NSRectFill(NSMakeRect(_offX + col * _cellW, y, _cellW, _cellH));
  }
  [self drawRow:row lo:lo hi:hi];
}

- (void)drawRect:(NSRect)rect {
  (void)rect;
  CGImageRef img = _backing != NULL ? CGBitmapContextCreateImage(_backing) : NULL;
  if (img == NULL) {
    [[NSColor blackColor] setFill];
    NSRectFill(self.bounds);
    return;
  }
  NSImage *image = [[NSImage alloc] initWithCGImage:img size:self.bounds.size];
  CGImageRelease(img);
  [image drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositingOperationCopy fraction:1.0 respectFlipped:YES hints:nil];
}

@end
