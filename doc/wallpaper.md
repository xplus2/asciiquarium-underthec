# Plasma wallpaper

Under The C as a KDE Plasma 6 wallpaper, selectable for the desktop and for the lock screen.

## Installation

Build it first, see [build.md](build.md), with install prefix `/usr`.

Plasma has to find both parts: the QML module in Qt's QML import directory (the lock screen runs in its own process and only searches Qt's standard locations),
and the wallpaper package in `share/plasma/wallpapers`. Install both system-wide, as an account that may write to those directories:

Either `cmake --install build` or `make install` does the job.

The module goes to the QML directory of your Qt (`qtpaths6 --query QT_INSTALL_QML`).
Override it with `-DUNDERTHEC_QML_MODULEDIR=DIR` or `./configure --qml-moduledir=DIR`.
The package goes to `PREFIX/share/plasma/wallpapers/org.underthec.wallpaper`.
Override it with `-DUNDERTHEC_PLASMA_PACKAGEDIR=DIR` or `./configure --plasma-packagedir=DIR`.

## Manual installation

Run the commands from the CMake build directory (or the unpacked release tarball).
It contains the `plasma_wallpaper` directory with `qml/` and `package/`.
Both parts have to be installed, as an account that may write to the target directories.

The QML module goes to Qt's QML import directory, even if the package is installed for one user only:

```sh
QML_DIR=$(qtpaths6 --query QT_INSTALL_QML)
mkdir -p "$QML_DIR/org/underthec"
install -m 755 plasma_wallpaper/qml/org/underthec/libunderthec_qml.so "$QML_DIR/org/underthec/"
install -m 644 plasma_wallpaper/qml/org/underthec/qmldir "$QML_DIR/org/underthec/"
kpackagetool6 -t Plasma/Wallpaper -g -i plasma_wallpaper/package
```

or by copying it to `/usr/share/plasma/wallpapers/org.underthec.wallpaper`.

When updating, replace the library with `install` (which removes the old file first) and do not overwrite it in place with `cp`:
a running `plasmashell` or screen locker has it loaded and crashes when the file changes under it.
Restart afterwards: `systemctl --user restart plasma-plasmashell`.

## Uninstall

Remove the two installed parts, as an account that may write to those directories:

```sh
rm -r "$(qtpaths6 --query QT_INSTALL_QML)/org/underthec"
rm -r /usr/share/plasma/wallpapers/org.underthec.wallpaper
```

The second path is `PREFIX/share/plasma/wallpapers/org.underthec.wallpaper` of your install prefix.
A package installed with `kpackagetool6` is removed with `kpackagetool6 -t Plasma/Wallpaper [-g] -r org.underthec.wallpaper`.
