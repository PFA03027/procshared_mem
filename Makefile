
# If you would like to build with target or environment specific configuration:
# 1. please prepare XXXX.cmake that includes build options
# 2. provide file information of XXXX.cmake like below to CMakeLists.txt
#    with -D option like below
#        $ cmake -D BUILD_TARGET=XXXX
#    or
#        $ make BUILD_CONFIG=XXXX
# 
#    common.cmake is default configurations
#    codecoverage.cmake is the configuration for code coverage of gcov
# 
BUILD_CONFIG?=normal

# Debug or Release or ...
# BUILD_TYPE=Debug
# BUILD_TYPE=Release
# 
BUILD_TYPE?=Release

# Select build library type
# IPSM_BUILD_SHARED_LIBS=OFF -> static library
# IPSM_BUILD_SHARED_LIBS=ON -> shared library
IPSM_BUILD_SHARED_LIBS?=ON

# Sanitizer test option:
# SANITIZER_TYPE= 1 ~ 20 or ""
#
# Please see common.cmake for detail
# 
SANITIZER_TYPE?=


# Option tu use Ninja
# NINJA_SELECTION=AUTO: check path of ninja and then if found it, select ninja as build tool for CMake
# NINJA_SELECTION=NINJA: select ninja as build tool for CMake
# NINJA_SELECTION=MAKE: select make as build tool for CMake
# NINJA_SELECTION=*: other than AUTO or NINJA, select Unix make as build tool for CMake
BUILD_TOOL_SELECTION ?= AUTO

#############################################################################################
#############################################################################################
#############################################################################################
##### internal variable
BUILD_DIR?=build
#MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))	# 相対パス名を得るならこちら。
#MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))	# 絶対パス名を得るならこちら。

SANITIZER_ALL_IDS=$(shell seq 1 5)

#TEST_EXECS = $(shell find ${BUILD_DIR} -type f -executable -name "test_*")
TEST_EXECS = $(shell find . -type f -executable -name "test_*")

CMAKE_CONFIGURE_OPTS  = -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  # for clang-tidy
CMAKE_CONFIGURE_OPTS += -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
CMAKE_CONFIGURE_OPTS += -DBUILD_CONFIG=${BUILD_CONFIG}
CMAKE_CONFIGURE_OPTS += -DIPSM_BUILD_SHARED_LIBS=${IPSM_BUILD_SHARED_LIBS}

CPUS=$(shell grep cpu.cores /proc/cpuinfo | sort -u | sed 's/[^0-9]//g')
JOBS?=$(shell expr ${CPUS} + ${CPUS} / 2)

ifeq ($(BUILD_TOOL_SELECTION),AUTO)
NINJA_PATH := $(shell whereis -b ninja | sed -e 's/ninja:\s*//g')
ifeq ($(NINJA_PATH),)
CMAKE_GENERATE_TARGET = Unix Makefiles
else	# NINJA_PATH
CMAKE_GENERATE_TARGET = Ninja
endif	# NINJA_PATH
else	# BUILD_TOOL_SELECTION
ifeq ($(BUILD_TOOL_SELECTION),NINJA)
CMAKE_GENERATE_TARGET = Ninja
else	# BUILD_TOOL_SELECTION
CMAKE_GENERATE_TARGET = Unix Makefiles
endif	# BUILD_TOOL_SELECTION
endif	# BUILD_TOOL_SELECTION


#############################################################################################
all: configure-cmake-no-sanitizer
	cmake --build ${BUILD_DIR} -j ${JOBS} -v --target all

clean:
	-cmake --build ${BUILD_DIR} -j ${JOBS} -v --target clean

clean-all:
	-rm -fr ${BUILD_DIR}
#	-rm -fr ${BUILD_DIR} build.*

# This is inatall command example
install: all
	DESTDIR=/tmp/install-test cmake --install ${BUILD_DIR} --prefix /opt/xxx

#############################################################################################
test: build-test
	setarch $(uname -m) -R ctest --test-dir ${BUILD_DIR} -j ${JOBS} -v

test-no-sanitizer: build-test-no-sanitizer
	setarch $(uname -m) -R ctest --test-dir ${BUILD_DIR} -j ${JOBS} -v

build-test: configure-cmake.1.sanitizer
	cmake --build ${BUILD_DIR} -j ${JOBS} -v --target build-test

build-test-no-sanitizer: configure-cmake-no-sanitizer
	cmake --build ${BUILD_DIR} -j ${JOBS} -v --target build-test

#############################################################################################
sample: build-sample
	-./build/sample/sample_01_msg_exchange_via_shared_memory
	-./build/sample/sample_02_list_of_unique_ptr
	-./build/sample/sample_03_offset_shared_ptr

build-sample: configure-cmake-no-sanitizer
	cmake --build ${BUILD_DIR} -j ${JOBS} -v --target build-sample

