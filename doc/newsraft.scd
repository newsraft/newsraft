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

Feeds file contains feed entries that Newsraft will display and process. There
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

	_# you will gain from using it, for sure_

Search precedence:
. _$XDG_CONFIG_HOME_/newsraft/feeds
. _$HOME_/.config/newsraft/feeds
. _$HOME_/.newsraft/feeds

## CONFIG FILE

Config file is used to override default settings and bindings of Newsraft.
Presence of config file is totally optional and Newsraft will work without it
just fine. There are 3 types of lines in config file.

Setting lines start with the *set* word. Complete list of configurable settings
can be found in the *SETTINGS* section. Format of these lines is as follows,
where _value_ can be enclosed in single or double quotes:

	*set* setting _value_

Binding lines start with the *bind* word. They define actions that are performed
when certain keys are pressed. Complete list of assigned actions can be found in
the *ACTIONS* section. Format of these lines is as follows:

	*bind* key _action_

There is also a way to assign command bindings. When the key to which the
command binding is assigned is pressed, the specifiers in the _command_ are
replaced with the corresponding values and the command is executed. Available
specifiers are: *%l* for URL. Format of these lines is as follows:

	*bind* key *exec* _command_

In case you want to disable some binding which was set in Newsraft by default,
you can use a line according to this format:

	*unbind* key

Comment lines start with # character. These lines are completely ignored. For
example:

	_# look closely, the beautiful may be small_

Search precedence:
. _$XDG_CONFIG_HOME_/newsraft/config
. _$HOME_/.config/newsraft/config
. _$HOME_/.newsraft/config

## DATABASE FILE

Database file stores everything you download from feeds in *sqlite3*(1) format.
Although you now know the format in which the data is stored, it is highly
recommended to avoid modifying the database manually - things will break and
it will be very sad.

Search precedence:
. _$XDG_DATA_HOME_/newsraft/newsraft.sqlite3
. _$HOME_/.local/share/newsraft/newsraft.sqlite3

# SETTINGS

_items-count-limit_ (default: *0*)
	Maximum number of items stored in one feed. Zero disables the limit.

_download-timeout_ (default: *20*)
	Maximum time in seconds that you allow Newsraft to download one feed. Zero
	disables the limit.

_download-speed-limit_ (default: *0*)
	Maximum download speed in kilobytes per second (kB/s). Zero disables the
	limit.

_update-threads-count_ (default: *0*)
	Maximum number of update processes running simultaneously. If set to zero,
	value of the setting is set automatically according to the following rule:
	if your system has more than one processor online, setting value is set to
	*2*; otherwise setting value is set to *1*. Values greater than *2* may
	reduce performance if there is not enough network bandwidth available.

_status-messages-count-limit_ (default: *10000*)
	Maximum number of status messages stored in memory. Zero disables the limit.

_open-in-browser-command_ (default: *auto*)
	Shell command for opening URL in web browser. The URL to be opened is
	appended to the end of the setting value as the last command argument.
	If it is set to "auto", then Newsraft will try to obtain setting value
	from the BROWSER environment variable. If environment variable BROWSER
	is not set, then its value is taken as "xdg-open".

_copy-to-clipboard-command_ (default: *auto*)
	Shell command for copying text to clipboard. All copied information is sent
	to the standard input of the command. If it is set to "auto", then Newsraft
	will set the setting value depending on the environment variables: "wl-copy"
	command will be set if environment variable WAYLAND_DISPLAY is present and
	"xclip -selection clipboard" command will be set if environment variable
	DISPLAY is present, otherwise "false" command will be set.

_proxy_ (default: *""*)
	Sets the proxy to use for the network requests. It must be either a hostname
	or dotted numerical IPv4 address. To specify IPv6 address you have to enclose
	it within square brackets. Port number can be set by appending :PORT to the
	end of setting value. By default proxy protocol is considered HTTP, but you
	can set a different one by prepending SCHEME:// to the setting value.

_proxy-user_ (default: *""*)
	User for authentication with the proxy server.

_proxy-password_ (default: *""*)
	Password for authentication with the proxy server.

_global-section-name_ (default: *Global*)
	Name of the section that contains all feeds.

_empty-title-placeholder_ (default: *(untitled)*)
	Text that is displayed in place of empty titles.

_send-user-agent-header_ (default: *true*)
	Attach user-agent header to download requests. See _user-agent_ setting.

_user-agent_ (default: *auto*)
	User-agent string to attach to download requests. If it is set to "auto",
	Newsraft will generate it in the following form:

		*"newsraft/"* + NEWSRAFT_VERSION + *" ("* + OS_NAME + *")"*

	OS_NAME shouldn't be a matter of privacy concern, because on most systems it
	contains nothing more like "Linux" or "Darwin". If you want to be sure of
	this, check Newsraft log to see which _user-agent_ is set at startup.

_item-formation-order_ (default: *feed-url,title,authors,published,updated,max-content*)
	Sets the order in which the data of item entry is formed for viewing in a
	pager. Possible fields are: *feed-url*, *title*, *link*, *published*,
	*updated*, *authors*, *contributors*, *editors* and *max-content*.

_content-date-format_ (default: *%a, %d %b %Y %H:%M:%S %z*)
	Date format in the content view. Specifier values correspond to the
	*strftime*(3) format.

