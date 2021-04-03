# feedeater

simple feed reader for terminal (work in progress)

## Description

Command line based application for reading feed channels in RSS & Atom formats. User interface is implemented in ncurses. Configuration is done through files.

Project **non-goals** are:

* support for third-party feed services

* utilization of large number of libraries

## Dependencies

* ncurses >= 6.2

* libcurl >= 7.75.0

* expat >= 2.2.10

* gcc >= 10.2.0 (build-time)

* ninja >= 1.10.2 (build-time)

## Building

	ninja

## Configuration

### Files

* `feeds` is file that contains list of feed URLs you want to process (mandatory)

* `config` is file that contains custom settings (optional)

#### feeds

See `feeds` file sample in `examples` directory.

#### config

See `config` file sample in `examples` directory.

### Search order for essential files

Directory with configuration files is searched in the following order:

1. **$FEEDEATER_CONFIG**

2. **$XDG_CONFIG_HOME**/feedeater

3. **$HOME**/.config/feedeater

4. **$HOME**/.feedeater

5. /etc/feedeater

Directory with data files is searched in the following order:

1. **$FEEDEATER_DATA**

2. **$XDG_DATA_HOME**/feedeater

3. **$HOME**/.local/share/feedeater

## License

See `LICENSE` file in repository root.
