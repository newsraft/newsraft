.POSIX:
.PHONY: all install install-newsraft install-doc install-examples doc clean check cppcheck

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
AUX_CFLAGS    = $(CURL_CFLAGS) $(CURSES_CFLAGS) $(EXPAT_CFLAGS) $(GUMBO_CFLAGS) $(SQLITE_CFLAGS) $(YAJL_CFLAGS)
LDLIBS        = $(CURL_LIBS) $(CURSES_LIBS) $(EXPAT_LIBS) $(GUMBO_LIBS) $(SQLITE_LIBS) $(YAJL_LIBS) $(PTHREAD_LIBS)
DESTDIR       =
PREFIX        = /usr/local
BINDIR        = $(PREFIX)/bin
MANDIR        = $(PREFIX)/share/man
EXAMPLES_DIR  = $(PREFIX)/share/newsraft/examples

# find src -name '*.c' | sort | sed 's/\.c/.o/' | tr '\n' ' '
OBJECTS = src/binds.o src/commands.o src/dates.o src/db.o src/db-items.o src/feeds.o src/feeds-parse.o src/interface.o src/interface-colors.o src/interface-input.o src/interface-list.o src/interface-list-pager.o src/interface-status.o src/interface-status-pager.o src/items.o src/items-list.o src/items-metadata.o src/items-metadata-content.o src/items-metadata-links.o src/items-metadata-persons.o src/items-pager.o src/load_config/config.o src/load_config/config-copytoclipboardcommand.o src/load_config/config-parse.o src/load_config/config-parse-colors.o src/load_config/config-parse-input.o src/load_config/config-useragent.o src/load_config/load_config.o src/log.o src/newsraft.o src/path.o src/prepare_to_render_data/prepare-text-html.o src/prepare_to_render_data/prepare-text-plain.o src/prepare_to_render_data/prepare_to_render_data.o src/render-block.o src/render_data/line.o src/render_data/render_data.o src/render_data/render-text-html.o src/render_data/render-text-html-table.o src/sections.o src/signal.o src/string.o src/string-serialize.o src/update_feed/download.o src/update_feed/insert_feed/insert_feed.o src/update_feed/insert_feed/insert-feed-data.o src/update_feed/insert_feed/insert-item-data.o src/update_feed/parse_json/engage-json-parser.o src/update_feed/parse_xml/engage-xml-parser.o src/update_feed/parse_xml/xml-common.o src/update_feed/parse_xml/xml-handler-atom10.o src/update_feed/parse_xml/xml-handler-dublincore.o src/update_feed/parse_xml/xml-handler-georss.o src/update_feed/parse_xml/xml-handler-georss-gml.o src/update_feed/parse_xml/xml-handler-mediarss.o src/update_feed/parse_xml/xml-handler-rss.o src/update_feed/parse_xml/xml-handler-rsscontent.o src/update_feed/parse_xml/xml-handler-xhtml.o src/update_feed/struct-item.o src/update_feed/threading.o src/update_feed/update_feed.o src/wstring.o src/wstring-format.o

all: newsraft doc

install: install-newsraft install-doc install-examples

install-newsraft: newsraft
	install -Dm755 newsraft $(DESTDIR)$(BINDIR)/newsraft

install-doc: doc
	install -Dm644 newsraft.1 $(DESTDIR)$(MANDIR)/man1/newsraft.1

install-examples:
	install -Dm644 doc/examples/feeds $(DESTDIR)$(EXAMPLES_DIR)/feeds
	install -Dm644 doc/examples/config $(DESTDIR)$(EXAMPLES_DIR)/config

newsraft: $(OBJECTS)
	$(CC) -std=c99 $(CFLAGS) $(AUX_CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

libnewsraft.so: $(OBJECTS)
	$(CC) -std=c99 -shared $(CFLAGS) $(AUX_CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

doc: newsraft.1

.c.o:
	$(CC) -std=c99 $(CFLAGS) $(AUX_CFLAGS) -Isrc -D_XOPEN_SOURCE=700 -D_XOPEN_SOURCE_EXTENDED -c -o $@ $<

newsraft.1: doc/newsraft.scd
	scdoc < doc/newsraft.scd > newsraft.1 || true

clean:
	rm -rf newsraft newsraft.1 tests/makefile tests/src tests/libnewsraft.so tests/a.out flog vlog $(OBJECTS)

check:
	./tests/run-check.sh

cppcheck:
	find src -name "*.c" -exec cppcheck -q --enable=warning,performance,portability '{}' ';'
