# Нужен только для одной вещи: clang-tidy требует compile_commands.json.
#   cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
# В список попадают все файлы, которые проверяет make tidy.
cmake_minimum_required(VERSION 3.16)
project(code-health C)

set(CMAKE_C_STANDARD 11)
include_directories(include)

# Библиотека, чтобы каждый .c попал в compile_commands.json один раз:
# иначе clang-tidy проверяет greeting.c столько раз, сколько программ его используют.
add_library(health STATIC src/greeting.c src/shorten.c)

add_executable(app src/main.c)
add_executable(test_greeting tests/test_greeting.c)
add_executable(test_shorten tests/test_shorten.c)
add_executable(fuzz_greeting fuzz/fuzz_greeting.c)
foreach(prog app test_greeting test_shorten fuzz_greeting)
    target_link_libraries(${prog} health)
endforeach()

# «Грязный» файл с лекции не собирается (gets), поэтому не входит в all.
add_executable(lint-issues EXCLUDE_FROM_ALL lint/issues.c)
