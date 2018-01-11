/*
 * Feature-extractor plugin for GCC
 * Author: Rob Lyerly
 *
 * Extracts features from the code being compiled for analysis
 * as inputs into the performance models so the program can be
 * statically partitioned in the best manner possible.
 */

#include "features.h"
#include "tree.h"
#include "tree-pass.h"
#include "tree-check.h"
#include "c-family/c-common.h"

using namespace std;

/***************************** Name definitions ******************************/
#define P_NAME "feature_extractor"
#define P_REP_SSA "*all_optimizations"
#define P_VER "0.7"
#define P_DESC "Feature extractor plugin for GCC v4.8. \
		Extracts relevant features from the source code to give to \
		the Popcorn MKL performance models for analysis and partitioning."

/**************************** Argument definitions ***************************/
#define P_FILE_ARG "outfile"
#define P_NO_SAVE "nosave"
#define P_PRINT_OUTPUT "print"
#define P_DEBUGGING "debug"
#define P_DEBUG_MEMORY "memdebug"

/****************************** GCC definitions ******************************/
int plugin_is_GPL_compatible;
#define GCC_SUPPORTED_VERSION "4.8.2"

/****************************** Other variables ******************************/
static string output_filename;
static bool save_output = true;
static bool print_output = false;
static bool debugging = false;
static bool memory_debugging = false;

/************************ Plugin Function declarations ***********************/
int plugin_init(struct plugin_name_args* info, struct plugin_gcc_version* ver);
static bool extract_features_gate();
static unsigned extract_features();
void parse_args(struct plugin_name_args* info);

/**************************** Function definitions ***************************/
int plugin_init(struct plugin_name_args* info, struct plugin_gcc_version* ver) {

	static struct gimple_opt_pass fe_pass;
	static struct register_pass_info fe_reg_pass;
//	static struct plugin_info fe_info;
	string supported(GCC_SUPPORTED_VERSION);

	//Check GCC version number.  TODO -> more thorough checking?
	if (!supported.compare(ver->basever))
		return INCORRECT_VERSION;

	//Get any passed arguments
	parse_args(info);

	//Register our callback information.  Tell GCC what to call and
	//when to call it, as well as our version and description info.
//	fe_info.version = P_VER;
//	fe_info.help = P_DESC;

	fe_pass.pass.type = GIMPLE_PASS;
	fe_pass.pass.name = P_NAME;
	fe_pass.pass.gate = extract_features_gate;
	fe_pass.pass.execute = extract_features;
	fe_pass.pass.sub = NULL;
	fe_pass.pass.next = NULL;
	fe_pass.pass.static_pass_number = 0;
	fe_pass.pass.tv_id = TV_NONE;
	fe_pass.pass.properties_required = 0;
	fe_pass.pass.properties_provided = 0;
	fe_pass.pass.properties_destroyed = 0;
	fe_pass.pass.todo_flags_start = 0;
	fe_pass.pass.todo_flags_finish = 0;

	fe_reg_pass.pass = &fe_pass.pass;
	fe_reg_pass.reference_pass_name = P_REP_SSA;
	fe_reg_pass.ref_pass_instance_number = 1;
	fe_reg_pass.pos_op = PASS_POS_INSERT_AFTER;

	register_callback(P_NAME, PLUGIN_PASS_MANAGER_SETUP, NULL, &fe_reg_pass);
	//TODO #2 -> setting up plugin information doesn't seem to be working.
	//Need to check out more thoroughly (try other GCC versions)
	//register_callback(P_NAME, PLUGIN_INFO, NULL, &fe_info);

	return SUCCESS;
}

/* Parses the command line arguments to configure the plugin */
void parse_args(struct plugin_name_args* info) {
	for (int i = 0; i < info->argc; i++) {
		string cur_arg = ((info->argv)[i]).key;
		if (!cur_arg.compare(P_FILE_ARG)) {
			if (((info->argv)[i]).value)
				output_filename = ((info->argv)[i]).value;
		} else if (!cur_arg.compare(P_NO_SAVE)) {
			save_output = false;
		} else if (!cur_arg.compare(P_PRINT_OUTPUT)) {
			print_output = true;
		} else if (!cur_arg.compare(P_DEBUGGING)) {
			debugging = true;
		} else if (!cur_arg.compare(P_DEBUG_MEMORY)){
			memory_debugging = true;
		}
	}
}

/* Function that tells GCC whether or not to run the feature extractor */
static bool extract_features_gate() {
	return true;
}

/* Callback function used to process every basic block in each function.
 * This is where the main feature-extraction process occurs.
 */
static unsigned extract_features() {

	basic_block bb;
	long bb_num = 0, bb_count = 0;
	bool use_profile = false;
	gimple_stmt_iterator gsi;
	FeatureCount fc(main_input_filename, current_function_name(),
			n_basic_blocks, n_edges, debugging, memory_debugging);

	//Use profiling statistics, if enabled
	if (profile_status_for_function(cfun) == PROFILE_READ)
		use_profile = true;

	printf("Ran %s %lu times\n", current_function_name(), ENTRY_BLOCK_PTR_FOR_FUNCTION(cfun)->count);
	fc.set_func_count(ENTRY_BLOCK_PTR_FOR_FUNCTION(cfun)->count);

	//Iterate over each basic block in the function
	FOR_EACH_BB(bb)
	{
		if (use_profile)
			bb_count = bb->count;
		else
			bb_count = bb->frequency;
		if (bb_count < 1) //Sanity check
			bb_count = 1;
/*		if (bb_num == 0)
			fc.set_func_count(bb_count);*/

		//Iterate over each GIMPLE statement in the basic block
		for (gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi))
			fc.count_gimple_feature(bb_num, bb_count, gsi_stmt(gsi));

		//Iterate over each GIMPLE PHI node in the basic block
		for (gsi = gsi_start_phis(bb); !gsi_end_p(gsi); gsi_next(&gsi))
			fc.count_gimple_feature(bb_num, bb_count, gsi_stmt(gsi));

		bb_num++;
	}

	//Print to standard out
	if (print_output)
		fc.write_to_stdout();

	//Save the features to file
	if (save_output) {
		string filename;
		int period_pos = 0;
		if (output_filename.empty()) {
			filename = fc.get_filename();
			period_pos = filename.find_last_of('.');
			if (period_pos > 0)
				filename = filename.substr(0, period_pos);
		} else
			filename = output_filename;
		filename += "_";
		filename += fc.get_funcname();
		filename += ".features";
		if (fc.write_to_file(filename))
			return FILE_WRITE_ERROR;
	}

	return SUCCESS;
}
