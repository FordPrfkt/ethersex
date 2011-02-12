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

#define GWEATHER_CITYSIZE 16

#define GSERVICE_WEATHER_INIT {gweatherInit_v, gweatherGetRequestString_v, gweatherEndReceive_v, gweatherBeginReceive_v, gweatherParse_b}

int16_t gweatherUpdate_i16(char *cmd_pc, char *output_pc, uint16_t len_ui16);
void gweatherInit_v(void);
bool gweatherGetAttribute_b(char* inStr_pc, uint8_t inLen_ui8, char* outStr_pc, uint8_t outLen_ui8);
bool gweatherParse_b(char* data_pc, uint16_t len_ui16);
bool gweatherSetCity_b(char* city_pc, uint16_t len_ui16);
char* gweatherGetCity_ac(void);
void gweatherBeginReceive_v(void);
void gweatherEndReceive_v(void);
uint16_t gweatherGetRequestString_v(char request_ac[]);

#define HOOK_NAME gweather_updated
#define HOOK_ARGS (uint8_t result)
#include "hook.def"
#undef HOOK_NAME
#undef HOOK_ARGS

#include "config.h"
#ifdef DEBUG_GWEATHER
  #include "core/debug.h"
  #define GWEATHERDEBUG(a...)  debug_printf("gWeather: "a);
#else
  #define GWEATHERDEBUG(a...)
#endif

#endif  /* HAVE_GWEATHER_H */

/* EOF */
