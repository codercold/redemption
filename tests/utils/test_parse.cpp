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
   Copyright (C) Wallix 2014
   Author(s): Christophe Grosjean

   Unit test for char parse class

*/

#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE TestParse
#include <boost/test/auto_unit_test.hpp>

#define LOGPRINT
#include "log.hpp"

#include "parse.hpp"
#include <stdio.h>

BOOST_AUTO_TEST_CASE(TestParse)
{
    uint8_t buffer[] = { 0x10, 0xFF, 0xFF, 0x11, 0x12, 0x13, 0x14 };
    Parse data(buffer);
    BOOST_CHECK_EQUAL(0x10, data.in_uint8());
    BOOST_CHECK_EQUAL(0xFF, data.in_uint8());
    BOOST_CHECK_EQUAL(-1, data.in_sint8());
}

