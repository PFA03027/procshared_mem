cmake_minimum_required(VERSION 3.16)
project(
    libipsm_mem
    #
    DESCRIPTION "class of shared memory b/w processes"
    LANGUAGES CXX
    VERSION 1.0.0
    )

set(CMAKE_VERBOSE_MAKEFILE 1)

include(common.cmake)

# Switch build target
set(BUILD_CONFIG normal CACHE STRING "Build config?")
message("[BUILD] ${BUILD_CONFIG}")
include(${BUILD_CONFIG}.cmake)


enable_testing()

add_subdirectory(libipsm_mem)
add_subdirectory(test EXCLUDE_FROM_ALL)
