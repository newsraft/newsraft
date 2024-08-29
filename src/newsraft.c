#include <locale.h>
#include <unistd.h>
#include <curl/curl.h>
#include "newsraft.h"

/*
find -name '*.c' | sed -e 's/^\.\//#include "/' -e 's/$/"/' | grep -v 'newsraft.c' | sort
*/
#include "binds.c"
#include "commands.c"
#include "dates.c"
#include "db.c"
#include "db-items.c"
#include "downloader.c"
#include "errors.c"
#include "executor.c"
#include "feeds.c"
#include "feeds-parse.c"
#include "inserter.c"
#include "insert_feed/insert_feed.c"
#include "insert_feed/insert-feed-data.c"
#include "insert_feed/insert-item-data.c"
#include "interface.c"
#include "interface-colors.c"
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
#include "load_config/load_config.c"
#include "log.c"
#include "parse_json/setup-json-parser.c"
#include "parse_xml/common.c"
#include "parse_xml/format-atom.c"
#include "parse_xml/format-dublincore.c"
#include "parse_xml/format-georss.c"
#include "parse_xml/format-mediarss.c"
#include "parse_xml/format-rss.c"
#include "parse_xml/gperf-data.c"
#include "parse_xml/setup-xml-parser.c"
#include "path.c"
#include "queue.c"
#include "render-block.c"
#include "render_data/line.c"
#include "render_data/render_data.c"
#include "render_data/render-text-html.c"
#include "render_data/render-text-plain.c"
#include "sections.c"
#include "signal.c"
#include "sorting.c"
#include "string.c"
#include "string-serialize.c"
#include "struct-item.c"
#include "threads.c"
#include "wstring.c"
#include "wstring-format.c"

volatile bool they_want_us_to_stop = false;

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
				goto undo1;
			}
		} else if (opt == 'p') {
			error = 6;
			if (db_init()) {
				if (parse_feeds_file()) {
					error = purge_abandoned_feeds() && db_vacuum() ? 0 : 7;
					free_sections();
				}
				db_stop();
			}
			goto undo1;
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

	if (register_signal_handlers()        == false) { error = 6;  goto undo1; }
	if (assign_default_binds()            == false) { error = 7;  goto undo1; }
	if (curses_init()                     == false) { error = 8;  goto undo2; }
	if (parse_config_file()               == false) { error = 9;  goto undo3; }
	if (db_init()                         == false) { error = 10; goto undo3; }
	if (exec_database_file_optimization() == false) { error = 11; goto undo4; }
	if (parse_feeds_file()                == false) { error = 12; goto undo4; }
	if (adjust_list_menu()                == false) { error = 13; goto undo5; }
	if (status_recreate_unprotected()     == false) { error = 14; goto undo6; }
	if (curl_init()                       == false) { error = 15; goto undo7; }
	if (threads_start()                   == false) { error = 16; goto undo8; }

	struct timespec idling = {0, 100000000}; // 0.1 seconds
	struct menu_state *menu = setup_menu(&sections_menu_loop, NULL, NULL, 0, MENU_NORMAL);
	while (they_want_us_to_stop == false) {
		menu = menu->run(menu);
		if (menu == NULL) {
			break; // TODO: don't stop feed downloader?
			nanosleep(&idling, NULL); // Avoids CPU cycles waste while awaiting termination
			menu = setup_menu(&sections_menu_loop, NULL, NULL, 0, MENU_DISABLE_SETTINGS);
		}
	}

	they_want_us_to_stop = true;

	free_menus();
	threads_stop();
undo8:
	curl_stop();
undo7:
	status_delete();
undo6:
	free_list_menu();
undo5:
	free_sections();
undo4:
	db_stop();
undo3:
	endwin();
undo2:
	free_binds(NULL);
undo1:
	free_config();
	log_stop(error);
	flush_errors();
	return error;
}
