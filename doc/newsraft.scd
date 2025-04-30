NEWSRAFT(1)

# NAME

newsraft - feed reader for terminal

# SYNOPSIS

*newsraft* [*-f* _FILE1_] [*-c* _FILE2_] [*-d* _FILE3_] [*-l* _FILE4_] [*-e* _ACTION_] [*-v*] [*-h*]

# DESCRIPTION

Newsraft is a small text based program for reading syndication feeds. It obtains
content from a given set of sources and lets you browse it all via one
streamlined user interface.

# OPTIONS

*-f* _FILE_
	Force *feeds* file to _FILE_.

*-c* _FILE_
	Force *config* file to _FILE_.

*-d* _FILE_
	Force *database* file to _FILE_.

*-l* _FILE_
	Write logs to _FILE_.

*-e* _ACTION_
	Execute one-time _ACTION_ from the following list:
		*convert-opml-to-feeds* (takes OPML from standard input)++
*convert-feeds-to-opml* (takes feeds from *feeds* file)++
*reload-all*++
*print-unread-items-count*++
*purge-abandoned*

*-v*
	Print version information.

*-h*
	Print usage information.

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
Not every setting supports individual assignment - only settings with asterisk
*(\*)* on them do (see *SETTINGS* section).

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

Settings with asterisk *(\*)* on them can be set for individual feeds and sections.

## reload-period (*)
Default: _0_. Feed auto reload period in minutes. If set to _0_, no auto reloads
will be run.

## suppress-errors (*)
Default: _false_. If _true_, feed update error indication in the menu will be
disabled. It's recommended to set this setting only for specific feeds that are
expected to fail frequently and you are tired of seeing their errors.

## item-rule (*)
Default: _""_. Item search condition when accessing database. This can be very
useful in managing feeds with a heavy spam flow: you set a condition based on
some parameters and only those entries that meet this condition will be shown in
the feed. It's specified in SQL format. It probably only makes sense to set this
setting for individual feeds, and not globally (see *FEEDS FILE* section to
understand how).

