## Obtaining dependencies

| Operating system | Command                                                                                                                |
|------------------|------------------------------------------------------------------------------------------------------------------------|
| Alpine Linux     | `apk add build-base curl-dev expat-dev gumbo-parser-dev ncurses-dev sqlite-dev yajl-dev`                               |
| Arch Linux       | `pacman -S base-devel curl expat gumbo-parser ncurses sqlite yajl`                                                     |
| Source Mage      | `cast curl expat gumbo-parser ncurses sqlite yajl`                                                                     |
| Debian/Ubuntu    | `apt install build-essential libcurl4-openssl-dev libexpat-dev libgumbo-dev libncurses-dev libsqlite3-dev libyajl-dev` |
| Fedora Linux     | `dnf install gcc make libcurl-devel expat-devel gumbo-parser-devel ncurses-devel sqlite-devel yajl-devel`              |
| Void Linux       | `xbps-install base-devel libcurl-devel expat-devel gumbo-parser-devel ncurses-devel sqlite-devel yajl-devel`           |
| OpenBSD          | `pkg_add curl gumbo sqlite libyajl`                                                                                    |
| macOS            | `brew install gumbo-parser ncurses yajl`                                                                               |

## Compilation

| Operating system      | Command                                                                             |
|-----------------------|-------------------------------------------------------------------------------------|
| General Unix-like     | `make`                                                                              |
| OpenBSD               | `make CFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"`                     |
| macOS (Apple Silicon) | `make CFLAGS="-I/opt/homebrew/include"`                                             |
| macOS (Intel)         | `make CFLAGS="-I/usr/local/include"`                                                |

## Examination

```
./newsraft
```

## Installing (run as root)

```
make install
```
