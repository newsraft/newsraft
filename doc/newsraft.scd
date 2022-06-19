NEWSRAFT(1)

# NAME

newsraft - feed reader for terminal

# SYNOPSIS

*newsraft* [-f _FILE1_] [-c _FILE2_] [-d _FILE3_] [-l _FILE4_]

*newsraft* -h

*newsraft* -v

# DESCRIPTION

Newsraft is a piece of software that aggregates syndicated web content like
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

Feeds file contains feed entries that newsraft will display and process. There
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
. _$XDG_CONFIG_HOME_/newsraft/feeds
. _$HOME_/.config/newsraft/feeds
. _$HOME_/.newsraft/feeds

## CONFIG FILE

Config file is used to override default settings of newsraft. Complete list of
configurable options can be found in *SETTINGS* section. Presence of config file
is totally optional and newsraft will work without it just fine.

Search precedence:
. _$XDG_CONFIG_HOME_/newsraft/config
. _$HOME_/.config/newsraft/config
. _$HOME_/.newsraft/config

## DATABASE FILE

Database file stores everything you download from feeds in *sqlite3*(1) format.

Search precedence:
. _$XDG_DATA_HOME_/newsraft/newsraft.sqlite3
. _$HOME_/.local/share/newsraft/newsraft.sqlite3

# SETTINGS

_items-count-limit_ (default: *0*)
	Maximum number of items stored in one feed. Zero disables the limit.

_download-timeout_ (default: *20*)
	Maximum time in seconds that you allow newsraft to download one feed. Zero
	disables the limit.

_download-speed-limit_ (default: *0*)
	Maximum download speed in kilobytes per second (kB/s). Zero disables the
	limit.

_status-messages-limit_ (default: *10000*)
	Maximum number of status messages stored in memory. Zero disables the limit.

_size-conversion-threshold_ (default: *1200*)
	TODO

_copy-to-clipboard-command_ (default: *auto*)
	Shell command for copying text to clipboard. All copied information is sent
	to the standard input of the command. If it is set to "auto", then newsraft
	will set the setting value depending on the environment variables: "wl-copy"
	command will be set if environment variable WAYLAND_DISPLAY is present and
	"xclip -selection clipboard" command will be set if environment variable
	DISPLAY is present, otherwise "false" command will be set.

_proxy_ (default: *""*)
	TODO

_proxy-auth_ (default: *""*)
	TODO

_global-section-name_ (default: *Global*)
	Name of the section that contains all feeds.

_send-user-agent-header_ (default: *true*)
	Attach user-agent header to download requests. See _user-agent_ setting.

_user-agent_ (default: *auto*)
	User-agent string to attach to download requests. If it is set to "auto",
	newsraft will generate it in the following form:

		*"newsraft/"* + NEWSRAFT_VERSION + *" ("* + OS_NAME + *")"*

	OS_NAME shouldn't be a matter of privacy concern, because on most systems it
	contains nothing more like "Linux" or "Darwin". If you want to be sure of
	this, check newsraft log to see which _user-agent_ is set at startup.

_item-formation-order_ (default: *feed-url,title,authors,published,updated,max-content*)
	Sets the order in which the data of item entry is formed for viewing in a
	pager. Possible fields are: *feed-url*, *title*, *link*, *published*,
	*updated*, *authors*, *contributors*, *editors*, *summary*, *content* and
	*max-content*.

_content-date-format_ (default: *%a, %d %b %Y %H:%M:%S %z*)
	Date format in the content view. Specifier values correspond to the
	*strftime*(3) format.

_list-entry-date-format_ (default: *%b %d*)
	Date format of the list entries. Specifier values correspond to the
	*strftime*(3) format.

_menu-section-entry-format_ (default: *%4.0u │ %t*)
	TODO

_menu-feed-entry-format_ (default: *%4.0u │ %t*)
	TODO

_menu-item-entry-format_ (default: *" %u │ %d │ %t"*)
	TODO

