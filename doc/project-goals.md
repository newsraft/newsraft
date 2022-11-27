## Project goals

Main objective of the Newsraft program is to do almost everything
[Newsboat](https://newsboat.org) does, but simpler. Due to the project's
endeavor to be simple, some design choices were made differently than in
Newsboat. The main differences are listed below, so if you wanted to switch from
Newsboat to Newsraft, it is advised to examine them.

## Differences from Newsboat

#### Grouping of feeds into updatable sections instead of meta-feeds

In Newsraft you can update multiple feeds grouped in one section, while in
Newsboat it is impossible with meta-feeds due to their filtering purpose.

#### Simpler command bindings

You can bind any key to execute a command on the feed item with for example:

```
bind m exec mpv "%l"
```

No more stuff like:

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
