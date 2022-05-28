# UNDER CONSTRUCTION!

# Description

Newsraft is a piece of software that aggregates syndicated web content like blogs and podcasts in one location for easy access from your terminal via comfy curses interface. It is greatly inspired by [Newsboat](https://www.newsboat.org) and yet has some significant differences (see [doc/project-goals.md](https://codeberg.org/grisha/newsraft/src/branch/main/doc/project-goals.md)).

# Dependencies

| Name                                             | Version   | Necessity            |
|--------------------------------------------------|-----------|----------------------|
| [ncursesw](https://invisible-island.net/ncurses) | >= 6.2    | required             |
| [sqlite](https://www.sqlite.org)                 | >= 3.35.4 | required             |
| [curl](https://curl.se)                          | >= 7.80.0 | required             |
| [expat](https://github.com/libexpat/libexpat)    | >= 2.4.8  | required             |
| [yajl](https://github.com/lloyd/yajl)            | >= 2.1.0  | required             |
| [tidy](http://www.html-tidy.org)                 | >= 5.8.0  | required             |
| [gcc](https://gcc.gnu.org)                       | >= 8.1.0  | required, build-time |
| [meson](https://github.com/mesonbuild/meson)     | >= 0.58.1 | required, build-time |
| [scdoc](https://git.sr.ht/~sircmpwn/scdoc)       | >= 1.11.2 | optional, build-time |

# Building

## Compilation

```
meson setup builddir
meson compile -C builddir
```

## Installation

```
meson install -C builddir
```

# Configuration

Check out man page (or you can try to read it in scdoc format at [doc/newsraft.scd](https://codeberg.org/grisha/newsraft/src/branch/main/doc/newsraft.scd)).

# Contributing

See [doc/contributing.md](https://codeberg.org/grisha/newsraft/src/branch/main/doc/contributing.md).

# Copying

See [doc/license.txt](https://codeberg.org/grisha/newsraft/src/branch/main/doc/license.txt).
