.POSIX:
.PHONY: all install install-newsraft install-doc doc clean

CC             = cc
CFLAGS         = -O3
LDFLAGS        =
LDLIBS         = -lncursesw -lsqlite3 -lcurl -lexpat -lyajl -lgumbo
# for static linking
#LDFLAGS        = -static
#LDLIBS         = -lncursesw -lsqlite3 -lcurl -lexpat -lyajl -lgumbo -lbrotlidec -lbrotlienc -lbrotlicommon -lssl -lcrypto -lnghttp2 -lz
DESTDIR        =
PREFIX         = /usr/local
NEWSRAFT_FLAGS = -DNEWSRAFT_VERSION=\"0.7\" -DNEWSRAFT_FORMAT_SUPPORT_ATOM10 -DNEWSRAFT_FORMAT_SUPPORT_RSS -DNEWSRAFT_FORMAT_SUPPORT_RSSCONTENT -DNEWSRAFT_FORMAT_SUPPORT_DUBLINCORE -DNEWSRAFT_FORMAT_SUPPORT_MEDIARSS -DNEWSRAFT_FORMAT_SUPPORT_YANDEX -DNEWSRAFT_FORMAT_SUPPORT_RBCNEWS -DNEWSRAFT_FORMAT_SUPPORT_ATOM03 -DNEWSRAFT_FORMAT_SUPPORT_GEORSS -DNEWSRAFT_FORMAT_SUPPORT_GEORSS_GML -DNEWSRAFT_FORMAT_SUPPORT_JSONFEED

# find src -name '*.c' | sed 's/\.c/.o/' | tr '\n' ' '
OBJECTS = src/interface-list.o src/path.o src/db.o src/load_config/config-parse-input.o src/load_config/load_config.o src/load_config/config-updatethreadscount.o src/load_config/config.o src/load_config/config-openinbrowsercommand.o src/load_config/config-useragent.o src/load_config/config-parse.o src/load_config/config-verifier.o src/load_config/config-copytoclipboardcommand.o src/items-links.o src/feeds-parse.o src/interface-status.o src/threading.o src/items.o src/newsraft.o src/dates.o src/interface.o src/string.o src/interface-colors.o src/interface-pager.o src/items-metadata-content.o src/wstring.o src/interface-pager-status.o src/log.o src/update_feed/update_feed.o src/update_feed/insert_feed/insert-feed-data.o src/update_feed/insert_feed/insert_feed.o src/update_feed/insert_feed/insert-item-data.o src/update_feed/download.o src/update_feed/parse_json/engage-json-parser.o src/update_feed/struct-item.o src/update_feed/parse_xml/xml-handler-atom10.o src/update_feed/parse_xml/xml-handler-rss.o src/update_feed/parse_xml/xml-handler-rbcnews.o src/update_feed/parse_xml/xml-handler-atom03.o src/update_feed/parse_xml/xml-handler-georss-gml.o src/update_feed/parse_xml/xml-handler-mediarss.o src/update_feed/parse_xml/xml-handler-yandex.o src/update_feed/parse_xml/xml-common.o src/update_feed/parse_xml/xml-handler-rsscontent.o src/update_feed/parse_xml/xml-handler-dublincore.o src/update_feed/parse_xml/xml-handler-georss.o src/update_feed/parse_xml/engage-xml-parser.o src/render-block.o src/feeds.o src/db-items.o src/interface-pager-item.o src/items-metadata.o src/interface-counter.o src/render_data/line.o src/render_data/render-text-plain.o src/render_data/render-text-html.o src/render_data/render_data.o src/render_data/render-text-html-table.o src/items-persons.o src/sections.o src/string-serialize.o src/commands.o src/prepare_to_render_data/prepare-text-html.o src/prepare_to_render_data/prepare_to_render_data.o src/items-list.o src/input.o src/interface-list-format.o

all: newsraft doc

install: install-newsraft install-doc

install-newsraft: newsraft
	install -Dm755 newsraft -t $(DESTDIR)$(PREFIX)/bin

install-doc: doc
	install -Dm644 newsraft.1 -t $(DESTDIR)$(PREFIX)/share/man/man1

newsraft: $(OBJECTS)
	$(CC) -std=c99 $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

doc: newsraft.1

.c.o:
	$(CC) -std=c99 $(CFLAGS) -Isrc -D_XOPEN_SOURCE=700 $(NEWSRAFT_FLAGS) -c -o $@ $<

newsraft.1: doc/newsraft.scd
	scdoc < $< > $@

clean:
	rm -f newsraft newsraft.1 vlog flog $(OBJECTS)
