# Feedeater

Feed reader for terminal

## Description

Feedeater is a piece of software that aggregates syndicated web content such as blogs, newspapers and podcasts in one location for easy viewing in your terminal. The updates distributed may include news posts, journal tables of contents, audio and video recordings, etc.

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

## Configuration

All of the configuration is done through files.
