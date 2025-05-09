cmake_minimum_required(VERSION 3.30)

project(wiender)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endforeach()

option(WIERENDER_BUILD_SHARED_LIBS "Build shared library (DLL)" OFF)
option(WIERENDER_BUILD_STATIC_LIBS "Build static library" ON)
option(WIERENDER_BUILD_BASIC_TEST "Build basic_test executable" ON)

file(GLOB SOURCE "src/*.cpp")

if(CMAKE_CXX_COMPILER STREQUAL "MSVC")
    message(FATAL_ERROR "MSVC isn't a good choice.")
endif()

if(WIERENDER_BUILD_SHARED_LIBS)
    add_library(wiender SHARED ${SOURCE})
    set_target_properties(wiender PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
    message(STATUS "Building wiender as SHARED library (DLL)")
elseif(WIERENDER_BUILD_STATIC_LIBS)
    add_library(wiender STATIC ${SOURCE})
    set_target_properties(wiender PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    )
    message(STATUS "Building wiender as STATIC library")
endif()
target_include_directories(wiender PUBLIC "./includes")
find_package(Vulkan REQUIRED)
target_link_libraries(wiender PUBLIC Vulkan::Vulkan)
include(FetchContent)
FetchContent_Declare(
    spirv_reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG main
)
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(spirv_reflect)
target_include_directories(wiender PUBLIC ${spirv_reflect_SOURCE_DIR})
target_link_libraries(wiender PUBLIC spirv-reflect-static)

if(WIERENDER_BUILD_BASIC_TEST)
    add_executable(basic_test tests/basic_test/basic_test.cpp)
    set_target_properties(basic_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )
    target_include_directories(basic_test PRIVATE "./include" "./tests/tests_includes")
    target_link_libraries(basic_test PRIVATE wiender)
    add_test(NAME basic_test COMMAND basic_test)
    message(STATUS "Building basic_test executable")

    if(WIN32)
        set(RUN_SCRIPT "run_basic_test.bat")
        file(WRITE ${RUN_SCRIPT} "@echo off\n")
        file(APPEND ${RUN_SCRIPT} "cd /D \"%~dp0\"\n")
        file(APPEND ${RUN_SCRIPT} "\"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/basic_test.exe\"\n")
        file(APPEND ${RUN_SCRIPT} "if %errorlevel% neq 0 pause\n")
    else()
        set(RUN_SCRIPT "run_basic_test.sh")
        file(WRITE ${RUN_SCRIPT} "#!/bin/sh\n")
        file(APPEND ${RUN_SCRIPT} "cd \"$(dirname \"$0\")\"\n")
        file(APPEND ${RUN_SCRIPT} "\"./bin/basic_test\"\n")
        file(APPEND ${RUN_SCRIPT} "if [ $? -ne 0 ]; then read -p \"Press enter to continue\"; fi\n")
        
        execute_process(COMMAND chmod +x ${RUN_SCRIPT})
    endif()

    message(STATUS "Created test runner script: ${RUN_SCRIPT}")
endif()