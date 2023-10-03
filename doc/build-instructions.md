## Obtaining dependencies

| Operating system | Command                                                                                           |
|------------------|---------------------------------------------------------------------------------------------------|
| Alpine Linux     | `apk add curl-dev expat-dev gumbo-parser-dev ncurses-dev sqlite-dev yajl-dev`                     |
| Arch Linux       | `pacman -S curl expat gumbo-parser ncurses sqlite yajl`                                           |
| Source Mage      | `cast curl expat gumbo-parser ncurses sqlite yajl`                                                |
| Ubuntu           | `apt install curl expat libgumbo-dev libncursesw6 libsqlite3-dev libyajl-dev`                     |
| Void Linux       | `xbps-install libcurl-devel expat-devel gumbo-parser-devel ncurses-devel sqlite-devel yajl-devel` |
| OpenBSD          | `pkg_add curl gumbo sqlite libyajl`                                                               |

## Compilation

| Operating system | Command                                                                             |
|------------------|-------------------------------------------------------------------------------------|
| Generic Linux    | `make`                                                                              |
| Ubuntu           | `make CFLAGS="-DCURL_WRITEFUNC_ERROR=0xFFFFFFFF"`                                   |
| OpenBSD          | `make CFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib"`                     |
| macOS            | `make CFLAGS="-DCURL_WRITEFUNC_ERROR=0xFFFFFFFF" -D_C99_SOURCE -D_DARWIN_C_SOURCE"` |

## Examination

```
./newsraft
```

## Installing (run as root)

```
make install
```