_menu-overview-item-entry-format_ (default: *" %u │ %d │ %-28.28f │ %t"*)
	TODO

_mark-item-read-on-hover_ (default: *false*)
	Mark every item that gets selected as read.

_content-append-links_ (default: *true*)
	Whether or not append links section to content view.

_clean-database-on-startup_ (default: *true*)
	Run "VACUUM" SQLite command on the database every time you start newsraft.
	It rebuilds the database file, repacking it into a minimal amount of disk space.
	This can significantly increase startup time.

_analyze-database-on-startup_ (default: *true*)
	Run "ANALYZE" SQLite command on the database every time you start newsraft.
	It gathers statistics about database and uses it to optimize some queries.
	This can significantly increase startup time.

_respect-ttl-element_ (default: *true*)
	Prevents too frequent updates for some feeds. The limit is set by the
	creators of the feeds in order to save traffic and resources for a very
	rarely updated feeds. Disabling it is strongly discouraged.

_respect-expires-header_ (default: *true*)
	Prevents feed updates until the expiration date of the previously downloaded
	information in order to save traffic and resources. Disabling it is strongly
	discouraged.

_send-if-none-match-header_ (default: *true*)
	Sends a entity tag corresponding to the previously downloaded information.
	If the server from which the feed is downloaded contains information with
	the same tag, then in order to save traffic and resources, it will reject
	the download request. Disabling it is strongly discouraged.

_send-if-modified-since-header_ (default: *true*)
	TODO
	Disabling it is strongly discouraged.

_ssl-verify-host_ (default: *true*)
	TODO
	Disabling it is strongly discouraged.

_ssl-verify-peer_ (default: *true*)
	TODO
	Disabling it is strongly discouraged.

_color-status-good-fg_ (default: *green*)
	Text color of status area when reporting success.

_color-status-good-bg_ (default: *black*)
	Background color of status area when reporting success.

_color-status-info-fg_ (default: *cyan*)
	Text color of status area when reporting information.

_color-status-info-bg_ (default: *black*)
	Background color of status area when reporting information.

_color-status-fail-fg_ (default: *red*)
	Text color of status area when reporting failure.

_color-status-fail-bg_ (default: *black*)
	Background color of status area when reporting failure.

_color-list-item-fg_ (default: *white*)
	Text color of item list entry.

_color-list-item-bg_ (default: *black*)
	Background color of item list entry.

_color-list-item-unread-fg_ (default: *yellow*)
	Text color of unread item list entry.

_color-list-item-unread-bg_ (default: *black*)
	Background color of unread item list entry.

_color-list-feed-fg_ (default: *white*)
	Text color of feed list entry.

_color-list-feed-bg_ (default: *black*)
	Background color of feed list entry.

_color-list-feed-unread-fg_ (default: *yellow*)
	Text color of unread feed list entry.

_color-list-feed-unread-bg_ (default: *black*)
	Background color of unread feed list entry.

_color-list-section-fg_ (default: *white*)
	Text color of section list entry.

_color-list-section-bg_ (default: *black*)
	Background color of section list entry.

_color-list-section-unread-fg_ (default: *yellow*)
	Text color of unread section list entry.

_color-list-section-unread-bg_ (default: *black*)
	Background color of unread section list entry.

# ENVIRONMENT

Since all newsraft's interactions with the network occur through the
*libcurl*(3) library, all environment variables taken into account by
*libcurl*(3) can be set for newsraft as well. You can learn more about the
environment variables that *libcurl*(3) understands on *libcurl-env*(3).

## XDG_CONFIG_HOME

Directory for applications to store their configuration.

## XDG_DATA_HOME

Directory for applications to store their data.

## HOME

Home directory of your user.

# SEE ALSO

*sqlite3*(1), *strftime*(3), *libcurl*(3), *libcurl-env*(3)

# AUTHOR

Grigory Kirillov
