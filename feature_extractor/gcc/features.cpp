/*
 * Class that encapsulates all the features to be extracted
 * from source code.
 * Author: Rob Lyerly
 *
 * Implements counters and methods for parsing GIMPLE statements and
 * maintaining a feature count for various features, including:
 * 		-integer arithmetic
 * 		-floating-point arithmetic
 */

#include <sstream>

#include "features.h"

using namespace std;

/****************************** Checking macros ******************************/
#define CHECK_IO_ERR( val ) if(val < 0) return FILE_WRITE_ERROR

/******************************* Object methods ******************************/
/* Default constructor.  Creates a FeatureCount object for collecting
 * statistics from the given file and function (named by filename and
 * func_name, respectively).
 */
FeatureCount::FeatureCount(const char* filename, const char* func_name,
		long num_bb, long num_edges, bool useDebugging, bool useMemDebugging) {

	//Initialize general information
	this->filename = filename;
	this->func_name = func_name;
	this->num_bb = num_bb;
	this->num_edges = num_edges;

	//Initialize counters
	num_instructions = Counter(this);
	cond_branches = Counter(this);
	uncond_branches = Counter(this);
	int_ops = Counter(this);
	int_vec_ops = Counter(this);
	float_ops = Counter(this);
	float_vec_ops = Counter(this);
	logic_ops = Counter(this);
	vec_logic_ops = Counter(this);
	comparison_ops = Counter(this);
	work_items = Counter(this);
	load_ops = Counter(this);
	store_ops = Counter(this);
	func_calls = Counter(this);
	intrinsic_math_ops = Counter(this);

	//Turn debugging on/off
	debug = useDebugging;
	memDebug = useMemDebugging;

	//We don't ever start in a parallel section
	omp_parallel_start = false;

	//Start the debugging trace
	if (debug) {
		string startDebug = "*** Function ";
		startDebug += func_name;
		startDebug += " in file ";
		startDebug += filename;
		startDebug += " ***";
		print_debug_message(startDebug);
	}
}

// START ALY -- TEMPORARY CODE
bool FeatureCount::is_memory_access(tree_code code) {
	switch (code) {
	case ARRAY_REF:
	case ARRAY_RANGE_REF: // Only occurs in FORTRAN ???
	case INDIRECT_REF: // In my test code for INDIRECT_REF I actually see a MEM_REF ...
	case MEM_REF:
	case TARGET_MEM_REF: // Not sure whether this occurs for us ...
		return true;
	default:
		return false;
	}
}

void FeatureCount::extract_load_store(gimple stmt, long bb_count) {
	if (memDebug) {
		fprintf(stderr, "<<< GIMPLE_ASSIGN (looking for memory info): ");
		print_gimple_stmt(stderr, stmt, 0, 0);
	}

	tree_code lhs_code = TREE_CODE(gimple_assign_lhs(stmt));
	tree_code rhs_code = gimple_assign_rhs_code(stmt);

	if (memDebug) {
		fprintf(stderr, "    LHS tree code: %d (%s)\n", lhs_code,
				tree_code_name[lhs_code]);
		fprintf(stderr, "    RHS tree code: %d (%s)\n", rhs_code,
				tree_code_name[rhs_code]);
	}

	// data[index] = ...
	// *data = ...
	if (is_memory_access(lhs_code)) {
		store_ops.increment(bb_count);
		if (memDebug)
			fprintf(stderr, "    !!! THIS IS A MEMORY STORE !!!\n");
	}

	// ... = data[index];
	// ... = *data;
	if (is_memory_access(rhs_code)) {
		load_ops.increment(bb_count);
		if (memDebug)
			fprintf(stderr, "    !!! THIS IS A MEMORY LOAD !!!\n");
	}

	if (memDebug)
		fprintf(stderr, ">>> GIMPLE_ASSIGN\n");
}
// END ALY

