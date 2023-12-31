/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Factory for converting geometry to and from well known binary
 *           format.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
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
 ****************************************************************************/

#include "ogr_geometry.h"
#include "ogr_api.h"
#include "ogr_p.h"
#include "ogr_geos.h"

CPL_CVSID("$Id$");

#ifndef PI
#define PI  3.14159265358979323846
#endif 

/************************************************************************/
/*                           createFromWkb()                            */
/************************************************************************/

/**
 * \brief Create a geometry object of the appropriate type from it's well known binary representation.
 *
 * Note that if nBytes is passed as zero, no checking can be done on whether
 * the pabyData is sufficient.  This can result in a crash if the input
 * data is corrupt.  This function returns no indication of the number of
 * bytes from the data source actually used to represent the returned
 * geometry object.  Use OGRGeometry::WkbSize() on the returned geometry to
 * establish the number of bytes it required in WKB format.
 *
 * Also note that this is a static method, and that there
 * is no need to instantiate an OGRGeometryFactory object.  
 *
 * The C function OGR_G_CreateFromWkb() is the same as this method.
 *
 * @param pabyData pointer to the input BLOB data.
 * @param poSR pointer to the spatial reference to be assigned to the
 *             created geometry object.  This may be NULL.
 * @param ppoReturn the newly created geometry object will be assigned to the
 *                  indicated pointer on return.  This will be NULL in case
 *                  of failure.
 * @param nBytes the number of bytes available in pabyData, or -1 if it isn't
 *               known.
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

OGRErr OGRGeometryFactory::createFromWkb(unsigned char *pabyData,
                                         OGRSpatialReference * poSR,
                                         OGRGeometry **ppoReturn,
                                         int nBytes )

{
    OGRwkbGeometryType eGeometryType;
    int nGeometryType;
    OGRwkbByteOrder eByteOrder;
    OGRErr      eErr;
    OGRGeometry *poGeom;

    *ppoReturn = NULL;

    if( nBytes < 9 && nBytes != -1 )
        return OGRERR_NOT_ENOUGH_DATA;

/* -------------------------------------------------------------------- */
/*      Get the byte order byte.  The extra tests are to work around    */
/*      bug sin the WKB of DB2 v7.2 as identified by Safe Software.     */
/* -------------------------------------------------------------------- */
    eByteOrder = DB2_V72_FIX_BYTE_ORDER((OGRwkbByteOrder) *pabyData);


    if( eByteOrder != wkbXDR && eByteOrder != wkbNDR )
    {
        CPLDebug( "OGR", 
                  "OGRGeometryFactory::createFromWkb() - got corrupt data.\n"
                  "%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                  pabyData[0],
                  pabyData[1],
                  pabyData[2],
                  pabyData[3],
                  pabyData[4],
                  pabyData[5],
                  pabyData[6],
                  pabyData[7],
                  pabyData[8]);
        return OGRERR_CORRUPT_DATA;
    }

/* -------------------------------------------------------------------- */
/*      Get the geometry feature type.  For now we assume that          */
/*      geometry type is between 0 and 255 so we only have to fetch     */
/*      one byte.                                                       */
/* -------------------------------------------------------------------- */
    if( eByteOrder == wkbNDR )
        eGeometryType = (OGRwkbGeometryType) pabyData[1];
    else
        eGeometryType = (OGRwkbGeometryType) pabyData[4];

    memcpy(&nGeometryType, pabyData + 1, 4);
    if( OGR_SWAP( eByteOrder ) )
        nGeometryType = CPL_SWAP32(nGeometryType);
    if (nGeometryType == 1000001)
        eGeometryType = wkbCircularString;
    else if (nGeometryType == 3000003)
        eGeometryType = wkbCircularString25D;
    
/* -------------------------------------------------------------------- */
/*      Instantiate a geometry of the appropriate type, and             */
/*      initialize from the input stream.                               */
/* -------------------------------------------------------------------- */
    poGeom = createGeometry( eGeometryType );
    
    if( poGeom == NULL )
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;

/* -------------------------------------------------------------------- */
/*      Import from binary.                                             */
/* -------------------------------------------------------------------- */
    eErr = poGeom->importFromWkb( pabyData, nBytes );

/* -------------------------------------------------------------------- */
/*      Assign spatial reference system.                                */
/* -------------------------------------------------------------------- */
    if( eErr == OGRERR_NONE )
    {
        if ( wkbFlatten(poGeom->getGeometryType()) == wkbCircularString &&
             CSLTestBoolean(CPLGetConfigOption("OGR_STROKE_CURVE", "FALSE")) )
        {
            OGRLineString* poLine = ((OGRCircularString*)poGeom)->curveToLineString(0);
            delete poGeom;
            poGeom = poLine;
        }
        
        poGeom->assignSpatialReference( poSR );
        *ppoReturn = poGeom;
    }
    else
    {
        delete poGeom;
    }

    return eErr;
}

/************************************************************************/
/*                        OGR_G_CreateFromWkb()                         */
/************************************************************************/
/**
 * \brief Create a geometry object of the appropriate type from it's well known binary representation.
 *
 * Note that if nBytes is passed as zero, no checking can be done on whether
 * the pabyData is sufficient.  This can result in a crash if the input
 * data is corrupt.  This function returns no indication of the number of
 * bytes from the data source actually used to represent the returned
 * geometry object.  Use OGR_G_WkbSize() on the returned geometry to
 * establish the number of bytes it required in WKB format.
 *
 * The OGRGeometryFactory::createFromWkb() CPP method  is the same as this
 * function.
 *
 * @param pabyData pointer to the input BLOB data.
 * @param hSRS handle to the spatial reference to be assigned to the
 *             created geometry object.  This may be NULL.
 * @param phGeometry the newly created geometry object will 
 * be assigned to the indicated handle on return.  This will be NULL in case
 * of failure.
 * @param nBytes the number of bytes of data available in pabyData, or -1
 * if it is not known, but assumed to be sufficient.
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

OGRErr CPL_DLL OGR_G_CreateFromWkb( unsigned char *pabyData, 
                                    OGRSpatialReferenceH hSRS,
                                    OGRGeometryH *phGeometry, 
                                    int nBytes )

{
    return OGRGeometryFactory::createFromWkb( pabyData, 
                                              (OGRSpatialReference *) hSRS,
                                              (OGRGeometry **) phGeometry,
                                              nBytes );
}

/************************************************************************/
/*                           createFromWkt()                            */
/************************************************************************/

/**
 * \brief Create a geometry object of the appropriate type from it's well known text representation.
 *
 * The C function OGR_G_CreateFromWkt() is the same as this method.
 *
 * @param ppszData input zero terminated string containing well known text
 *                representation of the geometry to be created.  The pointer
 *                is updated to point just beyond that last character consumed.
 * @param poSR pointer to the spatial reference to be assigned to the
 *             created geometry object.  This may be NULL.
 * @param ppoReturn the newly created geometry object will be assigned to the
 *                  indicated pointer on return.  This will be NULL if the
 *                  method fails. 
 *
 *  <b>Example:</b>
 *
 *  <pre>
 *    const char* wkt= "POINT(0 0)";
 *  
 *    // cast because OGR_G_CreateFromWkt will move the pointer 
 *    char* pszWkt = (char*) wkt.c_str(); 
 *    OGRSpatialReferenceH ref = OSRNewSpatialReference(NULL);
 *    OGRGeometryH new_geom;
 *    OGRErr err = OGR_G_CreateFromWkt(&pszWkt, ref, &new_geom);
 *  </pre>
 *
 *
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

OGRErr OGRGeometryFactory::createFromWkt(char **ppszData,
                                         OGRSpatialReference * poSR,
                                         OGRGeometry **ppoReturn )

{
    OGRErr      eErr;
    char        szToken[OGR_WKT_TOKEN_MAX];
    char        *pszInput = *ppszData;
    OGRGeometry *poGeom;

    *ppoReturn = NULL;

/* -------------------------------------------------------------------- */
/*      Get the first token, which should be the geometry type.         */
/* -------------------------------------------------------------------- */
    if( OGRWktReadToken( pszInput, szToken ) == NULL )
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      Instantiate a geometry of the appropriate type.                 */
/* -------------------------------------------------------------------- */
    if( EQUAL(szToken,"POINT") )
    {
        poGeom = new OGRPoint();
    }

    else if( EQUAL(szToken,"LINESTRING") )
    {
        poGeom = new OGRLineString();
    }

    else if( EQUAL(szToken,"POLYGON") )
    {
        poGeom = new OGRPolygon();
    }
    
    else if( EQUAL(szToken,"GEOMETRYCOLLECTION") )
    {
        poGeom = new OGRGeometryCollection();
    }
    
    else if( EQUAL(szToken,"MULTIPOLYGON") )
    {
        poGeom = new OGRMultiPolygon();
    }

    else if( EQUAL(szToken,"MULTIPOINT") )
    {
        poGeom = new OGRMultiPoint();
    }

    else if( EQUAL(szToken,"MULTILINESTRING") )
    {
        poGeom = new OGRMultiLineString();
    }

    else if( EQUAL(szToken,"CIRCULARSTRING") )
    {
        poGeom = new OGRCircularString();
    }

    else
    {
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
    }

/* -------------------------------------------------------------------- */
/*      Do the import.                                                  */
/* -------------------------------------------------------------------- */
    eErr = poGeom->importFromWkt( &pszInput );
    
/* -------------------------------------------------------------------- */
/*      Assign spatial reference system.                                */
/* -------------------------------------------------------------------- */
    if( eErr == OGRERR_NONE )
    {
        if ( wkbFlatten(poGeom->getGeometryType()) == wkbCircularString &&
             CSLTestBoolean(CPLGetConfigOption("OGR_STROKE_CURVE", "FALSE")) )
        {
            OGRLineString* poLine = ((OGRCircularString*)poGeom)->curveToLineString(0);
            delete poGeom;
            poGeom = poLine;
        }

        poGeom->assignSpatialReference( poSR );
        *ppoReturn = poGeom;
        *ppszData = pszInput;
    }
    else
    {
        delete poGeom;
    }
    
    return eErr;
}

