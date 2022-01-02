#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include <expat.h>

#define NAMESPACE_SEPARATOR ' '

enum update_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
	PARSE_FAIL_XML_PARSE_ERROR,
	PARSE_FAIL_XML_UNABLE_TO_CREATE_PARSER,
	PARSE_FAIL_CURL_UNABLE_TO_CREATE_HANDLE,
	PARSE_FAIL_CURL_EASY_PERFORM_ERROR,
	PARSE_FAIL_DB_TRANSACTION_ERROR,
};

struct link {
	struct string *url;  // Link to data.
	struct string *type; // Standard MIME type of data.
	size_t size;         // Size of data in bytes.
	size_t duration;     // Duration of data in seconds (if it is an audio or video).
};

struct link_list {
	struct link *list; // Dynamic array of links.
	size_t len;        // Shows how many items is in list.
	size_t lim;        // Shows how many items list can fit.
};

struct person {
	struct string *name;
	struct string *email;
	struct string *link;
};

struct person_list {
	struct person *list; // Dynamic array of persons.
	size_t len;          // Shows how many items is in list.
	size_t lim;          // Shows how many items list can fit.
};

// Used to bufferize a feed before writing it to the database,
// so we can insert or replace one big chunk of data in one
// statement instead of frequent check-inserts of individual values.
struct feed_bucket {
	struct string *title;
	struct string *link;
	struct string *summary;
	struct string *summary_type;
	struct string *categories;
	struct string *language;
	struct string *generator;
	struct string *rights;
};

// Used to bufferize an item before writing it to the database,
// so we can ignore it in case if identical item is already cached.
struct item_bucket {
	struct string *guid;
	struct string *title;
	struct string *title_type;
	struct string *url; // TODO: make this of struct link * type
	struct string *summary;
	struct string *summary_type; // Format of text of summary (for example plain or html).
	struct string *content;
	struct string *content_type; // Format of text of content.
	struct string *comments;
	struct string *categories;
	struct link_list enclosures;
	struct person_list authors;
	// Dates in this struct are represented in seconds
	// since the Epoch (1970-01-01 00:00 UTC). If some
	// date is set to 0 then it is considered unset.
	time_t pubdate;
	time_t upddate;
};

struct parser_data {
	struct string *value;
	int depth;
	const struct string *feed_url;
	struct feed_bucket *feed;
	struct item_bucket *item;
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	int16_t rss20_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	int16_t atom10_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
	int8_t dc_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
	int8_t rss10content_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
	int8_t yandex_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	int16_t atom03_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	int8_t rss11_pos;
#endif
	XML_Parser parser;
	void (*start_handler)(struct parser_data *data, const XML_Char *name, const XML_Char **atts);
	void (*end_handler)(struct parser_data *data, const XML_Char *name);
	enum update_error error;
};

void delete_excess_items(const struct string *feed_url);

const char *get_value_of_attribute_key(const XML_Char **atts, const char *key);
void insert_item(const struct string *feed_url, const struct item_bucket *item);
bool we_are_inside_item(const struct parser_data *data);

// date
time_t parse_date_rfc822(const struct string *value);
time_t parse_date_rfc3339(const struct string *value);

struct string *convert_bytes_to_human_readable_size_string(int bytes);

// feed bucket functions
struct feed_bucket *create_feed_bucket(void);
bool insert_feed(const struct string *feed_url, struct feed_bucket *feed);
void free_feed_bucket(struct feed_bucket *feed);

// item bucket functions
struct item_bucket *create_item_bucket(void);
void empty_item_bucket(struct item_bucket *item);
void free_item_bucket(struct item_bucket *item);
bool add_category_to_item_bucket(const struct item_bucket *item, const char *value, size_t value_len);

// Functions to manage link_list.
void initialize_link_list(struct link_list *links);
bool expand_link_list_by_one_element(struct link_list *links);
bool add_url_to_last_link(struct link_list *links, const char *value, size_t value_len);
bool add_type_to_last_link(struct link_list *links, const char *value, size_t value_len);
bool add_size_to_last_link(struct link_list *links, const char *value);
void empty_link_list(struct link_list *links);
void free_link_list(struct link_list *links);
struct string *generate_link_list_string(const struct link_list *links);

