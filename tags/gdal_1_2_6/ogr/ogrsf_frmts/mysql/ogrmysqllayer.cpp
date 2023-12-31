/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRMySQLLayer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.3  2005/02/22 12:54:27  fwarmerdam
 * use OGRLayer base spatial filter support
 *
 * Revision 1.2  2005/02/02 20:54:27  fwarmerdam
 * track m_nFeaturesRead
 *
 * Revision 1.1  2004/10/07 20:56:15  fwarmerdam
 * New
 *
 */

#include "ogr_mysql.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           OGRMySQLLayer()                            */
/************************************************************************/

OGRMySQLLayer::OGRMySQLLayer()

{
    poDS = NULL;

    pszGeomColumn = NULL;
    pszQueryStatement = NULL;

    bHasFid = FALSE;
    pszFIDColumn = NULL;

    iNextShapeId = 0;
    nResultOffset = 0;

    poSRS = NULL;
    nSRSId = -2; // we haven't even queried the database for it yet. 

    poFeatureDefn = NULL;

    hResultSet = NULL;
}

/************************************************************************/
/*                           ~OGRMySQLLayer()                           */
/************************************************************************/

OGRMySQLLayer::~OGRMySQLLayer()

{
    if( m_nFeaturesRead > 0 && poFeatureDefn != NULL )
    {
        CPLDebug( "MySQL", "%d features read on layer '%s'.",
                  (int) m_nFeaturesRead, 
                  poFeatureDefn->GetName() );
    }

    ResetReading();

    CPLFree( pszGeomColumn );
    CPLFree( pszFIDColumn );
    CPLFree( pszQueryStatement );

    if( poSRS != NULL )
        poSRS->Dereference();

    if( poFeatureDefn )
        delete poFeatureDefn;
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRMySQLLayer::ResetReading()

{
    iNextShapeId = 0;

    if( hResultSet != NULL )
    {
        mysql_free_result( hResultSet );
        hResultSet = NULL;

        poDS->InterruptLongResult();
    }
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRMySQLLayer::GetNextFeature()

{

    for( ; TRUE; )
    {
        OGRFeature      *poFeature;

        poFeature = GetNextRawFeature();
        if( poFeature == NULL )
            return NULL;

        if( (m_poFilterGeom == NULL
            || FilterGeometry( poFeature->GetGeometryRef() ) )
            && (m_poAttrQuery == NULL
                || m_poAttrQuery->Evaluate( poFeature )) )
            return poFeature;

        delete poFeature;
    }
}
/************************************************************************/
/*                          RecordToFeature()                           */
/*                                                                      */
/*      Convert the indicated record of the current result set into     */
/*      a feature.                                                      */
/************************************************************************/

OGRFeature *OGRMySQLLayer::RecordToFeature( char **papszRow )

{
    mysql_field_seek( hResultSet, 0 );

/* -------------------------------------------------------------------- */
/*      Create a feature from the current result.                       */
/* -------------------------------------------------------------------- */
    int         iField;
    OGRFeature *poFeature = new OGRFeature( poFeatureDefn );

    poFeature->SetFID( iNextShapeId );
    m_nFeaturesRead++;

/* ==================================================================== */
/*      Transfer all result fields we can.                              */
/* ==================================================================== */
    for( iField = 0; 
         iField < (int) mysql_num_fields(hResultSet);
         iField++ )
    {
        int     iOGRField;
        MYSQL_FIELD *psMSField = mysql_fetch_field(hResultSet);

/* -------------------------------------------------------------------- */
/*      Handle FID.                                                     */
/* -------------------------------------------------------------------- */
        if( bHasFid && EQUAL(psMSField->name,pszFIDColumn) )
        {
            if( papszRow[iField] == NULL )
            {
                CPLError( CE_Failure, CPLE_AppDefined,
                          "NULL primary key in RecordToFeature()" );
                return NULL;
            }

            poFeature->SetFID( atoi(papszRow[iField]) );
        }

        if( papszRow[iField] == NULL )
            continue;

/* -------------------------------------------------------------------- */
/*      Transfer regular data fields.                                   */
/* -------------------------------------------------------------------- */
        iOGRField = poFeatureDefn->GetFieldIndex(psMSField->name);

        if( iOGRField < 0 )
            continue;

        poFeature->SetField( iOGRField, papszRow[iField] );
    }

    return poFeature;
}

/************************************************************************/
/*                         GetNextRawFeature()                          */
/************************************************************************/

OGRFeature *OGRMySQLLayer::GetNextRawFeature()

{
/* -------------------------------------------------------------------- */
/*      Do we need to establish an initial query?                       */
/* -------------------------------------------------------------------- */
    if( iNextShapeId == 0 && hResultSet == NULL )
    {
        CPLAssert( pszQueryStatement != NULL );

        poDS->RequestLongResult( this );

        if( mysql_query( poDS->GetConn(), pszQueryStatement ) )
        {
            poDS->ReportError( pszQueryStatement );
            return NULL;
        }

        hResultSet = mysql_use_result( poDS->GetConn() );
        if( hResultSet == NULL )
        {
            poDS->ReportError( "mysql_use_result() failed on query." );
            return FALSE;
        }
    }

/* -------------------------------------------------------------------- */
/*      Fetch next record.                                              */
/* -------------------------------------------------------------------- */
    char **papszRow;

    papszRow = mysql_fetch_row( hResultSet );
    if( papszRow == NULL )
    {
        ResetReading();
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Process record.                                                 */
/* -------------------------------------------------------------------- */
    OGRFeature *poFeature = RecordToFeature( papszRow );

    iNextShapeId++;

    return poFeature;
}

/************************************************************************/
/*                             GetFeature()                             */
/*                                                                      */
/*      Note that we actually override this in OGRMySQLTableLayer.      */
/************************************************************************/

OGRFeature *OGRMySQLLayer::GetFeature( long nFeatureId )

{
    return OGRLayer::GetFeature( nFeatureId );
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRMySQLLayer::TestCapability( const char * pszCap )

{
    return FALSE;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRMySQLLayer::GetSpatialRef()

{
#ifdef notdef
    if( poSRS == NULL && nSRSId > -1 )
    {
        poSRS = poDS->FetchSRS( nSRSId );
        if( poSRS != NULL )
            poSRS->Reference();
        else
            nSRSId = -1;
    }

    return poSRS;
#endif

    return NULL;
}
