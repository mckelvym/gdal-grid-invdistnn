#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace PCIDSK;

class SegmentsTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( SegmentsTest );
 
    CPPUNIT_TEST( testEltoro );
    CPPUNIT_TEST( testGeoref );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void testEltoro();
    void testGeoref();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( SegmentsTest );

void SegmentsTest::setUp()
{
}

void SegmentsTest::tearDown()
{
}

void SegmentsTest::testEltoro()
{
    PCIDSKFile *eltoro;

    eltoro = PCIDSK::Open( "eltoro.pix", "r", NULL );

    CPPUNIT_ASSERT( eltoro != NULL );

    PCIDSKSegment *seg = eltoro->GetSegment( 1 );

    CPPUNIT_ASSERT( seg != NULL );
    CPPUNIT_ASSERT( seg->GetSegmentType() == SEG_GEO );
    CPPUNIT_ASSERT( strcmp(seg->GetName(),"GEOref") == 0 );
    CPPUNIT_ASSERT( seg->GetSegmentNumber() == 1 );

    seg = eltoro->GetSegment( 3 );

    CPPUNIT_ASSERT( seg != NULL );
    CPPUNIT_ASSERT( seg->GetSegmentType() == SEG_LUT );
    CPPUNIT_ASSERT( strcmp(seg->GetName(),"Equal") == 0 );
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

    CPPUNIT_ASSERT( strcmp(georef->GetGeosys(),"UTM    11 S E000") == 0 );

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

