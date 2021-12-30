FEEDEATER(1)

# NAME

feedeater - a feed reader for terminal

# SYNOPSIS

*feedeater* [-f _feeds_file_] [-c _config_file_] [-d _data_file_] [-l _log_file_]

*feedeater* -h

*feedeater* -v

# DESCRIPTION

Feedeater is a piece of software that aggregates syndicated web content such
as blogs, newspapers and podcasts in one location for easy access from your
terminal via comfy curses interface.

Feedeater is greatly inspired by *newsboat*(1).

# OPTIONS

\-f _feeds_file_
	Force use of _feeds_file_ as feeds file.

\-c _config_file_
	Force use of _config_file_ as config file.

\-d _data_file_
	Force use of _data_file_ as data file.

\-l _log_file_
	Write log information to _log_file_.

\-h
	Print usage information and quit with 0 exit code.

\-v
	Print version and quit with 0 exit code.

# CONFIGURATION

## FEEDS FILE

TODO

Search order:
. _$XDG_CONFIG_HOME_/feedeater/feeds
. _$HOME_/.config/feedeater/feeds
. _$HOME_/.feedeater/feeds

## CONFIG FILE

Config file is used to override default settings of feedeater, such as
maximum number of items in one feed, action key bindings or set of line
breaking characters. Complete list of configurable options can be found
in *SETTINGS* section. Presence of config file is totally optional and
feedeater will work without it just fine.

Search order:
. _$XDG_CONFIG_HOME_/feedeater/config
. _$HOME_/.config/feedeater/config
. _$HOME_/.feedeater/config

## DATABASE FILE

It stores everything you download from feeds in *sqlite3*(1) database format.

Search order:
. _$XDG_DATA_HOME_/feedeater/feedeater.sqlite3
. _$HOME_/.local/share/feedeater/feedeater.sqlite3

# SETTINGS

_set-entry-format_ (default: *"%4.0u │ %t"*)
	Format of set entry.

_item-entry-format_ (default: *" %u │ %t"*)
	Format of item entry.

_date-format_ (default: *"%a, %d %b %Y %H:%M:%S %z"*)
	Format of date.

_break-at_ (default: *" \\t!@\*-+;:,./?"*)
	Characters that break the lines.

_feed-items-limit_ (default: *0*)
	Shows how many items one feed can store. Zero means that it has no limits.

_reload-at-startup_ (default: *false*)
	If set to *true*, automatically updates all feeds at startup.

TODO

# ENVIRONMENT

## XDG_CONFIG_HOME

Directory for applications to store their configuration.

## XDG_DATA_HOME

Directory for applications to store their data.

## HOME

Home directory of your user.

# AUTHOR

Grigory Kirillov
