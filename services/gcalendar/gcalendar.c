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
#include "gcalendar.h"
#include "protocols/uip/uip.h"
#include "protocols/dns/resolv.h"
#include "protocols/ecmd/parser.h"
#include "protocols/ecmd/ecmd-base.h"

#include "../glcdmenu/glcdmenu.h"
#include "../glcdmenu/menu-interpreter/menu-interpreter-config.h"

#ifndef DNS_SUPPORT
#error "gCalendar needs DNS support"
#endif

#define STATE (&uip_conn->appstate.gcalendar)
#define GCALENDAR_HOST "www.google.de"

static const char PROGMEM FEED_TAG[] = "feed";
static const char PROGMEM ENTRY_TAG[] = "entry";
static const char PROGMEM TITLE_TAG[] = "title type='text'";
static const char PROGMEM WHEN_TAG[] = "gd:when";

static const char PROGMEM REQUEST[] =
"GET /calendar/feeds/%s/full HTTP/1.1\nHost: "GCALENDAR_HOST"\nConnection: Close\r\n\r\n";

static uip_conn_t *gcalendar_conn;

static void gcalendarQueryCB_v(char *name, uip_ipaddr_t *ipaddr);
static void gcalendarMain_v(void);

char currentElement_ac[80];
char text_ac[120];
uint8_t textPos_ui8 = 0;

bool gweatherGetAttribute_b(char* inStr_pc, uint8_t inLen_ui8, char* outStr_pc, uint8_t outLen_ui8)
{
	int8_t pos1_si8 = -1;
	int8_t pos2_si8 = -1;
	uint8_t size_ui8 = 0;
	uint8_t seekPos_ui8 = 0;

	/* Seek for text within "" */
	while (++seekPos_ui8 < inLen_ui8)
	{
		if (inStr_pc[seekPos_ui8] == '"')
		{
			pos1_si8 = seekPos_ui8;
			break;
		}
	}

	seekPos_ui8 = inLen_ui8;
	while (seekPos_ui8-- > 0)
	{
		if (inStr_pc[seekPos_ui8] == '"')
		{
			pos2_si8 = seekPos_ui8;
			break;
		}
	}

	/* Text found ? */
	if ((pos1_si8 != -1) && (pos2_si8 != -1) && (pos2_si8 > pos1_si8) && (outLen_ui8 > 0))
	{
		/* Copy text */
		size_ui8 = (pos2_si8 - pos1_si8) - 1;

		if (size_ui8 >= outLen_ui8)
			size_ui8 = outLen_ui8 - 1;

		if (size_ui8 > 0)
		{
			memcpy(outStr_pc, inStr_pc + (pos1_si8 + 1), size_ui8);
			outStr_pc[size_ui8 + 1] = 0;
		}
	}
	else
	{
		printf("Could not get Attribute: %s\n", inStr_pc);
	}

	return ((size_ui8 > 0) ? true : false);
}

