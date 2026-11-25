TARGET := underthec
BUILDDIR := build

-include config.mk

# clean-only runs skip config checks
ifneq ($(if $(MAKECMDGOALS),$(filter-out clean,$(MAKECMDGOALS)),all),)
ifndef SRCS
$(error config.mk not found - run ./configure first)
endif
ifneq ($(shell test configure -nt config.mk && echo stale),)
$(error config.mk older than configure - rerun ./configure)
endif
endif

OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))
RC_OBJS := $(patsubst %.rc,$(BUILDDIR)/%.res.o,$(RC))
SCR_OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(SCR_SRCS)) $(patsubst %.rc,$(BUILDDIR)/%.res.o,$(SCR_RC))
XSCR_OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(XSCR_SRCS))
# object dir, needs -fPIC
VLOCK_OBJS := $(patsubst %.c,$(BUILDDIR)/vlock/%.o,$(VLOCK_SRCS))
# own object dir, no lto
SAVER_OBJS := $(patsubst %.c,$(BUILDDIR)/saver/%.o,$(filter %.c,$(SAVER_SRCS))) $(patsubst %.m,$(BUILDDIR)/saver/%.o,$(filter %.m,$(SAVER_SRCS)))
SAVER_PLIST := $(BUILDDIR)/underthec.saver/Contents/Info.plist
SAVER_THUMBS := $(BUILDDIR)/underthec.saver/Contents/Resources/thumbnail.png $(BUILDDIR)/underthec.saver/Contents/Resources/thumbnail@2x.png
# own object dir, needs -fPIC, no lto
PLASMA_OBJDIR := $(BUILDDIR)/plasma
PLASMA_OBJS := $(patsubst %.c,$(PLASMA_OBJDIR)/%.o,$(PLASMA_C_SRCS)) $(patsubst %.cpp,$(PLASMA_OBJDIR)/%.o,$(PLASMA_CXX_SRCS)) $(PLASMA_OBJDIR)/moc_plasma_item.o
PLASMA_FILES := $(BUILDDIR)/plasma_wallpaper/qml/org/underthec/qmldir $(BUILDDIR)/plasma_wallpaper/package/metadata.json
DEPS := $(sort $(patsubst %.o,%.d,$(filter-out %.res.o,$(OBJS) $(SCR_OBJS) $(XSCR_OBJS) $(VLOCK_OBJS) $(SAVER_OBJS) $(PLASMA_OBJS))))
NOLTO_CFLAGS = $(filter-out -flto=auto,$(CFLAGS))

# $(1): compiler + flags
define compile
	@mkdir -p $(dir $@)
	$(1) -MMD -MP -c $< -o $@
endef

define copy
	@mkdir -p $(dir $@)
	cp $< $@
endef

.PHONY: all clean install

all: $(TARGET) $(WEB_FILES) $(SCR_TARGET) $(XSCR_TARGET) $(VLOCK_TARGET) $(SAVER_TARGET) $(if $(SAVER_TARGET),$(SAVER_PLIST) $(SAVER_THUMBS)) $(PLASMA_TARGET) $(if $(PLASMA_TARGET),$(PLASMA_FILES))

$(TARGET): $(OBJS) $(RC_OBJS)
	$(CC) $(OBJS) $(RC_OBJS) $(LDFLAGS) $(TERM_LDFLAGS) -o $@

$(SCR_TARGET): $(SCR_OBJS)
	$(CC) $(SCR_OBJS) $(LDFLAGS) $(SCR_LDFLAGS) -o $@

# X11/Xft/fontconfig .a pull in unresolved transitive deps -> sorry, not static
$(XSCR_TARGET): $(XSCR_OBJS)
	$(CC) $(XSCR_OBJS) $(filter-out -static,$(LDFLAGS)) $(XSCR_LDFLAGS) -o $@

$(VLOCK_TARGET): $(VLOCK_OBJS)
	$(CC) -shared $(VLOCK_OBJS) $(filter-out -static,$(LDFLAGS)) -o $@

ifdef SAVER_TARGET
$(SAVER_TARGET): $(SAVER_OBJS)
	@mkdir -p $(dir $@)
	$(CC) -bundle $(SAVER_OBJS) $(filter-out -static -flto=auto,$(LDFLAGS)) -framework Cocoa -framework ScreenSaver -o $@

$(SAVER_PLIST): src/target/scr_macos/Info.plist.in src/version.h
	@mkdir -p $(dir $@)
	sed 's/@UNDERTHEC_VERSION@/$(SAVER_VERSION)/' $< > $@

$(BUILDDIR)/underthec.saver/Contents/Resources/%: src/target/scr_macos/%
	$(copy)

$(BUILDDIR)/saver/%.o: %.c
	$(call compile,$(CC) $(NOLTO_CFLAGS))

$(BUILDDIR)/saver/%.o: %.m
	$(call compile,$(CC) $(patsubst -std=c11,-std=gnu11,$(NOLTO_CFLAGS)) -fobjc-arc)
endif

ifdef PLASMA_TARGET
$(PLASMA_TARGET): $(PLASMA_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) -shared $(PLASMA_OBJS) $(filter-out -static -flto=auto,$(LDFLAGS)) $(PLASMA_LDFLAGS) -o $@

$(PLASMA_OBJDIR)/%.o: %.c
	$(call compile,$(CC) $(NOLTO_CFLAGS) -fPIC)

