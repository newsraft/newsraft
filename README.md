## Description

Newsraft is a [feed reader](https://en.wikipedia.org/wiki/News_aggregator) with
[ncurses](https://en.wikipedia.org/wiki/Ncurses) user interface. It's greatly
inspired by [Newsboat](https://www.newsboat.org) and tries to be its lightweight
counterpart.

![Newsraft in action](doc/newsraft.png)

## Features

* Parallel downloads
* Section-based feeds grouping
* Opening links in any program
* Viewing news from all feeds with explore mode
* Automatic updates for feeds and sections
* Multiple actions key bindings
* Support for feed generating scripts
* Text search by news titles
* Interactive interface for viewing news content

Check out [comparison of Newsraft and Newsboat](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/comparison-newsboat.md).

## Dependencies

| Name                                                               | Version   | Necessity                    |
|--------------------------------------------------------------------|-----------|------------------------------|
| [curl](https://curl.se)                                            | >= 7.87.0 | required                     |
| [expat](https://github.com/libexpat/libexpat)                      | >= 2.4.8  | required                     |
| [gumbo-parser](https://codeberg.org/grisha/gumbo-parser)           | >= 0.11.0 | required                     |
| [ncursesw](https://invisible-island.net/ncurses)                   | >= 6.2    | required                     |
| [sqlite](https://www.sqlite.org)                                   | >= 3.36.0 | required                     |
| [yajl](https://github.com/lloyd/yajl)                              | >= 2.1.0  | required                     |
| C compiler                                                         | Any C99   | required at build-time       |
| make                                                               | Any POSIX | required at build-time       |
| [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config) | >= 0.29.2 | optional at build-time       |
| [scdoc](https://git.sr.ht/~sircmpwn/scdoc)                         | >= 1.9.1  | only for rebuilding man page |

## Installing

Some package repositories have Newsraft package so it can be installed with a
package manager.

[![Packaging status](https://repology.org/badge/vertical-allrepos/newsraft.svg)](https://repology.org/project/newsraft/versions)

If your package repository doesn't provide Newsraft package, you can build it
from source. Detailed instructions for different platforms are described in
[doc/build-instructions.md](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/build-instructions.md).

## Learning more

The essential source of information about Newsraft is its
[man page](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/newsraft.1)
(if you already have Newsraft installed, you can open it with the `man newsraft` command).
Alternatively you can read the HTML version of this man page [here](https://txgk.ru/newsraft.html).

Examples on how to configure Newsraft are located in the
[doc/examples](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/examples)
directory.

## Contributing

Reporting bugs: [doc/contributing-report.md](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/contributing-report.md)

Making changes: [doc/contributing-change.md](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/contributing-change.md)

## Copying

Newsraft is distributed under the terms of the
[ISC license](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/license.txt).
