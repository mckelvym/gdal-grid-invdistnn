/******************************************************************************
 * $Id$
 *
 * Project:  TIGER/Line Translator
 * Purpose:  Implements OGRTigerLayer class.
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
 * Revision 1.8  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.7  2001/07/04 23:25:32  warmerda
 * first round implementation of writer
 *
 * Revision 1.6  2001/07/04 03:08:54  warmerda
 * fixed bug in GetNextFeature
 *
 * Revision 1.5  2001/06/19 15:50:23  warmerda
 * added feature attribute query support
 *
 * Revision 1.4  2001/01/19 21:15:20  warmerda
 * expanded tabs
 *
 * Revision 1.3  2000/01/13 05:18:11  warmerda
 * added support for multiple versions
 *
 * Revision 1.2  1999/12/22 15:38:15  warmerda
 * major update
 *
 * Revision 1.1  1999/10/07 18:19:21  warmerda
 * New
 *
 */

#include "ogr_tiger.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           OGRTigerLayer()                            */
/*                                                                      */
/*      Note that the OGRTigerLayer assumes ownership of the passed     */
/*      OGRFeatureDefn object.                                          */
/************************************************************************/

OGRTigerLayer::OGRTigerLayer( OGRTigerDataSource *poDSIn,
                              TigerFileBase * poReaderIn )

{
    poFilterGeom = NULL;

    poDS = poDSIn;
    poReader = poReaderIn;

    iLastFeatureId = 0;
    iLastModule = -1;

    nFeatureCount = 0;
    panModuleFCount = NULL;
    panModuleOffset = NULL;

/* -------------------------------------------------------------------- */
/*      Setup module feature counts.                                    */
/* -------------------------------------------------------------------- */
    if( !poDS->GetWriteMode() )
    {
        panModuleFCount = (int *) 
            CPLCalloc(poDS->GetModuleCount(),sizeof(int));
        panModuleOffset = (int *) 
            CPLCalloc(poDS->GetModuleCount()+1,sizeof(int));

        nFeatureCount = 0;

        for( int iModule = 0; iModule < poDS->GetModuleCount(); iModule++ )
        {
            if( poReader->SetModule( poDS->GetModule(iModule) ) )
                panModuleFCount[iModule] = poReader->GetFeatureCount();
            else
                panModuleFCount[iModule] = 0;

            panModuleOffset[iModule] = nFeatureCount;
            nFeatureCount += panModuleFCount[iModule];
        }

        // this entry is just to make range comparisons easy without worrying
        // about falling off the end of the array.
        panModuleOffset[poDS->GetModuleCount()] = nFeatureCount;
    }

    poReader->SetModule( NULL );
}

/************************************************************************/
/*                           ~OGRTigerLayer()                           */
/************************************************************************/

OGRTigerLayer::~OGRTigerLayer()

{
    delete poReader;

    if( poFilterGeom != NULL )
        delete poFilterGeom;

    CPLFree( panModuleFCount );
    CPLFree( panModuleOffset );
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRTigerLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        poFilterGeom = poGeomIn->clone();
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRTigerLayer::ResetReading()

{
    iLastFeatureId = 0;
    iLastModule = -1;
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRTigerLayer::GetFeature( long nFeatureId )

{
    if( nFeatureId < 1 || nFeatureId > nFeatureCount )
        return NULL;

/* -------------------------------------------------------------------- */
/*      If we don't have the current module open for the requested      */
/*      data, then open it now.                                         */
/* -------------------------------------------------------------------- */
    if( iLastModule == -1 
        || nFeatureId <= panModuleOffset[iLastModule]
        || nFeatureId > panModuleOffset[iLastModule+1] )
    {
        for( iLastModule = 0;
             iLastModule < poDS->GetModuleCount()
                 && nFeatureId > panModuleOffset[iLastModule+1];
             iLastModule++ ) {}

        if( !poReader->SetModule( poDS->GetModule(iLastModule) ) )
        {
            return NULL;
        }
    }

/* -------------------------------------------------------------------- */
/*      Fetch the feature associated with the record.                   */
/* -------------------------------------------------------------------- */
    OGRFeature  *poFeature;

    poFeature =
        poReader->GetFeature( nFeatureId-panModuleOffset[iLastModule]-1 );

    if( poFeature != NULL )
    {
        poFeature->SetFID( nFeatureId );

        if( poFeature->GetGeometryRef() != NULL )
            poFeature->GetGeometryRef()->assignSpatialReference(
                poDS->GetSpatialRef() );

        poFeature->SetField( 0, poReader->GetShortModule() );
    }

    return poFeature;
}


/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRTigerLayer::GetNextFeature()

{
/* -------------------------------------------------------------------- */
/*      Read features till we find one that satisfies our current       */
/*      spatial criteria.                                               */
/* -------------------------------------------------------------------- */
    while( iLastFeatureId < nFeatureCount )
    {
        OGRFeature      *poFeature = GetFeature( ++iLastFeatureId );

        if( poFeature == NULL )
            break;

        if( (poFilterGeom == NULL
             || poFeature->GetGeometryRef() == NULL 
             || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) )
            && (m_poAttrQuery == NULL
                || m_poAttrQuery->Evaluate( poFeature )) )
            return poFeature;

        delete poFeature;
    }

    return NULL;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRTigerLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return TRUE;

    else if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return TRUE;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else if( EQUAL(pszCap,OLCSequentialWrite) )
        return poDS->GetWriteMode();

    else 
        return FALSE;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRTigerLayer::GetSpatialRef()

{
    return poDS->GetSpatialRef();
}

/************************************************************************/
/*                            GetLayerDefn()                            */
/************************************************************************/

OGRFeatureDefn *OGRTigerLayer::GetLayerDefn()

{
    return poReader->GetFeatureDefn();
}

/************************************************************************/
/*                            CreateField()                             */
/************************************************************************/

OGRErr OGRTigerLayer::CreateField( OGRFieldDefn *poField, int bApproxOK )

{
    /* notdef/TODO: I should add some checking here eventually. */

    return OGRERR_NONE;
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

OGRErr OGRTigerLayer::CreateFeature( OGRFeature *poFeature )

{
    return poReader->CreateFeature( poFeature );
}

