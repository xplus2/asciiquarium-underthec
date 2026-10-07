#import <ScreenSaver/ScreenSaver.h>

#include <stdlib.h>
#include <string.h>

#import "saver.h"
#include "../../opts.h"
#include "../../version.h"

enum saver_opt { OPT_CLASSIC, OPT_AQUATIC, OPT_MESSAGE, OPT_COLOR, OPT_POSITION, OPT_CASTLE_NAME, OPT_NO_CASTLE, OPT_PACE, OPT_UTURN, OPT_FPS, OPT_COLORS, OPT_COUNT };

/* long CLI names */
static const char *const opt_names[OPT_COUNT] = {
  "classic", "aquatic-life", "message", "message-color", "message-position", "castle-name", "no-castle", "pace", "uturn-chance", "fps", "colors",
};

/* sheet names in errors */
static const char *const opt_labels[OPT_COUNT] = {
  "Classic", "Fish", "Message", "Color", "Position", "Castle name", "Castle", "Pace", "U-turn chance", "FPS", "Colors",
};

static const char *const classic_items[] = {"off", "1.0", "1.1"};
static const char *const colors_items[] = {
  "1", "1-green", "1-red", "1-blue", "1-yellow", "1-magenta", "1-cyan", "1-white",
  "2", "2-green", "2-red", "2-blue", "2-yellow", "2-magenta", "2-cyan", "2-white",
  "4", "7", "8", "8-bold", "16",
};
static const char *const colors_labels[] = {
  "1", "1-green", "1-red", "1-blue", "1-yellow", "1-magenta", "1-cyan", "1-white",
  "2", "2-green", "2-red", "2-blue", "2-yellow", "2-magenta", "2-cyan", "2-white",
  "4 (RGBW)", "7 (Teletext)", "8", "8-bold", "16 (ANSI)",
};
static const char *const color_items[] = {
  "(default)", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
  "Black", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White",
};
static const char *const color_labels[] = {
  "(default)", "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white",
  "Black (bold)", "Red (bold)", "Green (bold)", "Yellow (bold)", "Blue (bold)", "Magenta (bold)", "Cyan (bold)", "White (bold)",
};
/* enum message_position order */
static const char *const position_items[] = {"middle", "center", "marquee", "swim", "event"};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define FLAG_COLUMNS 3
#define FIELD_WIDTH 60

static int colors_index(int mode) {
  char eb[128];
  for (size_t i = 0; i < COUNT(colors_items); i++) {
    int parsed;
    if (opts_parse_colors(colors_items[i], &parsed, eb, sizeof eb) && parsed == mode) return (int)i;
  }
  return (int)COUNT(colors_items) - 1;
}

bool saver_config_load(struct config *cfg, char *err, size_t err_len) {
  ScreenSaverDefaults *defaults = [ScreenSaverDefaults defaultsForModuleWithName:SAVER_MODULE];
  for (size_t i = 0; i < OPT_COUNT; i++) {
    NSString *val = [defaults stringForKey:@(opt_names[i])];
    if (val == nil) continue;
    if (!config_set(cfg, opt_names[i], val.UTF8String, opt_names[i], err, err_len)) return false;
  }
  return true;
}

@implementation UnderTheCConfig {
  NSPopUpButton *_classic;
  NSButton *_fishAuto;
  NSTextField *_fish;
  NSMutableArray<NSButton *> *_flags;
  NSTextView *_message;
  NSPopUpButton *_msgColor;
  NSPopUpButton *_msgPos;
  NSButton *_castle;
  NSTextField *_castleName;
  NSTextField *_pace;
  NSTextField *_uturn;
  NSTextField *_fps;
  NSPopUpButton *_colors;
}

- (instancetype)init {
  self = [super init];
  if (self == nil) return nil;
  [self buildWindow];
  struct config cfg;
  config_init(&cfg);
  char err[256];
  if (!saver_config_load(&cfg, err, sizeof err)) {
    NSLog(@"underthec: %s", err);
    config_free(&cfg);
    config_init(&cfg);
  }
  [self loadControls:&cfg];
  config_free(&cfg);
  return self;
}

