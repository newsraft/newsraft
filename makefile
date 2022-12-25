.POSIX:
.PHONY: all install install-newsraft install-doc install-examples doc clean

CC             = cc
CFLAGS         = -O3
LDFLAGS        =
NEWSRAFT_FLAGS = -DNEWSRAFT_VERSION=\"0.12\"
CURL_LIBS      = -lcurl
CURSES_LIBS    = -lncursesw
EXPAT_LIBS     = -lexpat
GUMBO_LIBS     = -lgumbo
PTHREAD_LIBS   = -lpthread
SQLITE_LIBS    = -lsqlite3
YAJL_LIBS      = -lyajl
# for static linking
#LDFLAGS        = -static
#CURL_LIBS      = -lcurl -lbrotlidec -lbrotlienc -lbrotlicommon -lssl -lcrypto -lnghttp2 -lz
LDLIBS         = $(CURL_LIBS) $(CURSES_LIBS) $(EXPAT_LIBS) $(GUMBO_LIBS) $(PTHREAD_LIBS) $(SQLITE_LIBS) $(YAJL_LIBS)
DESTDIR        =
PREFIX         = /usr/local
BINDIR         = $(PREFIX)/bin
MANDIR         = $(PREFIX)/share/man
EXAMPLES_DIR   = $(PREFIX)/share/newsraft/examples

# find src -name '*.c' | sort | sed 's/\.c/.o/' | tr '\n' ' '
OBJECTS = src/commands.o src/dates.o src/db.o src/db-items.o src/feeds.o src/feeds-parse.o src/format.o src/input.o src/interface.o src/interface-colors.o src/interface-counter.o src/interface-list.o src/interface-pager.o src/interface-pager-item.o src/interface-pager-status.o src/interface-status.o src/items.o src/items-links.o src/items-list.o src/items-metadata.o src/items-metadata-content.o src/items-persons.o src/load_config/config.o src/load_config/config-copytoclipboardcommand.o src/load_config/config-openinbrowsercommand.o src/load_config/config-parse.o src/load_config/config-parse-input.o src/load_config/config-updatethreadscount.o src/load_config/config-useragent.o src/load_config/load_config.o src/log.o src/newsraft.o src/path.o src/prepare_to_render_data/prepare-text-html.o src/prepare_to_render_data/prepare_to_render_data.o src/render-block.o src/render_data/line.o src/render_data/render_data.o src/render_data/render-text-html.o src/render_data/render-text-html-table.o src/sections.o src/signal.o src/string.o src/string-serialize.o src/threading.o src/update_feed/download.o src/update_feed/insert_feed/insert_feed.o src/update_feed/insert_feed/insert-feed-data.o src/update_feed/insert_feed/insert-item-data.o src/update_feed/parse_json/engage-json-parser.o src/update_feed/parse_xml/engage-xml-parser.o src/update_feed/parse_xml/xml-common.o src/update_feed/parse_xml/xml-handler-atom03.o src/update_feed/parse_xml/xml-handler-atom10.o src/update_feed/parse_xml/xml-handler-dublincore.o src/update_feed/parse_xml/xml-handler-georss.o src/update_feed/parse_xml/xml-handler-georss-gml.o src/update_feed/parse_xml/xml-handler-mediarss.o src/update_feed/parse_xml/xml-handler-rbcnews.o src/update_feed/parse_xml/xml-handler-rss.o src/update_feed/parse_xml/xml-handler-rsscontent.o src/update_feed/parse_xml/xml-handler-yandex.o src/update_feed/struct-item.o src/update_feed/update_feed.o src/wstring.o

all: newsraft doc

install: install-newsraft install-doc install-examples

install-newsraft: newsraft
	install -Dm755 newsraft $(DESTDIR)$(BINDIR)/newsraft

install-doc: doc
	install -Dm644 newsraft.1 $(DESTDIR)$(MANDIR)/man1/newsraft.1

install-examples:
	install -Dm644 examples/feeds $(DESTDIR)$(EXAMPLES_DIR)/feeds
	install -Dm644 examples/config $(DESTDIR)$(EXAMPLES_DIR)/config

newsraft: $(OBJECTS)
	$(CC) -std=c99 $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

doc: newsraft.1

.c.o:
	$(CC) -std=c99 $(CFLAGS) -Isrc -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED $(NEWSRAFT_FLAGS) -c -o $@ $<

newsraft.1: doc/newsraft.scd
	scdoc < doc/newsraft.scd > newsraft.1

clean:
	rm -f newsraft newsraft.1 vlog flog $(OBJECTS)
