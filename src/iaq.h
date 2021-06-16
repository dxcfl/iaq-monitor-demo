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

#ifndef __IAQ_H
#define __IAQ_H

#include <zephyr.h>
#include "iaq.h"

uint8_t get_iaq_index(uint32_t temperature, uint32_t humidity, uint32_t eco2, uint32_t tvoc);

const char *get_iaq_rating(uint8_t iaq_index);

uint8_t get_min_iaq_index();

uint8_t get_max_iaq_index();

#endif
