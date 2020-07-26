cmake_minimum_required(VERSION 3.16)
project(meejson VERSION 0.1.0 LANGUAGES CXX)

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in ../cmake-build-debug/googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
endif()

add_library(meejson STATIC
        include/meejson/array.hpp
        include/meejson/box.hpp
        include/meejson/detail.hpp
        include/meejson/except.hpp
        include/meejson/lexer.hpp
        include/meejson/object.hpp
        include/meejson/parser.hpp
        include/meejson/type_list.hpp
        include/meejson/value.hpp)

target_sources(meejson PRIVATE
        src/except.cpp
        src/lexer.cpp
        src/parser.cpp)

set_target_properties(meejson PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF LINKER_LANGUAGE CXX)

add_executable(tests test/value.cpp test/parser.cpp)
target_link_libraries(tests gtest_main meejson)
set_target_properties(tests PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF LINKER_LANGUAGE CXX)
add_test(NAME value_test COMMAND value)

if(MSVC)
else()
    target_compile_options(meejson PUBLIC -Wall -Wextra -pedantic -Werror)
    target_compile_options(tests PUBLIC -Wall -Wextra -pedantic -Werror)
endif()