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


if (IPSM_BUILD_SHARED_LIBS)
  add_library(test_ipsm_common SHARED test_ipsm_common.cpp )
else()
  add_library(test_ipsm_common STATIC test_ipsm_common.cpp )
endif()

add_executable(loadtest_ipsm_mem_both_highload EXCLUDE_FROM_ALL test_ipsm_mem_both_highload.cpp)
target_link_libraries(loadtest_ipsm_mem_both_highload ipsm_mem )
add_dependencies(build-test loadtest_ipsm_mem_both_highload)

add_executable(loadtest_ipsm_mem_primary_highload EXCLUDE_FROM_ALL test_ipsm_mem_primary_highload.cpp)
target_link_libraries(loadtest_ipsm_mem_primary_highload ipsm_mem )
add_dependencies(build-test loadtest_ipsm_mem_primary_highload)

add_executable(loadtest_ipsm_mem_secondary_highload EXCLUDE_FROM_ALL test_ipsm_mem_secondary_highload.cpp)
target_link_libraries(loadtest_ipsm_mem_secondary_highload ipsm_mem )
add_dependencies(build-test loadtest_ipsm_mem_secondary_highload)

add_executable(test_ipsm_time_util EXCLUDE_FROM_ALL test_ipsm_time_util.cpp)
target_link_libraries(test_ipsm_time_util ipsm_mem gtest gtest_main )
add_dependencies(build-test test_ipsm_time_util)
add_test(NAME test_ipsm_time_util COMMAND $<TARGET_FILE:test_ipsm_time_util>)

add_executable(test_ipsm_mutex EXCLUDE_FROM_ALL test_ipsm_mutex.cpp)
target_link_libraries(test_ipsm_mutex ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_ipsm_mutex)
add_test(NAME test_ipsm_mutex COMMAND $<TARGET_FILE:test_ipsm_mutex>)

add_executable(test_ipsm_condition_variable EXCLUDE_FROM_ALL test_ipsm_condition_variable.cpp)
target_link_libraries(test_ipsm_condition_variable ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_ipsm_condition_variable)
add_test(NAME test_ipsm_condition_variable COMMAND $<TARGET_FILE:test_ipsm_condition_variable>)

add_executable(test_ipsm_mem EXCLUDE_FROM_ALL test_ipsm_mem.cpp)
target_link_libraries(test_ipsm_mem ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_ipsm_mem)
add_test(NAME test_ipsm_mem COMMAND $<TARGET_FILE:test_ipsm_mem>)

add_executable(test_ipsm_malloc EXCLUDE_FROM_ALL test_ipsm_malloc.cpp)
target_link_libraries(test_ipsm_malloc ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_ipsm_malloc)
add_test(NAME test_ipsm_malloc COMMAND $<TARGET_FILE:test_ipsm_malloc>)

add_executable(loadtest_ipsm_malloc_highload EXCLUDE_FROM_ALL test_ipsm_malloc_highload.cpp)
target_link_libraries(loadtest_ipsm_malloc_highload ipsm_mem )
add_dependencies(build-test loadtest_ipsm_malloc_highload)

add_executable(loadtest_ipsm_malloc_highload_sub EXCLUDE_FROM_ALL test_ipsm_malloc_highload_sub.cpp)
target_link_libraries(loadtest_ipsm_malloc_highload_sub ipsm_mem )
add_dependencies(build-test loadtest_ipsm_malloc_highload_sub)

add_executable(test_ipsm_mem_internal EXCLUDE_FROM_ALL test_ipsm_mem_internal_idfile.cpp test_ipsm_mem_internal_sem.cpp test_ipsm_mem_internal_shm.cpp)
target_include_directories( test_ipsm_mem_internal  PRIVATE ../libipsm_mem/src )
target_link_libraries(test_ipsm_mem_internal ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_ipsm_mem_internal)
add_test(NAME test_ipsm_mem_internal COMMAND $<TARGET_FILE:test_ipsm_mem_internal>)

add_executable(test_offset_malloc_impl EXCLUDE_FROM_ALL test_offset_malloc_impl.cpp)
target_include_directories(test_offset_malloc_impl  PUBLIC ../libipsm_mem/src  )
target_link_libraries(test_offset_malloc_impl ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_offset_malloc_impl)
add_test(NAME test_offset_malloc_impl COMMAND $<TARGET_FILE:test_offset_malloc_impl>)

add_executable(test_offset_malloc EXCLUDE_FROM_ALL test_offset_malloc.cpp)
target_include_directories(test_offset_malloc  PUBLIC ../libipsm_mem/src  )
target_link_libraries(test_offset_malloc ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_offset_malloc)
add_test(NAME test_offset_malloc COMMAND $<TARGET_FILE:test_offset_malloc>)

add_executable(test_offset_allocator EXCLUDE_FROM_ALL test_offset_allocator.cpp)
target_include_directories(test_offset_allocator  PUBLIC ../libipsm_mem/src  )
target_link_libraries(test_offset_allocator ipsm_mem test_ipsm_common gtest gtest_main )
add_dependencies(build-test test_offset_allocator)
add_test(NAME test_offset_allocator COMMAND $<TARGET_FILE:test_offset_allocator>)

add_executable(test_offset_ptr11 EXCLUDE_FROM_ALL test_offset_ptr.cpp)
# target_include_directories(test_offset_ptr11  PUBLIC ../inc  )
target_link_libraries(test_offset_ptr11 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_offset_ptr11)
add_test(NAME test_offset_ptr11 COMMAND $<TARGET_FILE:test_offset_ptr11>)
target_compile_features(test_offset_ptr11 PUBLIC cxx_std_11)

