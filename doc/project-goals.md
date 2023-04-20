## Project goals

Main objective of the Newsraft program is to do almost everything
[Newsboat](https://newsboat.org) does, but simpler. Due to the project's
endeavor to be simple, some design choices were made differently than in
Newsboat. The main differences are listed below, so if you wanted to switch from
Newsboat to Newsraft, it is advised to examine them.

## Differences from Newsboat

#### Grouping of feeds into sections instead of query feeds

Sections are needed to organize feeds in groups to be able to process them in
bulk. They are like directories, but for feeds. You can update, explore and
set auto updates for sections - this will all be applied to belonging feeds.

This makes Newsraft very different from Newsboat as latter uses query feeds
for that purpose which are based on the comprehensive filter language - it
brings many possibilities, but also introduces some significant limitations.

#### Simpler command bindings

Two lines below add the ability to open links in `mpv` to the feed reader (first
line corresponds to Newsraft, second line corresponds to Newsboat). Newsboat
requires you to first press the macro prefix key (`,` by default) to execute
bound command, while Newsraft doesn't.

```
bind m exec mpv "%l"
```

```
macro m set browser mpv; open-in-browser; set browser elinks
```

#### Vim-like bindings by default

You don't need to configure anything related to bindings if you are familiar
with the Vim text editor. Also Newsraft borrows from Vim the ability to specify
index for an entry on which you want to perform an action. So, for example, to
open the 9th link in the browser, you need to press `9` followed by the key of
the command to open the browser (`o` by default).

#### Faster feed updates

Newsraft uses streaming parsers to process feeds, while Newsboat uses DOM
parsers. Also Newsraft takes simpler approach to data structures, which also
gives a small performance boost.

#### Fully concurrent updates

When updating feeds, Newsboat allows you to navigate the interface only if all
feeds are being updated. In Newsraft, on the other hand, all feed updates are
concurrent and even in the case of a single feed update you are free to navigate
the interface.

#### More flexible automatic updates

In Newsboat you have settings for automatic updates but they are applied to all
feeds at once, while in Newsraft you can set auto update counters (numbers in
curly braces below) for separate sections and feeds like that:

```
http://example.org/feed1.xml
http://example.org/feed2.xml {30}

@ Replies {20}
http://example.org/feed3.xml {10}
http://example.org/feed4.xml "Forum notifications"

@ News {60}
http://example.org/feed5.xml
http://example.org/feed6.xml "Local weather" {0}
```

With this you are able to update the feeds that are very important to you more
often, while other feeds may be updated less frequently or not automatically
updated at all.

#### Much simpler codebase

In Newsraft's codebase there are only C source files while in the repository of
Newsboat you will find C++, Rust, STFL and Coco/R files which are direct
dependencies of the project. Also it's worth mentioning that Newsboat's source
lines of code count is around 40k, while Newsraft's is around 9k.

#### Omitted functionality

Some features will never be landed in Newsraft just because of its goal to be
simpler and lighter than Newsboat. Notably, feed entries filtering, feed
transformations, podcasts management and integration with third-party services
are bypassed by Newsraft. You should consider using Newsboat if you need any of
these, because it does a fairly good job at implementing it all together.
