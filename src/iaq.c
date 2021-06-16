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

#include <zephyr.h>
#include "iaq.h"

#define IAQ_REGARDED_MEASUREMENTS 4

/* 
This code provides calculations of the Indoor Air Qualitiy index according to
http://www.iaquk.org.uk/ESW/Files/IAQ_Rating_Index.pdf for
temperature, humidity, CO2 and TVOC concentration.
*/

/*
Temperature (°C)

Excellent: 18 - 21°C
Good: Plus or minus 1°C 
Fair: Plus or minus 2°C 
Poor: Plus or minus 3°C 
Inadequate: Plus or minus 4°C or more
*/
uint8_t points_temperature(uint32_t temperature)
{
    uint8_t points = 5;
    const uint32_t excellent_low = 18;
    const uint32_t excellent_high = 21;

    if (temperature < excellent_low)
    {
        points -= excellent_low - temperature > 4 ? 4 : excellent_low - temperature;
    }
    else if (temperature > excellent_high)
    {
        points -= temperature - excellent_high > 4 ? 4 : temperature - excellent_high;
    }

    return points;
}

/*
Relative Humidity (% RH)

Excellent: 40 - 60 % RH
Good: < 40 / > 60 % RH 
Fair: < 30 / > 70 % RH 
Poor: < 20 / > 80 % RH 
Inadequate: < 10 / > 90 % RH 
*/
uint8_t points_humidity(uint32_t humidity)
{
    uint8_t points = 0;

    if (humidity < 10 || humidity > 90)
    {
        points = 1;
    }
    else if (humidity < 20 || humidity > 80)
    {
        points = 2;
    }
    else if (humidity < 30 || humidity > 70)
    {
        points = 3;
    }
    else if (humidity < 40 || humidity > 60)
    {
        points = 4;
    }
    else if (humidity >= 40 && humidity <= 60)
    {
        points = 5;
    }

    return points;
}

/*
Carbon Dixoide (PPM)

Excellent: Below 600 PPM 
Good: 601 - 1000 PPM 
Fair: 1000 - 1500 PPM 
Poor: 1500 - 1800 PPM 
Inadequate: 1800 PPM + 
*/
uint8_t points_co2(uint32_t co2)
{
    uint8_t points = 0;

    if (co2 <= 600)
    {
        points = 5;
    }
    else if (co2 <= 800)
    {
        points = 4;
    }
    else if (co2 <= 1500)
    {
        points = 3;
    }
    else if (co2 <= 1800)
    {
        points = 2;
    }
    else if (co2 > 1800)
    {
        points = 1;
    }

    return points;
}

/*
TVOC (ppb)

Since the rating in http://www.iaquk.org.uk/ESW/Files/IAQ_Rating_Index.pdf 
is given only for units of mg/m3 and most of the available sensors produce
this value only in units of ppb, it would have been necessary to convert
measurements in ppb to values in mg/m3- 

In order to map the TVOC measurement in ppb to
mg/m3 a gas mixture would have to be assumed which represents a typical TVOC mixture.
Based on this mixture, an average molar mass could be calculated which could be further
used to directly convert ppb into mg/m3. 

As an alternative to this approach, the evaluation of the TVOC measurement in ppb,
is done by referencing a table published by the German Federal Environmental Agency.
Following the human perception, the German Federal Environmental Agency 
(Bundesgesundheitsblatt – Gesundheitsforschung Gesundheitsschutz 2007, 50:990–1005, 
Springer Medizin Verlag 2007. (DOI 10.1007/s00103-007-0290-y) translates TVOC concentration 
(parts per billion) on a logarithmic scale into five indoor air quality levels (IAQ):

Excellent: 0 - 0.065 ppm  (<= 65 ppb)
Good: 0.065 - 0.22 ppm    (< =220 ppb)
Moderate: 0.22 - 0.66 ppm (<= 660 ppb)
Poor: 0.66 - 2.2 ppm      (<= 2200 ppb)
Unhealthy: 2.2 - 5.5 ppm   (> 2200 ppb) 
*/
uint8_t points_tvoc(uint32_t tvoc)
{
    uint8_t points = 0;

    if (tvoc <= 65)
    {
        points = 5;
    }
    else if (tvoc <= 220)
    {
        points = 4;
    }
    else if (tvoc <= 660)
    {
        points = 3;
    }
    else if (tvoc <= 2200)
    {
        points = 2;
    }
    else if (tvoc > 2200)
    {
        points = 1;
    }

    return points;
}

/* IAQI: The sum of all calculated points for each given indicator / sensor value.
*/
uint8_t get_iaq_index(uint32_t temperature, uint32_t humidity, uint32_t eco2, uint32_t tvoc)
{
    uint8_t points = 0;

    points += points_temperature(temperature);
    points += points_humidity(humidity);
    points += points_co2(eco2);
    points += points_tvoc(tvoc);

    return points;
}

/* IAQI rating: 5 levels based on the given IAQI.
*/
const char *get_iaq_rating(uint8_t iaq_index)
{

    if (iaq_index < 2 * IAQ_REGARDED_MEASUREMENTS)
    {
        return "Inadequate";
    }
    else if (iaq_index < 3 * IAQ_REGARDED_MEASUREMENTS)
    {
        return "Poor";
    }
    else if (iaq_index < 4 * IAQ_REGARDED_MEASUREMENTS)
    {
        return "Fair";
    }
    else if (iaq_index < 5 * IAQ_REGARDED_MEASUREMENTS)
    {
        return "Good";
    }
    else
    {
        return "Excellent";
    }
}

uint8_t get_min_iaq_index()
{
    return IAQ_REGARDED_MEASUREMENTS;
}

uint8_t get_max_iaq_index()
{
    return IAQ_REGARDED_MEASUREMENTS * 5;
}
