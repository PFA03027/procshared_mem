
# set(CMAKE_CXX_STANDARD 11)	# for test purpose
# set(CMAKE_CXX_STANDARD 14)	# for test purpose
# set(CMAKE_CXX_STANDARD 17)	# for test purpose
# set(CMAKE_CXX_STANDARD 20)	# for test purpose

if("${SANITIZER_TYPE}" EQUAL "1")
 message("[Sanitizer] Enable Address Sanitizer")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTEST_ENABLE_ADDRESSSANITIZER -fno-omit-frame-pointer -fsanitize=address")	# for test purpose
 set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -fsanitize=address")	# for test purpose
elseif("${SANITIZER_TYPE}" EQUAL "2")
 message("[Sanitizer] Enable Stack protector")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fstack-protector-strong")	# for test purpose
elseif("${SANITIZER_TYPE}" EQUAL "3")
 message("[Sanitizer] Enable Leak Sanitizer")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTEST_ENABLE_LEAKSANITIZER -fno-omit-frame-pointer -fsanitize=leak")	# for test purpose. Should NOT set ALCONCURRENT_CONF_USE_MALLOC_ALLWAYS_FOR_DEBUG_WITH_SANITIZER
elseif("${SANITIZER_TYPE}" EQUAL "4")
 message("[Sanitizer] Enable Thread Sanitizer")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTEST_ENABLE_THREADSANITIZER -O2 -fno-omit-frame-pointer -fsanitize=thread")	# for test purpose. thread sanitizer needs -O1/-O2. Unfortunately this finds false positive.
elseif("${SANITIZER_TYPE}" EQUAL "5")
 message("[Sanitizer] Enable UB Sanitizer")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -DTEST_ENABLE_UBSANITIZER -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all")	# for test purpose
else()
 # no sanitizer option
 message("[Sanitizer] No Sanitizer")
endif()

