#ifndef __IAQ_H
#define __IAQ_H

#include <zephyr.h>
#include "iaq.h"

uint8_t get_iaq_index(uint32_t temperature, uint32_t humidity, uint32_t eco2, uint32_t tvoc);

const char *get_iaq_rating(uint8_t iaq_index);

uint8_t get_min_iaq_index();

uint8_t get_max_iaq_index();


#endif
