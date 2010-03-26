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
#include "protocols/dns/resolv.h"
#include "protocols/ecmd/parser.h"
#include "protocols/ecmd/ecmd-base.h"

static uip_conn_t *gweather_conn;
static void gweather_query_cb(char *name, uip_ipaddr_t *ipaddr);
static void gweather_main(void);

#define STATE (&uip_conn->appstate.gweather)
#define set_CONF_GOOGLE_IP(ip) uip_ipaddr((ip), 192,168,100,108)

#ifndef DNS_SUPPORT
  #error "gWeather needs DNS support"
#endif

static const char PROGMEM gweather_request[] =
    "GET / HTTP/1.1\r\nHost: 192.168.100.108\r\nConnection: Close\r\n\r\n";

int16_t
gweather_update(char *cmd, char *output, uint16_t len){
  GWEATHERDEBUG ("updating. Connecting to server\n");

  uip_ipaddr_t ip;
  set_CONF_GOOGLE_IP(&ip);
  gweather_conn = uip_connect(&ip, HTONS(80), gweather_main);
  STATE->stage = GWEATHER_SEND_REQUEST;

  //ip_p = resolv_lookup("google.com");

//  if (NULL == ip_p) {
//    GWEATHERDEBUG ("Resolving Address\n");
//    resolv_query("www.google.com", gweather_query_cb);
//  }
//  else {
//    GWEATHERDEBUG ("address resolved. Connecting\n");
//    gweather_conn = uip_connect(ip_p, HTONS(80), gweather_main);
//
//    if (NULL == gweather_conn) {
//      GWEATHERDEBUG ("no uip_conn available.\n");
//    }
//    else {
//      GWEATHERDEBUG ("Connected.\n");
//      STATE->stage = GWEATHER_SEND_REQUEST;
//    }

//    return ECMD_FINAL_OK;
//  }

  return ECMD_FINAL_OK;
}

uint8_t gweather_parse(void) {
  GWEATHERDEBUG((char*)uip_appdata);
  return 0;
}

void gweather_send_request(void) {
  GWEATHERDEBUG("Send GET request\n");
  memcpy_P (uip_sappdata, gweather_request, sizeof (gweather_request));
  GWEATHERDEBUG((char*)uip_sappdata);
  uip_send (uip_sappdata, sizeof (gweather_request));
}

static void
gweather_query_cb(char *name, uip_ipaddr_t *ipaddr)
{
  if (NULL == ipaddr) {
    GWEATHERDEBUG ("could not resolve address\n");
  }
  else {
    GWEATHERDEBUG ("address resolved. Connecting.\n");
    gweather_conn= uip_connect (ipaddr, HTONS (80), gweather_main);

    if (NULL != gweather_conn) {
      GWEATHERDEBUG ("Connected.\n");
      STATE->stage = GWEATHER_SEND_REQUEST;
    }
    else {
      GWEATHERDEBUG ("could not connect\n");
    }
  }
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

    if (uip_connected() || uip_rexmit() || (uip_poll() && STATE->stage == GWEATHER_SEND_REQUEST)) {
      gweather_send_request();
    }

    if (uip_acked()) {
      GWEATHERDEBUG("Request ACKed\n");
      STATE->stage = GWEATHER_WAIT_RESPONSE;
    }

    if (uip_newdata()) {
      STATE->stage = GWEATHER_RECEIVE;

      /* Zero-terminate */
      ((char *) uip_appdata)[uip_len] = 0;
      GWEATHERDEBUG("received data: %s\n", uip_appdata);

        if (gweather_parse ()) {
            uip_close ();               /* Parse error */
            return;
        }
    }
}

void
gweather_init(void)
{
  GWEATHERDEBUG("initializing google weather client\n");

//  #ifdef GWEATHER_EEPROM_SUPPORT
//    eeprom_restore(gweather_city, &gweather_city, GWEATHER_CITYSIZE);
//  #else
//    sprintf(gweather_city, "%s", CONF_GWEATHER_CITY);
//  #endif
}

int16_t
gweather_onrequest(char *cmd, char *output, uint16_t len){
  GWEATHERDEBUG ("main\n");
  // enter your code here

  return ECMD_FINAL_OK;
}


/*
  -- Ethersex META --
  header(services/gweather/gweather.h)
  net_init(gweather_init)

  state_header(services/gweather/gweather_state.h)
  state_tcp(struct gweather_connection_state_t gweather)
 */

/* EOF */
