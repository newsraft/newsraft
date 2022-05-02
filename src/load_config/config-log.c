#include "load_config/load_config.h"

void
log_config_settings(void)
{
	INFO("Final configuration settings are:");
	INFO("max_items = %zu", cfg.max_items);
	INFO("download_timeout = %zu", cfg.download_timeout);
	INFO("append_links = %d", cfg.append_links);
	INFO("run_cleaning_of_the_database_on_startup = %d", cfg.run_cleaning_of_the_database_on_startup);
	INFO("run_analysis_of_the_database_on_startup = %d", cfg.run_analysis_of_the_database_on_startup);
	INFO("send_useragent_header = %d", cfg.send_useragent_header);
	INFO("send_if_none_match_header = %d", cfg.send_if_none_match_header);
	INFO("send_if_modified_since_header = %d", cfg.send_if_modified_since_header);
	INFO("ssl_verify_host = %d", cfg.ssl_verify_host);
	INFO("ssl_verify_peer = %d", cfg.ssl_verify_peer);
	INFO("menu_section_entry_format = %ls", cfg.menu_section_entry_format->ptr);
	INFO("menu_feed_entry_format = %ls", cfg.menu_feed_entry_format->ptr);
	INFO("menu_item_entry_format = %ls", cfg.menu_item_entry_format->ptr);
	INFO("global_section_name = %s", cfg.global_section_name->ptr);
	INFO("contents_meta_data = %s", cfg.contents_meta_data->ptr);
	INFO("contents_date_format = %s", cfg.contents_date_format->ptr);
	INFO("useragent = %s", cfg.useragent->ptr);
	INFO("proxy is omitted because it is sensitive");
	INFO("proxy_auth  is omitted because it is sensitive");
	INFO("size_conversion_threshold = %zu", cfg.size_conversion_threshold);
}
