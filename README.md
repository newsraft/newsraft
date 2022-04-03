# UNDER CONSTRUCTION!

# Description

Feedeater is a piece of software that aggregates syndicated web content like blogs and podcasts in one location for easy access from your terminal via comfy curses interface. It is greatly inspired by [Newsboat](https://www.newsboat.org) and yet has some significant differences (see [doc/project-goals.md](https://codeberg.org/grisha/feedeater/src/branch/main/doc/project-goals.md)).

# Dependencies

* [ncursesw](https://invisible-island.net/ncurses) >= 6.2

* [sqlite](https://www.sqlite.org) >= 3.35.4

* [curl](https://curl.se) >= 7.80.0

* [tidy](http://www.html-tidy.org) >= 5.8.0

* [cjson](https://github.com/DaveGamble/cJSON) >= 1.7.15

* [gcc](https://gcc.gnu.org) >= 8.1.0 (build-time)

* [meson](https://github.com/mesonbuild/meson) >= 0.58.1 (build-time)

* [scdoc](https://git.sr.ht/~sircmpwn/scdoc) >= 1.11.2 (build-time, for man page)

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

Check out man page (or see [doc/feedeater.scd](https://codeberg.org/grisha/feedeater/src/branch/main/doc/feedeater.scd)).

# Contributing

See [doc/contributing.md](https://codeberg.org/grisha/feedeater/src/branch/main/doc/contributing.md).

# Copying

See [doc/license.txt](https://codeberg.org/grisha/feedeater/src/branch/main/doc/license.txt).