/************************************************************************/
/*                        OGR_G_CreateFromWkt()                         */
/************************************************************************/
/**
 * \brief Create a geometry object of the appropriate type from it's well known text representation.
 *
 * The OGRGeometryFactory::createFromWkt CPP method is the same as this
 * function.
 *
 * @param ppszData input zero terminated string containing well known text
 *                representation of the geometry to be created.  The pointer
 *                is updated to point just beyond that last character consumed.
 * @param hSRS handle to the spatial reference to be assigned to the
 *             created geometry object.  This may be NULL.
 * @param phGeometry the newly created geometry object will be assigned to the
 *                  indicated handle on return.  This will be NULL if the
 *                  method fails. 
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

OGRErr CPL_DLL OGR_G_CreateFromWkt( char **ppszData, 
                                    OGRSpatialReferenceH hSRS,
                                    OGRGeometryH *phGeometry )

{
    return OGRGeometryFactory::createFromWkt( ppszData,
                                              (OGRSpatialReference *) hSRS,
                                              (OGRGeometry **) phGeometry );
}

/************************************************************************/
/*                           createGeometry()                           */
/************************************************************************/

/** 
 * \brief Create an empty geometry of desired type.
 *
 * This is equivelent to allocating the desired geometry with new, but
 * the allocation is guaranteed to take place in the context of the 
 * GDAL/OGR heap. 
 *
 * This method is the same as the C function OGR_G_CreateGeometry().
 *
 * @param eGeometryType the type code of the geometry class to be instantiated.
 *
 * @return the newly create geometry or NULL on failure.
 */

OGRGeometry *
OGRGeometryFactory::createGeometry( OGRwkbGeometryType eGeometryType )

{
    switch( wkbFlatten(eGeometryType) )
    {
      case wkbPoint:
          return new OGRPoint();

      case wkbLineString:
          return new OGRLineString();

      case wkbPolygon:
          return new OGRPolygon();

      case wkbGeometryCollection:
          return new OGRGeometryCollection();

      case wkbMultiPolygon:
          return new OGRMultiPolygon();

      case wkbMultiPoint:
          return new OGRMultiPoint();

      case wkbMultiLineString:
          return new OGRMultiLineString();

      case wkbLinearRing:
          return new OGRLinearRing();

      case wkbCircularString:
          return new OGRCircularString();

      default:
          return NULL;
    }
}

/************************************************************************/
/*                        OGR_G_CreateGeometry()                        */
/************************************************************************/
/** 
 * \brief Create an empty geometry of desired type.
 *
 * This is equivelent to allocating the desired geometry with new, but
 * the allocation is guaranteed to take place in the context of the 
 * GDAL/OGR heap. 
 *
 * This function is the same as the CPP method 
 * OGRGeometryFactory::createGeometry.
 *
 * @param eGeometryType the type code of the geometry to be created.
 *
 * @return handle to the newly create geometry or NULL on failure.
 */

OGRGeometryH OGR_G_CreateGeometry( OGRwkbGeometryType eGeometryType )

{
    return (OGRGeometryH) OGRGeometryFactory::createGeometry( eGeometryType );
}


/************************************************************************/
/*                          destroyGeometry()                           */
/************************************************************************/

/**
 * \brief Destroy geometry object.
 *
 * Equivalent to invoking delete on a geometry, but it guaranteed to take 
 * place within the context of the GDAL/OGR heap.
 *
 * This method is the same as the C function OGR_G_DestroyGeometry().
 *
 * @param poGeom the geometry to deallocate.
 */

void OGRGeometryFactory::destroyGeometry( OGRGeometry *poGeom )

{
    delete poGeom;
}


/************************************************************************/
/*                        OGR_G_DestroyGeometry()                       */
/************************************************************************/
/**
 * \brief Destroy geometry object.
 *
 * Equivalent to invoking delete on a geometry, but it guaranteed to take 
 * place within the context of the GDAL/OGR heap.
 *
 * This function is the same as the CPP method 
 * OGRGeometryFactory::destroyGeometry.
 *
 * @param hGeom handle to the geometry to delete.
 */

void OGR_G_DestroyGeometry( OGRGeometryH hGeom )

{
    OGRGeometryFactory::destroyGeometry( (OGRGeometry *) hGeom );
}

/************************************************************************/
/*                           forceToPolygon()                           */
/************************************************************************/

/**
 * \brief Convert to polygon.
 *
 * Tries to force the provided geometry to be a polygon.  Currently
 * this just effects a change on multipolygons.  The passed in geometry is
 * consumed and a new one returned (or potentially the same one). 
 * 
 * @param poGeom the input geometry - ownership is passed to the method.
 * @return new geometry.
 */

OGRGeometry *OGRGeometryFactory::forceToPolygon( OGRGeometry *poGeom )

{
    if( poGeom == NULL )
        return NULL;

    OGRwkbGeometryType eGeomType = wkbFlatten(poGeom->getGeometryType());

    if( eGeomType != wkbGeometryCollection
        && eGeomType != wkbMultiPolygon )
        return poGeom;

    // build an aggregated polygon from all the polygon rings in the container.
    OGRPolygon *poPolygon = new OGRPolygon();
    OGRGeometryCollection *poGC = (OGRGeometryCollection *) poGeom;
    int iGeom;

    for( iGeom = 0; iGeom < poGC->getNumGeometries(); iGeom++ )
    {
        if( wkbFlatten(poGC->getGeometryRef(iGeom)->getGeometryType()) 
            != wkbPolygon )
            continue;

        OGRPolygon *poOldPoly = (OGRPolygon *) poGC->getGeometryRef(iGeom);
        int   iRing;
        
        if( poOldPoly->getExteriorRing() == NULL )
            continue;

        poPolygon->addRing( poOldPoly->getExteriorRing() );

        for( iRing = 0; iRing < poOldPoly->getNumInteriorRings(); iRing++ )
            poPolygon->addRing( poOldPoly->getInteriorRing( iRing ) );
    }
    
    delete poGC;

    return poPolygon;
}

/************************************************************************/
/*                        OGR_G_ForceToPolygon()                        */
/************************************************************************/

/**
 * \brief Convert to polygon.
 *
 * This function is the same as the C++ method 
 * OGRGeometryFactory::forceToPolygon().
 *
 * @param hGeom handle to the geometry to convert (ownership surrendered).
 * @return the converted geometry (ownership to caller).
 *
 * @since GDAL/OGR 1.8.0
 */

OGRGeometryH OGR_G_ForceToPolygon( OGRGeometryH hGeom )

{
    return (OGRGeometryH) 
        OGRGeometryFactory::forceToPolygon( (OGRGeometry *) hGeom );
}

/************************************************************************/
/*                        forceToMultiPolygon()                         */
/************************************************************************/

/**
 * \brief Convert to multipolygon.
 *
 * Tries to force the provided geometry to be a multipolygon.  Currently
 * this just effects a change on polygons.  The passed in geometry is
 * consumed and a new one returned (or potentially the same one). 
 * 
 * @return new geometry.
 */

OGRGeometry *OGRGeometryFactory::forceToMultiPolygon( OGRGeometry *poGeom )

{
    if( poGeom == NULL )
        return NULL;

    OGRwkbGeometryType eGeomType = wkbFlatten(poGeom->getGeometryType());

/* -------------------------------------------------------------------- */
/*      If this is already a MultiPolygon, nothing to do                */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbMultiPolygon )
    {
        return poGeom;
    }

/* -------------------------------------------------------------------- */
/*      Check for the case of a geometrycollection that can be          */
/*      promoted to MultiPolygon.                                       */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbGeometryCollection )
    {
        int iGeom;
        int bAllPoly = TRUE;
        OGRGeometryCollection *poGC = (OGRGeometryCollection *) poGeom;

        for( iGeom = 0; iGeom < poGC->getNumGeometries(); iGeom++ )
        {
            if( wkbFlatten(poGC->getGeometryRef(iGeom)->getGeometryType())
                != wkbPolygon )
                bAllPoly = FALSE;
        }

        if( !bAllPoly )
            return poGeom;
        
        OGRMultiPolygon *poMP = new OGRMultiPolygon();

        while( poGC->getNumGeometries() > 0 )
        {
            poMP->addGeometryDirectly( poGC->getGeometryRef(0) );
            poGC->removeGeometry( 0, FALSE );
        }

        delete poGC;

        return poMP;
    }

/* -------------------------------------------------------------------- */
/*      Eventually we should try to split the polygon into component    */
/*      island polygons.  But thats alot of work and can be put off.    */
/* -------------------------------------------------------------------- */
    if( eGeomType != wkbPolygon )
        return poGeom;

    OGRMultiPolygon *poMP = new OGRMultiPolygon();
    poMP->addGeometryDirectly( poGeom );

    return poMP;
}

/************************************************************************/
/*                     OGR_G_ForceToMultiPolygon()                      */
/************************************************************************/

/**
 * \brief Convert to multipolygon.
 *
 * This function is the same as the C++ method 
 * OGRGeometryFactory::forceToMultiPolygon().
 *
 * @param hGeom handle to the geometry to convert (ownership surrendered).
 * @return the converted geometry (ownership to caller).
 *
 * @since GDAL/OGR 1.8.0
 */

OGRGeometryH OGR_G_ForceToMultiPolygon( OGRGeometryH hGeom )

{
    return (OGRGeometryH) 
        OGRGeometryFactory::forceToMultiPolygon( (OGRGeometry *) hGeom );
}

/************************************************************************/
/*                        forceToMultiPoint()                           */
/************************************************************************/

/**
 * \brief Convert to multipoint.
 *
 * Tries to force the provided geometry to be a multipoint.  Currently
 * this just effects a change on points.  The passed in geometry is
 * consumed and a new one returned (or potentially the same one). 
 * 
 * @return new geometry.
 */

OGRGeometry *OGRGeometryFactory::forceToMultiPoint( OGRGeometry *poGeom )

{
    if( poGeom == NULL )
        return NULL;

    OGRwkbGeometryType eGeomType = wkbFlatten(poGeom->getGeometryType());

/* -------------------------------------------------------------------- */
/*      If this is already a MultiPoint, nothing to do                  */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbMultiPoint )
    {
        return poGeom;
    }

/* -------------------------------------------------------------------- */
/*      Check for the case of a geometrycollection that can be          */
/*      promoted to MultiPoint.                                         */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbGeometryCollection )
    {
        int iGeom;
        int bAllPoint = TRUE;
        OGRGeometryCollection *poGC = (OGRGeometryCollection *) poGeom;

        for( iGeom = 0; iGeom < poGC->getNumGeometries(); iGeom++ )
        {
            if( wkbFlatten(poGC->getGeometryRef(iGeom)->getGeometryType())
                != wkbPoint )
                bAllPoint = FALSE;
        }

        if( !bAllPoint )
            return poGeom;
        
        OGRMultiPoint *poMP = new OGRMultiPoint();

        while( poGC->getNumGeometries() > 0 )
        {
            poMP->addGeometryDirectly( poGC->getGeometryRef(0) );
            poGC->removeGeometry( 0, FALSE );
        }

        delete poGC;

        return poMP;
    }

    if( eGeomType != wkbPoint )
        return poGeom;

    OGRMultiPoint *poMP = new OGRMultiPoint();
    poMP->addGeometryDirectly( poGeom );

    return poMP;
}

