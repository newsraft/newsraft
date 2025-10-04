# newsraft 0.33 "energizor"

i'd like to start a tradition of mentioning contributors to the release, so here we go:

* caveman (@caveman) thanks for #132, #227
* David Pedersen (@Limero) thanks for #216, #233, #234
* Tafnur (@tafnur) thanks for #224, #230, #231
* W4RH4WK (@W4RH4WK) thanks for #225, #226

this release is a little late because some good stuff has been brewing over the last few days

* add `read-on-arrival` setting
* add `scrollwrap` setting (#216)
* add `color-list-item-selected` setting (#132)
* add `color-list-feed-selected` setting (#132)
* add `color-list-section-selected` setting (#132)
* fix colorN value offset by 1 in color settings (#231)
* gracefully handle zero size state of terminal emulator (#218)
* apply search cumulatively instead of overwriting previous query
* don't remove trailing slashes from feed urls (#224)
* store feed urls without trailing slashes in the database (#224)
* let go of terminal control while executing commands (#225)
* add total items count specifier to `menu-feed-entry-format` (#234)
* provide update error for generator feeds on failed command
* use esc mode only when escape key is bound (#227)
* add items sorting by download time (#123)
* bind `?` to `exec man newsraft` by default

> if you are used to wrapping behavior of list menu jumps (e.g. `next-unread`),
> now you have to enable `scrollwrap` to make it wrapping just as in 0.32

_**aaaand, big thanks to everyone involved in Newsraft packaging <3**_

# newsraft 0.32 "conflagratio"

* add `toggle-read` action (#213)
* add `toggle-important` action (#213)
* add `global-section-hide` setting (#144)
* make `FEATURECFLAGS` in makefile universally correct for most platforms
* fix termbox2 behavior to handle `TERMINFO` environment variable as in ncurses (#212, [github](https://github.com/termbox/termbox2/pull/104))
* provide more log information during termbox2 initialization (#212)
* prefer a link to webpage instead of feed URL when converting relative item links to absolute notation

thank you to all of you who keep ports updated, i see your work <3 <3 <3

# newsraft 0.31 "way fare"

* dependency on `ncurses` is gone, it's not needed to build Newsraft anymore

so currently only 4 libraries are needed to build Newsraft: libcurl, libexpat,
libgumbo and libsqlite3. note that you don't need scdoc/mandoc during the build

* add `download-max-connections` setting (#187)
* add `ignore-no-color` setting (#204)
* add `sort-by-time-update` action (#185)
* add `sort-by-time-publication` action (#185)
* fallback to `open` in `open-in-browser-command` setting on macOS (#203)
* don't call `make clean` before running tests every time
* prevent rebuild in some cases when calling `make install`

and yet again, a low bow to every one of you who lead Newsraft ports!

# newsraft 0.30 "one cabbage a day and the doc's never away"

this is a big one. dear repository maintainers, here's an important heads up:

* dependency on `yajl` is gone, it's not needed to build Newsraft anymore
* requirements for `sqlite` are raised to 3.38.0. now we use its json facilities
* new metadata file is available for packaging on Linux: `doc/newsraft.desktop`

ok i hope these are visible enough. other substantial changes are:

* add `edit` action (#31, #117, #133)
* add `find` action (#31, #117, #133)
* add `user_data` column to feeds and items database tables (#31, #117, #133)

so now you have the ability to play with the database. one example use of
this is a custom tagging functionality:

```
# mark item "toWatch"
bind w edit UPDATE items SET user_data = json_set(IFNULL(user_data, '{}'), '$.toWatch', 1) WHERE @selected
```
```
# unmark item "toWatch"
bind W edit UPDATE items SET user_data = json_set(IFNULL(user_data, '{}'), '$.toWatch', 0) WHERE @selected
```
```
# find all "toWatch" items in the current context
bind f find json_extract(user_data, '$.toWatch') = 1
```

more details on how it all works are in man page. but now we continue:

* add `next-error` action
* add `prev-error` action
* add `convert-opml-to-feeds` scenario (argument for `-e`)
* add `convert-feeds-to-opml` scenario (argument for `-e`)
* add `database-batch-transactions` setting (#145)
* add REGEXP operator to `item-rule` setting
* report error when `item-rule` setting is invalid (#149)
* make items counting respect applied `item-rule` setting (#149)
* fallback to OSC 9 in `notification-command` setting (#153)
* fallback to OSC 52 in `copy-to-clipboard-command` setting (#147)
* rename `analyze-database-on-startup` setting to `database-analyze-on-startup`
* rename `clean-database-on-startup` setting to `database-clean-on-startup`
* clarify that only one specifier can be put per field in `item-content-format` (#184)
* delete `yajl` dependency, use `json_tree()` from `sqlite` to parse json

big shout out to package maintainers as always, you're the best guys ;)

# newsraft 0.29 "san dian yi xian"

from now on there's no global pager for status messages. status messages related
to each feed will be saved for each feed individually. if feed has error
messages, it will be painted in red, as will the sections containing it.

to view errors of individual feed, you need to hover over it and press
`view-errors` (`v` by default). if `view-errors` is invoked on section, it will
show all errors of failed feeds in the given section. if some feed is failing
too frequently and you don't want to see its errors, apply `suppress-errors`
setting to it.

default binding for `mark-read-all` action is changed to `A` key, because `^D`
is now occupied by a very neat `select-next-page-half` action just like in vim.

* add `suppress-errors` setting (#141)
* add `menu-section-sorting` setting (#138)
* add `menu-responsiveness` setting (#135)
* add `color-list-feed-failed` setting
* add `color-list-section-failed` setting
* add `view-errors` action (`v` key)
* add `sort-by-initial` action (`z` key)
* add `select-next-page-half` action (`^D` key)
* add `select-prev-page-half` action (`^U` key)
* delete `status-history-menu` action
* delete `status-messages-count-limit` setting
* make `mark-item-read-on-hover` setting scalable

glory to package maintainers! <3

# newsraft 0.28 "creeping train"

this one brings a long awaited feature: feeds filtering via `item-rule` setting.
i will go as far and just dump the setting description from the man page here:

> Item search condition when accessing database. This can be very useful in
> managing feeds with a heavy spam flow: you set a condition based on some
> parameters and only those entries that meet this condition will be shown in
> the feed. It's specified in SQL format. It probably only makes sense to set
> this setting for individual feeds, and not globally (see *FEEDS FILE* section
> to understand how). Available parameters: _guid_, _title_, _link_, _content_,
> _attachments_, _persons_, _publication_date_, _update_date_.

also color settings and notification command can be set for individual feeds now

happy new year to everyone and especially to package maintainers :^)

* add `item-rule` setting (#104)
* add `download-max-host-connections` setting (#120)
* add `sort-by-rowid` action (#123)
* make `notification-command` setting scalable (#130)
* make color settings scalable (#122)

# newsraft 0.27 "confusing query"

fixed erroneous logic of `item-limit` setting and split its special behavior into new `item-limit-unread` and `item-limit-important` settings. also now `[X]` and `{Y}` counters are no longer supported in the feeds file. `< reload-period X` should be used instead of `[X]` and `< item-limit Y` should be used instead of `{Y}`

sorry to bother you with the second update in such a short time!

* add `item-limit-unread` setting
* add `item-limit-important` setting
* drop support for bracketed [update timers] and {item limits} in feeds file (#94)

# newsraft 0.26 "delicious tvorozhok"

apart from other nice things, threading logic is changed completely to make use of caching for dns, connections, tls sessions, ca certs. now there's just 4 threads in the process and `update-threads-count` is gone.

let the maintainers cook <3

* add tab characters rendering in plain text content (#109)
* add -e option to execute certain actions without getting into the menus (#45)
* add support for rdf-namespaced rss 1.0 feeds (#112)
* add support for relative links in feed elements (#113)
* add support for yyyy-mm-dd and yyyy/mm/dd dates in feeds
* add support for style attributes within cells of html tables
* make items menu regenerate upon returning from items menu obtained via `goto-feed` action
* make date parsing less strict
* report exit status of failed shell commands
* fix storing http headers behavior according to rfc9111 (4.3.4)
* fix difference in compiler flags between primary executable and test programs (#114)
* delete `update-threads-count` setting
* provide 2 woodpecker ci jobs for alpine and arch linuxes
* provide a change log
