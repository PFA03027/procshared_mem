cmake_minimum_required(VERSION 3.16)

include(FetchContent)
FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
)
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


add_custom_target(build-test)


########################
if (IPSM_BUILD_SHARED_LIBS)
  add_library(test_ipsm_common SHARED test_ipsm_common/test_ipsm_common.cpp )
else()
  add_library(test_ipsm_common STATIC test_ipsm_common/test_ipsm_common.cpp )
endif()
target_include_directories(test_ipsm_common  PUBLIC test_ipsm_common/ ../libipsm_mem/src )

########################
add_executable(test_ipsm_functions EXCLUDE_FROM_ALL)
target_sources(test_ipsm_functions PRIVATE
  test_ipsm_functions/test_ipsm_time_util.cpp
  test_ipsm_functions/test_ipsm_mutex.cpp
  test_ipsm_functions/test_ipsm_condition_variable.cpp
  test_ipsm_functions/test_ipsm_malloc.cpp
  test_ipsm_functions/test_ipsm_mem.cpp
  test_ipsm_functions/test_misc_utility_and_other.cpp
  )
target_include_directories( test_ipsm_functions  PRIVATE ../libipsm_mem/src )
target_link_libraries(test_ipsm_functions ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_ipsm_functions)
add_test(NAME test_ipsm_functions COMMAND $<TARGET_FILE:test_ipsm_functions>)

########################
set(TEST_OFFSET_FUNCTIONS_SOURCES
  test_offset_functions/test_offset_ptr.cpp
  test_offset_functions/test_offset_unique_ptr.cpp
  test_offset_functions/test_offset_list.cpp
  test_offset_functions/test_offset_shared_ptr.cpp
  test_offset_functions/test_offset_weak_ptr.cpp
  test_offset_functions/test_offset_shared_weak_highload.cpp

  test_offset_functions/test_offset_allocator.cpp
  test_offset_functions/test_offset_malloc_impl.cpp
  test_offset_functions/test_offset_malloc.cpp
  )

add_executable(test_offset_functions EXCLUDE_FROM_ALL ${TEST_OFFSET_FUNCTIONS_SOURCES})
# target_include_directories(test_offset_functions_cpp  PUBLIC ../inc  )
target_link_libraries(test_offset_functions ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_offset_functions)
add_test(NAME test_offset_functions COMMAND $<TARGET_FILE:test_offset_functions>)


##############################
add_executable(loadtest_ipsm_malloc_highload EXCLUDE_FROM_ALL test_ipsm_malloc_highload.cpp)
target_link_libraries(loadtest_ipsm_malloc_highload ipsm_mem )
add_dependencies(build-test loadtest_ipsm_malloc_highload)

add_executable(loadtest_ipsm_malloc_highload_sub EXCLUDE_FROM_ALL test_ipsm_malloc_highload_sub.cpp)
target_link_libraries(loadtest_ipsm_malloc_highload_sub ipsm_mem )
add_dependencies(build-test loadtest_ipsm_malloc_highload_sub)

add_executable(loadtest_ipsm_mem_setup_highload EXCLUDE_FROM_ALL test_ipsm_mem_setup_highload.cpp)
target_link_libraries(loadtest_ipsm_mem_setup_highload ipsm_mem )
add_dependencies(build-test loadtest_ipsm_mem_setup_highload)

add_executable(loadtest_ipsm_mem_setup_highload_subprocess EXCLUDE_FROM_ALL test_ipsm_mem_setup_highload_subprocess.cpp)
target_link_libraries(loadtest_ipsm_mem_setup_highload_subprocess ipsm_mem )
add_dependencies(build-test loadtest_ipsm_mem_setup_highload_subprocess)
