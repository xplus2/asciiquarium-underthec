#import <ScreenSaver/ScreenSaver.h>

#include <string.h>

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
  return (double)CFAbsoluteTimeGetCurrent();
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
}
@end

@implementation UnderTheCView

- (instancetype)initWithFrame:(NSRect)frame isPreview:(BOOL)isPreview {
  self = [super initWithFrame:frame isPreview:isPreview];
  if (self != nil) [self setAnimationTimeInterval:1.0 / 24.0];
  return self;
}

- (void)dealloc {
  if (_started) app_free(&_app);
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
}

- (void)startAnimation {
  [super startAnimation];
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
  if (!_started) return;
  app_free(&_app);
  _started = false;
}

- (void)animateOneFrame {
  if (!_started) return;
  app_frame(&_app, now_seconds());
  [self setNeedsDisplay:YES];
}

- (NSColor *)colorFor:(int)col bold:(bool)bold {
  return _colors[(NSUInteger)col * 2 + (bold ? 1 : 0)];
}

/* runs of same fg attr per row */
- (void)drawRow:(int)row {
  const struct canvas *c = &_app.canvas;
  const struct cell *line = &c->cells[(size_t)row * (size_t)c->width];
  CGFloat y = _offY + row * _cellH;
  char text[RUN_MAX];
  int n = 0;
  CGFloat run_x = _offX;
  const struct cell *run_attr = NULL;
  for (int col = 0; col <= c->width; col++) {
    const struct cell *cell = col < c->width ? &line[col] : NULL;
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

- (void)drawRect:(NSRect)rect {
  (void)rect;
  [[NSColor blackColor] setFill];
  NSRectFill(self.bounds);
  const struct canvas *c = &_app.canvas;
  if (!_started || _attrs == nil || c->cells == NULL || c->width <= 0) return;
  for (int row = 0; row < c->height; row++) {
    for (int col = 0; col < c->width; col++) {
      const struct cell *cell = &c->cells[(size_t)row * (size_t)c->width + (size_t)col];
      if (cell->bg == COL_DEFAULT) continue;
      [[self colorFor:cell->bg bold:cell->bg_bold] setFill];
      NSRectFill(NSMakeRect(_offX + col * _cellW, _offY + row * _cellH, _cellW, _cellH));
    }
  }
  for (int row = 0; row < c->height; row++) [self drawRow:row];
}

@end
