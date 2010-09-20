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
#include <stdbool.h>

#include "config.h"
#include "gweather.h"
#include "protocols/uip/uip.h"
#include "protocols/dns/resolv.h"
#include "protocols/ecmd/parser.h"
#include "protocols/ecmd/ecmd-base.h"

#include "../glcdmenu/glcdmenu.h"
#include "../glcdmenu/menu-interpreter/menu-interpreter-config.h"

#ifndef DNS_SUPPORT
#error "gWeather needs DNS support"
#endif

#define STATE (&uip_conn->appstate.gweather)
#define GWEATHER_HOST "www.google.de"

static uip_conn_t *gweather_conn;

static void gweatherQueryCB_v(char *name, uip_ipaddr_t *ipaddr);
static void gweatherMain_v(void);

void gweatherSendRequest_v(void)
{
	uint16_t len_ui16;
	len_ui16 = sprintf_P(uip_sappdata, REQUEST, gweatherCity_ac);
	GWEATHERDEBUG("Send GET request(%i): %s\n", len_ui16, ((char*)uip_sappdata));
	uip_send(uip_sappdata, len_ui16);
}

static void gweatherQueryCB_v(char *name, uip_ipaddr_t *ipaddr)
{
	if (NULL == ipaddr)
	{
		GWEATHERDEBUG ("Could not resolve address\n");
	}
	else
	{
		GWEATHERDEBUG ("Address resolved\n");
		gweather_conn = uip_connect(ipaddr, HTONS (80), gweatherMain_v);

		if (NULL != gweather_conn)
		{
			GWEATHERDEBUG ("Wait for connect\n");
			STATE->stage_e = GWEATHER_CONNECT;
		}
		else
		{
			GWEATHERDEBUG ("no uip_conn available.\n");
		}
	}
}

int16_t gweatherUpdate_i16(char *cmd_pc, char *output_pc, uint16_t len_ui16)
{
	uip_ipaddr_t* ip_p;

	GWEATHERDEBUG ("updating.\n");

	ip_p = resolv_lookup(GWEATHER_HOST);

	if (NULL == ip_p)
	{
		GWEATHERDEBUG ("Resolving Address\n");
		resolv_query(GWEATHER_HOST, gweatherQueryCB_v);
	}
	else
	{
		GWEATHERDEBUG ("address already resolved\n");
		gweather_conn = uip_connect(ip_p, HTONS(80), gweatherMain_v);

		if (NULL == gweather_conn)
		{
			GWEATHERDEBUG ("no uip_conn available.\n");
		}
		else
		{
			GWEATHERDEBUG ("Wait for connect\n");
			STATE->stage_e = GWEATHER_CONNECT;
		}

		return ECMD_FINAL_OK;
	}

	return ECMD_FINAL_OK;
}

static void gweatherMain_v(void)
{
	if (uip_aborted() || uip_timedout())
	{
		GWEATHERDEBUG ("connection aborted\n");
		gweather_conn = NULL;
	}

	if (uip_closed())
	{
		GWEATHERDEBUG("connection closed\n");
		gweather_conn = NULL;
		glcdmenuRedraw();
	}

	if (uip_connected() && STATE->stage_e == GWEATHER_CONNECT)
	{
		GWEATHERDEBUG("Connected\n");
		STATE->stage_e = GWEATHER_SEND_REQUEST;
	}

	if (uip_rexmit() && (STATE->stage_e == GWEATHER_SEND_REQUEST
			|| STATE->stage_e == GWEATHER_WAIT_RESPONSE))
	{
		GWEATHERDEBUG("Re-Xmit\n");
		gweatherSendRequest_v();
		STATE->stage_e = GWEATHER_WAIT_RESPONSE;
	}

	if (uip_poll() && (STATE->stage_e == GWEATHER_SEND_REQUEST))
	{
		gweatherSendRequest_v();
		STATE->stage_e = GWEATHER_WAIT_RESPONSE;
	}

	if (uip_acked() && STATE->stage_e == GWEATHER_WAIT_RESPONSE)
	{
		GWEATHERDEBUG("Request ACKed\n");
		STATE->stage_e = GWEATHER_RECEIVE;
		STATE->parserState_e = PARSER_WAIT_START;
		STATE->elementParserState_e = ELEMPARSER_WAIT_BEGIN;
	}

	if (uip_newdata())
	{
		if (uip_len && STATE->stage_e == GWEATHER_RECEIVE)
		{
			GWEATHERDEBUG("New Data\n");
			if (gweatherParse_b(((char *) uip_appdata), uip_len) == false)
			{
				GWEATHERDEBUG("Parser error\n");
				uip_close (); /* Parse error */
				return;
			}
		}
	}
}

void gweatherInit_v(void)
{
	GWEATHERDEBUG("initializing google weather client\n");

#ifdef GWEATHER_EEPROM_SUPPORT
	eeprom_restore(gweatherCity_ac, &gweatherCity_ac, GWEATHER_CITYSIZE);
#else
	sprintf(gweatherCity_ac, PSTR("%s"), CONF_GWEATHER_CITY);
#endif
}

int16_t gweather_onrequest(char *cmd, char *output, uint16_t len)
{
	GWEATHERDEBUG ("main\n");
	// enter your code here

	return ECMD_FINAL_OK;
}

/*
 -- Ethersex META --
 header(services/gweather/gweather.h)
 net_init(gweatherInit_v)

 state_header(services/gweather/gweather_state.h)
 state_tcp(struct gweather_connection_state_t gweather)
 */

/* EOF */