add_executable(test_offset_ptr20 EXCLUDE_FROM_ALL test_offset_ptr.cpp)
# target_include_directories(test_offset_ptr20  PUBLIC ../inc  )
target_link_libraries(test_offset_ptr20 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_offset_ptr20)
add_test(NAME test_offset_ptr20 COMMAND $<TARGET_FILE:test_offset_ptr20>)
target_compile_features(test_offset_ptr20 PUBLIC cxx_std_20)
# target_compile_options(test_offset_ptr20 PUBLIC -fconcepts-diagnostics-depth=2)

add_executable(test_offset_unique_ptr11 EXCLUDE_FROM_ALL test_offset_unique_ptr.cpp)
# target_include_directories(test_offset_unique_ptr11  PUBLIC ../inc  )
target_link_libraries(test_offset_unique_ptr11 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_offset_unique_ptr11)
add_test(NAME test_offset_unique_ptr11 COMMAND $<TARGET_FILE:test_offset_unique_ptr11>)
target_compile_features(test_offset_unique_ptr11 PUBLIC cxx_std_11)

add_executable(test_offset_unique_ptr20 EXCLUDE_FROM_ALL test_offset_unique_ptr.cpp)
# target_include_directories(test_offset_unique_ptr20  PUBLIC ../inc  )
target_link_libraries(test_offset_unique_ptr20 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_offset_unique_ptr20)
add_test(NAME test_offset_unique_ptr20 COMMAND $<TARGET_FILE:test_offset_unique_ptr20>)
target_compile_features(test_offset_unique_ptr20 PUBLIC cxx_std_20)
# target_compile_options(test_offset_unique_ptr20 PUBLIC -fconcepts-diagnostics-depth=2)

add_executable(test_offset_shared_ptr11 EXCLUDE_FROM_ALL test_offset_shared_ptr.cpp test_offset_weak_ptr.cpp test_offset_shared_weak_highload.cpp)
# target_include_directories(test_offset_shared_ptr11  PUBLIC ../inc  )
target_link_libraries(test_offset_shared_ptr11 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_offset_shared_ptr11)
add_test(NAME test_offset_shared_ptr11 COMMAND $<TARGET_FILE:test_offset_shared_ptr11>)
target_compile_features(test_offset_shared_ptr11 PUBLIC cxx_std_11)

add_executable(test_offset_shared_ptr20 EXCLUDE_FROM_ALL test_offset_shared_ptr.cpp test_offset_weak_ptr.cpp test_offset_shared_weak_highload.cpp)
# target_include_directories(test_offset_shared_ptr20  PUBLIC ../inc  )
target_link_libraries(test_offset_shared_ptr20 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_offset_shared_ptr20)
add_test(NAME test_offset_shared_ptr20 COMMAND $<TARGET_FILE:test_offset_shared_ptr20>)
target_compile_features(test_offset_shared_ptr20 PUBLIC cxx_std_20)
# target_compile_options(test_offset_shared_ptr20 PUBLIC -fconcepts-diagnostics-depth=2)

add_executable(test_offset_list11 EXCLUDE_FROM_ALL test_offset_list.cpp)
# target_include_directories(test_offset_list11  PUBLIC ../inc  )
target_link_libraries(test_offset_list11 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_offset_list11)
add_test(NAME test_offset_list11 COMMAND $<TARGET_FILE:test_offset_list11>)
target_compile_features(test_offset_list11 PUBLIC cxx_std_11)

add_executable(test_offset_list20 EXCLUDE_FROM_ALL test_offset_list.cpp)
# target_include_directories(test_offset_list20  PUBLIC ../inc  )
target_link_libraries(test_offset_list20 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_offset_list20)
add_test(NAME test_offset_list20 COMMAND $<TARGET_FILE:test_offset_list20>)
target_compile_features(test_offset_unique_ptr20 PUBLIC cxx_std_20)
# target_compile_options(test_offset_unique_ptr20 PUBLIC -fconcepts-diagnostics-depth=2)

add_executable(test_misc_utility_and_other11 EXCLUDE_FROM_ALL test_misc_utility_and_other.cpp)
target_include_directories(test_misc_utility_and_other11  PUBLIC ../libipsm_mem/src )
target_link_libraries(test_misc_utility_and_other11 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_misc_utility_and_other11)
add_test(NAME test_misc_utility_and_other11 COMMAND $<TARGET_FILE:test_misc_utility_and_other11>)
target_compile_features(test_misc_utility_and_other11 PUBLIC cxx_std_11)

add_executable(test_misc_utility_and_other20 EXCLUDE_FROM_ALL test_misc_utility_and_other.cpp)
target_include_directories(test_misc_utility_and_other20  PUBLIC ../libipsm_mem/src )
target_link_libraries(test_misc_utility_and_other20 ipsm_mem gtest gtest_main )
add_dependencies(build-test test_misc_utility_and_other20)
add_test(NAME test_misc_utility_and_other20 COMMAND $<TARGET_FILE:test_misc_utility_and_other20>)
target_compile_features(test_misc_utility_and_other20 PUBLIC cxx_std_20)
# target_compile_options(test_offset_unique_ptr20 PUBLIC -fconcepts-diagnostics-depth=2)

