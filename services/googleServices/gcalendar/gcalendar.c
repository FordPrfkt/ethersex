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
#include "../googleservices.h"
#include "../googleservices_shared.h"
#include "protocols/ecmd/parser.h"
#include "protocols/ecmd/ecmd-base.h"

#include "../../glcdmenu/glcdmenu.h"
#include "../../glcdmenu/menu-interpreter/menu-interpreter-config.h"

enum
{
	PARSER_WAIT_TAG,
	PARSER_IN_ENTRY,
	PARSER_IN_TITLE,
	PARSER_DONE
};

enum
{
	ELEMPARSER_WAIT_BEGIN,
	ELEMPARSER_IN_ELEMENT,
	ELEMPARSER_GET_LABEL,
	ELEMPARSER_DONE
};

uint8_t parserState_e;
uint8_t elementParserState_e;
uint8_t elemPos_ui8;
uint8_t fcPos_ui8;
char currentElement_ac[80];
char text_ac[120];
uint8_t textPos_ui8 = 0;

static const char PROGMEM FEED_TAG[] = "feed";
static const char PROGMEM ENTRY_TAG[] = "entry";
static const char PROGMEM TITLE_TAG[] = "title type='text'";
static const char PROGMEM WHEN_TAG[] = "gd:when";

static const char
		PROGMEM REQUEST[] =
				"GET /calendar/feeds/%s/full HTTP/1.1\nHost: "GSERVICES_HOST"\nConnection: Close\r\n\r\n";

bool gcalendarParse_b(char* data_pc, uint16_t len_ui16)
{
	uint16_t pos_ui16 = 0;
	bool result_b = true;

	while (pos_ui16 < len_ui16)
	{
		switch (elementParserState_e)
		{
		case ELEMPARSER_WAIT_BEGIN:
			if (data_pc[pos_ui16] == '<')
			{
				memset(currentElement_ac, 0, sizeof(currentElement_ac));
				elementParserState_e = ELEMPARSER_IN_ELEMENT;
				elemPos_ui8 = 0;
			}
			break;

		case ELEMPARSER_IN_ELEMENT:
			if (data_pc[pos_ui16] != '>')
			{
				if (elemPos_ui8 < sizeof(currentElement_ac))
				{
					currentElement_ac[elemPos_ui8] = data_pc[pos_ui16];
					elemPos_ui8++;
				}
			}
			else
			{
				elementParserState_e = ELEMPARSER_DONE;
			}
			break;

		case ELEMPARSER_GET_LABEL:
			if (data_pc[pos_ui16] != '<')
			{
				if (elemPos_ui8 < sizeof(currentElement_ac))
				{
					currentElement_ac[elemPos_ui8] = data_pc[pos_ui16];
					elemPos_ui8++;
				}
			}
			else
			{
				elementParserState_e = ELEMPARSER_DONE;
				pos_ui16--;
			}
			break;

		case ELEMPARSER_DONE:
			elementParserState_e = ELEMPARSER_WAIT_BEGIN;
			pos_ui16--; // Reparse the current character;
			break;

		default:
			/* Unknown state  */
			GCALENDARDEBUG("Oops! Dunno what to do!\n")
			;
			result_b = false;
			break;
		}

		if (ELEMPARSER_DONE == elementParserState_e)
		{
			switch (parserState_e)
			{
			case PARSER_WAIT_TAG:
				if (strcmp_P(currentElement_ac, ENTRY_TAG) == 0)
				{
					GCALENDARDEBUG("Parser: Entry tag\n");
					parserState_e = PARSER_IN_ENTRY;
					fcPos_ui8 = 0;
				}
				else if (strcmp(currentElement_ac, "/feed") == 0)
				{
					GCALENDARDEBUG("Parser: End feed tag\n");
					parserState_e = PARSER_DONE;
					fcPos_ui8 = 0;
				}
				break;

			case PARSER_IN_ENTRY:
				if (strstr_P(currentElement_ac, TITLE_TAG) != 0)
				{
					parserState_e = PARSER_IN_TITLE;
					elementParserState_e = ELEMPARSER_GET_LABEL;
					memset(currentElement_ac, 0, sizeof(currentElement_ac));
					elemPos_ui8 = 0;
				}
				else if (strcmp(currentElement_ac, "/gd:when") == 0)
				{
					GCALENDARDEBUG("Parser: End when tag\n");
				}
				else if (strstr_P(currentElement_ac, WHEN_TAG) != 0)
				{
					GCALENDARDEBUG("Parser: When tag\n");
					memcpy(text_ac + textPos_ui8, currentElement_ac + 17, 10);
					textPos_ui8 += 10;
					text_ac[textPos_ui8] = ' ';
					textPos_ui8++;
					memcpy(text_ac + textPos_ui8, currentElement_ac + 28, 8);
					textPos_ui8 += 8;
					text_ac[textPos_ui8] = '\n';
					textPos_ui8++;
					text_ac[textPos_ui8] = '\n';
					textPos_ui8++;
				}
				else if (strcmp(currentElement_ac, "/entry") == 0)
				{
					GCALENDARDEBUG("Parser: End when tag\n");
					parserState_e = PARSER_WAIT_TAG;
				}
				else
				{
					GCALENDARDEBUG("Parser: Ignored tag\n");
				}
				break;

			case PARSER_IN_TITLE:
				if (strcmp(currentElement_ac, "/title") == 0)
				{
					GCALENDARDEBUG("Parser: End title tag\n");
					parserState_e = PARSER_IN_ENTRY;
				}
				else
				{
					memcpy(text_ac + textPos_ui8, currentElement_ac,
							elemPos_ui8);
					textPos_ui8 += elemPos_ui8;
					text_ac[textPos_ui8] = '\n';
					textPos_ui8++;
					GCALENDARDEBUG("Parser: Title", text_ac);
				}
				break;

			case PARSER_DONE:
				GCALENDARDEBUG("Parser: Done %s\n", text_ac);
				break;

			default:
				GCALENDARDEBUG("Oops! Dunno what to do!\n");
				result_b = false;
				break;
			}
		}

		pos_ui16++;
	}

	return result_b;
}

void gcalendarBeginReceive_v(void)
{
	parserState_e = PARSER_WAIT_TAG;
	elementParserState_e = ELEMPARSER_WAIT_BEGIN;

	memset(text_ac, 0, sizeof(text_ac));

	glcdmenuSetString(MENU_TEXT_TI_LIST, (unsigned char*) text_ac);
}

void gcalendarEndReceive_v(void)
{
	glcdmenuRedraw();
}

uint16_t gcalendarGetRequestString_v(char request_ac[])
{
	uint16_t len_ui16;
	len_ui16 = sprintf_P(request_ac, REQUEST, gCalendarLogin_ac);
	GCALENDARDEBUG("Request(%i): %s\n", len_ui16, ((char*)request_ac));
	return len_ui16;
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
	gservicesUpdate_b(GCALENDAR_SERVICE);
	return ECMD_FINAL_OK;
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

/* EOF */
