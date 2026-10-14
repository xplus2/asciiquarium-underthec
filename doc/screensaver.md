# Screensavers, wallpapers and lock screens


## vlock screen locker plugin
> Linux (virtual) consoles only.

### Build
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

---

## Windows screensaver

Everything above Windows 10 1609 should be happy.

### Build

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

---

## Android Daydream

Minimum Android version: 5.0 (API 21 Lollipop)

The Android TV app (`underthec-<version>-androidtv.apk`) and the phone/tablet app (`underthec-<version>-android.apk`) both contain the Daydream.
On phones and tablets, select "Under The C" in Settings -> Display -> Screen saver.

Some Android TV boxes don't provide a selection in their UI.
So, when you are installing the `.apk` using sideloading anyway, like this:
```
adb connect yourtv
adb install underthec-<version>-androidtv.apk
```
it's just one extra step:
```
adb shell settings put secure screensaver_components org.underthec/.UnderTheCDream
```
The phone/tablet app has the application ID `org.underthec.mobile`:
```
adb shell settings put secure screensaver_components org.underthec.mobile/org.underthec.UnderTheCDream
```

---

## macOS screensaver

We don't set a minimal version.
For arm64, this means 11.0 (Big Sur).
If you build it yourself on Intel, the oldest target of current XCode toolchains is 10.13.

### Installation

Downloaded bundles are quarantined. Clear that before first use:
```
xattr -d -r com.apple.quarantine underthec.saver
```

Double-click `underthec.saver`, or copy it to `~/Library/Screen Savers` (single-user install) or `/Library/Screen Savers` (global).
Then select "Under The C" in System Settings -> Screen Saver.

---

## KDE Plasma wallpaper / lock screen

Under The C as a KDE Plasma 6 wallpaper, selectable for the desktop and for the lock screen.

### Installation

Build it first, see [build.md](build.md), with install prefix `/usr`.
Plasma has to find both parts: the QML module in Qt's QML import directory (the lock screen runs in its own process and only searches Qt's standard locations),
and the wallpaper package in `share/plasma/wallpapers`.

Either `cmake --install build` or `make install` usually automates it.

Overrides:
* QML module: `-DUNDERTHEC_QML_MODULEDIR=DIR` or `./configure --qml-moduledir=DIR`.
* Plasma package: `-DUNDERTHEC_PLASMA_PACKAGEDIR=DIR` or `./configure --plasma-packagedir=DIR`.

### Manual installation

Run the commands from the CMake build directory (or the unpacked release tarball).
It contains the `plasma_wallpaper` directory with `qml/` and `package/`.

```sh
QML_DIR=$(qtpaths6 --query QT_INSTALL_QML)
mkdir -p "$QML_DIR/org/underthec"
install -m 755 plasma_wallpaper/qml/org/underthec/libunderthec_qml.so "$QML_DIR/org/underthec/"
install -m 644 plasma_wallpaper/qml/org/underthec/qmldir "$QML_DIR/org/underthec/"
kpackagetool6 -t Plasma/Wallpaper -g -i plasma_wallpaper/package
```

Restart Plasma after updates: `systemctl --user restart plasma-plasmashell`.

### Uninstall

```sh
rm -r "$(qtpaths6 --query QT_INSTALL_QML)/org/underthec"
rm -r /usr/share/plasma/wallpapers/org.underthec.wallpaper
```

A package installed with `kpackagetool6` is removed by `kpackagetool6 -t Plasma/Wallpaper [-g] -r org.underthec.wallpaper`.

---

## X11 screensaver
> Note: This is an XScreensaver, not to be confused with what Wayland compositors do on current distros.

### Build
Building it requires X11 development headers (`libx11`) and optionally Xft (`libxft`) for anti-aliased text.
The normal build (`cmake -B build && cmake --build build`) also tries to build the xscreensaver.
You can override install paths like this:

```sh
cmake -B build \
    -DUNDERTHEC_XSCREENSAVER_BINDIR=lib/x86_64-linux-gnu/xscreensaver \
    -DUNDERTHEC_XSCREENSAVER_CONFIGDIR=share/xscreensaver/config
```

### Registration
To register it with the xscreensaver daemon, add it to the `programs:` resource, either per-user in `~/.xscreensaver`:

```
programs: \
  ...                                    \n\
  "Under The C" underthec                \n\
  ...
```

or system-wide in the `XScreenSaver` app-defaults file (distro-dependent, maybe `/etc/X11/app-defaults/XScreenSaver`).
