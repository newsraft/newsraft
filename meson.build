project(
	'feedeater',
	'c',
	license: 'MIT',
	default_options: [
		'c_std=c18',
		'warning_level=2',
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
		# support for various formats
		'-DFEEDEATER_FORMAT_SUPPORT_RSS20',        # RSS 0.91, 0.92, 0.93, 0.94, 2.0
		'-DFEEDEATER_FORMAT_SUPPORT_RSS11',        # RSS 0.9, 1.0, 1.1
		'-DFEEDEATER_FORMAT_SUPPORT_ATOM10',       # Atom 1.0
		'-DFEEDEATER_FORMAT_SUPPORT_ATOM03',       # Atom 0.3
		'-DFEEDEATER_FORMAT_SUPPORT_DUBLINCORE',
		'-DFEEDEATER_FORMAT_SUPPORT_RSS10CONTENT',
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
	'src/log.c',
	'src/path.c',
	'src/string.c',
	'src/wstring.c',
	'src/db.c',
	'src/db-items.c',
	'src/config.c',
	'src/format.c',
	'src/config-defaults.c',
	'src/sets.c',
	'src/sets-tags.c',
	'src/sets-condition.c',
	'src/interface.c',
	'src/interface-list-menu.c',
	'src/interface-status.c',
	'src/interface-input.c',
	'src/interface-pager-menu.c',
	'src/items.c',
	'src/items-metadata.c',
	'src/date-utils.c',
	'src/html-entities.c',
	'src/html-plainifier.c',
	'src/update_feed/update_feed.c',
	'src/update_feed/item-bucket.c',
	'src/update_feed/db-insert-item.c',
	'src/update_feed/db-update-feed.c',
	'src/update_feed/xml.c',
	'src/update_feed/xml-format-rss20.c',
	'src/update_feed/xml-namespace.c',
	'src/update_feed/xml-namespace-atom10.c',
	'src/update_feed/xml-namespace-atom03.c',
	#'src/update_feed/xml-namespace-mrss.c',
	'src/update_feed/xml-namespace-rss11.c',
	'src/update_feed/xml-namespace-rss10content.c',
	'src/update_feed/xml-namespace-dc.c',
	#'src/update_feed/xml-namespace-rss10syndication.c',
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
			'-Og', # better debugging experience
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
