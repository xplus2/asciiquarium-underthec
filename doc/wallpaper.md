# Plasma wallpaper

Under The C as a KDE Plasma 6 wallpaper, selectable for the desktop and for the lock screen.

## Installation

Build it first, see [build.md](build.md), with install prefix `/usr`.

Plasma has to find both parts: the QML module in Qt's QML import directory (the lock screen runs in its own process and only searches Qt's standard locations),
and the wallpaper package in `share/plasma/wallpapers`.

Either `cmake --install build` or `make install` usually automates it.

Overrides:
* QML module: `-DUNDERTHEC_QML_MODULEDIR=DIR` or `./configure --qml-moduledir=DIR`.
* Plasma package: `-DUNDERTHEC_PLASMA_PACKAGEDIR=DIR` or `./configure --plasma-packagedir=DIR`.

## Manual installation

Run the commands from the CMake build directory (or the unpacked release tarball).
It contains the `plasma_wallpaper` directory with `qml/` and `package/`.

```sh
QML_DIR=$(qtpaths6 --query QT_INSTALL_QML)
mkdir -p "$QML_DIR/org/underthec"
install -m 755 plasma_wallpaper/qml/org/underthec/libunderthec_qml.so "$QML_DIR/org/underthec/"
install -m 644 plasma_wallpaper/qml/org/underthec/qmldir "$QML_DIR/org/underthec/"
kpackagetool6 -t Plasma/Wallpaper -g -i plasma_wallpaper/package
```

Restart after updates: `systemctl --user restart plasma-plasmashell`.

## Uninstall

```sh
rm -r "$(qtpaths6 --query QT_INSTALL_QML)/org/underthec"
rm -r /usr/share/plasma/wallpapers/org.underthec.wallpaper
```

A package installed with `kpackagetool6` is removed by `kpackagetool6 -t Plasma/Wallpaper [-g] -r org.underthec.wallpaper`.