- (NSPopUpButton *)popupWith:(const char *const *)items count:(size_t)count {
  NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
  for (size_t i = 0; i < count; i++) [popup addItemWithTitle:@(items[i])];
  return popup;
}

- (NSTextField *)field {
  NSTextField *field = [NSTextField textFieldWithString:@""];
  [field.widthAnchor constraintEqualToConstant:FIELD_WIDTH].active = YES;
  return field;
}

- (NSStackView *)rowOf:(NSArray<NSView *> *)views {
  NSStackView *row = [NSStackView stackViewWithViews:views];
  row.orientation = NSUserInterfaceLayoutOrientationHorizontal;
  row.spacing = 8;
  return row;
}

- (NSView *)flagsView {
  _flags = [NSMutableArray array];
  NSMutableArray<NSArray<NSView *> *> *rows = [NSMutableArray array];
  NSMutableArray<NSView *> *row = [NSMutableArray array];
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) {
    NSButton *box = [NSButton checkboxWithTitle:@(scene_aquatic_flag_name(i)) target:nil action:nil];
    [_flags addObject:box];
    [row addObject:box];
    if (row.count == FLAG_COLUMNS) {
      [rows addObject:row];
      row = [NSMutableArray array];
    }
  }
  while (row.count > 0 && row.count < FLAG_COLUMNS) [row addObject:[NSGridCell emptyContentView]];
  if (row.count > 0) [rows addObject:row];
  NSGridView *grid = [NSGridView gridViewWithViews:rows];
  grid.columnSpacing = 12;
  grid.rowSpacing = 4;
  return grid;
}

- (NSView *)messageView {
  NSScrollView *scroll = [[NSScrollView alloc] initWithFrame:NSZeroRect];
  scroll.hasVerticalScroller = YES;
  scroll.borderType = NSBezelBorder;
  _message = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 260, 56)];
  _message.minSize = NSMakeSize(0, 56);
  _message.maxSize = NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX);
  _message.verticallyResizable = YES;
  _message.autoresizingMask = NSViewWidthSizable;
  _message.textContainer.widthTracksTextView = YES;
  _message.richText = NO;
  _message.font = [NSFont fontWithName:@"Menlo-Regular" size:11];
  _message.automaticQuoteSubstitutionEnabled = NO;
  _message.automaticDashSubstitutionEnabled = NO;
  _message.automaticTextReplacementEnabled = NO;
  scroll.documentView = _message;
  [scroll.widthAnchor constraintEqualToConstant:260].active = YES;
  [scroll.heightAnchor constraintEqualToConstant:56].active = YES;
  return scroll;
}

- (NSView *)buttonRow {
  NSButton *defaults = [NSButton buttonWithTitle:@"Defaults" target:self action:@selector(defaults:)];
  NSButton *cancel = [NSButton buttonWithTitle:@"Cancel" target:self action:@selector(cancel:)];
  cancel.keyEquivalent = @"\033";
  NSButton *ok = [NSButton buttonWithTitle:@"OK" target:self action:@selector(ok:)];
  ok.keyEquivalent = @"\r";
  NSTextField *version = [NSTextField labelWithString:@TOOL_DISPLAY_NAME " v" TOOL_VERSION];
  version.textColor = [NSColor secondaryLabelColor];
  NSView *spacer = [[NSView alloc] initWithFrame:NSZeroRect];
  [spacer setContentHuggingPriority:1 forOrientation:NSLayoutConstraintOrientationHorizontal];
  return [self rowOf:@[defaults, version, spacer, cancel, ok]];
}

