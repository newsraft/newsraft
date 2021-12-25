#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "feedeater.h"

struct entity_entry {
	const wchar_t *const name;
	// Store numbers as strings to compare them without conversion.
	const wchar_t *const number;
	const wchar_t *const hex_number;
	const wchar_t *const value;
};

// Good list of HTML entities with their hexadecimal and decimal values.
// https://dev.w3.org/html5/html-author/charref

// WARNING!
// Don't forget to update MAX_ENTITY_NAME_LENGTH every time
// you add another entity with the longest name.
static const struct entity_entry entities[] = {
	{L"nbsp",          L"160",   L"A0",    L" "},
	{L"lsquo",         L"8216",  L"2018",  L"‘"},
	{L"rsquo",         L"8217",  L"2019",  L"’"},
	{L"rsquor",        L"8217",  L"2019",  L"’"},
	{L"sbquo",         L"8218",  L"201A",  L"‚"},
	{L"lsquor",        L"8218",  L"201A",  L"‚"},
	{L"ldquo",         L"8220",  L"201C",  L"“"},
	{L"rdquo",         L"8221",  L"201D",  L"”"},
	{L"rdquor",        L"8221",  L"201D",  L"”"},
	{L"bdquo",         L"8222",  L"201E",  L"„"},
	{L"ldquor",        L"8222",  L"201E",  L"„"},
	{L"laquo",         L"171",   L"AB",    L"«"},
	{L"raquo",         L"187",   L"BB",    L"»"},
	{L"dash",          L"8208",  L"2010",  L"-"},
	{L"hyphen",        L"8208",  L"2010",  L"-"},
	{L"ndash",         L"8211",  L"2013",  L"–"},
	{L"mdash",         L"8212",  L"2014",  L"—"},
	{L"horbar",        L"8213",  L"2015",  L"―"},
	{L"quot",          L"34",    L"22",    L"\""},
	{L"apos",          L"39",    L"27",    L"'"},
	{L"num",           L"35",    L"23",    L"#"},
	{L"colon",         L"58",    L"3A",    L":"},
	{L"semi",          L"59",    L"3B",    L";"},
	{L"lsaquo",        L"8249",  L"2039",  L"‹"},
	{L"rsaquo",        L"8250",  L"203A",  L"›"},
	{L"hellip",        L"8230",  L"2026",  L"…"},
	{L"mldr",          L"8230",  L"2026",  L"…"},
	{L"bull",          L"8226",  L"2022",  L"•"},
	{L"bullet",        L"8226",  L"2022",  L"•"},
	{L"larrhk",        L"8617",  L"21A9",  L"↩"},
	{L"hookleftarrow", L"8617",  L"21A9",  L"↩"},
	{L"trade",         L"8482",  L"2122",  L"™"},
	{L"copy",          L"169",   L"A9",    L"©"},
	{L"reg",           L"174",   L"AE",    L"®"},
	{L"numero",        L"8470",  L"2116",  L"№"},
	{L"sect",          L"167",   L"A7",    L"§"},
	{L"sup1",          L"185",   L"B9",    L"¹"},
	{L"sup2",          L"178",   L"B2",    L"²"},
	{L"sup3",          L"179",   L"B3",    L"³"},
	{L"amp",           L"38",    L"26",    L"&"},
	{L"lt",            L"60",    L"3C",    L"<"},
	{L"gt",            L"62",    L"3E",    L">"},
	{L"rightarrow",    L"8594",  L"2192",  L"→"},
	{L"RightArrow",    L"8594",  L"2192",  L"→"},
	{L"rarr",          L"8594",  L"2192",  L"→"},
	{L"Rightarrow",    L"8658",  L"21D2",  L"⇒"},
	{L"rArr",          L"8658",  L"21D2",  L"⇒"},
	{L"Rarr",          L"8608",  L"21A0",  L"↠"},
	{L"rdsh",          L"8627",  L"21B3",  L"↳"},
	{L"rdca",          L"10551", L"2937",  L"⤷"},
	{NULL,             L"65038", L"FE0E",  L""},
	{NULL,             L"65039", L"FE0F",  L""},
	{L"half",          L"189",   L"BD",    L"½"},
	{L"frac12",        L"189",   L"BD",    L"½"},
	{L"frac13",        L"8531",  L"2153",  L"⅓"},
	{L"frac23",        L"8532",  L"2154",  L"⅔"},
	{L"frac14",        L"188",   L"BC",    L"¼"},
	{L"frac34",        L"190",   L"BE",    L"¾"},
	{L"frac15",        L"8533",  L"2155",  L"⅕"},
	{L"frac25",        L"8534",  L"2156",  L"⅖"},
	{L"frac35",        L"8535",  L"2157",  L"⅗"},
	{L"frac45",        L"8536",  L"2158",  L"⅘"},
	{L"frac16",        L"8537",  L"2159",  L"⅙"},
	{L"frac56",        L"8538",  L"215A",  L"⅚"},
	{L"frac18",        L"8539",  L"215B",  L"⅛"},
	{L"frac38",        L"8540",  L"215C",  L"⅜"},
	{L"frac58",        L"8541",  L"215D",  L"⅝"},
	{L"frac78",        L"8542",  L"215E",  L"⅞"},
	{L"check",         L"10003", L"2713",  L"✓"},
	{L"checkmark",     L"10003", L"2713",  L"✓"},
	{L"deg",           L"176",   L"B0",    L"°"},
	{L"divide",        L"247",   L"F7",    L"÷"},
	{L"div",           L"247",   L"F7",    L"÷"},
	{L"dagger",        L"8224",  L"2020",  L"†"},
	{L"permil",        L"8240",  L"2030",  L"‰"},
	{L"dollar",        L"36",    L"24",    L"$"},
	{L"euro",          L"8364",  L"20AC",  L"€"},
	{L"pound",         L"163",   L"A3",    L"£"},
	{L"cent",          L"162",   L"A2",    L"¢"},
	{L"yen",           L"165",   L"A5",    L"¥"},
	{L"Pi",            L"928",   L"3A0",   L"Π"},
	{L"pi",            L"960",   L"3C0",   L"π"},
	{L"Alpha",         L"913",   L"391",   L"Α"},
	{L"alpha",         L"945",   L"3B1",   L"α"},
	{L"Delta",         L"916",   L"394",   L"Δ"},
	{L"delta",         L"948",   L"3B4",   L"δ"},
	{L"hearts",        L"9829",  L"2665",  L"♥"},
	{L"heartsuit",     L"9829",  L"2665",  L"♥"},
	{L"diams",         L"9830",  L"2666",  L"♦"},
	{L"diamondsuit",   L"9830",  L"2666",  L"♦"},
	{L"clubs",         L"9827",  L"2663",  L"♣"},
	{L"clubsuit",      L"9827",  L"2663",  L"♣"},
	{L"spades",        L"9824",  L"2660",  L"♠"},
	{L"spadesuit",     L"9824",  L"2660",  L"♠"},
	{L"pertenk",       L"8241",  L"2031",  L"‱"},
};

