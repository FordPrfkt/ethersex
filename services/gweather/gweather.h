/*
 * Copyright (c) 2010 by Daniel Walter <fordprfkt@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef HAVE_GWEATHER_H
#define HAVE_GWEATHER_H

#include <stdbool.h>

#define FC_NUM_ELEM 4
#define GWEATHER_CITYSIZE 16

typedef struct
{
	char dayOfWeek_ac[4];
	char lowTemp_ac[5];
	char highTemp_ac[5];
	char condition_ac[21];
} gWeatherForecast_t;

char city_ac[31];
char date_ac[11];
char condition_ac[21];
char temperature_ac[5];
char humidity_ac[21];
char wind_ac[51];
gWeatherForecast_t forecast_as[FC_NUM_ELEM];
char currentElement_ac[80];
char gweatherCity_ac[GWEATHER_CITYSIZE];

int16_t gweather_onrequest(char *cmd, char *output, uint16_t len);
int16_t gweatherUpdate_i16(char *cmd_pc, char *output_pc, uint16_t len_ui16);
void gweatherInit_v(void);
bool gweatherParse_b(char* data_pc, uint16_t len_ui16);
bool gweatherSetCity_b(char* city_pc, uint16_t len_ui16);

#include "config.h"
#ifdef DEBUG_GWEATHER
  #include "core/debug.h"
  #define GWEATHERDEBUG(a...)  debug_printf("gWeather: "a);
#else
  #define GWEATHERDEBUG(a...)
#endif

#endif  /* HAVE_GWEATHER_H */

/* EOF */