bool gcalendarParse_b(char* data_pc, uint16_t len_ui16)
{
	uint8_t pos_ui8 = 0;
	bool result_b = false;

	while (pos_ui8 < len_ui16)
	{
		switch (STATE->elementParserState_e)
		{
			case ELEMPARSER_WAIT_BEGIN:
			  if (data_pc[pos_ui8] == '<')
			  {
				  memset(currentElement_ac, 0, sizeof(currentElement_ac));
				  STATE->elementParserState_e = ELEMPARSER_IN_ELEMENT;
				  STATE->elemPos_ui8 = 0;
			  }
			break;

			case ELEMPARSER_IN_ELEMENT:
				if (data_pc[pos_ui8] != '>')
				{
					if (STATE->elemPos_ui8 < sizeof(currentElement_ac))
					{
						currentElement_ac[STATE->elemPos_ui8] = data_pc[pos_ui8];
						STATE->elemPos_ui8++;
					}
				}
				else
				{
					STATE->elementParserState_e = ELEMPARSER_DONE;
				}
			break;

			case ELEMPARSER_GET_LABEL:
				if (data_pc[pos_ui8] != '<')
				{
					if (STATE->elemPos_ui8 < sizeof(currentElement_ac))
					{
						currentElement_ac[STATE->elemPos_ui8] = data_pc[pos_ui8];
						STATE->elemPos_ui8++;
					}
				}
				else
				{
					STATE->elementParserState_e = ELEMPARSER_DONE;
					pos_ui8--;
				}
			break;

			case ELEMPARSER_DONE:
				STATE->elementParserState_e = ELEMPARSER_WAIT_BEGIN;
				pos_ui8--; // Reparse the current character;
			break;
		}

		if (ELEMPARSER_DONE == STATE->elementParserState_e)
		{
			switch(STATE->parserState_e)
			{
				case PARSER_WAIT_TAG:
				 if (strcmp(currentElement_ac, ENTRY_TAG) == 0)
				 {
					STATE->parserState_e = PARSER_IN_ENTRY;
//					printf("Entry Tag: %s\n", currentElement_ac);
					STATE->fcPos_ui8 = 0;
				 }
				 else
				 if (strcmp(currentElement_ac, "/feed") == 0)
				 {
					STATE->parserState_e = PARSER_DONE;
//					printf("End Tag: %s\n", currentElement_ac);
					STATE->fcPos_ui8 = 0;
				 }
				break;

				case PARSER_IN_ENTRY:
				 if (strstr(currentElement_ac, TITLE_TAG) != 0)
				 {
//					 printf("Ttitle Tag: %s\n", currentElement_ac);
					 STATE->parserState_e = PARSER_IN_TITLE;
					 STATE->elementParserState_e = ELEMPARSER_GET_LABEL;
 				     memset(currentElement_ac, 0, sizeof(currentElement_ac));
 				    STATE->elemPos_ui8 = 0;
				 }
				 else
				 if (strcmp(currentElement_ac, "/gd:when") == 0)
				 {
//					 printf("End when Tag: %s\n", currentElement_ac);
				 }
				 else
				 if (strstr(currentElement_ac, WHEN_TAG) != 0)
				 {
//					 printf("When Tag: %s\n", currentElement_ac);
					 memcpy(text_ac+textPos_ui8, currentElement_ac+17, 10);
					 textPos_ui8 += 10;
					 text_ac[textPos_ui8] = ' ';
					 textPos_ui8++;
					 memcpy(text_ac+textPos_ui8, currentElement_ac+28, 8);
					 textPos_ui8 += 8;
					 text_ac[textPos_ui8] = '\n';
					 textPos_ui8++;
					 text_ac[textPos_ui8] = '\n';
					 textPos_ui8++;
				 }
				 else
				 if (strcmp(currentElement_ac, "/entry") == 0)
				 {
	//				printf("End entry Tag: %s\n", currentElement_ac);
					STATE->parserState_e = PARSER_WAIT_TAG;
				 }
				 else
				 {
					 //printf("Unknown Tag: %s\n", currentElement_ac);
				 }
				break;

				case PARSER_IN_TITLE:
				 if (strcmp(currentElement_ac, "/title") == 0)
				 {
		//			printf("End title Tag: %s\n", currentElement_ac);
					STATE->parserState_e = PARSER_IN_ENTRY;
				 }
				 else
				 {
//					 printf("Title: %s\n", currentElement_ac);
					 memcpy(text_ac+textPos_ui8, currentElement_ac, STATE->elemPos_ui8);
					 textPos_ui8 += STATE->elemPos_ui8;
					 text_ac[textPos_ui8] = '\n';
					 textPos_ui8++;
				 }
				break;

				case PARSER_DONE:
				 printf("Done\n");
				 result_b = true;
				break;

				default:
					printf("Oops! Dunno what to do!\n");
				break;
			}
		}

		pos_ui8++;
	}

	return result_b;
}

void gcalendarSendRequest_v(void)
{
	uint16_t len_ui16;
	len_ui16 = sprintf_P(uip_sappdata, REQUEST, gCalendarLogin_ac);
	GCALENDARDEBUG("Send GET request(%i): %s\n", len_ui16, ((char*)uip_sappdata));
	uip_send(uip_sappdata, len_ui16);
}

static void gcalendarQueryCB_v(char *name, uip_ipaddr_t *ipaddr)
{
	if (NULL == ipaddr)
	{
		GCALENDARDEBUG ("Could not resolve address\n");
	}
	else
	{
		GCALENDARDEBUG ("Address resolved\n");
		gcalendar_conn = uip_connect(ipaddr, HTONS (80), gcalendarMain_v);

		if (NULL != gcalendar_conn)
		{
			GCALENDARDEBUG ("Wait for connect\n");
			STATE->stage_e = GCALENDAR_CONNECT;
		}
		else
		{
			GCALENDARDEBUG ("no uip_conn available.\n");
		}
	}
}

