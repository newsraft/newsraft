.POSIX:
.PHONY: all install install-newsraft install-man install-examples man html clean check cppcheck clang-tidy

CC            = cc
CFLAGS        = -O3
LDFLAGS       =
CURL_CFLAGS   = `pkg-config --cflags libcurl  2>/dev/null`
CURL_LIBS     = `pkg-config --libs   libcurl  2>/dev/null || echo '-lcurl'`
CURSES_CFLAGS = `pkg-config --cflags ncursesw 2>/dev/null`
CURSES_LIBS   = `pkg-config --libs   ncursesw 2>/dev/null || echo '-lncursesw'`
EXPAT_CFLAGS  = `pkg-config --cflags expat    2>/dev/null`
EXPAT_LIBS    = `pkg-config --libs   expat    2>/dev/null || echo '-lexpat'`
GUMBO_CFLAGS  = `pkg-config --cflags gumbo    2>/dev/null`
GUMBO_LIBS    = `pkg-config --libs   gumbo    2>/dev/null || echo '-lgumbo'`
SQLITE_CFLAGS = `pkg-config --cflags sqlite3  2>/dev/null`
SQLITE_LIBS   = `pkg-config --libs   sqlite3  2>/dev/null || echo '-lsqlite3'`
YAJL_CFLAGS   = `pkg-config --cflags yajl     2>/dev/null`
YAJL_LIBS     = `pkg-config --libs   yajl     2>/dev/null || echo '-lyajl'`
PTHREAD_LIBS  = -lpthread
# for static linking
#LDFLAGS       = -static
#CURL_LIBS     = -lcurl -lbrotlidec -lbrotlienc -lbrotlicommon -lssl -lcrypto -lnghttp2 -lz
AUXCFLAGS     = $(CURL_CFLAGS) $(CURSES_CFLAGS) $(EXPAT_CFLAGS) $(GUMBO_CFLAGS) $(SQLITE_CFLAGS) $(YAJL_CFLAGS)
LDLIBS        = $(CURL_LIBS) $(CURSES_LIBS) $(EXPAT_LIBS) $(GUMBO_LIBS) $(SQLITE_LIBS) $(YAJL_LIBS) $(PTHREAD_LIBS)
DESTDIR       =
PREFIX        = /usr/local
BINDIR        = $(PREFIX)/bin
MANDIR        = $(PREFIX)/share/man
EXAMPLES_DIR  = $(PREFIX)/share/newsraft/examples

all: newsraft

install: install-newsraft install-man install-examples

install-newsraft: newsraft
	install -Dm755 newsraft $(DESTDIR)$(BINDIR)/newsraft

install-man:
	install -Dm644 doc/newsraft.1 $(DESTDIR)$(MANDIR)/man1/newsraft.1

install-examples:
	install -Dm644 doc/examples/feeds $(DESTDIR)$(EXAMPLES_DIR)/feeds
	install -Dm644 doc/examples/config $(DESTDIR)$(EXAMPLES_DIR)/config

newsraft:
	$(CC) -std=c99 $(CFLAGS) $(AUXCFLAGS) -Isrc -D_XOPEN_SOURCE=700 -D_XOPEN_SOURCE_EXTENDED $(LDFLAGS) -o $@ src/newsraft.c $(LDLIBS)

libnewsraft.so:
	$(CC) -std=c99 -shared $(CFLAGS) $(AUXCFLAGS) -Isrc -D_XOPEN_SOURCE=700 -D_XOPEN_SOURCE_EXTENDED $(LDFLAGS) -o $@ src/newsraft.c $(LDLIBS)

man:
	scdoc < doc/newsraft.scd > doc/newsraft.1

html:
	mandoc -T html ./doc/newsraft.1 > doc/newsraft.html

clean:
	rm -rf newsraft tests/makefile tests/src tests/libnewsraft.so tests/a.out flog vlog

check:
	./tests/run-check.sh

cppcheck:
	find src -name "*.c" -exec cppcheck -q --enable=warning,performance,portability '{}' ';'

clang-tidy:
	clang-tidy --checks='-clang-analyzer-security.insecureAPI.*' $$(find src -name '*.c') -- -Isrc -D_XOPEN_SOURCE=700 -D_XOPEN_SOURCE_EXTENDED
