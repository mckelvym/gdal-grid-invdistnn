/******************************************************************************
 * $Id$
 *
 * Project:  FMEObjects Translator
 * Purpose:  Implementation of the OGRFMELayerCached class.  This is the
 *           class implementing behaviour for layers that are built into a
 *           temporary spatial cache (as opposed to live read database).
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, 2001 Safe Software Inc.
 * All Rights Reserved
 *
 * This software may not be copied or reproduced, in all or in part, 
 * without the prior written consent of Safe Software Inc.
 *
 * The entire risk as to the results and performance of the software,
 * supporting text and other information contained in this file
 * (collectively called the "Software") is with the user.  Although
 * Safe Software Incorporated has used considerable efforts in preparing 
 * the Software, Safe Software Incorporated does not warrant the
 * accuracy or completeness of the Software. In no event will Safe Software 
 * Incorporated be liable for damages, including loss of profits or 
 * consequential damages, arising out of the use of the Software.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.2  2002/11/08 21:20:58  warmerda
 * ensure a query is issued if resetreading never called
 *
 * Revision 1.1  2002/05/24 06:23:57  warmerda
 * New
 *
 * Revision 1.7  2002/05/24 06:17:01  warmerda
 * clean up dependencies on strimp.h, and fme2ogrspatialref func
 *
 * Revision 1.6  2002/05/24 03:59:56  warmerda
 * added support for cache index and related stuff
 *
 * Revision 1.5  2002/05/06 14:06:37  warmerda
 * override coordsys from cached features if needed
 *
 * Revision 1.4  2002/04/10 20:10:38  warmerda
 * Added support for getting geometry extents
 *
 * Revision 1.3  2001/11/19 22:12:14  warmerda
 * avoid small memory leak
 *
 * Revision 1.2  2001/11/01 21:52:39  warmerda
 * added mutex to protect access to session
 *
 * Revision 1.1  2001/09/07 15:53:39  warmerda
 * New
 *
 */

#include "fme2ogr.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                         OGRFMELayerCached()                          */
/************************************************************************/

OGRFMELayerCached::OGRFMELayerCached( OGRFMEDataSource *poDSIn ) 
        : OGRFMELayer( poDSIn )

{
    nPreviousFeature = -1;

    pszIndexBase = NULL;
    poIndex = NULL;

    memset( &sExtents, 0, sizeof(sExtents) );

    bQueryActive = FALSE;
}

/************************************************************************/
/*                         ~OGRFMELayerCached()                         */
/************************************************************************/

OGRFMELayerCached::~OGRFMELayerCached()

{
    CPLFree( pszIndexBase );
    if( poIndex != NULL )
    {
#ifdef SUPPORT_PERSISTENT_CACHE
        poIndex->close( FME_FALSE );
#else
        poIndex->close( FME_TRUE );
#endif
        poDS->GetFMESession()->destroySpatialIndex( poIndex );
    }
}

/************************************************************************/
/*                            AssignIndex()                             */
/*                                                                      */
/*      Assign spatial index .. spatial index is opened for read        */
/*      access.                                                         */
/************************************************************************/

int OGRFMELayerCached::AssignIndex( const char *pszBase,
                                    const OGREnvelope *psExtents,
                                    OGRSpatialReference *poSRS )
    
{
    CPLAssert( poIndex == NULL );
    
    pszIndexBase = CPLStrdup( pszBase );
    poIndex = 
        poDS->GetFMESession()->createSpatialIndex( pszBase, "READ", NULL );
    if( poIndex == NULL )
        return FALSE;
    if( poIndex->open() != 0 )
    {
        poDS->GetFMESession()->destroySpatialIndex( poIndex );
        poIndex = NULL;
        return FALSE;
    }

    if( psExtents != NULL )
        sExtents = *psExtents;

    if( poSRS != NULL )
    {
        if( poSpatialRef != NULL )
            delete poSpatialRef;
        poSpatialRef = poSRS;
    }

    return TRUE;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRFMELayerCached::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return FALSE;

    else if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return TRUE;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return TRUE;

    else if( EQUAL(pszCap,OLCFastGetExtent) )
        return TRUE;

    else 
        return FALSE;
}

/************************************************************************/
/*                        ReadNextIndexFeature()                        */
/************************************************************************/

OGRFeature *OGRFMELayerCached::ReadNextIndexFeature()