#############################################################################################
configure-cmake.%.sanitizer:
	cmake -S . -B ${BUILD_DIR} -G "${CMAKE_GENERATE_TARGET}" ${CMAKE_CONFIGURE_OPTS} -DSANITIZER_TYPE=$*

configure-cmake-no-sanitizer:
	cmake -S . -B ${BUILD_DIR} -G "${CMAKE_GENERATE_TARGET}" ${CMAKE_CONFIGURE_OPTS} -DSANITIZER_TYPE=


#############################################################################################
profile: build-profile
	$(MAKE) -j ${JOBS} make-profile-out

exec-profile: build-profile
	setarch $(uname -m) -R ctest --test-dir ${BUILD_DIR} -j ${JOBS} -v

build-profile:
	$(MAKE) BUILD_CONFIG=gprof BUILDTYPE=${BUILDTYPE} build-test-no-sanitizer

ifeq ($(strip $(TEST_EXECS)),)
## $(TEST_EXECS)が空の場合＝buildが実行されていない状況
profile.%: build-profile
	$(MAKE) profile.$*

else
## $(TEST_EXECS)になんらかの値が入っている場合=buildが実行されている状況
# 第1引数: テスト実行ファイルのファイル名
# 第2引数: テスト実行ファイルのあるディレクトリ名
# 第3引数: テスト実行ファイルのフルパス名
define TEST_EXEC_TEMPLATE
profile.$(1): build-profile
	-rm -f $(2)gmon.out
	set -e; (cd $(2); ./$(1) $(TEST_OPTS))
	gprof $(3) $(2)gmon.out > $(2)prof.out.txt
	gprof -A $(3) $(2)gmon.out > $(2)prof_func.out.txt

endef

$(foreach pgm,$(TEST_EXECS),$(eval $(call TEST_EXEC_TEMPLATE,$(notdir $(pgm)),$(dir $(pgm)),$(pgm))))

endif

make-profile-out: $(addprefix profile.,$(notdir $(TEST_EXECS)))

#############################################################################################
coverage: exec-coverage
	cd ${BUILD_DIR}; \
	find . -type f -name "*.gcda" | xargs -P${JOBS} -I@ gcov -l -b @; \
	lcov --rc branch_coverage=1 --rc geninfo_unexecuted_blocks=1 --ignore-errors negative --ignore-errors mismatch -c -d . --include '*/libipsm_mem/*' -o output.info; \
	genhtml --branch-coverage -o OUTPUT -p . -f output.info

exec-coverage: clean
	$(MAKE) BUILD_CONFIG=codecoverage BUILDTYPE=Debug test-no-sanitizer

#############################################################################################
sanitizer:
	set -e; \
	for i in $(SANITIZER_ALL_IDS); do \
		$(MAKE) sanitizer.$$i.sanitizer; \
		echo $$i / 5 done; \
	done

sanitizer.%.sanitizer: configure-cmake.%.sanitizer
	cmake --build ${BUILD_DIR} -j ${JOBS} -v --target build-test
	setarch $(uname -m) -R ctest --test-dir ${BUILD_DIR} -j ${JOBS} -v

#############################################################################################
tidy-fix: configure-cmake-no-sanitizer
	find ./ -name '*.cpp' -or -name '*.hpp'|grep ./procshared_mem|xargs -t -P${JOBS} -n1 clang-tidy -p=${BUILD_DIR} --fix
	find ./ -name '*.cpp' -or -name '*.hpp'|grep ./procshared_mem|xargs -t -P${JOBS} -n1 clang-format -i

tidy: configure-cmake-no-sanitizer
	find ./ -name '*.cpp' -or -name '*.hpp'|grep ./procshared_mem|xargs -t -P${JOBS} -n1 clang-tidy -p=${BUILD_DIR}



# tidy-fix: configure-cmake
# 	find ./ -name '*.cpp'|grep -v googletest|grep -v ./build/|xargs -t -P${JOBS} -n1 clang-tidy -p=build --fix
# 	find ./ -name '*.cpp'|grep -v googletest|grep -v ./build/|xargs -t -P${JOBS} -n1 clang-format -i
# 	find ./ -name '*.hpp'|grep -v googletest|grep -v ./build/|xargs -t -P${JOBS} -n1 clang-format -i

# tidy: configure-cmake
# 	find ./ -name '*.cpp'|xargs -t -P${JOBS} -n1 clang-tidy -p=build

.PHONY: test build sanitizer


load-test: build/test/loadtest_ipsm_malloc_highload build/test/loadtest_ipsm_mem_both_highload build/test/loadtest_ipsm_mem_primary_highload
	build/test/loadtest_ipsm_mem_primary_highload
	build/test/loadtest_ipsm_mem_both_highload
	build/test/loadtest_ipsm_malloc_highload

build/test/loadtest_ipsm_malloc_highload: build-test
build/test/loadtest_ipsm_mem_both_highload: build-test
build/test/loadtest_ipsm_mem_primary_highload: build-test