/************************************************************************/
/*                      OGR_G_ForceToMultiPoint()                       */
/************************************************************************/

/**
 * \brief Convert to multipoint.
 *
 * This function is the same as the C++ method 
 * OGRGeometryFactory::forceToMultiPoint().
 *
 * @param hGeom handle to the geometry to convert (ownership surrendered).
 * @return the converted geometry (ownership to caller).
 *
 * @since GDAL/OGR 1.8.0
 */

OGRGeometryH OGR_G_ForceToMultiPoint( OGRGeometryH hGeom )

{
    return (OGRGeometryH) 
        OGRGeometryFactory::forceToMultiPoint( (OGRGeometry *) hGeom );
}

/************************************************************************/
/*                        forceToMultiLinestring()                      */
/************************************************************************/

/**
 * \brief Convert to multilinestring.
 *
 * Tries to force the provided geometry to be a multilinestring.
 *
 * - linestrings are placed in a multilinestring.
 * - geometry collections will be converted to multilinestring if they only 
 * contain linestrings.
 * - polygons will be changed to a collection of linestrings (one per ring).
 *
 * The passed in geometry is
 * consumed and a new one returned (or potentially the same one). 
 * 
 * @return new geometry.
 */

OGRGeometry *OGRGeometryFactory::forceToMultiLineString( OGRGeometry *poGeom )

{
    if( poGeom == NULL )
        return NULL;

    OGRwkbGeometryType eGeomType = wkbFlatten(poGeom->getGeometryType());

/* -------------------------------------------------------------------- */
/*      If this is already a MultiLineString, nothing to do             */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbMultiLineString )
    {
        return poGeom;
    }

/* -------------------------------------------------------------------- */
/*      Check for the case of a geometrycollection that can be          */
/*      promoted to MultiLineString.                                    */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbGeometryCollection )
    {
        int iGeom;
        int bAllLines = TRUE;
        OGRGeometryCollection *poGC = (OGRGeometryCollection *) poGeom;

        for( iGeom = 0; iGeom < poGC->getNumGeometries(); iGeom++ )
        {
            if( wkbFlatten(poGC->getGeometryRef(iGeom)->getGeometryType())
                != wkbLineString )
                bAllLines = FALSE;
        }

        if( !bAllLines )
            return poGeom;
        
        OGRMultiLineString *poMP = new OGRMultiLineString();

        while( poGC->getNumGeometries() > 0 )
        {
            poMP->addGeometryDirectly( poGC->getGeometryRef(0) );
            poGC->removeGeometry( 0, FALSE );
        }

        delete poGC;

        return poMP;
    }

/* -------------------------------------------------------------------- */
/*      Turn a linestring into a multilinestring.                       */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbLineString )
    {
        OGRMultiLineString *poMP = new OGRMultiLineString();
        poMP->addGeometryDirectly( poGeom );
        return poMP;
    }

/* -------------------------------------------------------------------- */
/*      Convert polygons into a multilinestring.                        */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbPolygon )
    {
        OGRMultiLineString *poMP = new OGRMultiLineString();
        OGRPolygon *poPoly = (OGRPolygon *) poGeom;
        int iRing;

        for( iRing = 0; iRing < poPoly->getNumInteriorRings()+1; iRing++ )
        {
            OGRLineString *poNewLS, *poLR;

            if( iRing == 0 )
            {
                poLR = poPoly->getExteriorRing();
                if( poLR == NULL )
                    break;
            }
            else
                poLR = poPoly->getInteriorRing(iRing-1);

            if (poLR == NULL || poLR->getNumPoints() == 0)
                continue;

            poNewLS = new OGRLineString();
            poNewLS->addSubLineString( poLR );
            poMP->addGeometryDirectly( poNewLS );
        }
        
        delete poPoly;

        return poMP;
    }

/* -------------------------------------------------------------------- */
/*      Convert multi-polygons into a multilinestring.                  */
/* -------------------------------------------------------------------- */
    if( eGeomType == wkbMultiPolygon )
    {
        OGRMultiLineString *poMP = new OGRMultiLineString();
        OGRMultiPolygon *poMPoly = (OGRMultiPolygon *) poGeom;
        int iPoly;

        for( iPoly = 0; iPoly < poMPoly->getNumGeometries(); iPoly++ )
        {
            OGRPolygon *poPoly = (OGRPolygon*) poMPoly->getGeometryRef(iPoly);
            int iRing;

            for( iRing = 0; iRing < poPoly->getNumInteriorRings()+1; iRing++ )
            {
                OGRLineString *poNewLS, *poLR;
                
                if( iRing == 0 )
                {
                    poLR = poPoly->getExteriorRing();
                    if( poLR == NULL )
                        break;
                }
                else
                    poLR = poPoly->getInteriorRing(iRing-1);
    
                if (poLR == NULL || poLR->getNumPoints() == 0)
                    continue;
                
                poNewLS = new OGRLineString();
                poNewLS->addSubLineString( poLR );
                poMP->addGeometryDirectly( poNewLS );
            }
        }
        delete poMPoly;

        return poMP;
    }

    return poGeom;
}

/************************************************************************/
/*                    OGR_G_ForceToMultiLineString()                    */
/************************************************************************/

/**
 * \brief Convert to multilinestring.
 *
 * This function is the same as the C++ method 
 * OGRGeometryFactory::forceToMultiLineString().
 *
 * @param hGeom handle to the geometry to convert (ownership surrendered).
 * @return the converted geometry (ownership to caller).
 *
 * @since GDAL/OGR 1.8.0
 */

OGRGeometryH OGR_G_ForceToMultiLineString( OGRGeometryH hGeom )

{
    return (OGRGeometryH) 
        OGRGeometryFactory::forceToMultiLineString( (OGRGeometry *) hGeom );
}

/************************************************************************/
/*                          organizePolygons()                          */
/************************************************************************/

typedef struct _sPolyExtended sPolyExtended;

struct _sPolyExtended
{
    OGRPolygon*     poPolygon;
    OGREnvelope     sEnvelope;
    OGRLinearRing*  poExteriorRing;
    OGRPoint        poAPoint;
    int             nInitialIndex;
    int             bIsTopLevel;
    OGRPolygon*     poEnclosingPolygon;
    double          dfArea;
    int             bIsCW;
};

static int OGRGeometryFactoryCompareArea(const void* p1, const void* p2)
{
    const sPolyExtended* psPoly1 = (const sPolyExtended*) p1;
    const sPolyExtended* psPoly2 = (const sPolyExtended*) p2;
    if (psPoly2->dfArea < psPoly1->dfArea)
        return -1;
    else if (psPoly2->dfArea > psPoly1->dfArea)
        return 1;
    else
        return 0;
}

static int OGRGeometryFactoryCompareByIndex(const void* p1, const void* p2)
{
    const sPolyExtended* psPoly1 = (const sPolyExtended*) p1;
    const sPolyExtended* psPoly2 = (const sPolyExtended*) p2;
    if (psPoly1->nInitialIndex < psPoly2->nInitialIndex)
        return -1;
    else if (psPoly1->nInitialIndex > psPoly2->nInitialIndex)
        return 1;
    else
        return 0;
}

#define N_CRITICAL_PART_NUMBER   100

typedef enum
{
   METHOD_NORMAL,
   METHOD_SKIP,
   METHOD_ONLY_CCW
} OrganizePolygonMethod;

/**
 * \brief Organize polygons based on geometries.
 *
 * Analyse a set of rings (passed as simple polygons), and based on a 
 * geometric analysis convert them into a polygon with inner rings, 
 * or a MultiPolygon if dealing with more than one polygon.
 *
 * All the input geometries must be OGRPolygons with only a valid exterior
 * ring (at least 4 points) and no interior rings. 
 *
 * The passed in geometries become the responsibility of the method, but the
 * papoPolygons "pointer array" remains owned by the caller.
 *
 * For faster computation, a polygon is considered to be inside
 * another one if a single point of its external ring is included into the other one.
 * (unless 'OGR_DEBUG_ORGANIZE_POLYGONS' configuration option is set to TRUE.
 * In that case, a slower algorithm that tests exact topological relationships 
 * is used if GEOS is available.)
 *
 * In cases where a big number of polygons is passed to this function, the default processing
 * may be really slow. You can skip the processing by adding METHOD=SKIP
 * to the option list (the result of the function will be a multi-polygon with all polygons
 * as toplevel polygons) or only make it analyze counterclockwise polygons by adding
 * METHOD=ONLY_CCW to the option list if you can assume that the outline
 * of holes is counterclockwise defined (this is the convention for shapefiles e.g.)
 *
 * If the OGR_ORGANIZE_POLYGONS configuration option is defined, its value will override
 * the value of the METHOD option of papszOptions (usefull to modify the behaviour of the
 * shapefile driver)
 *
 * @param papoPolygons array of geometry pointers - should all be OGRPolygons.
 * Ownership of the geometries is passed, but not of the array itself.
 * @param nPolygonCount number of items in papoPolygons
 * @param pbIsValidGeometry value will be set TRUE if result is valid or 
 * FALSE otherwise. 
 * @param papszOptions a list of strings for passing options
 *
 * @return a single resulting geometry (either OGRPolygon or OGRMultiPolygon).
 */