_list-entry-date-format_ (default: *%b %d*)
	Date format of the list entries. Specifier values correspond to the
	*strftime*(3) format.

_menu-section-entry-format_ (default: *%4.0u @ %t*)
	Format of the section list entries. Specifiers are as follows:
	- *t* - title;
	- *u* - unread items count;
	- *n* - index number.

_menu-feed-entry-format_ (default: *%4.0u │ %o*)
	Format of the feed list entries. Specifiers are as follows:
	- *o* - title if set otherwise URL;
	- *t* - title;
	- *l* - URL;
	- *u* - unread items count;
	- *n* - index number.

_menu-item-entry-format_ (default: *" %u │ %d │ %t"*)
	Format of the item list entries. Specifiers are as follows:
	- *t* - title if set _empty-title-placeholder_ otherwise;
	- *d* - update date formatted according _list-entry-date-format_;
	- *o* - feed title if set otherwise feed URL;
	- *l* - URL;
	- *u* - "N" if item is unread " " otherwise;
	- *n* - index number.

_menu-explore-item-entry-format_ (default: *" %u │ %d │ %-28.28o │ %t"*)
	Format of the item list entries in explore mode. Specifiers are as follows:
	- *t* - title if set _empty-title-placeholder_ otherwise;
	- *d* - update date formatted according _list-entry-date-format_;
	- *o* - feed title if set otherwise feed URL;
	- *l* - URL;
	- *u* - "N" if item is unread " " otherwise;
	- *n* - index number.

_mark-item-read-on-hover_ (default: *false*)
	Mark every item that gets selected as read.

_content-append-links_ (default: *true*)
	Whether or not append links section to content view.

_clean-database-on-startup_ (default: *true*)
	Run "VACUUM" SQLite command on the database every time you start Newsraft.
	It rebuilds the database file, repacking it into a minimal amount of disk space.
	This can significantly increase startup time.

_analyze-database-on-startup_ (default: *true*)
	Run "ANALYZE" SQLite command on the database every time you start Newsraft.
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
	Sends an entity tag corresponding to the previously downloaded information.
	If the server from which the feed is downloaded contains information with
	the same tag, then in order to save traffic and resources, it will reject
	the download request. Disabling it is strongly discouraged.

_send-if-modified-since-header_ (default: *true*)
	Sends a date corresponding to the last modification of previously downloaded
	information. If the server from which the feed is downloaded contains
	information with the same modification date, then in order to save traffic
	and resources, it will reject the download request. Disabling it is strongly
	discouraged.

_ssl-verify-host_ (default: *true*)
	Verifies that the server certificate corresponds to its identity when a
	secure connection is established. Connections with an invalid certificate
	will fail. Disabling it is strongly discouraged.

_ssl-verify-peer_ (default: *true*)
	Verifies that the peer certificate corresponds to its identity when a
	secure connection is established. Connections with an invalid certificate
	will fail. Disabling it is strongly discouraged.

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

_color-list-item-important-fg_ (default: *magenta*)
	Text color of important item list entry.

_color-list-item-important-bg_ (default: *black*)
	Background color of important item list entry.

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

# ACTIONS

_select-next_ (default: *j*, *KEY_DOWN*)

_select-next-unread_ (default: *n*)

_select-next-page_ (default: *KEY_NPAGE*)

_select-prev_ (default: *k*, *KEY_UP*)

_select-prev-unread_ (default: *p*)

_select-prev-page_ (default: *KEY_PPAGE*)

_select-first_ (default: *g*, *KEY_HOME*)

_select-last_ (default: *G*, *KEY_END*)

_enter_ (default: *l*, *^J*, *KEY_RIGHT*, *KEY_ENTER*)

_reload_ (default: *r*)

_reload-all_ (default: *^R*)

_mark-read_ (default: *d*)

_mark-read-all_ (default: *^D*)

_mark-unread_ (default: *D*)

_mark-unread-all_ (default: not set)

_mark-important_ (default: *i*)

_mark-unimportant_ (default: *I*)

_explore-menu_ (default: *e*)

_status-history-menu_ (default: *v*)

_open-in-browser_ (default: *o*)

_copy-to-clipboard_ (default: *y*, *c*)

_quit_ (default: *h*, *q*, *KEY_LEFT*, *KEY_BACKSPACE*)

_quit-hard_ (default: *Q*)

# ENVIRONMENT

Since all Newsraft's interactions with the network occur through the
*libcurl*(3) library, all environment variables taken into account by
*libcurl*(3) can be set for Newsraft as well. You can learn more about the
environment variables that *libcurl*(3) understands on *libcurl-env*(3).

## XDG_CONFIG_HOME

Directory for applications to store their configuration.

## XDG_DATA_HOME

Directory for applications to store their data.

## HOME

Home directory of your user.

## BROWSER

Web browser of your user.

## WAYLAND_DISPLAY

Identifier of the Wayland graphics display.

## DISPLAY

Identifier of the X graphics display.

## NO_COLOR

Setting that forcibly makes the interface monochrome when set.

# SEE ALSO

*sqlite3*(1), *strftime*(3), *libcurl*(3), *libcurl-env*(3)

# BUGS

Don't be ridiculous...

# AUTHOR

Grigory Kirillov <txgk@bk.ru>
