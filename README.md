# Description

Feedeater is a piece of software that aggregates syndicated web content such as blogs, newspapers and podcasts in one location for easy access from your terminal via comfy curses interface. It is greatly inspired by [Newsboat](https://www.newsboat.org) and yet has some significant differences (see [doc/project-goals.md](https://gitlab.com/got2teas/feedeater/-/blob/main/doc/project-goals.md)).

# Dependencies

* [ncursesw](https://invisible-island.net/ncurses) >= 6.2

* [libcurl](https://github.com/curl/curl) >= 7.75.0

* [expat](https://github.com/libexpat/libexpat) >= 2.2.10

* [sqlite](https://www.sqlite.org) >= 3.35.4

* [gcc](https://gcc.gnu.org) >= 8.1.0 (build-time)

* [meson](https://github.com/mesonbuild/meson) >= 0.58.1 (build-time)

* [scdoc](https://git.sr.ht/~sircmpwn/scdoc) >= 1.11.2 (build-time, for man page)

# Building

To build an executable:

```
meson setup builddir
```

```
meson compile -C builddir
```

To generate a man page:

```
scdoc < doc/feedeater.scd > feedeater.1
```

# Configuration

Check out man page.

# Contributing

See [doc/contributing.md](https://gitlab.com/got2teas/feedeater/-/blob/main/doc/contributing.md).
