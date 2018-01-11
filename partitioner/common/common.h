/*
 * common.h - common definitions and namespaces used by passes.
 *
 *  Created on: Apr 5, 2013
 *      Author: rlyerly
 */

#ifndef COMMON_H_
#define COMMON_H_

/*
 * Return values
 */
#define SUCCESS 0
#define FAILURE 1
#define NO_INPUT_FILES 2

/*
 * Namespaces commonly used
 */
using namespace std;
using namespace StringUtility;
using namespace SageBuilder;
using namespace SageInterface;
using namespace NodeQuery;

/*
 * Specifies the hardware partitions.
 *
 * NOTE: For now, MPI is arranged before GPU because this is used as a rank
 */
enum Hardware {
	X86,
	MPI,
	GPU
};

/*
 * Official names for architectures.
 */
#define X86_STR "x86"
#define GPU_STR "gpu"
#define MPI_STR "mpi"

/*
 * Specifies the analysis status of an object.
 */
enum analysisStatus {
	NOT_ANALYZED,
	IN_PROGRESS,
	ANALYZED
};

/*
 * Debugging helpers
 */
#define DEBUG(tool, msg) cerr << tool << " (Info): " << msg << endl
#define WARNING(tool, msg) cerr << tool << " (WARNING): " << msg << endl
#define ERROR(tool, msg) cerr << tool << " (ERROR): " << msg << endl

/*
 * Other useful helpers.
 */
#define EMPTY ""
#define NAME( var ) (var)->get_name().getString()

#endif /* COMMON_H_ */
