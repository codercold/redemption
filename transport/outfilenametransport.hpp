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
   Copyright (C) Wallix 2012
   Author(s): Christophe Grosjean

   Transport layer abstraction
*/

#ifndef _REDEMPTION_TRANSPORT_OUTFILENAMETRANSPORT_HPP_
#define _REDEMPTION_TRANSPORT_OUTFILENAMETRANSPORT_HPP_

#include "transport.hpp"
#include "rio/rio.h"

class OutFilenameTransport : public Transport {
public:
    SQ   seq;
    RIO  rio;
    char path[512];

    OutFilenameTransport(
            SQ_FORMAT format,
            const char * const prefix,
            const char * const filename,
            const char * const extension,
            const int groupid,
            auth_api * authentifier = NULL,
            unsigned verbose = 0)
    {
        if (authentifier) {
            this->set_authentifier(authentifier);
        }

        RIO_ERROR status1 = sq_init_outfilename(&this->seq, format, prefix, filename, extension, groupid);
        if (status1 != RIO_ERROR_OK){
            LOG(LOG_ERR, "Sequence outfilename initialisation failed (%u)", status1);
            throw Error(ERR_TRANSPORT);
        }
        RIO_ERROR status2 = rio_init_outsequence(&this->rio, &this->seq);
        if (status2 != RIO_ERROR_OK){
            LOG(LOG_ERR, "rio outsequence initialisation failed (%u)", status2);
            throw Error(ERR_TRANSPORT);
        }

        size_t max_path_length = sizeof(path) - 1;
        strncpy(this->path, prefix, max_path_length);
        this->path[max_path_length] = 0;
    }

    ~OutFilenameTransport()
    {
        rio_clear(&this->rio);
        if (this->full_cleaning_requested)
        {
            sq_full_clear(&this->seq);
        }
        else
        {
            sq_clear(&this->seq);
        }
    }

    using Transport::send;
    virtual void send(const char * const buffer, size_t len) throw (Error) {
        ssize_t res = rio_send(&this->rio, buffer, len);
        if (res < 0){
            if (errno == ENOSPC) {
                char message[1024];
                snprintf(message, sizeof(message), "100|%s", this->path);
                this->authentifier->report("FILESYSTEM_FULL", message);
            }

            LOG(LOG_INFO, "Write to transport failed (F): code=%d", errno);
            throw Error(ERR_TRANSPORT_WRITE_FAILED, errno);
        }
    }

    virtual void timestamp(timeval now)
    {
        sq_timestamp(&this->seq, &now);
        Transport::timestamp(now);
    }

    using Transport::recv;
    virtual void recv(char**, size_t) throw (Error)
    {
        LOG(LOG_ERR, "OutFilenameTransport used for recv");
        throw Error(ERR_TRANSPORT_OUTPUT_ONLY_USED_FOR_SEND, 0);
    }

    virtual void seek(int64_t offset, int whence) throw (Error)
    {
        ssize_t res = rio_seek(&this->rio, offset, whence);
        if (res != RIO_ERROR_OK){
            LOG(LOG_WARNING, "OutFilenameTransport::seek failed");
            throw Error(ERR_TRANSPORT_SEEK_FAILED, errno);
        }
    }

    virtual bool next()
    {
        sq_next(&this->seq);
        return Transport::next();
    }
};



/*****************************
* CryptoOutFilenameTransport *
*****************************/

class CryptoOutFilenameTransport : public Transport {
public:
    SQ   seq;
    RIO  rio;
    char path[512];

    CryptoOutFilenameTransport(
            CryptoContext * crypto_ctx,
            SQ_FORMAT format,
            const char * const prefix,
            const char * const filename,
            const char * const extension,
            const int groupid,
            auth_api * authentifier = NULL,
            unsigned verbose = 0)
    {
        if (authentifier) {
            this->set_authentifier(authentifier);
        }

        RIO_ERROR status1 = sq_init_cryptooutfilename(&this->seq, crypto_ctx, format, prefix, filename, extension, groupid);
        if (status1 != RIO_ERROR_OK){
            LOG(LOG_ERR, "Sequence outfilename initialisation failed (%u)", status1);
            throw Error(ERR_TRANSPORT);
        }
        RIO_ERROR status2 = rio_init_outsequence(&this->rio, &this->seq);
        if (status2 != RIO_ERROR_OK){
            LOG(LOG_ERR, "rio outsequence initialisation failed (%u)", status2);
            throw Error(ERR_TRANSPORT);
        }

        size_t max_path_length = sizeof(path) - 1;
        strncpy(this->path, prefix, max_path_length);
        this->path[max_path_length] = 0;
    }

    ~CryptoOutFilenameTransport()
    {
        rio_clear(&this->rio);
        sq_clear(&this->seq);
    }

    using Transport::send;
    virtual void send(const char * const buffer, size_t len) throw (Error) {
        ssize_t res = rio_send(&this->rio, buffer, len);
        if (res < 0){
            if (errno == ENOSPC) {
                char message[1024];
                snprintf(message, sizeof(message), "100|%s", this->path);
                this->authentifier->report("FILESYSTEM_FULL", message);
            }

            LOG(LOG_INFO, "Write to transport failed (CF): code=%d", errno);
            throw Error(ERR_TRANSPORT_WRITE_FAILED, errno);
        }
    }

    virtual void timestamp(timeval now)
    {
        sq_timestamp(&this->seq, &now);
        Transport::timestamp(now);
    }

    using Transport::recv;
    virtual void recv(char**, size_t) throw (Error)
    {
        LOG(LOG_INFO, "OutFilenameTransport used for recv");
        throw Error(ERR_TRANSPORT_OUTPUT_ONLY_USED_FOR_SEND, 0);
    }

    virtual void seek(int64_t offset, int whence) throw (Error) {
        throw Error(ERR_TRANSPORT_SEEK_NOT_AVAILABLE, errno);
    }

    virtual bool next()
    {
        sq_next(&this->seq);
        return Transport::next();
    }
};

#endif
