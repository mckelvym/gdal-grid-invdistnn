/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The OGRCircularString geometry class.
 * Author:   Even Rouault, even dot rouault at mines dash paris dot ogr
 *
 ******************************************************************************
 * Copyright (c) 2010, Even Rouault
 * Copyright (c) 1999, Frank Warmerdam
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

#include "ogr_geometry.h"
#include "ogr_p.h"
#include <assert.h>

CPL_CVSID("$Id");

/************************************************************************/
/*                         OGRCircularString()                          */
/************************************************************************/

/**
 * \brief Create an empty circular string.
 */

OGRCircularString::OGRCircularString() : OGRLineString()

{
}

/************************************************************************/
/*                         ~OGRCircularString()                         */
/************************************************************************/

OGRCircularString::~OGRCircularString()

{
}

/************************************************************************/
/*                          getGeometryType()                           */
/************************************************************************/

OGRwkbGeometryType OGRCircularString::getGeometryType() const

{
    if( getCoordinateDimension() == 3 )
        return wkbCircularString25D;
    else
        return wkbCircularString;
}

/************************************************************************/
/*                          getGeometryName()                           */
/************************************************************************/

const char * OGRCircularString::getGeometryName() const

{
    return "CIRCULARSTRING";
}

/************************************************************************/
/*                               clone()                                */
/************************************************************************/

OGRGeometry *OGRCircularString::clone() const

{
    OGRCircularString       *poNewCircularString;

    poNewCircularString = new OGRCircularString();

    poNewCircularString->assignSpatialReference( getSpatialReference() );
    poNewCircularString->setPoints( nPointCount, paoPoints, padfZ );
    poNewCircularString->setCoordinateDimension( getCoordinateDimension() );
    
    return poNewCircularString;
}

/************************************************************************/
/*                              WkbSize()                               */
/*                                                                      */
/*      Return the size of this object in well known binary             */
/*      representation including the byte order, and type information.  */
/************************************************************************/

int OGRCircularString::WkbSize() const

{
    return 5 + 4 + 8 * nPointCount * getCoordinateDimension();
}

/************************************************************************/
/*                           importFromWkb()                            */
/*                                                                      */
/*      Initialize from serialized stream in well known binary          */
/*      format.                                                         */
/************************************************************************/

OGRErr OGRCircularString::importFromWkb( unsigned char * pabyData,
                                     int nSize )

