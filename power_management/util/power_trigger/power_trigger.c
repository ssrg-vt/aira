/*****************************************************************************/
/* Implementation of public API                                              */
/*****************************************************************************/

#include <stdbool.h>

#include "parallel_io.h"
#include "power_trigger.h"

int powertrigger_begin_logging(unsigned long pio_addr)
{
#ifdef _VERBOSE
	fprintf(stderr, "Begin logging\n");
#endif

	if(pio_init(pio_addr))
		return -1;
	pio_up(pio_addr);
	return 0;
}

int powertrigger_end_logging(unsigned long pio_addr)
{
#ifdef _VERBOSE
	fprintf(stderr, "End logging\n");
#endif

	pio_down(pio_addr);
	if(pio_teardown(pio_addr))
		return -1;
	return 0;
}

