NEWSRAFT(1)

# NAME

newsraft - feed reader for terminal

# SYNOPSIS

*newsraft* [*-f* _FILE1_] [*-c* _FILE2_] [*-d* _FILE3_] [*-l* _FILE4_] [*-p*] [*-v*] [*-h*]

# DESCRIPTION

Newsraft is a small text based program for reading syndication feeds - it
enables you to obtain content from many blogs and platforms and view all this
via one single interface in an efficient and streamlined manner.

# OPTIONS

*-f* _FILE_
	Force use of _FILE_ as *feeds* file.

*-c* _FILE_
	Force use of _FILE_ as *config* file.

*-d* _FILE_
	Force use of _FILE_ as *database* file.

*-l* _FILE_
	Write log information to _FILE_.

*-p*
	Purge feeds not specified in the *feeds* file.

*-v*
	Print version information.

*-h*
	Print usage information.

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

With this you will be able to open any link in *mpv*(1) and *feh*(1) directly
from your terminal! Isn't it awesome? It is freaking amazing!

For both _sections menu_ and _feeds menu_ there is a special explore mode. You
can toggle it by pressing the *tab* key (*^I*). It's truly miraculous: it shows
all the news in the current context (i. e. in _sections menu_ it will show you
all the news in the database, whereas in _feeds menu_ it will show you all the
news of feeds in the current _feeds menu_). This mode may come in handy when you
want to quickly scroll through all the news without switching between sections
and feeds back and forth.

