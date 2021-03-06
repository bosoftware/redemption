/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2013
 *   Author(s): Christophe Grosjean, Xiaopeng Zhou, Jonathan Poelen
 */

#include "mod/internal/internal_mod.hpp"
#include "core/front_api.hpp"
#include "core/channel_list.hpp"


InternalMod::InternalMod(
    FrontAPI & front, uint16_t front_width, uint16_t front_height,
    Font const & font, Theme const & theme, bool dont_resize
)
    : front_width(front_width)
    , front_height(front_height)
    , front(front)
    , screen(front, font, nullptr, theme)
{

    this->screen.set_wh(front_width, front_height);

    if (!dont_resize) {
        this->front.server_resize(front_width, front_height, 24);
    }
}

void InternalMod::send_to_front_channel(
    CHANNELS::ChannelNameId mod_channel_name,
    uint8_t const * data, size_t length, size_t chunk_size, int flags)
{
    const CHANNELS::ChannelDef * front_channel =
        this->front.get_channel_list().get_by_name(mod_channel_name);
    if (front_channel) {
        this->front.send_to_channel(*front_channel, data, length, chunk_size, flags);
    }
}
