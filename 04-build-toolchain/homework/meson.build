# meson.build — то же приложение, описанное для Meson.
# Сборка:  meson setup builddir && meson compile -C builddir
# С другими системами не мешает: Makefile и CMakeLists.txt рядом.

project('hello', 'c',
  version: '1.1.0',
  default_options: ['c_std=c11', 'warning_level=3', 'werror=true'])

greet = static_library('greet', 'greet.c')
stats = static_library('stats', 'stats.c')

executable('hello', 'main.c', link_with: [greet, stats])
