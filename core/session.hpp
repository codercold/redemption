/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010-2012
   Author(s): Christophe Grosjean, Javier Caverni, Raphael Zhou, Meng Tan
*/

#ifndef _REDEMPTION_CORE_SESSION_HPP_
#define _REDEMPTION_CORE_SESSION_HPP_

#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>

#include <array>

#include "server.hpp"
#include "colors.hpp"
#include "stream.hpp"
#include "front.hpp"
#include "ssl_calls.hpp"
#include "rect.hpp"
#include "client_info.hpp"
#include "netutils.hpp"

#include "config.hpp"
#include "wait_obj.hpp"
#include "transport.hpp"
#include "bitmap.hpp"

#include "authentifier.hpp"

using namespace std;

enum {
    // before anything else : exchange of credentials
//    SESSION_STATE_RSA_KEY_HANDSHAKE,
    // initial state no module loaded, init not done
    SESSION_STATE_ENTRY,
    // no module loaded
    // init_done
    // login window destoyed if necessary
    // user clicked on OK to run module  or provided connection info on cmd line
    // but did not received credentials yet
    SESSION_STATE_WAITING_FOR_NEXT_MODULE,
    // a module is loaded and active but required some action
    // involving requesting remote context
    SESSION_STATE_WAITING_FOR_CONTEXT,
    // init_done, module loaded and running
    SESSION_STATE_RUNNING,
    // display dialog when connection is closed
    SESSION_STATE_CLOSE_CONNECTION,
    // disconnect session
    SESSION_STATE_STOP
};

struct Session {
    Inifile  * ini;
    uint32_t & verbose;

    int internal_state;
    long id;                     // not used

    Front * front;

    SessionManager * acl;

    UdevRandom gen;

    SocketTransport * ptr_auth_trans;
    wait_obj        * ptr_auth_event;

