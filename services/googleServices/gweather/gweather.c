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
#include "../googleservices.h"
#include "../googleservices_shared.h"
#include "protocols/ecmd/parser.h"
#include "protocols/ecmd/ecmd-base.h"

#include "../../glcdmenu/glcdmenu.h"
#include "../../glcdmenu/menu-interpreter/menu-interpreter-config.h"

typedef enum {
	PARSER_WAIT_START,
	PARSER_WAIT_TAG,
	PARSER_IN_FORECAST,
	PARSER_IN_CURRENTCOND,
	PARSER_IN_FORECASTCOND,
	PARSER_DONE
}gweatherParserState_t;

typedef enum {
	ELEMPARSER_WAIT_BEGIN,
	ELEMPARSER_IN_ELEMENT,
	ELEMPARSER_DONE
}gweatherElemParserState_t;

static gweatherParserState_t parserState_e;
static gweatherElemParserState_t elementParserState_e;
static uint8_t elemPos_ui8;
static uint8_t fcPos_ui8;

static char city_ac[31];
static char date_ac[11];
static char condition_ac[21];
static char temperature_ac[5];
static char humidity_ac[21];
static char wind_ac[51];
static gWeatherForecast_t forecast_as[FC_NUM_ELEM];
static char currentElement_ac[80];
static char gweatherCity_ac[GWEATHER_CITYSIZE];

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
				"GET /ig/api?weather=%s HTTP/1.1\nHost: "GSERVICES_HOST"\nConnection: Close\r\n\r\n";

const gServiceFunctions_t functionPtrs_s = {
	gweatherGetRequestString_v,
	gweatherEndReceive_v,
	gweatherBeginReceive_v,
	gweatherParse_b
};

