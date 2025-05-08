cmake_minimum_required(VERSION 3.30)

project(wiender)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

file(GLOB SOURCE "src/*.cpp")

if(CMAKE_CXX_COMPILER STREQUAL "MSVC")
    message(FATAL_ERROR "MSVC isn't a good choice.")
endif()

if(WIN32)
    add_library(wiender STATIC ${SOURCE})
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
else()
    message(FATAL_ERROR "Unknown platform")
endif()

add_executable(basic_test tests/basic_test/basic_test.cpp)
target_include_directories(basic_test PRIVATE "./include" "./tests/tests_includes")
target_link_libraries(basic_test PRIVATE wiender)
add_test(NAME basic_test COMMAND basic_test)