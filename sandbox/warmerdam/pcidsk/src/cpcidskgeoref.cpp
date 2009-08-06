/******************************************************************************
 *
 * Purpose:  Implementation of the CPCIDSKGeoref class.
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

#include "pcidsk_p.h"
#include <assert.h>

using namespace PCIDSK;

/************************************************************************/
/*                           CPCIDSKGeoref()                            */
/************************************************************************/

CPCIDSKGeoref::CPCIDSKGeoref( CPCIDSKFile *file, int segment,
                              const char *segment_pointer )
        : CPCIDSKSegment( file, segment, segment_pointer )

{
    loaded = false;
}

/************************************************************************/
/*                           ~CPCIDSKGeoref()                           */
/************************************************************************/

CPCIDSKGeoref::~CPCIDSKGeoref()

{
}

/************************************************************************/
/*                                Load()                                */
/************************************************************************/

void CPCIDSKGeoref::Load()

{
    if( loaded )
        return;

    // TODO: this should likely be protected by a mutex. 

/* -------------------------------------------------------------------- */
/*      Load the segment contents into a buffer.                        */
/* -------------------------------------------------------------------- */
    seg_data.SetSize( (int) (data_size - 1024) );

    ReadFromFile( seg_data.buffer, 0, data_size - 1024 );

/* -------------------------------------------------------------------- */
/*      Handle simple case of a POLYNOMIAL.                             */
/* -------------------------------------------------------------------- */
    if( strncmp(seg_data.buffer,"POLYNOMIAL",10) == 0 )
    {
        seg_data.Get(32,16,geosys);
        
        if( seg_data.GetInt(48,8) != 3 || seg_data.GetInt(56,8) != 3 )
            ThrowPCIDSKException( "Unexpected number of coefficients in POLYNOMIAL GEO segment." );

        a1   = seg_data.GetDouble(212+26*0,26);
        a2   = seg_data.GetDouble(212+26*1,26);
        xrot = seg_data.GetDouble(212+26*2,26);

        b1   = seg_data.GetDouble(1642+26*0,26);
        yrot = seg_data.GetDouble(1642+26*1,26);
        b3   = seg_data.GetDouble(1642+26*2,26);
    }

/* -------------------------------------------------------------------- */
/*      Handle the case of a PROJECTION segment - for now we ignore     */
/*      the actual projection parameters.                               */
/* -------------------------------------------------------------------- */
    else if( strncmp(seg_data.buffer,"PROJECTION",10) == 0 )
    {
        seg_data.Get(32,16,geosys);
        
        if( seg_data.GetInt(48,8) != 3 || seg_data.GetInt(56,8) != 3 )
            ThrowPCIDSKException( "Unexpected number of coefficients in POLYNOMIAL GEO segment." );

        a1   = seg_data.GetDouble(1980+26*0,26);
        a2   = seg_data.GetDouble(1980+26*1,26);
        xrot = seg_data.GetDouble(1980+26*2,26);

        b1   = seg_data.GetDouble(2526+26*0,26);
        yrot = seg_data.GetDouble(2526+26*1,26);
        b3   = seg_data.GetDouble(2526+26*2,26);
    }

    else
    {
        ThrowPCIDSKException( "Unexpected GEO segment type: %s", 
                              seg_data.Get(0,16) );
    }
}

/************************************************************************/
/*                             GetGeosys()                              */
/************************************************************************/

std::string CPCIDSKGeoref::GetGeosys()

{
    Load();
    return geosys;
}

/************************************************************************/
/*                            GetTransform()                            */
/************************************************************************/

void CPCIDSKGeoref::GetTransform( double &a1, double &a2, double &xrot, 
                                  double &b1, double &yrot, double &b3 )

{
    Load();

    a1   = this->a1;
    a2   = this->a2;
    xrot = this->xrot;
    b1   = this->b1;
    yrot = this->yrot;
    b3   = this->b3;
}

/************************************************************************/
/*                            WriteSimple()                             */
/************************************************************************/

void CPCIDSKGeoref::WriteSimple( std::string geosys, 
                                 double a1, double a2, double xrot, 
                                 double b1, double yrot, double b3 )

{
    loaded = false;

/* -------------------------------------------------------------------- */
/*      Handle simple case of a POLYNOMIAL.                             */
/* -------------------------------------------------------------------- */
    seg_data.SetSize( 6 * 512 );

    seg_data.Put( " ", 0, seg_data.buffer_size );

    // SD.GEO.P1
    seg_data.Put( "POLYNOMIAL", 0, 16 );
    
    // SD.GEO.P2
    seg_data.Put( "PIXEL", 16, 16 );
    
    // SD.GEO.P3
    seg_data.Put( geosys.c_str(), 32, 16 );

    // SD.GEO.P4
    seg_data.Put( 3, 48, 8 );
    
    // SD.GEO.P5
    seg_data.Put( 3, 56, 8 );

    // SD.GEO.P7
    seg_data.Put( a1,   212 + 0*26, 26, "%26.18E" );
    seg_data.Put( a2,   212 + 1*26, 26, "%26.18E" );
    seg_data.Put( xrot, 212 + 2*26, 26, "%26.18E" );
    
    // SD.GEO.P8
    seg_data.Put( b1,   1642 + 0*26, 26, "%26.18E" );
    seg_data.Put( yrot, 1642 + 1*26, 26, "%26.18E" );
    seg_data.Put( b3,   1642 + 2*26, 26, "%26.18E" );

    WriteToFile( seg_data.buffer, 0, seg_data.buffer_size );
}
