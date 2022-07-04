## Description

Newsraft is a [feed reader](https://en.wikipedia.org/wiki/News_aggregator) with
[ncurses](https://en.wikipedia.org/wiki/Ncurses) user interface. It is greatly
inspired by [Newsboat](https://www.newsboat.org) and tries to be its lightweight
counterpart. The design differences betwen these projects are mentioned in the
[doc/project-goals.md](https://codeberg.org/grisha/newsraft/src/branch/main/doc/project-goals.md).

## Dependencies

| Name                                             | Version   | Necessity            |
|--------------------------------------------------|-----------|----------------------|
| [ncursesw](https://invisible-island.net/ncurses) | >= 6.2    | required             |
| [sqlite](https://www.sqlite.org)                 | >= 3.35.4 | required             |
| [curl](https://curl.se)                          | >= 7.80.0 | required             |
| [expat](https://github.com/libexpat/libexpat)    | >= 2.4.8  | required             |
| [yajl](https://github.com/lloyd/yajl)            | >= 2.1.0  | required             |
| [gumbo](https://github.com/google/gumbo-parser)  | >= 0.10.1 | required             |
| [gcc](https://gcc.gnu.org)                       | >= 8.1.0  | required, build-time |
| [meson](https://github.com/mesonbuild/meson)     | >= 0.58.1 | required, build-time |
| [scdoc](https://git.sr.ht/~sircmpwn/scdoc)       | >= 1.11.2 | optional, build-time |

## Building

Compile:

```
meson setup builddir
meson compile -C builddir
```

Install:

```
meson install -C builddir
```

## Learning more

The main source of information about Newsraft is its man page. If you've built
Newsraft from source with `scdoc` dependency, man page must be located in the
build directory. Alternatively you can try to read it in the raw scdoc format at
[doc/newsraft.scd](https://codeberg.org/grisha/newsraft/src/branch/main/doc/newsraft.scd).
Examples on how to configure Newsraft are located in the
[examples](https://codeberg.org/grisha/newsraft/src/branch/main/examples) directory.

If you still have any questions, feel free to ask them in the #newsraft IRC
channel which is hosted on the [OFTC](https://www.oftc.net) network. Waiting for
the response may take some time, stay connected!

## Contributing

Helping others: [doc/contributing-support.md](https://codeberg.org/grisha/newsraft/src/branch/main/doc/contributing-support.md)

Reporting bugs: [doc/contributing-report.md](https://codeberg.org/grisha/newsraft/src/branch/main/doc/contributing-report.md)

Making changes: [doc/contributing-change.md](https://codeberg.org/grisha/newsraft/src/branch/main/doc/contributing-change.md)

Sending donation: [doc/contributing-donation.md](https://codeberg.org/grisha/newsraft/src/branch/main/doc/contributing-donation.md)

## Copying

Newsraft is distributed under the terms of the
[MIT License](https://codeberg.org/grisha/newsraft/src/branch/main/doc/license.txt).
