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
	Force use of _FILE_ as *feeds* file.

\-c _FILE_
	Force use of _FILE_ as *config* file.

\-d _FILE_
	Force use of _FILE_ as database file.

\-l _FILE_
	Write log information to _FILE_.

\-p
	Purge feeds not specified in the *feeds* file.

\-h
	Print usage information and quit with 0 exit code.

\-v
	Print version information and quit with 0 exit code.

# STARTER GUIDE

To start using Newsraft you have to create a *feeds* file with the list of links
that point to feeds you want to receive news from. Format of this file and valid
paths for the location of this file are discussed in the *FEEDS FILE* section.

When your *feeds* file is ready, you can launch Newsraft. Navigation in the
interface is carried out by the same keys as in the *vi*(1) text editor. There
are only 4 menus you're gonna deal with: _sections menu_, _feeds menu_,
_items menu_ and _pager menu_.

_Sections menu_ consists of section entries which are needed to organize feeds
in groups to be able to process them in bulk. They are kind of directories for
feeds. If you didn't specify any section declarations in your *feeds* file then
you will get to the _feeds menu_ straightaway.

_Feeds menu_ consists of feed entries. Every feed entry contains news downloaded
from one specific source which you have set in *feeds* file. To update a single
feed you have to select it and press *r*. If you want to update all feeds,
*Ctrl-R* (or *^R* for short) will do this fine. From _feeds menu_ you can get to
the _items menu_ by entering some feed.

_Items menu_ consists of feed item entries (i. e. single pieces of news) which
you get when you update feeds in the previous menu. Every feed item entry has
two switchable properties - read state and importance state. Keys to force read
state change: *d* to mark read, *D* to mark unread, *^D* to mark everything
read. Keys to force importance state change: *i* to mark important, *I* to mark
unimportant. To view item's content you have to go to _pager menu_ by entering
selected item.

_Pager menu_ will display some details about selected item and render its
content if it was provided by feed. Usually feed item entries have a links
section with one link pointing to a related web page and several links that were
mentioned in the item's content. You can copy these links into your clipboard
with *y* key and open them in your web browser with *o* key. To target a key
action to link with a specific index you have to prefix your key with this
index. For example, *5y* will copy fifth link and *17o* will open seventeenth
link in the web browser. You can also setup custom command bindings to execute
any commands with these links. Consider this *config* file:

	_bind m exec mpv "%l"_++
_bind f exec feh "%l"_

With this you will be able to open any link in _mpv_ and _feh_ directly from
your terminal! Isn't it awesome? It is freaking amazing!

Actually, I lied that there are only 4 menus. There are actually 5 of them, and
the fifth secret menu is legendary as the greatest and the most useful menu in
feed readers history! This menu is known by the name of _explore menu_ and is
invoked by pressing the *e* key in either _sections menu_ or _feeds menu_. What
it does is truly miraculous - it gives you the list of feed item entries of all
sections or feeds in the current scope. Open it in _sections menu_ and you will
get all feed item entries you have in the database, open it in _feeds menu_ and
you will get all feed item entries of feeds in this _feeds menu_. This menu may
come in handy when you want to quickly scroll through all the news without
switching between sections and feeds back and forth.

# CONFIGURATION

## FEEDS FILE

This file contains feed entries that Newsraft will display and process. There
are 3 types of lines in *feeds* file.

Feed lines start with a URL. After at least one whitespace character, the name
of the feed may be specified - it must be enclosed in double quotes. For
example:

	_https://example.org/feed.xml "Lorem Ipsum Blog"_

Section lines start with @ character. After any number of whitespace characters,
the name of the section must be specified. For example:

	_@ Software Releases_

Comment lines start with # character. These lines are completely ignored. For
example:

	_# You will gain from using it, for sure._

Both feed and section lines allow you to set an auto update counter at the end,
which will force section or feed related to this line to be automatically
updated at a certain frequency. The period of automatic updates is specified in
minutes and must be enclosed in curly braces. If you set an auto update counter
for a feed that belongs to a section with an auto update counter set, then the
auto update counter of the feed will take precedence. Auto update counter of *0*
disables automatic updates. Let's walk through a small example and see how it
all works. Consider this *feeds* file:

	_http://example.org/feed1.xml_++
_http://example.org/feed2.xml {30}_

	_@ Replies {20}_++
_http://example.org/feed3.xml {10}_++
_http://example.org/feed4.xml "Forum notifications"_

	_@ News {60}_++
_http://example.org/feed5.xml_++
_http://example.org/feed6.xml "Local weather" {0}_

In this example, _feed1_ will never update automatically, _feed2_ will update
every *30* minutes, _feed3_ will update every *10* minutes, _feed4_ will update
every *20* minutes, _feed5_ will update every *60* minutes and _feed6_ will
never update automatically.

