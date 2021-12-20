#ifndef UPDATE_FEED_H
#define UPDATE_FEED_H
#include <expat.h>

#define NAMESPACE_SEPARATOR ' '

enum parse_error {
	PARSE_OKAY = 0,
	PARSE_FAIL_NOT_ENOUGH_MEMORY,
	PARSE_FAIL_CURL_EASY_PERFORM_ERROR,
	PARSE_FAIL_XML_PARSE_ERROR,
	PARSE_FAIL_DB_TRANSACTION_ERROR,
};

struct link {
	struct string *url; // string with the url to data
	struct string *type; // standard MIME type of data
	size_t size; // size of data in bytes
	size_t duration; // duration of data (if it is a audio or video)
};

struct author {
	struct string *name;
	struct string *email;
	struct string *link;
};

// Used to bufferize an item before writing it to the database,
// so we can reject it in case if identical item is already cached.
struct item_bucket {
	struct string *guid;
	struct string *title;
	struct string *url; // TODO: make this of struct link * type
	struct string *summary;
	struct string *summary_type; // Format of text of summary (for example plain or html).
	struct string *content;
	struct string *content_type; // Format of text of content.
	struct string *comments;
	struct string *categories;
	struct link *enclosures;
	size_t enclosures_len; // Actual number of enclosures in enclosures buffer.
	size_t enclosures_lim; // Shows how many enclosures can fit in current enclosures buffer.
	struct author *authors;
	size_t authors_len; // Actual number of authors in authors buffer.
	size_t authors_lim; // Shows how many authors can fit in current authors buffer.
	// Dates in this struct are represented in seconds since the Epoch (1970-01-01 00:00 UTC).
	// If some date set to 0 then it is considered unset.
	time_t pubdate;
	time_t upddate;
};

struct parser_data {
	char *value;
	size_t value_len;
	size_t value_lim;
	int depth;
	const struct string *feed_url;
	struct item_bucket *bucket;
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS20
	int16_t rss20_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM10
	int16_t atom10_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_ATOM03
	int16_t atom03_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_DUBLINCORE
	int8_t dc_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS11
	int8_t rss11_pos;
#endif
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
	int8_t rss10content_pos;
#endif
	XML_Parser parser;
	void (*start_handler)(struct parser_data *data, const XML_Char *name, const XML_Char **atts);
	void (*end_handler)(struct parser_data *data, const XML_Char *name);
	enum parse_error error;
};

void db_update_feed_text(const struct string *feed_url, const char *column, const char *data, size_t data_len);
const char *get_value_of_attribute_key(const XML_Char **atts, const char *key);
void try_item_bucket(const struct item_bucket *bucket, const struct string *feed_url);
bool we_are_inside_item(const struct parser_data *data);

// item bucket functions
struct item_bucket *create_item_bucket(void);
void empty_item_bucket(struct item_bucket *bucket);
void free_item_bucket(struct item_bucket *bucket);
int add_category_to_item_bucket(const struct item_bucket *bucket, const char *category, size_t category_len);
int expand_enclosures_of_item_bucket_by_one_element(struct item_bucket *bucket);
int add_url_to_last_enclosure_of_item_bucket(struct item_bucket *bucket, const char *str, size_t str_len);
int add_type_to_last_enclosure_of_item_bucket(struct item_bucket *bucket, const char *str, size_t str_len);
int add_size_to_last_enclosure_of_item_bucket(struct item_bucket *bucket, const char *str);
int expand_authors_of_item_bucket_by_one_element(struct item_bucket *bucket);
int add_name_to_last_author_of_item_bucket(struct item_bucket *bucket, const char *str, size_t str_len);
int add_email_to_last_author_of_item_bucket(struct item_bucket *bucket, const char *str, size_t str_len);
int add_link_to_last_author_of_item_bucket(struct item_bucket *bucket, const char *str, size_t str_len);


// Element handlers

int parse_namespace_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
int parse_namespace_element_end   (struct parser_data *data, const XML_Char *name);

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
	RSS20_CHANNEL = 1024,
};
void parse_rss20_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_rss20_element_end   (struct parser_data *data, const XML_Char *name);
#endif
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
#ifdef FEEDEATER_FORMAT_SUPPORT_RSS10CONTENT
enum rss10content_position {
	RSS10CONTENT_NONE = 0,
	RSS10CONTENT_ENCODED = 1,
};
void parse_rss10content_element_start (struct parser_data *data, const XML_Char *name, const XML_Char **atts);
void parse_rss10content_element_end   (struct parser_data *data, const XML_Char *name);
#endif

#endif // UPDATE_FEED_H
