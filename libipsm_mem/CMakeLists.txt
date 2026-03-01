cmake_minimum_required(VERSION 3.16)

file(GLOB SOURCES src/*.cpp )
file(GLOB INSTALL_PUBLIC_HEADER_FILES  inc/*.hpp )


file(GLOB SOURCES src/*.cpp )

# IPSM_BUILD_SHARED_LIBSというオプションを作成。デフォルトをONに設定。
option(IPSM_BUILD_SHARED_LIBS "build ipsm_mem as a shared library" ON)

if (IPSM_BUILD_SHARED_LIBS)
  add_library(ipsm_mem SHARED ${SOURCES} )
else()
  add_library(ipsm_mem STATIC ${SOURCES} )
endif()

target_include_directories( ipsm_mem  PUBLIC inc )
target_compile_options( ipsm_mem  PRIVATE "-Werror" )

set_target_properties(ipsm_mem PROPERTIES PUBLIC_HEADER  "${INSTALL_PUBLIC_HEADER_FILES}")

include(GNUInstallDirs)
install(TARGETS ipsm_mem
        PUBLIC_HEADER
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
)
