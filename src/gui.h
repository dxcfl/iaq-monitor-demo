/*
SPDX-License-Identifier: GPL-3.0-or-later

iaq-monitor-demo
Copyright (C) 2021  dxcfl

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __GUI_H
#define __GUI_H

#include <zephyr.h>
#include <drivers/sensor.h>

void gui_setup(void);

void gui_update_sensor_value(enum sensor_channel channel, struct sensor_value value);

void gui_update_qmeter(int8_t quality, const char *rating);

void gui_update_headline(const char *str);

#endif