OGRGeometry* OGRGeometryFactory::organizePolygons( OGRGeometry **papoPolygons,
                                                   int nPolygonCount,
                                                   int *pbIsValidGeometry,
                                                   const char** papszOptions )
{
    int bUseFastVersion;
    int i, j;
    OGRGeometry* geom = NULL;
    OrganizePolygonMethod method = METHOD_NORMAL;

/* -------------------------------------------------------------------- */
/*      Trivial case of a single polygon.                               */
/* -------------------------------------------------------------------- */
    if (nPolygonCount == 1)
    {
        geom = papoPolygons[0];
        papoPolygons[0] = NULL;

        if( pbIsValidGeometry )
            *pbIsValidGeometry = TRUE;

        return geom;
    }

    if (CSLTestBoolean(CPLGetConfigOption("OGR_DEBUG_ORGANIZE_POLYGONS", "NO")))
    {
        /* -------------------------------------------------------------------- */
        /*      A wee bit of a warning.                                         */
        /* -------------------------------------------------------------------- */
        static int firstTime = 1;
        if (!haveGEOS() && firstTime)
        {
            CPLDebug("OGR",
                    "In OGR_DEBUG_ORGANIZE_POLYGONS mode, GDAL should be built with GEOS support enabled in order "
                    "OGRGeometryFactory::organizePolygons to provide reliable results on complex polygons.");
            firstTime = 0;
        }
        bUseFastVersion = !haveGEOS();
    }
    else
    {
        bUseFastVersion = TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Setup per polygon envelope and area information.                */
/* -------------------------------------------------------------------- */
    sPolyExtended* asPolyEx = new sPolyExtended[nPolygonCount];

    int go_on = TRUE;
    int bMixedUpGeometries = FALSE;
    int bNonPolygon = FALSE;
    int bFoundCCW = FALSE;

    const char* pszMethodValue = CSLFetchNameValue( (char**)papszOptions, "METHOD" );
    const char* pszMethodValueOption = CPLGetConfigOption("OGR_ORGANIZE_POLYGONS", NULL);
    if (pszMethodValueOption != NULL && pszMethodValueOption[0] != '\0')
        pszMethodValue = pszMethodValueOption;

    if (pszMethodValue != NULL)
    {
        if (EQUAL(pszMethodValue, "SKIP"))
        {
            method = METHOD_SKIP;
            bMixedUpGeometries = TRUE;
        }
        else if (EQUAL(pszMethodValue, "ONLY_CCW"))
        {
            method = METHOD_ONLY_CCW;
        }
        else if (!EQUAL(pszMethodValue, "DEFAULT"))
        {
            CPLError(CE_Warning, CPLE_AppDefined,
                     "Unrecognized value for METHOD option : %s", pszMethodValue);
        }
    }

    int nCountCWPolygon = 0;
    int indexOfCWPolygon = -1;

    for(i=0;i<nPolygonCount;i++)
    {
        asPolyEx[i].nInitialIndex = i;
        asPolyEx[i].poPolygon = (OGRPolygon*)papoPolygons[i];
        papoPolygons[i]->getEnvelope(&asPolyEx[i].sEnvelope);

        if( wkbFlatten(papoPolygons[i]->getGeometryType()) == wkbPolygon
            && ((OGRPolygon *) papoPolygons[i])->getNumInteriorRings() == 0
            && ((OGRPolygon *)papoPolygons[i])->getExteriorRing()->getNumPoints() >= 4)
        {
            asPolyEx[i].dfArea = asPolyEx[i].poPolygon->get_Area();
            asPolyEx[i].poExteriorRing = asPolyEx[i].poPolygon->getExteriorRing();
            asPolyEx[i].poExteriorRing->getPoint(0, &asPolyEx[i].poAPoint);
            asPolyEx[i].bIsCW = asPolyEx[i].poExteriorRing->isClockwise();
            if (asPolyEx[i].bIsCW)
            {
                indexOfCWPolygon = i;
                nCountCWPolygon ++;
            }
            if (!bFoundCCW)
                bFoundCCW = ! (asPolyEx[i].bIsCW);
        }
        else
        {
            if( !bMixedUpGeometries )
            {
                CPLError( 
                    CE_Warning, CPLE_AppDefined, 
                    "organizePolygons() received an unexpected geometry.\n"
                    "Either a polygon with interior rings, or a polygon with less than 4 points,\n"
                    "or a non-Polygon geometry.  Return arguments as a collection." );
                bMixedUpGeometries = TRUE;
            }
            if( wkbFlatten(papoPolygons[i]->getGeometryType()) != wkbPolygon )
                bNonPolygon = TRUE;
        }
    }

    /* If we are in ONLY_CCW mode and that we have found that there is only one outer ring, */
    /* then it is pretty easy : we can assume that all other rings are inside */
    if (method == METHOD_ONLY_CCW && nCountCWPolygon == 1 && bUseFastVersion)
    {
        geom = asPolyEx[indexOfCWPolygon].poPolygon;
        for(i=0; i<nPolygonCount; i++)
        {
            if (i != indexOfCWPolygon)
            {
                ((OGRPolygon*)geom)->addRing(asPolyEx[i].poPolygon->getExteriorRing());
                delete asPolyEx[i].poPolygon;
            }
        }
        delete [] asPolyEx;
        if (pbIsValidGeometry)
            *pbIsValidGeometry = TRUE;
        return geom;
    }

    /* Emits a warning if the number of parts is sufficiently big to anticipate for */
    /* very long computation time, and the user didn't specify an explicit method */
    if (nPolygonCount > N_CRITICAL_PART_NUMBER && method == METHOD_NORMAL && pszMethodValue == NULL)
    {
        static int firstTime = 1;
        if (firstTime)
        {
            if (bFoundCCW)
            {
                CPLError( CE_Warning, CPLE_AppDefined, 
                     "organizePolygons() received a polygon with more than %d parts. "
                     "The processing may be really slow.\n"
                     "You can skip the processing by setting METHOD=SKIP, "
                     "or only make it analyze counter-clock wise parts by setting "
                     "METHOD=ONLY_CCW if you can assume that the "
                     "outline of holes is counter-clock wise defined", N_CRITICAL_PART_NUMBER);
            }
            else
            {
                CPLError( CE_Warning, CPLE_AppDefined, 
                        "organizePolygons() received a polygon with more than %d parts. "
                        "The processing may be really slow.\n"
                        "You can skip the processing by setting METHOD=SKIP.",
                        N_CRITICAL_PART_NUMBER);
            }
            firstTime = 0;
        }
    }


    /* This a several steps algorithm :
       1) Sort polygons by descending areas
       2) For each polygon of rank i, find its smallest enclosing polygon
          among the polygons of rank [i-1 ... 0]. If there are no such polygon,
          this is a toplevel polygon. Otherwise, depending on if the enclosing
          polygon is toplevel or not, we can decide if we are toplevel or not
       3) Re-sort the polygons to retrieve their inital order (nicer for some applications)
       4) For each non toplevel polygon (= inner ring), add it to its outer ring
       5) Add the toplevel polygons to the multipolygon

       Complexity : O(nPolygonCount^2)
    */

    /* Compute how each polygon relate to the other ones
       To save a bit of computation we always begin the computation by a test 
       on the enveloppe. We also take into account the areas to avoid some 
       useless tests.  (A contains B implies envelop(A) contains envelop(B) 
       and area(A) > area(B)) In practise, we can hope that few full geometry 
       intersection of inclusion test is done:
       * if the polygons are well separated geographically (a set of islands 
       for example), no full geometry intersection or inclusion test is done. 
       (the envelopes don't intersect each other)

       * if the polygons are 'lake inside an island inside a lake inside an 
       area' and that each polygon is much smaller than its enclosing one, 
       their bounding boxes are stricly contained into each oter, and thus, 
       no full geometry intersection or inclusion test is done
    */

    if (!bMixedUpGeometries)
    {
        /* STEP 1 : Sort polygons by descending area */
        qsort(asPolyEx, nPolygonCount, sizeof(sPolyExtended), OGRGeometryFactoryCompareArea);
    }
    papoPolygons = NULL; /* just to use to avoid it afterwards */

/* -------------------------------------------------------------------- */
/*      Compute relationships, if things seem well structured.          */
/* -------------------------------------------------------------------- */

    /* The first (largest) polygon is necessarily top-level */
    asPolyEx[0].bIsTopLevel = TRUE;
    asPolyEx[0].poEnclosingPolygon = NULL;

    int nCountTopLevel = 1;

    /* STEP 2 */
    for(i=1; !bMixedUpGeometries && go_on && i<nPolygonCount; i++)
    {
        if (method == METHOD_ONLY_CCW && asPolyEx[i].bIsCW)
        {
            nCountTopLevel ++;
            asPolyEx[i].bIsTopLevel = TRUE;
            asPolyEx[i].poEnclosingPolygon = NULL;
            continue;
        }

        for(j=i-1; go_on && j>=0;j--)
        {
            int b_i_inside_j = FALSE;

            if (method == METHOD_ONLY_CCW && asPolyEx[j].bIsCW == FALSE)
            {
                /* In that mode, i which is CCW if we reach here can only be */
                /* included in a CW polygon */
                continue;
            }

            if (asPolyEx[j].sEnvelope.Contains(asPolyEx[i].sEnvelope))
            {
                if (bUseFastVersion)
                {
                    /* Note that isPointInRing only test strict inclusion in the ring */
                    if (asPolyEx[j].poExteriorRing->isPointInRing(&asPolyEx[i].poAPoint, FALSE))
                    {
                        b_i_inside_j = TRUE;
                    }
                    else if (asPolyEx[j].poExteriorRing->isPointOnRingBoundary(&asPolyEx[i].poAPoint, FALSE))
                    {
                        /* If the point of i is on the boundary of j, we will iterate over the other points of i */
                        int k, nPoints = asPolyEx[i].poExteriorRing->getNumPoints();
                        for(k=1;k<nPoints;k++)
                        {
                            OGRPoint point;
                            asPolyEx[i].poExteriorRing->getPoint(k, &point);
                            if (asPolyEx[j].poExteriorRing->isPointInRing(&point, FALSE))
                            {
                                /* If then point is strictly included in j, then i is considered inside j */
                                b_i_inside_j = TRUE;
                                break;
                            }
                            else if (asPolyEx[j].poExteriorRing->isPointOnRingBoundary(&point, FALSE))
                            {
                                /* If it is on the boundary of j, iterate again */ 
                            }
                            else
                            {
                                /* If it is outside, then i cannot be inside j */
                                break;
                            }
                        }
                    }
                }
                else if (asPolyEx[j].poPolygon->Contains(asPolyEx[i].poPolygon))
                {
                    b_i_inside_j = TRUE;
                }
            }


            if (b_i_inside_j)
            {
                if (asPolyEx[j].bIsTopLevel)
                {
                    /* We are a lake */
                    asPolyEx[i].bIsTopLevel = FALSE;
                    asPolyEx[i].poEnclosingPolygon = asPolyEx[j].poPolygon;
                }
                else
                {
                    /* We are included in a something not toplevel (a lake), */
                    /* so in OGCSF we are considered as toplevel too */
                    nCountTopLevel ++;
                    asPolyEx[i].bIsTopLevel = TRUE;
                    asPolyEx[i].poEnclosingPolygon = NULL;
                }
                break;
            }
            /* We use Overlaps instead of Intersects to be more 
               tolerant about touching polygons */ 
            else if ( bUseFastVersion || !asPolyEx[i].sEnvelope.Intersects(asPolyEx[j].sEnvelope)
                     || !asPolyEx[i].poPolygon->Overlaps(asPolyEx[j].poPolygon) )
            {

            }
            else
            {
                /* Bad... The polygons are intersecting but no one is
                   contained inside the other one. This is a really broken
                   case. We just make a multipolygon with the whole set of
                   polygons */
                go_on = FALSE;
#ifdef DEBUG
                char* wkt1;
                char* wkt2;
                asPolyEx[i].poPolygon->exportToWkt(&wkt1);
                asPolyEx[j].poPolygon->exportToWkt(&wkt2);
                CPLDebug( "OGR", 
                          "Bad intersection for polygons %d and %d\n"
                          "geom %d: %s\n"
                          "geom %d: %s", 
                          i, j, i, wkt1, j, wkt2 );
                CPLFree(wkt1);
                CPLFree(wkt2);
#endif
            }
        }

        if (j < 0)
        {
            /* We come here because we are not included in anything */
            /* We are toplevel */
            nCountTopLevel ++;
            asPolyEx[i].bIsTopLevel = TRUE;
            asPolyEx[i].poEnclosingPolygon = NULL;
        }
    }

    if (pbIsValidGeometry)
        *pbIsValidGeometry = go_on && !bMixedUpGeometries;

/* -------------------------------------------------------------------- */
/*      Things broke down - just turn everything into a multipolygon.   */
/* -------------------------------------------------------------------- */
    if ( !go_on || bMixedUpGeometries )
    {
        if( bNonPolygon )
            geom = new OGRGeometryCollection();
        else
            geom = new OGRMultiPolygon();

        for( i=0; i < nPolygonCount; i++ )
        {
            ((OGRGeometryCollection*)geom)->
                addGeometryDirectly( asPolyEx[i].poPolygon );
        }
    }

/* -------------------------------------------------------------------- */
/*      Try to turn into one or more polygons based on the ring         */
/*      relationships.                                                  */
/* -------------------------------------------------------------------- */
    else
    {
        /* STEP 3: Resort in initial order */
        qsort(asPolyEx, nPolygonCount, sizeof(sPolyExtended), OGRGeometryFactoryCompareByIndex);

        /* STEP 4: Add holes as rings of their enclosing polygon */
        for(i=0;i<nPolygonCount;i++)
        {
            if (asPolyEx[i].bIsTopLevel == FALSE)
            {
                asPolyEx[i].poEnclosingPolygon->addRing(
                    asPolyEx[i].poPolygon->getExteriorRing());
                delete asPolyEx[i].poPolygon;
            }
            else if (nCountTopLevel == 1)
            {
                geom = asPolyEx[i].poPolygon;
            }
        }

        /* STEP 5: Add toplevel polygons */
        if (nCountTopLevel > 1)
        {
            for(i=0;i<nPolygonCount;i++)
            {
                if (asPolyEx[i].bIsTopLevel)
                {
                    if (geom == NULL)
                    {
                        geom = new OGRMultiPolygon();
                        ((OGRMultiPolygon*)geom)->addGeometryDirectly(asPolyEx[i].poPolygon);
                    }
                    else
                    {
                        ((OGRMultiPolygon*)geom)->addGeometryDirectly(asPolyEx[i].poPolygon);
                    }
                }
            }
        }
    }

    delete[] asPolyEx;

    return geom;
}

/************************************************************************/
/*                           createFromGML()                            */
/************************************************************************/

/**
 * \brief Create geometry from GML.
 *
 * This method translates a fragment of GML containing only the geometry
 * portion into a corresponding OGRGeometry.  There are many limitations
 * on the forms of GML geometries supported by this parser, but they are
 * too numerous to list here. 
 *
 * The following GML2 elements are parsed : Point, LineString, Polygon,
 * MultiPoint, MultiLineString, MultiPolygon, MultiGeometry.
 *
 * (OGR >= 1.8.0) The following GML3 elements are parsed : Surface, MultiSurface,
 * PolygonPatch, Triangle, Rectangle, Curve, MultiCurve, LineStringSegment, Arc,
 * Circle, CompositeSurface, OrientableSurface, Solid, Tin, TriangulatedSurface.
 *
 * Arc and Circle elements are stroked to linestring, by using a
 * 4 degrees step, unless the user has overridden the value with the
 * OGR_ARC_STEPSIZE configuration variable.
 *
 * The C function OGR_G_CreateFromGML() is the same as this method.
 *
 * @param pszData The GML fragment for the geometry.
 *
 * @return a geometry on succes, or NULL on error.  
 */

OGRGeometry *OGRGeometryFactory::createFromGML( const char *pszData )

{
    OGRGeometryH hGeom;

    hGeom = OGR_G_CreateFromGML( pszData );
    
    return (OGRGeometry *) hGeom;
}

/************************************************************************/
/*                           createFromGEOS()                           */
/************************************************************************/

OGRGeometry *
OGRGeometryFactory::createFromGEOS( GEOSGeom geosGeom )

{
#ifndef HAVE_GEOS 

    CPLError( CE_Failure, CPLE_NotSupported, 
              "GEOS support not enabled." );
    return NULL;

#else

    size_t nSize = 0;
    unsigned char *pabyBuf = NULL;
    OGRGeometry *poGeometry = NULL;

    /* Special case as POINT EMPTY cannot be translated to WKB */
    if (GEOSGeomTypeId(geosGeom) == GEOS_POINT &&
        GEOSisEmpty(geosGeom))
        return new OGRPoint();

#if GEOS_VERSION_MAJOR > 3 || (GEOS_VERSION_MAJOR == 3 && GEOS_VERSION_MINOR >= 3)
    /* GEOSGeom_getCoordinateDimension only available in GEOS 3.3.0 (unreleased at time of writing) */
    int nCoordDim = GEOSGeom_getCoordinateDimension(geosGeom);
    GEOSWKBWriter* wkbwriter = GEOSWKBWriter_create();
    GEOSWKBWriter_setOutputDimension(wkbwriter, nCoordDim);
    pabyBuf = GEOSWKBWriter_write( wkbwriter, geosGeom, &nSize );
    GEOSWKBWriter_destroy(wkbwriter);
#else
    pabyBuf = GEOSGeomToWKB_buf( geosGeom, &nSize );
#endif
    if( pabyBuf == NULL || nSize == 0 )
    {
        return NULL;
    }

    if( OGRGeometryFactory::createFromWkb( (unsigned char *) pabyBuf, 
                                           NULL, &poGeometry, (int) nSize )
        != OGRERR_NONE )
    {
        poGeometry = NULL;
    }

    if( pabyBuf != NULL )
    {
        /* Since GEOS 3.1.1, so we test 3.2.0 */
#if GEOS_CAPI_VERSION_MAJOR >= 2 || (GEOS_CAPI_VERSION_MAJOR == 1 && GEOS_CAPI_VERSION_MINOR >= 6)
        GEOSFree( pabyBuf );
#else
        free( pabyBuf );
#endif
    }

    return poGeometry;

#endif /* HAVE_GEOS */
}

/************************************************************************/
/*                       getGEOSGeometryFactory()                       */
/************************************************************************/

void *OGRGeometryFactory::getGEOSGeometryFactory() 

{
    // XXX - mloskot - What to do with this call
    // after GEOS C++ API has been stripped?
    return NULL;
}

/************************************************************************/
/*                              haveGEOS()                              */
/************************************************************************/

/**
 * \brief Test if GEOS enabled.
 *
 * This static method returns TRUE if GEOS support is built into OGR,
 * otherwise it returns FALSE.
 *
 * @return TRUE if available, otherwise FALSE.
 */

int OGRGeometryFactory::haveGEOS()

{
#ifndef HAVE_GEOS 
    return FALSE;
#else
    return TRUE;
#endif
}

/************************************************************************/
/*                           createFromFgf()                            */
/************************************************************************/

/**
 * \brief Create a geometry object of the appropriate type from it's FGF (FDO Geometry Format) binary representation.
 *
 * Also note that this is a static method, and that there
 * is no need to instantiate an OGRGeometryFactory object.  
 *
 * The C function OGR_G_CreateFromFgf() is the same as this method.
 *
 * @param pabyData pointer to the input BLOB data.
 * @param poSR pointer to the spatial reference to be assigned to the
 *             created geometry object.  This may be NULL.
 * @param ppoReturn the newly created geometry object will be assigned to the
 *                  indicated pointer on return.  This will be NULL in case
 *                  of failure.
 * @param nBytes the number of bytes available in pabyData.
 * @param pnBytesConsumed if not NULL, it will be set to the number of bytes 
 * consumed (at most nBytes).
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

OGRErr OGRGeometryFactory::createFromFgf( unsigned char *pabyData,
                                          OGRSpatialReference * poSR,
                                          OGRGeometry **ppoReturn,
                                          int nBytes,
                                          int *pnBytesConsumed )

{
    OGRErr       eErr = OGRERR_NONE;
    OGRGeometry *poGeom = NULL;
    GInt32       nGType, nGDim;
    int          nTupleSize = 0;
    int          iOrdinal = 0;
    
    (void) iOrdinal;

    *ppoReturn = NULL;

    if( nBytes < 4 )
        return OGRERR_NOT_ENOUGH_DATA;

/* -------------------------------------------------------------------- */
/*      Decode the geometry type.                                       */
/* -------------------------------------------------------------------- */
    memcpy( &nGType, pabyData + 0, 4 );
    CPL_LSBPTR32( &nGType );

    if( nGType < 0 || nGType > 13 )
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;

/* -------------------------------------------------------------------- */
/*      Decode the dimentionality if appropriate.                       */
/* -------------------------------------------------------------------- */
    switch( nGType )
    {
      case 1: // Point
      case 2: // LineString
      case 3: // Polygon
        
        if( nBytes < 8 )
            return OGRERR_NOT_ENOUGH_DATA;

        memcpy( &nGDim, pabyData + 4, 4 );
        CPL_LSBPTR32( &nGDim );
        
        if( nGDim < 0 || nGDim > 3 )
            return OGRERR_CORRUPT_DATA;

        nTupleSize = 2;
        if( nGDim & 0x01 ) // Z
            nTupleSize++;
        if( nGDim & 0x02 ) // M
            nTupleSize++;

        break;

      default:
        break;
    }

/* -------------------------------------------------------------------- */
/*      None                                                            */
/* -------------------------------------------------------------------- */
    if( nGType == 0 ) 
    {
        if( pnBytesConsumed )
            *pnBytesConsumed = 4;
    }

/* -------------------------------------------------------------------- */
/*      Point                                                           */
/* -------------------------------------------------------------------- */
    else if( nGType == 1 )
    {
        double  adfTuple[4];

        if( nBytes < nTupleSize * 8 + 8 )
            return OGRERR_NOT_ENOUGH_DATA;

        memcpy( adfTuple, pabyData + 8, nTupleSize*8 );
#ifdef CPL_MSB
        for( iOrdinal = 0; iOrdinal < nTupleSize; iOrdinal++ )
            CPL_SWAP64PTR( adfTuple + iOrdinal );
#endif
        if( nTupleSize > 2 )
            poGeom = new OGRPoint( adfTuple[0], adfTuple[1], adfTuple[2] );
        else
            poGeom = new OGRPoint( adfTuple[0], adfTuple[1] );

        if( pnBytesConsumed )
            *pnBytesConsumed = 8 + nTupleSize * 8;
    }

/* -------------------------------------------------------------------- */
/*      LineString                                                      */
/* -------------------------------------------------------------------- */
    else if( nGType == 2 )
    {
        double adfTuple[4];
        GInt32 nPointCount;
        int    iPoint;
        OGRLineString *poLS;

        if( nBytes < 12 )
            return OGRERR_NOT_ENOUGH_DATA;

        memcpy( &nPointCount, pabyData + 8, 4 );
        CPL_LSBPTR32( &nPointCount );

        if (nPointCount < 0 || nPointCount > INT_MAX / (nTupleSize * 8))
            return OGRERR_CORRUPT_DATA;

        if( nBytes - 12 < nTupleSize * 8 * nPointCount )
            return OGRERR_NOT_ENOUGH_DATA;

        poGeom = poLS = new OGRLineString();
        poLS->setNumPoints( nPointCount );

        for( iPoint = 0; iPoint < nPointCount; iPoint++ )
        {
            memcpy( adfTuple, pabyData + 12 + 8*nTupleSize*iPoint, 
                    nTupleSize*8 );
#ifdef CPL_MSB
            for( iOrdinal = 0; iOrdinal < nTupleSize; iOrdinal++ )
                CPL_SWAP64PTR( adfTuple + iOrdinal );
#endif
            if( nTupleSize > 2 )
                poLS->setPoint( iPoint, adfTuple[0], adfTuple[1], adfTuple[2] );
            else
                poLS->setPoint( iPoint, adfTuple[0], adfTuple[1] );
        }

        if( pnBytesConsumed )
            *pnBytesConsumed = 12 + nTupleSize * 8 * nPointCount;
    }

/* -------------------------------------------------------------------- */
/*      Polygon                                                         */
/* -------------------------------------------------------------------- */
    else if( nGType == 3 )
    {
        double adfTuple[4];
        GInt32 nPointCount;
        GInt32 nRingCount;
        int    iPoint, iRing;
        OGRLinearRing *poLR;
        OGRPolygon *poPoly;
        int    nNextByte;

        if( nBytes < 12 )
            return OGRERR_NOT_ENOUGH_DATA;

        memcpy( &nRingCount, pabyData + 8, 4 );
        CPL_LSBPTR32( &nRingCount );

        if (nRingCount < 0 || nRingCount > INT_MAX / 4)
            return OGRERR_CORRUPT_DATA;

        /* Each ring takes at least 4 bytes */
        if (nBytes - 12 < nRingCount * 4)
            return OGRERR_NOT_ENOUGH_DATA;

        nNextByte = 12;
        
        poGeom = poPoly = new OGRPolygon();

        for( iRing = 0; iRing < nRingCount; iRing++ )
        {
            if( nBytes - nNextByte < 4 )
                return OGRERR_NOT_ENOUGH_DATA;

            memcpy( &nPointCount, pabyData + nNextByte, 4 );
            CPL_LSBPTR32( &nPointCount );

            if (nPointCount < 0 || nPointCount > INT_MAX / (nTupleSize * 8))
                return OGRERR_CORRUPT_DATA;

            nNextByte += 4;

            if( nBytes - nNextByte < nTupleSize * 8 * nPointCount )
                return OGRERR_NOT_ENOUGH_DATA;

            poLR = new OGRLinearRing();
            poLR->setNumPoints( nPointCount );
            
            for( iPoint = 0; iPoint < nPointCount; iPoint++ )
            {
                memcpy( adfTuple, pabyData + nNextByte, nTupleSize*8 );
                nNextByte += nTupleSize * 8;

#ifdef CPL_MSB
                for( iOrdinal = 0; iOrdinal < nTupleSize; iOrdinal++ )
                    CPL_SWAP64PTR( adfTuple + iOrdinal );
#endif
                if( nTupleSize > 2 )
                    poLR->setPoint( iPoint, adfTuple[0], adfTuple[1], adfTuple[2] );
                else
                    poLR->setPoint( iPoint, adfTuple[0], adfTuple[1] );
            }

            poPoly->addRingDirectly( poLR );
        }

        if( pnBytesConsumed )
            *pnBytesConsumed = nNextByte;
    }

/* -------------------------------------------------------------------- */
/*      GeometryCollections of various kinds.                           */
/* -------------------------------------------------------------------- */
    else if( nGType == 4         // MultiPoint
             || nGType == 5      // MultiLineString
             || nGType == 6      // MultiPolygon
             || nGType == 7 )    // MultiGeometry
    {
        OGRGeometryCollection *poGC = NULL;
        GInt32 nGeomCount;
        int iGeom, nBytesUsed;

        if( nGType == 4 )
            poGC = new OGRMultiPoint();
        else if( nGType == 5 )
            poGC = new OGRMultiLineString();
        else if( nGType == 6 )
            poGC = new OGRMultiPolygon();
        else if( nGType == 7 )
            poGC = new OGRGeometryCollection();

        if( nBytes < 8 )
            return OGRERR_NOT_ENOUGH_DATA;

        memcpy( &nGeomCount, pabyData + 4, 4 );
        CPL_LSBPTR32( &nGeomCount );

        if (nGeomCount < 0 || nGeomCount > INT_MAX / 4)
            return OGRERR_CORRUPT_DATA;

        /* Each geometry takes at least 4 bytes */
        if (nBytes - 8 < 4 * nGeomCount)
            return OGRERR_NOT_ENOUGH_DATA;

        nBytesUsed = 8;

        for( iGeom = 0; iGeom < nGeomCount; iGeom++ )
        {
            int nThisGeomSize;
            OGRGeometry *poThisGeom = NULL;
         
            eErr = createFromFgf( pabyData + nBytesUsed, poSR, &poThisGeom,
                                  nBytes - nBytesUsed, &nThisGeomSize);
            if( eErr != OGRERR_NONE )
            {
                delete poGC;
                return eErr;
            }

            nBytesUsed += nThisGeomSize;
            eErr = poGC->addGeometryDirectly( poThisGeom );
            if( eErr != OGRERR_NONE )
            {
                delete poGC;
                return eErr;
            }
        }

        poGeom = poGC;
        if( pnBytesConsumed )
            *pnBytesConsumed = nBytesUsed;
    }

/* -------------------------------------------------------------------- */
/*      Currently unsupported geometry.                                 */
/*                                                                      */
/*      We need to add 10/11/12/13 curve types in some fashion.         */
/* -------------------------------------------------------------------- */
    else
    {
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
    }

/* -------------------------------------------------------------------- */
/*      Assign spatial reference system.                                */
/* -------------------------------------------------------------------- */
    if( eErr == OGRERR_NONE )
    {
        if( poGeom != NULL && poSR )
            poGeom->assignSpatialReference( poSR );
        *ppoReturn = poGeom;
    }
    else
    {
        delete poGeom;
    }

    return eErr;
}

