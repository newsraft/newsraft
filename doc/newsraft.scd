NEWSRAFT(1)

# NAME

newsraft - feed reader for terminal

# SYNOPSIS

*newsraft* [*-f* _FILE1_] [*-c* _FILE2_] [*-d* _FILE3_] [*-l* _FILE4_] [*-p*] [*-v*] [*-h*]

# DESCRIPTION

Newsraft is a small text based program for reading syndication feeds. It obtains
content from a given set of sources and lets you browse it all via one
streamlined user interface.

# OPTIONS

|[ *-f* _FILE_
:< Force *feeds* file to _FILE_.
|[ *-c* _FILE_
:[ Force *config* file to _FILE_.
|[ *-d* _FILE_
:[ Force *database* file to _FILE_.
|[ *-l* _FILE_
:[ Write log information to _FILE_.
|[ *-p*
:[ Purge feeds not specified in the *feeds* file.
|[ *-v*
:[ Print version information.
|[ *-h*
:[ Print usage information.

# STARTER GUIDE

To start using Newsraft you have to create a *feeds* file with the list of links
to feeds you want to receive news from. Check out *FEEDS FILE* section for file
syntax and valid paths.

When *feeds* file is ready, you can launch Newsraft. There are only 4 menus you
will have to deal with: _sections_, _feeds_, _items_ and _pager_. Default binds
are listed in *ACTIONS* section.

_Sections_ menu consists of section entries which are needed to organize feeds
in groups to be able to process them in bulk. They are kind of directories for
feeds. If you didn't specify any section declarations in your *feeds* file then
you will get to the _feeds_ menu straightaway.

_Feeds_ menu consists of feed entries. Every feed entry contains news downloaded
from one specific source which you have set in *feeds* file. To update a single
feed you have to select it and press *r* or *R* if you want to update all feeds.
From _feeds_ menu you can get to the _items_ menu by entering some feed.

_Items_ menu consists of feed item entries (i. e. single pieces of news) which
you get when you update feeds in the previous menu. Every feed item entry has
two switchable properties - read state and importance state. Keys to change read
state: *d* to mark read, *D* to mark unread, *^D* to mark everything read. Keys
to change importance state: *f* to flag important, *F* to flag unimportant. To
view item's content you have to go to _pager_ menu by entering selected item.

_Pager_ menu will display some details about selected item and render its
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

For both _sections_ menu and _feeds_ menu there is a special explore mode. You
can toggle it by pressing the *tab* key. It's truly miraculous: it reveals all
the news in the current context (combines news from all feeds of the current
menu into one list). This mode may come in handy when you want to quickly scroll
through all the news without switching between sections and feeds back and forth.

And for dessert, I'll tell you about search functionality. You can type */* to
begin search input - enter the desired query here and press *Enter*. This will
open an _items_ menu with a search query applied. To make filtering go away just
make an empty search (press */* and *Enter* in series).

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

Both feed and section lines allow you to set individual settings and binds for
them. The syntax is as follows:

	_@ Lorem Ipsum < reload-period 1440_++
_http://example.org/feed1.xml "Dolor Sit" < reload-period 60; item-limit 500_++
_http://example.org/feed2.xml "Id Est" < bind b mark-read; exec book.sh "%l"_

Settings set for feeds take precedence over the settings specified for sections.
Not every setting supports individual assignment - only settings with *scalable*
mark in *SETTINGS* section do.

Search precedence:
. _$XDG_CONFIG_HOME_/newsraft/feeds
. _$HOME_/.config/newsraft/feeds
. _$HOME_/.newsraft/feeds

## CONFIG FILE

This file is used to override default settings and bindings of Newsraft.
Presence of *config* file is totally optional and Newsraft will work without it
just fine. There are 3 types of lines in *config* file.

Setting lines start with a setting name and end with a setting value. Available
settings are listed in the *SETTINGS* and *COLOR SETTINGS* sections. Here are a
couple of examples:

	_scrolloff 5000_++
_list-entry-date-format "%D"_++
_feeds-menu-paramount-explore true_

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

Settings with *scalable* mark can be set for individual feeds and sections.

_reload-period_ (*scalable*, default: *0*)
	Feed auto reload period in minutes. If set to *0*, no auto reloads are run.

_item-limit_ (*scalable*, default: *0*)
	Maximum number of items stored in a feed. If set to *0*, no limit is set.

_scrolloff_ (default: *0*)
	Minimal number of list menu entries to keep above and below the selected
	entry. If you set it to a very large value the selected entry will always be
	in the middle of the list menu (except for start and end of the list menu).

_pager-width_ (*scalable*, default: *100*)
	Pager width in characters. If set to *0*, the pager will take up all available
	space.

_pager-centering_ (*scalable*, default: *true*)
	Whether the pager should center its content when _pager-width_ is not *0*. If
	set to *false*, the pager will be aligned to the left.

_menu-feed-sorting_ (default: *none*)
	Sorting order for the feeds menu. Available values: *unread-desc*,
	*unread-asc*, *alphabet-desc*, *alphabet-asc*.

_menu-item-sorting_ (default: *time-desc*)
	Sorting order for the items menu. Available values: *time-desc*, *time-asc*,
	*unread-desc*, *unread-asc*, *important-desc*, *important-asc*,
	*alphabet-desc*, *alphabet-asc*.

_open-in-browser-command_ (*scalable*, default: *${BROWSER:-xdg-open} "%l"*)
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

_proxy_ (*scalable*, default: *""*)
	Sets the proxy to use for the network requests. It must be either a hostname
	or dotted numerical IPv4 address. To specify IPv6 address you have to enclose
	it within square brackets. Port number can be set by appending :PORT to the
	end of setting value. By default proxy protocol is considered HTTP, but you
	can set a different one by prepending SCHEME:// to the setting value.

_proxy-user_ (*scalable*, default: *""*)
	User for authentication with the proxy server.

_proxy-password_ (*scalable*, default: *""*)
	Password for authentication with the proxy server.

_global-section-name_ (default: *Global*)
	Name of the section that contains all feeds.

_status-show-menu-path_ (default: *true*)
	Print menu path in the status bar.

_status-placeholder_ (default: *r:reload  R:reload-all  tab:explore  d:read  D:unread  f:important  F:unimportant  n:next-unread  N:prev-unread  p:next-important  P:prev-important*)
	Placeholder which is put in the status bar if it's empty.

_item-content-format_ (*scalable*, default: *<b>Feed</b>:&nbsp;&nbsp;%f<br>|<b>Title</b>:&nbsp;%t<br>|<b>Date</b>:&nbsp;&nbsp;%d<br>|<br>%c<br>|<br><hr>%L*)
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

_item-content-date-format_ (*scalable*, default: *%a, %d %b %Y %H:%M:%S %z*)
	Date format in the item's content. Specifier values correspond to the
	*strftime*(3) format.

_item-content-link-format_ (*scalable*, default: *<b>[%i]</b>:&nbsp;%l<br>*)
	Link format in the links list of item's content. *%i* and *%l* will be
	replaced by link index and link address respectively.

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

_mark-item-unread-on-change_ (*scalable*, default: *false*)
	Mark every item that changes on a feed update as unread.

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

_download-timeout_ (*scalable*, default: *20*)
	Maximum time in seconds that you allow Newsraft to download one feed. Setting
	to *0* disables the timeout.

_download-speed-limit_ (*scalable*, default: *0*)
	Maximum download speed in kilobytes per second (kB/s). Setting to *0* disables
	the limit.

_status-messages-count-limit_ (default: *1000*)
	Maximum number of status messages stored in memory. If set to *0*, status
	messages history will not be stored in memory.

_user-agent_ (*scalable*, default: *auto*)
	User-Agent header sent with download requests. If it is set to *"auto"*,
	Newsraft will generate it according to the following format:

		*"newsraft/"* + NEWSRAFT_VERSION + *" ("* + OS_NAME + *")"*

	OS_NAME shouldn't be a matter of privacy concern, because on most systems it
	contains nothing more like *"Linux"* or *"Darwin"*. If you want to be sure
	of this, check Newsraft log to see how _user-agent_ is set at startup.

	If set to *""*, User-Agent header will not be sent.

_respect-ttl-element_ (*scalable*, default: *true*)
	Prevents too frequent updates for some feeds. The limit is set by the
	creators of the feeds in order to save traffic and resources for a very
	rarely updated feeds. Disabling it is strongly discouraged.

_respect-expires-header_ (*scalable*, default: *true*)
	Prevents feed updates until the expiration date of the previously downloaded
	information in order to save traffic and resources. Disabling it is strongly
	discouraged.

_send-if-none-match-header_ (*scalable*, default: *true*)
	Sends an entity tag corresponding to the previously downloaded information.
	If the server from which the feed is downloaded contains information with
	the same tag, then in order to save traffic and resources, it will reject
	the download request. Disabling it is strongly discouraged.

_send-if-modified-since-header_ (*scalable*, default: *true*)
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
|[ _color-status_
:[ *green default bold*
|[ _color-status-info_
:[ *cyan default bold*
|[ _color-status-fail_
:[ *red default bold*
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
|[ _select-next_
:[ *j*, *KEY_DOWN*, *^E*
|[ _select-prev_
:[ *k*, *KEY_UP*, *^Y*
|[ _select-next-page_
:[ *space*, *^F*, *KEY_NPAGE*
|[ _select-prev-page_
:[ *^B*, *KEY_PPAGE*
|[ _select-first_
:[ *g*, *KEY_HOME*
|[ _select-last_
:[ *G*, *KEY_END*
|[ _jump-to-next_
:[ *J*
|[ _jump-to-prev_
:[ *K*
|[ _jump-to-next-unread_
:[ *n*
|[ _jump-to-prev-unread_
:[ *N*
|[ _jump-to-next-important_
:[ *p*
|[ _jump-to-prev-important_
:[ *P*
|[ _goto-feed_
:[ *\**
|[ _shift-west_
:[ *,*
|[ _shift-east_
:[ *.*
|[ _shift-reset_
:[ *<*
|[ _sort-by-time_
:[ *t*
|[ _sort-by-unread_
:[ *u*
|[ _sort-by-important_
:[ *i*
|[ _sort-by-alphabet_
:[ *a*
|[ _enter_
:[ *enter*, *l*, *KEY_ENTER*, *KEY_RIGHT*
|[ _reload_
:[ *r*
|[ _reload-all_
:[ *R*, *^R*
|[ _mark-read_; _jump-to-next_
:[ *d*
|[ _mark-unread_; _jump-to-next_
:[ *D*
|[ _mark-read-all_
:[ *^D*
|[ _mark-unread-all_
:[ (not set)
|[ _mark-important_
:[ *f*
|[ _mark-unimportant_
:[ *F*
|[ _toggle-explore-mode_
:[ *tab*, *e*
|[ _status-history-menu_
:[ *v*
|[ _open-in-browser_
:[ *o*
|[ _copy-to-clipboard_
:[ *y*, *c*
|[ _start-search-input_
:[ */*
|[ _clean-status_
:[ *escape*
|[ _navigate-back_
:[ *h*, *backspace*, *KEY_LEFT*, *KEY_BACKSPACE*
|[ _quit_
:[ *q*
|[ _quit-hard_
:[ *Q*

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

|[ *XDG_CONFIG_HOME*
:< Directory in which user-specific configuration files are stored.
|[ *XDG_DATA_HOME*
:[ Directory in which user-specific data files are stored.
|[ *HOME*
:[ User home directory.
|[ *BROWSER*
:[ User web browser.
|[ *WAYLAND_DISPLAY*
:[ Identifier of the Wayland graphics display.
|[ *DISPLAY*
:[ Identifier of the X graphics display.
|[ *NO_COLOR*
:[ Makes the interface monochrome when present.

# SEE ALSO

*mpv*(1), *feh*(1), *sqlite3*(1), *strftime*(3), *ncurses*(3), *libcurl*(3), *libcurl-env*(3)

# BUGS

Don't be ridiculous...

# AUTHORS

Grigory Kirillov and contributors
