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

cc = meson.get_compiler('c')
libgetfeed = cc.find_library('getfeed')

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
		'src/update_feed/bucket-feed-insert.c',
		'src/update_feed/bucket-item-insert.c',
		'src/update_feed/bucket-structure-link.c',
		'src/update_feed/bucket-structure-person.c',
		'src/update_feed/bucket-structure-generator.c',
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
		libgetfeed,
		dependency('sqlite3'),
		dependency('tidy'),
	],
	#link_args: ['-static'],
)
