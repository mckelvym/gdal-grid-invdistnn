/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRShapeLayer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999,  Les Technologies SoftMap Inc.
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
 * Revision 1.21  2005/09/21 00:53:20  fwarmerdam
 * fixup OGRFeatureDefn and OGRSpatialReference refcount handling
 *
 * Revision 1.20  2005/09/10 22:32:34  fwarmerdam
 * Make GetNextShape() more bulletproof if SHPReadOGRFeature() fails.
 *
 * Revision 1.19  2005/02/22 12:51:56  fwarmerdam
 * use OGRLayer base spatial filter support
 *
 * Revision 1.18  2005/02/02 20:01:02  fwarmerdam
 * added SetNextByIndex support
 *
 * Revision 1.17  2005/01/04 03:43:41  fwarmerdam
 * added support for creating and destroying spatial indexes
 *
 * Revision 1.16  2005/01/03 22:26:21  fwarmerdam
 * updated to use spatial indexing
 *
 * Revision 1.15  2003/05/21 04:03:54  warmerda
 * expand tabs
 *
 * Revision 1.14  2003/04/21 19:03:20  warmerda
 * added SyncToDisk support
 *
 * Revision 1.13  2003/04/15 20:45:54  warmerda
 * Added code to update the feature count in CreateFeature().
 *
 * Revision 1.12  2003/03/05 05:09:41  warmerda
 * initialize panMatchingFIDs
 *
 * Revision 1.11  2003/03/04 05:49:05  warmerda
 * added attribute indexing support
 *
 * Revision 1.10  2002/10/09 14:13:40  warmerda
 * ensure width is at least 1
 *
 * Revision 1.9  2002/04/05 19:59:37  warmerda
 * Output file was getting different geometries in some cases.
 *
 * Revision 1.8  2002/03/27 21:04:38  warmerda
 * Added support for reading, and creating lone .dbf files for wkbNone geometry
 * layers.  Added support for creating a single .shp file instead of a directory
 * if a path ending in .shp is passed to the data source create method.
 *
 * Revision 1.7  2001/09/04 15:35:14  warmerda
 * add support for deferring geometry type selection till first feature
 *
 * Revision 1.6  2001/08/21 21:44:27  warmerda
 * fixed count logic if attribute query in play
 *
 * Revision 1.5  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.4  2001/06/19 15:50:23  warmerda
 * added feature attribute query support
 *
 * Revision 1.3  2001/03/16 22:16:10  warmerda
 * added support for ESRI .prj files
 *
 * Revision 1.2  2001/03/15 04:21:50  danmo
 * Added GetExtent()
 *
 * Revision 1.1  1999/11/04 21:16:11  warmerda
 * New
 *
 */

#include "ogrshape.h"
#include "cpl_conv.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           OGRShapeLayer()                            */
/************************************************************************/

OGRShapeLayer::OGRShapeLayer( const char * pszName,
                              SHPHandle hSHPIn, DBFHandle hDBFIn, 
                              OGRSpatialReference *poSRSIn, int bUpdate,
                              OGRwkbGeometryType eReqType )

{
    poSRS = poSRSIn;

    pszFullName = CPLStrdup(pszName);
    
    hSHP = hSHPIn;
    hDBF = hDBFIn;
    bUpdateAccess = bUpdate;

    iNextShapeId = 0;
    panMatchingFIDs = NULL;

    bCheckedForQIX = FALSE;
    fpQIX = NULL;

    bHeaderDirty = FALSE;

    if( hSHP != NULL )
        nTotalShapeCount = hSHP->nRecords;
    else 
        nTotalShapeCount = hDBF->nRecords;
    
    poFeatureDefn = SHPReadOGRFeatureDefn( CPLGetBasename(pszName), 
                                           hSHP, hDBF );

    eRequestedGeomType = eReqType;
}

/************************************************************************/
/*                           ~OGRShapeLayer()                           */
/************************************************************************/

