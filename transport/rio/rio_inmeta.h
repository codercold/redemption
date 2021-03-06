/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARIO *ICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Raphael Zhou

   Template for new Inmeta RedTransport class
*/

#ifndef _REDEMPTION_TRANSPORT_RIO_RIO_INMETA_H_
#define _REDEMPTION_TRANSPORT_RIO_RIO_INMETA_H_

#include "rio.h"

extern "C" {
    struct RIOInmeta {
        struct SQ * seq;
        struct RIO * insequence;
    };

    /* This method does not allocate space for object itself,
        but initialize it's properties
        and allocate and initialize it's subfields if necessary
    */
    inline RIO_ERROR rio_m_RIOInmeta_constructor(RIOInmeta * self, SQ ** seq, const char * prefix, const char * extension)
    {
        RIO_ERROR status = RIO_ERROR_OK;
        SQ * sequence = sq_new_inmeta(&status, prefix, extension);

        if (status != RIO_ERROR_OK){
            return status;
        }
        RIO * rt = rio_new_insequence(&status, sequence);
        if (status != RIO_ERROR_OK){
            sq_delete(sequence);
            return status;
        }
        self->seq = *seq = sequence;
        self->insequence = rt;
        return RIO_ERROR_OK;
    }

    /* This method deallocate any space used for subfields if any
    */
    inline RIO_ERROR rio_m_RIOInmeta_destructor(RIOInmeta * self)
    {
        rio_delete(self->insequence);
        self->insequence = NULL;
        sq_delete(self->seq);
        self->seq = NULL;
        return RIO_ERROR_CLOSED;
    }

    /* This method return a signature based on the data written
    */
    static inline RIO_ERROR rio_m_RIOInmeta_sign(RIOInmeta * self, unsigned char * buf, size_t size, size_t * len) {
         rio_m_RIOInmeta_destructor(self);
         return RIO_ERROR_RECV_ONLY;
    }

    /* This method receive len bytes of data into buffer
       target buffer *MUST* be large enough to contains len data
       returns len actually received (may be 0),
       or negative value to signal some error.
       If an error occurs after reading some data the amount read will be returned
       and an error returned on subsequent call.
    */
    inline ssize_t rio_m_RIOInmeta_recv(RIOInmeta * self, void * data, size_t len)
    {
         return rio_recv(self->insequence, data, len);
    }

    /* This method send len bytes of data from buffer to current transport
       buffer must actually contains the amount of data requested to send.
       returns len actually sent (may be 0),
       or negative value to signal some error.
       If an error occurs after sending some data the amount sent will be returned
       and an error returned on subsequent call.
    */
    inline ssize_t rio_m_RIOInmeta_send(RIOInmeta * self, const void * data, size_t len)
    {
        return -RIO_ERROR_RECV_ONLY;
    }

    static inline RIO_ERROR rio_m_RIOInmeta_seek(RIOInmeta * self, int64_t offset, int whence)
    {
        return RIO_ERROR_SEEK_NOT_AVAILABLE;
    }

    static inline RIO_ERROR rio_m_RIOInmeta_get_status(RIOInmeta * self)
    {
        return rio_get_status(self->insequence);
    }



    /******************
    * RIOCryptoInmeta *
    ******************/

    struct RIOCryptoInmeta {
        struct SQ * seq;
        struct RIO * insequence;
    };

    /* This method does not allocate space for object itself,
        but initialize it's properties
        and allocate and initialize it's subfields if necessary
    */
    inline RIO_ERROR rio_m_RIOCryptoInmeta_constructor(RIOCryptoInmeta * self, SQ ** seq, CryptoContext * crypto_ctx, const char * prefix, const char * extension)
    {
        RIO_ERROR status = RIO_ERROR_OK;
        SQ * sequence = sq_new_cryptoinmeta(&status, crypto_ctx, prefix, extension);

        if (status != RIO_ERROR_OK){
            return status;
        }
        RIO * rt = rio_new_insequence(&status, sequence);
        if (status != RIO_ERROR_OK){
            sq_delete(sequence);
            return status;
        }
        self->seq = *seq = sequence;
        self->insequence = rt;
        return RIO_ERROR_OK;
    }

    /* This method deallocate any space used for subfields if any
    */
    inline RIO_ERROR rio_m_RIOCryptoInmeta_destructor(RIOCryptoInmeta * self)
    {
        rio_delete(self->insequence);
        self->insequence = NULL;
        sq_delete(self->seq);
        self->seq = NULL;
        return RIO_ERROR_OK;
    }

    /* This method return a signature based on the data written
    */
    static inline RIO_ERROR rio_m_RIOCryptoInmeta_sign(RIOCryptoInmeta * self, unsigned char * buf, size_t size, size_t * len) {
        rio_m_RIOCryptoInmeta_destructor(self);
        return RIO_ERROR_RECV_ONLY;
    }

    /* This method receive len bytes of data into buffer
       target buffer *MUST* be large enough to contains len data
       returns len actually received (may be 0),
       or negative value to signal some error.
       If an error occurs after reading some data the amount read will be returned
       and an error returned on subsequent call.
    */
    inline ssize_t rio_m_RIOCryptoInmeta_recv(RIOCryptoInmeta * self, void * data, size_t len)
    {
         return rio_recv(self->insequence, data, len);
    }

    /* This method send len bytes of data from buffer to current transport
       buffer must actually contains the amount of data requested to send.
       returns len actually sent (may be 0),
       or negative value to signal some error.
       If an error occurs after sending some data the amount sent will be returned
       and an error returned on subsequent call.
    */
    inline ssize_t rio_m_RIOCryptoInmeta_send(RIOCryptoInmeta * self, const void * data, size_t len)
    {
        rio_m_RIOCryptoInmeta_destructor(self);
        return -RIO_ERROR_RECV_ONLY;
    }

    static inline RIO_ERROR rio_m_RIOCryptoInmeta_seek(RIOCryptoInmeta * self, int64_t offset, int whence)
    {
        return RIO_ERROR_SEEK_NOT_AVAILABLE;
    }

    static inline RIO_ERROR rio_m_RIOCryptoInmeta_get_status(RIOCryptoInmeta * self)
    {
        return rio_get_status(self->insequence);
    }
};

#endif

