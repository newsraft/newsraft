project(
	'feedeater',
	'c',
	license: 'MIT',
	default_options: [
		'c_std=c18',
		'warning_level=3',
		'buildtype=release',
	],
	version: 'this is not even pre-alpha',
)
add_project_arguments(
	[
		# enable namespaces functionality of expat
		'-DXML_NS',
		# enable parameter entity parsing functionality of expat
		'-DXML_DTD',
		# make expat use large integers for file/stream positions
		'-DXML_LARGE_SIZE',
		# set integer modifier to "ll" because of argument above
		'-DXML_FMT_INT_MOD="ll"',
		# undefine XML_UNICODE to make expat work in UTF-8 mode (when defined, expat uses UTF-16)
		'-UXML_UNICODE',
		# undefine XML_UNICODE_WCHAR_T to make expat use char for characters
		'-UXML_UNICODE_WCHAR_T',
		# set string specifier to "s" because of argument above
		'-DXML_FMT_STR="s"',
	],
	language : 'c',
)
feedeater_dep = [
	dependency('ncursesw'),
	dependency('libcurl'),
	dependency('expat'),
	dependency('sqlite3'),
]
feedeater_src = [
	'src/feedeater.h',
	'src/feedeater.c',
	'src/debug.c',
	'src/path.c',
	'src/string.c',
	'src/wstring.c',
	'src/db.c',
	'src/db-feeds.c',
	'src/db-items.c',
	'src/config.c',
	'src/config-defaults.c',
	'src/sets.c',
	'src/sets-format.c',
	'src/sets-tags.c',
	'src/sets-condition.c',
	'src/interface-list-menu.c',
	'src/interface-status.c',
	'src/interface-input.c',
	'src/items.c',
	'src/items-format.c',
	'src/date-utils.c',
	'src/parse.c',
	'src/parse-format.c',
	'src/parse-format-generic.c',
	'src/parse-format-rss20.c',
	'src/parse-namespace.c',
	'src/parse-namespace-atom10.c',
	'src/parse-namespace-atom03.c',
	#'src/parse-namespace-mrss.c',
	'src/parse-namespace-rss10.c',
	#'src/parse-namespace-rss10content.c',
	'src/parse-namespace-dc.c',
	#'src/parse-namespace-rss10syndication.c',
	'src/contents.c',
	'src/contents-metadata.c',
	'src/html-entities.c',
	'src/html-plainifier.c',
]
feedeater_inc = include_directories('src')

if get_option('buildtype') == 'release'
	add_project_arguments(
		[
			'-fdiagnostics-color=never',
		],
		language : 'c',
	)
else
	add_project_arguments(
		[
			'-fanalyzer',
			'-fdiagnostics-color=always',
		],
		language : 'c',
	)
endif

executable(
	'feedeater',
	#link_args: ['-static'],
	sources: feedeater_src,
	dependencies: feedeater_dep,
	include_directories: feedeater_inc,
)