Available parameters:
|[ _guid_
:[ (string)
:< Globally unique identifier of the article which is expected to be unique within the originating feed
|[ _title_
:[ (string)
:[ Title of the article
|[ _link_
:[ (string)
:[ URL of the article
|[ _content_
:[ (string)
:[ Content of the article which is stored exactly as it appears in the feed, with all original HTML preserved
|[ _attachments_
:[ (string)
:[ Serialized string of all attachments with URL links, MIME types and byte sizes
|[ _persons_
:[ (string)
:[ Serialized string of all people related to the article with names and email addresses
|[ _publication_date_
:[ (integer)
:[ Timestamp of publication date in seconds since 1970
|[ _update_date_
:[ (integer)
:[ Timestamp of update date in seconds since 1970

Here are some examples of correct setting values:

	_title NOT LIKE '%Rust%'_++
_persons LIKE '%PHARMACIST%' OR persons LIKE '%OFFL1NX%'_++
_attachments LIKE '%audio/mp3%' OR attachments LIKE '%video/mp4%'_++
_CAST(strftime('%Y', publication_date, 'unixepoch') AS INTEGER) >= 2024_++
_NOT (title REGEXP '.+:.+\\".+\\".+by' OR title LIKE '%(ultra slowed)%' OR title LIKE '%(1 hour version)%')_

REGEXP operator uses POSIX Extended Regular Expression syntax, see *regex*(7).

## item-limit (*)
Default: _0_. Maximum number of items stored in a feed. If set to _0_, no limit
will be set.

## item-limit-unread (*)
Default: _true_. If _true_, *item-limit* setting will also cap unread items.

## item-limit-important (*)
Default: _false_. If _true_, *item-limit* setting will also cap important items.

## scrolloff
Default: _0_. Minimal number of list menu entries to keep above and below the
selected entry. If you set it to a very large value the selected entry will
always be in the middle of the list menu (except for start and end of the list
menu).

## pager-width (*)
Default: _100_. Pager width in characters. If set to _0_, the pager will take up
all available space.

## pager-centering (*)
Default: _true_. If _true_ and *pager-width* is not _0_, pager will center its
content horizontally.

## menu-item-sorting
Default: _time-desc_. Sorting order for the items menu. Available values:
_time-desc_, _time-asc_, _rowid-desc_, _rowid-asc_, _unread-desc_, _unread-asc_,
_important-desc_, _important-asc_, _alphabet-desc_, _alphabet-asc_.

## menu-feed-sorting
Default: _none_. Sorting order for the feeds menu. Available values:
_unread-desc_, _unread-asc_, _alphabet-desc_, _alphabet-asc_.

## menu-section-sorting
Default: _none_. Sorting order for the sections menu. Available values:
_unread-desc_, _unread-asc_, _alphabet-desc_, _alphabet-asc_.

## menu-responsiveness
Default: _true_. If _true_, update menu contents as soon as possible.
If _false_, the menu will be updated only when you re-open it.

## open-in-browser-command (*)
Default: _${BROWSER:-xdg-open} "%l"_. Shell command for opening URL in a web
browser. The URL to be opened is put in place where _%l_ specifier is located.

## copy-to-clipboard-command
Default: _auto_. Shell command for copying text to clipboard. All copied data is
piped into the standard input of the specified command. If it's set to _"auto"_,
Newsraft will determine the appropriate command based on the user environment:

- _"wl-copy"_ if environment variable WAYLAND_DISPLAY is set
- _"xclip -selection clipboard"_ if environment variable DISPLAY is set
- _"pbcopy"_ if Newsraft was built for macOS operating system
- _"newsraft-osc-52"_ otherwise

_"newsraft-osc-52"_ is a special value that doesn't run an external command but
instead triggers the OSC 52 escape sequence, instructing the terminal to copy
data directly to the system clipboard. This behavior is especially useful when
running Newsraft over *ssh*(1), as it allows clipboard operations to affect the
local system rather than the remote one.

## notification-command (*)
Default: _auto_. Shell command for invoking system notifications about new news
received. If it's set to _"auto"_, Newsraft will determine the appropriate
command based on the user environment:

- _"notify-send 'Newsraft brought %q news!'"_ if WAYLAND_DISPLAY or DISPLAY is set in environment
- _"osascript -e 'display notification "Newsraft brought %q news!"'"_ if Newsraft was built for macOS operating system
- _"printf '\\e]9;Newsraft brought %q news!\\a'"_ (OSC 9 escape sequence) otherwise

## proxy (*)
Default: _""_. Sets the proxy to use for the network requests. It must be either
a hostname or dotted numerical IPv4 address. To specify IPv6 address you have to
enclose it within square brackets. Port number can be set by appending :PORT to
the end of setting value. By default proxy protocol is considered HTTP, but you
can set a different one by prepending SCHEME:// to the setting value.

## proxy-user (*)
Default: _""_. User for authentication with the proxy server.

## proxy-password (*)
Default: _""_. Password for authentication with the proxy server.

## global-section-name
Default: _Global_. Name of the section that contains all feeds.

## status-show-menu-path
Default: _true_. If _true_, print menu path in the status bar.

## status-placeholder
Default: _r:reload  R:reload-all  tab:explore  d:read  D:unread  f:important F:unimportant  n:next-unread  N:prev-unread  p:next-important P:prev-important_.

Placeholder which is put in the status bar if it's empty.

## item-content-format (*)
Default: _<b>Feed</b>:&nbsp;&nbsp;%f<br>|<b>Title</b>:&nbsp;%t<br>|<b>Date</b>:&nbsp;&nbsp;%d<br>|<br>%c<br>|<br><hr>%L_.

Sets the format according to which the item's content will be generated. The
text in this format string is HTML formatted. Fields are separated by _|_
character. If an item doesn't have a value corresponding to the specifier in the
field, then the entire field will not be shown. Specifiers are as follows:

	_f_	feed title if set, feed link otherwise;++
_t_	item title;++
_l_	item link;++
_d_	item date;++
_a_	item authors;++
_c_	item content;++
_L_	item links list.

## item-content-date-format (*)
Default: _%a, %d %b %Y %H:%M:%S %z_. Date format in the item's content.
Specifier values correspond to the *strftime*(3) format.

## item-content-link-format (*)
Default: _<b>[%i]</b>:&nbsp;%l<br>_. Link format in the links list of item's
content. _%i_ and _%l_ will be replaced by link index and link address
respectively.

## list-entry-date-format
Default: _%b %d_. Date format of the list entries. Specifier values correspond
to the *strftime*(3) format.

## menu-section-entry-format
Default: _%5.0u @ %t_. Format of the section list entries. Specifiers are as
follows:

	_i_	index number;++
_u_	unread items count;++
_t_	section title.

## menu-feed-entry-format
Default: _%5.0u │ %t_. Format of the feed list entries. Specifiers are as
follows:

	_i_	index number;++
_u_	unread items count;++
_l_	feed link;++
_t_	feed name if set, feed link otherwise.

## menu-item-entry-format
Default: _" %u │ %d │ %o"_. Format of the item list entries. Specifiers are
as follows:

	_i_	index number;++
_u_	"N" if item is unread, " " otherwise;++
_d_	update date formatted according to *list-entry-date-format*;++
_D_	publication date formatted according to *list-entry-date-format*;++
_l_	item link;++
_t_	item title;++
_o_	item title if set, item link otherwise;++
_L_	feed link;++
_T_	feed title;++
_O_	feed title if set, feed link otherwise.

## menu-explore-item-entry-format
Default: _" %u │ %d │ %-28O │ %o"_. Format of the item list entries in
explore mode. Specifiers are the same as in *menu-item-entry-format*.

## sections-menu-paramount-explore
Default: _false_. Enables explore mode in sections menu by default.

## feeds-menu-paramount-explore
Default: _false_. Enables explore mode in feeds menu by default.

## mark-item-unread-on-change (*)
Default: _false_. Mark every item that changes on a feed update as unread.

## mark-item-read-on-hover (*)
Default: _false_. Mark every item that gets selected as read.

## database-batch-transactions
Default: _true_. Apply all changes to the *database* file in one big transaction
after all feed updates have finished instead of using a separate transaction for
each feed update. This improves update performance a lot with the downside that
fetched content will not be saved if you quit Newsraft before update finishes.

## analyze-database-on-startup
Default: _true_. Run "ANALYZE" SQLite command on the database every time you
start Newsraft. It gathers statistics about database and uses it to optimize
some queries making runtime faster.

## clean-database-on-startup
Default: _false_. Run "VACUUM" SQLite command on the database every time you
start Newsraft. It rebuilds the database file by packing it into a minimal
amount of disk space. This can significantly increase startup time.

## download-timeout (*)
Default: _20_. Maximum time in seconds that you allow Newsraft to download one
feed. Setting to _0_ disables the timeout.

## download-speed-limit (*)
Default: _0_. Maximum download speed in kilobytes per second (kB/s). Setting to
_0_ disables the limit.

## download-max-host-connections
Default: _0_. Maximum amount of simultaneously open connections Newsraft may
hold a single host. If set to _0_, there is no limit.

## user-agent (*)
Default: _auto_. User-Agent header sent with download requests. If it is set to
_"auto"_, Newsraft will generate it according to the following format:

	_"newsraft/"_ + NEWSRAFT_VERSION + _" ("_ + OS_NAME + _")"_

OS_NAME shouldn't be a matter of privacy concern, because on most systems it
contains nothing more like _"Linux"_ or _"Darwin"_. If you want to be sure of
this, check Newsraft log to see how *user-agent* is set at startup.

If set to _""_, User-Agent header will not be sent.

## respect-ttl-element (*)
Default: _true_. Prevents too frequent updates for some feeds. The limit is set
by the creators of the feeds in order to save traffic and resources for a very
rarely updated feeds. Disabling it is strongly discouraged.

## respect-expires-header (*)
Default: _true_. Prevents feed updates until the expiration date of the
previously downloaded information in order to save traffic and resources.
Disabling it is strongly discouraged.

## send-if-none-match-header (*)
Default: _true_. Sends an entity tag corresponding to the previously downloaded
information. If the server from which the feed is downloaded contains
information with the same tag, then in order to save traffic and resources, it
will reject the download request. Disabling it is strongly discouraged.

## send-if-modified-since-header (*)
Default: _true_. Sends a date corresponding to the last modification of
previously downloaded information. If the server from which the feed is
downloaded contains information with the same modification date, then in order
to save traffic and resources, it will reject the download request. Disabling it
is strongly discouraged.

# COLOR SETTINGS

Color settings are the same settings as above, but they take two color words
(foreground and background) and optional attribute words. Available colors are
_default_, _black_, _red_, _green_, _yellow_, _blue_, _magenta_, _cyan_, _white_
and _colorN_ (_N_ can be a number from _0_ to _255_). Available attributes are
_bold_, _italic_ and _underlined_.

|[ Color setting
:[ Default value
|[ *color-status*
:[ _green default bold_
|[ *color-status-info*
:[ _cyan default bold_
|[ *color-status-fail*
:[ _red default bold_
|[ *color-list-item*
:[ _default default_
|[ *color-list-item-unread*
:[ _yellow default_
|[ *color-list-item-important*
:[ _magenta default_
|[ *color-list-feed*
:[ _default default_
|[ *color-list-feed-unread*
:[ _yellow default_
|[ *color-list-feed-failed*
:[ _red default_
|[ *color-list-section*
:[ _default default_
|[ *color-list-section-unread*
:[ _yellow default_
|[ *color-list-section-failed*
:[ _red default_

# ACTIONS

|[ Actions
:[ Keys
|[ *select-next*
:[ _j_, _KEY_DOWN_, _^E_
|[ *select-prev*
:[ _k_, _KEY_UP_, _^Y_
|[ *select-next-page*
:[ _space_, _^F_, _KEY_NPAGE_
|[ *select-next-page-half*
:[ _^D_
|[ *select-prev-page*
:[ _^B_, _KEY_PPAGE_
|[ *select-prev-page-half*
:[ _^U_
|[ *select-first*
:[ _g_, _KEY_HOME_
|[ *select-last*
:[ _G_, _KEY_END_
|[ *jump-to-next*
:[ _J_
|[ *jump-to-prev*
:[ _K_
|[ *jump-to-next-unread*
:[ _n_
|[ *jump-to-prev-unread*
:[ _N_
|[ *jump-to-next-important*
:[ _p_
|[ *jump-to-prev-important*
:[ _P_
|[ *next-error*
:[ _e_
|[ *prev-error*
:[ _E_
|[ *goto-feed*
:[ _\*_
|[ *shift-west*
:[ _,_
|[ *shift-east*
:[ _._
|[ *shift-reset*
:[ _<_
|[ *sort-by-time*
:[ _t_
|[ *sort-by-rowid*
:[ _w_
|[ *sort-by-unread*
:[ _u_
|[ *sort-by-initial*
:[ _z_
|[ *sort-by-alphabet*
:[ _a_
|[ *sort-by-important*
:[ _i_
|[ *enter*
:[ _enter_, _l_, _KEY_ENTER_, _KEY_RIGHT_
|[ *reload*
:[ _r_
|[ *reload-all*
:[ _R_, _^R_
|[ *mark-read*; *jump-to-next*
:[ _d_
|[ *mark-unread*; *jump-to-next*
:[ _D_
|[ *mark-read-all*
:[ _A_
|[ *mark-unread-all*
:[ (not set)
|[ *mark-important*
:[ _f_
|[ *mark-unimportant*
:[ _F_
|[ *toggle-explore-mode*
:[ _tab_
|[ *view-errors*
:[ _v_
|[ *open-in-browser*
:[ _o_
|[ *copy-to-clipboard*
:[ _y_, _c_
|[ *start-search-input*
:[ _/_
|[ *clean-status*
:[ _escape_
|[ *navigate-back*
:[ _h_, _backspace_, _KEY_LEFT_, _KEY_BACKSPACE_
|[ *quit*
:[ _q_
|[ *quit-hard*
:[ _Q_

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

*mpv*(1), *feh*(1), *sqlite3*(1), *regex*(7), *ssh*(1), *strftime*(3), *ncurses*(3), *libcurl*(3), *libcurl-env*(3)

# BUGS

Don't be ridiculous...

# AUTHORS

Grigory Kirillov and contributors
