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

#ifndef HAVE_GCALENDAR_H
#define HAVE_GCALENDAR_H

#include <stdbool.h>

#define GCALENDAR_LOGINSIZE 70

char gCalendarLogin_ac[GCALENDAR_LOGINSIZE];

int16_t gcalendarUpdate_i16(char *cmd_pc, char *output_pc, uint16_t len_ui16);
void gcalendarInit_v(void);
bool gcalendarGetAttribute_b(char* inStr_pc, uint8_t inLen_ui8, char* outStr_pc, uint8_t outLen_ui8);
bool gcalendarParse_b(char* data_pc, uint16_t len_ui16);
bool gcalendarSetLogin_b(char* login_pc, uint16_t len_ui16);
void gcalendarBeginReceive_v(void);
void gcalendarEndReceive_v(void);
uint16_t gcalendarGetRequestString_v(char request_ac[]);

#include "config.h"
#ifdef DEBUG_GCALENDAR
# include "core/debug.h"
# define GCALENDARDEBUG(a...)  debug_printf("gCalendar: "a);
#else
# define GCALENDARDEBUG(a...)
#endif

#endif  /* HAVE_GCALENDAR_H */
