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
#include "interface-errors-pager.c"
#include "interface-list.c"
#include "interface-list-pager.c"
#include "interface-status.c"
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

struct newsraft_execution_stage {
	const char *description;
	bool (*constructor)(void);
	void (*destructor)(void);
};

volatile bool they_want_us_to_stop = false;

static inline void
print_usage(void)
{
	fputs("newsraft - feed reader for terminal\n"
	      "\n"
	      "  -f PATH    force use of PATH as feeds file\n"
	      "  -c PATH    force use of PATH as config file\n"
	      "  -d PATH    force use of PATH as database file\n"
	      "  -l PATH    write log information to PATH\n"
	      "  -e ACTION  execute ACTION and exit\n"
	      "  -v         print version and successfully exit\n"
	      "  -h         print this message and successfully exit\n"
	      "\n"
	      "ACTION is one of the following: reload-all, print-unread-items-count, purge-abandoned\n",
	      stderr);
}

static const struct newsraft_execution_stage regular_mode[] = {
	{"register signal handlers",      register_signal_handlers,        NULL},
	{"assign default binds",          assign_default_binds,            free_default_binds},
	{"initialize curses library",     curses_init,                     curses_stop},
	{"load config file",              parse_config_file,               NULL},
	{"initialize database",           db_init,                         db_stop},
	{"execute database optimization", exec_database_file_optimization, NULL},
	{"load feeds file",               parse_feeds_file,                free_sections},
	{"create list menu",              adjust_list_menu,                free_list_menu},
	{"create status field",           status_recreate_unprotected,     status_delete},
	{"initialize curl library",       curl_init,                       curl_stop},
	{"start worker threads",          threads_start,                   threads_stop},
	{"run menu loop",                 run_menu_loop,                   free_menus},
};

static const struct newsraft_execution_stage reload_mode[] = {
	{"register signal handlers",      register_signal_handlers,        NULL},
	{"load config file",              parse_config_file,               NULL},
	{"initialize database",           db_init,                         db_stop},
	{"execute database optimization", exec_database_file_optimization, NULL},
	{"load feeds file",               parse_feeds_file,                free_sections},
	{"initialize curl library",       curl_init,                       curl_stop},
	{"start worker threads",          threads_start,                   threads_stop},
	{"update all feeds",              start_updating_all_feeds_and_wait_finish, NULL},
};

static const struct newsraft_execution_stage print_unread_mode[] = {
	{"initialize database",           db_init,                         db_stop},
	{"load feeds file",               parse_feeds_file,                free_sections},
	{"print unread items count",      print_unread_items_count,        NULL},
};

static const struct newsraft_execution_stage purge_mode[] = {
	{"initialize database",           db_init,                         db_stop},
	{"load feeds file",               parse_feeds_file,                free_sections},
	{"purge abandoned feeds",         purge_abandoned_feeds,           NULL},
};

static int
run_scenario(const struct newsraft_execution_stage *scenario, size_t scenario_stages_count)
{
	int error = 0;
	size_t stage = 0;
	for (stage = 0; stage < scenario_stages_count; ++stage) {
		INFO("Trying to %s...", scenario[stage].description);
		if (scenario[stage].constructor() != true) {
			write_error("Failed to %s\n", scenario[stage].description);
			error = stage;
			break;
		}
	}
	for (they_want_us_to_stop = true; stage > 0; --stage) {
		if (scenario[stage - 1].destructor) {
			INFO("Cleaning after %s...", scenario[stage - 1].description);
			scenario[stage - 1].destructor();
		}
	}
	return error;
}

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");

	int error = 0;
	int opt;
	while ((opt = getopt(argc, argv, "f:c:d:l:e:vh")) != -1) {
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
		} else if (opt == 'e') {
			if (strcmp(optarg, "reload-all") == 0) {
				error = run_scenario(reload_mode, LENGTH(reload_mode));
			} else if (strcmp(optarg, "print-unread-items-count") == 0) {
				error = run_scenario(print_unread_mode, LENGTH(print_unread_mode));
			} else if (strcmp(optarg, "purge-abandoned") == 0) {
				error = run_scenario(purge_mode, LENGTH(purge_mode));
			} else {
				fputs("Invalid action for execution. See newsraft(1) for details.\n", stderr);
				error = 6;
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

	error = run_scenario(regular_mode, LENGTH(regular_mode));

undo1:
	free_config();
	log_stop(error);
	flush_errors();
	return error;
}