/************************************************************************/
/*                        OGR_G_CreateFromFgf()                         */
/************************************************************************/

OGRErr CPL_DLL OGR_G_CreateFromFgf( unsigned char *pabyData, 
                                    OGRSpatialReferenceH hSRS,
                                    OGRGeometryH *phGeometry, 
                                    int nBytes, int *pnBytesConsumed )

{
    return OGRGeometryFactory::createFromFgf( pabyData, 
                                              (OGRSpatialReference *) hSRS,
                                              (OGRGeometry **) phGeometry,
                                              nBytes, pnBytesConsumed );
}

/************************************************************************/
/*                SplitLineStringAtDateline()                           */
/************************************************************************/

#define SWAP_DBL(a,b) do { double tmp = a; a = b; b = tmp; } while(0)

static void SplitLineStringAtDateline(OGRGeometryCollection* poMulti,
                                      const OGRLineString* poLS)
{
    int i;
    int bIs3D = poLS->getCoordinateDimension() == 3;
    OGRLineString* poNewLS = new OGRLineString();
    poMulti->addGeometryDirectly(poNewLS);
    for(i=0;i<poLS->getNumPoints();i++)
    {
        double dfX = poLS->getX(i);
        if (i > 0 && fabs(dfX - poLS->getX(i-1)) > 350)
        {
            double dfX1 = poLS->getX(i-1);
            double dfY1 = poLS->getY(i-1);
            double dfZ1 = poLS->getY(i-1);
            double dfX2 = poLS->getX(i);
            double dfY2 = poLS->getY(i);
            double dfZ2 = poLS->getY(i);

            if (dfX1 > -180 && dfX1 < -170 && dfX2 == 180 &&
                i+1 < poLS->getNumPoints() &&
                poLS->getX(i+1) > -180 && poLS->getX(i+1) < -170)
            {
                if( bIs3D )
                    poNewLS->addPoint(-180, poLS->getY(i), poLS->getZ(i));
                else
                    poNewLS->addPoint(-180, poLS->getY(i));

                i++;

                if( bIs3D )
                    poNewLS->addPoint(poLS->getX(i), poLS->getY(i), poLS->getZ(i));
                else
                    poNewLS->addPoint(poLS->getX(i), poLS->getY(i));
                continue;
            }
            else if (dfX1 > 170 && dfX1 < 180 && dfX2 == -180 &&
                     i+1 < poLS->getNumPoints() &&
                     poLS->getX(i+1) > 170 && poLS->getX(i+1) < 180)
            {
                if( bIs3D )
                    poNewLS->addPoint(180, poLS->getY(i), poLS->getZ(i));
                else
                    poNewLS->addPoint(180, poLS->getY(i));

                i++;

                if( bIs3D )
                    poNewLS->addPoint(poLS->getX(i), poLS->getY(i), poLS->getZ(i));
                else
                    poNewLS->addPoint(poLS->getX(i), poLS->getY(i));
                continue;
            }

            if (dfX1 < -170 && dfX2 > 170)
            {
                SWAP_DBL(dfX1, dfX2);
                SWAP_DBL(dfY1, dfY2);
                SWAP_DBL(dfZ1, dfZ2);
            }
            if (dfX1 > 170 && dfX2 < -170)
                dfX2 += 360;

            if (dfX1 <= 180 && dfX2 >= 180 && dfX1 < dfX2)
            {
                double dfRatio = (180 - dfX1) / (dfX2 - dfX1);
                double dfY = dfRatio * dfY2 + (1 - dfRatio) * dfY1;
                double dfZ = dfRatio * dfZ2 + (1 - dfRatio) * dfZ1;
                if( bIs3D )
                    poNewLS->addPoint(poLS->getX(i-1) > 170 ? 180 : -180, dfY, dfZ);
                else
                    poNewLS->addPoint(poLS->getX(i-1) > 170 ? 180 : -180, dfY);
                poNewLS = new OGRLineString();
                if( bIs3D )
                    poNewLS->addPoint(poLS->getX(i-1) > 170 ? -180 : 180, dfY, dfZ);
                else
                    poNewLS->addPoint(poLS->getX(i-1) > 170 ? -180 : 180, dfY);
                poMulti->addGeometryDirectly(poNewLS);
            }
            else
            {
                poNewLS = new OGRLineString();
                poMulti->addGeometryDirectly(poNewLS);
            }
        }
        if( bIs3D )
            poNewLS->addPoint(dfX, poLS->getY(i), poLS->getZ(i));
        else
            poNewLS->addPoint(dfX, poLS->getY(i));
    }
}

