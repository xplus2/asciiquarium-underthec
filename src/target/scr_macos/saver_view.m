#import <ScreenSaver/ScreenSaver.h>

#import "saver.h"
#include "../../app.h"
#include "../../color.h"
#include "../../monotime.h"

#define RUN_MAX 512
#define MIN_FONT_PX 10
#define MIN_FONT_PX_PREVIEW 5

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
      uint32_t rgb = color_rgb((enum color)c, b);
      NSColor *color = [NSColor colorWithSRGBRed:((rgb >> 16) & 0xff) / 255.0
                                           green:((rgb >> 8) & 0xff) / 255.0
                                            blue:(rgb & 0xff) / 255.0
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
  config_start(&cfg, &_app, monotime_now());
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
  app_frame(&_app, monotime_now());
  if (_backing == NULL || _attrs == nil) return;
  struct canvas *c = &_app.canvas;
  if (!canvas_diff(c)) return;
  [NSGraphicsContext saveGraphicsState];
  [NSGraphicsContext setCurrentContext:_backingGfx];
  for (int row = 0; row < c->height; row++)
    if (c->row_hi[row] >= c->row_lo[row]) [self drawSpan:row lo:c->row_lo[row] hi:c->row_hi[row]];
  [NSGraphicsContext restoreGraphicsState];
  for (int row = 0; row < c->height; row++) {
    if (c->row_hi[row] < c->row_lo[row]) continue;
    [self setNeedsDisplayInRect:NSMakeRect(_offX + c->row_lo[row] * _cellW, _offY + row * _cellH, (c->row_hi[row] - c->row_lo[row] + 1) * _cellW, _cellH)];
  }
}

- (NSColor *)colorFor:(int)col bold:(bool)bold {
  return _colors[(NSUInteger)col * 2 + (bold ? 1 : 0)];
}

- (void)drawRunRow:(int)row col0:(int)col0 col1:(int)col1 attr:(const struct cell *)attr {
  char text[RUN_MAX];
  int n = canvas_run_utf8(&_app.canvas, row, col0, col1, text, RUN_MAX);
  NSString *s = [[NSString alloc] initWithBytes:text length:(NSUInteger)n encoding:NSUTF8StringEncoding];
  [s drawAtPoint:NSMakePoint(_offX + col0 * _cellW, _offY + row * _cellH) withAttributes:_attrs[(NSUInteger)attr->col * 2 + (attr->bold ? 1 : 0)]];
}

static void draw_run(void *ctx, int row, int col0, int col1, const struct cell *attr) {
  [(__bridge UnderTheCView *)ctx drawRunRow:row col0:col0 col1:col1 attr:attr];
}

- (void)drawRow:(int)row lo:(int)lo hi:(int)hi {
  canvas_row_runs(&_app.canvas, row, lo, hi, RUN_MAX / 4, draw_run, (__bridge void *)self);
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
