/*
 * Copyright (c) 2010 by Daniel Walter <fordprfkt@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifndef HAVE_GOOGLESERVICES_SHARED_H
#define HAVE_GOOGLESERVICES_SHARED_H
#include "googleservices.h"
#include <stdbool.h>

#define GSERVICES_HOST "www.google.de"

/* Pointer to callout into the services */
typedef struct {
	void(*init_v)(void);					/* Init after reset */
	uint16_t(*getRequestString)(char[]);	/* Get request string for service data */
	void(*endReceive_v)(void);				/* All data received */
	void(*beginReceive_v)(void);			/* Start of incoming data */
	bool(*parse_b)(char*, uint16_t);		/* Parser for incoming data */
}gServiceFunctions_t;

bool gservicesUpdate_b(gservicesServiceTypes_t serviceType_e);

#endif  /* HAVE_GOOGLESERVICES_SHARED_H */
/* EOF */
