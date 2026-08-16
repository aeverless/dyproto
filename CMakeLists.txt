cmake_minimum_required(VERSION 4.3.2)

project(dyproto LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_compile_options(-O3 -Wpedantic -Wall)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	message(STATUS "Compiling with Clang. Suppressing `c2y-extensions` warnings.")
	add_compile_options(--warning-suppression-mappings=${PROJECT_SOURCE_DIR}/suppression_mappings.txt)
endif()

include_directories(include)

add_library(${PROJECT_NAME} INTERFACE)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include/dyproto/dyproto.hpp)

if (BUILD_TESTING)
	add_executable(${PROJECT_NAME}-tests tests/rw.cpp)
	target_compile_features(${PROJECT_NAME}-tests PRIVATE cxx_std_23)

	add_test(NAME tests COMMAND ${PROJECT_NAME}-tests)

	add_subdirectory(thirdparty)

	catch_discover_tests(${PROJECT_NAME}-tests)
endif()
