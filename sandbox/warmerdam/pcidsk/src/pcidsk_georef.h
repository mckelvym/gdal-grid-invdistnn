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

        virtual void GetTransform( double &a1, double &a2, double &xrot, 
            double &b1, double &yrot, double &b3 ) = 0;
        virtual std::string GetGeosys() = 0;

        virtual void WriteSimple( std::string geosys, 
            double a1, double a2, double xrot, 
            double b1, double yrot, double b3 ) = 0;
    };
}; // end namespace PCIDSK

#endif // __INCLUDE_PCIDSK_GEOREF_H