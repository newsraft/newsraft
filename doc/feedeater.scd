FEEDEATER(1)

# NAME

feedeater - feed reader for terminal

# SYNOPSIS

*feedeater* [-f _FILE1_] [-c _FILE2_] [-d _FILE3_] [-l _FILE4_]

*feedeater* -h

*feedeater* -v

# DESCRIPTION

Feedeater is a piece of software that aggregates syndicated web content like
blogs and podcasts in one location for easy access from your terminal via comfy
curses interface.

# OPTIONS

\-f _FILE_
	Force use of _FILE_ as feeds file.

\-c _FILE_
	Force use of _FILE_ as configuration file.

\-d _FILE_
	Force use of _FILE_ as database file.

\-l _FILE_
	Write log information to _FILE_.

\-h
	Print usage information and quit with 0 exit code.

\-v
	Print version information and quit with 0 exit code.

# CONFIGURATION

## FEEDS FILE

Feeds file contains feed entries that feedeater will display and process. There
are 3 types of lines in feeds file.

Feed lines start with a URL. After at least one whitespace character, the name
of the feed may be specified - it must be enclosed in double quotes. For
example:

	_https://example.org/feed.xml "Lorem Ipsum Blog"_

Section lines start with @ character. After any number of whitespace characters,
the name of the section must be specified. For example:

	_@ Software Releases_

Comment lines start with # character. These lines are completely ignored. For
example:

	_# haha i'm being ignored_

Search precedence:
. _$XDG_CONFIG_HOME_/feedeater/feeds
. _$HOME_/.config/feedeater/feeds
. _$HOME_/.feedeater/feeds

## CONFIG FILE

Config file is used to override default settings of feedeater. Complete list of
configurable options can be found in *SETTINGS* section. Presence of config file
is totally optional and feedeater will work without it just fine.

Search precedence:
. _$XDG_CONFIG_HOME_/feedeater/config
. _$HOME_/.config/feedeater/config
. _$HOME_/.feedeater/config

## DATABASE FILE

Database file stores everything you download from feeds in *sqlite3*(1) format.

Search precedence:
. _$XDG_DATA_HOME_/feedeater/feedeater.sqlite3
. _$HOME_/.local/share/feedeater/feedeater.sqlite3

# SETTINGS

_max-items_ (default: *0*)
	Maximum number of items stored in one feed. Zero disables the limit.

_download-timeout_ (default: *20*)
	Maximum time in seconds that you allow feedeater to download one feed. Zero
	disables the limit.

_status-messages-limit_ (default: *10000*)
	Maximum number of status messages stored in memory. Zero disables the limit.

_size-conversion-threshold_ (default: *1200*)
	TODO

_menu-section-entry-format_ (default: *"%4.0u │ %t"*)
	TODO

_menu-feed-entry-format_ (default: *"%4.0u │ %t"*)
	TODO

_menu-item-entry-format_ (default: *" %u │ %t"*)
	TODO

_proxy_ (default: *""*)
	TODO

_proxy-auth_ (default: *""*)
	TODO

_global-section-name_ (default: *"Global"*)
	Name of the section that contains all feeds.

_send-user-agent-header_ (default: *true*)
	Attach user-agent header to download requests. See _user-agent_ setting.

_user-agent_ (default: *""*)
	User-agent string to attach to download requests. If it is empty, feedeater
	will generate it in the following form:

		*"feedeater/"* + _FEEDEATER_VERSION_ + *" ("* + _OS_NAME_ + *")"*

	_OS_NAME_ shouldn't contain any confidential information - it is probably
	something just like "Linux" or "Darwin".

_content-data-order_ (default: *"feed,title,authors,published,updated,max-summary-content"*)
	TODO

_content-date-format_ (default: *"%a, %d %b %Y %H:%M:%S %z"*)
	Format of date in the content view.

_content-append-links_ (default: *true*)
	Whether or not append links section to content view.

_clean-database-on-startup_ (default: *true*)
	Run "VACUUM" SQLite command on the database every time you start feedeater.
	It rebuilds the database file, repacking it into a minimal amount of disk space.
	This can significantly increase startup time.

_analyze-database-on-startup_ (default: *true*)
	Run "ANALYZE" SQLite command on the database every time you start feedeater.
	It gathers statistics about database and uses it to optimize some queries.
	This can significantly increase startup time.

_send-if-none-match-header_ (default: *true*)
	TODO
	Disabling it is discouraged.

_send-if-modified-since-header_ (default: *true*)
	TODO
	Disabling it is discouraged.

_ssl-verify-host_ (default: *true*)
	TODO
	Disabling it is discouraged.

_ssl-verify-peer_ (default: *true*)
	TODO
	Disabling it is discouraged.

# ENVIRONMENT

Since all feedeater's interactions with the network occur through the
*libcurl*(3) library, all environment variables taken into account by
*libcurl*(3) can be set for feedeater as well. You can learn more about the
environment variables that *libcurl*(3) understands on *libcurl-env*(3).

## XDG_CONFIG_HOME

Directory for applications to store their configuration.

## XDG_DATA_HOME

Directory for applications to store their data.

## HOME

Home directory of your user.

# SEE ALSO

*newsboat*(1), *sqlite3*(1), *libcurl*(3), *libcurl-env*(3)

# AUTHOR

Grigory Kirillov