/* Extract features from the passed GIMPLE statement. */
int FeatureCount::count_gimple_feature(long bb_num, long bb_count,
		gimple stmt) {

	num_instructions.increment(bb_count);
	unsigned int gcode = gimple_code(stmt);

	if (debug) {
		log_gimple(GIMPLENAME( stmt ), gimple_num_ops(stmt));
		print_gimple_stmt(stderr, stmt, 0, 0);
	}

		//Perform different processing depending on the type of GIMPLE statement
	switch (gcode) {
	case GIMPLE_ASSIGN:
		return extract_assign(stmt, bb_count);
	case GIMPLE_CALL:
		func_calls.increment(bb_count);
		if (!omp_parallel_start) { // If not a parallel section, check for the start of one
			omp_parallel_start = !strcmp("__builtin_GOMP_parallel_start",
					get_call_function_name(stmt));
		} else { // If a Parallel Section, check for the end, and increment for each function inside
			omp_parallel_start = (strcmp("__builtin_GOMP_parallel_end",
					get_call_function_name(stmt)) != 0);
			if (omp_parallel_start) {
				if (debug) {
					fprintf(stderr, "WORK ITEMS INCREMENTED!!! --> ");
					print_gimple_stmt(stderr, stmt, 0, 0);
				}
				work_items.increment(bb_count);
			}
		}
		if(is_intrinsic_math_call(get_call_function_name(stmt))){
			intrinsic_math_ops.increment(bb_count);
		}
		return SUCCESS;
	case GIMPLE_COND:
		cond_branches.increment(bb_count);
		return extract_cond(stmt, bb_count);
	case GIMPLE_GOTO:
		uncond_branches.increment(bb_count);
		break;
	case GIMPLE_PHI:
		break;
	case GIMPLE_SWITCH:
		break;
	case GIMPLE_OMP_ATOMIC_LOAD:
		break;
	case GIMPLE_OMP_ATOMIC_STORE:
		break;
	case GIMPLE_OMP_FOR:
	case GIMPLE_OMP_TASK:
	case GIMPLE_OMP_SECTION:
		break;
	case GIMPLE_RETURN:
		break;
	default:
		if (debug) {
			string message = "Unhandled GIMPLE type: ";
			message += GIMPLENAME(stmt);
			print_debug_message(message);
		}
		break;
	}
	return SUCCESS;
}

/* Extract features from the GIMPLE assign statement */
int FeatureCount::extract_assign(gimple stmt, long bb_count) {

	int assign_code = gimple_assign_rhs_code(stmt);
	int rhsclass = gimple_assign_rhs_class(stmt);
	tree vector = NULL;

	//Check for loads and stores in the statement
	extract_load_store(stmt, bb_count);

	//Check the type of assign statement
	if (rhsclass == GIMPLE_UNARY_RHS) {

		//Check for vector operations
		if (VALTYPE( gimple_assign_rhs1(stmt) ) == VECTOR_TYPE)
			vector = gimple_assign_rhs1(stmt);

		//Check which assign operation is occurring
		switch (assign_code) {
		case NEGATE_EXPR:
		case BIT_NOT_EXPR:
		case TRUTH_NOT_EXPR:
			if (vector != NULL) {
				vec_logic_ops.increment(bb_count);
			} else {
				logic_ops.increment(bb_count);
			}
			break;
		default:
			break;
		}
	} else if (rhsclass == GIMPLE_BINARY_RHS) {

		//Check for vector operations
		if (VALTYPE( gimple_assign_rhs1(stmt) ) == VECTOR_TYPE)
			vector = gimple_assign_rhs1(stmt);
		else if (VALTYPE( gimple_assign_rhs2(stmt) ) == VECTOR_TYPE)
			vector = gimple_assign_rhs2(stmt);

		//Check which specific type of assign operation is occurring
		int assign_type = VALTYPE( gimple_assign_rhs1(stmt) );
		switch (assign_code) {
		case PLUS_EXPR:
		case POINTER_PLUS_EXPR:
			if (vector != NULL) {
				if (VALTYPE(TREE_TYPE(vector)) == REAL_TYPE) {
					float_vec_ops.increment(bb_count);
				} else {
					int_vec_ops.increment(bb_count);
				}
			} else {
				if (assign_type == REAL_TYPE) {
					float_ops.increment(bb_count);
				} else {
					int_ops.increment(bb_count);
				}
			}
			break;
		case MINUS_EXPR:
			if (vector != NULL) {
				if (VALTYPE(TREE_TYPE(vector)) == INTEGER_TYPE) {
					int_vec_ops.increment(bb_count);
				} else {
					float_vec_ops.increment(bb_count);
				}
			} else {
				if (assign_type == INTEGER_TYPE) {
					int_ops.increment(bb_count);
				} else {
					float_ops.increment(bb_count);
				}
			}
			break;
		case MULT_EXPR:
		case MULT_HIGHPART_EXPR:
			if (vector != NULL) {
				if (VALTYPE(TREE_TYPE(vector)) == INTEGER_TYPE) {
					int_vec_ops.increment(bb_count);
				} else {
					float_vec_ops.increment(bb_count);
				}
			} else {
				if (assign_type == INTEGER_TYPE) {
					int_ops.increment(bb_count);
				} else {
					float_ops.increment(bb_count);
				}
			}
			break;
		case TRUNC_DIV_EXPR:
		case CEIL_DIV_EXPR:
		case FLOOR_DIV_EXPR:
		case ROUND_DIV_EXPR:
		case EXACT_DIV_EXPR:
			if (vector != NULL) {
				int_vec_ops.increment(bb_count);
			} else {
				int_ops.increment(bb_count);
			}
			break;
		case TRUNC_MOD_EXPR:
		case CEIL_MOD_EXPR:
		case FLOOR_MOD_EXPR:
		case ROUND_MOD_EXPR:
			if (vector != NULL) {
				int_vec_ops.increment(bb_count);
			} else {
				int_ops.increment(bb_count);
			}
			break;
		case RDIV_EXPR:
			if (vector != NULL) {
				float_vec_ops.increment(bb_count);
			} else {
				float_ops.increment(bb_count);
			}
			break;
		case LSHIFT_EXPR:
		case RSHIFT_EXPR:
		case LROTATE_EXPR:
		case RROTATE_EXPR:
		case BIT_IOR_EXPR:
		case BIT_XOR_EXPR:
		case BIT_AND_EXPR:
		case TRUTH_ANDIF_EXPR:
		case TRUTH_AND_EXPR:
		case TRUTH_ORIF_EXPR:
		case TRUTH_OR_EXPR:
		case TRUTH_XOR_EXPR:
			if (vector != NULL) {
				vec_logic_ops.increment(bb_count);
			} else {
				logic_ops.increment(bb_count);
			}
			break;
		default:
			break;
		}
	} else {
		return FAILURE;
	}

	return SUCCESS;
}

