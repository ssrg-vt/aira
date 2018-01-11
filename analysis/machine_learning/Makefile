NAME := aira-ml
TRAIN := train
UNITTEST := unittest
GENCSV := tests/gendata.py
BUILD := build

LIBRARY_SRC := $(shell ls lib/*.cc)
GENERAL_SRC := $(shell ls *.cc | grep -v "train.cc")
TEST_SRC := $(shell ls tests/*.cc)
TEST_CSV := tests/unittest_3x3_linear.csv tests/unittest_100x25_linear.csv \
            tests/unittest_100x24_123.csv tests/unittest_100x23_123.csv \
            tests/unittest_3x3_123.csv
MAIN_SRC := train.cc

LIB := ${BUILD}/lib${NAME}.so
LIBRARY_OBJ := $(addprefix ${BUILD}/, ${LIBRARY_SRC:.cc=.po})
GENERAL_OBJ := $(addprefix ${BUILD}/, ${GENERAL_SRC:.cc=.o})
TEST_OBJ := $(addprefix ${BUILD}/, ${TEST_SRC:.cc=.o})
MAIN_OBJ := $(addprefix ${BUILD}/, ${MAIN_SRC:.cc=.o})
HEAD := $(shell ls *.hh */*.hh)

CXX := g++
CXXFLAGS := -std=c++11 -Wall -Iinclude `pkg-config opencv --cflags` -pipe \
						-march=native -O2 -g
LDFLAGS := `pkg-config opencv --libs` -lmpfr -L${BUILD}/ -l${NAME} -g

all: ${LIB} ${TRAIN} ${TEST}

${BUILD}/.keep:
	@echo "  MKDIR ${BUILD}"
	@mkdir -p ${BUILD} # Use -p to not warn if directory already exists.
	@touch ${BUILD}/.keep

${BUILD}/lib/.keep: ${BUILD}/.keep
	@echo "  MKDIR ${BUILD}/lib"
	@mkdir -p ${BUILD}/lib
	@touch ${BUILD}/lib/.keep

${BUILD}/tests/.keep: ${BUILD}/.keep
	@echo "  MKDIR ${BUILD}/tests"
	@mkdir -p ${BUILD}/tests
	@touch ${BUILD}/tests/.keep

${BUILD}/%.po: %.cc ${HEAD} ${BUILD}/lib/.keep
	@echo "  CXX $@ (-fpic)"
	@${CXX} ${CXXFLAGS} -fpic -c $< -o $@

${BUILD}/%.o: %.cc ${HEAD} ${BUILD}/.keep ${BUILD}/tests/.keep
	@echo "  CXX $@"
	@${CXX} ${CXXFLAGS} -c $< -o $@

%.csv: ${GENCSV}
	@echo "  CSV $@"
	@${GENCSV} $@

${LIB}: ${LIBRARY_OBJ}
	@echo "  LD.so $@"
	@${CXX} -shared $^ -o $@

${TRAIN}: ${LIB} ${GENERAL_OBJ} ${BUILD}/train.o
	@echo "  LD $@"
	@${CXX} -Wl,-rpath,./build ${GENERAL_OBJ} ${BUILD}/train.o ${LDFLAGS} -o $@

${UNITTEST}: ${LIB} ${TEST_OBJ}
	@echo "  LD $@"
	@${CXX} ${TEST_OBJ} ${LDFLAGS} -lgtest_main -o $@

runtest: ${UNITTEST} ${TEST_CSV}
	@echo "  RUNTEST"
	@LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${BUILD}/" ./${UNITTEST}

rundemo: ${TRAIN}
	@echo "  RUNDEMO"
	@LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${BUILD}/" ./${TRAIN} \
	    --data ../results/merge-static.csv

runsanitize: clean
	@echo "  CLANG SANITIZE"
	@$(MAKE) rundemo \
	    CXX=clang++ \
	    CXXEXTRAFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer \
		               -g -O1" \
	    LDEXTRAFLAGS="-fsanitize=address,undefined -g"

runanalysis_clang: clean
	@echo "  CLANG STATIC ANALYSIS"
	@scan-build $(MAKE)

runanalysis_cppcheck:
	@echo "  CPPCHECK STATIC ANALYSIS"
	@cppcheck --quiet --enable=all --std=c++11 \
	    ${MAIN_SRC} ${GENERAL_SRC} ${LIBRARY_SRC} ${TEST_SRC}

clean:
	@echo "  CLEAN"
	@rm -f ${TRAIN} ${TEST} ${UNITTEST} ${TEST_CSV}
	@rm -f ${LIBRARY_OBJ} ${GENERAL_OBJ} ${TEST_OBJ} ${MAIN_OBJ} ${LIB}
	@rm -rf ${BUILD}/tests/.keep ${BUILD}/lib/.keep ${BUILD}/.keep
	@rm -rf ${BUILD}/lib # Harmless error if BUILD is ".".
	@rm -rf ${BUILD}/

.PHONY: all clean runtest rundemo
.PHONY: runsanitize runanalysis_clang runanalysis_cppcheck