static inline size_t
count_insignificant_zeros(const wchar_t *entity)
{
	size_t i = 0;
	while (entity[i] == L'0') {
		++i;
	}
	return i;
}

const wchar_t *
translate_html_entity(wchar_t *entity)
{
	size_t i;

	if (entity[0] == L'#') {
		// Entity starts with '#' so this is a number entity.
		// Shift one character because we already know that this is a
		// number entity and all number entities have '#' at the beginning.
		wchar_t *num_entity = entity + 1;

		if (num_entity[0] == L'x') {
			// If number entity has 'x' right after '#' then this is a
			// hexadecimal number entity and we should shift one more character.
			num_entity += 1;

			// Shift insignificant zeros of number.
			num_entity += count_insignificant_zeros(num_entity);

			// Convert all hex digits of entity to upper case.
			for (i = 0; num_entity[i] != L'\0'; ++i) {
				num_entity[i] = towupper(num_entity[i]);
			}

			for (i = 0; i < LENGTH(entities); ++i) {
				if (wcscmp(num_entity, entities[i].hex_number) == 0) {
					return entities[i].value;
				}
			}

		} else {

			// Shift insignificant zeros of number.
			num_entity += count_insignificant_zeros(num_entity);

			for (i = 0; i < LENGTH(entities); ++i) {
				if (wcscmp(num_entity, entities[i].number) == 0) {
					return entities[i].value;
				}
			}

		}

	} else {

		for (i = 0; i < LENGTH(entities); ++i) {
			if ((entities[i].name != NULL) && (wcscmp(entity, entities[i].name) == 0)) {
				return entities[i].value;
			}
		}

	}

	WARN("Met an unknown HTML entity: %ls", entity);

	return NULL;
}