OGRShapeLayer::~OGRShapeLayer()

{
    if( m_nFeaturesRead > 0 && poFeatureDefn != NULL )
    {
        CPLDebug( "Shape", "%d features read on layer '%s'.",
                  (int) m_nFeaturesRead, 
                  poFeatureDefn->GetName() );
    }

    CPLFree( panMatchingFIDs );
    panMatchingFIDs = NULL;

    CPLFree( pszFullName );

    poFeatureDefn->Release();
    poSRS->Release();

    if( hDBF != NULL )
        DBFClose( hDBF );

    if( hSHP != NULL )
        SHPClose( hSHP );

    if( fpQIX != NULL )
        VSIFClose( fpQIX );
}

/************************************************************************/
/*                            CheckForQIX()                             */
/************************************************************************/

int OGRShapeLayer::CheckForQIX()

{
    const char *pszQIXFilename;

    if( bCheckedForQIX )
        return fpQIX != NULL;

    pszQIXFilename = CPLResetExtension( pszFullName, "qix" );

    fpQIX = VSIFOpen( pszQIXFilename, "rb" );

    bCheckedForQIX = TRUE;

    return fpQIX != NULL;
}

/************************************************************************/
/*                            ScanIndices()                             */
/*                                                                      */
/*      Utilize optional spatial and attribute indices if they are      */
/*      available.                                                      */
/************************************************************************/

int OGRShapeLayer::ScanIndices()