    Session(int sck, Inifile * ini)
            : ini(ini)
            , verbose(this->ini->debug.session)
            , acl(NULL)
            , ptr_auth_trans(NULL)
            , ptr_auth_event(NULL) {
        try {
            SocketTransport front_trans("RDP Client", sck, "", 0, this->ini->debug.front);
            wait_obj front_event(&front_trans);
            // Contruct auth_trans (SocketTransport) and auth_event (wait_obj)
            //  here instead of inside Sessionmanager

            this->ptr_auth_trans = NULL;
            this->ptr_auth_event = NULL;
            this->acl            = NULL;

            this->internal_state = SESSION_STATE_ENTRY;

            const bool enable_fastpath = true;
            const bool mem3blt_support = true;

            this->front = new Front( &front_trans, SHARE_PATH "/" DEFAULT_FONT_NAME, &this->gen
                                   , ini, enable_fastpath, mem3blt_support);

            ModuleManager mm(*this->front, *this->ini);
            BackEvent_t signal = BACK_EVENT_NONE;

            // Under conditions (if this->ini->video.inactivity_pause == true)
            PauseRecord pause_record(this->ini->video.inactivity_timeout);

            if (this->verbose) {
                LOG(LOG_INFO, "Session::session_main_loop() starting");
            }

            const time_t start_time = time(NULL);

            const timeval time_mark = { 3, 0 };

            bool run_session = true;

            constexpr std::array<unsigned, 4> timers{{ 30*60, 10*60, 5*60, 1*60, }};
            unsigned osd_state = timers.size();

            while (run_session) {
                unsigned max = 0;
                fd_set rfds;
                fd_set wfds;

                FD_ZERO(&rfds);
                FD_ZERO(&wfds);
                timeval timeout = time_mark;

                front_event.add_to_fd_set(rfds, max, timeout);
                if (this->front->capture) {
                    this->front->capture->capture_event.add_to_fd_set(rfds, max, timeout);
                }
                TODO("Looks like acl and mod can be unified into a common class, where events can happen");
                TODO("move ptr_auth_event to acl");
                if (this->acl) {
                    this->ptr_auth_event->add_to_fd_set(rfds, max, timeout);
                }
                mm.mod->get_event().add_to_fd_set(rfds, max, timeout);

                const bool has_pending_data =
                    (front_event.st->tls && SSL_pending(front_event.st->allocated_ssl));
                if (has_pending_data)
                    memset(&timeout, 0, sizeof(timeout));

                int num = select(max + 1, &rfds, &wfds, 0, &timeout);

                if (num < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    // Cope with EBADF, EINVAL, ENOMEM : none of these should ever happen
                    // EBADF: means fd has been closed (by me) or as already returned an error on another call
                    // EINVAL: invalid value in timeout (my fault again)
                    // ENOMEM: no enough memory in kernel (unlikely fort 3 sockets)

                    LOG(LOG_ERR, "Proxy data wait loop raised error %u : %s", errno, strerror(errno));
                    run_session = false;
                    continue;
                }

                time_t now = time(NULL);

                if (front_event.is_set(rfds) ||
                    (front_event.st->tls && SSL_pending(front_event.st->allocated_ssl))) {
                    try {
                        this->front->incoming(*mm.mod);
                    } catch (...) {
                        run_session = false;
                        continue;
                    };
                }

                try {
                    if (this->front->up_and_running) {
                        if (this->ini->video.inactivity_pause
                            && mm.connected
                            && this->front->capture) {
                            pause_record.check(now, *this->front);
                        }
                        // new value incomming from acl
                        if (this->ini->check_from_acl()) {
                            this->front->update_config(*this->ini);
                            mm.check_module();
                        }
                        // Process incoming module trafic
                        if (mm.mod->get_event().is_set(rfds)) {
                            mm.mod->draw_event(now);

                            if (mm.mod->get_event().signal != BACK_EVENT_NONE) {
                                signal = mm.mod->get_event().signal;
                                mm.mod->get_event().reset();
                            }
                        }
                        if (this->front->capture
                            && this->front->capture->capture_event.is_set(rfds)) {
                            this->front->periodic_snapshot();
                        }
                        // Incoming data from ACL, or opening acl
                        if (!this->acl) {
                            if (!mm.last_module) {
                                // acl never opened or closed by me (close box)
                                try {
                                    int client_sck = ip_connect(this->ini->globals.authip,
                                                                this->ini->globals.authport,
                                                                30,
                                                                1000,
                                                                this->ini->debug.auth);

                                    if (client_sck == -1) {
                                        LOG(LOG_ERR, "Failed to connect to authentifier");
                                        throw Error(ERR_SOCKET_CONNECT_FAILED);
                                    }

                                    this->ptr_auth_trans = new SocketTransport( "Authentifier"
                                                                                , client_sck
                                                                                , this->ini->globals.authip
                                                                                , this->ini->globals.authport
                                                                                , this->ini->debug.auth
                                                                                );
                                    this->ptr_auth_event = new wait_obj(this->ptr_auth_trans);
                                    this->acl = new SessionManager( *this->ini
                                                                  , *front
                                                                  , *this->ptr_auth_trans
                                                                  , start_time // proxy start time
                                                                  , now        // acl start time
                                                                  );
                                    osd_state = [&](uint32_t enddata) -> unsigned {
                                        if (!enddata || enddata <= start_time) {
                                            return timers.size();
                                        }
                                        unsigned i = timers.rend() - std::lower_bound(
                                            timers.rbegin(), timers.rend(), enddata - start_time
                                        );
                                        return i ? i-1 : 0;
                                    }(this->ini->context.end_date_cnx.get());
                                    signal = BACK_EVENT_NEXT;
                                }
                                catch (...) {
                                    mm.invoke_close_box("No authentifier available",signal, now);
                                }
                            }
                        }
                        else {
                            if (this->ptr_auth_event->is_set(rfds)) {
                                // acl received updated values
                                this->acl->receive();
                            }
                        }

                        {
                            const uint32_t enddate = this->ini->context.end_date_cnx.get();
                            if (enddate
                            && osd_state < timers.size()
                            && enddate - now <= timers[osd_state]
                            && mm.is_up_and_running()) {
                                std::string mes;
                                mes.reserve(128);
                                mes += TR("connection_closed", *this->ini);
                                mes += " : ";
                                mes += std::to_string((enddate - now + 30) / 60);
                                mes += TR("minutes", *this->ini);
                                mm.osd_message(mes);
                                ++osd_state;
                            }
                        }

                        if (this->acl) {
                            run_session = this->acl->check(mm, now, front_trans, signal);
                        }
                        else if (signal == BACK_EVENT_STOP) {
                            mm.mod->get_event().reset();
                            run_session = false;
                        }
                        if (mm.last_module) {
                            if (this->acl) {
                                delete this->acl;
                                this->acl = NULL;
                            }
                        }
                    }
                } catch (Error & e) {
                    LOG(LOG_INFO, "Session::Session exception = %d!\n", e.id);
                    time_t now = time(NULL);
                    mm.invoke_close_box(e.errmsg(), signal, now);
                };
            }
            if (mm.mod) {
                mm.mod->disconnect();
            }
            this->front->disconnect();
        }
        catch (const Error & e) {
            LOG(LOG_INFO, "Session::Session Init exception = %d!\n", e.id);
        }
        catch(...) {
            LOG(LOG_INFO, "Session::Session other exception in Init\n");
        }
        LOG(LOG_INFO, "Session::Client Session Disconnected\n");
        this->front->stop_capture();
    }

    ~Session() {
        delete this->front;
        if (this->acl) { delete this->acl; }
        if (this->ptr_auth_event) { delete this->ptr_auth_event; }
        if (this->ptr_auth_trans) { delete this->ptr_auth_trans; }
        // Suppress Session file from disk (original name with PID or renamed with session_id)
        if (!this->ini->context.session_id.get().is_empty()) {
            char new_session_file[256];
            snprintf( new_session_file, sizeof(new_session_file), "%s/session_%s.pid"
                    , PID_PATH , this->ini->context.session_id.get_cstr());
            unlink(new_session_file);
        }
        else {
            int child_pid = getpid();
            char old_session_file[256];
            sprintf(old_session_file, "%s/session_%d.pid", PID_PATH, child_pid);
            unlink(old_session_file);
        }
    }
};

#endif
