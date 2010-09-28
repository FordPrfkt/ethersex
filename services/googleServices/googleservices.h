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

#ifndef HAVE_GOOGLESERVICES_H
#define HAVE_GOOGLESERVICES_H

#include <stdbool.h>

typedef enum {
		GWEATHER_SERVICE,
		GCALENDAR_SERVICE,
		GSERVICES_NUM_SERVICES
}gservicesServiceTypes_t;

void gservicesInit_v(void);

#include "config.h"
#ifdef DEBUG_GSERVICE
  #include "core/debug.h"
  #define GSERVICESDEBUG(a...)  debug_printf("gService: "a);
#else
  #define GSERVICESDEBUG(a...)
#endif

#endif  /* HAVE_GOOGLESERVICES_H */

/* EOF */
