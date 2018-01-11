#ifndef _POWER_TRIGGER_H
#define _POWER_TRIGGER_H

#ifdef __cplusplus
extern "C" {
#endif

int powertrigger_begin_logging(unsigned long pio_addr);
int powertrigger_end_logging(unsigned long pio_addr);

#ifdef __cplusplus
}
#endif

#endif /* _POWER_TRIGGER_H */