$(PLASMA_OBJDIR)/%.o: %.cpp $(PLASMA_OBJDIR)/plasma_plugin.moc
	$(call compile,$(CXX) $(PLASMA_CXXFLAGS) -fPIC -I$(PLASMA_OBJDIR))

$(PLASMA_OBJDIR)/moc_plasma_item.o: $(PLASMA_OBJDIR)/moc_plasma_item.cpp
	$(call compile,$(CXX) $(PLASMA_CXXFLAGS) -fPIC)

$(PLASMA_OBJDIR)/moc_plasma_item.cpp: src/target/plasma/plasma_item.h
	@mkdir -p $(dir $@)
	$(MOC) $(PLASMA_INCS) $< -o $@

$(PLASMA_OBJDIR)/plasma_plugin.moc: src/target/plasma/plasma_plugin.cpp
	@mkdir -p $(dir $@)
	$(MOC) $(PLASMA_INCS) $< -o $@

$(BUILDDIR)/plasma_wallpaper/qml/org/underthec/qmldir: src/target/plasma/qmldir
	$(copy)

$(BUILDDIR)/plasma_wallpaper/package/metadata.json: $(shell find src/target/plasma/package -type f)
	@mkdir -p $(dir $@)
	cp -R src/target/plasma/package/. $(dir $@)
endif

$(BUILDDIR)/%.o: %.c
	$(call compile,$(CC) $(CFLAGS))

$(BUILDDIR)/src/target/scr_x11/%.o: CFLAGS += $(XSCR_CFLAGS)

$(BUILDDIR)/vlock/%.o: %.c
	$(call compile,$(CC) $(CFLAGS) -fPIC)

$(BUILDDIR)/%.res.o: %.rc src/version.h src/target/scr_windows/scr_res.h src/target/scr_windows/scr.manifest src/target/windows/underthec.ico src/target/windows/scr.ico
	@mkdir -p $(dir $@)
	$(WINDRES) -Isrc -O coff $< -o $@

$(BUILDDIR)/index.html $(BUILDDIR)/apple-touch-icon.png: $(BUILDDIR)/%: src/target/web/%
	$(copy)

$(BUILDDIR)/favicon.ico: src/target/windows/underthec.ico
	$(copy)

-include $(DEPS)

clean:
	rm -rf $(BUILDDIR) $(TARGET)
	rm -rf src/target/android/.gradle src/target/androidtv/.gradle src/target/androidwear/.gradle src/target/android/lib/build src/target/android/lib/.cxx src/target/android/mobile/build src/target/androidtv/build src/target/androidwear/build

install: $(TARGET)
	@if [ -n "$(WEB_FILES)" ]; then echo "install: not supported for the web build" >&2; exit 1; fi
	@if [ -n "$(ANDROID)" ]; then echo "install: not supported for the android build" >&2; exit 1; fi
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@if [ -n "$(SCR_TARGET)" ]; then install -m 755 $(SCR_TARGET) $(DESTDIR)$(PREFIX)/bin/underthec.scr; fi
	@if [ -n "$(XSCR_TARGET)" ]; then \
		install -d $(DESTDIR)$(XSCR_BINDIR); \
		install -m 755 $(XSCR_TARGET) $(DESTDIR)$(XSCR_BINDIR)/underthec; \
		install -d $(DESTDIR)$(XSCR_CONFIGDIR); \
		install -m 644 src/target/scr_x11/underthec.xml $(DESTDIR)$(XSCR_CONFIGDIR)/underthec.xml; \
	fi
	@if [ -n "$(VLOCK_TARGET)" ]; then \
		install -d $(DESTDIR)$(VLOCK_MODULEDIR); \
		install -m 755 $(VLOCK_TARGET) $(DESTDIR)$(VLOCK_MODULEDIR)/underthec.so; \
	fi
	@if [ -n "$(SAVER_TARGET)" ]; then \
		install -d "$(DESTDIR)$(SAVER_DIR)/underthec.saver/Contents/MacOS"; \
		install -m 755 $(SAVER_TARGET) "$(DESTDIR)$(SAVER_DIR)/underthec.saver/Contents/MacOS/underthec"; \
		install -m 644 $(SAVER_PLIST) "$(DESTDIR)$(SAVER_DIR)/underthec.saver/Contents/Info.plist"; \
		install -d "$(DESTDIR)$(SAVER_DIR)/underthec.saver/Contents/Resources"; \
		install -m 644 $(SAVER_THUMBS) "$(DESTDIR)$(SAVER_DIR)/underthec.saver/Contents/Resources/"; \
	fi
	@if [ -n "$(PLASMA_TARGET)" ]; then \
		install -d $(DESTDIR)$(PLASMA_QMLDIR)/org/underthec; \
		install -m 755 $(PLASMA_TARGET) $(DESTDIR)$(PLASMA_QMLDIR)/org/underthec/libunderthec_qml.so; \
		install -m 644 src/target/plasma/qmldir $(DESTDIR)$(PLASMA_QMLDIR)/org/underthec/qmldir; \
		cd src/target/plasma/package && find . -type d -exec install -d $(DESTDIR)$(PLASMA_PACKAGEDIR)/{} \; && \
		find . -type f -exec install -m 644 {} $(DESTDIR)$(PLASMA_PACKAGEDIR)/{} \; ; \
	fi
