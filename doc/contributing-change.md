# Introduction

First of all, thank you for your desire to contribute some code! But before
you start, there are a few things that need to be clarified.

Since this project is designed to be very stable and maintainable over a long
time, every decision will be annoyingly thought out and meticulously tested.
So in order to not waste our precious time, make sure that the proposed
functionality doesn't conflict with the goals of the project (see
[doc/project-goals.md](https://codeberg.org/grisha/newsraft/src/branch/main/doc/project-goals.md))
and its source code conforms to C99 language standard and project's code
guidelines (see below).

In terms of dependencies, Newsraft is very unpretentious - it uses
[ncurses](https://invisible-island.net/ncurses) to draw user interface,
[SQLite](https://www.sqlite.org) to store data,
[curl](https://curl.se) to download feeds,
[Expat](https://github.com/libexpat/libexpat) to parse XML,
[YAJL](https://github.com/lloyd/yajl) to parse JSON and
[Gumbo](https://github.com/google/gumbo-parser) to parse HTML.
To build the project you will also need any C compiler that supports the C99
standard, any POSIX-compliant Make and, in case you want to generate a man page
(of course you do), [scdoc](https://git.sr.ht/~sircmpwn/scdoc). If your
contribution involves the introduction of a new dependency in the project, then
there must be a very very very good reason for this.

# Code guidelines

## Indentation characters

One tab per one level of indentation.

## Naming convention

### Variables, functions, structures, etc

Stick to the snake case (use underscores to separate words).

### Files

Stick to the kebab case (use dashes to separate words).

### Directories

Every directory in the `src` directory is named after the function that the files in that directory implement.

## Control statements

Every control expression in the control statement must be followed by a curly brace to describe compound statement even if it has only one command. The first curly brace must be on the same line with the control expression, and the closing curly brace must be on a separate line. The exception is when the condition in the control expression is longer than 120 characters, then it needs to be continued in the next line and the first curly brace placed on its own line.

For example:

```
for (size_t i = 0; i < items_count; ++i) {
	mark_item_read(i);
}
```

```
for (size_t i = 0, j = 1; (j < enormously_long_variable) \
	&& ((i < very_long_variable) || (i > another_very_long_variable)); ++i)
{
	j += count_important_stuff(i);
}
```

Note that it is forbidden to use assignment operator within conditions of `if`, `while` and `switch` statements.

## Function definitions

Every function name in a function definition must start on a new line, and the curly braces that enclose the body of the function must be on their own lines. By the way, this makes it very easy to search for functions throughout the codebase with `grep -oR '^[a-z].*(.*)'` command.

For example:

```
static void
character_data_handler(void *userData, const XML_Char *s, int len)
{
	struct stream_callback_data *data = userData;
	catas(data->value, s, len);
}
```
