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

namespace PCIDSK
{
#ifdef notdef
    typedef  union
    {
        float		fFloat;
        double	dDouble;
        char		*pszString;
        int32		nInteger;
        int32		*panInts;
    } GDBField, *GDBRecord;
#endif

    const static int NullShapeId = -1;
    typedef int32 ShapeId;

    typedef struct 
    {
        double x;
        double y;
        double z;
    } ShapeVertex;

/************************************************************************/
/*                         PCIDSKVectorSegment                          */
/************************************************************************/

//! Interface to PCIDSK georeferencing segment.

    class PCIDSK_DLL PCIDSKVectorSegment
    {
    public:
        virtual	~PCIDSKVectorSegment() {}

        virtual std::string GetRst() = 0;

        virtual ShapeId     FindFirst() = 0;
        virtual ShapeId     FindNext(ShapeId) = 0;
        
        virtual void        GetVertices( ShapeId, std::vector<ShapeVertex>& ) = 0;
        virtual void        GetRingStart( ShapeId, std::vector<int32>& ) = 0;
        
//        std::vector<Field>       GetFields( ShapeId ) = 0;

#ifdef notdef
    GDBShapeId	(*GetNextShapeId)(GDBLayer , GDBShapeId);
    void	(*DeleteShape)   (GDBLayer , GDBShapeId);
    GDBShapeId	(*CreateShape)   (GDBLayer , GDBShapeId);

    GDBVertex	*(*GetVertices)  (GDBLayer ,GDBShapeId, int *, int32**);
    void	(*SetVertices)   (GDBLayer ,GDBShapeId, int,GDBVertex *, int32*);

    int32	*(*GetRingStart)  (GDBLayer , GDBShapeId);

    TBool       (*GetShapeExtents) (GDBLayer, GDBShapeId, GDBVertex *);

    GDBField *	(*GetRecord)     (GDBLayer, GDBShapeId);
    void        (*SetRecord)     (GDBLayer, GDBShapeId, GDBField *);

    GDBShape    *(*GetShape)     (GDBLayer , GDBShapeId);
    void	 (*SetShape)     (GDBLayer , GDBShapeId, GDBShape *);

    TBool	(*AddField)      (GDBLayer, char *, GDBFieldType, GDBField *);
    TBool       (*DeleteField)   (GDBLayer , char *);
    void	(*SetFieldFormat)(GDBLayer,  int, const char *);

    void       *(*GetRst)        (GDBLayer);
    void	(*SetRst)        (GDBLayer, void *);
    TBool       (*ClearLayer)    (GDBLayer);
#endif        
    };
}; // end namespace PCIDSK

#endif // __INCLUDE_PCIDSK_VECTORSEGMENT_H