{
    OGRFeature          *poOGRFeature = NULL;
    FME_Boolean         endOfQuery;

    if( poIndex == NULL )
        return NULL;

    if( !bQueryActive )
        ResetReading();

    poDS->AcquireSession();

    if( poIndex->fetch( *poFMEFeature, endOfQuery ) == 0
        && !endOfQuery )
    {
        poOGRFeature = poDS->ProcessFeature( this, poFMEFeature );

        if( poOGRFeature != NULL )
        {
            poOGRFeature->SetFID( ++nPreviousFeature );
        }
    }

    poDS->ReleaseSession();

    return poOGRFeature;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRFMELayerCached::GetNextFeature()

{
    OGRFeature      *poFeature;

    while( TRUE ) 
    {
        poFeature = ReadNextIndexFeature();

        if( poFeature != NULL )
            nPreviousFeature = poFeature->GetFID();
        else
            break;

        if( m_poAttrQuery == NULL 
            || poIndex == NULL
            || m_poAttrQuery->Evaluate( poFeature ) )
            break;

        delete poFeature;
    }

    return poFeature;
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/
 
void OGRFMELayerCached::ResetReading()

{
    nPreviousFeature = -1;

    if( poIndex == NULL )
    {
        CPLAssert( FALSE );
        return;
    }

    poDS->AcquireSession();

    if( poFilterGeom == NULL )
    {
        poIndex->queryAll();
    }
    else
    {
        OGREnvelope      oEnvelope;

        poFilterGeom->getEnvelope( &oEnvelope );

        poFMEFeature->resetFeature();

        poFMEFeature->setDimension( FME_TWO_D );
        poFMEFeature->setGeometryType( FME_GEOM_LINE );
        poFMEFeature->addCoordinate( oEnvelope.MinX, oEnvelope.MinY );
        poFMEFeature->addCoordinate( oEnvelope.MaxX, oEnvelope.MaxY );
            
        poIndex->queryEnvelope( *poFMEFeature );
    }

    bQueryActive = TRUE;

    poDS->ReleaseSession();
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/************************************************************************/

int OGRFMELayerCached::GetFeatureCount( int bForce )

{
    int    nResult;

    poDS->AcquireSession();

    if( m_poAttrQuery != NULL || poIndex == NULL )
    {
        nResult = OGRLayer::GetFeatureCount( bForce );
    }
    else
    {
        ResetReading();
        nResult = poIndex->entries();
    }

    poDS->ReleaseSession();

    return nResult;
}

/************************************************************************/
/*                             GetExtent()                              */
/************************************************************************/

OGRErr OGRFMELayerCached::GetExtent( OGREnvelope *psExtent, int /* bForce */ )

{
    if( sExtents.MinX == 0.0 && sExtents.MaxX == 0.0
        && sExtents.MinY == 0.0 && sExtents.MaxY == 0.0 )
        return OGRERR_FAILURE;

    *psExtent = sExtents;

    return OGRERR_NONE;
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *OGRFMELayerCached::SerializeToXML()

{
    CPLXMLNode      *psLayer;
    char            szGeomType[64];

    psLayer = CPLCreateXMLNode( NULL, CXT_Element, "OGRLayer" );
    
/* -------------------------------------------------------------------- */
/*      Handle various layer values.                                    */
/* -------------------------------------------------------------------- */
    CPLCreateXMLElementAndValue( psLayer, "Name", poFeatureDefn->GetName());
    sprintf( szGeomType, "%d", (int) poFeatureDefn->GetGeomType() );
    CPLCreateXMLElementAndValue( psLayer, "GeomType", szGeomType );    

    CPLCreateXMLElementAndValue( psLayer, "SpatialCacheName", 
                                 pszIndexBase );
    
/* -------------------------------------------------------------------- */
/*      Handle spatial reference if available.                          */
/* -------------------------------------------------------------------- */
    if( GetSpatialRef() != NULL )
    {
        char *pszWKT = NULL;
        OGRSpatialReference *poSRS = GetSpatialRef();
        
        poSRS->exportToWkt( &pszWKT );

        if( pszWKT != NULL )
        {
            CPLCreateXMLElementAndValue( psLayer, "SRS", pszWKT );
            CPLFree( pszWKT );
        }
    }

/* -------------------------------------------------------------------- */
/*      Handle extents if available.                                    */
/* -------------------------------------------------------------------- */
    OGREnvelope sEnvelope;
    if( GetExtent( &sEnvelope, FALSE ) == OGRERR_NONE )
    {
        char szExtent[512];

        sprintf( szExtent, "%24.15E,%24.15E,%24.15E,%24.15E", 
                 sEnvelope.MinX, sEnvelope.MinY, 
                 sEnvelope.MaxX, sEnvelope.MaxY );
        CPLCreateXMLElementAndValue( psLayer, "Extent", szExtent );
    }

/* -------------------------------------------------------------------- */
/*      Emit the field schemas.                                         */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psSchema = CPLCreateXMLNode( psLayer, CXT_Element, "Schema" );
    
    for( int iField = 0; iField < poFeatureDefn->GetFieldCount(); iField++ )
    {
        OGRFieldDefn *poFieldDef = poFeatureDefn->GetFieldDefn( iField );
        const char *pszType;
        char szWidth[32], szPrecision[32];
        CPLXMLNode *psXMLFD;

        sprintf( szWidth, "%d", poFieldDef->GetWidth() );
        sprintf( szPrecision, "%d", poFieldDef->GetPrecision() );
        
        if( poFieldDef->GetType() == OFTInteger )
            pszType = "Integer";
        else if( poFieldDef->GetType() == OFTIntegerList )
            pszType = "IntegerList";
        else if( poFieldDef->GetType() == OFTReal )
            pszType = "Real";
        else if( poFieldDef->GetType() == OFTRealList )
            pszType = "RealList";
        else if( poFieldDef->GetType() == OFTString )
            pszType = "String";
        else if( poFieldDef->GetType() == OFTStringList )
            pszType = "StringList";
        else if( poFieldDef->GetType() == OFTWideString )
            pszType = "WideString";
        else if( poFieldDef->GetType() == OFTWideStringList )
            pszType = "WideStringList";
        else if( poFieldDef->GetType() == OFTBinary )
            pszType = "Binary";
        else
            pszType = "Unsupported";

        psXMLFD = CPLCreateXMLNode( psSchema, CXT_Element, "OGRFieldDefn" );
        CPLCreateXMLElementAndValue( psXMLFD, "Name",poFieldDef->GetNameRef());
        CPLCreateXMLElementAndValue( psXMLFD, "Type", pszType );
        CPLCreateXMLElementAndValue( psXMLFD, "Width", szWidth );
        CPLCreateXMLElementAndValue( psXMLFD, "Precision", szPrecision );
    }

    return psLayer;
}

/************************************************************************/
/*                         InitializeFromXML()                          */
/************************************************************************/

int OGRFMELayerCached::InitializeFromXML( CPLXMLNode *psLayer )

{
/* -------------------------------------------------------------------- */
/*      Create the feature definition.                                  */
/* -------------------------------------------------------------------- */
    poFeatureDefn = new OGRFeatureDefn( CPLGetXMLValue(psLayer,"Name","X") );
    
/* -------------------------------------------------------------------- */
/*      Set the geometry type, if available.                            */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLNode( psLayer, "GeomType" ) != NULL )
        poFeatureDefn->SetGeomType( (OGRwkbGeometryType) 
                     atoi(CPLGetXMLValue( psLayer, "GeomType", "0" )) );

/* -------------------------------------------------------------------- */
/*      If we can extract extents, apply them to the layer.             */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLNode( psLayer, "Extent" ) != NULL )
    {
        if( sscanf( CPLGetXMLValue( psLayer, "Extent", "" ), 
                    "%lf,%lf,%lf,%lf", 
                    &sExtents.MinX, 
                    &sExtents.MaxX, 
                    &sExtents.MinY, 
                    &sExtents.MaxY ) != 4 )
        {
            memset( &sExtents, 0, sizeof(sExtents) );
        }
    }

/* -------------------------------------------------------------------- */
/*      Apply a SRS if found in the XML.                                */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLNode( psLayer, "SRS" ) != NULL )
    {
        char *pszSRS = (char *) CPLGetXMLValue( psLayer, "SRS", "" );
        OGRSpatialReference oSRS;

        if( oSRS.importFromWkt( &pszSRS ) == OGRERR_NONE )
        {
            if( poSpatialRef != NULL )
                delete poSpatialRef;

            poSpatialRef = oSRS.Clone();
        }
    }

/* -------------------------------------------------------------------- */
/*      Apply the Schema.                                               */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psFieldDef;

    for( psFieldDef = CPLGetXMLNode( psLayer, "Schema.OGRFieldDefn" );
         psFieldDef != NULL; psFieldDef = psFieldDef->psNext )
    {
        const char *pszType;
        OGRFieldType eType;

        if( !EQUAL(psFieldDef->pszValue,"OGRFieldDefn") )
            continue;

        pszType = CPLGetXMLValue( psFieldDef, "type", "String" );
        if( EQUAL(pszType,"String") )
            eType = OFTString;
        else if( EQUAL(pszType,"StringList") )
            eType = OFTStringList;
        else if( EQUAL(pszType,"Integer") )
            eType = OFTInteger;
        else if( EQUAL(pszType,"IntegerList") )
            eType = OFTIntegerList;
        else if( EQUAL(pszType,"Real") )
            eType = OFTReal;
        else if( EQUAL(pszType,"RealList") )
            eType = OFTRealList;
        else if( EQUAL(pszType,"WideString") )
            eType = OFTWideString;
        else if( EQUAL(pszType,"WideStringList") )
            eType = OFTWideStringList;
        else if( EQUAL(pszType,"Binary") )
            eType = OFTBinary;
        else
            eType = OFTString;

        OGRFieldDefn oField( CPLGetXMLValue( psFieldDef, "name", "default" ),
                             eType );

        oField.SetWidth( atoi(CPLGetXMLValue(psFieldDef,"width","0")) );
        oField.SetPrecision( atoi(CPLGetXMLValue(psFieldDef,"precision","0")));

        poFeatureDefn->AddFieldDefn( &oField );
    }

/* -------------------------------------------------------------------- */
/*      Ensure we have a working string and feature object              */
/* -------------------------------------------------------------------- */
    poFMEFeature = poDS->GetFMESession()->createFeature();

    return TRUE;
}
