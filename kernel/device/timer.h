#include "../header/stdint.h"
static void frequency_setter(uint8_t counter_port,
                             uint8_t counter_no,
                             uint8_t rwl,
                             uint8_t counter_mode,
                             uint16_t counter_value);
void timer_init();