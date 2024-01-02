# Comparison of Newsraft and Newsboat

Due to Newsraft's endeavor to be simpler than Newsboat, some design choices were
made differently than in Newsboat. The main differences are listed below, so if
you're considering switching from Newsboat to Newsraft, it's advised to examine them.

## TL;DR

| Criterion                        | Newsraft                        | Newsboat               |
|:---------------------------------|:--------------------------------|:-----------------------|
| Parallel downloads               | +                               | +                      |
| Multiple actions key bindings    | +                               | +                      |
| Interactive content pager        | +                               | +                      |
| Built-in HTML renderer           | +                               | +                      |
| Sorting                          | +                               | +                      |
| Feeds grouping                   | Sections                        | Query feeds            |
| Automatic updates                | Square brackets in `feeds` file | `reload-time` setting  |
| Feed capacity limits             | Curly brackets in `feeds` file  | `max-items` setting    |
| Command feeds                    | `$(cmd arg1 arg2)`              | `"exec:cmd arg1 arg2"` |
| Download management              | -                               | +                      |
| Support for third-party services | -                               | +                      |
| Internal command-line            | -                               | +                      |
| Programming languages used       | C99                             | C++11, Rust, Coco/R    |
| Source lines of code             | ~9k                             | ~40k                   |

Feel free to submit an issue if you think that table above contains outdated information.

## Grouping of feeds into sections instead of query feeds

Sections are needed to organize feeds in groups to be able to process them in
bulk. They are like directories, but for feeds. You can update, explore and
set auto updates for sections - this will all be applied to belonging feeds.

This makes Newsraft very different from Newsboat as latter uses query feeds
for that purpose which are based on the comprehensive filter language - it
brings many possibilities, but also introduces some significant limitations
(for example, query feeds
[can't be reloaded](https://github.com/newsboat/newsboat/issues/978) and they
have to be constantly populated which may be pretty slow for hundreds of feeds).

## Simpler command bindings

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

## Vim-like bindings by default

You don't need to configure anything related to bindings if you are familiar
with the Vim text editor. Also Newsraft borrows from Vim the ability to specify
index for an entry on which you want to perform an action. So, for example, to
open the 9th link in the browser, you need to press `9` followed by the key of
the command to open the browser (`o` by default).

## Faster feed updates

Newsraft uses streaming parsers to process feeds, while Newsboat uses DOM
parsers. Also Newsraft takes simpler approach to data structures, which also
gives a small performance boost.

## Flexible automatic updates and capacity limits

In Newsboat you have settings for automatic updates and capacity limits but they
are applied to all feeds at once, while in Newsraft you can set auto update
timers (numbers in square brackets) and capacity limits (numbers in curly
brackets) for individual sections and feeds like this:

```
http://example.org/feed3.xml "Phonk" [60]
http://example.org/feed1.xml "Weather" {100}

@ News [30]
http://example.org/feed5.xml "World news" [0] {50}
http://example.org/feed4.xml "Tech news"
```

To set automatic updates or capacity limits for all feeds, just apply brackets
expression to Global section at the very beginning of the `feeds` file.
