// Must define this to avoid mpi.h setting it to an attribute that ROSE does
// not like.
#define OMPI_DECLSPEC
// @@MPI_INCLUDE_HERE@@
#include <limits.h>
#include <stdlib.h>

// @@NEW_INCLUDES_HERE@@

#define FUNCTION_CODE_END 0

// TODO: Make the status variable global, for now
MPI_Status __mpi_status__;

int main(int argc, char **argv) {
  int finished = 0;

  MPI_Init(&argc, &argv);

  while (!finished) {
    int function_code = INT_MAX;
    MPI_Recv(&function_code, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &__mpi_status__);
    // TODO: Check status?

    switch (function_code) {
      case FUNCTION_CODE_END:
        finished = 1;
	break;
      default:
        // TODO Log an error.
        finished = 1;
    }
  }

  MPI_Finalize();
  return 0;
}
/* vim: set expandtab shiftwidth=2 tabstop=2 softtabstop=2: */
