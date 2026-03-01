cmake_minimum_required(VERSION 3.16)
project(
    libipsm_mem
    #
    DESCRIPTION "class of shared memory b/w processes"
    LANGUAGES CXX
    VERSION 1.0.0
    )

set(CMAKE_VERBOSE_MAKEFILE 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# set(CMAKE_UNITY_BUILD TRUE)

# Switch build target
set(BUILD_CONFIG common CACHE STRING "Build config?")
message("[BUILD] ${BUILD_CONFIG}")
include(${BUILD_CONFIG}.cmake)

if("${CMAKE_BUILD_TYPE}" EQUAL "Debug")
 include(debug_option.cmake)
endif()

enable_testing()

add_subdirectory(libipsm_mem)
add_subdirectory(test EXCLUDE_FROM_ALL)