/************************************************************************/
/*               FixPolygonCoordinatesAtDateLine()                      */
/************************************************************************/

#ifdef HAVE_GEOS
static void FixPolygonCoordinatesAtDateLine(OGRPolygon* poPoly)
{
    int i, iPart;
    for(iPart = 0; iPart < 1 + poPoly->getNumInteriorRings(); iPart++)
    {
        OGRLineString* poLS = (iPart == 0) ? poPoly->getExteriorRing() :
                                             poPoly->getInteriorRing(iPart-1);
        int bGoEast = FALSE;
        int bIs3D = poLS->getCoordinateDimension() == 3;
        for(i=1;i<poLS->getNumPoints();i++)
        {
            double dfX = poLS->getX(i);
            double dfPrevX = poLS->getX(i-1);
            double dfDiffLong = fabs(dfX - dfPrevX);
            if (dfDiffLong > 350)
            {
                if ((dfPrevX > 170 && dfX < -170) || (dfX < 0 && bGoEast))
                {
                    dfX += 360;
                    bGoEast = TRUE;
                    if( bIs3D )
                        poLS->setPoint(i, dfX, poLS->getY(i), poLS->getZ(i));
                    else
                        poLS->setPoint(i, dfX, poLS->getY(i));
                }
                else if (dfPrevX < -170 && dfX > 170)
                {
                    int j;
                    for(j=i-1;j>=0;j--)
                    {
                        dfX = poLS->getX(j);
                        if (dfX < 0)
                        {
                            if( bIs3D )
                                poLS->setPoint(j, dfX + 360, poLS->getY(j), poLS->getZ(j));
                            else
                                poLS->setPoint(j, dfX + 360, poLS->getY(j));
                        }
                    }
                    bGoEast = FALSE;
                }
                else
                {
                    bGoEast = FALSE;
                }
            }
        }
    }
}
#endif