{
    iMatchingFID = 0;

/* -------------------------------------------------------------------- */
/*      Utilize attribute index if appropriate.                         */
/* -------------------------------------------------------------------- */
    if( m_poAttrQuery != NULL )
    {
        CPLAssert( panMatchingFIDs == NULL );
        panMatchingFIDs = m_poAttrQuery->EvaluateAgainstIndices( this,
                                                                 NULL );
    }

/* -------------------------------------------------------------------- */
/*      Check for spatial index if we have a spatial query.             */
/* -------------------------------------------------------------------- */
    if( m_poFilterGeom != NULL && !bCheckedForQIX )
        CheckForQIX();

/* -------------------------------------------------------------------- */
/*      Utilize spatial index if appropriate.                           */
/* -------------------------------------------------------------------- */
    if( m_poFilterGeom && fpQIX )
    {
        int nSpatialFIDCount, *panSpatialFIDs;
        double adfBoundsMin[4], adfBoundsMax[4];
        OGREnvelope oEnvelope;

        m_poFilterGeom->getEnvelope( &oEnvelope );

        adfBoundsMin[0] = oEnvelope.MinX;
        adfBoundsMin[1] = oEnvelope.MinY;
        adfBoundsMin[2] = 0.0;
        adfBoundsMin[3] = 0.0;
        adfBoundsMax[0] = oEnvelope.MaxX;
        adfBoundsMax[1] = oEnvelope.MaxY;
        adfBoundsMax[2] = 0.0;
        adfBoundsMax[3] = 0.0;

        panSpatialFIDs = SHPSearchDiskTree( fpQIX, 
                                            adfBoundsMin, adfBoundsMax, 
                                            &nSpatialFIDCount );
        CPLDebug( "SHAPE", "Used spatial index, got %d matches.", 
                  nSpatialFIDCount );

        // Use resulting list as matching FID list (but reallocate and
        // terminate with OGRNullFID).

        if( panMatchingFIDs == NULL )
        {
            int i;

            panMatchingFIDs = (long *) 
                CPLMalloc(sizeof(long) * (nSpatialFIDCount+1) );
            for( i = 0; i < nSpatialFIDCount; i++ )
                panMatchingFIDs[i] = (long) panSpatialFIDs[i];
            panMatchingFIDs[nSpatialFIDCount] = OGRNullFID;
            free( panSpatialFIDs );
        }

        // Cull attribute index matches based on those in the spatial index
        // result set.  We assume that the attribute results are in sorted
        // order.
        else
        {
            int iRead, iWrite=0, iSpatial=0;

            for( iRead = 0; panMatchingFIDs[iRead] != OGRNullFID; iRead++ )
            {
                while( iSpatial < nSpatialFIDCount
                       && panSpatialFIDs[iSpatial] < panMatchingFIDs[iRead] )
                    iSpatial++;

                if( iSpatial == nSpatialFIDCount )
                    continue;

                if( panSpatialFIDs[iSpatial] == panMatchingFIDs[iRead] )
                    panMatchingFIDs[iWrite++] = panMatchingFIDs[iRead];
            }
            panMatchingFIDs[iWrite] = OGRNullFID;
        }
    }

    return TRUE;
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRShapeLayer::ResetReading()

{
/* -------------------------------------------------------------------- */
/*      Clear previous index search result, if any.                     */
/* -------------------------------------------------------------------- */
    CPLFree( panMatchingFIDs );
    panMatchingFIDs = NULL;
    iMatchingFID = 0;

    iNextShapeId = 0;

    if( bHeaderDirty )
        SyncToDisk();
}

/************************************************************************/
/*                           SetNextByIndex()                           */
/*                                                                      */
/*      If we already have an FID list, we can easily resposition       */
/*      ourselves in it.                                                */
/************************************************************************/

OGRErr OGRShapeLayer::SetNextByIndex( long nIndex )

{
    // Eventually we should try to use panMatchingFIDs list 
    // if available and appropriate. 
    if( m_poFilterGeom != NULL || m_poAttrQuery != NULL )
        return OGRLayer::SetNextByIndex( nIndex );

    iNextShapeId = nIndex;

    return OGRERR_NONE;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRShapeLayer::GetNextFeature()

{
    OGRFeature  *poFeature;

/* -------------------------------------------------------------------- */
/*      Collect a matching list if we have attribute or spatial         */
/*      indices.  Only do this on the first request for a given pass    */
/*      of course.                                                      */
/* -------------------------------------------------------------------- */
    if( (m_poAttrQuery != NULL || m_poFilterGeom != NULL)
        && iNextShapeId == 0 && panMatchingFIDs == NULL )
    {
        ScanIndices();
    }
    
/* -------------------------------------------------------------------- */
/*      Loop till we find a feature matching our criteria.              */
/* -------------------------------------------------------------------- */
    while( TRUE )
    {
        if( panMatchingFIDs != NULL )
        {
            if( panMatchingFIDs[iMatchingFID] == OGRNullFID )
                return NULL;

            poFeature = SHPReadOGRFeature( hSHP, hDBF, poFeatureDefn, 
                                           panMatchingFIDs[iMatchingFID++] );
        }
        else
        {
            if( iNextShapeId >= nTotalShapeCount )
                return NULL;
    
            poFeature = SHPReadOGRFeature( hSHP, hDBF, poFeatureDefn,
                                           iNextShapeId++ );
        }

        if( poFeature != NULL )
        {
            m_nFeaturesRead++;

            if( (m_poFilterGeom == NULL
                 || FilterGeometry( poFeature->GetGeometryRef() ) )
                && (m_poAttrQuery == NULL
                    || m_poAttrQuery->Evaluate( poFeature )) )
                return poFeature;

            delete poFeature;
        }
    }        
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRShapeLayer::GetFeature( long nFeatureId )

{
    OGRFeature *poFeature = 
         SHPReadOGRFeature( hSHP, hDBF, poFeatureDefn, nFeatureId );
    
    if( poFeature != NULL )
        m_nFeaturesRead++;

    return poFeature;
}

/************************************************************************/
/*                             SetFeature()                             */
/************************************************************************/

OGRErr OGRShapeLayer::SetFeature( OGRFeature *poFeature )

{
    return SHPWriteOGRFeature( hSHP, hDBF, poFeatureDefn, poFeature );
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

OGRErr OGRShapeLayer::CreateFeature( OGRFeature *poFeature )

{
    OGRErr eErr;

    bHeaderDirty = TRUE;

    poFeature->SetFID( OGRNullFID );

    if( nTotalShapeCount == 0 
        && eRequestedGeomType == wkbUnknown 
        && poFeature->GetGeometryRef() != NULL )
    {
        OGRGeometry     *poGeom = poFeature->GetGeometryRef();
        int             nShapeType;
        
        switch( poGeom->getGeometryType() )
        {
          case wkbPoint:
            nShapeType = SHPT_POINT;
            eRequestedGeomType = wkbPoint;
            break;

          case wkbPoint25D:
            nShapeType = SHPT_POINTZ;
            eRequestedGeomType = wkbPoint25D;
            break;

          case wkbMultiPoint:
            nShapeType = SHPT_MULTIPOINT;
            eRequestedGeomType = wkbMultiPoint;
            break;

          case wkbMultiPoint25D:
            nShapeType = SHPT_MULTIPOINTZ;
            eRequestedGeomType = wkbMultiPoint25D;
            break;

          case wkbLineString:
            nShapeType = SHPT_ARC;
            eRequestedGeomType = wkbLineString;
            break;

          case wkbLineString25D:
            nShapeType = SHPT_ARCZ;
            eRequestedGeomType = wkbLineString25D;
            break;

          case wkbPolygon:
          case wkbMultiPolygon:
            nShapeType = SHPT_POLYGON;
            eRequestedGeomType = wkbPolygon;
            break;

          case wkbPolygon25D:
          case wkbMultiPolygon25D:
            nShapeType = SHPT_POLYGONZ;
            eRequestedGeomType = wkbPolygon25D;
            break;

          default:
            nShapeType = -1;
            break;
        }

        if( nShapeType != -1 )
        {
            ResetGeomType( nShapeType );
        }
    }
    
    eErr = SHPWriteOGRFeature( hSHP, hDBF, poFeatureDefn, poFeature );

    if( hSHP != NULL )
        nTotalShapeCount = hSHP->nRecords;
    else 
        nTotalShapeCount = hDBF->nRecords;
    
    return eErr;
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/*                                                                      */
/*      If a spatial filter is in effect, we turn control over to       */
/*      the generic counter.  Otherwise we return the total count.      */
/*      Eventually we should consider implementing a more efficient     */
/*      way of counting features matching a spatial query.              */
/************************************************************************/

int OGRShapeLayer::GetFeatureCount( int bForce )

{
    if( m_poFilterGeom != NULL || m_poAttrQuery != NULL )
        return OGRLayer::GetFeatureCount( bForce );
    else
        return nTotalShapeCount;
}

/************************************************************************/
/*                             GetExtent()                              */
/*                                                                      */
/*      Fetch extent of the data currently stored in the dataset.       */
/*      The bForce flag has no effect on SHO files since that value     */
/*      is always in the header.                                        */
/*                                                                      */
/*      Returns OGRERR_NONE/OGRRERR_FAILURE.                            */
/************************************************************************/

OGRErr OGRShapeLayer::GetExtent (OGREnvelope *psExtent, int bForce)

{
    double adMin[4], adMax[4];

    if( hSHP == NULL )
        return OGRERR_FAILURE;

    SHPGetInfo(hSHP, NULL, NULL, adMin, adMax);

    psExtent->MinX = adMin[0];
    psExtent->MinY = adMin[1];
    psExtent->MaxX = adMax[0];
    psExtent->MaxY = adMax[1];

    return OGRERR_NONE;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRShapeLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return TRUE;

    else if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return bUpdateAccess;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return m_poFilterGeom == NULL;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastGetExtent) )
        return TRUE;

    else if( EQUAL(pszCap,OLCFastSetNextByIndex) )
        return m_poFilterGeom == NULL && m_poAttrQuery == NULL;

    else if( EQUAL(pszCap,OLCCreateField) )
        return TRUE;

    else 
        return FALSE;
}

/************************************************************************/
/*                            CreateField()                             */
/************************************************************************/

OGRErr OGRShapeLayer::CreateField( OGRFieldDefn *poField, int bApproxOK )

{
    int         iNewField;
    if( GetFeatureCount(TRUE) != 0 )
    {
        CPLError( CE_Failure, CPLE_NotSupported,
                  "Can't create fields on a Shapefile layer with features.\n");
        return OGRERR_FAILURE;
    }

    if( !bUpdateAccess )
    {
        CPLError( CE_Failure, CPLE_NotSupported,
                  "Can't create fields on a read-only shapefile layer.\n");
        return OGRERR_FAILURE;
    }

    if( poField->GetType() == OFTInteger )
    {
        if( poField->GetWidth() == 0 )
            iNewField =
                DBFAddField( hDBF, poField->GetNameRef(), FTInteger, 11, 0 );
        else
            iNewField = DBFAddField( hDBF, poField->GetNameRef(), FTInteger,
                                     poField->GetWidth(), 0 );

        if( iNewField != -1 )
            poFeatureDefn->AddFieldDefn( poField );
    }
    else if( poField->GetType() == OFTReal )
    {
        if( poField->GetWidth() == 0 )
            iNewField =
                DBFAddField( hDBF, poField->GetNameRef(), FTDouble, 24, 15 );
        else
            iNewField =
                DBFAddField( hDBF, poField->GetNameRef(), FTDouble,
                             poField->GetWidth(), poField->GetPrecision() );

        if( iNewField != -1 )
            poFeatureDefn->AddFieldDefn( poField );
    }
    else if( poField->GetType() == OFTString )
    {
        if( poField->GetWidth() < 1 )
            iNewField =
                DBFAddField( hDBF, poField->GetNameRef(), FTString, 80, 0 );
        else
            iNewField = DBFAddField( hDBF, poField->GetNameRef(), FTString, 
                                     poField->GetWidth(), 0 );

        if( iNewField != -1 )
            poFeatureDefn->AddFieldDefn( poField );
    }
    else
    {
        CPLError( CE_Failure, CPLE_NotSupported,
                  "Can't create fields of type %s on shapefile layers.\n",
                  OGRFieldDefn::GetFieldTypeName(poField->GetType()) );

        return OGRERR_FAILURE;
    }

    if( iNewField != -1 )
        return OGRERR_NONE;
    else        
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Can't create field %s in Shape DBF file, reason unknown.\n",
                  poField->GetNameRef() );

        return OGRERR_FAILURE;
    }
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRShapeLayer::GetSpatialRef()

{
    return poSRS;
}

/************************************************************************/
/*                           ResetGeomType()                            */
/*                                                                      */
/*      Modify the geometry type for this file.  Used to convert to     */
/*      a different geometry type when a layer was created with a       */
/*      type of unknown, and we get to the first feature to             */
/*      establish the type.                                             */
/************************************************************************/

int OGRShapeLayer::ResetGeomType( int nNewGeomType )

{
    char        abyHeader[100];
    int         nStartPos;

    if( nTotalShapeCount > 0 )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Update .shp header.                                             */
/* -------------------------------------------------------------------- */
    nStartPos = ftell( hSHP->fpSHP );

    if( fseek( hSHP->fpSHP, 0, SEEK_SET ) != 0
        || fread( abyHeader, 100, 1, hSHP->fpSHP ) != 1 )
        return FALSE;

    *((GInt32 *) (abyHeader + 32)) = CPL_LSBWORD32( nNewGeomType );

    if( fseek( hSHP->fpSHP, 0, SEEK_SET ) != 0
        || fwrite( abyHeader, 100, 1, hSHP->fpSHP ) != 1 )
        return FALSE;

    if( fseek( hSHP->fpSHP, nStartPos, SEEK_SET ) != 0 )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Update .shx header.                                             */
/* -------------------------------------------------------------------- */
    nStartPos = ftell( hSHP->fpSHX );

    if( fseek( hSHP->fpSHX, 0, SEEK_SET ) != 0
        || fread( abyHeader, 100, 1, hSHP->fpSHX ) != 1 )
        return FALSE;

    *((GInt32 *) (abyHeader + 32)) = CPL_LSBWORD32( nNewGeomType );

    if( fseek( hSHP->fpSHX, 0, SEEK_SET ) != 0
        || fwrite( abyHeader, 100, 1, hSHP->fpSHX ) != 1 )
        return FALSE;

    if( fseek( hSHP->fpSHX, nStartPos, SEEK_SET ) != 0 )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Update other information.                                       */
/* -------------------------------------------------------------------- */
    hSHP->nShapeType = nNewGeomType;

    return TRUE;
}

/************************************************************************/
/*                             SyncToDisk()                             */
/************************************************************************/

OGRErr OGRShapeLayer::SyncToDisk()

{
    if( bHeaderDirty )
    {
        if( hSHP != NULL )
            SHPWriteHeader( hSHP );

        if( hDBF != NULL )
            DBFUpdateHeader( hDBF );
        
        bHeaderDirty = FALSE;
    }

    if( hSHP != NULL )
    {
        fflush( hSHP->fpSHP );
        fflush( hSHP->fpSHX );
    }

    if( hDBF != NULL )
        fflush( hDBF->fp );

    return OGRERR_NONE;
}

/************************************************************************/
/*                          DropSpatialIndex()                          */
/************************************************************************/

OGRErr OGRShapeLayer::DropSpatialIndex()

{
    if( !CheckForQIX() )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Layer %s has no spatial index, DROP SPATIAL INDEX failed.",
                  poFeatureDefn->GetName() );
        return OGRERR_FAILURE;
    }

    VSIFClose( fpQIX );
    fpQIX = NULL;
    bCheckedForQIX = FALSE;
    
    const char *pszQIXFilename;

    pszQIXFilename = CPLResetExtension( pszFullName, "qix" );
    CPLDebug( "SHAPE", "Unlinking index file %s", pszQIXFilename );

    if( VSIUnlink( pszQIXFilename ) != 0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Failed to delete file %s.\n%s", 
                  pszQIXFilename, VSIStrerror( errno ) );
        return OGRERR_FAILURE;
    }
    else
        return OGRERR_NONE;
}

