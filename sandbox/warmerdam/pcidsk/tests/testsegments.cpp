/******************************************************************************
 *
 * Purpose:  CPPUnit test for generic segment access and specialized access
 *           to some segment types.
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
#include "pcidsk_georef.h"
#include "pcidsk_pct.h"
#include "pcidsk_gcp.h"
#include "pcidsk_gcpsegment.h"
#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>

using namespace PCIDSK;

class SegmentsTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( SegmentsTest );
 
    CPPUNIT_TEST( testEltoro );
    CPPUNIT_TEST( testGeoref );
    CPPUNIT_TEST( testPCTRead );
    CPPUNIT_TEST( testPCTWrite );
    CPPUNIT_TEST( testSegDelete );
    CPPUNIT_TEST( testGCPAdd );
    CPPUNIT_TEST( testGCPWrite );
    CPPUNIT_TEST( testGCPRead );
    CPPUNIT_TEST( testBitmapRead );
    CPPUNIT_TEST( testBitmapWrite );
    
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void testEltoro();
    void testGeoref();
    void testPCTRead();
    void testPCTWrite();
    void testSegDelete();
    void testGCPAdd();
    void testGCPWrite();
    void testGCPRead();
    void testBitmapRead();
    void testBitmapWrite();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( SegmentsTest );

void SegmentsTest::setUp()
{
    eChanType channel_types[1] = { CHN_8U };
    PCIDSKFile* file = 
        PCIDSK::Create("gcp_file.pix", 50, 50, 1, channel_types, "BAND");
    delete file;
}

void SegmentsTest::tearDown()
{
    unlink("gcp_file.pix");
}

void SegmentsTest::testEltoro()
{
    PCIDSKFile *eltoro;

    eltoro = PCIDSK::Open( "eltoro.pix", "r", NULL );

    CPPUNIT_ASSERT( eltoro != NULL );

    PCIDSKSegment *seg = eltoro->GetSegment( 1 );

    CPPUNIT_ASSERT( seg != NULL );
    CPPUNIT_ASSERT( seg->GetSegmentType() == SEG_GEO );
    CPPUNIT_ASSERT( seg->GetName() == "GEOref" );
    CPPUNIT_ASSERT( seg->GetSegmentNumber() == 1 );

    seg = eltoro->GetSegment( 3 );

    CPPUNIT_ASSERT( seg != NULL );
    CPPUNIT_ASSERT( seg->GetSegmentType() == SEG_LUT );
    CPPUNIT_ASSERT( seg->GetName() == "Equal" );
    CPPUNIT_ASSERT( seg->GetSegmentNumber() == 3 );

    delete eltoro;
}

void SegmentsTest::testGeoref()
{
    PCIDSKFile *eltoro;

    eltoro = PCIDSK::Open( "eltoro.pix", "r", NULL );

    CPPUNIT_ASSERT( eltoro != NULL );

    PCIDSKSegment *seg = eltoro->GetSegment( 1 );

    CPPUNIT_ASSERT( seg != NULL );

    PCIDSKGeoref *georef = dynamic_cast<PCIDSKGeoref*>(seg);

    CPPUNIT_ASSERT( georef != NULL );

    CPPUNIT_ASSERT( georef->GetGeosys() == "UTM    11 S E000" );

    double a1, a2, xrot, b1, yrot, b3;

    georef->GetTransform( a1, a2, xrot, b1, yrot, b3 );

    CPPUNIT_ASSERT( a1 == 430640.0 );
    CPPUNIT_ASSERT( a2 == 10.0 );
    CPPUNIT_ASSERT( xrot == 0.0 );
    CPPUNIT_ASSERT( b1 == 3732300.0 );
    CPPUNIT_ASSERT( yrot == 0.0);
    CPPUNIT_ASSERT( b3 == -10.0 );
    
    delete eltoro;
}

void SegmentsTest::testPCTRead()
{
    PCIDSKFile *file;

    file = PCIDSK::Open( "irvine.pix", "r", NULL );

    CPPUNIT_ASSERT( file != NULL );

    PCIDSKSegment *seg = file->GetSegment( 7 );

    CPPUNIT_ASSERT( seg != NULL );

    PCIDSK_PCT *pct_seg = dynamic_cast<PCIDSK_PCT*>(seg);

    CPPUNIT_ASSERT( pct_seg != NULL );

    unsigned char pct[768];

    pct_seg->ReadPCT( pct );

    CPPUNIT_ASSERT( pct[0] == 0 );
    CPPUNIT_ASSERT( pct[255] == 255 );
    CPPUNIT_ASSERT( pct[767] == 255 );
    
    delete file;
}

void SegmentsTest::testPCTWrite()
{
    eChanType channel_types[1] = {CHN_8U};
    PCIDSKFile *file = 
        PCIDSK::Create( "pct_file.pix", 50, 40, 1, channel_types, "BAND", NULL);

    CPPUNIT_ASSERT( file != NULL );

    int iSeg = file->CreateSegment( "TSTPCT", "Desc", SEG_PCT, 0 );

    PCIDSKSegment *seg = file->GetSegment( iSeg );

    CPPUNIT_ASSERT( seg != NULL );

    PCIDSK_PCT *pct_seg = dynamic_cast<PCIDSK_PCT*>(seg);

    CPPUNIT_ASSERT( pct_seg != NULL );

    unsigned char pct[768], pct2[768];
    int i;

    for( i = 0; i < 256; i++ )
    {
        pct[i] = i;
        pct[256+i] = 255-i;;
        pct[512+i] = i/2;
    }
    pct_seg->WritePCT( pct );

    pct_seg->ReadPCT( pct2 );

    CPPUNIT_ASSERT( pct[0] == 0 );
    CPPUNIT_ASSERT( pct[255] == 255 );
    CPPUNIT_ASSERT( pct[767] == 127 );

    delete file;
    unlink( "pct_file.pix" );
}

void SegmentsTest::testSegDelete()
{
    eChanType channel_types[1] = {CHN_8U};
    PCIDSKFile *file = 
        PCIDSK::Create( "pct_file.pix", 50, 40, 1, channel_types, "BAND", NULL);

    CPPUNIT_ASSERT( file != NULL );

    int iSeg = file->CreateSegment( "TSTPCT", "Desc", SEG_PCT, 0 );

    PCIDSKSegment *seg = file->GetSegment( iSeg );

    CPPUNIT_ASSERT( seg != NULL );

    PCIDSK_PCT *pct_seg = dynamic_cast<PCIDSK_PCT*>(seg);

    CPPUNIT_ASSERT( pct_seg != NULL );

    unsigned char pct[768];
    int i;

    for( i = 0; i < 256; i++ )
    {
        pct[i] = i;
        pct[256+i] = 255-i;;
        pct[512+i] = i/2;
    }
    pct_seg->WritePCT( pct );
    seg->SetMetadataValue( "TEST", "VALUE" );

    file->DeleteSegment( iSeg );

    delete file;

    file = PCIDSK::Open( "pct_file.pix", "r+", NULL );

    CPPUNIT_ASSERT( file->GetSegment( iSeg ) == NULL );

    delete file;

    unlink( "pct_file.pix" );
}

void SegmentsTest::testGCPAdd()
{
    PCIDSKFile* file = 
        PCIDSK::Open("gcp_file.pix", "r+");
    
    int segnum = file->CreateSegment("GCP2TEST", "GCP2 Test Segment", SEG_GCP2, 0);
    PCIDSKSegment* seg = file->GetSegment(segnum);
    CPPUNIT_ASSERT(seg != NULL);
    PCIDSKGCPSegment* gcp_seg = dynamic_cast<PCIDSKGCPSegment*>(seg);
    CPPUNIT_ASSERT(gcp_seg != NULL);
    
    CPPUNIT_ASSERT(gcp_seg->GetGCPCount() == 0);
    
    delete file;
}

void SegmentsTest::testGCPWrite()
{
    PCIDSKFile* file = 
        PCIDSK::Open("gcp_file.pix", "r+");
        
    // Get the GCP segment
    int seg_num = file->CreateSegment("GCP2TEST", "GCP2 Test write", SEG_GCP2, 0);
    PCIDSKSegment* seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKGCPSegment* gcp_seg = dynamic_cast<PCIDSKGCPSegment*>(seg);
    
    CPPUNIT_ASSERT(gcp_seg != NULL);
    
    std::vector<PCIDSK::GCP> gcps;
    
    // Create a few GCPs
    for (unsigned int y = 0; y <= 50; y += 10) {
        for (unsigned int x = 0; x <= 50; x += 10) {
            std::stringstream ss;
            ss << "GCP #" << y * x;
            PCIDSK::GCP gcp(x, y, 1.0,
                x, y, ss.str(), "LAT/LONG D000");
            gcps.push_back(gcp);
        }
    }
    
    // Write out some GCPs
    gcp_seg->SetGCPs(gcps);
    
    CPPUNIT_ASSERT(gcp_seg->GetGCPCount() == gcps.size());
    
    // Read the GCPs back
    std::vector<PCIDSK::GCP> const& gcps_read = gcp_seg->GetGCPs();
    
    CPPUNIT_ASSERT(gcps_read.size() == gcps.size());
    
    delete file;
    
    file = PCIDSK::Open("gcp_file.pix", "r");
    CPPUNIT_ASSERT(file != NULL);
    seg = file->GetSegment(seg_num);
    CPPUNIT_ASSERT(seg != NULL);
    gcp_seg = dynamic_cast<PCIDSKGCPSegment*>(seg);
    CPPUNIT_ASSERT(gcp_seg != NULL);
    CPPUNIT_ASSERT(gcp_seg->GetGCPCount() == gcps.size());
    
    delete file;
}

void SegmentsTest::testGCPRead()
{
    PCIDSKFile* file =
        PCIDSK::Open("irvine_gcp2.pix", "r+");
    
    CPPUNIT_ASSERT(file != NULL);
    
    // Get irvine's GCP2 segment
    PCIDSKSegment* seg = file->GetSegment(9);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKGCPSegment* gcp_seg = dynamic_cast<PCIDSKGCPSegment*>(seg);
    
    CPPUNIT_ASSERT(gcp_seg != NULL);
    
    // Get the GCPs
    std::vector<PCIDSK::GCP> const& gcps_read = gcp_seg->GetGCPs();
    
    CPPUNIT_ASSERT(gcps_read.size() == 22);
    
    delete file;
}

void SegmentsTest::testBitmapRead()

{
    PCIDSKFile* file = PCIDSK::Open("irvine.pix", "r");
    
    CPPUNIT_ASSERT(file != NULL);
    
    // Get bitmap segment.
    PCIDSKSegment* seg = file->GetSegment(10);
    
    CPPUNIT_ASSERT(seg != NULL);

    PCIDSKChannel  *bitmap = dynamic_cast<PCIDSKChannel*>(seg);
    
    CPPUNIT_ASSERT(bitmap != NULL);

    // Confirm expected sizes.

    CPPUNIT_ASSERT( bitmap->GetType() == CHN_BIT );
    CPPUNIT_ASSERT( bitmap->GetWidth() == 512 );
    CPPUNIT_ASSERT( bitmap->GetHeight() == 512 );
    CPPUNIT_ASSERT( bitmap->GetBlockWidth() == 512 );
    CPPUNIT_ASSERT( bitmap->GetBlockHeight() == 8 );
    CPPUNIT_ASSERT( bitmap->GetBlockCount() == 64 );
    CPPUNIT_ASSERT( bitmap->GetOverviewCount() == 0 );

    // Confirm the channel machinery is working.
    CPPUNIT_ASSERT( bitmap->GetDescription() 
                    == "Training site for type 2 water" );

    // Fetch all the data and do a sort of checksum.
    unsigned char data[512];
    uint64 checksum = 0;

    for( int i = 0; i < bitmap->GetBlockCount(); i++ )
    {
        bitmap->ReadBlock( i, data );

        for( unsigned int j = 0; j < sizeof(data); j++ )
            checksum += (i+j+1) * data[j];
    }

    CPPUNIT_ASSERT( checksum == 1256278 );
    
    delete file;
}

void SegmentsTest::testBitmapWrite()

{
    PCIDSKFile* file = 
        PCIDSK::Create("bitmap_file.pix", 50, 50, 0, NULL, "PIXEL");
        
    // Get the GCP segment
    int seg_num = file->CreateSegment("BIT_TEST", "Bitmap Test write", 
                                      SEG_BIT, 0);

    PCIDSKSegment* seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKChannel* bitmap = dynamic_cast<PCIDSKChannel*>(seg);
    
    CPPUNIT_ASSERT(bitmap != NULL);

    // Confirm expected sizes.

    CPPUNIT_ASSERT( bitmap->GetType() == CHN_BIT );
    CPPUNIT_ASSERT( bitmap->GetWidth() == 50 );
    CPPUNIT_ASSERT( bitmap->GetHeight() == 50 );
    CPPUNIT_ASSERT( bitmap->GetBlockWidth() == 50 );
    CPPUNIT_ASSERT( bitmap->GetBlockHeight() == 8 );
    CPPUNIT_ASSERT( bitmap->GetBlockCount() == 7 );
    CPPUNIT_ASSERT( bitmap->GetOverviewCount() == 0 );

    // Write simple bitmap block.
    unsigned char data[50];
    unsigned int  i;

    for( i = 0; i < sizeof(data); i++ )
        data[i] = i;

    bitmap->WriteBlock( 2, data );

    // Confirm unwritten block is all zeros.
    bitmap->ReadBlock( 3, data );

    for( i = 0; i < sizeof(data); i++ )
        CPPUNIT_ASSERT( data[i] == 0 );

    // Confirm written block is as expected
    bitmap->ReadBlock( 2, data );

    for( i = 0; i < sizeof(data); i++ )
        CPPUNIT_ASSERT( data[i] == i );

    delete file;
}
