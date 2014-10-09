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
    Author(s): Christophe Grosjean, Raphael Zhou
*/

#ifndef _REDEMPTION_CAPTURE_TRANSPARENTPLAYER_HPP_
#define _REDEMPTION_CAPTURE_TRANSPARENTPLAYER_HPP_

#include "transport.hpp"
#include "transparentchunk.hpp"

class TransparentPlayer {
private:
    Transport * t;
    FrontAPI  * consumer;

    bool meta_ok;

    timeval record_now;
    timeval replay_now;

public:
    TransparentPlayer(Transport * t, FrontAPI * consumer)
    : t(t), consumer(consumer), meta_ok(false) {
        while (this->meta_ok) {
            this->interpret_chunk();
        }
    }

    bool interpret_chunk(bool real_time = true) {
        try {
            BStream header(TRANSPARENT_CHUNT_HEADER_SIZE);
            this->t->recv(&header.end, TRANSPARENT_CHUNT_HEADER_SIZE);

            uint8_t  chunk_type = header.in_uint8();
            uint16_t data_size  = header.in_uint16_le();

            timeval last_record_now = this->record_now;
            header.in_timeval_from_uint64le_usec(this->record_now);

            //LOG(LOG_INFO, "chunk_type=%u data_size=%u", chunk_type, data_size);

            Array array(65535);
            uint8_t * end = array.get_data();
            this->t->recv(&end, data_size);
            InStream payload(array, 0, 0, end - array.get_data());

            switch (chunk_type) {
                case CHUNK_TYPE_META:
                    {
                        uint8_t trm_format_version = payload.in_uint8();
(void)trm_format_version;

                        this->replay_now = tvtime();
                        this->meta_ok    = true;
                    }
                    break;
                case CHUNK_TYPE_FASTPATH:
                    {
                        this->consumer->send_fastpath_data(payload);
                    }
                    break;
                case CHUNK_TYPE_FRONTCHANNEL:
                    {
                        uint8_t  mod_channel_name_length = payload.in_uint8();
                        uint16_t length                  = payload.in_uint16_le();
                        uint16_t chunk_size              = payload.in_uint16_le();
                        uint32_t flags                   = payload.in_uint32_le();

                        char mod_channel_name[256];
                        payload.in_copy_bytes(mod_channel_name, mod_channel_name_length);
                        mod_channel_name[mod_channel_name_length] = 0;

                        const CHANNELS::ChannelDef * front_channel =
                            this->consumer->get_channel_list().get_by_name(mod_channel_name);
                        if (front_channel) {
                            this->consumer->send_to_channel(*front_channel,
                                const_cast<uint8_t *>(payload.in_uint8p(length)), length, chunk_size, flags);
                        }
                    }
                    break;
                case CHUNK_TYPE_SLOWPATH:
                    {
                        uint16_t channelId = payload.in_uint16_le();

                        HStream data(1024, 65535);
                        size_t  length(data_size - sizeof(uint16_t));
                        data.out_copy_bytes(payload.in_uint8p(length), length);
                        data.mark_end();

                        this->consumer->send_data_indication_ex(channelId, data);
                    }
                    break;
                case CHUNK_TYPE_RESIZE:
                    {
                        uint16_t width  = payload.in_uint16_le();
                        uint16_t height = payload.in_uint16_le();
                        uint8_t  bpp    = payload.in_uint8();

                        this->consumer->server_resize(width, height, bpp);
                    }
                    break;
                default:
                    LOG(LOG_ERR, "Unknown chunt type(%d)", chunk_type);
                    throw Error(ERR_TRM_UNKNOWN_CHUNK_TYPE);
            }

            if (real_time && (chunk_type != CHUNK_TYPE_META)) {
                timeval  now     = tvtime();
                uint64_t elapsed = difftimeval(now, this->replay_now);

                this->replay_now = now;

                uint64_t record_elapsed = difftimeval(this->record_now, last_record_now);

                if (elapsed <= record_elapsed) {
                    struct timespec wtime     = {
                          static_cast<time_t>((record_elapsed - elapsed) / 1000000LL)
                        , static_cast<time_t>(((record_elapsed - elapsed) % 1000000LL) * 1000)
                        };
                    struct timespec wtime_rem = { 0, 0 };

                    while ((nanosleep(&wtime, NULL) == -1) && (errno == EINTR)) {
                        wtime = wtime_rem;
                    }
                }
            }
        }
        catch (Error & e){
            LOG(LOG_INFO,"receive error %u : end of transport", e.id);
            // receive error, end of transport
            return false;
        }

        return true;
    }
};

#endif  // #ifndef _REDEMPTION_CAPTURE_TRANSPARENTPLAYER_HPP_
