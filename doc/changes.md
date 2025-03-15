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
