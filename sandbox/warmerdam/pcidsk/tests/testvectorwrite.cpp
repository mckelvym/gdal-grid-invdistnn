/******************************************************************************
 *
 * Purpose:  CPPUnit test for writing vector segments.
 * 
 ******************************************************************************
 * Copyright (c) 2009
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
#include "pcidsk_vectorsegment.h"
#include <cmath>
#include <cppunit/extensions/HelperMacros.h>
#include <vector>

using namespace PCIDSK;

class VectorWriteTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( VectorWriteTest );
 
    CPPUNIT_TEST( testProjection );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void testProjection();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( VectorWriteTest );

void VectorWriteTest::setUp()
{
}

void VectorWriteTest::tearDown()
{
}

void VectorWriteTest::testProjection()
{
    PCIDSKFile *file;
    eChanType channel_types[1] = {CHN_8U};
    
    file = PCIDSK::Create( "vectestproj.pix", 50, 40, 1, channel_types,
                           "BAND", NULL );

    CPPUNIT_ASSERT( file != NULL );

    // Get the GCP segment
    int seg_num = file->CreateSegment("VTEST", "Test write", SEG_VEC, 0);
    
    PCIDSKSegment* seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKVectorSegment* vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    std::string geosys;
    std::vector<double> parms = vec_seg->GetProjection( geosys );

    CPPUNIT_ASSERT( geosys == "METRE           " );
    for( unsigned int i = 0; i < 18; i++ )
    {
        if( i == 17 )
            CPPUNIT_ASSERT( parms[i] == UNIT_METER );
        else
            CPPUNIT_ASSERT( parms[i] == 0 );
    }

    // Try setting a simple projection.
    vec_seg->SetProjection( "LONG        D000", parms );

    parms = vec_seg->GetProjection( geosys );

    CPPUNIT_ASSERT( geosys == "LONG        D000" );
    for( unsigned int i = 0; i < 18; i++ )
    {
        if( i == 17 )
            CPPUNIT_ASSERT( parms[i] == -1 );
        else
            CPPUNIT_ASSERT( parms[i] == 0 );
    }

    // Try a complex projection with parameters.

    parms[2] = -117.0;
    parms[3] = 33;
    parms[6] = 200000;
    parms[7] = 100000;
    parms[8] = 0.998;
    parms[17] = 2;

    vec_seg->SetProjection( "TM          D000", parms );

    // Close and reopen.
    delete file;

    file = PCIDSK::Open( "vectestproj.pix", "r", NULL );

    seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Confirm changes stuck.

    std::vector<double> old_parms = parms;
    parms.clear();

    parms = vec_seg->GetProjection( geosys );

    CPPUNIT_ASSERT( geosys == "TM          D000" );
    for( unsigned int i = 0; i < 17; i++ )
    {
        CPPUNIT_ASSERT( parms[i] == old_parms[i] );
    }

    CPPUNIT_ASSERT( parms[17] == -1 );

    delete file;

    unlink( "vectestproj.pix" );
}