/* Extract features from GIMPLE conditional statements. */
int FeatureCount::extract_cond(gimple stmt, long bb_count) {

	//Determine the type of conditional found
	unsigned int cond_code = gimple_cond_code(stmt);
	switch (cond_code) {
	case LT_EXPR:
		comparison_ops.increment(bb_count);
		break;
	case LE_EXPR:
		comparison_ops.increment(bb_count);
		break;
	case GT_EXPR:
		comparison_ops.increment(bb_count);
		break;
	case GE_EXPR:
		comparison_ops.increment(bb_count);
		break;
	case EQ_EXPR:
		comparison_ops.increment(bb_count);
		break;
	case NE_EXPR:
		comparison_ops.increment(bb_count);
		break;
	default:
		if (debug) {
			string message = "Unhandled GIMPLE conditional code: ";
			message += CODE2NAME(cond_code);
			print_debug_message(message);
		}
		break;
	}
	return SUCCESS;
}

/* Print statistics to standard output */
void FeatureCount::write_to_stdout() {
	print(stdout);
}

/* Save the feature count to file named by the argument */
int FeatureCount::write_to_file(string filename) {

	//Open the file for writing.  Overwrite any existing file.
	FILE* fp = fopen(filename.c_str(), "w");
	if (!fp)
		return FILE_OPEN_ERROR;
	this->print(fp);
	fclose(fp);

	return SUCCESS;
}

/* Internal method to print statistics to the passed file pointer.  This
 * can be an actual file, or something else (like stdout).
 */
