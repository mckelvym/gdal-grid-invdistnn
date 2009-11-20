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
#include <iterator>
#include "pcidsk_shape.h"

namespace PCIDSK
{
    class ShapeIterator;
    
/************************************************************************/
/*                         PCIDSKVectorSegment                          */
/************************************************************************/

//! Interface to PCIDSK vector segment.

    class PCIDSK_DLL PCIDSKVectorSegment
    {
    public:
        virtual	~PCIDSKVectorSegment() {}

/**
\brief Fetch RST. 

No attempt is made to parse the RST, it is up to the caller to decode it. 

NOTE: There is some header info on RST format that may be needed to do this 
for older RSTs.

@return RST as a string.
*/
        virtual std::string GetRst() = 0;


/**
\brief Get field count.

Note that this includes any system attributes, like RingStart, that would
not normally be shown to the user.

@return the number of attribute fields defined on this layer.
*/

        virtual int         GetFieldCount() = 0;

/**
\brief Get field name.

@param field_index index of the field requested from zero to GetFieldCount()-1.
@return the field name. 
*/
        virtual std::string GetFieldName(int field_index) = 0;

/**
\brief Get field description.

@param field_index index of the field requested from zero to GetFieldCount()-1.
@return the field description, often empty.
*/
        virtual std::string GetFieldDescription(int field_index) = 0;

/**
\brief Get field type.

@param field_index index of the field requested from zero to GetFieldCount()-1.
@return the field type.
*/
        virtual ShapeFieldType GetFieldType(int field_index) = 0;
        virtual std::string GetFieldFormat(int field_index) = 0;
        virtual ShapeField  GetFieldDefault(int field_index) = 0;

        virtual ShapeIterator begin() = 0;
        virtual ShapeIterator end() = 0;

        virtual ShapeId     FindFirst() = 0;
        virtual ShapeId     FindNext(ShapeId) = 0;
        
        virtual void        GetVertices( ShapeId, std::vector<ShapeVertex>& ) = 0;
        virtual void        GetFields( ShapeId, std::vector<ShapeField>& ) = 0;
    };

/************************************************************************/
/*                            ShapeIterator                             */
/************************************************************************/

//! Iterator over shapeids in a vector segment.

    class ShapeIterator : public std::iterator<std::input_iterator_tag, ShapeId>
    {
        ShapeId id;
        PCIDSKVectorSegment *seg;
        
    public:
        ShapeIterator(PCIDSKVectorSegment *seg_in)
                : seg(seg_in)  { id = seg->FindFirst(); }
        ShapeIterator(PCIDSKVectorSegment *seg_in, ShapeId id_in )
                : id(id_in), seg(seg_in)  {}
        ShapeIterator(const ShapeIterator& mit) : id(mit.id), seg(mit.seg) {}
        ShapeIterator& operator++() { id=seg->FindNext(id); return *this;}
        ShapeIterator& operator++(int) { id=seg->FindNext(id); return *this;}
        bool operator==(const ShapeIterator& rhs) {return id == rhs.id;}
        bool operator!=(const ShapeIterator& rhs) {return id != rhs.id;}
        ShapeId& operator*() {return id;}
    };

}; // end namespace PCIDSK

#endif // __INCLUDE_PCIDSK_VECTORSEGMENT_H
