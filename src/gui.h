#ifndef __GUI_H
#define __GUI_H

#include <zephyr.h>
#include <drivers/sensor.h>

void gui_setup(void);

void gui_update_sensor_value(enum sensor_channel channel, struct sensor_value value);

void gui_update_qmeter(int8_t quality, const char *rating);

void gui_update_headline(const char *str);

#endif
