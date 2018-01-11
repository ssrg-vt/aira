#include <time.h>
#include <stdbool.h>
#include <assert.h>
#ifdef _VERBOSE
#include <stdio.h>
#endif

#include <unistd.h>
#include <sys/io.h>

#include "power_trigger.h"

/* Ports */
#define DATA_PORT( base )   (base)
#define STATUS_PORT( base ) (base + 1)
#define CTRL_PORT( base )   (base + 2)

/* Bit-wise operations, "val" is assumed to be a byte */
#define RD_BIT( val, num )  ((val >> num) & 0x1)
#define SET_BIT( val, num ) val = (val | (1 << num))
#define CLR_BIT( val, num ) val = (val & (0xff - (1 << num)))

/* Status bits */
#define IRQ( val )   RD_BIT(val, 2)
#define ERROR( val ) RD_BIT(val, 3)
#define SLCT( val )  RD_BIT(val, 4)
#define PE( val )    RD_BIT(val, 5)
#define ACK( val )   RD_BIT(val, 6)
#define BUSY( val )  (~RD_BIT(val, 7))

/* Control bits */
#define SET_STROBE( val )     CLR_BIT(val, 0)
#define CLR_STROBE( val )     SET_BIT(val, 0)
#define SET_AUTO_FD_XT( val ) CLR_BIT(val, 1)
#define CLR_AUTO_FD_XT( val ) SET_BIT(val, 1)
#define SET_INIT( val )       SET_BIT(val, 2)
#define CLR_INIT( val )       CLR_BIT(val, 2)
#define SET_SLCT_IN( val )    CLR_BIT(val, 3)
#define CLR_SLCT_IN( val )    SET_BIT(val, 3)
#define ENABLE_IRQ( val )     SET_BIT(val, 4)
#define DISABLE_IRQ( val )    CLR_BIT(val, 4)
#define XM_READ( val )        SET_BIT(val, 5)
#define XM_WRITE( val )       CLR_BIT(val, 5)

bool pio_init(unsigned long pio_addr)
{
	if(geteuid() != 0)
	{
#ifdef _VERBOSE
		fprintf(stderr, "Must be root to enable trigger!\n");
#endif
		return true;
	}

	// Enable dev
	if(ioperm(DATA_PORT(pio_addr), 1, true))
	{
#ifdef _VERBOSE
		fprintf(stderr, "Could not enable device at %x!\n", pio_addr);
#endif
		return true;
	}
	else
		return false;
}

void pio_up(unsigned long pio_addr)
{
	unsigned char trigger_on = 0xff;
#ifdef _VERBOSE
	fprintf(stderr, "Pulling up\n");
#endif
	outb(trigger_on, DATA_PORT(pio_addr));
}

void pio_down(unsigned long pio_addr)
{
	unsigned char trigger_off = 0x00;
#ifdef _VERBOSE
	fprintf(stderr, "Pulling down\n");
#endif
	outb(trigger_off, DATA_PORT(pio_addr));
}

bool pio_teardown(unsigned long pio_addr)
{
	if(geteuid() != 0)
	{
#ifdef _VERBOSE
		fprintf(stderr, "Must be root to disable trigger!\n");
#endif
		return true;
	}

	// Disable dev
	if(ioperm(DATA_PORT(pio_addr), 1, false))
	{
#ifdef _VERBOSE
		fprintf(stderr, "Could not disable device %d!\n", pio_addr);
#endif
		return true;
	}
	else
		return false;
}
