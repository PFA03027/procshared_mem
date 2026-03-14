cmake_minimum_required(VERSION 3.16)

file(GLOB SOURCES src/*.cpp )
file(GLOB INSTALL_PUBLIC_HEADER_FILES  inc/*.hpp )

# IPSM_BUILD_SHARED_LIBSというオプションを作成。デフォルトをONに設定。
option(IPSM_BUILD_SHARED_LIBS "build ipsm_mem as a shared library" ON)

if (IPSM_BUILD_SHARED_LIBS)
  add_library(ipsm_mem SHARED ${SOURCES} )
else()
  add_library(ipsm_mem STATIC ${SOURCES} )
endif()

target_include_directories( ipsm_mem  PUBLIC inc )
target_compile_options( ipsm_mem  PRIVATE -Wall -Wconversion -Wsign-conversion -Werror )
# target_compile_definitions(ipsm_mem PUBLIC ENABLE_DELEGATION_OF_FILTERING)  # function option
# target_compile_definitions(ipsm_mem PUBLIC ENABLE_DEBUG_LOGOUTPUT)          # for test purpose
# target_compile_definitions(ipsm_mem PUBLIC ENABLE_BACKTRACE_LOGOUTPUT)      # for test purpose
# target_compile_definitions(ipsm_mem PUBLIC ENABLE_PTHREAD_MUTEX_ERRORTYPE)	# for test purpose

set_target_properties(ipsm_mem PROPERTIES PUBLIC_HEADER  "${INSTALL_PUBLIC_HEADER_FILES}")

include(GNUInstallDirs)
install(TARGETS ipsm_mem
        PUBLIC_HEADER
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/
)
