TARGET := underthec
BUILDDIR := build

-include config.mk

ifndef SRCS
$(error config.mk not found - run ./configure first)
endif

OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))
RC_OBJS := $(patsubst %.rc,$(BUILDDIR)/%.res.o,$(RC))
SCR_OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(SCR_SRCS)) $(patsubst %.rc,$(BUILDDIR)/%.res.o,$(SCR_RC))
XSCR_OBJS := $(patsubst %.c,$(BUILDDIR)/%.o,$(XSCR_SRCS))
# object dir, needs -fPIC
VLOCK_OBJS := $(patsubst %.c,$(BUILDDIR)/vlock/%.o,$(VLOCK_SRCS))
DEPS := $(sort $(OBJS:.o=.d) $(patsubst %.c,$(BUILDDIR)/%.d,$(SCR_SRCS)) $(patsubst %.c,$(BUILDDIR)/%.d,$(XSCR_SRCS)) $(patsubst %.c,$(BUILDDIR)/vlock/%.d,$(VLOCK_SRCS)))

.PHONY: all clean install

all: $(TARGET) $(WEB_FILES) $(SCR_TARGET) $(XSCR_TARGET) $(VLOCK_TARGET)

$(TARGET): $(OBJS) $(RC_OBJS)
	$(CC) $(OBJS) $(RC_OBJS) $(LDFLAGS) $(TERM_LDFLAGS) -o $@

$(SCR_TARGET): $(SCR_OBJS)
	$(CC) $(SCR_OBJS) $(LDFLAGS) $(SCR_LDFLAGS) -o $@

# X11/Xft/fontconfig .a pull in unresolved transitive deps -> sorry, not static
$(XSCR_TARGET): $(XSCR_OBJS)
	$(CC) $(XSCR_OBJS) $(filter-out -static,$(LDFLAGS)) $(XSCR_LDFLAGS) -o $@

$(VLOCK_TARGET): $(VLOCK_OBJS)
	$(CC) -shared $(VLOCK_OBJS) $(filter-out -static,$(LDFLAGS)) -o $@

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILDDIR)/vlock/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -MMD -MP -c $< -o $@

$(BUILDDIR)/%.res.o: %.rc src/version.h src/target/scr_windows/scr_res.h src/target/scr_windows/scr.manifest src/target/windows/underthec.ico src/target/windows/scr.ico
	@mkdir -p $(dir $@)
	$(WINDRES) -Isrc -O coff $< -o $@

$(BUILDDIR)/index.html: src/target/web/index.html
	@mkdir -p $(dir $@)
	cp $< $@

$(BUILDDIR)/apple-touch-icon.png: src/target/web/apple-touch-icon.png
	@mkdir -p $(dir $@)
	cp $< $@

$(BUILDDIR)/favicon.ico: src/target/windows/underthec.ico
	@mkdir -p $(dir $@)
	cp $< $@

-include $(DEPS)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

install: $(TARGET)
	@if [ -n "$(WEB_FILES)" ]; then echo "install: not supported for the web build" >&2; exit 1; fi
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
