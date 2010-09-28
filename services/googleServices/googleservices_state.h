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

#ifndef HAVE_GOOGLESERVICE_STATE_H
#define HAVE_GOOGLESERVICE_STATE_H

#include "googleservices_shared.h"
#include "googleservices.h"

typedef enum {
	GSERVICE_IDLE,
    GSERVICE_CONNECT,
    GSERVICE_SEND_REQUEST,
    GSERVICE_WAIT_RESPONSE,
    GSERVICE_RECEIVE
}gServicesConnectionStage_t;

#include <inttypes.h>
#include "protocols/ecmd/via_tcp/ecmd_state.h"

struct gservices_connection_state_t{
	gServicesConnectionStage_t stage_e;
	gservicesServiceTypes_t service_e;
};

#endif  /* HAVE_GOOGLESERVICE_STATE_H */
/* EOF */