/************************************************************************/
/*                            Sub360ToLon()                             */
/************************************************************************/

static void Sub360ToLon( OGRGeometry* poGeom )
{
    switch (wkbFlatten(poGeom->getGeometryType()))
    {
        case wkbPolygon:
        case wkbMultiLineString:
        case wkbMultiPolygon:
        case wkbGeometryCollection:
        {
            int nSubGeomCount = OGR_G_GetGeometryCount((OGRGeometryH)poGeom);
            for( int iGeom = 0; iGeom < nSubGeomCount; iGeom++ )
            {
                Sub360ToLon((OGRGeometry*)OGR_G_GetGeometryRef((OGRGeometryH)poGeom, iGeom));
            }
            
            break;
        }
            
        case wkbLineString:
        case wkbLinearRing:
        {
            OGRLineString* poLineString = (OGRLineString* )poGeom;
            int nPointCount = poLineString->getNumPoints();
            int nCoordDim = poLineString->getCoordinateDimension();
            for( int iPoint = 0; iPoint < nPointCount; iPoint++)
            {
                if (nCoordDim == 2)
                    poLineString->setPoint(iPoint,
                                     poLineString->getX(iPoint) - 360,
                                     poLineString->getY(iPoint));
                else
                    poLineString->setPoint(iPoint,
                                     poLineString->getX(iPoint) - 360,
                                     poLineString->getY(iPoint),
                                     poLineString->getZ(iPoint));
            }
            break;
        }
            
        default:
            break;
    }
}

/************************************************************************/
/*                        AddSimpleGeomToMulti()                        */
/************************************************************************/

static void AddSimpleGeomToMulti(OGRGeometryCollection* poMulti,
                                 const OGRGeometry* poGeom)
{
    switch (wkbFlatten(poGeom->getGeometryType()))
    {
        case wkbPolygon:
        case wkbLineString:
            poMulti->addGeometry(poGeom);
            break;
            
        case wkbMultiLineString:
        case wkbMultiPolygon:
        case wkbGeometryCollection:
        {
            int nSubGeomCount = OGR_G_GetGeometryCount((OGRGeometryH)poGeom);
            for( int iGeom = 0; iGeom < nSubGeomCount; iGeom++ )
            {
                OGRGeometry* poSubGeom =
                    (OGRGeometry*)OGR_G_GetGeometryRef((OGRGeometryH)poGeom, iGeom);
                AddSimpleGeomToMulti(poMulti, poSubGeom);
            }
            break;
        }
            
        default:
            break;
    }
}

/************************************************************************/
/*                 CutGeometryOnDateLineAndAddToMulti()                 */
/************************************************************************/

static void CutGeometryOnDateLineAndAddToMulti(OGRGeometryCollection* poMulti,
                                               const OGRGeometry* poGeom)
{
    OGRwkbGeometryType eGeomType = wkbFlatten(poGeom->getGeometryType());
    switch (eGeomType)
    {
        case wkbPolygon:
        case wkbLineString:
        {
            int bWrapDateline = FALSE;
            int bSplitLineStringAtDateline = FALSE;
            OGREnvelope oEnvelope;
            
            poGeom->getEnvelope(&oEnvelope);
            
            /* Naive heuristics... Place to improvement... */
            OGRGeometry* poDupGeom = NULL;
            
            if (oEnvelope.MinX > 170 && oEnvelope.MaxX > 180)
            {
#ifndef HAVE_GEOS
                CPLError( CE_Failure, CPLE_NotSupported, 
                        "GEOS support not enabled." );
#else
                bWrapDateline = TRUE;
#endif
            }
            else
            {
                OGRLineString* poLS;
                if (eGeomType == wkbPolygon)
                    poLS = ((OGRPolygon*)poGeom)->getExteriorRing();
                else
                    poLS = (OGRLineString*)poGeom;
                if (poLS)
                {
                    int i;
                    double dfMaxSmallDiffLong = 0;
                    int bHasBigDiff = FALSE;
                    /* Detect big gaps in longitude */
                    for(i=1;i<poLS->getNumPoints();i++)
                    {
                        double dfPrevX = poLS->getX(i-1);
                        double dfX = poLS->getX(i);
                        double dfDiffLong = fabs(dfX - dfPrevX);
                        if (dfDiffLong > 350 &&
                            ((dfX > 170 && dfPrevX < -170) || (dfPrevX > 170 && dfX < -170)))
                            bHasBigDiff = TRUE;
                        else if (dfDiffLong > dfMaxSmallDiffLong)
                            dfMaxSmallDiffLong = dfDiffLong;
                    }
                    if (bHasBigDiff && dfMaxSmallDiffLong < 10)
                    {
                        if (eGeomType == wkbLineString)
                            bSplitLineStringAtDateline = TRUE;
                        else
                        {
#ifndef HAVE_GEOS
                            CPLError( CE_Failure, CPLE_NotSupported, 
                                    "GEOS support not enabled." );
#else
                            bWrapDateline = TRUE;
                            poDupGeom = poGeom->clone();
                            FixPolygonCoordinatesAtDateLine((OGRPolygon*)poDupGeom);
#endif
                        }
                    }
                }
            }

            if (bSplitLineStringAtDateline)
            {
                SplitLineStringAtDateline(poMulti, (OGRLineString*)poGeom);
            }
            else if (bWrapDateline)
            {
                const OGRGeometry* poWorkGeom = (poDupGeom) ? poDupGeom : poGeom;
                OGRGeometry* poRectangle1 = NULL;
                OGRGeometry* poRectangle2 = NULL;
                const char* pszWKT1 = "POLYGON((0 90,180 90,180 -90,0 -90,0 90))";
                const char* pszWKT2 = "POLYGON((180 90,360 90,360 -90,180 -90,180 90))";
                OGRGeometryFactory::createFromWkt((char**)&pszWKT1, NULL, &poRectangle1);
                OGRGeometryFactory::createFromWkt((char**)&pszWKT2, NULL, &poRectangle2);
                OGRGeometry* poGeom1 = poWorkGeom->Intersection(poRectangle1);
                OGRGeometry* poGeom2 = poWorkGeom->Intersection(poRectangle2);
                delete poRectangle1;
                delete poRectangle2;
                
                if (poGeom1 != NULL && poGeom2 != NULL)
                {
                    AddSimpleGeomToMulti(poMulti, poGeom1);
                    Sub360ToLon(poGeom2);
                    AddSimpleGeomToMulti(poMulti, poGeom2);
                }
                else
                {
                    AddSimpleGeomToMulti(poMulti, poGeom);
                }
                
                delete poGeom1;
                delete poGeom2;
                delete poDupGeom;
            }
            else
            {
                poMulti->addGeometry(poGeom);
            }   
            break;
        }
            
        case wkbMultiLineString:
        case wkbMultiPolygon:
        case wkbGeometryCollection:
        {
            int nSubGeomCount = OGR_G_GetGeometryCount((OGRGeometryH)poGeom);
            for( int iGeom = 0; iGeom < nSubGeomCount; iGeom++ )
            {
                OGRGeometry* poSubGeom =
                    (OGRGeometry*)OGR_G_GetGeometryRef((OGRGeometryH)poGeom, iGeom);
                CutGeometryOnDateLineAndAddToMulti(poMulti, poSubGeom);
            }
            break;
        }
            
        default:
            break;
    }
}

/************************************************************************/
/*                       transformWithOptions()                         */
/************************************************************************/

