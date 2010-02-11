/******************************************************************************
 *
 * Purpose:  CPPUnit test for various channel access methods (not imagery)
 * 
 ******************************************************************************
 * Copyright (c) 2010
 * PCI Geomatics, 50 West Wilmot Street, Richmond Hill, Ont, Canada
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "pcidsk.h"
#include "pcidsk_georef.h"
#include "pcidsk_pct.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace PCIDSK;

class ChannelsTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( ChannelsTest );
 
    CPPUNIT_TEST( testChannelInfoRead );
    CPPUNIT_TEST( testChannelInfoUpdate );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void testChannelInfoRead();
    void testChannelInfoUpdate();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( ChannelsTest );

void ChannelsTest::setUp()
{
}

void ChannelsTest::tearDown()
{
}

/************************************************************************/
/*                        testChannelInfoRead()                         */
/************************************************************************/

void ChannelsTest::testChannelInfoRead()
{
    PCIDSKFile *eltoro;

    eltoro = PCIDSK::Open( "eltoro.pix", "r", NULL );

    CPPUNIT_ASSERT( eltoro != NULL );

    PCIDSKChannel *chan = eltoro->GetChannel(1);

    CPPUNIT_ASSERT( chan != NULL );
    CPPUNIT_ASSERT( chan->GetDescription() == "SPOT HRV1 Panchromatic (0.51 - 0.73 um)" );

    delete eltoro;
}

/************************************************************************/
/*                       testChannelInfoUpdate()                        */
/************************************************************************/

void ChannelsTest::testChannelInfoUpdate()
{
    PCIDSKFile *file;
    PCIDSKChannel *channel;
    eChanType channel_types[2] = {CHN_8U, CHN_8U};

    file = PCIDSK::Create( "channelupdate.pix", 100, 125, 2, channel_types,
                           "PIXEL", NULL );
    
    CPPUNIT_ASSERT( file != NULL );
    CPPUNIT_ASSERT( file->GetUpdatable() );

    channel = file->GetChannel(1);
    channel->SetDescription( "Test Description" );

    channel = file->GetChannel(2);
    channel->SetDescription( "This is a very long, even a too long, description.  Make sure it is truncated." );

    delete file;

    file = PCIDSK::Open( "channelupdate.pix", "r+", NULL );

    channel = file->GetChannel(1);
    CPPUNIT_ASSERT( channel->GetDescription() == "Test Description" );
    
    channel = file->GetChannel(2);
    CPPUNIT_ASSERT( channel->GetDescription() == "This is a very long, even a too long, description.  Make sure it" );
    
    delete file;

    unlink( "channelupdate.pix" );
}

