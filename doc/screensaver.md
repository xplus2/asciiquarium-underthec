# Screensaver

## X11 screensaver
> Note: This is an XScreensaver, not to be confused with what Wayland compositors do on current distros.

### Building it
Building it requires X11 development headers (`libx11`) and optionally Xft (`libxft`) for anti-aliased text.
The normal build (`cmake -B build && cmake --build build`) also tries to build the xscreensaver.
You can override install paths like this:

```sh
cmake -B build \
    -DUNDERTHEC_XSCREENSAVER_BINDIR=lib/x86_64-linux-gnu/xscreensaver \
    -DUNDERTHEC_XSCREENSAVER_CONFIGDIR=share/xscreensaver/config
```

### Registering it
To register it with the xscreensaver daemon, add it to the `programs:` resource, either per-user in `~/.xscreensaver`:

```
programs: \
  ...                                    \n\
  "Under The C" underthec                \n\
  ...
```

or system-wide in the `XScreenSaver` app-defaults file (distro-dependent, maybe `/etc/X11/app-defaults/XScreenSaver`).

## Windows screensaver

### Building it

The normal Windows build (from Linux) will also create the `.scr`, as long as `windres`
(that usually already comes with mingw-w64) exists.

```sh
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=toolchain/mingw-w64-toolchain.cmake
cmake --build build-win
```

### Installation

`underthec.scr` is a native Windows multi-monitor screensaver.
Right-click it and choose "Install", or copy it to `%windir%\System32`,
then select it in the screensaver settings.

## vlock screen locker plugin
> Linux (virtual) consoles only.

### Building it
`cmake -B build && cmake --build build` also builds `underthec.so`, a vlock plugin module.
You can override the installation path like this: 
`cmake -B build -DUNDERTHEC_VLOCK_MODULEDIR=/usr/lib/vlock/modules` or 
`./configure --vlock-moduledir=DIR`

### Installation
`cmake --install build` (or `make install`) copies `underthec.so` into
the configured module directory. vlock must have been built with plugin support (enabled on most distros).

### Usage
Run vlock with an idle timeout and the plugin name: `vlock -t 10 underthec`

### Configuration
Environment variables (see [README.md](../README.md) for a list) do the job and can also be set/exported
in `~/.vlockrc`.
