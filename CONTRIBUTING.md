# Dependency description

Feedeater uses [ncurses](https://invisible-island.net/ncurses) to draw interface, [curl](https://curl.se) to download feeds, [Expat](https://libexpat.github.io) to process XML and [SQLite](https://www.sqlite.org) to store data.

# Code guidelines

## Naming convention

Use underscores to separate words in names (variables, functions, structures, etc)

For example:

```
static size_t sets_count = 0;
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
