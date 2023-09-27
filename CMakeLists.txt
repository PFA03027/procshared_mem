cmake_minimum_required(VERSION 3.1)
project(libprocshared_mem CXX)
set(CMAKE_VERBOSE_MAKEFILE 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Switch build target
set(BUILD_TARGET common CACHE STRING "Build target?")
message("[BUILD] ${BUILD_TARGET}")
include(${BUILD_TARGET}.cmake)


enable_testing()

file(GLOB SOURCES src/*.cpp )

# GREETINGS_BUILD_SHARED_LIBSというオプションを作成。デフォルトをOFFに設定。
option(ALCONCURRENT_BUILD_SHARED_LIBS "build procshared_mem as a shared library" OFF)

if (ALCONCURRENT_BUILD_SHARED_LIBS)
  add_library(procshared_mem SHARED ${SOURCES} )
else()
  add_library(procshared_mem STATIC ${SOURCES} )
endif()

target_include_directories(procshared_mem  PUBLIC inc  )

add_subdirectory(test)
