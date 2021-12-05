# Description

Feedeater is a piece of software that aggregates syndicated web content such as blogs, newspapers and podcasts in one location for easy access from your terminal.

Feedeater is greatly inspired by [Newsboat](https://www.newsboat.org).

# Dependencies

* ncursesw >= 6.2

* libcurl >= 7.75.0

* expat >= 2.2.10

* sqlite >= 3.35.4

* gcc >= 8.1.0 (build-time)

* meson >= 0.58.1 (build-time)

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

See [CONFIGURATION.md](https://gitlab.com/got2teas/feedeater/-/blob/main/CONFIGURATION.md).


# Contributing

See [CONTRIBUTING.md](https://gitlab.com/got2teas/feedeater/-/blob/main/CONTRIBUTING.md).
