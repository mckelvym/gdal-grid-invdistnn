/******************************************************************************
 *
 * Purpose:  PCIDSK Vector Segment public interface. Declaration.
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

#ifndef __INCLUDE_PCIDSK_VECTORSEGMENT_H
#define __INCLUDE_PCIDSK_VECTORSEGMENT_H

#include <string>
#include <vector>
#include "pcidsk_shape.h"

namespace PCIDSK
{
/************************************************************************/
/*                         PCIDSKVectorSegment                          */
/************************************************************************/

//! Interface to PCIDSK georeferencing segment.

    class PCIDSK_DLL PCIDSKVectorSegment
    {
    public:
        virtual	~PCIDSKVectorSegment() {}

        virtual std::string GetRst() = 0;

        virtual int         GetFieldCount() = 0;
        virtual std::string GetFieldName(int) = 0;
        virtual std::string GetFieldDescription(int) = 0;
        virtual ShapeFieldType GetFieldType(int) = 0;
        virtual std::string GetFieldFormat(int) = 0;
        virtual ShapeField  GetFieldDefault(int) = 0;

        virtual ShapeId     FindFirst() = 0;
        virtual ShapeId     FindNext(ShapeId) = 0;
        
        virtual void        GetVertices( ShapeId, std::vector<ShapeVertex>& ) = 0;
        virtual void        GetFields( ShapeId, std::vector<ShapeField>& ) = 0;
    };
}; // end namespace PCIDSK

#endif // __INCLUDE_PCIDSK_VECTORSEGMENT_H
