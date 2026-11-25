# CMake-сборка hello.
# Сборка вне исходников:
#   cmake -S . -B build && cmake --build build && ctest --test-dir build
# С санитайзерами:
#   cmake -S . -B build-asan -DHELLO_SANITIZE=ON && cmake --build build-asan && ctest --test-dir build-asan
cmake_minimum_required(VERSION 3.16)
project(hello C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(HELLO_SANITIZE "Собрать с AddressSanitizer и UBSan" OFF)

# Одни предупреждения на все цели, предупреждение = ошибка.
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

if(HELLO_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=address,undefined)
endif()

# Каждый модуль — своя библиотека, заголовок отдаём наружу через PUBLIC.
add_library(greet STATIC greet.c)
target_include_directories(greet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(stats STATIC stats.c)
target_include_directories(stats PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(hello main.c)
target_link_libraries(hello PRIVATE greet stats)

# Те же запуски, что в make check.
enable_testing()
add_test(NAME no_args COMMAND hello)
add_test(NAME with_arg COMMAND hello Linux)
add_test(NAME empty_arg COMMAND hello "")
add_test(NAME cyrillic COMMAND hello "Привет 2024")
