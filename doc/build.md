# Building Under The C

## CMake (primary)

```sh
cmake -B build
cmake --build build
```

The `Release` build type (the default) links statically wherever the target platform allows it, and strips the resulting binary.

Static musl build on Linux (requires `musl-gcc`):
```sh
cmake -B build-musl -DCMAKE_TOOLCHAIN_FILE=toolchain/musl-toolchain.cmake
cmake --build build-musl
```

If `libx11` and `libxft` headers are installed, this also builds the X11 screensaver (non-static).

If the Qt6 Quick development files (Qt 6.4 or newer, e.g. `qt6-base-dev` and `qt6-declarative-dev`) and a C++ compiler are installed,
this also builds the Plasma wallpaper plugin (non-static, target `plasma_wallpaper`, output in `build/plasma_wallpaper/`).
`-DUNDERTHEC_PLASMA_WALLPAPER=OFF` disables it. See [wallpaper.md](wallpaper.md) for installation.

### Windows
This sections describes cross-compiling static Windows builds from Linux using the `mingw-w64` cross toolchain.
They also produce the screensaver `underthec.scr` (needs `windres` that usually already comes with mingw-w64).

```sh
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=toolchain/mingw-w64-toolchain.cmake
cmake --build build-win
```
With configure + make, `--host=*-mingw32` builds `build/underthec.scr`.

> Building natively on Windows: Technically possible.

### macOS
Same as on Linux:
```sh
cmake -B build
cmake --build build
```

One small difference: Apple's libSystem does not support fully static binaries.

It also builds the screensaver bundle `underthec.saver` (Objective-C, needs Xcode command line tools).
`-DUNDERTHEC_MACOS_SAVER=OFF` disables it. See [screensaver.md](screensaver.md) for installation.

### Android TV

#### NDK Part
The native library needs the Android NDK (r27 and r29 tested). Android 5.0 (API 21) is the minimum.

```sh
# -DANDROID_ABI=armeabi-v7a for 32 bit
cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-21
cmake --build build-android
```
Result is `build-android/libunderthec_android.so`.

#### SDK Part
Needs a JDK 17 or newer, the Android SDK (platform 34, build-tools 34.0.0, CMake 3.22.1) and an NDK.
The native library is built from the top level `CMakeLists.txt`.

Create `src/target/androidtv/local.properties` with `sdk.dir=` pointing to the SDK,
and `cmake.dir=` pointing to a CMake install if the SDK has none.

The APK is built by Gradle from `src/target/androidtv`, which runs this CMake build for both ABIs.
It needs a JDK 17 or newer, SDK platform 34, build-tools 34.0.0 and CMake 3.22.1:

```sh
cd src/target/androidtv
./gradlew assembleDebug -PUNDERTHEC_NDK=$NDK
```

`UNDERTHEC_NDK` (or `ANDROID_NDK_ROOT`) selects the NDK.
The APK ends up in `build/outputs/apk/debug/underthec-VERSION-androidtv-debug.apk`.
`assembleRelease` builds the release APK next to it. It is signed when these Gradle properties are set
(for example in `~/.gradle/gradle.properties`), and unsigned otherwise:
* `ANDROID_KEYSTORE_BASE64`: the keystore file, base64 encoded on one line.
* `ANDROID_KEYSTORE_PASSWORD`
* `ANDROID_KEY_ALIAS`
* `ANDROID_KEY_PASSWORD`


### WebAssembly
Building the WebAssembly requires emscripten (`emcc`, `emcmake`) and the output will be `underthec.js`, `underthec.wasm`
and `index.html` in the build directory.

```sh
emcmake cmake -B build-web
cmake --build build-web
```
or `./configure --emcc && make`

Browsers don't load wasm from `file://`, so serve the directory over HTTP.

---
## configure + make alternative
Nothing unexpected:
```sh
./configure
make
make install
```

`./configure --help` lists the available options.

It builds the Plasma wallpaper plugin if `pkg-config` finds Qt6 Quick 6.4 or newer, a C++ compiler and Qt's `moc`
(`--qml-moduledir` and `--plasma-packagedir` set the installation locations, see [wallpaper.md](wallpaper.md)).

Pass `--debug` for an unstripped `-g -O0` build.

### The Android exception
While you can build the NDK part traditionally, like `./configure --android-ndk=DIR [--android-abi=ABI] [--android-api=N]` for `build/libunderthec_android.so`,
the APK needs the Gradle project - which is a CMake thing.
