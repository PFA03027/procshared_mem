cmake_minimum_required(VERSION 3.16)

find_package(Git QUIET)

# Gitがインストールされており、トップディレクトリに.gitディレクトリがある
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    enable_testing()
    option(GOOGLETEST "Check Google Test during build" ON)
    # -DGOOGLETEST=onオプションを付けて実行したらsubmoduleを最新版にする
    if(GOOGLETEST)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/test/googletest/CMakeLists.txt")
message(FATAL_ERROR "The submodules were not downloaded! GOOGLETEST was turned off or failed. Please update submodules and try again.")
endif()

# Google Testの不要なキャッシュ変数をオフにしておく
option(BUILD_GMOCK "Builds the googlemock subprojects" OFF)
option(INSTALL_GTEST "Enables installation of googletest" OFF)

add_custom_target(build-test)

add_subdirectory(googletest)

if (ALCONCURRENT_BUILD_SHARED_LIBS)
  add_library(test_procshared_common SHARED test_procshared_common.cpp )
else()
  add_library(test_procshared_common STATIC test_procshared_common.cpp )
endif()

add_executable(loadtest_procshared_mem_both_highload EXCLUDE_FROM_ALL test_procshared_mem_both_highload.cpp)
target_link_libraries(loadtest_procshared_mem_both_highload procshared_mem )
add_dependencies(build-test loadtest_procshared_mem_both_highload)

add_executable(loadtest_procshared_mem_primary_highload EXCLUDE_FROM_ALL test_procshared_mem_primary_highload.cpp)
target_link_libraries(loadtest_procshared_mem_primary_highload procshared_mem )
add_dependencies(build-test loadtest_procshared_mem_primary_highload)

add_executable(loadtest_procshared_mem_secondary_highload EXCLUDE_FROM_ALL test_procshared_mem_secondary_highload.cpp)
target_link_libraries(loadtest_procshared_mem_secondary_highload procshared_mem )
add_dependencies(build-test loadtest_procshared_mem_secondary_highload)

add_executable(test_procshared_mutex EXCLUDE_FROM_ALL test_procshared_mutex.cpp)
target_link_libraries(test_procshared_mutex procshared_mem test_procshared_common gtest gtest_main )
add_dependencies(build-test test_procshared_mutex)
add_test(NAME test_procshared_mutex COMMAND $<TARGET_FILE:test_procshared_mutex>)

add_executable(test_procshared_condition_variable EXCLUDE_FROM_ALL test_procshared_condition_variable.cpp)
target_link_libraries(test_procshared_condition_variable procshared_mem test_procshared_common gtest gtest_main )
add_dependencies(build-test test_procshared_condition_variable)
add_test(NAME test_procshared_condition_variable COMMAND $<TARGET_FILE:test_procshared_condition_variable>)

add_executable(test_procshared_mem EXCLUDE_FROM_ALL test_procshared_mem.cpp)
target_link_libraries(test_procshared_mem procshared_mem test_procshared_common gtest gtest_main )
add_dependencies(build-test test_procshared_mem)
add_test(NAME test_procshared_mem COMMAND $<TARGET_FILE:test_procshared_mem>)

add_executable(test_procshared_mem_internal EXCLUDE_FROM_ALL test_procshared_mem_internal_idfile.cpp test_procshared_mem_internal_sem.cpp test_procshared_mem_internal_shm.cpp)
target_include_directories( test_procshared_mem_internal  PRIVATE ../src )
target_link_libraries(test_procshared_mem_internal procshared_mem test_procshared_common gtest gtest_main )
add_dependencies(build-test test_procshared_mem_internal)
add_test(NAME test_procshared_mem_internal COMMAND $<TARGET_FILE:test_procshared_mem_internal>)

add_executable(test_offset_krmalloc EXCLUDE_FROM_ALL test_offset_krmalloc.cpp)
target_include_directories(test_offset_krmalloc  PUBLIC ../src  )
target_link_libraries(test_offset_krmalloc procshared_mem test_procshared_common gtest gtest_main )
add_dependencies(build-test test_offset_krmalloc)
add_test(NAME test_offset_krmalloc COMMAND $<TARGET_FILE:test_offset_krmalloc>)

