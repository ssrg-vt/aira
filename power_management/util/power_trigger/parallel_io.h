#ifndef _PARALLEL_IO_H
#define _PARALLEL_IO_H

#ifdef __cplusplus
extern "C" {
#endif

bool pio_init(unsigned long pio_addr);
void pio_up(unsigned long pio_addr);
void pio_down(unsigned long pio_addr);
bool pio_teardown(unsigned long pio_addr);

#ifdef __cplusplus
}
#endif

#endif /* _PARALLEL_IO_H */

