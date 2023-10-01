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

add_custom_target(build-test)

add_subdirectory(googletest)

if (ALCONCURRENT_BUILD_SHARED_LIBS)
  add_library(test_procshared_common SHARED test_procshared_common.cpp )
else()
  add_library(test_procshared_common STATIC test_procshared_common.cpp )
endif()

add_executable(test_procshared_mem_both_highload EXCLUDE_FROM_ALL test_procshared_mem_both_highload.cpp)
target_link_libraries(test_procshared_mem_both_highload procshared_mem )
add_dependencies(build-test test_procshared_mem_both_highload)
# add_test(NAME test_procshared_mem_both_highload COMMAND $<TARGET_FILE:test_procshared_mem_both_highload>)

add_executable(test_procshared_mem_primary_highload EXCLUDE_FROM_ALL test_procshared_mem_primary_highload.cpp)
target_link_libraries(test_procshared_mem_primary_highload procshared_mem )
add_dependencies(build-test test_procshared_mem_primary_highload)
# add_test(NAME test_procshared_mem_primary_highload COMMAND $<TARGET_FILE:test_procshared_mem_primary_highload>)

add_executable(test_procshared_mem_secondary_highload EXCLUDE_FROM_ALL test_procshared_mem_secondary_highload.cpp)
target_link_libraries(test_procshared_mem_secondary_highload procshared_mem )
add_dependencies(build-test test_procshared_mem_secondary_highload)

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

add_executable(test_offset_ptr EXCLUDE_FROM_ALL test_offset_ptr.cpp)
target_include_directories(test_offset_ptr  PUBLIC ../inc  )
target_link_libraries(test_offset_ptr gtest gtest_main )
add_dependencies(build-test test_offset_ptr)
add_test(NAME test_offset_ptr COMMAND $<TARGET_FILE:test_offset_ptr>)
target_compile_features(test_offset_ptr PUBLIC cxx_std_20)
target_compile_options(test_offset_ptr PUBLIC -fconcepts-diagnostics-depth=2)