{
    OGRwkbByteOrder     eByteOrder;
    
    if( nSize < 9 && nSize != -1 )
        return OGRERR_NOT_ENOUGH_DATA;

/* -------------------------------------------------------------------- */
/*      Get the byte order byte.                                        */
/* -------------------------------------------------------------------- */
    eByteOrder = DB2_V72_FIX_BYTE_ORDER((OGRwkbByteOrder) *pabyData);
    if (!( eByteOrder == wkbXDR || eByteOrder == wkbNDR ))
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      Get the geometry feature type.  For now we assume that          */
/*      geometry type is between 0 and 255 so we only have to fetch     */
/*      one byte.                                                       */
/* -------------------------------------------------------------------- */
    OGRwkbGeometryType eGeometryType;
    int bIs3D = FALSE;

    if( eByteOrder == wkbNDR )
    {
        eGeometryType = (OGRwkbGeometryType) pabyData[1];
        bIs3D = pabyData[4] & 0x80 || pabyData[2] & 0x80;
    }
    else
    {
        eGeometryType = (OGRwkbGeometryType) pabyData[4];
        bIs3D = pabyData[1] & 0x80 || pabyData[3] & 0x80;
    }

    int nGeometryType;
    memcpy(&nGeometryType, pabyData + 1, 4);
    if( OGR_SWAP( eByteOrder ) )
        nGeometryType = CPL_SWAP32(nGeometryType);
    if (eGeometryType != wkbCircularString &&
        /* SQL MM Part 3 */ nGeometryType != 1000001 && nGeometryType != 3000003)
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      Get the vertex count.                                           */
/* -------------------------------------------------------------------- */
    int         nNewNumPoints;
    
    memcpy( &nNewNumPoints, pabyData + 5, 4 );
    
    if( OGR_SWAP( eByteOrder ) )
        nNewNumPoints = CPL_SWAP32(nNewNumPoints);
    
    /* Check if the wkb stream buffer is big enough to store
     * fetched number of points.
     * 16 or 24 - size of point structure
     */
    int nPointSize = (bIs3D ? 24 : 16);
    if (nNewNumPoints < 0 || nNewNumPoints > INT_MAX / nPointSize)
        return OGRERR_CORRUPT_DATA;
    int nBufferMinSize = nPointSize * nNewNumPoints;

    if( nSize != -1 && nBufferMinSize > nSize-9 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Length of input WKB is too small" );
        return OGRERR_NOT_ENOUGH_DATA;
    }

    setNumPoints( nNewNumPoints );
    if (nPointCount < nNewNumPoints)
        return OGRERR_FAILURE;
    
    if( bIs3D )
        Make3D();
    else
        Make2D();
    
/* -------------------------------------------------------------------- */
/*      Get the vertex.                                                 */
/* -------------------------------------------------------------------- */
    int i = 0;
    
    if( bIs3D )
    {
        for( i = 0; i < nPointCount; i++ )
        {
            memcpy( paoPoints + i, pabyData + 9 + i*24, 16 );
            memcpy( padfZ + i, pabyData + 9 + 16 + i*24, 8 );
        }
    }
    else
    {
        memcpy( paoPoints, pabyData + 9, 16 * nPointCount );
    }
    
/* -------------------------------------------------------------------- */
/*      Byte swap if needed.                                            */
/* -------------------------------------------------------------------- */
    if( OGR_SWAP( eByteOrder ) )
    {
        for( i = 0; i < nPointCount; i++ )
        {
            CPL_SWAPDOUBLE( &(paoPoints[i].x) );
            CPL_SWAPDOUBLE( &(paoPoints[i].y) );
        }

        if( bIs3D )
        {
            for( i = 0; i < nPointCount; i++ )
            {
                CPL_SWAPDOUBLE( padfZ + i );
            }
        }
    }
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkb()                             */
/*                                                                      */
/*      Build a well known binary representation of this object.        */
/************************************************************************/

OGRErr  OGRCircularString::exportToWkb( OGRwkbByteOrder eByteOrder,
                               unsigned char * pabyData ) const

{
/* -------------------------------------------------------------------- */
/*      Set the byte order.                                             */
/* -------------------------------------------------------------------- */
    pabyData[0] = DB2_V72_UNFIX_BYTE_ORDER((unsigned char) eByteOrder);

/* -------------------------------------------------------------------- */
/*      Set the geometry feature type.                                  */
/* -------------------------------------------------------------------- */

    /* use SQL MM Part 3 */
    GUInt32 nGType = ( getCoordinateDimension() == 3 ) ? 3000003 : 1000001;
    
    if( eByteOrder == wkbNDR )
        nGType = CPL_LSBWORD32( nGType );
    else
        nGType = CPL_MSBWORD32( nGType );

    memcpy( pabyData + 1, &nGType, 4 );
    
/* -------------------------------------------------------------------- */
/*      Copy in the data count.                                         */
/* -------------------------------------------------------------------- */
    memcpy( pabyData+5, &nPointCount, 4 );

/* -------------------------------------------------------------------- */
/*      Copy in the raw data.                                           */
/* -------------------------------------------------------------------- */
    int         i;
    
    if( getCoordinateDimension() == 3 )
    {
        for( i = 0; i < nPointCount; i++ )
        {
            memcpy( pabyData + 9 + 24*i, paoPoints+i, 16 );
            memcpy( pabyData + 9 + 16 + 24*i, padfZ+i, 8 );
        }
    }
    else
        memcpy( pabyData+9, paoPoints, 16 * nPointCount );

/* -------------------------------------------------------------------- */
/*      Swap if needed.                                                 */
/* -------------------------------------------------------------------- */
    if( OGR_SWAP( eByteOrder ) )
    {
        int     nCount;

        nCount = CPL_SWAP32( nPointCount );
        memcpy( pabyData+5, &nCount, 4 );

        for( i = getCoordinateDimension() * nPointCount - 1; i >= 0; i-- )
        {
            CPL_SWAP64PTR( pabyData + 9 + 8 * i );
        }
    }
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                           importFromWkt()                            */
/*                                                                      */
/*      Instantiate from well known text format.  Currently this is     */
/*      `CIRCULARSTRING ( x y, x y, ...)',                                  */
/************************************************************************/

OGRErr OGRCircularString::importFromWkt( char ** ppszInput )

{
    OGRErr eErr = OGRLineString::importFromWkt(ppszInput);
    if (eErr == OGRERR_NONE)
    {
        if (nPointCount != 0 && !(nPointCount >= 3 && (nPointCount % 2) == 1))
        {
            nPointCount = 0;

            CPLFree( paoPoints );
            paoPoints = NULL;

            CPLFree( padfZ );
            padfZ = NULL;

            return OGRERR_CORRUPT_DATA;
        }
    }
    return eErr;
}

/************************************************************************/
/*                             get_Length()                             */
/*                                                                      */
/*      For now we return a simple euclidian 2D distance.               */
/************************************************************************/

double OGRCircularString::get_Length() const

{
    OGRLineString* poLine = curveToLineString(0);
    double      dfLength = poLine->get_Length();
    delete poLine;
    return dfLength;
}


/************************************************************************/
/*                            getEnvelope()                             */
/************************************************************************/

void OGRCircularString::getEnvelope( OGREnvelope * psEnvelope ) const

{
    OGRLineString* poLine = curveToLineString(0);
    poLine->getEnvelope(psEnvelope);
    delete poLine;
}

/************************************************************************/
/*                     OGRCircularString::segmentize()                  */
/************************************************************************/

/* FIXME : implementation is incorrect */

void OGRCircularString::segmentize( double dfMaxLength )
{
    if (dfMaxLength <= 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "dfMaxLength must be strictly positive");
        return;
    }

    int i;
    OGRRawPoint* paoNewPoints = NULL;
    double* padfNewZ = NULL;
    int nNewPointCount = 0;
    double dfSquareMaxLength = dfMaxLength * dfMaxLength;

    for( i = 0; i < nPointCount; i++ )
    {
        paoNewPoints = (OGRRawPoint *)
            OGRRealloc(paoNewPoints, sizeof(OGRRawPoint) * (nNewPointCount + 1));
        paoNewPoints[nNewPointCount] = paoPoints[i];

        if( getCoordinateDimension() == 3 )
        {
            padfNewZ = (double *)
                OGRRealloc(padfNewZ, sizeof(double) * (nNewPointCount + 1));
            padfNewZ[nNewPointCount] = padfZ[i];
        }

        nNewPointCount++;

        if (i == nPointCount - 1)
            break;

        double dfX = paoPoints[i+1].x - paoPoints[i].x;
        double dfY = paoPoints[i+1].y - paoPoints[i].y;
        double dfSquareDist = dfX * dfX + dfY * dfY;
        if (dfSquareDist > dfSquareMaxLength)
        {
            int nIntermediatePoints = (int)floor(sqrt(dfSquareDist / dfSquareMaxLength));
            int j;

            paoNewPoints = (OGRRawPoint *)
                OGRRealloc(paoNewPoints, sizeof(OGRRawPoint) * (nNewPointCount + nIntermediatePoints));
            if( getCoordinateDimension() == 3 )
            {
                padfNewZ = (double *)
                    OGRRealloc(padfNewZ, sizeof(double) * (nNewPointCount + nIntermediatePoints));
            }

            for(j=1;j<=nIntermediatePoints;j++)
            {
                paoNewPoints[nNewPointCount + j - 1].x = paoPoints[i].x + j * dfX / (nIntermediatePoints + 1);
                paoNewPoints[nNewPointCount + j - 1].y = paoPoints[i].y + j * dfY / (nIntermediatePoints + 1);
                if( getCoordinateDimension() == 3 )
                {
                    /* No interpolation */
                    padfNewZ[nNewPointCount + j - 1] = 0;
                }
            }

            nNewPointCount += nIntermediatePoints;
        }
    }

    OGRFree(paoPoints);
    paoPoints = paoNewPoints;
    nPointCount = nNewPointCount;

    if( getCoordinateDimension() == 3 )
    {
        OGRFree(padfZ);
        padfZ = padfNewZ;
    }
}


/************************************************************************/
/*                          curveToLineString()                         */
/************************************************************************/

OGRLineString* OGRCircularString::curveToLineString(double dfMaxAngleStepSizeDegrees) const
{
    OGRLineString* poLine = new OGRLineString();
    int i;
    for(i=0;i<nPointCount-1;i+=2)
    {
        OGRLineString* poArc = OGRGeometryFactory::curveToLineString(
                                    paoPoints[i].x, paoPoints[i].y,
                                    paoPoints[i+1].x, paoPoints[i+1].y,
                                    paoPoints[i+2].x, paoPoints[i+2].y,
                                    FALSE, dfMaxAngleStepSizeDegrees);
        poLine->addSubLineString(poArc, (i == 0) ? 0 : 1);
    }
    return poLine;
}
