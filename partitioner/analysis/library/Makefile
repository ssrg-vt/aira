LIB := ptrack_stubs.so
UNITTEST := unittest
BUILD := build

SRC := interface.cpp page_tracking.cpp function_graph.cpp graph.cpp
TEST_SRC := test/test_page_tracking.cpp test/test_function_graph.cpp \
            test/test_graph.cpp
HEAD := $(shell ls *.h)

OBJ := $(addprefix ${BUILD}/, ${SRC:.cpp=.o})
TEST_OBJ := $(addprefix ${BUILD}/, ${TEST_SRC:.cpp=.o})

CXX := clang++ # This library will be linked against clang produced binaries.
CXXFLAGS := -Wall -pipe -march=native -O2
LDFLAGS :=

.PHONY: all
all: ${LIB}

${BUILD}/.keep:
	@echo -e "\tMKDIR\t${BUILD}"
	@mkdir -p ${BUILD} # Use -p to not warn if directory already exists.
	@touch ${BUILD}/.keep

${BUILD}/test/.keep: ${BUILD}/.keep
	@echo -e "\tMKDIR\t${BUILD}/test"
	@mkdir -p ${BUILD}/test # Use -p to not warn if directory already exists.
	@touch ${BUILD}/test/.keep

${BUILD}/%.o: %.cpp ${HEAD} ${BUILD}/.keep
	@echo -e "\tCXX\t$@ (-fpic)"
	@${CXX} -std=c++11 ${CXXFLAGS} -fpic -c $< -o $@

${BUILD}/test/%.o: test/%.cpp ${HEAD} ${BUILD}/test/.keep
	@echo -e "\tCXX\t$@"
	@${CXX} -std=c++11 ${CXXFLAGS} -c $< -o $@

${LIB}: ${OBJ}
	@echo -e "\tLD.so\t$@"
	@${CXX} -shared $^ -o $@

${UNITTEST}: ${LIB} ${TEST_OBJ}
	@echo -e "\tLD\t$@"
	@${CXX} ${LIB} ${TEST_OBJ} ${LDFLAGS} -lgtest -lgtest_main -o $@

.PHONY: test
test: ${UNITTEST}
	@echo -e "\tRUNTEST"
	@LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:." ./${UNITTEST}

.PHONY: clean
clean:
	@echo -e "\tCLEAN"
	@rm -f ${OBJ} ${LIB} ${BUILD}/.keep
	@rm -f ${TEST_OBJ} ${UNITTEST} ${BUILD}/test/.keep
	@rm -f --dir ${BUILD}/test
	@rm -f --dir ${BUILD}/
