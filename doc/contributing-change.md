# Dependency description

Newsraft uses [ncurses](https://invisible-island.net/ncurses) to draw user interface, [SQLite](https://www.sqlite.org) to store data, [curl](https://curl.se) to download feeds, [Expat](https://github.com/libexpat/libexpat) to parse XML, [YAJL](https://github.com/lloyd/yajl) to parse JSON and [Gumbo](https://github.com/google/gumbo-parser) to parse HTML.

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

Every control statement must be followed by parentheses to describe compound statement even if it has only one command. The first parenthesis must be on the same line with the control expression, and the closing parenthesis must be on a separate line.

For example:

```
for (size_t i = 0; i < items_count; ++i) {
	mark_item_read(i);
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
