## Project goals

Main objective of Newsraft program is to do almost everything
[Newsboat](https://newsboat.org) does, but simpler and lighter.

## Differences from Newsboat

#### Grouping of feeds into updatable sections instead of meta-feeds

In Newsraft you can update multiple feeds grouped in one section, while in
Newsboat it is impossible with meta-feeds due to their filtering purpose.

#### Simpler command bindings

You can bind any key to execute command on the feed item with for example:

```
bind m exec mpv %l
```

No more stuff like:

```
macro m set browser mpv; open-in-browser; set browser w3m
```

#### Faster feed updates

Newsraft uses streaming parsers to process feeds, while Newsboat uses DOM
parsers. Also Newsraft takes simpler approach to data structures, which also
gives a small performance boost.

#### 4 times smaller codebase

Newsraft's source lines of code count is around 9k, while Newsboat's is around
36k.

#### Omitted functionality

Some features will never be landed in Newsraft just because of its goal to be
simpler and lighter than Newsboat. Notably, feed entries filtering, feed
transformations, podcasts management and integration with third-party services
are bypassed by Newsraft - consider using Newsboat if you need any of those.