If you want to just automatically update all feeds then you should set an auto
update counter for *Global* section, but before doing so, think about what load
can be exerted on a servers by frequent downloads from a large number of users.
Going for a small auto update counter on *Global* section will most likely
result in a waste of a lot of resources. You have been warned.

Search precedence:
. _$XDG_CONFIG_HOME_/newsraft/feeds
. _$HOME_/.config/newsraft/feeds
. _$HOME_/.newsraft/feeds

## CONFIG FILE

This file is used to override default settings and bindings of Newsraft.
Presence of *config* file is totally optional and Newsraft will work without it
just fine. There are 3 types of lines in *config* file.

Setting lines start with the *set* word. You can find available settings in the
*SETTINGS* and *COLOR SETTINGS* sections. Format of these lines is as follows,
where _value_ can be enclosed in single or double quotes:

	*set* setting _value_

Binding lines start with the *bind* word. They define actions that are performed
when certain keys are pressed. Complete list of assigned actions can be found in
the *ACTIONS* section. Format of these lines is as follows:

	*bind* key _action_

There is also a way to assign command bindings. When a key with a command
binding assigned is pressed while feed item is selected, the specifiers in the
_command_ are replaced with the corresponding feed item values and the command
is executed. You can find which specifiers are available in the description of
the _menu-item-entry-format_ setting. Format of these lines is as follows:

	*bind* key *exec* _command_

In case you want to disable some binding which was set in Newsraft by default,
you can use a line according to this format:

	*unbind* key

Comment lines start with # character. These lines are completely ignored. For
example:

	_# Look closely, the beautiful may be small._

Search precedence:
. _$XDG_CONFIG_HOME_/newsraft/config
. _$HOME_/.config/newsraft/config
. _$HOME_/.newsraft/config

## DATABASE FILE

This file stores everything you download from feeds in *sqlite3*(1) format.
Although you now know the format in which the data is stored, it is highly
recommended to avoid modifying the database manually - things will break and
it will be very sad.

Search precedence:
. _$XDG_DATA_HOME_/newsraft/newsraft.sqlite3
. _$HOME_/.local/share/newsraft/newsraft.sqlite3
. _$HOME_/.newsraft/newsraft.sqlite3

# SETTINGS

_scrolloff_ (default: *0*)
	Minimal number of list menu entries to keep above and below the selected
	entry. If you set it to a very large value the selected entry will always be
	in the middle of the list menu (except at the start or end of the list menu).

_items-count-limit_ (default: *0*)
	Maximum number of items stored in one feed. Zero disables the limit.

_update-threads-count_ (default: *0*)
	Maximum number of update processes running simultaneously. If set to zero,
	value of the setting is set to the number of CPU cores available. You may want
	to set this setting to a lower value in cases of a slow network connection
	where parallel downloads can slow down the update process. If set to a value
	greater than the number of CPU cores available, it will be implicitly reduced
	to the number of CPU cores to avoid performance issues.

_open-in-browser-command_ (default: *${BROWSER:-xdg-open} "%l"*)
	Shell command for opening URL in a web browser. The URL to be opened is put
	in place where *%l* specifier is located.

_copy-to-clipboard-command_ (default: *auto*)
	Shell command for copying text to clipboard. All copied data is sent to the
	standard input of the command. If it is set to *"auto"*, then Newsraft will
	set the setting value depending on the environment variables: *"wl-copy"*
	command will be set if environment variable WAYLAND_DISPLAY is present and
	*"xclip -selection clipboard"* command will be set if environment variable
	DISPLAY is present, otherwise *"false"* command will be set.

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

_item-formation-order_ (default: *feed,title,authors,published,updated,max-content*)
	Sets the order in which the data of item entry is formed for viewing in a
	pager. Possible fields are: *feed*, *title*, *link*, *published*, *updated*,
	*authors*, *contributors*, *editors* and *max-content*.

_content-date-format_ (default: *%a, %d %b %Y %H:%M:%S %z*)
	Date format in the content view. Specifier values correspond to the
	*strftime*(3) format.

_list-entry-date-format_ (default: *%b %d*)
	Date format of the list entries. Specifier values correspond to the
	*strftime*(3) format.

_menu-section-entry-format_ (default: *%5.0u @ %t*)
	Format of the section list entries. Specifiers are as follows:
	- *n* - index number;
	- *u* - unread items count;
	- *t* - section title.

_menu-feed-entry-format_ (default: *%5.0u │ %o*)
	Format of the feed list entries. Specifiers are as follows:
	- *n* - index number;
	- *u* - unread items count;
	- *l* - feed link;
	- *t* - feed title;
	- *o* - feed title if set, feed link otherwise.

_menu-item-entry-format_ (default: *" %u │ %d │ %o"*)
	Format of the item list entries. Specifiers are as follows:
	- *n* - index number;
	- *u* - "N" if item is unread, " " otherwise;
	- *d* - update date formatted according to _list-entry-date-format_;
	- *l* - item link;
	- *t* - item title;
	- *o* - item title if set, item link otherwise;
	- *L* - feed link;
	- *T* - feed title;
	- *O* - feed title if set, feed link otherwise.

