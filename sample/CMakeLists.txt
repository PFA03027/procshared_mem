cmake_minimum_required(VERSION 3.16)

add_custom_target(build-sample)


########################
add_executable(sample_01_msg_exchange_via_shared_memory EXCLUDE_FROM_ALL)
target_sources(sample_01_msg_exchange_via_shared_memory PRIVATE
  sample_01_msg_exchange_via_shared_memory.cpp
  )
target_link_libraries(sample_01_msg_exchange_via_shared_memory ipsm_mem gtest gtest_main )
target_compile_options( sample_01_msg_exchange_via_shared_memory  PRIVATE -Wall -Wconversion -Wsign-conversion -Werror )
add_dependencies(build-sample sample_01_msg_exchange_via_shared_memory)