add_executable(test_offset_mem_allocator EXCLUDE_FROM_ALL test_offset_mem_allocator.cpp)
target_include_directories(test_offset_mem_allocator  PUBLIC ../src  )
target_link_libraries(test_offset_mem_allocator procshared_mem test_procshared_common gtest gtest_main )
add_dependencies(build-test test_offset_mem_allocator)
add_test(NAME test_offset_mem_allocator COMMAND $<TARGET_FILE:test_offset_mem_allocator>)

add_executable(test_offset_ptr11 EXCLUDE_FROM_ALL test_offset_ptr.cpp)
target_include_directories(test_offset_ptr11  PUBLIC ../inc  )
target_link_libraries(test_offset_ptr11 gtest gtest_main )
add_dependencies(build-test test_offset_ptr11)
add_test(NAME test_offset_ptr11 COMMAND $<TARGET_FILE:test_offset_ptr11>)
target_compile_features(test_offset_ptr11 PUBLIC cxx_std_11)

add_executable(test_offset_ptr20 EXCLUDE_FROM_ALL test_offset_ptr.cpp)
target_include_directories(test_offset_ptr20  PUBLIC ../inc  )
target_link_libraries(test_offset_ptr20 gtest gtest_main )
add_dependencies(build-test test_offset_ptr20)
add_test(NAME test_offset_ptr20 COMMAND $<TARGET_FILE:test_offset_ptr20>)
target_compile_features(test_offset_ptr20 PUBLIC cxx_std_20)
target_compile_options(test_offset_ptr20 PUBLIC -fconcepts-diagnostics-depth=2)

add_executable(test_offset_unique_ptr11 EXCLUDE_FROM_ALL test_offset_unique_ptr.cpp)
target_include_directories(test_offset_unique_ptr11  PUBLIC ../inc  )
target_link_libraries(test_offset_unique_ptr11 gtest gtest_main )
add_dependencies(build-test test_offset_unique_ptr11)
add_test(NAME test_offset_unique_ptr11 COMMAND $<TARGET_FILE:test_offset_unique_ptr11>)
target_compile_features(test_offset_unique_ptr11 PUBLIC cxx_std_11)

add_executable(test_offset_unique_ptr20 EXCLUDE_FROM_ALL test_offset_unique_ptr.cpp)
target_include_directories(test_offset_unique_ptr20  PUBLIC ../inc  )
target_link_libraries(test_offset_unique_ptr20 gtest gtest_main )
add_dependencies(build-test test_offset_unique_ptr20)
add_test(NAME test_offset_unique_ptr20 COMMAND $<TARGET_FILE:test_offset_unique_ptr20>)
target_compile_features(test_offset_unique_ptr20 PUBLIC cxx_std_20)
target_compile_options(test_offset_unique_ptr20 PUBLIC -fconcepts-diagnostics-depth=2)

add_executable(test_offset_shared_ptr11 EXCLUDE_FROM_ALL test_offset_shared_ptr.cpp test_offset_weak_ptr.cpp test_offset_shared_weak_highload.cpp)
target_include_directories(test_offset_shared_ptr11  PUBLIC ../inc  )
target_link_libraries(test_offset_shared_ptr11 procshared_mem gtest gtest_main )
add_dependencies(build-test test_offset_shared_ptr11)
add_test(NAME test_offset_shared_ptr11 COMMAND $<TARGET_FILE:test_offset_shared_ptr11>)
target_compile_features(test_offset_shared_ptr11 PUBLIC cxx_std_11)

add_executable(test_offset_shared_ptr20 EXCLUDE_FROM_ALL test_offset_shared_ptr.cpp test_offset_weak_ptr.cpp test_offset_shared_weak_highload.cpp)
target_include_directories(test_offset_shared_ptr20  PUBLIC ../inc  )
target_link_libraries(test_offset_shared_ptr20 procshared_mem gtest gtest_main )
add_dependencies(build-test test_offset_shared_ptr20)
add_test(NAME test_offset_shared_ptr20 COMMAND $<TARGET_FILE:test_offset_shared_ptr20>)
target_compile_features(test_offset_shared_ptr20 PUBLIC cxx_std_20)
target_compile_options(test_offset_shared_ptr20 PUBLIC -fconcepts-diagnostics-depth=2)
