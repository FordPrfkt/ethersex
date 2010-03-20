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

#ifndef HAVE_GWEATHER_STATE_H
#define HAVE_GWEATHER_STATE_H

enum {
    GWEATHER_INIT,
    GWEATHER_OPEN_CONN,
    GWEATHER_SEND_REQUEST,
    GWEATHER_WAIT_RESPONSE,
    GWEATHER_RECEIVE,
    GWEATHER_CLOSE_CONN
};

#include <inttypes.h>
#include "protocols/ecmd/via_tcp/ecmd_state.h"

struct gweather_connection_state_t {
    uint8_t stage;
};

#endif  /* HAVE_GWEATHER_STATE_H */
/* EOF */