int FeatureCount::print(FILE* fp) {

	//Write the counters to the specified file pointer
	//Milepost-GCC features
	CHECK_IO_ERR(
			fprintf(fp,
					"<--------- Statistics for function \"%s\" " "in file \"%s\" "
							"executed %ld times --------->\n",
					func_name.c_str(), filename.c_str(), func_count));
	CHECK_IO_ERR(
			fprintf(fp,
					"<--------- Numbers are static_per_func, dynamic_per_one_call, "
							"dynamic_per_program --------->\n"));

	//General function characteristics
	CHECK_IO_ERR(fprintf(fp, "1. General program characteristics:\n"));
	CHECK_IO_ERR(num_instructions.print(fp, "a. Number of instructions"));
	CHECK_IO_ERR(
			int_ops.print(fp, "b. Number of scalar integer math operations"));
	CHECK_IO_ERR(
			int_vec_ops.print(fp,
					"c. Number of vector integer math operations"));
	CHECK_IO_ERR(
			float_ops.print(fp,
					"d. Number of scalar floating-point math operations"));
	CHECK_IO_ERR(
			float_vec_ops.print(fp,
					"e. Number of vector floating-point math operations"));
	CHECK_IO_ERR(
			logic_ops.print(fp, "f. Number of bitwise and boolean operations"));
	CHECK_IO_ERR(
			vec_logic_ops.print(fp,
					"g. Number of vector bitwise and boolean operations"));
	CHECK_IO_ERR(load_ops.print(fp, "h. Amount of load operations"));
	CHECK_IO_ERR(store_ops.print(fp, "i. Amount of store operations"));
	CHECK_IO_ERR(func_calls.print(fp, "j. Number of function calls"));
	CHECK_IO_ERR(intrinsic_math_ops.print(fp, "k. Number of intrinsic math operations"));
	//Comparison operations maybe? CHECK_IO_ERR( comparison_ops.print(fp, "Number of comparison operations") );
	CHECK_IO_ERR(fprintf(fp, "\n"));

	//Memory access patterns
	CHECK_IO_ERR(fprintf(fp, "2. Memory access patterns:\n"));
	//a. Percent of memory accesses that are coalesced
	//b. Amount of private memory needed
	//c. Amount of shared memory needed
	//d. Ratio of memory accesses/computation
	CHECK_IO_ERR(fprintf(fp, "\n"));

	//Control Flow
	CHECK_IO_ERR(fprintf(fp, "3. Control Flow:\n"));
	//a. Control flow paths per work item (Cyclomatic Complexity used as stand-in for time being)
	CHECK_IO_ERR(
			fprintf(fp, "a. Cyclomatic Complexity: %ld\n",
					num_edges - num_bb + 2));
	CHECK_IO_ERR(cond_branches.print(fp, "b. Number of Conditional Branches"));
	CHECK_IO_ERR(
			uncond_branches.print(fp, "c. Number of Unconditional Branches"));
	CHECK_IO_ERR(fprintf(fp, "\n"));

	//Available parallelism
	CHECK_IO_ERR(fprintf(fp, "4. Available parallelism:\n"));
	//a. Number of work items available
	CHECK_IO_ERR(
			work_items.print(fp,
					"a. Number of parallelizable functions (OpenMP for, task, or section)"));
	//b. Number of synchronization operations per work item
	CHECK_IO_ERR(fprintf(fp, "\n"));

	//Runtime
	//CHECK_IO_ERR(fprintf(fp, "5. Runtime:\n"));
	//a. Amount of memory to transfer per work item
	//CHECK_IO_ERR(fprintf(fp, "\n"));

	return SUCCESS;
}

/*
 * Writes debugging information about gimple statements to standard error.
 */
int FeatureCount::log_gimple(string name, int numOperands) {
	string message = name;
	message += " (";
	message += numToStr(numOperands);
	message += ")";
	return print_debug("\nGimple statement", message);
}

/*
 * Writes debugging information about expressions to standard error.
 */
int FeatureCount::log_expression(string name, string type, int numOperands) {
	string name_and_type = name + " (" + type + ",";
	name_and_type += numToStr(numOperands);
	name_and_type += ")";
	return print_debug("Expression", name_and_type);
}

/*
 * Prints the type and name debugging information to standard error.
 */
int FeatureCount::print_debug(string type, string name) {
	return print_debug_message(type + ": " + name);
}

/*
 * Prints a message to standard error.
 */
int FeatureCount::print_debug_message(string message) {
	return fprintf(stderr, "%s\n", message.c_str());
}

/******************************** Getters ************************************/
const string FeatureCount::get_filename() {
	return this->filename;
}

const string FeatureCount::get_funcname() {
	return this->func_name;
}

long FeatureCount::get_num_bb() {
	return this->num_bb;
}

long FeatureCount::get_num_edges() {
	return this->num_edges;
}

string FeatureCount::numToStr(int number) {
	ostringstream ss;
	ss << number;
	return ss.str();
}

int FeatureCount::Counter::print(FILE *stream, const char *msg) {
	return fprintf(stream, "%s: %lld, %f, %lld\n", msg, fixed, per_func(),
			dynamic);
}

