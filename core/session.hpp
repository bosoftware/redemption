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
   Author(s): Christophe Grosjean, Javier Caverni
   Based on xrdp Copyright (C) Jay Sorg 2004-2010

*/

#if !defined(__SESSION_HPP__)
#define __SESSION_HPP__

#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <string.h>

#include "colors.hpp"
#include "stream.hpp"
#include "constants.hpp"
#include "ssl_calls.hpp"
#include "file_loc.hpp"
#include "rect.hpp"
#include "client_info.hpp"

#include "config.hpp"
#include "wait_obj.hpp"
#include "transport.hpp"
#include "bitmap.hpp"
#include "modcontext.hpp"

#include "keymap.hpp"
#include "callback.hpp"
#include "sesman.hpp"

using namespace std;

enum {
    // before anything else : exchange of credentials
    SESSION_STATE_RSA_KEY_HANDSHAKE,
    // initial state no module loaded, init not done
    SESSION_STATE_ENTRY,
    // no module loaded
    // init_done
    // login window destoyed if necessary
    // user clicked on OK to run module  or provided connection info on cmd line
    // but did not received credentials yet
    SESSION_STATE_WAITING_FOR_NEXT_MODULE,
    // init_done, module loaded and running
    SESSION_STATE_RUNNING,
    // display dialog when connection is closed
    SESSION_STATE_CLOSE_CONNECTION,
    // disconnect session
    SESSION_STATE_STOP,
};

struct Session {

    struct SessionCallback : public Callback
    {
        Session & session;
        SessionCallback(Session & session) : session(session)
        {
        }

        virtual int callback(int msg, long param1, long param2, long param3, long param4)
        {
            return this->session.callback(msg, param1, param2, param3, param4);
        }
    } * session_callback;


    ModContext * context;
    int internal_state;
    long id;
    struct SocketTransport * trans;
    time_t keep_alive_time;

    int sck;
    wait_obj * front_event;
    wait_obj * back_event;

    Inifile * ini;

    struct client_mod * mod; /* module interface */
    struct client_mod * no_mod;

    struct Front* front;
    #warning caches are related to rdp client. Put them in rdp.hpp or client_mod
    struct Cache* cache;
    int mouse_x;
    int mouse_y;

    #warning move this to screen
    /* keyboard info */
    int keys[256]; /* key states 0 up 1 down*/
    int key_flags; // scrool_lock = 1, num_lock = 2, caps_lock = 4
    struct Keymap * keymap;

    struct Font* default_font;

    SessionManager * sesman;

    Session(int sck, const char * ip_source, Inifile * ini);
    ~Session();
    int pointer(char* data, char* mask, int x, int y);
    void invalidate(const Rect & rect);

    int session_main_loop();
    int callback(int msg, long param1, long param2, long param3, long param4);

    int step_STATE_KEY_HANDSHAKE(struct timeval & time);
    int step_STATE_ENTRY(struct timeval & time);
    int step_STATE_WAITING_FOR_NEXT_MODULE(struct timeval & time);
    int step_STATE_RUNNING(struct timeval & time);
    int step_STATE_CLOSE_CONNECTION();

    bool session_setup_mod(int next_state, const ModContext * context);


};

#endif