OGRGeometry* OGRGeometryFactory::transformWithOptions( const OGRGeometry* poSrcGeom,
                                                       OGRCoordinateTransformation *poCT,
                                                       char** papszOptions )
{
    OGRGeometry* poDstGeom = poSrcGeom->clone();
    if (poCT != NULL)
    {
        OGRErr eErr = poDstGeom->transform(poCT);
        if (eErr != OGRERR_NONE)
        {
            delete poDstGeom;
            return NULL;
        }
    }
    
    if (CSLTestBoolean(CSLFetchNameValueDef(papszOptions, "WRAPDATELINE", "NO")))
    {
        OGRwkbGeometryType eType = wkbFlatten(poSrcGeom->getGeometryType());
        OGRwkbGeometryType eNewType;
        if (eType == wkbPolygon || eType == wkbMultiPolygon)
            eNewType = wkbMultiPolygon;
        else if (eType == wkbLineString || eType == wkbMultiLineString)
            eNewType = wkbMultiLineString;
        else
            eNewType = wkbGeometryCollection;
        
        OGRGeometryCollection* poMulti =
            (OGRGeometryCollection* )createGeometry(eNewType);
            
        CutGeometryOnDateLineAndAddToMulti(poMulti, poDstGeom);
        
        if (poMulti->getNumGeometries() == 0)
        {
            delete poMulti;
        }            
        else if (poMulti->getNumGeometries() == 1)
        {
            delete poDstGeom;
            poDstGeom = poMulti->getGeometryRef(0)->clone();
            delete poMulti;
        }
        else
        {
            delete poDstGeom;
            poDstGeom = poMulti;
        }
    }

    return poDstGeom;
}

/************************************************************************/
/*                        approximateArcAngles()                        */
/************************************************************************/

/**
 * Stroke arc to linestring.
 *
 * Stroke an arc of a circle to a linestring based on a center
 * point, radius, start angle and end angle, all angles in degrees.
 *
 * If the dfMaxAngleStepSizeDegrees is zero, then a default value will be
 * used.  This is currently 4 degrees unless the user has overridden the
 * value with the OGR_ARC_STEPSIZE configuration variable. 
 *
 * @see CPLSetConfigOption()
 *
 * @param dfCenterX center X
 * @param dfCenterY center Y
 * @param dfZ center Z
 * @param dfPrimaryRadius X radius of ellipse.
 * @param dfSecondaryRadius Y radius of ellipse. 
 * @param dfRotation rotation of the ellipse clockwise.
 * @param dfStartAngle angle to first point on arc (clockwise of X-positive) 
 * @param dfEndAngle angle to last point on arc (clockwise of X-positive) 
 * @param dfMaxAngleStepSizeDegrees the largest step in degrees along the
 * arc, zero to use the default setting.
 * 
 * @return OGRLineString geometry representing an approximation of the arc.
 *
 * @since OGR 1.8.0
 */

OGRGeometry* OGRGeometryFactory::approximateArcAngles( 
    double dfCenterX, double dfCenterY, double dfZ,
    double dfPrimaryRadius, double dfSecondaryRadius, double dfRotation, 
    double dfStartAngle, double dfEndAngle,
    double dfMaxAngleStepSizeDegrees )

{
    double             dfSlice;
    int                iPoint, nVertexCount;
    OGRLineString     *poLine = new OGRLineString();
    double             dfRotationRadians = dfRotation * PI / 180.0;

    // support default arc step setting.
    if( dfMaxAngleStepSizeDegrees == 0 )
    {
        dfMaxAngleStepSizeDegrees = 
            atof(CPLGetConfigOption("OGR_ARC_STEPSIZE","4"));
    }

    // switch direction 
    dfStartAngle *= -1;
    dfEndAngle *= -1;

    // Figure out the number of slices to make this into.
    nVertexCount = (int) 
        ceil(fabs(dfEndAngle - dfStartAngle)/dfMaxAngleStepSizeDegrees) + 1;
    nVertexCount = MAX(2,nVertexCount);
    dfSlice = (dfEndAngle-dfStartAngle)/(nVertexCount-1);

/* -------------------------------------------------------------------- */
/*      Compute the interpolated points.                                */
/* -------------------------------------------------------------------- */
    for( iPoint=0; iPoint < nVertexCount; iPoint++ )
    {
        double      dfAngleOnEllipse;
        double      dfArcX, dfArcY;
        double      dfEllipseX, dfEllipseY;

        dfAngleOnEllipse = (dfStartAngle + iPoint * dfSlice) * PI / 180.0;

        // Compute position on the unrotated ellipse. 
        dfEllipseX = cos(dfAngleOnEllipse) * dfPrimaryRadius;
        dfEllipseY = sin(dfAngleOnEllipse) * dfSecondaryRadius;
        
        // Rotate this position around the center of the ellipse.
        dfArcX = dfCenterX 
            + dfEllipseX * cos(dfRotationRadians) 
            + dfEllipseY * sin(dfRotationRadians);
        dfArcY = dfCenterY 
            - dfEllipseX * sin(dfRotationRadians)
            + dfEllipseY * cos(dfRotationRadians);

        poLine->setPoint( iPoint, dfArcX, dfArcY, dfZ );
    }

    return poLine;
}

/************************************************************************/
/*                     OGR_G_ApproximateArcAngles()                     */
/************************************************************************/

/**
 * Stroke arc to linestring.
 *
 * Stroke an arc of a circle to a linestring based on a center
 * point, radius, start angle and end angle, all angles in degrees.
 *
 * If the dfMaxAngleStepSizeDegrees is zero, then a default value will be
 * used.  This is currently 4 degrees unless the user has overridden the
 * value with the OGR_ARC_STEPSIZE configuration variable.
 *
 * @see CPLSetConfigOption()
 *
 * @param dfCenterX center X
 * @param dfCenterY center Y
 * @param dfZ center Z
 * @param dfPrimaryRadius X radius of ellipse.
 * @param dfSecondaryRadius Y radius of ellipse.
 * @param dfRotation rotation of the ellipse clockwise.
 * @param dfStartAngle angle to first point on arc (clockwise of X-positive)
 * @param dfEndAngle angle to last point on arc (clockwise of X-positive)
 * @param dfMaxAngleStepSizeDegrees the largest step in degrees along the
 * arc, zero to use the default setting.
 *
 * @return OGRLineString geometry representing an approximation of the arc.
 *
 * @since OGR 1.8.0
 */

OGRGeometryH CPL_DLL 
OGR_G_ApproximateArcAngles( 
    double dfCenterX, double dfCenterY, double dfZ,
    double dfPrimaryRadius, double dfSecondaryRadius, double dfRotation,
    double dfStartAngle, double dfEndAngle,
    double dfMaxAngleStepSizeDegrees )

{
    return (OGRGeometryH) OGRGeometryFactory::approximateArcAngles(
        dfCenterX, dfCenterY, dfZ, 
        dfPrimaryRadius, dfSecondaryRadius, dfRotation,
        dfStartAngle, dfEndAngle, dfMaxAngleStepSizeDegrees );
}

/************************************************************************/
/*                         curveToLineString()                          */
/************************************************************************/

OGRLineString* OGRGeometryFactory::curveToLineString(
    double x0, double y0, double x1, double y1, double x2, double y2,
    int bIsCircle,
    double dfMaxAngleStepSizeDegrees )
{
    OGRLineString* poLine = new OGRLineString();

    double dx01 = x1 - x0;
    double dy01 = y1 - y0;
    double dx12 = x2 - x1;
    double dy12 = y2 - y1;
    double c01 = dx01 * (x0 + x1) / 2 + dy01 * (y0 + y1) / 2;
    double c12 = dx12 * (x1 + x2) / 2 + dy12 * (y1 + y2) / 2;
    double det = dx01 * dy12 - dx12 * dy01;
    if (det == 0)
    {
        poLine->addPoint(x0, y0);
        poLine->addPoint(x1, y1);
        poLine->addPoint(x2, y2);
        return poLine;
    }
    double cx =  (c01 * dy12 - c12 * dy01) / det;
    double cy =  (- c01 * dx12 + c12 * dx01) / det;

    double alpha0 = atan2(y0 - cy, x0 - cx);
    double alpha1 = atan2(y1 - cy, x1 - cx);
    double alpha2 = atan2(y2 - cy, x2 - cx);
    double alpha3;
    double R = sqrt((x0 - cx) * (x0 - cx) + (y0 - cy) * (y0 - cy));

    /* if det is negative, the orientation if clockwise */
    if (det < 0)
    {
        if (alpha1 > alpha0)
            alpha1 -= 2 * PI;
        if (alpha2 > alpha1)
            alpha2 -= 2 * PI;
        alpha3 = alpha0 - 2 * PI;
    }
    else
    {
        if (alpha1 < alpha0)
            alpha1 += 2 * PI;
        if (alpha2 < alpha1)
            alpha2 += 2 * PI;
        alpha3 = alpha0 + 2 * PI;
    }

    CPLAssert((alpha0 <= alpha1 && alpha1 <= alpha2 && alpha2 <= alpha3) ||
                (alpha0 >= alpha1 && alpha1 >= alpha2 && alpha2 >= alpha3));

    int nSign = (det >= 0) ? 1 : -1;

    // support default arc step setting.
    if( dfMaxAngleStepSizeDegrees == 0 )
    {
        dfMaxAngleStepSizeDegrees =
            atof(CPLGetConfigOption("OGR_ARC_STEPSIZE","4"));
    }

    double alpha;
    double dfStep =
        dfMaxAngleStepSizeDegrees / 180 * PI;
    if (dfStep <= 0.1)
        dfStep = 4. / 180 * PI;

    dfStep *= nSign;

    for(alpha = alpha0; (alpha - alpha1) * nSign < 0; alpha += dfStep)
    {
        poLine->addPoint(cx + R * cos(alpha), cy + R * sin(alpha));
    }
    for(alpha = alpha1; (alpha - alpha2) * nSign < 0; alpha += dfStep)
    {
        poLine->addPoint(cx + R * cos(alpha), cy + R * sin(alpha));
    }

    if (bIsCircle)
    {
        for(alpha = alpha2; (alpha - alpha3) * nSign < 0; alpha += dfStep)
        {
            poLine->addPoint(cx + R * cos(alpha), cy + R * sin(alpha));
        }
        poLine->addPoint(cx + R * cos(alpha3), cy + R * sin(alpha3));
    }
    else
    {
        poLine->addPoint(cx + R * cos(alpha2), cy + R * sin(alpha2));
    }

    return poLine;
}
