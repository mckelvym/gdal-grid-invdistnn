/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Define some core portability services for cross-platform OGR code.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.12  2002/11/08 18:25:45  warmerda
 * remove extranious comma in enum, confuses HPUX compiler
 *
 * Revision 1.11  2002/11/08 15:42:41  warmerda
 * ensure type correctness of wkbFlatten
 *
 * Revision 1.10  2002/10/24 20:53:02  warmerda
 * expand tabs
 *
 * Revision 1.9  2002/09/26 18:13:17  warmerda
 * moved some defs to ogr_core.h for sharing with ogr_api.h
 *
 * Revision 1.8  2000/07/11 20:15:12  warmerda
 * apply CPL_DLL to OGR functions
 *
 * Revision 1.7  2000/07/09 20:47:35  warmerda
 * added CPL_START/END
 *
 * Revision 1.6  1999/11/18 19:02:19  warmerda
 * expanded tabs
 *
 * Revision 1.5  1999/07/07 04:23:07  danmo
 * Fixed typo in  #define _OGR_..._H_INCLUDED  line
 *
 * Revision 1.4  1999/07/05 18:56:52  warmerda
 * now includes cpl_port.h
 *
 * Revision 1.3  1999/07/05 17:19:03  warmerda
 * added OGRERR_UNSUPPORTED_SRS
 *
 * Revision 1.2  1999/05/31 15:00:37  warmerda
 * added generic OGRERR_FAILURE error code.
 *
 * Revision 1.1  1999/05/20 14:35:00  warmerda
 * New
 *
 */

#ifndef _OGR_CORE_H_INCLUDED
#define _OGR_CORE_H_INCLUDED

#include "cpl_port.h"

/**
 * Simple container for a bounding region.
 */

#ifdef __cplusplus
class OGREnvelope
{
  public:
        OGREnvelope()
        {
                MinX = MaxX = MinY = MaxY = 0;
        }
    double      MinX;
    double      MaxX;
    double      MinY;
    double      MaxY;
};
#else
typedef struct
{
    double      MinX;
    double      MaxX;
    double      MinY;
    double      MaxY;
} OGREnvelope;
#endif

CPL_C_START

void CPL_DLL *OGRMalloc( size_t );
void CPL_DLL *OGRCalloc( size_t, size_t );
void CPL_DLL *OGRRealloc( void *, size_t );
char CPL_DLL *OGRStrdup( const char * );
void CPL_DLL OGRFree( void * );

typedef int OGRErr;

#define OGRERR_NONE                0
#define OGRERR_NOT_ENOUGH_DATA     1    /* not enough data to deserialize */
#define OGRERR_NOT_ENOUGH_MEMORY   2
#define OGRERR_UNSUPPORTED_GEOMETRY_TYPE 3
#define OGRERR_UNSUPPORTED_OPERATION 4
#define OGRERR_CORRUPT_DATA        5
#define OGRERR_FAILURE             6
#define OGRERR_UNSUPPORTED_SRS     7

typedef int     OGRBoolean;

/* -------------------------------------------------------------------- */
/*      ogr_geometry.h related definitions.                             */
/* -------------------------------------------------------------------- */
/**
 * List of well known binary geometry types.  These are used within the BLOBs
 * but are also returned from OGRGeometry::getGeometryType() to identify the
 * type of a geometry object.
 */

typedef enum 
{
    wkbUnknown = 0,             // non-standard
    wkbPoint = 1,               // rest are standard WKB type codes
    wkbLineString = 2,
    wkbPolygon = 3,
    wkbMultiPoint = 4,
    wkbMultiLineString = 5,
    wkbMultiPolygon = 6,
    wkbGeometryCollection = 7,
    wkbNone = 100,              // non-standard, for pure attribute records
    wkbPoint25D = 0x80000001,       // 2.5D extensions as per 99-402
    wkbLineString25D = 0x80000002,
    wkbPolygon25D = 0x80000003,
    wkbMultiPoint25D = 0x80000004,
    wkbMultiLineString25D = 0x80000005,
    wkbMultiPolygon25D = 0x80000006,
    wkbGeometryCollection25D = 0x80000007
} OGRwkbGeometryType;

#define wkb25DBit 0x80000000
#define wkbFlatten(x)  ((OGRwkbGeometryType) ((x) & (~wkb25DBit)))

#define ogrZMarker 0x21125711

const char CPL_DLL * OGRGeometryTypeToName( OGRwkbGeometryType eType );

typedef enum 
{
    wkbXDR = 0,         /* MSB/Sun/Motoroloa: Most Significant Byte First   */
    wkbNDR = 1          /* LSB/Intel/Vax: Least Significant Byte First      */
} OGRwkbByteOrder;

/************************************************************************/
/*                  ogr_feature.h related definitions.                  */
/************************************************************************/

/**
 * List of feature field types.  This list is likely to be extended in the
 * future ... avoid coding applications based on the assumption that all
 * field types can be known.
 */

typedef enum 
{
  /** Simple 32bit integer */                   OFTInteger = 0,
  /** List of 32bit integers */                 OFTIntegerList = 1,
  /** Double Precision floating point */        OFTReal = 2,
  /** List of doubles */                        OFTRealList = 3,
  /** String of ASCII chars */                  OFTString = 4,
  /** Array of strings */                       OFTStringList = 5,
  /** Double byte string (unsupported) */       OFTWideString = 6,
  /** List of wide strings (unsupported) */     OFTWideStringList = 7,
  /** Raw Binary data (unsupported) */          OFTBinary = 8
} OGRFieldType;

/**
 * Display justification for field values.
 */

typedef enum 
{
    OJUndefined = 0,
    OJLeft = 1,
    OJRight = 2
} OGRJustification;

#define OGRNullFID            -1
#define OGRUnsetMarker        -21121

/************************************************************************/
/*                               OGRField                               */
/************************************************************************/

/**
 * OGRFeature field attribute value union.
 */

typedef union {
    int         Integer;
    double      Real;
    char       *String;
    // wchar    *WideString;
    
    struct {
        int     nCount;
        int     *paList;
    } IntegerList;
    
    struct {
        int     nCount;
        double  *paList;
    } RealList;
    
    struct {
        int     nCount;
        char    **paList;
    } StringList;

//    union {
//        int   nCount;
//        wchar *paList;
//    } WideStringList;

    struct {
        int     nMarker1;
        int     nMarker2;
    } Set;
} OGRField;

/* -------------------------------------------------------------------- */
/*      Constants from ogrsf_frmts.h for capabilities.                  */
/* -------------------------------------------------------------------- */
#define OLCRandomRead          "RandomRead"
#define OLCSequentialWrite     "SequentialWrite"
#define OLCRandomWrite         "RandomWrite"
#define OLCFastSpatialFilter   "FastSpatialFilter"
#define OLCFastFeatureCount    "FastFeatureCount"
#define OLCFastGetExtent       "FastGetExtent"
#define OLCCreateField         "CreateField"
#define OLCTransactions        "Transactions"

#define ODsCCreateLayer        "CreateLayer"

#define ODrCCreateDataSource   "CreateDataSource"

CPL_C_END

#endif /* ndef _OGR_CORE_H_INCLUDED */

