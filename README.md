# Asciiquarium - Under The C

Asciiquarium is an aquarium/sea animation in ASCII art.
This is a C-port of [Asciiquarium v1.1](https://github.com/cmatsuoka/asciiquarium),
see "Credits" below for the original authors.

Here is a [Live Demo](https://xplus2.github.io/asciiquarium-underthec/?m=Press+h+for+help&M=white&n=UnderTheC) of the WebAssembly.
See [doc/wasm.md](doc/wasm.md) for a list of available GET parameters.

Release platforms:
* Linux
  - Terminal: amd64, arm64, armel, armhf, i386, riscv64 (musl, static)
  - [XScreensaver](doc/screensaver.md): amd64, arm64 (glibc 2.39+)
  - [vlock plugin](doc/screensaver.md): amd64, arm64 (glibc 2.39+), armel, armhf, i386, riscv64 (musl)
  - [KDE Plasma wallpaper](doc/screensaver.md) and lock screen: amd64, arm64 (glibc 2.39+)
* Windows (1609+)
  - Terminal: amd64, arm64
  - [Screensaver](doc/screensaver.md): amd64, arm64
* macOS (26+)
  - Terminal: arm64
  - [Screensaver](doc/screensaver.md): arm64
* Android TV, phones and tablets (5+)
  - App: arm64-v8a, armeabi-v7a
  - [Daydream screensaver](doc/screensaver.md): arm64-v8a, armeabi-v7a
* Wear OS (3+)
  - App: arm64-v8a, armeabi-v7a
* DOS (3.2+)
  - Terminal: 386+
* [WebAssembly](doc/wasm.md): [demo](https://xplus2.github.io/asciiquarium-underthec/?n=UnderTheC)
* [EBU Teletext](doc/teletext.md) 
  - text and mosaic, t42 and TS/PES

... or build it yourself on others.

## Build
See [doc/build.md](doc/build.md) for detailed instructions.
It's a walk in the waterpark, but if you prefer pre-built releases by GitHub workflows, look [here](https://github.com/xplus2/asciiquarium-underthec/releases).

### Dependencies

Not really.

Some of the optional release targets have special (platform dependent) needs, mostly build-time only:
* `libx11` for the X11 screensaver, plus `libxft` for nicer fonts.
* `qt6-base-dev`+`qt6-declarative-dev` (Qt 6.4+) for the Plasma wallpaper.
* Emscripten for the WebAssembly.
* A JDK (17+) plus the Android SDK (34+) and NDK (r27+) for the Android apps.
* `mingw-w64` to build Windows stuff on Linux.
* Xcode command line tools for the macOS screensaver.

From time to time, it is recommended to feed the fish.

## Installation

* Terminal binaries (Linux, Windows, macOS, DOS): Just run them.
* Android apps (TV, phones, tablets and Wear OS): Sideloading (enable ADB, `adb connect`, `adb install <filename>.apk`)
* WebAssembly: put `index.html`, `underthec.js` and `underthec.wasm` in the same directory of a webserver.
* Screensavers, lock screens and wallpapers: see [doc/screensaver.md](doc/screensaver.md)
* EBU Teletext: see [doc/teletext.md](doc/teletext.md)

## Usage

```sh
underthec [-c [1.0|1.1]] [-s] [-t] [-p pace] [-u N] [-C 1|2|4|7|8|8-bold|16] [-f N] [-m text|-] [-M color] [-P position] [--time [24|12]] [--time-color color] [-n text] [--no-castle]
          [-a definition] [teletext options...]
underthec {-h|-v}
```

| Short | Long                 | Parameter              | Description                           |
|-------|----------------------|------------------------|---------------------------------------|
| `-a`  | `--aquatic-life`     | `<def>`                | decide what's in (see below)          |
| `-c`  | `--classic`          | `[1.0\|1.1]`           | Asciiquarium 1.0 / 1.1 modes          |
| `-m`  | `--message`          | `<text>`               | background `text`. `-` for stdin      |
| `-M`  | `--message-color`    | `<color>`              | `-m`'s text color (see below)         |
| `-P`  | `--message-position` | `<pos>`                | `-m`'s placement (see below)          |
|       | `--time`             | `[24\|12]`             | clock in the middle (see below)       |
|       | `--time-color`       | `<color>`              | `--time` color (default: blue)        |
| `-p`  | `--pace`             | `<pace>`               | speed, 0.01-10 (default: 1)           |
| `-u`  | `--uturn-chance`     | `<N>`                  | fish turn chance (default: 1 in 400)  |
| `-C`  | `--colors`           | `<1\|2\|4\|7\|8\|16>`  | fg colors (default: 16)               |
| `-f`  | `--fps`              | `<N>`                  | render fps, 1-240 (default: 30)       |
| `-s`  | `--screensaver`      |                        | (terminal) exit on any keypress       |
| `-t`  | `--transparent`      |                        | transparent background                |
|       | `--teletext`         | `<t42\|ts>`            | Teletext to stdout (see below)        |
|       | `--teletext-mode`    | `<text\|mosaic>`       | teletext glyphs, default: text        |
|       | `--mcast`            | `<GROUP:PORT>`         | MPEG-TS teletext multicast group      |
|       | `--ttl`              | `<N>`                  | multicast TTL (default: 1)            |
|       | `--iface`            | `<if>`                 | multicast interface                   |
|       | `--teletext-caption` | `<text>`               | teletext caption (default: UNDERTHEC) |
| `-n`  | `--castle-name`      | `<text>`               | text on the castle, max 11 chars      |
|       | `--no-castle`        |                        | disable the castle                    |
| `-h`  | `--help`             |                        | show usage                            |
| `-v`  | `--version`          |                        | show version                          |

### Classic mode `-c`/`--classic`
- `-c 1.0` (or bare `-c`): original 1.0 fish/monster look
- `-c 1.1`: upstream 1.1 fish/monster look

Both use the original lineup (ship, whale, monster, bigfish, shark) and the original shark and castle.
Fixed at 10 fps. 

Not combinable with `-a`, `-f`, `-u`, `-m`, `-M`, `-P`, `-n`, `--time`, `--time-color`.
Environment variables and saved settings in conflict with classic mode are ignored.

### Foreground colors `-C`|`--colors`
Limit the foreground colors.
* 1: just white
* 2: white and gray (ANSI "bold black")
* 4: RGBW (red, green, blue, white)
* 7: same as for teletext
* 8: 8 colors, no bold
* 16: ANSI (default)

`1` and `2` can optionally be suffixed by `-<colorname>` to differ from white (like `-C 2-green`).
`8` can be suffixed by `-bold` to use the 8 bold colors instead.

### Message color `-M`|`--message-color`
Valid colors: `red`, `green`, `blue`, `yellow`, `magenta`, `cyan`, `white`, `black`.

Capitalized first letter: bold.

### Message position `-P`|`--message-position`
| Value     | Placement                                              |
|-----------|--------------------------------------------------------|
| `middle`  | horizontally+vertically centered (default)             |
| `center`  | horizontally centered, vertical top                    |
| `marquee` | vertically centered, scrolls right to left, repeats    |
| `swim`    | top row, scrolls right to left, repeats                |
| `event`   | like `swim`, but takes turns with ducks/swan/ship/etc  |

### Time `--time`
Shows the local time as large ASCII art where `-m` would sit with `-P middle`, behind everything else.
`--time` or `--time 24`: 24 hours, `--time 12`: 12 hours with am/pm. Hours are not padded (`3 : 30 pm`).
The color is set with `--time-color` (like `-M`, default: blue).

Enabling the clock conflicts with an explicit `-P middle`.
Without `-P`, a `-m` message is placed like `-P center`.

### Aquatic life `-a`/`--aquatic-life` 
Define what's going on in your asciiquarium. It takes a comma-separated definition:

- `fish=<N|auto>`: number of fish (default: `auto`, sized to the terminal)
- flags, present=on, omitted=off: `bigfish`, `crab`, `dolphins`, `ducks`, `fishhook`, `jellyfish`,
  `kaiju`, `monster`, `rowers`, `sailboat`, `seahorse`, `shark`, `ship`, `submarine`, `swan`, `swordfish`, 
  `turtle`, `whale`

Default (no `-a`): every flag on, `fish=auto`. Example: `-a fish=10,jellyfish,dolphins`

### Environment variables
Each mirrors a command-line option.
If both an env var and its cmdline option are given, the cmdline option wins.
`UNDERTHEC_FISH` wins over `fish=` in `UNDERTHEC_AQUATIC_LIFE`.

| Variable                                       | Mirrors              |
|------------------------------------------------|----------------------|
| `UNDERTHEC_FISH=auto\|number`                  | `fish=` from `-a`    |
| `UNDERTHEC_AQUATIC_LIFE=<def>`                 | `-a`                 |
| `UNDERTHEC_CLASSIC=1.0\|1.1\|0\|off`           | `-c`                 |
| `UNDERTHEC_MESSAGE=<text>`                     | `-m`                 |
| `UNDERTHEC_MESSAGE_COLOR=<color>`              | `-M`                 |
| `UNDERTHEC_MESSAGE_POSITION=<pos>`             | `-P`                 |
| `UNDERTHEC_TIME=24\|12`                        | `--time`             |
| `UNDERTHEC_TIME_COLOR=<color>`                 | `--time-color`       |
| `UNDERTHEC_PACE=<pace>`                        | `-p`                 |
| `UNDERTHEC_FPS=<N>`                            | `-f`                 |
| `UNDERTHEC_SCREENSAVER=0\|1`                   | `-s`                 |
| `UNDERTHEC_UTURN_CHANCE=<N>`                   | `-u`                 |
| `UNDERTHEC_COLORS=<1\|2\|4\|7\|8\|8-bold\|16>` | `-C`                 |
| `UNDERTHEC_TRANSPARENT=0\|1`                   | `-t`                 |
| `UNDERTHEC_TELETEXT=t42\|ts`                   | `--teletext`         |
| `UNDERTHEC_TELETEXT_MODE=text\|mosaic`         | `--teletext-mode`    |
| `UNDERTHEC_TELETEXT_CAPTION=<text>`            | `--teletext-caption` |
| `UNDERTHEC_MCAST=<GROUP:PORT>`                 | `--mcast`            |
| `UNDERTHEC_MCAST_TTL=<N>`                      | `--ttl`              |
| `UNDERTHEC_MCAST_IFACE=<if>`                   | `--iface`            |
| `UNDERTHEC_CASTLE_NAME=<text>`                 | `-n`                 |
| `UNDERTHEC_NO_CASTLE=0\|1`                     | `--no-castle`        |

### Key bindings

| Key | Alternative | Action                         |
|-----|-------------|--------------------------------|
| `f` |  SIGUSR1    | Feed: drop flakes (not in classic mode) |
| `h` |             | Help                           |
| `p` | `[space]`   | Pause / resume                 |
| `r` |             | Redraw                         |
| `s` |             | Open settings                  |
| `t` |             | Toggle background transparency |
| `v` |             | Cycle text modes (DOS only) |
| `q` | `^C`        | Quit                           |

In the settings dialog, the arrow keys are used to select a setting, 
`+`, `-` or `[space]` make changes.

Android TV:
* `OK` feeds (not in classic mode)
* `up` or holding `OK` opens the settings

Android phones and tablets:
* Touching the screen feeds at that position (not in classic mode)
* Swiping up, left or right opens the settings (start away from the screen edges)
* Battery saver: 10 fps by default, 5 fps while the system battery saver is on

Wear OS:
* Touching the screen feeds at that position (not in classic mode)
* Long press opens a menu (pause, settings)
* The app is not shown in ambient mode

## Credits

* The original asciiquarium program and most of its design are by [Kirk Baucom](https://robobunny.com/projects/asciiquarium/html/)
* A lot of the ASCII art is by [Joan Stark](https://web.archive.org/web/20091027174549/http://www.geocities.com/SoHo/7373/)
* This started as a direct port of [cmatsuoka/asciiquarium](https://github.com/cmatsuoka/asciiquarium)
* Jellyfish is from [nothub/asciiquarium](https://github.com/nothub/asciiquarium) 

## License

It is a ship of Theseus. Anyway, Asciiquarium's original skipper (and crew, see "Credits" above) 
put it under GPL-2.0-or-later, see [LICENSE](LICENSE).

So, GPL-2.0-or-later it is.
