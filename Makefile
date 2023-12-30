
# If you would like to build with target or environment specific configuration:
# 1. please prepare XXXX.cmake that includes build options
# 2. provide file information of XXXX.cmake like below to CMakeLists.txt
#    with -D option like below
#        $ cmake -D BUILD_TARGET=XXXX
#    or
#        $ make BUILDTARGET=XXXX
# 
#    common.cmake is default configurations
#    codecoverage.cmake is the configuration for code coverage of gcov
# 
BUILDTARGET?=common

# Debug or Release or ...
# BUILDTYPE=Debug
# BUILDTYPE=Release
# 
BUILDTYPE?=Release

# Select build library type
# PROCSHARED_BUILD_SHARED_LIBS=OFF -> static library
# PROCSHARED_BUILD_SHARED_LIBS=ON -> shared library
PROCSHARED_BUILD_SHARED_LIBS?=OFF

# Sanitizer test option:
# SANITIZER_TYPE= 1 ~ 20 or ""
#
# Please see common.cmake for detail
# 
SANITIZER_TYPE?=

##### internal variable
BUILDIMPLTARGET?=all
BUILD_DIR?=build

CPUS=$(shell grep cpu.cores /proc/cpuinfo | sort -u | sed 's/[^0-9]//g')
JOBS=$(shell expr ${CPUS} + ${CPUS} / 2)

all: build
	set -e; \
	cd ${BUILD_DIR}; \
	cmake -DCMAKE_BUILD_TYPE=${BUILDTYPE} -DBUILD_TARGET=${BUILDTARGET} -DSANITIZER_TYPE=${SANITIZER_TYPE} -DALCONCURRENT_BUILD_SHARED_LIBS=${ALCONCURRENT_BUILD_SHARED_LIBS} -G "Unix Makefiles" ../; \
	cmake --build . -j ${JOBS} -v --target ${BUILDIMPLTARGET}

test: build-test
	set -e; \
	cd ${BUILD_DIR}; \
	ctest -j ${JOBS} -v

# test2: test
# 	build/test/test_procshared_mem_primary_highload
# 	build/test/test_procshared_mem_both_highload

build-test:
	make BUILDIMPLTARGET=build-test all

build:
	mkdir -p ${BUILD_DIR}

clean:
	-rm -fr ${BUILD_DIR}

coverage: clean
	set -e; \
	make BUILDTARGET=codecoverage BUILDTYPE=Debug test;  \
	cd ${BUILD_DIR}; \
	find . -type f -name "*.gcda" | xargs -P${JOBS} -I@ gcov -l -b @; \
	lcov --rc lcov_branch_coverage=1 -c -d . -o tmp.info; \
	lcov --rc lcov_branch_coverage=1 -b -c -d . -r tmp.info  '/usr/include/*' -o tmp2.info; \
	lcov --rc lcov_branch_coverage=1 -b -c -d . -r tmp2.info  '*/test/*' -o output.info; \
	genhtml --branch-coverage -o OUTPUT -p . -f output.info

coverage2: clean
	set -e; \
	make BUILDTARGET=codecoverage BUILDTYPE=Debug test2;  \
	cd ${BUILD_DIR}; \
	find . -type f -name "*.gcda" | xargs -P${JOBS} -I@ gcov -l -b @; \
	lcov --rc lcov_branch_coverage=1 -c -d . -o tmp.info; \
	lcov --rc lcov_branch_coverage=1 -b -c -d . -r tmp.info  '/usr/include/*' -o tmp2.info; \
	lcov --rc lcov_branch_coverage=1 -b -c -d . -r tmp2.info  '*/test/*' -o output.info; \
	genhtml --branch-coverage -o OUTPUT -p . -f output.info

profile: clean
	set -e; \
	make BUILDTARGET=gprof BUILDTYPE=Release test;  \
	cd ${BUILD_DIR}; \
	find . -type f -executable -name "test_*" | xargs -P${JOBS} -I@ sh ../gprof_exec.sh @

sanitizer:
	set -e; \
	for i in `seq 1 13`; do \
		make sanitizer.$$i.sanitizer; \
		echo $$i / 13 done; \
	done

sanitizer.%.sanitizer: clean
	make BUILDTARGET=common BUILDTYPE=Debug SANITIZER_TYPE=$* test


.PHONY: test build sanitizer


