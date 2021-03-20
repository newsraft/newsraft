# feedeater

simple feed reader for terminal (work in progress)

## Dependencies

* gcc >= 10.2.0

* ncurses >= 6.2

* libcurl >= 7.75.0

* libxml2 >= 2.9.10

TODO: lower the requirments

## Building

	ninja

## Configuration

Directory with configuration files is searched in the following order:

1. **$FEEDEATER_HOME**

2. **$XDG_CONFIG_HOME**/feedeater

3. **$HOME**/.config/feedeater

4. **$HOME**/.feedeater

5. /etc/feedeater
