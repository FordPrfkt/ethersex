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

#ifndef DNS_SUPPORT
#error "gWeather needs DNS support"
#endif

#define STATE (&uip_conn->appstate.gweather)
#define GWEATHER_HOST "www.google.de"

static const char PROGMEM WEATHER_TAG[] = "weather";
static const char PROGMEM FORECAST_TAG[] = "forecast_information";
static const char PROGMEM CURRENT_COND_TAG[] = "current_conditions";
static const char PROGMEM FORECASTCOND_TAG[] = "forecast_conditions";
static const char PROGMEM CITY_TAG[] = "city data=";
static const char PROGMEM FORECAST_DATE_TAG[] = "forecast_date data=";
static const char PROGMEM CONDITION_TAG[] = "condition data=";
static const char PROGMEM TEMP_TAG[] = "temp_c data=";
static const char PROGMEM HUMIDITY_TAG[] = "humidity data=";
static const char PROGMEM WIND_TAG[] = "wind_condition data=";
static const char PROGMEM DOW_TAG[] = "day_of_week data=";
static const char PROGMEM LOW_TAG[] = "low data=";
static const char PROGMEM HIGH_TAG[] = "high data=";
static const char
		PROGMEM REQUEST[] =
				"GET /ig/api?weather=%s HTTP/1.1\nHost: "GWEATHER_HOST"\nConnection: Close\r\n\r\n";

static uip_conn_t *gweather_conn;

static void gweatherQueryCB_v(char *name, uip_ipaddr_t *ipaddr);
static void gweatherMain_v(void);

bool gweatherGetAttribute_b(char* inStr_pc, char* outStr_pc, uint8_t len_ui8)
{
	char* pos1 = 0;
	char* pos2 = 0;
	uint8_t size_ui8 = 0;
	char str = '"';

	/* Seek for text within "" */
	pos1 = strpbrk(inStr_pc, &str);
	pos2 = strrchr(inStr_pc, '"');

	/* Text found ? */
	if (pos1 != 0 && pos2 != 0 && pos2 > pos1)
	{
		/* Copy text */
		size_ui8 = (pos2 - pos1) - 1;

		if (size_ui8 > len_ui8)
			size_ui8 = len_ui8;
		memcpy(outStr_pc, pos1 + 1, size_ui8);
		outStr_pc[size_ui8] = 0;
	}
	else
	{
		GWEATHERDEBUG("Could not get Attribute: %s\n", inStr_pc);
	}

	return ((size_ui8 > 0) ? true : false);
}

