/*
 * Class that encapsulates all the features to be extracted
 * from source code.
 * Author: Rob Lyerly
 *
 * Defines counters and methods for parsing GIMPLE statements and
 * maintaining a feature count for various features, including:
 * 		-integer arithmetic
 * 		-floating-point arithmetic
 */

#include <cstdio>
#include <string>
#include <map>

#include "gcc-plugin.h"
#include "coretypes.h"
#include "gimple.h"
//#include "tree-flow-inline.h"
#include "tree-flow.h"
#include "tree-pretty-print.h"
#include "dumpfile.h"
#include "gimple-pretty-print.h"
#include "langhooks.h"

/* Various shorthands for obtaining tree codes/names */
#define VAL2CODE( val ) TREE_CODE( val )
#define CODE2NAME( val ) tree_code_name[ val ]
#define VAL2NAME( val ) tree_code_name[TREE_CODE( val )]
#define VALTYPE( val ) VAL2CODE(TREE_TYPE( val ))

#define GIMPLENAME( val ) gimple_code_name[gimple_code( val )]

/* Various definitions */
#define SUCCESS 0
#define FAILURE 1
#define INCORRECT_VERSION 2
#define FILE_OPEN_ERROR 3
#define FILE_WRITE_ERROR 4

/* Edge types */
#define PREDECESSOR 0
#define SUCCESSOR 1

class FeatureCount {

private:
	/* Store and process data related to a single feature.
	 */
	class Counter {
	private:
		long long fixed;   // Number of times feature happens in a function.
		long long dynamic; // fixed * bb_count (number of times BB is executed).
		const FeatureCount *parent;

	public:
		Counter() :
				fixed(0), dynamic(0), parent(NULL) {
		}
		Counter(const FeatureCount *fc) :
				fixed(0), dynamic(0), parent(fc) {
		}

		void increment(long bb_count) {
			fixed += 1;
			dynamic += bb_count;
		}
		int print(FILE *stream, const char *msg);

	private:
		double per_func() {
			return ((double) dynamic) / ((double) parent->func_count);
		}
	};

	/************************** Features **************************/

	/* Information & counters about the current function */
	std::string filename;
	std::string func_name;
	long func_count;
	long num_bb;
	long num_edges;
	Counter num_instructions;

	/* Branch Counters */
	Counter cond_branches;
	Counter uncond_branches;

	/* Integer math operations */
	Counter int_ops;
	Counter int_vec_ops;

	/* Floating-point math operations */
	Counter float_ops;
	Counter float_vec_ops;

	/* Logic and Bitwise operations */
	Counter logic_ops;
	Counter vec_logic_ops;
	Counter comparison_ops;

	/* Load and Store operations */
	Counter load_ops;
	Counter store_ops;

	/* OpenMP for, task, or sections */
	Counter work_items;

	/* Function Calls */
	Counter func_calls;

	/* Intrinsic Math Operations */
	Counter intrinsic_math_ops;

	/********************** Non-counter variables ****************************/

	/* Debugging Switches */
	bool debug;
	bool memDebug;

	/* Flags For Counting Work Items */
	bool omp_parallel_start;

	/* Private Methods */
	int extract_assign(gimple stmt, long bb_count);
	int extract_cond(gimple stmt, long bb_count);
	int print(FILE* fp);
	int log_gimple(std::string name, int numOperands);
	int log_expression(std::string name, std::string type, int numOperands);
	int print_debug(std::string type, std::string name);
	int print_debug_message(std::string message);
	std::string numToStr(int number);
	void extract_load_store(gimple stmt, long bb_count);
	bool is_memory_access(tree_code code);
	const char* get_call_function_name(gimple stmt);
	bool is_intrinsic_math_call(const char *func_name);

public:
	/* Constructor for the FeatureCount object.  Initializes all
	 * counters to 0 and stores the method/file name.
	 *
	 * filename - string containing the filename which is being processed
	 * func_name - string containing the function name which is being processed
	 * num_bb - number of basic blocks in the function
	 * num_edges - number of edges in the control flow graph of the function
	 */
	FeatureCount(const char* filename, const char* func_name, long num_bb,
			long num_edges, bool useDebugging, bool useMemDebugging);

	/* Extract the feature from the given GIMPLE statement and add it
	 * to the feature count maintained by this object.
	 *
	 * bb_num - the number of the basic block currently being processed
	 * stmt - the GIMPLE statement being processed
	 */
	int count_gimple_feature(long bb_num, long bb_count, gimple stmt);

	/* Write the feature count out to standard output */
	void write_to_stdout();

	/* Write the feature count out to the file specified in the argument.
	 *
	 * filename - the name of the file to be opened and written to.  this
	 * 		function DOES clobber the file if it already exists
	 */
	int write_to_file(std::string filename);

	/* Getters */
	const std::string get_filename();
	const std::string get_funcname();
	long get_num_bb();
	long get_num_edges();

	/* Setters */
	void set_func_count(long fc) {
		func_count = fc;
	}
};
