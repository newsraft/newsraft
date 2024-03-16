#include <locale.h>
#include <unistd.h>
#include <curl/curl.h>
#include "newsraft.h"

// find -name '*.c' | sed 's/^\.\///' | grep -v '^newsraft.c$' | sort | sed 's/^/#include "/' | sed 's/$/"/'
#include "binds.c"
#include "commands.c"
#include "dates.c"
#include "db.c"
#include "db-items.c"
#include "feeds.c"
#include "feeds-parse.c"
#include "interface.c"
#include "interface-input.c"
#include "interface-list.c"
#include "interface-list-pager.c"
#include "interface-status.c"
#include "interface-status-pager.c"
#include "items.c"
#include "items-list.c"
#include "items-metadata.c"
#include "items-metadata-content.c"
#include "items-metadata-links.c"
#include "items-metadata-persons.c"
#include "items-pager.c"
#include "load_config/config-auto.c"
#include "load_config/config-parse.c"
#include "load_config/config-parse-colors.c"
#include "load_config/config-parse-input.c"
#include "load_config/load_config.c"
#include "log.c"
#include "path.c"
#include "prepare_to_render_data/prepare-text-html.c"
#include "prepare_to_render_data/prepare-text-plain.c"
#include "prepare_to_render_data/prepare_to_render_data.c"
#include "render-block.c"
#include "render_data/line.c"
#include "render_data/render_data.c"
#include "render_data/render-text-html.c"
#include "render_data/render-text-html-table.c"
#include "sections.c"
#include "signal.c"
#include "string.c"
#include "string-serialize.c"
#include "update_feed/download.c"
#include "update_feed/execute.c"
#include "update_feed/insert_feed/insert_feed.c"
#include "update_feed/insert_feed/insert-feed-data.c"
#include "update_feed/insert_feed/insert-item-data.c"
#include "update_feed/parse_json/setup-json-parser.c"
#include "update_feed/parse_xml/common.c"
#include "update_feed/parse_xml/format-atom.c"
#include "update_feed/parse_xml/format-dublincore.c"
#include "update_feed/parse_xml/format-georss.c"
#include "update_feed/parse_xml/format-mediarss.c"
#include "update_feed/parse_xml/format-rss.c"
#include "update_feed/parse_xml/gperf-data.c"
#include "update_feed/parse_xml/setup-xml-parser.c"
#include "update_feed/struct-item.c"
#include "update_feed/update_feed.c"
#include "wstring.c"
#include "wstring-format.c"

volatile bool they_want_us_to_terminate = false;

static inline void
print_usage(void)
{
	fputs("newsraft - feed reader for terminal\n"
	      "-f PATH  force use of PATH as feeds file\n"
	      "-c PATH  force use of PATH as config file\n"
	      "-d PATH  force use of PATH as database file\n"
	      "-l PATH  write log information to PATH\n"
	      "-p       purge feeds not specified in the feeds file\n"
	      "-v       print version and successfully exit\n"
	      "-h       print this message and successfully exit\n",
	      stderr);
}

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");

	int error = 0;
	int opt;
	while ((opt = getopt(argc, argv, "f:c:d:l:pvh")) != -1) {
		if (opt == 'f') {
			if (set_feeds_path(optarg) == false) {
				error = 2;
				goto undo1;
			}
		} else if (opt == 'c') {
			if (set_config_path(optarg) == false) {
				error = 3;
				goto undo1;
			}
		} else if (opt == 'd') {
			if (set_db_path(optarg) == false) {
				error = 4;
				goto undo1;
			}
		} else if (opt == 'l') {
			if (log_init(optarg) == false) {
				error = 5;
				goto undo0;
			}
		} else if (opt == 'p') {
			if (parse_config_file()     == false) { error = 6;  goto undo1; }
			if (load_config()           == false) { error = 7;  goto undo3; }
			if (db_init()               == false) { error = 8;  goto undo3; }
			if (parse_feeds_file()      == false) { error = 9;  goto undo4; }
			if (purge_abandoned_feeds() == false) { error = 10; goto undo5; }
			if (db_vacuum()             == false) { error = 11; goto undo5; }
			goto undo5;
		} else if (opt == 'v') {
			fputs(NEWSRAFT_VERSION "\n", stderr);
			goto undo1;
		} else if (opt == 'h') {
			print_usage();
			goto undo1;
		} else {
			fprintf(stderr, "Try '%s -h' for more information.\n", argv[0]);
			error = 1;
			goto undo1;
		}
	}

	if (register_signal_handlers()         == false) { error = 6;  goto undo1;  }
	if (assign_default_binds()             == false) { error = 7;  goto undo1;  }
	if (curses_init()                      == false) { error = 13; goto undo2;  }
	if (parse_config_file()                == false) { error = 8;  goto undo3;  }
	if (load_config()                      == false) { error = 9;  goto undo4;  }
	if (db_init()                          == false) { error = 10; goto undo4;  }
	if (query_database_file_optimization() == false) { error = 11; goto undo5;  }
	if (parse_feeds_file()                 == false) { error = 12; goto undo5;  }
	if (adjust_list_menu()                 == false) { error = 14; goto undo6;  }
	if (status_recreate_unprotected()      == false) { error = 15; goto undo7;  }
	if (allocate_status_messages_buffer()  == false) { error = 16; goto undo8;  }
	if (counter_recreate_unprotected()     == false) { error = 17; goto undo8;  }
	if (curl_global_init(CURL_GLOBAL_DEFAULT)  != 0) { error = 18; goto undo9;  }
	if (start_feed_updater()               == false) { error = 19; goto undo10; }

	struct timespec idling = {0, 100000000}; // 0.1 seconds
	struct menu_state *menu = setup_menu(&sections_menu_loop, NULL, 0, MENU_NO_FLAGS);
	while (they_want_us_to_terminate == false) {
		menu = menu->run(menu);
		if (menu == NULL) {
			if (try_to_stop_feed_updater()) break;
			nanosleep(&idling, NULL); // Avoids CPU cycles waste while awaiting termination
			menu = setup_menu(&sections_menu_loop, NULL, 0, MENU_DISABLE_SETTINGS);
		}
	}

	free_menus();
undo10:
	curl_global_cleanup();
undo9:
	counter_delete();
undo8:
	status_delete();
undo7:
	free_list_menu();
undo6:
	free_sections();
undo5:
	db_stop();
undo4:
	free_config();
undo3:
	endwin();
undo2:
	free_binds();
undo1:
	log_stop(error);
undo0:
	return error;
}