_menu-explore-item-entry-format_ (default: *" %u │ %d │ %-28.28O │ %o"*)
	Format of the item list entries in explore mode. Specifiers are the same as
	in _menu-item-entry-format_.

_mark-item-read-on-hover_ (default: *false*)
	Mark every item that gets selected as read.

_analyze-database-on-startup_ (default: *true*)
	Run "ANALYZE" SQLite command on the database every time you start Newsraft.
	It gathers statistics about database and uses it to optimize some queries
	making runtime faster.

_clean-database-on-startup_ (default: *false*)
	Run "VACUUM" SQLite command on the database every time you start Newsraft.
	It rebuilds the database file by packing it into a minimal amount of disk space.
	This can significantly increase startup time.

_download-timeout_ (default: *10*)
	Maximum time in seconds that you allow Newsraft to download one feed. Zero
	disables the limit.

_download-speed-limit_ (default: *0*)
	Maximum download speed in kilobytes per second (kB/s). Zero disables the
	limit.

_status-messages-count-limit_ (default: *1000*)
	Maximum number of status messages stored in memory. If set to zero, status
	messages history won't be stored in memory.

_send-user-agent-header_ (default: *true*)
	Attach user-agent header to download requests. See _user-agent_ setting.

_user-agent_ (default: *auto*)
	User-agent string to attach to download requests. If it is set to *"auto"*,
	Newsraft will generate it according to the following format:

		*"newsraft/"* + NEWSRAFT_VERSION + *" ("* + OS_NAME + *")"*

	OS_NAME shouldn't be a matter of privacy concern, because on most systems it
	contains nothing more like *"Linux"* or *"Darwin"*. If you want to be sure
	of this, check Newsraft log to see how _user-agent_ is set at startup.

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

# COLOR SETTINGS

Color settings are the same settings as above, but they take a certain set of
whitespace separated tokens for their values: one hue word and optional format
attributes. Available tokens for hue are *black*, *red*, *green*, *yellow*,
*blue*, *magenta*, *cyan* and *white*. Available tokens for format attributes
are *bold*, *italic* and *underlined*.

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

_select-next_ (default: *j*, *KEY_DOWN*)++
_select-prev_ (default: *k*, *KEY_UP*)++
_select-next-page_ (default: *space*, *^F*, *KEY_NPAGE*)++
_select-prev-page_ (default: *^B*, *KEY_PPAGE*)++
_select-first_ (default: *g*, *KEY_HOME*)++
_select-last_ (default: *G*, *KEY_END*)++
_jump-to-next_ (default: *J*)++
_jump-to-prev_ (default: *K*)++
_jump-to-next-unread_ (default: *n*)++
_jump-to-prev-unread_ (default: *N*)++
_jump-to-next-important_ (default: *p*)++
_jump-to-prev-important_ (default: *P*)++
_sorting-method-next_ (default: *s*)++
_sorting-method-prev_ (default: *S*)++
_enter_ (default: *l*, *^J*, *KEY_RIGHT*, *KEY_ENTER*)++
_reload_ (default: *r*)++
_reload-all_ (default: *^R*)++
_mark-read_ (default: not set)++
_mark-read-and-jump-to-next_ (default: *d*)++
_mark-read-and-jump-to-next-unread_ (default: not set)++
_mark-unread_ (default: not set)++
_mark-unread-and-jump-to-next_ (default: *D*)++
_mark-read-all_ (default: *^D*)++
_mark-unread-all_ (default: not set)++
_mark-important_ (default: *i*)++
_mark-unimportant_ (default: *I*)++
_explore-menu_ (default: *e*)++
_status-history-menu_ (default: *v*)++
_open-in-browser_ (default: *o*)++
_copy-to-clipboard_ (default: *y*, *c*)++
_quit_ (default: *h*, *q*, *KEY_LEFT*, *KEY_BACKSPACE*)++
_quit-hard_ (default: *Q*)

# FORMATS SUPPORT

Data formats of feeds which Newsraft recognizes. Not the whole functionality of
these formats is implemented, but only the functionality that is most likely to
carry the most essential information.

_RSS 2.0_, _1.1_, _1.0_, _0.94_, _0.93_, _0.92_, _0.91_, _0.9_++
_Atom 1.0_++
_RSS Content Module_++
_Media RSS_++
_DublinCore 1.1 Elements_++
_JSON Feed_ (it's such a joke)

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

*vi*(1), *sqlite3*(1), *strftime*(3), *libcurl*(3), *libcurl-env*(3)

# BUGS

Don't be ridiculous...

# AUTHOR

Grigory Kirillov <txgk@bk.ru>
