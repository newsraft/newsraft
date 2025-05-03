# Comparison of Newsraft and Newsboat

Due to Newsraft's endeavor to be simpler than Newsboat, some design choices were
made differently than in Newsboat. The main differences are listed below, so if
you're considering switching from Newsboat to Newsraft, it's advised to examine them.

## TL;DR

| Criterion                                     | Newsraft                        | Newsboat               |
|:----------------------------------------------|:--------------------------------|:-----------------------|
| Feeds grouping                                | Sections                        | Query feeds            |
| Parallel downloads                            | +                               | +                      |
| Multiple actions key bindings                 | +                               | +                      |
| Interactive content pager                     | +                               | +                      |
| Built-in HTML renderer                        | +                               | +                      |
| Sorting                                       | +                               | +                      |
| Automatic updates                             | +                               | +                      |
| Item limits                                   | +                               | +                      |
| Per-feed settings                             | +                               | -                      |
| Command feeds                                 | `$(cmd arg1 arg2)`              | `"exec:cmd arg1 arg2"` |
| Scripting capabilities                        | `newsraft -e ACTION`            | `newsboat -x ACTION`   |
| Download manager                              | -                               | `podboat`              |
| Integration with third-party services         | -                               | +                      |
| Internal command-line                         | -                               | +                      |
| Programming languages used                    | C99                             | C++17, Rust            |
| [Source lines of code](#source-lines-of-code) | ~9k                             | ~44k                   |

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

## Per-feed and per-section settings

Newsboat [doesn't support individual configuration for feeds](https://github.com/newsboat/newsboat/issues/83).
Newsraft, on the other hand, supports many settings to be set on individual
feeds and sections. For example, you can do something like this:

```
http://example.org/feed1.xml "Phonk"      < reload-period 120
http://example.org/feed2.xml "Weather"    < proxy socks5h://127.0.0.1:9050

@ News                                    < reload-period 60
http://example.org/feed3.xml "World news" < reload-period 0; item-limit 50
http://example.org/feed4.xml "Tech news"  < suppress-errors
```

## Vim-like bindings by default

You don't need to configure anything related to bindings if you are familiar
with the Vim text editor. Also Newsraft borrows from Vim the ability to specify
index for an entry on which you want to perform an action. So, for example, to
open the 9th link in the browser, you need to press `9` followed by the key of
the command to open the browser (`o` by default).

## Faster feed updates

Newsraft uses streaming parsers to process feeds and parses feed elements with
O(1) time complexity by using perfect hashing, while Newsboat uses DOM parsers
and parses feed elements with O(n) time complexity. Newsboat takes
object-oriented approach to processing feeds which can sometimes result in
[a huge memory footprint](https://github.com/newsboat/newsboat/issues/977),
while Newsraft represents feeds as simple structures of strings and numbers.

## Source lines of code

This is how SLOC is calculated. As you can see, Newsraft is roughly 5 times smaller than Newsboat:

```
~/src/newsraft > git show -s --pretty=format:"%H %ad"
ca1a6bfcd43c3cc6c7c68076b46a8346863afa02 Sat May 3 22:42:07 2025 +0300
~/src/newsraft > find src -regex ".*\.\(c\|h\)" -exec awk NF {} + | wc -l
8909
```
```
~/src/newsboat > git show -s --pretty=format:"%H %ad"
0141cfc90e4150f18aeefa7c478da0fdf9a1ab38 Thu May 1 20:51:09 2025 +0200
~/src/newsboat > find src rust rss filter include newsboat.cpp podboat.cpp config.h -regex ".*\.\(cpp\|h\|rs\)" -exec awk NF {} + | wc -l
44508
```
