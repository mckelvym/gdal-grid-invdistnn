/******************************************************************************
 *
 * Purpose:  PCIDSK Georeferencing information storage class. Declaration.
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
#ifndef __INCLUDE_PCIDSK_GEOREF_H
#define __INCLUDE_PCIDSK_GEOREF_H

#include <string>

namespace PCIDSK
{
/************************************************************************/
/*                             PCIDSKGeoref                             */
/************************************************************************/

//! Interface to PCIDSK georeferencing segment.

    class PCIDSK_DLL PCIDSKGeoref
    {
    public:
        virtual	~PCIDSKGeoref() {}

/**
\brief Get georeferencing transformation.

Returns the affine georeferencing transform coefficients for this image.
Used to map from pixel/line coordinates to georeferenced coordinates using
the transformation:

 Xgeo = a1 +   a2 * Xpix + xrot * Ypix

 Ygeo = b1 + yrot * Xpix +   b2 * Ypix

where Xpix and Ypix are pixel line locations with (0,0) being the top left
corner of the top left pixel, and (0.5,0.5) being the center of the top left
pixel.  For an ungeoreferenced image the values will be 
(0.0,1.0,0.0,0.0,0.0,1.0).

@param a1 returns easting of top left corner.
@param a2 returns easting pixel size.
@param xrot returns rotational coefficient, normally zero.
@param b1 returns northing of the top left corner.
@param yrot returns rotational coefficient, normally zero.
@param b3 returns northing pixel size, normally negative indicating north-up.

*/
        virtual void GetTransform( double &a1, double &a2, double &xrot, 
            double &b1, double &yrot, double &b3 ) = 0;

/**
\brief Fetch georeferencing string.

Returns the short, 16 character, georeferncing string.  This string is
sufficient to document the coordinate system of simple coordinate
systems (like "UTM    17 S D000"), while other coordinate systems are
only fully defined with additional projection parameters.

@return the georeferencing string. 

*/        
        virtual std::string GetGeosys() = 0;

/**
\brief Write simple georeferencing information

Writes out a georeferencing string and geotransform to the segment. 

@param geosys 16 character coordinate system, like "UTM    17 S D000".
@param a1 easting of top left corner.
@param a2 easting pixel size.
@param xrot rotational coefficient, normally zero.
@param b1 northing of the top left corner.
@param yrot rotational coefficient, normally zero.
@param b3 northing pixel size, normally negative indicating north-up.

*/
        virtual void WriteSimple( std::string geosys, 
            double a1, double a2, double xrot, 
            double b1, double yrot, double b3 ) = 0;
    };
}; // end namespace PCIDSK

#endif // __INCLUDE_PCIDSK_GEOREF_H