bool gweatherGetAttribute_b(char* inStr_pc, uint8_t inLen_ui8, char* outStr_pc,
		uint8_t outLen_ui8)
{
	int16_t pos1 = -1;
	int16_t pos2 = -1;
	uint8_t size_ui8 = 0;
	uint8_t seekPos_ui8 = 0;

	/* Seek for text within "" */
	while (++seekPos_ui8 < inLen_ui8)
	{
		if (inStr_pc[seekPos_ui8] == '"')
		{
			pos1 = seekPos_ui8;
			break;
		}
	}

	seekPos_ui8 = inLen_ui8;
	while (seekPos_ui8-- > 0)
	{
		if (inStr_pc[seekPos_ui8] == '"')
		{
			pos2 = seekPos_ui8;
			break;
		}
	}

	/* Text found ? */
	if ((pos1 != -1) && (pos2 != -1) && (pos2 > pos1) && (outLen_ui8 > 0))
	{
		/* Copy text */
		size_ui8 = (pos2 - pos1) - 1;

		if (size_ui8 >= outLen_ui8)
			size_ui8 = outLen_ui8 - 1;

		if (size_ui8 > 0)
		{
			memcpy(outStr_pc, inStr_pc + (pos1 + 1), size_ui8);
			outStr_pc[size_ui8 + 1] = 0;
		}
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
		switch (elementParserState_e)
		{
		case ELEMPARSER_WAIT_BEGIN: /* Seek for tag opening "<" */
			if (data_pc[pos_ui16] == '<')
			{
				/* Found. Begin copy element */
				memset(currentElement_ac, 0, sizeof(currentElement_ac));
				elementParserState_e = ELEMPARSER_IN_ELEMENT;
				elemPos_ui8 = 0;
			}
			break;

		case ELEMPARSER_IN_ELEMENT:
			if (data_pc[pos_ui16] != '>')
			{
				/* Copy element content until closing tag ">" into currentElement_ac */
				if (elemPos_ui8 < sizeof(currentElement_ac) - 1)
				{
					currentElement_ac[elemPos_ui8] = data_pc[pos_ui16];
					elemPos_ui8++;
				}
			}
			else
			{
				/* Closing tag found */
				elementParserState_e = ELEMPARSER_DONE;
			}
			break;

		case ELEMPARSER_DONE:
			/* Wait for next tag */
			elementParserState_e = ELEMPARSER_WAIT_BEGIN;
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
		if (ELEMPARSER_DONE == elementParserState_e)
		{
			switch (parserState_e)
			{
			case PARSER_WAIT_START:
				/* Wait for content start */
				if (strstr_P(currentElement_ac, WEATHER_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: Content start\n");
					parserState_e = PARSER_WAIT_TAG;
					fcPos_ui8 = 0;
				}
				break;

			case PARSER_WAIT_TAG:
				/* Wait for section begin */

				if (strstr_P(currentElement_ac, FORECAST_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: Forecast tag\n");
					/* Forecast information: City, etc. */
					parserState_e = PARSER_IN_FORECAST;
				}
				else if (strstr_P(currentElement_ac, CURRENT_COND_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: Current cond. tag\n");
					/* Current condition */
					parserState_e = PARSER_IN_CURRENTCOND;
				}
				else if (strstr_P(currentElement_ac, FORECASTCOND_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: Forecast cond. tag\n");
					/* forecast condition for the next days, this block
					 * is repeated in the document */
					if (fcPos_ui8 < FC_NUM_ELEM)
					{
						/* While we have space for new data */
						parserState_e = PARSER_IN_FORECASTCOND;
					}
				}
				else if (strstr_P(currentElement_ac, WEATHER_TAG) != 0)
				{
					GWEATHERDEBUG("Parser: End tag\n");
					/* Encountered the opening tag again. End of document */
					parserState_e = PARSER_DONE;
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
							elemPos_ui8, city_ac, sizeof(city_ac));
				}
				else if (strstr_P(currentElement_ac, FORECAST_DATE_TAG) != 0)
				{
					/* Condition tag */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							elemPos_ui8, date_ac, sizeof(date_ac));
				}
				else if (strstr_P(currentElement_ac, FORECAST_TAG) != 0)
				{
					/* Closing tag found. Wait for next block. */
					parserState_e = PARSER_WAIT_TAG;
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
							elemPos_ui8, wind_ac, sizeof(wind_ac));
				}
				else if (strstr_P(currentElement_ac, CONDITION_TAG) != 0)
				{
					/* Condtion */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							elemPos_ui8, condition_ac,
							sizeof(condition_ac));
				}
				else if (strstr_P(currentElement_ac, TEMP_TAG) != 0)
				{
					/* Temperature */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							elemPos_ui8, temperature_ac,
							sizeof(temperature_ac));
				}
				else if (strstr_P(currentElement_ac, HUMIDITY_TAG) != 0)
				{
					/* Humidity */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							elemPos_ui8, humidity_ac,
							sizeof(humidity_ac));
				}
				else if (strstr_P(currentElement_ac, CURRENT_COND_TAG) != 0)
				{
					/* Closing tag found */
					parserState_e = PARSER_WAIT_TAG;
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
							elemPos_ui8,
							forecast_as[fcPos_ui8].dayOfWeek_ac,
							sizeof(forecast_as[fcPos_ui8].dayOfWeek_ac));
				}
				else if (strstr_P(currentElement_ac, LOW_TAG) != 0)
				{
					/* Low temperature */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							elemPos_ui8,
							forecast_as[fcPos_ui8].lowTemp_ac,
							sizeof(forecast_as[fcPos_ui8].lowTemp_ac));
				}
				else if (strstr_P(currentElement_ac, HIGH_TAG) != 0)
				{
					/* High temperature */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							elemPos_ui8,
							forecast_as[fcPos_ui8].highTemp_ac,
							sizeof(forecast_as[fcPos_ui8].highTemp_ac));
				}
				else if (strstr_P(currentElement_ac, CONDITION_TAG) != 0)
				{
					/* Weather condition */
					result_b = gweatherGetAttribute_b(currentElement_ac,
							elemPos_ui8,
							forecast_as[fcPos_ui8].condition_ac,
							sizeof(forecast_as[fcPos_ui8].condition_ac));
				}
				else if (strstr_P(currentElement_ac, FORECASTCOND_TAG) != 0)
				{
					/* Closing tag found. Next forecast? */
					fcPos_ui8++;
					parserState_e = PARSER_WAIT_TAG;
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

void gweatherBeginReceive_v(void)
{
	parserState_e = PARSER_WAIT_START;
	elementParserState_e = ELEMPARSER_WAIT_BEGIN;

	memset(forecast_as, 0, sizeof(forecast_as));
	memset(city_ac, 0, sizeof(city_ac));
	memset(date_ac, 0, sizeof(date_ac));
	memset(condition_ac, 0, sizeof(condition_ac));
	memset(temperature_ac, 0, sizeof(temperature_ac));
	memset(humidity_ac, 0, sizeof(humidity_ac));
	memset(wind_ac, 0, sizeof(wind_ac));
	glcdmenuSetString(MENU_TEXT_W_CITY, (unsigned char*) city_ac);
	glcdmenuSetString(MENU_TEXT_W_DATE, (unsigned char*) date_ac);
	glcdmenuSetString(MENU_TEXT_W_WIND, (unsigned char*) wind_ac);
	glcdmenuSetString(MENU_TEXT_W_COND, (unsigned char*) condition_ac);
	glcdmenuSetString(MENU_TEXT_W_TEMP, (unsigned char*) temperature_ac);
	glcdmenuSetString(MENU_TEXT_W_HUMID, (unsigned char*) humidity_ac);
	glcdmenuSetString(MENU_TEXT_W_DOW1,
			(unsigned char*) forecast_as[0].dayOfWeek_ac);
	glcdmenuSetString(MENU_TEXT_W_DOW2,
			(unsigned char*) forecast_as[1].dayOfWeek_ac);
	glcdmenuSetString(MENU_TEXT_W_DOW3,
			(unsigned char*) forecast_as[2].dayOfWeek_ac);
	glcdmenuSetString(MENU_TEXT_W_DOW4,
			(unsigned char*) forecast_as[3].dayOfWeek_ac);
	glcdmenuSetString(MENU_TEXT_W_FT1,
			(unsigned char*) forecast_as[0].lowTemp_ac);
	glcdmenuSetString(MENU_TEXT_W_FT2,
			(unsigned char*) forecast_as[0].highTemp_ac);
	glcdmenuSetString(MENU_TEXT_W_FT3,
			(unsigned char*) forecast_as[1].lowTemp_ac);
	glcdmenuSetString(MENU_TEXT_W_FT4,
			(unsigned char*) forecast_as[1].highTemp_ac);
	glcdmenuSetString(MENU_TEXT_W_FT5,
			(unsigned char*) forecast_as[2].lowTemp_ac);
	glcdmenuSetString(MENU_TEXT_W_FT6,
			(unsigned char*) forecast_as[2].highTemp_ac);
	glcdmenuSetString(MENU_TEXT_W_FT7,
			(unsigned char*) forecast_as[3].lowTemp_ac);
	glcdmenuSetString(MENU_TEXT_W_FT8,
			(unsigned char*) forecast_as[3].highTemp_ac);
	glcdmenuSetString(MENU_TEXT_W_FC1,
			(unsigned char*) forecast_as[1].condition_ac);
	glcdmenuSetString(MENU_TEXT_W_FC3,
			(unsigned char*) forecast_as[2].condition_ac);
	glcdmenuSetString(MENU_TEXT_W_FC4,
			(unsigned char*) forecast_as[3].condition_ac);
}

void gweatherEndReceive_v(void)
{
	glcdmenuRedraw();
}

uint16_t gweatherGetRequestString_v(char request_ac[])
{
	uint16_t len_ui16;
	len_ui16 = sprintf_P(request_ac, REQUEST, gweatherCity_ac);
	GWEATHERDEBUG("Request(%i): %s\n", len_ui16, ((char*)request_ac));
	return len_ui16;
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

void gweatherInit_v(void)
{
	GWEATHERDEBUG("initializing google weather client\n");

#ifdef GWEATHER_EEPROM_SUPPORT
	eeprom_restore(gweatherCity_ac, &gweatherCity_ac, GWEATHER_CITYSIZE);
#else
	sprintf(gweatherCity_ac, PSTR("%s"), CONF_GWEATHER_CITY);
#endif
}

int16_t gweatherUpdate_i16(char *cmd_pc, char *output_pc, uint16_t len_ui16)
{
	gservicesUpdate_b(&functionPtrs_s);
	return ECMD_FINAL_OK;
}

int16_t gweather_onrequest(char *cmd, char *output, uint16_t len)
{
	GWEATHERDEBUG ("main\n");
	// enter your code here

	return ECMD_FINAL_OK;
}

/* EOF */
