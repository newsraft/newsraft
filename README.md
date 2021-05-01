# feedeater

simple feed reader for terminal (work in progress)

## Description

Command line based application for reading feed channels in RSS & Atom formats. User interface is implemented in ncurses. Configuration is done through files.

Project **non-goals** are:

* support for third-party feed services

* utilization of large number of libraries

## Dependencies

* ncurses >= 6.2

* libcurl >= 7.75.0

* expat >= 2.2.10

* sqlite >= 3.35.4

* gcc >= 10.2.0 (build-time)

* ninja >= 1.10.2 (build-time)

## Building

	ninja
