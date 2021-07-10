# feedeater

feed reader for terminal

## Description

Terminal-based application for reading feed channels in various Web syndication formats. User interface is implemented in ncurses. Configuration is done through files.

Project goals are:

* avoid the support for bunch of cloud-based third-party feed services

* utilize few libraries

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
