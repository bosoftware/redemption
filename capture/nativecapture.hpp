/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010
   Author(s): Christophe Grosjean, Javier Caverni, Xavier Dunat, Martin Potier
*/

#if !defined(__CAPTURE_NATIVECAPTURE_HPP__)
#define __CAPTURE_NATIVECAPTURE_HPP__

#include <errno.h>
#include "rdtsc.hpp"
#include "bitmap.hpp"
#include "rect.hpp"
#include "constants.hpp"
#include <time.h>
#include "difftimeval.hpp"

#include "RDP/orders/RDPOrdersCommon.hpp"
#include "RDP/orders/RDPOrdersSecondaryColorCache.hpp"
#include "RDP/orders/RDPOrdersSecondaryBmpCache.hpp"

#include "RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"
#include "RDP/orders/RDPOrdersPrimaryScrBlt.hpp"
#include "RDP/orders/RDPOrdersPrimaryDestBlt.hpp"
#include "RDP/orders/RDPOrdersPrimaryMemBlt.hpp"
#include "RDP/orders/RDPOrdersPrimaryPatBlt.hpp"
#include "RDP/orders/RDPOrdersPrimaryLineTo.hpp"
#include "RDP/orders/RDPOrdersPrimaryGlyphIndex.hpp"

#include "error.hpp"
#include "config.hpp"
#include "RDP/caches/bmpcache.hpp"
#include "colors.hpp"
#include "stream.hpp"

#include "GraphicToFile.hpp"
#include "png.hpp"
#include "auto_buffer.hpp"
#include "cipher_transport.hpp"

#include "zlib.hpp"

class NativeCapture : public RDPGraphicDevice
{
public:
    int width;
    int height;
    int bpp;

    uint64_t frame_interval;
    struct timeval start_native_capture;
    uint64_t inter_frame_interval_native_capture;

    uint64_t break_interval;
    struct timeval start_break_capture;
    uint64_t inter_frame_interval_start_break_capture;

    BStream stream;
    BmpCache bmp_cache;
    GraphicToFile recorder;
    uint32_t nb_file;

    NativeCapture(const timeval & now, Transport & trans, int width, int height)
    : width(width)
    , height(height)
    , bpp(24)
    , stream(65536)
    , bmp_cache(24, 8192, 768, 8192, 3072, 8192, 12288)
    , recorder(now, &trans, &this->stream, NULL, width, height, 24, bmp_cache)
    , nb_file(0)
    {
        // frame interval is in 1/100 s, default value, 1 timestamp mark every 40/100 s
        this->start_native_capture = now;
        this->frame_interval = 40;
        this->inter_frame_interval_native_capture       =  this->frame_interval * 10000; // 1 000 000 us is 1 sec

        this->start_break_capture = now;
        this->break_interval = 60 * 10; // break interval is in s, default value 1 break every 10 minutes
        this->inter_frame_interval_start_break_capture  = 1000000 * this->break_interval; // 1 000 000 us is 1 sec

        LOG(LOG_INFO, "update configuration frame_interval=%u break_interval=%u",
            this->frame_interval, this->break_interval);
    }

    ~NativeCapture(){
        this->recorder.flush();
    }

    void update_config(const Inifile & ini)
    {
        if (ini.globals.frame_interval != this->frame_interval){
            // frame interval is in 1/100 s, default value, 1 timestamp mark every 40/100 s
            this->frame_interval = ini.globals.frame_interval;
            this->inter_frame_interval_native_capture       =  this->frame_interval * 10000; // 1 000 000 us is 1 sec
        }

        if (ini.globals.break_interval != this->break_interval){
            this->break_interval = ini.globals.break_interval; // break interval is in s, default value 1 break every 10 minutes
            this->inter_frame_interval_start_break_capture  = 1000000 * this->break_interval; // 1 000 000 us is 1 sec
        }
        LOG(LOG_INFO, "updated configuration frame_interval=%u break_interval=%u",
             this->frame_interval, this->break_interval);
    }
        
    TODO("pointer_already_displayed and no_timestamp are constants, not need to pass then at every snapshot call")
    void snapshot(const timeval & now, int x, int y, bool pointer_already_displayed, bool no_timestamp)
    {
        if (difftimeval(now, this->start_native_capture) >= this->inter_frame_interval_native_capture){
            LOG(LOG_INFO, "recorder timestamp");
            this->recorder.timestamp(now);
            this->start_native_capture = now;
            if (difftimeval(now, this->start_break_capture) >= this->inter_frame_interval_start_break_capture){
                this->breakpoint();
                this->start_break_capture = now;
            }
        }
        this->recorder.flush();

    }
    
    virtual void flush()
    {}

    virtual void draw(const RDPScrBlt & cmd, const Rect & clip)
    {
        this->recorder.draw(cmd, clip);
    }

    virtual void draw(const RDPMemBlt & cmd, const Rect & clip, const Bitmap & bmp)
    {
        this->recorder.draw(cmd, clip, bmp);
    }

    virtual void draw(const RDPOpaqueRect & cmd, const Rect & clip)
    {
        this->recorder.draw(cmd, clip);
    }

    virtual void draw(const RDPDestBlt & cmd, const Rect & clip)
    {
        this->recorder.draw(cmd, clip);
    }

    virtual void draw(const RDPPatBlt & cmd, const Rect & clip)
    {
        this->recorder.draw(cmd, clip);
    }

    virtual void draw(const RDPLineTo & cmd, const Rect & clip)
    {
        this->recorder.draw(cmd, clip);
    }

    virtual void draw(const RDPGlyphIndex & cmd, const Rect & clip)
    {
        this->recorder.draw(cmd, clip);
    }

private:

    void breakpoint()
    {
        this->recorder.breakpoint();
    }
};

#endif
