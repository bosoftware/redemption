/*
    This program is free software; you can redistribute it and/or modify it
     under the terms of the GNU General Public License as published by the
     Free Software Foundation; either version 2 of the License, or (at your
     option) any later version.

    This program is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
     Public License for more details.

    You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     675 Mass Ave, Cambridge, MA 02139, USA.

    Product name: redemption, a FLOSS RDP proxy
    Copyright (C) Wallix 2013
    Author(s): Christophe Grosjean, Meng Tan, Jonathan Poelen, Raphael Zhou
*/

#include "core/front_api.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMemBlt.hpp"
#include "core/RDP/orders/RDPOrdersSecondaryColorCache.hpp"
#include "mod/internal/widget_test_mod.hpp"
#include "keyboard/keymap2.hpp"
#include "utils/bitmap.hpp"
#include "utils/bitmap_private_data.hpp"
#include "utils/sugar/update_lock.hpp"

#include <cstring>


// Pimpl
class WidgetTestMod::WidgetTestModPrivate {};

WidgetTestMod::WidgetTestMod(
    FrontAPI & front, uint16_t width, uint16_t height, Font const & font)
: InternalMod(front, width, height, font, Theme{}, false)
{
    front.server_resize(width, height, 8);
}

WidgetTestMod::~WidgetTestMod()
{
    this->screen.clear();
}

void WidgetTestMod::rdp_input_invalidate(Rect /*r*/)
{}

void WidgetTestMod::rdp_input_mouse(int /*device_flags*/, int /*x*/, int /*y*/, Keymap2 * /*keymap*/)
{}

void WidgetTestMod::rdp_input_scancode(
    long /*param1*/, long /*param2*/, long /*param3*/, long /*param4*/, Keymap2 * keymap)
{
    if (keymap->nb_kevent_available() > 0
        && keymap->get_kevent() == Keymap2::KEVENT_ENTER) {
        this->event.signal = BACK_EVENT_STOP;
        this->event.set_trigger_time(wait_obj::NOW);
    }
}

void WidgetTestMod::refresh(Rect clip)
{
    this->rdp_input_invalidate(clip);
}

void WidgetTestMod::draw_event(time_t, gdi::GraphicApi& gd)
{
    update_lock<decltype(this->front)> update_lock{this->front};

    auto mono_palette = [&](BGRColor const& color) {
        BGRColor d[256];
        for (auto & c : d) {
            c = color;
        }
        return BGRPalette{d};
    };

    const auto clip = this->get_screen_rect();
    const auto color_ctx = gdi::ColorCtx::depth8(BGRPalette::classic_332());
    const auto encode_color = encode_color8();
    const auto cx = clip.cx / 2;
    const auto cy = clip.cy / 3;

//     auto send_mono_palette = [&](BGRColor const& color){
//         this->front.sync();
//         this->front.set_palette(mono_palette(color));
//     };

    auto draw_rect = [&](int x, int y, BGRColor color){
        this->front.draw(RDPOpaqueRect(Rect(x*cx, y*cy, cx, cy), encode_color(color)), clip, color_ctx);
    };

    auto draw_text = [&](int x, int y, char const* txt){
        gdi::server_draw_text(gd, this->font(), 10+x*cx, 10+y*cy, txt, encode_color(WHITE), encode_color(BLACK), color_ctx, clip);
    };

    auto draw_img = [&](int x, int y, int col, Bitmap const& bitmap){
        this->front.draw(RDPMemBlt(col, Rect(x*cx, y*cy, cx, cy), 0xCC, 0, 0, 0), clip, bitmap);
    };

    auto plain_img = [&](BGRColor const& color){
        Bitmap img;
        Bitmap::PrivateData::Data & data = Bitmap::PrivateData::initialize(img, 8, cx, cy);
        memset(data.get(), encode_color(color).as_bgr().to_u32(), data.line_size() * cy);
        //data.palette() = BGRPalette::classic_332();
        data.palette() = mono_palette(color);
        return img;
    };

    const auto img1 = plain_img(GREEN);
    const auto img2 = plain_img(ANTHRACITE);

    draw_rect(0, 0, BLUE);
    draw_rect(1, 0, RED);
    this->front.sync();
    draw_img(0, 1, 0, img1);
    draw_img(1, 1, 1, img2);
    this->front.sync();
    this->front.draw(RDPColCache(0, mono_palette(WHITE)));
    this->front.draw(RDPColCache(1, mono_palette(WHITE)));
    this->front.sync();
    const auto img3 = plain_img(DARK_WABGREEN);
    const auto img4 = plain_img(BLUE);
    draw_img(0, 2, 0, img3);
    draw_img(1, 2, 1, img4);
    draw_text(0, 0, "blue");
    draw_text(1, 0, "red");
    draw_text(0, 1, "cyan img");
    draw_text(1, 1, "pink img");
    draw_text(0, 2, "yellow palette");
    draw_text(1, 2, "yellow palette");

    this->event.set_trigger_time(std::chrono::seconds(3));
}
