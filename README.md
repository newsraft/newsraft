# feedeater

simple feed reader for terminal (work in progress)

## Dependencies

* ncurses >= 6.2

* libcurl >= 7.75.0

* openssl >= 1.1.1.j

* libxml2 >= 2.9.10

* gcc >= 10.2.0 *(build-time)*

* ninja >= 1.10.2 *(build-time)*

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