/************************************************************************/
/*                         CreateSpatialIndex()                         */
/************************************************************************/

OGRErr OGRShapeLayer::CreateSpatialIndex( int nMaxDepth )

{
/* -------------------------------------------------------------------- */
/*      If we have an existing spatial index, blow it away first.       */
/* -------------------------------------------------------------------- */
    if( CheckForQIX() )
        DropSpatialIndex();

    bCheckedForQIX = FALSE;

/* -------------------------------------------------------------------- */
/*      Build a quadtree structure for this file.                       */
/* -------------------------------------------------------------------- */
    SHPTree	*psTree;

    SyncToDisk();
    psTree = SHPCreateTree( hSHP, 2, nMaxDepth, NULL, NULL );

/* -------------------------------------------------------------------- */
/*      Trim unused nodes from the tree.                                */
/* -------------------------------------------------------------------- */
    SHPTreeTrimExtraNodes( psTree );

/* -------------------------------------------------------------------- */
/*      Dump tree to .qix file.                                         */
/* -------------------------------------------------------------------- */
    char *pszQIXFilename;

    pszQIXFilename = CPLStrdup(CPLResetExtension( pszFullName, "qix" ));

    CPLDebug( "SHAPE", "Creating index file %s", pszQIXFilename );

    SHPWriteTree( psTree, pszQIXFilename );
    CPLFree( pszQIXFilename );


/* -------------------------------------------------------------------- */
/*      cleanup                                                         */
/* -------------------------------------------------------------------- */
    SHPDestroyTree( psTree );

    CheckForQIX();

    return OGRERR_NONE;
}
