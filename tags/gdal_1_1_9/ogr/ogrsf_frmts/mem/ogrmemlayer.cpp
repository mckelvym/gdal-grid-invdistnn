/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRMemLayer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.2  2003/04/08 20:58:16  warmerda
 * added support for adding fields when existing features exist
 *
 * Revision 1.1  2003/04/08 19:32:47  warmerda
 * New
 *
 */

#include "ogr_mem.h"
#include "cpl_conv.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                            OGRMemLayer()                             */
/************************************************************************/

OGRMemLayer::OGRMemLayer( const char * pszName, OGRSpatialReference *poSRSIn, 
                          OGRwkbGeometryType eReqType )

{
    poFilterGeom = NULL;
    if( poSRSIn == NULL )
        poSRS = NULL;
    else
        poSRS = poSRSIn->Clone();
    
    iNextReadFID = 0;
    iNextCreateFID = 0;

    nFeatureCount = 0;
    nMaxFeatureCount = 0;
    papoFeatures = NULL;

    poFeatureDefn = new OGRFeatureDefn( pszName );
    poFeatureDefn->SetGeomType( eReqType );
}

/************************************************************************/
/*                           ~OGRMemLayer()                           */
/************************************************************************/

OGRMemLayer::~OGRMemLayer()

{
    for( int i = 0; i < nMaxFeatureCount; i++ )
    {
        if( papoFeatures[i] != NULL )
            delete papoFeatures[i];
    }
    CPLFree( papoFeatures );
    
    delete poFeatureDefn;

    if( poSRS != NULL && poSRS->Dereference() == 0 )
        delete poSRS;

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRMemLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

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

void OGRMemLayer::ResetReading()

{
    iNextReadFID = 0;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRMemLayer::GetNextFeature()

{
    while( iNextReadFID < nMaxFeatureCount )
    {
        OGRFeature *poFeature = papoFeatures[iNextReadFID++];

        if( poFeature == NULL )
            continue;

        if( (poFilterGeom == NULL
            || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) )
            && (m_poAttrQuery == NULL
                || m_poAttrQuery->Evaluate( poFeature )) )
            return poFeature->Clone();
    }

    return NULL;
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRMemLayer::GetFeature( long nFeatureId )

{
    if( nFeatureId < 0 || nFeatureId >= nMaxFeatureCount )
        return NULL;
    else if( papoFeatures[nFeatureId] == NULL )
        return NULL;
    else
        return papoFeatures[nFeatureId]->Clone();
}

/************************************************************************/
/*                             SetFeature()                             */
/************************************************************************/

OGRErr OGRMemLayer::SetFeature( OGRFeature *poFeature )

{
    if( poFeature == NULL )
        return OGRERR_FAILURE;

    if( poFeature->GetFID() == OGRNullFID )
    {
        while( iNextCreateFID < nMaxFeatureCount 
               && papoFeatures[iNextCreateFID] != NULL )
            iNextCreateFID++;
        poFeature->SetFID( iNextCreateFID++ );
    }

    if( poFeature->GetFID() >= nMaxFeatureCount )
    {
        int nNewCount = MAX(2*nMaxFeatureCount+10, poFeature->GetFID() + 1 );

        papoFeatures = (OGRFeature **) 
            CPLRealloc( papoFeatures, sizeof(OGRFeature *) * nNewCount);
        memset( papoFeatures + nMaxFeatureCount, 0, 
                sizeof(OGRFeature *) * (nNewCount - nMaxFeatureCount) );
        nMaxFeatureCount = nNewCount;
    }

    if( papoFeatures[poFeature->GetFID()] != NULL )
    {
        delete papoFeatures[poFeature->GetFID()];
        papoFeatures[poFeature->GetFID()] = NULL;
        nFeatureCount--;
    }

    papoFeatures[poFeature->GetFID()] = poFeature->Clone();
    nFeatureCount++;

    return OGRERR_NONE;
}

/************************************************************************/
/*                           CreateFeature()                            */
/************************************************************************/

OGRErr OGRMemLayer::CreateFeature( OGRFeature *poFeature )

{
    if( poFeature->GetFID() != OGRNullFID 
        && poFeature->GetFID() >= 0
        && poFeature->GetFID() < nMaxFeatureCount )
    {
        if( papoFeatures[poFeature->GetFID()] != NULL )
            poFeature->SetFID( OGRNullFID );
    }

    if( poFeature->GetFID() > 10000000 )
        poFeature->SetFID( OGRNullFID );

    return SetFeature( poFeature );
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/*                                                                      */
/*      If a spatial filter is in effect, we turn control over to       */
/*      the generic counter.  Otherwise we return the total count.      */
/*      Eventually we should consider implementing a more efficient     */
/*      way of counting features matching a spatial query.              */
/************************************************************************/

int OGRMemLayer::GetFeatureCount( int bForce )

{
    if( poFilterGeom != NULL || m_poAttrQuery != NULL )
        return OGRLayer::GetFeatureCount( bForce );
    else
        return nFeatureCount;
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

OGRErr OGRMemLayer::GetExtent (OGREnvelope *psExtent, int bForce)

{
    return OGRLayer::GetExtent( psExtent, bForce );
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRMemLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return TRUE;

    else if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return TRUE;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return poFilterGeom == NULL;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastGetExtent) )
        return TRUE;

    else if( EQUAL(pszCap,OLCCreateField) )
        return TRUE;

    else 
        return FALSE;
}

/************************************************************************/
/*                            CreateField()                             */
/************************************************************************/

OGRErr OGRMemLayer::CreateField( OGRFieldDefn *poField, int bApproxOK )

{
/* -------------------------------------------------------------------- */
/*      simple case, no features exist yet.                             */
/* -------------------------------------------------------------------- */
    if( nFeatureCount == 0 )
    {
        poFeatureDefn->AddFieldDefn( poField );
        return OGRERR_NONE;
    }

/* -------------------------------------------------------------------- */
/*      Add field definition and setup remap definition.                */
/* -------------------------------------------------------------------- */
    int  *panRemap;
    int   i;

    poFeatureDefn->AddFieldDefn( poField );

    panRemap = (int *) CPLMalloc(sizeof(int) * poFeatureDefn->GetFieldCount());
    for( i = 0; i < poFeatureDefn->GetFieldCount(); i++ )
    {
        if( i < poFeatureDefn->GetFieldCount() - 1 )
            panRemap[i] = i;
        else
            panRemap[i] = -1;
    }

/* -------------------------------------------------------------------- */
/*      Remap all the internal features.  Hopefully there aren't any    */
/*      external features referring to our OGRFeatureDefn!              */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nMaxFeatureCount; i++ )
    {
        if( papoFeatures[i] != NULL )
            papoFeatures[i]->RemapFields( NULL, panRemap );
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRMemLayer::GetSpatialRef()

{
    return poSRS;
}
