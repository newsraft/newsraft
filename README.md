# Feedeater

Feed reader for terminal

## Description

Terminal-based application for reading feed channels in various Web syndication formats.

Feedeater uses [ncurses](https://invisible-island.net/ncurses) to draw interface, [curl](https://curl.se) to download feeds, [Expat](https://libexpat.github.io) to process XML and [SQLite](https://www.sqlite.org) to store data. All configuration is done through command-line arguments, environment variables and files.

Feedeater is greatly inspired by [Newsboat](https://www.newsboat.org).

## Dependencies

* ncursesw >= 6.2

* libcurl >= 7.75.0

* expat >= 2.2.10

* sqlite >= 3.35.4

* gcc >= 8.1.0 (build-time)

* meson >= 0.58.1 (build-time)

## Building

	meson setup builddir
	cd builddir
	meson compile
