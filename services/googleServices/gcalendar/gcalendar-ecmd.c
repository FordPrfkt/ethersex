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

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "config.h"
#include "gcalendar.h"
#include "protocols/ecmd/ecmd-base.h"

int16_t parse_cmd_cal_update(char *cmd, char *output, uint16_t len)
{
  return gcalendarUpdate_i16(cmd, output, len);
}

int16_t parse_cmd_login(char *cmd, char *output, uint16_t len)
{
   if (*cmd == '\0')
   {
	   return ECMD_FINAL(snprintf_P(output, len, PSTR("%s"), gCalendarLogin_ac));
   }

   return gcalendarSetLogin_b(cmd, len) == true ? ECMD_FINAL_OK:ECMD_ERR_PARSE_ERROR;
}

/*
-- Ethersex META --
block(Google_Calendar)
ecmd_feature(cal_update, "calendar cal_update",, gCalendar cal_update)
ecmd_feature(login, "calendar login ", LOGIN, gCalendar set LOGIN)
*/
