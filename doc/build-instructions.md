## Obtaining dependencies

| Operating system | Command                                                                                     |
|------------------|---------------------------------------------------------------------------------------------|
| Alpine Linux     | `apk add build-base curl-dev expat-dev gumbo-parser-dev sqlite-dev`                         |
| Arch Linux       | `pacman -S base-devel curl expat gumbo-parser sqlite`                                       |
| Source Mage      | `cast curl expat gumbo-parser sqlite`                                                       |
| Debian/Ubuntu    | `apt install build-essential libcurl4-openssl-dev libexpat-dev libgumbo-dev libsqlite3-dev` |
| Fedora Linux     | `dnf install gcc make libcurl-devel expat-devel gumbo-parser-devel sqlite-devel`            |
| Void Linux       | `xbps-install base-devel libcurl-devel expat-devel gumbo-parser-devel sqlite-devel`         |
| OpenBSD          | `pkg_add curl gumbo sqlite`                                                                 |
| Haiku            | `pkgman install curl_devel expat_devel gumbo_devel sqlite_devel`                            |
| macOS            | `brew install gumbo-parser`                                                                 |

## Compilation

| Operating system      | Command                                                                             |
|-----------------------|-------------------------------------------------------------------------------------|
| General Unix-like     | `make`                                                                              |
| OpenBSD               | `make CFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"`                     |
| macOS (Apple Silicon) | `make CFLAGS="-I/opt/homebrew/include" LDFLAGS="-L/opt/homebrew/lib"`             |
| macOS (Intel)         | `make CFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"`                   |

## Examination

```
./newsraft
```

## Installing (run as root)

```
make install
```
