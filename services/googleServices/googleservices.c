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
#include "googleservices_shared.h"
#include "googleservices_state.h"
#include "googleservices.h"
#include "protocols/uip/uip.h"
#include "protocols/dns/resolv.h"
#include "protocols/ecmd/parser.h"
#include "protocols/ecmd/ecmd-base.h"

#ifndef DNS_SUPPORT
#error "google services need DNS support"
#endif

#define STATE (&uip_conn->appstate.gservices)

static void gservicesQueryCB_v(char *name, uip_ipaddr_t *ipaddr);
static void gservicesMain_v(void);

void gservicesSendRequest_v(char request_ac[])
{
}

static void gservicesQueryCB_v(char *name, uip_ipaddr_t *ipaddr)
{
	if (NULL == ipaddr)
	{
		GSERVICESDEBUG ("Could not resolve address\n");
	}
	else
	{
		GSERVICESDEBUG ("Address resolved\n");
	}
}

bool gservicesUpdate_b(const gServiceFunctions_t* functions_ps)
{
	uip_ipaddr_t* ip_p;
	uip_conn_t* connection_ps;

	GSERVICESDEBUG ("updating.\n");

	ip_p = resolv_lookup(GSERVICES_HOST);

	if (NULL == ip_p)
	{
		GSERVICESDEBUG ("Resolving Address\n");
		resolv_query(GSERVICES_HOST, gservicesQueryCB_v);
	}
	else
	{
		GSERVICESDEBUG ("address already resolved\n");
		connection_ps = uip_connect(ip_p, HTONS(80), gservicesMain_v);

		if (NULL == connection_ps)
		{
			GSERVICESDEBUG ("no uip_conn available.\n");
		}
		else
		{
			GSERVICESDEBUG ("Wait for connect\n");
			connection_ps->appstate.gservices.stage_e = GSERVICE_CONNECT;
			memcpy(&connection_ps->appstate.gservices.functions_s, functions_ps, sizeof(gServiceFunctions_t));
			return true;
		}
	}

	return false;
}

static void gservicesMain_v(void)
{
	uint16_t len_ui16;

	if (uip_aborted() || uip_timedout())
	{
		GSERVICESDEBUG ("connection aborted\n");
		STATE->stage_e = GSERVICE_IDLE;
		STATE->functions_s.endReceive_v();
	}

	if (uip_closed())
	{
		GSERVICESDEBUG("connection closed\n");
		STATE->stage_e = GSERVICE_IDLE;
		STATE->functions_s.endReceive_v();
	}

	if (uip_connected() && STATE->stage_e == GSERVICE_CONNECT)
	{
		GSERVICESDEBUG("Connected\n");
		STATE->stage_e = GSERVICE_SEND_REQUEST;
	}

	if (uip_rexmit() && (STATE->stage_e == GSERVICE_SEND_REQUEST
			|| STATE->stage_e == GSERVICE_WAIT_RESPONSE))
	{
		GSERVICESDEBUG("Re-Xmit\n");
		len_ui16 = STATE->functions_s.getRequestString(uip_sappdata);
		GSERVICESDEBUG("Send GET request(%i): %s\n", len_ui16, ((char*)uip_sappdata));
		uip_send(uip_sappdata, len_ui16);

		STATE->stage_e = GSERVICE_WAIT_RESPONSE;
	}

	if (uip_poll() && (STATE->stage_e == GSERVICE_SEND_REQUEST))
	{
		len_ui16 = STATE->functions_s.getRequestString(uip_sappdata);
		GSERVICESDEBUG("Send GET request(%i): %s\n", len_ui16, ((char*)uip_sappdata));
		uip_send(uip_sappdata, len_ui16);
		STATE->stage_e = GSERVICE_WAIT_RESPONSE;
	}

	if (uip_acked() && STATE->stage_e == GSERVICE_WAIT_RESPONSE)
	{
		GSERVICESDEBUG("Request ACKed\n");
		STATE->stage_e = GSERVICE_RECEIVE;
		STATE->functions_s.beginReceive_v();
	}

	if (uip_newdata())
	{
		if (uip_len && STATE->stage_e == GSERVICE_RECEIVE)
		{
			GSERVICESDEBUG("New Data\n");
			if (STATE->functions_s.parse_b(((char *) uip_appdata), uip_len) == false)
			{
				GSERVICESDEBUG("Parser error\n");
				uip_close (); /* Parse error */
				return;
			}
		}
	}
}

void gservicesInit_v(void)
{
	GSERVICESDEBUG("initializing google services client\n");
}

/*
 -- Ethersex META --
 header(services/googleServices/googleservices.h)
 net_init(gservicesInit_v)

 state_header(services/googleServices/googleservices_state.h)
 state_tcp(struct gservices_connection_state_t gservices)
 */

/* EOF */
