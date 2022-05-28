# Dependency description

Newsraft uses [ncurses](https://invisible-island.net/ncurses) to draw user interface, [SQLite](https://www.sqlite.org) to store data, [curl](https://curl.se) to download feeds, [Expat](https://github.com/libexpat/libexpat) to parse XML, [YAJL](https://github.com/lloyd/yajl) to parse JSON and [Tidy](http://www.html-tidy.org) to parse HTML.

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
