#ifndef UNDERTHEC_SAVER_H
#define UNDERTHEC_SAVER_H

#import <Cocoa/Cocoa.h>
#include <stdbool.h>
#include <stddef.h>

#include "../../config.h"

#define SAVER_MODULE @"org.underthec.screensaver"

/* defaults missing = defaults. false: err */
bool saver_config_load(struct config *cfg, char *err, size_t err_len);

@interface UnderTheCConfig : NSObject
@property(readonly) NSWindow *window;
@end

#endif
