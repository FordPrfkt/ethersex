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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "googleservices_shared.h"
#include "googleservices_state.h"
#include "googleservices.h"
#include "gweather/gweather.h"
#include "gcalendar/gcalendar.h"
#include "protocols/uip/uip.h"
#include "protocols/dns/resolv.h"

/* Network state */
#define STATE (&uip_conn->appstate.gservices)

/* Pointers to to the google services */
static const gServiceFunctions_t funcPtrs[GSERVICES_NUM_SERVICES] = {
		GSERVICE_WEATHER_INIT,
		GSERVICE_CALENDAR_INIT
};

/* Prototypes */
static void gservicesQueryCB_v(char *name, uip_ipaddr_t *ipaddr);
static void gservicesMain_v(void);

/**
 * @brief Callback for resolv_query.
 *
 * Called from the network stack.
 * The UIP stack calls this function to indicate the result of
 * resolv_query.
 *
 * @param name name of the address being resolved
 * @param ipaddr pointer to the result
 * @returns void
 */
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

/**
 * @brief Requests a update for the given service.
 *
 * Starts a update for the service given in serviceType_e.
 * The result indicates if a network connection for the update
 * could be obtained.
 *
 * @param serviceType_e The service to update
 * @returns TRUE = update started, FALSE = update failed
 */
bool gservicesUpdate_b(gservicesServiceTypes_t serviceType_e)
{
	uip_ipaddr_t* ip_p;
	uip_conn_t* connection_ps;

	GSERVICESDEBUG ("updating.\n");

	/* Try to find the network address */
	ip_p = resolv_lookup(GSERVICES_HOST);

	/* Address was not resolved yet */
	if (NULL == ip_p)
	{
		GSERVICESDEBUG ("Resolving Address\n");
		resolv_query(GSERVICES_HOST, gservicesQueryCB_v);
	}
	else
	{
		/* Address already resolved */
		GSERVICESDEBUG ("address already resolved\n");

		/* Try to open a connection */
		connection_ps = uip_connect(ip_p, HTONS(80), gservicesMain_v);

		/* Could not get a connection */
		if (NULL == connection_ps)
		{
			GSERVICESDEBUG ("no uip_conn available.\n");
		}
		else
		{
			/* Connection OK, start update */
			GSERVICESDEBUG ("Wait for connect\n");
			connection_ps->appstate.gservices.stage_e = GSERVICE_CONNECT;
			connection_ps->appstate.gservices.service_e = serviceType_e;
			return true;
		}
	}

	return false;
}

/**
 * @brief Main callback from the UIP stack.
 *
 * This function is called from the UIP stack on any network events
 * (connection open, closed, aborted, timeout, new data received).
 * It calls the handler functions of the requesting service to process the event.
 *
 * @param void
 * @returns void
 */
static void gservicesMain_v(void)
{
	uint16_t len_ui16;

	/* New data received */
	if (uip_newdata())
	{
		/* Check if data length >0 and we are expecting new data */
		if (uip_len && STATE->stage_e == GSERVICE_RECEIVE)
		{
			GSERVICESDEBUG("New Data\n");

			/* Call the parser of the requesting service */
			if (funcPtrs[STATE->service_e].parse_b(((char *) uip_appdata),
					uip_len) == false)
			{
				GSERVICESDEBUG("Parser error\n");
				uip_close (); /* Parse error */
				return;
			}
		}
	}

	/* Connection aborted or timeout occurred */
	if (uip_aborted() || uip_timedout())
	{
		GSERVICESDEBUG ("connection aborted\n");

		/* Go back to idle state and wait for new request */
		STATE->stage_e = GSERVICE_IDLE;
		funcPtrs[STATE->service_e].endReceive_v();
	}

	/* Connection closed, if it was open previously */
	if (uip_closed() && STATE->stage_e != GSERVICE_IDLE)
	{
		GSERVICESDEBUG("connection closed\n");

		/* Go back to idle state and wait for new request */
		STATE->stage_e = GSERVICE_IDLE;
		funcPtrs[STATE->service_e].endReceive_v();
	}

	/* Host connected. Check if we are expecting this */
	if (uip_connected() && STATE->stage_e == GSERVICE_CONNECT)
	{
		GSERVICESDEBUG("Connected\n");
		STATE->stage_e = GSERVICE_SEND_REQUEST;
	}

	/* Host request to re-send the previous request. Only do that if we
	 * we already sent a request. */
	if (uip_rexmit() && (STATE->stage_e == GSERVICE_SEND_REQUEST
			|| STATE->stage_e == GSERVICE_WAIT_RESPONSE))
	{
		GSERVICESDEBUG("Re-Xmit\n");

		/* Get the request string from the service and send it */
		len_ui16 = funcPtrs[STATE->service_e].getRequestString(uip_sappdata);
		GSERVICESDEBUG("Send GET request(%i): %s\n", len_ui16, ((char*)uip_sappdata));
		uip_send(uip_sappdata, len_ui16);

		STATE->stage_e = GSERVICE_WAIT_RESPONSE;
	}

	/* UIP asks if we want to do something. If we want to send a request, do that */
	if (uip_poll() && (STATE->stage_e == GSERVICE_SEND_REQUEST))
	{
		/* Get the request string from the service and send it */
		len_ui16 = funcPtrs[STATE->service_e].getRequestString(uip_sappdata);
		GSERVICESDEBUG("Send GET request(%i): %s\n", len_ui16, ((char*)uip_sappdata));
		uip_send(uip_sappdata, len_ui16);
		STATE->stage_e = GSERVICE_WAIT_RESPONSE;
	}

	/* Host has ACKed the request. Wait for incoming data. */
	if (uip_acked() && STATE->stage_e == GSERVICE_WAIT_RESPONSE)
	{
		GSERVICESDEBUG("Request ACKed\n");
		STATE->stage_e = GSERVICE_RECEIVE;

		/* Prepare the service for incoming data */
		funcPtrs[STATE->service_e].beginReceive_v();
	}
}

 /**
  * @brief Initializer function called from esex on reset
  *
  * Initializes the app and the services after a reset.
  *
  * @param void
  * @returns void
  */
void gservicesInit_v(void)
{
	uint8_t i;
	GSERVICESDEBUG("initializing google services client\n");

	/* Try to resolve host address first, so hopefully
	 * the address is resolved when we send out the first request. */
	resolv_query(GSERVICES_HOST, gservicesQueryCB_v);

	/* Call init functions for all services */
	for (i = 0; i < GSERVICES_NUM_SERVICES; i++)
	{
		funcPtrs[i].init_v();
	}
}

/*
 -- Ethersex META --
 header(services/googleServices/googleservices.h)
 net_init(gservicesInit_v)

 state_header(services/googleServices/googleservices_state.h)
 state_tcp(struct gservices_connection_state_t gservices)
 */

/* EOF */