bool gweatherParse_b(char* data_pc, uint16_t len_ui16)
{
	uint16_t pos_ui16 = 0;
	bool result_b = true;

	/* Check the received data block */
	while (pos_ui16 < len_ui16)
	{
		switch (STATE->elementParserState_e)
		{
		case ELEMPARSER_WAIT_BEGIN: /* Seek for tag opening "<" */
			if (data_pc[pos_ui16] == '<')
			{
				/* Found. Begin copy element */
				memset(currentElement_ac, 0, sizeof(currentElement_ac));
				STATE->elementParserState_e = ELEMPARSER_IN_ELEMENT;
				STATE->elemPos_ui8 = 0;
			}
			break;

		case ELEMPARSER_IN_ELEMENT:
			if (data_pc[pos_ui16] != '>')
			{
				/* Copy element content until closing tag ">" into currentElement_ac */
				if (STATE->elemPos_ui8 < sizeof(currentElement_ac) - 1)
				{
					currentElement_ac[STATE->elemPos_ui8] = data_pc[pos_ui16];
					STATE->elemPos_ui8++;
				}
			}
			else
			{
				/* Closing tag found */
				STATE->elementParserState_e = ELEMPARSER_DONE;
			}
			break;

		case ELEMPARSER_DONE:
			/* Wait for next tag */
			STATE->elementParserState_e = ELEMPARSER_WAIT_BEGIN;
			if (pos_ui16 > 0)
				pos_ui16--; /* Reparse the current character */
			break;

		default:
			/* Unknown state  */
			GWEATHERDEBUG("Oops! Dunno what to do!\n")
			;
			result_b = false;
			break;
		}

		/* If a tag was found, check the content */
		if (ELEMPARSER_DONE == STATE->elementParserState_e)
		{
			switch (STATE->parserState_e)
			{
			case PARSER_WAIT_START:
				/* Wait for content start */
				if (strstr_P(currentElement_ac, WEATHER_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: Content start\n");
					STATE->parserState_e = PARSER_WAIT_TAG;
					STATE->fcPos_ui8 = 0;
				}
				break;

			case PARSER_WAIT_TAG:
				/* Wait for section begin */

				if (strstr_P(currentElement_ac, FORECAST_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: Forecast tag\n");
					/* Forecast information: City, etc. */
					STATE->parserState_e = PARSER_IN_FORECAST;
				}
				else if (strstr_P(currentElement_ac, CURRENT_COND_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: Current cond. tag\n");
					/* Current condition */
					STATE->parserState_e = PARSER_IN_CURRENTCOND;
				}
				else if (strstr_P(currentElement_ac, FORECASTCOND_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: Forecast cond. tag\n");
					/* forecast condition for the next days, this block
					 * is repeated in the document */
					if (STATE->fcPos_ui8 < FC_NUM_ELEM)
					{
						/* While we have space for new data */
						STATE->parserState_e = PARSER_IN_FORECASTCOND;
					}
				}
				else if (strstr_P(currentElement_ac, WEATHER_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: End tag\n");
					/* Encountered the opening tag again. End of document */
					STATE->parserState_e = PARSER_DONE;
				}
				else
				{
					GWEATHERDEBUG("Ignored Tag: %s\n", currentElement_ac);
				}
				break;

			case PARSER_IN_FORECAST:
				if (strstr_P(currentElement_ac, CITY_TAG) != 0)
				{
					/* City tag */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							city_ac, sizeof(city_ac));
				}
				else if (strstr_P(currentElement_ac, FORECAST_DATE_TAG) != 0)
				{
					/* Condition tag */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							date_ac, sizeof(date_ac));
				}
				else if (strstr_P(currentElement_ac, FORECAST_TAG) != 0)
				{
					/* Closing tag found. Wait for next block. */
					STATE->parserState_e = PARSER_WAIT_TAG;
				}
				else
				{
					GWEATHERDEBUG("Ignored Tag: %s\n", currentElement_ac);
				}
				break;

			case PARSER_IN_CURRENTCOND:
				if (strstr_P(currentElement_ac, WIND_TAG) != 0)
				{
					/* Wind condition. This must be before the "Condtion"
					 * test */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							wind_ac, sizeof(wind_ac));
				}
				else if (strstr_P(currentElement_ac, CONDITION_TAG) != 0)
				{
					/* Condtion */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							condition_ac, sizeof(condition_ac));
				}
				else if (strstr_P(currentElement_ac, TEMP_TAG) != 0)
				{
					/* Temperature */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							temperature_ac, sizeof(temperature_ac));
				}
				else if (strstr_P(currentElement_ac, HUMIDITY_TAG) != 0)
				{
					/* Humidity */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							humidity_ac, sizeof(humidity_ac));
				}
				else if (strstr_P(currentElement_ac, CURRENT_COND_TAG) != 0)
				{
					/* Closing tag found */
					STATE->parserState_e = PARSER_WAIT_TAG;
				}
				else
				{
					GWEATHERDEBUG("Ignored Tag: %s\n", currentElement_ac);
				}
				break;

			case PARSER_IN_FORECASTCOND:
				if (strstr_P(currentElement_ac, DOW_TAG) != 0)
				{
					/* Day of week */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							forecast_as[STATE->fcPos_ui8].dayOfWeek_ac,
							sizeof(forecast_as[STATE->fcPos_ui8].dayOfWeek_ac));
				}
				else if (strstr_P(currentElement_ac, LOW_TAG) != 0)
				{
					/* Low temperature */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							forecast_as[STATE->fcPos_ui8].lowTemp_ac,
							sizeof(forecast_as[STATE->fcPos_ui8].lowTemp_ac));
				}
				else if (strstr_P(currentElement_ac, HIGH_TAG) != 0)
				{
					/* High temperature */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							forecast_as[STATE->fcPos_ui8].highTemp_ac,
							sizeof(forecast_as[STATE->fcPos_ui8].highTemp_ac));
				}
				else if (strstr_P(currentElement_ac, CONDITION_TAG) != 0)
				{
					/* Weather condition */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							forecast_as[STATE->fcPos_ui8].condition_ac,
							sizeof(forecast_as[STATE->fcPos_ui8].condition_ac));
				}
				else if (strstr_P(currentElement_ac, FORECASTCOND_TAG) != 0)
				{
					/* Closing tag found. Next forecast? */
					STATE->fcPos_ui8++;
					STATE->parserState_e = PARSER_WAIT_TAG;
				}
				else
				{
					GWEATHERDEBUG("Ignored Tag: %s\n", currentElement_ac);
				}
				break;

			case PARSER_DONE:
			{
#ifdef DEBUG_GWEATHER
				uint8_t i;
				GWEATHERDEBUG("Done\n");
				GWEATHERDEBUG("City: %s\n", city_ac);
				GWEATHERDEBUG("Date: %s\n", date_ac);
				GWEATHERDEBUG("Current conditions:\n");
				GWEATHERDEBUG("Condition: %s\n", condition_ac);
				GWEATHERDEBUG("Temperature: %s °C\n", temperature_ac);
				GWEATHERDEBUG("Humidity: %s\n", humidity_ac);
				GWEATHERDEBUG("Wind: %s\n", wind_ac);
				GWEATHERDEBUG("Forecast:\n");
				for (i = 0; i < FC_NUM_ELEM; i++)
				{
					GWEATHERDEBUG("Day: %s\n", forecast_as[i].dayOfWeek_ac);
					GWEATHERDEBUG("Temperatures from %s to %s °C\n", forecast_as[i].lowTemp_ac, forecast_as[i].highTemp_ac);
					GWEATHERDEBUG("Condition: %s\n", forecast_as[i].condition_ac);
				}
#endif
			}
				break;

			default:
				/* Unknown state  */
				GWEATHERDEBUG("Oops! Dunno what to do!\n")
				;
				result_b = false;
				break;
			}
		}

		/* Next character */
		pos_ui16++;
	}

	return result_b;
}

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

bool gweatherSetCity_b(char* city_pc, uint16_t len_ui16)
{
	if (len_ui16 > GWEATHER_CITYSIZE)
		return false;

	memcpy(city_pc, gweatherCity_ac, len_ui16);

#ifdef GWEATHER_EEPROM_SUPPORT
	eeprom_save(gweatherCity_ac, gweatherCity_ac, GWEATHER_CITYSIZE);
	eeprom_update_chksum();
#endif

	return true;
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

		memset(forecast_as, 0, sizeof(forecast_as));
		memset(city_ac, 0, sizeof(city_ac));
		memset(date_ac, 0, sizeof(date_ac));
		memset(condition_ac, 0, sizeof(condition_ac));
		memset(temperature_ac, 0, sizeof(temperature_ac));
		memset(humidity_ac, 0, sizeof(humidity_ac));
		memset(wind_ac, 0, sizeof(wind_ac));
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