const char *FeatureCount::get_call_function_name(gimple stmt) {
#define TEST_AND_RETURN( name, val )  if( val == NULL ) fprintf( stderr, "%s --> %s", name, "NULL" ); return val;

	tree op0 = gimple_call_fn(stmt);

	if (gimple_call_internal_p(stmt)) {
//		TEST_AND_RETURN("internal_fn_name(gimple_call_internal_fn(stmt))",
//				internal_fn_name(gimple_call_internal_fn(stmt)))
	} else {
		if (TREE_CODE (op0) == NON_LVALUE_EXPR)
			op0 = TREE_OPERAND (op0, 0);

			again:
			switch (TREE_CODE (op0))
			{
				case VAR_DECL:
				case PARM_DECL:
				case FUNCTION_DECL:
				if (DECL_NAME (op0)) {
					TEST_AND_RETURN("lang_hooks.decl_printable_name (op0, 1)",lang_hooks.decl_printable_name (op0, 1))
				}
				else{
//					if (DECL_NAME (op0)){
//						TEST_AND_RETURN("DECL_NAME (op0)", DECL_NAME (op0))
//					}
				}
				break;

				case ADDR_EXPR:
				case INDIRECT_REF:
				case NOP_EXPR:
				op0 = TREE_OPERAND (op0, 0);
				goto again;

				case COND_EXPR:
			      break;

			    case ARRAY_REF:
			      if (TREE_CODE (TREE_OPERAND (op0, 0)) == VAR_DECL){
			    	  if (DECL_NAME (TREE_OPERAND (op0, 0))){
			    	  					TEST_AND_RETURN("lang_hooks.decl_printable_name (TREE_OPERAND (op0, 0), 1)", lang_hooks.decl_printable_name (TREE_OPERAND (op0, 0), 1));
//			    	  					if (DECL_NAME (TREE_OPERAND (op0, 0))){
//			    	  						TEST_AND_RETURN("DECL_NAME (TREE_OPERAND (op0, 0))", DECL_NAME (TREE_OPERAND (op0, 0)))
//			    	  					}
			    	  }
			      }
			      else{
				TEST_AND_RETURN("dump_generic_node (buffer, op0, 0, flags, false)", "Not implemented...")
			      }
			      break;

			    case MEM_REF:
			      if (integer_zerop (TREE_OPERAND (op0, 1)))
				{
				  op0 = TREE_OPERAND (op0, 0);
				  goto again;
				}
			      /* Fallthru.  */
			    case COMPONENT_REF:
			    case SSA_NAME:
			    case OBJ_TYPE_REF:
			    	TEST_AND_RETURN("dump_generic_node (buffer, op0, 0, flags, false)", "Not implemented...")
			      break;

			    default:
			      TEST_AND_RETURN("NIY", "Not implemented...")
			    }
		}

	return NULL;
#undef TEST_AND_RETURN
}

bool FeatureCount::is_intrinsic_math_call(const char *func_name){
	return !(strcmp(func_name, "__builtin_absf")
			&& strcmp(func_name, "__builtin_acosf")
			&& strcmp(func_name, "__builtin_asinf")
			&& strcmp(func_name, "__builtin_atanf")
			&& strcmp(func_name, "__builtin_atan2f")
			&& strcmp(func_name, "__builtin_ceilf")
			&& strcmp(func_name, "__builtin_cosf")
			&& strcmp(func_name, "__builtin_coshf")
			&& strcmp(func_name, "__builtin_expf")
			&& strcmp(func_name, "__builtin_fabsf")
			&& strcmp(func_name, "__builtin_floorf")
			&& strcmp(func_name, "__builtin_fmodf")
			&& strcmp(func_name, "__builtin_frexpf")
			&& strcmp(func_name, "__builtin_ldexpf")
			&& strcmp(func_name, "__builtin_logf")
			&& strcmp(func_name, "__builtin_log10f")
			&& strcmp(func_name, "__builtin_modf")
			&& strcmp(func_name, "__builtin_powf")
			&& strcmp(func_name, "__builtin_sinf")
			&& strcmp(func_name, "__builtin_sinhf")
			&& strcmp(func_name, "__builtin_sqrtf")
			&& strcmp(func_name, "__builtin_tanf")
			&& strcmp(func_name, "__builtin_tanhf"));
}