- (void)buildWindow {
  _classic = [self popupWith:classic_items count:COUNT(classic_items)];
  _classic.target = self;
  _classic.action = @selector(toggled:);
  _fishAuto = [NSButton checkboxWithTitle:@"auto" target:self action:@selector(toggled:)];
  _fish = [self field];
  _msgColor = [self popupWith:color_labels count:COUNT(color_labels)];
  _msgPos = [self popupWith:position_items count:COUNT(position_items)];
  _castle = [NSButton checkboxWithTitle:@"enabled" target:self action:@selector(toggled:)];
  _castleName = [self field];
  _pace = [self field];
  _uturn = [self field];
  _fps = [self field];
  _colors = [self popupWith:colors_labels count:COUNT(colors_labels)];

  NSArray<NSArray<NSView *> *> *rows = @[
    @[[NSTextField labelWithString:@"Classic"], _classic],
    @[[NSTextField labelWithString:@"Fish"], [self rowOf:@[_fishAuto, _fish]]],
    @[[NSTextField labelWithString:@"Creatures"], [self flagsView]],
    @[[NSTextField labelWithString:@"Message"], [self messageView]],
    @[[NSTextField labelWithString:@"Color"], _msgColor],
    @[[NSTextField labelWithString:@"Position"], _msgPos],
    @[[NSTextField labelWithString:@"Castle"], [self rowOf:@[_castle, _castleName]]],
    @[[NSTextField labelWithString:@"Pace"], _pace],
    @[[NSTextField labelWithString:@"U-turn chance"], _uturn],
    @[[NSTextField labelWithString:@"FPS"], _fps],
    @[[NSTextField labelWithString:@"Colors"], _colors],
  ];
  NSGridView *grid = [NSGridView gridViewWithViews:rows];
  grid.rowSpacing = 8;
  grid.columnSpacing = 10;
  [grid columnAtIndex:0].xPlacement = NSGridCellPlacementTrailing;
  [grid columnAtIndex:1].xPlacement = NSGridCellPlacementLeading;

  NSStackView *content = [NSStackView stackViewWithViews:@[grid, [self buttonRow]]];
  content.orientation = NSUserInterfaceLayoutOrientationVertical;
  content.alignment = NSLayoutAttributeLeading;
  content.spacing = 16;
  content.edgeInsets = NSEdgeInsetsMake(16, 16, 16, 16);

  _window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 400, 400)
                                        styleMask:NSWindowStyleMaskTitled
                                          backing:NSBackingStoreBuffered
                                            defer:NO];
  _window.title = @TOOL_DISPLAY_NAME;
  _window.contentView = content;
  [content layoutSubtreeIfNeeded];
  [_window setContentSize:content.fittingSize];
}

- (void)setText:(NSTextField *)field to:(const char *)s {
  field.stringValue = @(s != NULL ? s : "");
}

- (void)loadControls:(struct config *)cfg {
  [_classic selectItemAtIndex:cfg->classic_ver];
  bool fishAuto = cfg->aquatic.fish_count < 0;
  _fishAuto.state = fishAuto ? NSControlStateValueOn : NSControlStateValueOff;
  _fish.stringValue = fishAuto ? @"" : [NSString stringWithFormat:@"%d", cfg->aquatic.fish_count];
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++)
    _flags[i].state = *scene_aquatic_flag(&cfg->aquatic, i) ? NSControlStateValueOn : NSControlStateValueOff;
  _message.string = @(cfg->message != NULL ? cfg->message : "");
  NSInteger color = 0;
  for (size_t i = 1; cfg->message_color != NULL && i < COUNT(color_items); i++)
    if (strcmp(cfg->message_color, color_items[i]) == 0) color = (NSInteger)i;
  [_msgColor selectItemAtIndex:color];
  [_msgPos selectItemAtIndex:cfg->message_position];
  _castle.state = cfg->no_castle ? NSControlStateValueOff : NSControlStateValueOn;
  [self setText:_castleName to:cfg->castle_name];
  _pace.stringValue = [NSString stringWithFormat:@"%g", cfg->pace];
  _uturn.stringValue = [NSString stringWithFormat:@"%d", cfg->uturn_chance];
  _fps.stringValue = [NSString stringWithFormat:@"%d", cfg->fps];
  [_colors selectItemAtIndex:colors_index(cfg->colors_mode)];
  [self updateEnabled];
}

/* classic fixes all but castle, pace and colors */
- (void)updateEnabled {
  bool freeLife = _classic.indexOfSelectedItem == 0;
  _fishAuto.enabled = freeLife;
  _fish.enabled = freeLife && _fishAuto.state != NSControlStateValueOn;
  for (NSButton *box in _flags) box.enabled = freeLife;
  _message.editable = freeLife;
  _message.selectable = freeLife;
  _message.textColor = freeLife ? [NSColor textColor] : [NSColor disabledControlTextColor];
  _msgColor.enabled = freeLife;
  _msgPos.enabled = freeLife;
  _uturn.enabled = freeLife;
  _fps.enabled = freeLife;
  _castleName.enabled = freeLife && _castle.state == NSControlStateValueOn;
}