And for dessert, I'll tell you about search functionality. You can type */* in
_items menu_ to start search input - enter the desired query here and press
*Enter*. This will generate a new list of items based on given search query. To
return to the original items list, you just need to make an empty search
(press */* and *Enter* in sequence). Searching like that in _sections menu_ and
_feeds menu_ will automatically enable explore mode and apply your search query.

# CONFIGURATION

## FEEDS FILE

This file contains feed entries that Newsraft will display and process. There
are 4 types of lines in *feeds* file.

Feed lines start with a URL. After at least one whitespace character, the name
of the feed may be specified - it must be enclosed in double quotes. For
example:

	_https://example.org/feed.xml "Lorem Ipsum Blog"_

Generator lines start with a command enclosed in *$()*. These act just like feed
lines but instead of fetching resources from a remote server they use the output
of the specified command to obtain the content.

	_$(cat ~/local-feed.xml) "Lorem Ipsum Blog"_

Section lines start with *@* character. After any number of whitespace characters,
the name of the section must be specified. For example:

	_@ Software Releases_

Comment lines start with *#* character. These lines are completely ignored. For
example:

	_# Look closely. The beautiful may be small._

Both feed and section lines allow you to set an auto update timer at the end,
which will force section or feed related to this line to be automatically
updated at a certain frequency. The period of automatic updates is specified in
minutes and must be enclosed in square brackets. If you set an auto update timer
for a feed that belongs to a section with an auto update timer set, then the
auto update timer of the feed will take precedence. Auto update timer of *0*
disables automatic updates. Let's walk through a small example and see how it
all works. Consider this *feeds* file:

	_http://example.org/feed1.xml_++
_http://example.org/feed2.xml [30]_

	_@ Replies [20]_++
_http://example.org/feed3.xml [10]_++
_http://example.org/feed4.xml "Forum notifications"_

	_@ News [60]_++
_http://example.org/feed5.xml_++
_http://example.org/feed6.xml "Local weather" [0]_

In this example, _feed1_ will never update automatically, _feed2_ will update
every *30* minutes, _feed3_ will update every *10* minutes, _feed4_ will update
every *20* minutes, _feed5_ will update every *60* minutes and _feed6_ will
never update automatically.

If you just want to automatically update all feeds then you should set an auto
update timer for *Global* section at the very beginning of the file, but before
doing so, think about what load can be exerted on a servers by frequent
downloads. Going for a small auto update timer on *Global* section will most
likely result in a waste of a lot of resources. You have been warned.

In a similar to update timers fashion, item limits can be set for feeds and
sections. They make targeted feeds contain no more than a set amount of news.
Precedence rules are the same as for update timers. The only difference is type
of brackets used: they use curly brackets instead of square ones. For example:

	_http://example.org/feed.xml "Politics" [1440] {100}_

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

There is also a way to assign command bindings. When a key with assigned command
binding is pressed, the specifiers in the _command_ are replaced with values of
the corresponding entry and the command is executed. You can find which
specifiers are available in the description of the _menu-item-entry-format_
setting. Format of these lines is as follows:

	*bind* key *exec* _command_

Binding lines support assigning multiple actions to one key. Assigned actions
must be separated with semicolon symbols, for example:

	*bind* key _action1_; _action2_; *exec* _command1_; *exec* _command2_; _action5_

In case you want to disable some binding which was set in Newsraft by default,
you can use a line according to this format:

	*unbind* key

Comment lines start with *#* character. These lines are completely ignored. For
example:

	_# Good design is as little design as possible._

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
	in the middle of the list menu (except for start and end of the list menu).

_pager-width_ (default: *100*)
	Pager width in characters. If set to *0*, pager will take up all available
	space.

_update-threads-count_ (default: *0*)
	Maximum number of updates running simultaneously. If set to *0*, the setting
	value will be set to the number of available CPU cores multiplied by *10*.
	However, the setting value will never exceed *100*. You might want to set this
	setting to a lower value if you have a poor network connection causing
	slowdowns due to overloading.

_menu-feed-sorting_ (default: *none*)
	Sorting order for the feeds menu. Available values: *unread-desc*,
	*unread-asc*, *alphabet-desc*, *alphabet-asc*.

_menu-item-sorting_ (default: *time-desc*)
	Sorting order for the items menu. Available values: *time-desc*, *time-asc*,
	*unread-desc*, *unread-asc*, *alphabet-desc*, *alphabet-asc*.

_open-in-browser-command_ (default: *${BROWSER:-xdg-open} "%l"*)
	Shell command for opening URL in a web browser. The URL to be opened is put
	in place where *%l* specifier is located.

_copy-to-clipboard-command_ (default: *auto*)
	Shell command for copying text to clipboard. All copied data is sent to the
	standard input of the command. If it is set to *"auto"*, then Newsraft will
	set the setting value to *"wl-copy"* if environment variable WAYLAND_DISPLAY
	is set and to *"xclip -selection clipboard"* if environment variable DISPLAY
	is set. Systems on macOS will force setting value to *"pbcopy"*. In other
	cases the setting value will be set to *"false"*.

_notification-command_ (default: *auto*)
	Shell command for invoking system notifications about new news received. If
	it is set to *"auto"*, then Newsraft will set the setting value to
	*"notify-send 'Newsraft brought %q news!'"* for most Unix systems and to
	*"osascript -e 'display notification "Newsraft brought %q news!"'"* for macOS.

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

_item-content-format_ (default: *<b>Feed</b>:&nbsp;&nbsp;%f<br>|<b>Title</b>:&nbsp;%t<br>|<b>Date</b>:&nbsp;&nbsp;%d<br>|<br>%c<br>|<br><b>Links</b>:<br>%L*)
	Sets the format according to which the item's content will be generated.
	The text in this format string is HTML formatted. Fields are separated by *|*
	character. If an item doesn't have a value corresponding to the specifier in
	the field, then the entire field will not be shown. Specifiers are as follows:++
*f*	feed title if set, feed link otherwise;++
*t*	item title;++
*l*	item link;++
*d*	item date;++
*a*	item authors;++
*c*	item content;++
*L*	item links list.

_item-content-date-format_ (default: *%a, %d %b %Y %H:%M:%S %z*)
	Date format in the item's content. Specifier values correspond to the
	*strftime*(3) format.

_list-entry-date-format_ (default: *%b %d*)
	Date format of the list entries. Specifier values correspond to the
	*strftime*(3) format.

_menu-section-entry-format_ (default: *%5.0u @ %t*)
	Format of the section list entries. Specifiers are as follows:++
*i*	index number;++
*u*	unread items count;++
*t*	section title.

_menu-feed-entry-format_ (default: *%5.0u │ %t*)
	Format of the feed list entries. Specifiers are as follows:++
*i*	index number;++
*u*	unread items count;++
*l*	feed link;++
*t*	feed name if set, feed link otherwise.

_menu-item-entry-format_ (default: *" %u │ %d │ %o"*)
	Format of the item list entries. Specifiers are as follows:++
*i*	index number;++
*u*	"N" if item is unread, " " otherwise;++
*d*	update date formatted according to _list-entry-date-format_;++
*D*	publication date formatted according to _list-entry-date-format_;++
*l*	item link;++
*t*	item title;++
*o*	item title if set, item link otherwise;++
*L*	feed link;++
*T*	feed title;++
*O*	feed title if set, feed link otherwise.

_menu-explore-item-entry-format_ (default: *" %u │ %d │ %-28O │ %o"*)
	Format of the item list entries in explore mode. Specifiers are the same as
	in _menu-item-entry-format_.

_sections-menu-paramount-explore_ (default: *false*)
	Enables explore mode in sections menu by default.

_feeds-menu-paramount-explore_ (default: *false*)
	Enables explore mode in feeds menu by default.

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

_download-timeout_ (default: *20*)
	Maximum time in seconds that you allow Newsraft to download one feed. Setting
	to *0* disables the timeout.

_download-speed-limit_ (default: *0*)
	Maximum download speed in kilobytes per second (kB/s). Setting to *0* disables
	the limit.

_status-messages-count-limit_ (default: *1000*)
	Maximum number of status messages stored in memory. If set to *0*, status
	messages history will not be stored in memory.

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

# COLOR SETTINGS

Color settings are the same settings as above, but they take two color words
(foreground and background) and optional attribute words. Available colors are
*default*, *black*, *red*, *green*, *yellow*, *blue*, *magenta*, *cyan*, *white*
and *colorN* (*N* can be a number from *0* to *255*). Available attributes are
*bold*, *italic* and *underlined*.

|[ Color setting
:[ Default value
|[ _color-status-good_
:[ *green default*
|[ _color-status-info_
:[ *cyan default*
|[ _color-status-fail_
:[ *red default*
|[ _color-list-item_
:[ *default default*
|[ _color-list-item-unread_
:[ *yellow default*
|[ _color-list-item-important_
:[ *magenta default*
|[ _color-list-feed_
:[ *default default*
|[ _color-list-feed-unread_
:[ *yellow default*
|[ _color-list-section_
:[ *default default*
|[ _color-list-section-unread_
:[ *yellow default*

# ACTIONS

|[ Keys
:[ Actions
|[ *j*, *KEY_DOWN*, *^E*
:[ _select-next_
|[ *k*, *KEY_UP*, *^Y*
:[ _select-prev_
|[ *space*, *^F*, *KEY_NPAGE*
:[ _select-next-page_
|[ *^B*, *KEY_PPAGE*
:[ _select-prev-page_
|[ *g*, *KEY_HOME*
:[ _select-first_
|[ *G*, *KEY_END*
:[ _select-last_
|[ *J*
:[ _jump-to-next_
|[ *K*
:[ _jump-to-prev_
|[ *n*
:[ _jump-to-next-unread_
|[ *N*
:[ _jump-to-prev-unread_
|[ *p*
:[ _jump-to-next-important_
|[ *P*
:[ _jump-to-prev-important_
|[ *t*
:[ _sort-by-time_
|[ *u*
:[ _sort-by-unread_
|[ *a*
:[ _sort-by-alphabet_
|[ *l*, *^J*, *KEY_RIGHT*, *KEY_ENTER*
:[ _enter_
|[ *r*
:[ _reload_
|[ *^R*
:[ _reload-all_
|[ *d*
:[ _mark-read_; _jump-to-next_
|[ *D*
:[ _mark-unread_; _jump-to-next_
|[ *^D*
:[ _mark-read-all_
|[ (not set)
:[ _mark-unread-all_
|[ *i*
:[ _mark-important_
|[ *I*
:[ _mark-unimportant_
|[ *^I*, *e*
:[ _toggle-explore-mode_
|[ *v*
:[ _status-history-menu_
|[ *o*
:[ _open-in-browser_
|[ *y*, *c*
:[ _copy-to-clipboard_
|[ */*
:[ _start-search-input_
|[ *h*, *^?*, *KEY_LEFT*, *KEY_BACKSPACE*
:[ _navigate-back_
|[ *q*
:[ _quit_
|[ *Q*
:[ _quit-hard_

# FORMATS SUPPORT

Data formats of feeds which Newsraft recognizes. Not the whole functionality of
these formats is implemented, but only the functionality that is most likely to
carry the most essential information.

_RSS 2.0_, _1.1_, _1.0_, _0.94_, _0.93_, _0.92_, _0.91_, _0.9_++
_Atom 1.0_++
_RSS Content Module_++
_Media RSS_++
_DublinCore 1.1 Elements_++
_JSON Feed_

# ENVIRONMENT

Newsraft's behavior depends on the environment variables set, however not all
environment variables affect Newsraft directly - many environment variables
affect libraries that Newsraft is built on. Thus, *ncurses*(3) and *libcurl*(3)
recognize a large number of different environment variables which you can learn
more about on *ncurses*(3) and *libcurl-env*(3) respectively.

However, there is one significant *ncurses*(3) environment variable that is
worth mentioning here - *ESCDELAY*. It sets delay for reading Escape key. It
may surprise you that its default value is 1000 ms, which is well explained
in *ncurses*(3), but many may prefer a value much less than that or even 0.

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

*vi*(1), *mpv*(1), *feh*(1), *sqlite3*(1), *strftime*(3), *ncurses*(3), *libcurl*(3), *libcurl-env*(3)

# BUGS

Don't be ridiculous...

# AUTHOR

Grigory Kirillov <txgk@bk.ru>
