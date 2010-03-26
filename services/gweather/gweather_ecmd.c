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
#include "gweather.h"
#include "protocols/ecmd/ecmd-base.h"

int16_t parse_cmd_update(char *cmd, char *output, uint16_t len)
{
  debug_printf("gWeather: update");
  return gweather_update(cmd, output, len);
}

int16_t parse_cmd_city(char *cmd, char *output, uint16_t len)
{
  if (len > 0){
    while (*cmd == ' ')
        cmd++;

    //sprintf(gweather_city, "%s", *cmd);

    if (len > GWEATHER_CITYSIZE)
      return ECMD_ERR_PARSE_ERROR;

    #ifdef GWEATHER_EEPROM_SUPPORT
      eeprom_save(gweather_city, gweather_city, GWEATHER_CITYSIZE);
      eeprom_update_chksum();
    #endif
  }

  return ECMD_FINAL(snprintf_P(output, GWEATHER_CITYSIZE, gweather_city));
}

/*
-- Ethersex META --
block(Google_Weather)
ecmd_feature(update, "weather update",, gWeather update)
ecmd_feature(city, "weather city", CITYNAME, gWeather set CITYNAME)
*/

/* EOF */