bool gcalendarSetLogin_b(char* login_pc, uint16_t len_ui16)
{
	if (len_ui16 > GCALENDAR_LOGINSIZE)
		return false;

	memcpy(login_pc, gCalendarLogin_ac, len_ui16);

#ifdef GCALENDAR_EEPROM_SUPPORT
	eeprom_save(gCalendarLogin_ac, gCalendarLogin_ac, GCALENDAR_LOGINSIZE);
	eeprom_update_chksum();
#endif

	return true;
}

int16_t gcalendarUpdate_i16(char *cmd_pc, char *output_pc, uint16_t len_ui16)
{
	uip_ipaddr_t* ip_p;

	GCALENDARDEBUG ("updating.\n");

	ip_p = resolv_lookup(GCALENDAR_HOST);

	if (NULL == ip_p)
	{
		GCALENDARDEBUG ("Resolving Address\n");
		resolv_query(GCALENDAR_HOST, gcalendarQueryCB_v);
	}
	else
	{
		GCALENDARDEBUG ("address already resolved\n");
		gcalendar_conn = uip_connect(ip_p, HTONS(80), gcalendarMain_v);

		if (NULL == gcalendar_conn)
		{
			GCALENDARDEBUG ("no uip_conn available.\n");
		}
		else
		{
			GCALENDARDEBUG ("Wait for connect\n");
			STATE->stage_e = GCALENDAR_CONNECT;
		}

		return ECMD_FINAL_OK;
	}

	return ECMD_FINAL_OK;
}

static void gcalendarMain_v(void)
{
	if (uip_aborted() || uip_timedout())
	{
		GCALENDARDEBUG ("connection aborted\n");
		gcalendar_conn = NULL;
	}

	if (uip_closed())
	{
		GCALENDARDEBUG("connection closed\n");
		gcalendar_conn = NULL;
		glcdmenuRedraw();
	}

	if (uip_connected() && STATE->stage_e == GCALENDAR_CONNECT)
	{
		GCALENDARDEBUG("Connected\n");
		STATE->stage_e = GCALENDAR_SEND_REQUEST;
	}

	if (uip_rexmit() && (STATE->stage_e == GCALENDAR_SEND_REQUEST
			|| STATE->stage_e == GCALENDAR_WAIT_RESPONSE))
	{
		GCALENDARDEBUG("Re-Xmit\n");
		gcalendarSendRequest_v();
		STATE->stage_e = GCALENDAR_WAIT_RESPONSE;
	}

	if (uip_poll() && (STATE->stage_e == GCALENDAR_SEND_REQUEST))
	{
		gcalendarSendRequest_v();
		STATE->stage_e = GCALENDAR_WAIT_RESPONSE;
	}

	if (uip_acked() && STATE->stage_e == GCALENDAR_WAIT_RESPONSE)
	{
		GCALENDARDEBUG("Request ACKed\n");
		STATE->stage_e = GCALENDAR_RECEIVE;
		STATE->parserState_e = PARSER_WAIT_START;
		STATE->elementParserState_e = ELEMPARSER_WAIT_BEGIN;

		memset(text_ac, 0, sizeof(text_ac));
		//glcdmenuSetString(MENU_TEXT_W_CITY, (unsigned char*) city_ac);
	}

	if (uip_newdata())
	{
		if (uip_len && STATE->stage_e == GCALENDAR_RECEIVE)
		{
			GCALENDARDEBUG("New Data\n");
			if (gcalendarParse_b(((char *) uip_appdata), uip_len) == false)
			{
				GCALENDARDEBUG("Parser error\n");
				uip_close (); /* Parse error */
				return;
			}
		}
	}
}

void gcalendarInit_v(void)
{
	GCALENDARDEBUG("initializing google calendar client\n");

#ifdef GCALENDAR_EEPROM_SUPPORT
	eeprom_restore(gCalendarLogin_ac, &gCalendarLogin_ac, GCALENDAR_LOGINSIZE);
#else
	sprintf(gCalendarLogin_ac, PSTR("%s"), CONF_GCALENDAR_LOGIN);
#endif
}

/*
 -- Ethersex META --
 header(services/gcalendar/gcalendar.h)
 net_init(gcalendarInit_v)
 state_header(services/gcalendar/gcalendar_state.h)
 state_tcp(struct gcalendar_connection_state_t gcalendar)
 */

/* EOF */
