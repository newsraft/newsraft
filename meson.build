project(
	'feedeater',
	'c',
	default_options: [
		'c_std=gnu18',
		'warning_level=3',
		'buildtype=release',
	],
	license: 'MIT',
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
		'-DFEEDEATER_FORMAT_SUPPORT_YANDEX',
	],
	language: 'c',
)

if get_option('buildtype') == 'release'
	add_project_arguments(
		[
			'-fdiagnostics-color=never',
		],
		language: 'c',
	)
else
	add_project_arguments(
		[
			'-fdiagnostics-color=always',
			#'-fanalyzer',
			#'-Og', # meson takes care of it
			#'-Xanalyzer', # for clang
			#'-Qunused-arguments', # for clang
		],
		language: 'c',
	)
	# run "ninja cppcheck" to run code analysis
	run_target('cppcheck', command: ['cppcheck', '--project=' + join_paths(meson.build_root(), 'compile_commands.json')])
endif

executable(
	'feedeater',
	include_directories: [
		include_directories('src'),
	],
	sources: [
		'src/feedeater.c',
		'src/log.c',
		'src/path.c',
		'src/string.c',
		'src/wstring.c',
		'src/db.c',
		'src/db-items.c',
		'src/config.c',
		'src/config-defaults.c',
		'src/config-verifier.c',
		'src/sets.c',
		'src/sets-tags.c',
		'src/sets-condition.c',
		'src/interface.c',
		'src/interface-input.c',
		'src/interface-status.c',
		'src/interface-menu-list.c',
		'src/interface-menu-list-format.c',
		'src/interface-menu-pager.c',
		'src/items.c',
		'src/items-links.c',
		'src/items-content.c',
		'src/items-metadata.c',
		'src/update_feed/update_feed.c',
		'src/update_feed/bucket.c',
		'src/update_feed/bucket-feed.c',
		'src/update_feed/bucket-feed-insert.c',
		'src/update_feed/bucket-item.c',
		'src/update_feed/bucket-item-insert.c',
		'src/update_feed/bucket-structure-link.c',
		'src/update_feed/bucket-structure-person.c',
		'src/update_feed/bucket-structure-generator.c',
		'src/update_feed/date-utils.c',
		'src/update_feed/xml.c',
		'src/update_feed/xml-handler.c',
		'src/update_feed/xml-handler-atom10.c',
		'src/update_feed/xml-handler-rss20.c',
		'src/update_feed/xml-handler-dc.c',
		'src/update_feed/xml-handler-yandex.c',
		'src/update_feed/xml-handler-rss10content.c',
		'src/update_feed/xml-handler-atom03.c',
		'src/update_feed/xml-handler-rss11.c',
		#'src/update_feed/xml-handler-mrss.c',
		#'src/update_feed/xml-handler-rss10syndication.c',
		'src/prepare_to_render_data/prepare_to_render_data.c',
		'src/prepare_to_render_data/prepare-text-html.c',
		'src/render_data/render_data.c',
		'src/render_data/line.c',
		'src/render_data/render-text-plain.c',
		'src/render_data/render-text-html.c',
		'src/render_data/render-text-html-table.c',
	],
	dependencies: [
		dependency('ncursesw'),
		dependency('libcurl'),
		dependency('expat'),
		dependency('sqlite3'),
		dependency('tidy'),
	],
	#link_args: ['-static'],
)
