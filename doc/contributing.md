# Dependency description

Feedeater uses [ncurses](https://invisible-island.net/ncurses) to draw interface, [curl](https://curl.se) to download feeds, [Expat](https://libexpat.github.io) to process XML, [SQLite](https://www.sqlite.org) to store data and [Tidy](http://www.html-tidy.org) to process HTML.

# Code guidelines

## Indentation characters

One tab per one level of indentation.

## Naming convention

Use underscores to separate words in names (variables, functions, structures, etc)

For example:

```
static size_t sets_count = 0;
```

## Control statements decoration

Every control statement must be followed by parentheses to describe compound statement even if it has only one command. The first parenthesis must be on the same line with the control expression, and the closing parenthesis must be on a separate line.

For example:

```
for (size_t i = 0; i < items_count; ++i) {
	mark_item_read(i);
}
```

## Function definitions

Every function definition in the source code has to be of the form:

```
SPECIFIERS TYPE
NAME(ARGUMENTS)
{
	BODY
}
```

For example:

```
static void
redraw_items_windows(void)
{
	clear();
	refresh();
	show_items();
}
```
