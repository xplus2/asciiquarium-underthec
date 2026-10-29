# WebAssembly

Keys like `h` for help, `f` to feed or `s` for settings work. Click or tap also drops some flakes.

## Query params
Options can go in the URL query, by long or short name (long wins if both are set),
with the same values as on the command line:

| long               | short | example               |
|--------------------|-------|-----------------------|
| `classic`          | `c`   | `?c=1.1`              |
| `aquatic-life`     | `a`   | `?a=fish=5,ducks`     |
| `message`          | `m`   | `?m=HowMuchIsTheFish` |
| `message-color`    | `M`   | `?M=Yellow`           |
| `message-position` | `P`   | `?P=middle`           |
| `time`             |       | `?time=12`            |
| `time-color`       |       | `?time-color=red`     |
| `pace`             | `p`   | `?p=2`                |
| `uturn-chance`     | `u`   | `?u=50`               |
| `colors`           | `C`   | `?colors=16`          |
| `fps`              | `f`   | `?f=30`               |
| `castle-name`      | `n`   | `?n=MyCastle`         |
| `no-castle`        |       | `?no-castle`          |

An invalid value shows the error instead of the aquarium.
With `classic`, conflicting params (`a`, `m`, `M`, `P`, `u`, `f`, `n`, `time`, `time-color`) are ignored.

> `&m=` is not necessarily limited to a single line (%0D%0A)
