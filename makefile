.POSIX:
.PHONY: all install install-newsraft install-man install-icon install-desktop install-examples man html clean check gperf cppcheck clang-tidy

CC            = cc
CFLAGS        = -O3
LDFLAGS       =
CURL_CFLAGS   = `pkg-config --cflags libcurl  2>/dev/null`
CURL_LIBS     = `pkg-config --libs   libcurl  2>/dev/null || echo '-lcurl'`
EXPAT_CFLAGS  = `pkg-config --cflags expat    2>/dev/null`
EXPAT_LIBS    = `pkg-config --libs   expat    2>/dev/null || echo '-lexpat'`
GUMBO_CFLAGS  = `pkg-config --cflags gumbo    2>/dev/null`
GUMBO_LIBS    = `pkg-config --libs   gumbo    2>/dev/null || echo '-lgumbo'`
SQLITE_CFLAGS = `pkg-config --cflags sqlite3  2>/dev/null`
SQLITE_LIBS   = `pkg-config --libs   sqlite3  2>/dev/null || echo '-lsqlite3'`
PTHREAD_LIBS  = -lpthread
# for static linking
#LDFLAGS       = -static
#CURL_LIBS     = -lcurl -lbrotlidec -lbrotlienc -lbrotlicommon -lssl -lcrypto -lnghttp2 -lz
AUXCFLAGS     = $(CURL_CFLAGS) $(EXPAT_CFLAGS) $(GUMBO_CFLAGS) $(SQLITE_CFLAGS)
FEATURECFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700
LDLIBS        = $(CURL_LIBS) $(EXPAT_LIBS) $(GUMBO_LIBS) $(SQLITE_LIBS) $(PTHREAD_LIBS)
DESTDIR       =
PREFIX        = /usr/local
BINDIR        = $(PREFIX)/bin
MANDIR        = $(PREFIX)/share/man
ICONSDIR      = $(PREFIX)/share/icons/hicolor/scalable/apps
DESKTOPDIR    = $(PREFIX)/share/applications
EXAMPLES_DIR  = $(PREFIX)/share/newsraft/examples

all: newsraft

install: install-newsraft install-man install-icon install-examples

install-newsraft:
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m755 newsraft $(DESTDIR)$(BINDIR)/.

install-man:
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	install -m644 doc/newsraft.1 $(DESTDIR)$(MANDIR)/man1/.

install-icon:
	mkdir -p $(DESTDIR)$(ICONSDIR)
	install -m644 doc/newsraft.svg $(DESTDIR)$(ICONSDIR)/.

install-desktop:
	mkdir -p $(DESTDIR)$(DESKTOPDIR)
	install -m644 doc/newsraft.desktop $(DESTDIR)$(DESKTOPDIR)/.

install-examples:
	mkdir -p $(DESTDIR)$(EXAMPLES_DIR)
	install -m644 doc/examples/feeds $(DESTDIR)$(EXAMPLES_DIR)/.
	install -m644 doc/examples/config $(DESTDIR)$(EXAMPLES_DIR)/.

newsraft:
	$(CC) -std=c99 $(CFLAGS) $(AUXCFLAGS) $(FEATURECFLAGS) -Isrc $(LDFLAGS) -o $@ src/newsraft.c $(LDLIBS)

libnewsraft.so:
	$(CC) -std=c99 -shared $(CFLAGS) $(AUXCFLAGS) $(FEATURECFLAGS) -Isrc $(LDFLAGS) -o $@ src/newsraft.c $(LDLIBS)

test-program:
	$(CC) -std=c99 $(CFLAGS) $(AUXCFLAGS) $(FEATURECFLAGS) -Isrc -o newsraft-test $(TEST_FILE) -L. -lnewsraft

gperf:
	gperf -m 1000 -I -t -F ,0,NULL,NULL < src/parse_xml/gperf-data.in > src/parse_xml/gperf-data.c

man:
	scdoc < doc/newsraft.scd > doc/newsraft.1

html:
	mandoc -T html ./doc/newsraft.1 > doc/newsraft.html
	sed -i 's/<body>/<body style="color: #BBB; background: #111; width: 100vmin; margin-left: auto; margin-right: auto;">/' doc/newsraft.html

check:
	./tests/run-check.sh

clean:
	rm -rf newsraft newsraft-test newsraft-test-log newsraft-test-feeds newsraft-test-database* libnewsraft.so flog vlog

cppcheck:
	find src -name "*.c" -exec cppcheck -q --enable=warning,performance,portability '{}' ';'

clang-tidy:
	clang-tidy --checks='-clang-analyzer-security.insecureAPI.*' $$(find src -name '*.c') -- $(FEATURECFLAGS) -Isrc

compile_commands.json: clean
	bear -- $(MAKE)
