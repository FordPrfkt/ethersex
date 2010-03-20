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

#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "gweather.h"
#include "protocols/uip/uip.h"
#include "protocols/ecmd/parser.h"
#include "protocols/ecmd/ecmd-base.h"

static uip_conn_t *gweather_conn;
#define STATE (&uip_conn->appstate.gweather)

int16_t
gweather_update(char *cmd, char *output, uint16_t len){
  GWEATHERDEBUG ("updating\n");

  uip_ipaddr_t ip;
  set_CONF_GWEATHER_SERVER(&ip);
  gweather_conn = uip_connect(&ip, HTONS(80), gweather_main);

  if (! gweather_conn) {
    GWEATHERDEBUG ("no uip_conn available.\n");
    return;
  }

#ifdef GWEATHER_EEPROM_SUPPORT
      eeprom_restore(gweather_city, &gweather_city, GWEATHER_CITYSIZE);
#else
      sprintf(gweather_city, "%s", CONF_GWEATHER_CITY);
#endif
}

void gweather_parse(void) {

}

void gweather_send_request(void) {

}

static void
gweather_main(void)
{
    if (uip_aborted() || uip_timedout()) {
      GWEATHERDEBUG ("connection aborted\n");
      gweather_conn = NULL;
    }

    if (uip_closed()) {
      GWEATHERDEBUG("connection closed\n");
      gweather_conn = NULL;
    }

    if (uip_connected()) {
      GWEATHERDEBUG("new connection\n");
      STATE->stage = GWEATHER_SEND_REQUEST;
    }

    if (uip_acked() && STATE->stage == GWEATHER_SEND_REQUEST) {
      STATE->stage = GWEATHER_WAIT_RESPONSE;
    }

    if (uip_newdata() && uip_len) {
      STATE->stage = GWEATHER_RECEIVE;

      /* Zero-terminate */
      ((char *) uip_appdata)[uip_len] = 0;
      GWEATHERDEBUG("received data: %s\n", uip_appdata);

        if (gweather_parse ()) {
            uip_close ();               /* Parse error */
            return;
        }
    }

    if (uip_poll() && STATE->stage == GWEATHER_SEND_REQUEST) {
      gweather_send_request();
      STATE->stage == GWEATHER_WAIT_RESPONSE;
    }
    else if (uip_rexmit()) {
      gweather_send_request();
    }
    else {

    }
}

void
gweather_init(void)
{
  GWEATHERDEBUG("initializing google weather client\n");
}

int16_t
gweather_onrequest(char *cmd, char *output, uint16_t len){
  GWEATHERDEBUG ("main\n");
  // enter your code here
}


/*
  -- Ethersex META --
  header(services/gweather/gweather.h)
  net_init(gweather_init)

  state_header(services/gweather/gweather_state.h)
  state_tcp(struct gweather_connection_state_t gweather)
 */

/* EOF */
