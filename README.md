## Description

Newsraft is a [feed reader](https://en.wikipedia.org/wiki/News_aggregator) with
text-based user interface. It's greatly inspired by
[Newsboat](https://www.newsboat.org) and tries to be its lightweight counterpart

![Newsraft in action](doc/newsraft.png)

## Features

* Parallel downloads
* Section-based feeds grouping
* Opening links in any program
* [News filtering using SQL conditions](https://newsraft.codeberg.page/#item-rule_(*))
* Viewing news from all feeds with explore mode
* [Automatic updates for feeds and sections](https://newsraft.codeberg.page/#reload-period_(*))
* Per-feed settings and key bindings
* Assigning multiple actions to key bindings
* Text searching by news titles and content
* Interactive news content viewing
* Menu sorting by your most desired parameters
* Detailed error reports on failed updates
* Processing feeds from command output
* [Support for practically all feed formats](https://newsraft.codeberg.page/#FORMATS_SUPPORT)
* Import/export OPML
* Come try segfault me, baby ;)

Check out [comparison of Newsraft and Newsboat](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/comparison-newsboat.md).

## Dependencies

| Library                                                        | Version   |
|:---------------------------------------------------------------|:----------|
| [curl](https://curl.se)                                        | >= 7.87.0 |
| [expat](https://github.com/libexpat/libexpat)                  | >= 2.4.8  |
| [gumbo-parser](https://codeberg.org/gumbo-parser/gumbo-parser) | >= 0.11.0 |
| [sqlite](https://www.sqlite.org)                               | >= 3.38.0 |

Development tools: C99 compiler, POSIX make, [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config), [gperf](https://www.gnu.org/software/gperf), [scdoc](https://git.sr.ht/~sircmpwn/scdoc), [mandoc](https://mandoc.bsd.lv)

## Installing

Some package repositories have Newsraft package so it can be installed with a
package manager.

[![Packaging status](https://repology.org/badge/vertical-allrepos/newsraft.svg?columns=4)](https://repology.org/project/newsraft/versions)

If your package repository doesn't provide Newsraft package, you can build it
from source: see [doc/build-instructions.md](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/build-instructions.md)
for instructions.

## Learning more

The essential source of information about Newsraft is its
[man page](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/newsraft.1)
(if you already have Newsraft installed, you can open it with the `man newsraft` command).
Alternatively you can read the HTML version of this man page [here](https://newsraft.codeberg.page).

Examples on how to configure Newsraft are located in the
[doc/examples](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/examples)
directory.

If you want to connect with fellow users of Newsraft, come to **#newsraft** at [libera.chat](https://libera.chat) IRC server.

## Contributing

Reporting bugs: [doc/contributing-report.md](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/contributing-report.md)

Making changes: [doc/contributing-change.md](https://codeberg.org/newsraft/newsraft/src/branch/main/doc/contributing-change.md)

## FAQ

<details>
	<summary>Why it's called Newsraft?</summary>
	This is a rip-off of <a href="https://www.newsboat.org">Newsboat</a>, replacing "boat" with "raft", which emphasizes a smaller codebase.
</details>

<details>
	<summary>How do I bind mpv to run in the background?</summary>
	<code>bind m exec setsid mpv --terminal=no "%l" &amp;</code>
</details>

<details>
	<summary>How do I filter out things I don't want to see in my feed?</summary>
	See <a href="https://newsraft.codeberg.page/#item-rule_(*)">item-rule</a> setting.
</details>

<details>
	<summary>I want Newsraft to show me a help screen on ? key press.</summary>
	Easy. Just put <code>bind ? exec man newsraft</code> into your <code>config</code> file.
</details>

<details>
	<summary>Can I alter feed's content before Newsraft processes it?</summary>
	Yes, you can do practically anything before Newsraft takes over. It's done
	via shell interlayer: any shell command in between of <code>$(</code>
	and <code>)</code> will be executed on reload and its standard output will
	be taken for a feed content. Here are examples of such feeds:<br>
	<code>$(gemget -sq gemini://example.org/feed.xml) "Simple blog"</code><br>
	<code>$($HOME/bin/html2rss http://example.org/index.html) "Local news"</code>
</details>

<details>
	<summary>Why some of my feeds are lagging behind the upstream website even after updating?</summary>
	Some web servers ask Newsraft to withhold content to reduce network load. Newsraft fulfills these web server wishes by default. There are settings to disable Newsraft's respect for web servers and make it a bad boy, if you are that kind of person.
</details>

<details>
	<summary>I get a lot of getaddrinfo() thread failed to start errors. What do I do?</summary>
	Usually it happens because your setup can't handle many concurrent DNS resolves. Try to reduce the value of setting <code>download-max-connections</code>.
</details>

<details>
	<summary>My database is over 9000 GB now. What do I do?</summary>
	<ul>
		<li>Set capacity limit on some of your heavy feeds via <a href="https://newsraft.codeberg.page/#item-limit_(*)">item-limit</a> setting</li>
		<li>Delete cache of feeds you unsubscribed from with <code>newsraft -e purge-abandoned</code></li>
	</ul>
</details>

<details>
	<summary>Where I can find a change log?</summary>
	See <a href="https://codeberg.org/newsraft/newsraft/src/branch/main/doc/changes.md">doc/changes.md</a> file.
</details>

<details>
	<summary>How is Newsraft licensed?</summary>
	The license is <a href="https://codeberg.org/newsraft/newsraft/src/branch/main/doc/license.txt">ISC</a> because its name is sweet.
</details>