// Functions to manage person_list.
void initialize_person_list(struct person_list *persons);
bool expand_person_list_by_one_element(struct person_list *persons);
bool add_name_to_last_person(struct person_list *persons, const struct string *value);
bool add_email_to_last_person(struct person_list *persons, const struct string *value);
bool add_link_to_last_person(struct person_list *persons, const struct string *value);
void empty_person_list(struct person_list *persons);
void free_person_list(struct person_list *persons);
struct string *generate_person_list_string(const struct person_list *persons);


// Element handlers

int parse_namespace_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
int parse_namespace_element_end   (struct parser_data *data, const XML_Char *name);

#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
enum atom10_position {
	ATOM10_NONE = 0,
	ATOM10_ENTRY = 1,
	ATOM10_ID = 2,
	ATOM10_TITLE = 4,
	ATOM10_SUMMARY = 8,
	ATOM10_CONTENT = 16,
	ATOM10_PUBLISHED = 32,
	ATOM10_UPDATED = 64,
	ATOM10_AUTHOR = 128,
	ATOM10_NAME = 256,
	ATOM10_URI = 512,
	ATOM10_EMAIL = 1024,
};
void parse_atom10_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_atom10_element_end   (struct parser_data *data, const XML_Char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
enum rss20_position {
	RSS20_NONE = 0,
	RSS20_ITEM = 1,
	RSS20_TITLE = 2,
	RSS20_DESCRIPTION = 4,
	RSS20_LINK = 8,
	RSS20_PUBDATE = 16,
	RSS20_GUID = 32,
	RSS20_AUTHOR = 64,
	RSS20_SOURCE = 128,
	RSS20_CATEGORY = 256,
	RSS20_COMMENTS = 512,
	RSS20_LANGUAGE = 1024,
	RSS20_CHANNEL = 2048,
};
void parse_rss20_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_rss20_element_end   (struct parser_data *data, const XML_Char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
enum dc_position {
	DC_NONE = 0,
	DC_TITLE = 1,
	DC_DESCRIPTION = 2,
	DC_CREATOR = 4,
	DC_SUBJECT = 8,
};
void parse_dc_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_dc_element_end   (struct parser_data *data, const XML_Char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
enum rss10content_position {
	RSS10CONTENT_NONE = 0,
	RSS10CONTENT_ENCODED = 1,
};
void parse_rss10content_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_rss10content_element_end   (struct parser_data *data, const XML_Char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_YANDEX
enum yandex_position {
	YANDEX_NONE = 0,
	YANDEX_FULLTEXT = 1,
	YANDEX_GENRE = 2,
};
void parse_yandex_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_yandex_element_end   (struct parser_data *data, const XML_Char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
enum atom03_position {
	ATOM03_NONE = 0,
	ATOM03_ENTRY = 1,
	ATOM03_ID = 2,
	ATOM03_TITLE = 4,
	ATOM03_SUMMARY = 8,
	ATOM03_CONTENT = 16,
	ATOM03_ISSUED = 32,
	ATOM03_MODIFIED = 64,
	ATOM03_AUTHOR = 128,
	ATOM03_NAME = 256,
	ATOM03_URL = 512,
	ATOM03_EMAIL = 1024,
};
void parse_atom03_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_atom03_element_end   (struct parser_data *data, const XML_Char *name);
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
enum rss11_position {
	RSS11_NONE = 0,
	RSS11_ITEM = 1,
	RSS11_TITLE = 2,
	RSS11_LINK = 4,
	RSS11_DESCRIPTION = 8,
	RSS11_IMAGE = 16,
	RSS11_URL = 32,
};
void parse_rss11_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_rss11_element_end   (struct parser_data *data, const XML_Char *name);
#endif
#endif // UPDATE_FEED_H
