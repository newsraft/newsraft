# Description

Feedeater is a piece of software that aggregates syndicated web content such as blogs, newspapers and podcasts in one location for easy access from your terminal.

Feedeater is greatly inspired by [Newsboat](https://www.newsboat.org).

# Dependencies

* [ncursesw](https://invisible-island.net/ncurses) >= 6.2

* [libcurl](https://github.com/curl/curl) >= 7.75.0

* [expat](https://github.com/libexpat/libexpat) >= 2.2.10

* [sqlite](https://www.sqlite.org) >= 3.35.4

* [gcc](https://gcc.gnu.org) >= 8.1.0 (build-time)

* [meson](https://github.com/mesonbuild/meson) >= 0.58.1 (build-time)

* [scdoc](https://git.sr.ht/~sircmpwn/scdoc) >= 1.11.2 (build-time, for man page)

# Building

```
meson setup builddir
```

```
cd builddir
```

```
meson compile
```

# Configuration

Check out man page.

# Contributing

See [CONTRIBUTING.md](https://gitlab.com/got2teas/feedeater/-/blob/main/CONTRIBUTING.md).
