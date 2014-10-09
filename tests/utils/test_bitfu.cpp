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
   Author(s): Christophe Grosjean

   Unit test for bitmap class, compression performance

*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestBitFu
#include <boost/test/auto_unit_test.hpp>

#define LOGNULL
#include "log.hpp"

#include "bitfu.hpp"
#include <stdio.h>

//#include "rdtsc.hpp"
//long long ustime() {
//    struct timeval now;
//    gettimeofday(&now, NULL);
//    return (long long)now.tv_sec*1000000LL + (long long)now.tv_usec;
//}


// out_bytes_be is only defined for 1 to 4 bytes
BOOST_AUTO_TEST_CASE(TestAlign4)
{
//  uint8_t buffer[4] = {};
    BOOST_CHECK_EQUAL(0x00, align4(0));
    BOOST_CHECK_EQUAL(0x04, align4(1));
    BOOST_CHECK_EQUAL(0x04, align4(2));
    BOOST_CHECK_EQUAL(0x04, align4(3));
    BOOST_CHECK_EQUAL(0x04, align4(4));
    BOOST_CHECK_EQUAL(0x08, align4(5));
    BOOST_CHECK_EQUAL(0x08, align4(6));
    BOOST_CHECK_EQUAL(0x08, align4(7));
    BOOST_CHECK_EQUAL(0x08, align4(8));
    BOOST_CHECK_EQUAL(12, align4(9));
}


BOOST_AUTO_TEST_CASE(TestReverseEven)
{
    uint8_t buf[6] = {0, 1, 2, 3, 4, 5};
    reverseit(buf, 6);
    BOOST_CHECK_EQUAL(buf[0], 5);
    BOOST_CHECK_EQUAL(buf[1], 4);
    BOOST_CHECK_EQUAL(buf[2], 3);
    BOOST_CHECK_EQUAL(buf[3], 2);
    BOOST_CHECK_EQUAL(buf[4], 1);
    BOOST_CHECK_EQUAL(buf[5], 0);
}

BOOST_AUTO_TEST_CASE(TestReverseOdd)
{
    uint8_t buf[5] = {1, 2, 3, 4, 5};
    reverseit(buf, 5);
    BOOST_CHECK_EQUAL(buf[0], 5);
    BOOST_CHECK_EQUAL(buf[1], 4);
    BOOST_CHECK_EQUAL(buf[2], 3);
    BOOST_CHECK_EQUAL(buf[3], 2);
    BOOST_CHECK_EQUAL(buf[4], 1);
}

BOOST_AUTO_TEST_CASE(TestNbBytes)
{
    BOOST_CHECK_EQUAL(4, nbbytes(8*5-8));
    BOOST_CHECK_EQUAL(4, nbbytes_large(8*5-8));
    BOOST_CHECK_EQUAL(5, nbbytes(8*5-7));
    BOOST_CHECK_EQUAL(5, nbbytes_large(8*5-7));
    BOOST_CHECK_EQUAL(5, nbbytes(8*5-1));
    BOOST_CHECK_EQUAL(5, nbbytes_large(8*5-1));
    BOOST_CHECK_EQUAL(5, nbbytes(8*5));
    BOOST_CHECK_EQUAL(5, nbbytes_large(8*5));
    BOOST_CHECK_EQUAL(6, nbbytes(8*5+1));
    BOOST_CHECK_EQUAL(6, nbbytes_large(8*5+1));

    BOOST_CHECK_EQUAL(320 % 256, nbbytes(320 * 8));
    BOOST_CHECK_EQUAL(320, nbbytes_large(320 * 8));
}

// out_bytes_be is only defined for 1 to 4 bytes
BOOST_AUTO_TEST_CASE(TestOutBytesLe)
{
    uint8_t buffer[10] = {};
    out_bytes_le(buffer, 3, 0xFFEEDDCC);
    BOOST_CHECK_EQUAL(0xCC, buffer[0]);
    BOOST_CHECK_EQUAL(0xDD, buffer[1]);
    BOOST_CHECK_EQUAL(0xEE, buffer[2]);
    BOOST_CHECK_EQUAL(0x00, buffer[3]);

    out_bytes_le(buffer, 4, 0xBBAA9988);
    BOOST_CHECK_EQUAL(0x88, buffer[0]);
    BOOST_CHECK_EQUAL(0x99, buffer[1]);
    BOOST_CHECK_EQUAL(0xAA, buffer[2]);
    BOOST_CHECK_EQUAL(0xBB, buffer[3]);
}


// out_bytes_be is only defined for 1 to 4 bytes
BOOST_AUTO_TEST_CASE(TestBufOutUint32)
{
    uint8_t buffer[4] = {};
    buf_out_uint32(buffer, 0xFFEEDDCC);
    BOOST_CHECK_EQUAL(0xCC, buffer[0]);
    BOOST_CHECK_EQUAL(0xDD, buffer[1]);
    BOOST_CHECK_EQUAL(0xEE, buffer[2]);
    BOOST_CHECK_EQUAL(0xFF, buffer[3]);
}


// in_bytes_le is only defined for 1 to 4 bytes
BOOST_AUTO_TEST_CASE(TestInBytesLe)
{
    uint8_t buffer[10] = {0xCC, 0xDD, 0xEE, 0xFF };
    BOOST_CHECK_EQUAL(0xEEDDCC , in_uint32_from_nb_bytes_le(3, buffer));

    uint8_t buffer2[10] = {0x88, 0x99, 0xAA, 0xBB };
    BOOST_CHECK_EQUAL(0xBBAA9988 , in_uint32_from_nb_bytes_le(4, buffer2));
}

// in_bytes_be is only defined for 1 to 4 bytes
BOOST_AUTO_TEST_CASE(TestInBytesBe)
{
    uint8_t buffer[10] = {0xCC, 0xDD, 0xEE, 0xFF };
    BOOST_CHECK_EQUAL(0xCCDDEE , in_uint32_from_nb_bytes_be(3, buffer));

    uint8_t buffer2[10] = {0x88, 0x99, 0xAA, 0xBB };
    BOOST_CHECK_EQUAL(0x8899AABB , in_uint32_from_nb_bytes_be(4, buffer2));
}

// rmemcpy : the area must not overlap
BOOST_AUTO_TEST_CASE(TestRmemcpy)
{
    uint8_t buffer[]   = {0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
    uint8_t dest[sizeof(buffer)] = {};
    uint8_t expected[] = {0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xFF, 0xEE, 0xDD, 0xCC };
    rmemcpy(dest, buffer, sizeof(buffer));
    BOOST_CHECK(0 == memcmp(dest, expected, sizeof(expected)));

    uint8_t buffer2[]   = {0xCC, 0xDD, 0xEE };
    uint8_t dest2[sizeof(buffer)] = {};
    uint8_t expected2[] = {0xEE, 0xDD, 0xCC };
    rmemcpy(dest2, buffer2, sizeof(buffer2));
    BOOST_CHECK(0 == memcmp(dest2, expected2, sizeof(expected2)));

}
