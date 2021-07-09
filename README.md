# feedeater

feed reader for terminal

## Description

Terminal-based application for reading feed channels in various Web syndication formats. User interface is implemented in ncurses. Configuration is done through files.

Project goals are:

* shift html processing responsibilities to web browser

* omit the support for bunch of "cloud"-based feed services

* utilize a small amount of libraries

## Dependencies

* ncurses >= 6.2

* libcurl >= 7.75.0

* expat >= 2.2.10

* sqlite >= 3.35.4

* web browser of your choice

* gcc >= 10.2.0 (build-time)

* ninja >= 1.10.2 (build-time)

* meson >= 0.58.1 (build-time)

## Building

	meson setup builddir
	cd builddir
	meson compile
