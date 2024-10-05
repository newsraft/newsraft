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
