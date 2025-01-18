/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m 1000 -I -t -F ,0,NULL,NULL  */
/* Computed positions: -k'1,8,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

struct xml_element_handler;
#include <string.h>

#define TOTAL_KEYWORDS 57
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 48
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 91
/* maximum key range = 89, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static unsigned char asso_values[] =
    {
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92,  0, 92, 92,
      15, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92,  0, 92, 17,
      17,  6, 92,  0,  0,  0, 92, 37,  0,  0,
      28,  0,  0, 92, 11, 16,  0, 92, 92,  0,
      92, 25, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92, 92, 92, 92, 92,
      92, 92, 92, 92, 92, 92
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[7]];
      /*FALLTHROUGH*/
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct xml_element_handler *
in_word_set (register const char *str, register size_t len)
{
  static struct xml_element_handler wordlist[] =
    {
      {"",0,NULL,NULL}, {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"ttl",                                              XML_UNKNOWN_POS,  NULL,                    &rss_ttl_end},
      {"item",                                             GENERIC_ITEM,     &generic_item_starter,   &generic_item_ender},
      {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"Channel",                                          GENERIC_FEED,     NULL,                    NULL},
      {"",0,NULL,NULL}, {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"title",                                            XML_UNKNOWN_POS,  NULL,                    &generic_title_end},
      {"",0,NULL,NULL},
      {"pubDate",                                          XML_UNKNOWN_POS,  NULL,                    &rss_pubdate_end},
      {"",0,NULL,NULL}, {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"author",                                           XML_UNKNOWN_POS,  NULL,                    &rss_author_end},
      {"",0,NULL,NULL},
      {"lastBuildDate",                                    XML_UNKNOWN_POS,  NULL,                    &log_xml_element_content_end},
      {"generator",                                        XML_UNKNOWN_POS,  NULL,                    &log_xml_element_content_end},
      {"guid",                                             XML_UNKNOWN_POS,  &rss_guid_start,         &generic_guid_end},
      {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"channel",                                          GENERIC_FEED,     NULL,                    NULL},
      {"managingEditor",                                   XML_UNKNOWN_POS,  NULL,                    &rss_managingeditor_end},
      {"webMaster",                                        XML_UNKNOWN_POS,  NULL,                    &log_xml_element_content_end},
      {"http://www.rbc.ru full-text",                      XML_UNKNOWN_POS,  NULL,                    &generic_plain_content_end},
      {"source",                                           XML_UNKNOWN_POS,  &rss_source_start,       &rss_source_end},
      {"http://purl.org/rss/1.0/ item",                    GENERIC_ITEM,     &generic_item_starter,   &generic_item_ender},
      {"http://turbo.yandex.ru content",                   XML_UNKNOWN_POS,  NULL,                    &generic_html_content_end},
      {"http://www.w3.org/2005/Atom uri",                  XML_UNKNOWN_POS,  NULL,                    &uri_end},
      {"enclosure",                                        XML_UNKNOWN_POS,  &rss_enclosure_start,    NULL},
      {"http://www.w3.org/2005/Atom email",                XML_UNKNOWN_POS,  NULL,                    &email_end},
      {"http://www.georss.org/georss point",               XML_UNKNOWN_POS,  NULL,                    &georss_point_end},
      {"http://www.w3.org/2005/Atom content",              XML_UNKNOWN_POS,  &atom_content_start,     &atom_content_end},
      {"http://purl.org/rss/1.0/ title",                   XML_UNKNOWN_POS,  NULL,                    &generic_title_end},
      {"",0,NULL,NULL},
      {"http://www.w3.org/2005/Atom name",                 XML_UNKNOWN_POS,  NULL,                    &name_end},
      {"http://www.w3.org/2005/Atom title",                XML_UNKNOWN_POS,  NULL,                    &generic_title_end},
      {"http://purl.org/dc/elements/1.1/ subject",         XML_UNKNOWN_POS,  NULL,                    &generic_category_end},
      {"link",                                             XML_UNKNOWN_POS,  NULL,                    &rss_link_end},
      {"http://www.w3.org/2005/Atom subtitle",             XML_UNKNOWN_POS,  &atom_subtitle_start,    &atom_subtitle_end},
      {"http://purl.org/dc/elements/1.1/ date",            XML_UNKNOWN_POS,  NULL,                    &update_date_end},
      {"http://purl.org/dc/elements/1.1/ title",           XML_UNKNOWN_POS,  NULL,                    &dublincore_title_end},
      {"http://www.w3.org/2005/Atom author",               ATOM_AUTHOR,      &author_start,           NULL},
      {"http://www.opengis.net/gml pos",                   XML_UNKNOWN_POS,  NULL,                    &georss_point_end},
      {"http://www.w3.org/2005/Atom id",                   XML_UNKNOWN_POS,  NULL,                    &generic_guid_end},
      {"http://www.w3.org/2005/Atom generator",            XML_UNKNOWN_POS,  &atom_generator_start,   &log_xml_element_content_end},
      {"http://www.w3.org/2005/Atom feed",                 GENERIC_FEED,     NULL,                    NULL},
      {"http://www.w3.org/2005/Atom contributor",          ATOM_AUTHOR,      &contributor_start,      NULL},
      {"http://purl.org/dc/elements/1.1/ creator",         XML_UNKNOWN_POS,  NULL,                    &dublincore_creator_end},
      {"http://www.w3.org/2005/Atom updated",              XML_UNKNOWN_POS,  NULL,                    &update_date_end},
      {"http://search.yahoo.com/mrss/ content",            MEDIARSS_CONTENT, &mediarss_content_start, NULL},
      {"http://www.w3.org/2005/Atom published",            XML_UNKNOWN_POS,  NULL,                    &published_end},
      {"http://purl.org/dc/elements/1.1/ contributor",     XML_UNKNOWN_POS,  NULL,                    &dublincore_contributor_end},
      {"description",                                      XML_UNKNOWN_POS,  NULL,                    &generic_html_content_end},
      {"comments",                                         XML_UNKNOWN_POS,  NULL,                    &rss_comments_end},
      {"http://www.w3.org/2005/Atom entry",                GENERIC_ITEM,     &generic_item_starter,   &generic_item_ender},
      {"http://news.yandex.ru full-text",                  XML_UNKNOWN_POS,  NULL,                    &generic_html_content_end},
      {"http://www.w3.org/2005/Atom summary",              XML_UNKNOWN_POS,  &atom_content_start,     &atom_content_end},
      {"http://www.w3.org/2005/Atom category",             XML_UNKNOWN_POS,  &atom_category_start,    NULL},
      {"http://www.w3.org/1999/02/22-rdf-syntax-ns# RDF",  GENERIC_FEED,     NULL,                    NULL},
      {"http://search.yahoo.com/mrss/ player",             XML_UNKNOWN_POS,  &embed_or_player_start,  NULL},
      {"http://purl.org/rss/1.0/ description",             XML_UNKNOWN_POS,  NULL,                    &generic_html_content_end},
      {"http://purl.org/rss/1.0/modules/content/ encoded", XML_UNKNOWN_POS,  NULL,                    &generic_html_content_end},
      {"http://purl.org/rss/1.0/ link",                    XML_UNKNOWN_POS,  NULL,                    &rss_link_end},
      {"",0,NULL,NULL},
      {"http://search.yahoo.com/mrss/ embed",              XML_UNKNOWN_POS,  &embed_or_player_start,  NULL},
      {"http://www.w3.org/2005/Atom link",                 XML_UNKNOWN_POS,  &atom_link_start,        NULL},
      {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"http://purl.org/dc/elements/1.1/ description",     XML_UNKNOWN_POS,  NULL,                    &generic_plain_content_end},
      {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"category",                                         XML_UNKNOWN_POS,  NULL,                    &generic_category_end},
      {"",0,NULL,NULL}, {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"",0,NULL,NULL}, {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"",0,NULL,NULL}, {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"http://search.yahoo.com/mrss/ description",        XML_UNKNOWN_POS,  &description_start,      &description_end},
      {"",0,NULL,NULL}, {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"",0,NULL,NULL}, {"",0,NULL,NULL},
      {"http://search.yahoo.com/mrss/ peerLink",           XML_UNKNOWN_POS,  &peerlink_start,         NULL}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strcmp (str + 1, s + 1))
            return &wordlist[key];
        }
    }
  return 0;
}