- (void)toggled:(id)sender {
  (void)sender;
  [self updateEnabled];
}

- (NSString *)aquaticDefinition {
  NSMutableString *def = [NSMutableString stringWithString:@"fish="];
  [def appendString:_fishAuto.state == NSControlStateValueOn ? @"auto" : _fish.stringValue];
  for (size_t i = 0; i < SCENE_AQUATIC_FLAG_COUNT; i++) {
    if (_flags[i].state != NSControlStateValueOn) continue;
    [def appendFormat:@",%s", scene_aquatic_flag_name(i)];
  }
  return def;
}

/* NSNull: option not stored */
- (NSArray<id> *)collect {
  NSMutableArray<id> *vals = [NSMutableArray arrayWithCapacity:OPT_COUNT];
  for (size_t i = 0; i < OPT_COUNT; i++) [vals addObject:[NSNull null]];
  NSInteger classic = _classic.indexOfSelectedItem;
  if (classic > 0) vals[OPT_CLASSIC] = @(classic_items[classic]);
  else vals[OPT_AQUATIC] = [self aquaticDefinition];
  NSString *message = _message.string;
  if (message.length > 0) vals[OPT_MESSAGE] = message;
  NSInteger color = _msgColor.indexOfSelectedItem;
  if (color > 0) vals[OPT_COLOR] = @(color_items[color]);
  vals[OPT_POSITION] = @(position_items[_msgPos.indexOfSelectedItem]);
  if (_castleName.stringValue.length > 0) vals[OPT_CASTLE_NAME] = _castleName.stringValue;
  vals[OPT_NO_CASTLE] = _castle.state == NSControlStateValueOn ? @"0" : @"1";
  vals[OPT_PACE] = _pace.stringValue;
  vals[OPT_UTURN] = _uturn.stringValue;
  vals[OPT_FPS] = _fps.stringValue;
  vals[OPT_COLORS] = @(colors_items[_colors.indexOfSelectedItem]);
  return vals;
}

- (void)warn:(NSString *)text {
  NSAlert *alert = [[NSAlert alloc] init];
  alert.alertStyle = NSAlertStyleWarning;
  alert.messageText = @TOOL_DISPLAY_NAME;
  alert.informativeText = text;
  [alert beginSheetModalForWindow:_window completionHandler:nil];
}

- (void)close {
  NSWindow *parent = _window.sheetParent;
  if (parent != nil) [parent endSheet:_window];
  else [_window close];
}

- (void)ok:(id)sender {
  (void)sender;
  NSArray<id> *vals = [self collect];
  char err[256];
  struct config cfg;
  config_init(&cfg);
  bool ok = true;
  for (size_t i = 0; i < OPT_COUNT && ok; i++) {
    if (![vals[i] isKindOfClass:[NSString class]]) continue;
    ok = config_set(&cfg, opt_names[i], [(NSString *)vals[i] UTF8String], opt_labels[i], err, sizeof err);
  }
  if (ok) ok = config_check(&cfg, err, sizeof err);
  config_free(&cfg);
  if (!ok) {
    [self warn:@(err)];
    return;
  }
  ScreenSaverDefaults *defaults = [ScreenSaverDefaults defaultsForModuleWithName:SAVER_MODULE];
  for (size_t i = 0; i < OPT_COUNT; i++) {
    if ([vals[i] isKindOfClass:[NSString class]]) [defaults setObject:vals[i] forKey:@(opt_names[i])];
    else [defaults removeObjectForKey:@(opt_names[i])];
  }
  [defaults synchronize];
  [self close];
}

- (void)cancel:(id)sender {
  (void)sender;
  [self close];
}

- (void)defaults:(id)sender {
  (void)sender;
  struct config cfg;
  config_init(&cfg);
  [self loadControls:&cfg];
  config_free(&cfg);
}

@end
