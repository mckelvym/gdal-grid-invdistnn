/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRMySQLLayer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 * Author:   Howard Butler, hobu@hobu.net
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
 * Revision 1.17  2006/02/16 17:21:53  fwarmerdam
 * improve initialization
 *
 * Revision 1.16  2006/02/16 16:53:57  fwarmerdam
 * Remove debug statement for null attributes,  it is way overkill.
 *
 * Revision 1.15  2006/02/13 04:15:04  hobu
 * major formatting cleanup
 * Added myself as an author
 *
 * Revision 1.14  2006/02/11 18:08:34  hobu
 * Moved FetchSRS to happen on the datasource like PG
 * Implemented CreateField for TableLayer
 *
 * Revision 1.13  2006/02/07 18:17:15  hobu
 * Always return geometries as "native"
 *
 * Revision 1.12  2006/02/02 01:24:17  hobu
 * make sure we properly sett hResultSet to NULL after all frees.
 *
 * Revision 1.11  2006/02/01 01:40:09  hobu
 * separate fetching of SRID
 *
 * Revision 1.10  2006/01/31 06:17:08  hobu
 * move SRS fetching to mysqllayer.cpp
 *
 * Revision 1.9  2006/01/31 05:35:02  hobu
 * move SRS fetching to mysqllayer.cpp
 *
 * Revision 1.8  2006/01/31 02:48:09  fwarmerdam
 * Added heuristic to determine whether we have WKB or SRID+WKB.
 *
 * Revision 1.7  2006/01/27 01:27:49  fwarmerdam
 * added GetFIDColumn and GetGeometryColumn support
 *
 * Revision 1.6  2006/01/16 16:05:38  hobu
 * Handle geometry column
 *
 * Revision 1.5  2005/09/21 01:00:01  fwarmerdam
 * fixup OGRFeatureDefn and OGRSpatialReference refcount handling
 *
 * Revision 1.4  2005/08/30 23:53:16  fwarmerdam
 * implement binary field support
 *
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
    pszGeomColumnTable = NULL;
    pszFIDColumn = NULL;
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
    CPLFree( pszGeomColumnTable );
    CPLFree( pszFIDColumn );
    CPLFree( pszQueryStatement );

    if( poSRS != NULL )
        poSRS->Release();

    if( poFeatureDefn )
        poFeatureDefn->Release();
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

OGRFeature *OGRMySQLLayer::RecordToFeature( char **papszRow,
                                            unsigned long *panLengths )

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
        {
//            CPLDebug("MYSQL", "%s was null for %d", psMSField->name,
//                     iNextShapeId);
            continue;
        }

/* -------------------------------------------------------------------- */
/*      Handle MySQL geometry                                           */
/* -------------------------------------------------------------------- */
        if( pszGeomColumn && EQUAL(psMSField->name,pszGeomColumn))
        {
            OGRGeometry *poGeometry = NULL;
            
            // Geometry columns will have the first 4 bytes contain the SRID.
            OGRGeometryFactory::createFromWkb(
                ((GByte *)papszRow[iField]) + 4, 
                NULL,
                &poGeometry,
                panLengths[iField] - 4 );

            if( poGeometry != NULL )
                poFeature->SetGeometryDirectly( poGeometry );

            continue;
        }


/* -------------------------------------------------------------------- */
/*      Transfer regular data fields.                                   */
/* -------------------------------------------------------------------- */
        iOGRField = poFeatureDefn->GetFieldIndex(psMSField->name);
        if( iOGRField < 0 )
            continue;

        OGRFieldDefn *psFieldDefn = poFeatureDefn->GetFieldDefn( iOGRField );

        if( psFieldDefn->GetType() == OFTBinary )
        {
            poFeature->SetField( iOGRField, panLengths[iField], 
                                 (GByte *) papszRow[iField] );
        }
        else
        {
            poFeature->SetField( iOGRField, papszRow[iField] );
        }
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
    unsigned long *panLengths;

    papszRow = mysql_fetch_row( hResultSet );
    if( papszRow == NULL )
    {
        ResetReading();
        return NULL;
    }

    panLengths = mysql_fetch_lengths( hResultSet );

/* -------------------------------------------------------------------- */
/*      Process record.                                                 */
/* -------------------------------------------------------------------- */
    OGRFeature *poFeature = RecordToFeature( papszRow, panLengths );

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
    if( EQUAL(pszCap,OLCRandomRead) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return TRUE;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return TRUE;

    else if( EQUAL(pszCap,OLCTransactions) )
        return FALSE;

	else if( EQUAL(pszCap,OLCFastGetExtent) )
		return FALSE;

    else
        return FALSE;
}



    
/************************************************************************/
/*                            GetFIDColumn()                            */
/************************************************************************/

const char *OGRMySQLLayer::GetFIDColumn() 

{
    if( pszFIDColumn != NULL )
        return pszFIDColumn;
    else
        return "";
}

/************************************************************************/
/*                         GetGeometryColumn()                          */
/************************************************************************/

const char *OGRMySQLLayer::GetGeometryColumn() 

{
    if( pszGeomColumn != NULL )
        return pszGeomColumn;
    else
        return "";
}


/************************************************************************/
/*                         FetchSRSId()                                 */
/************************************************************************/

int OGRMySQLLayer::FetchSRSId()
{
	char         szCommand[1024];
    char           **papszRow;  
    
    if( hResultSet != NULL )
        mysql_free_result( hResultSet );
		hResultSet = NULL;
				
    sprintf( szCommand, 
             "SELECT srid FROM geometry_columns "
             "WHERE f_table_name = '%s'",
             pszGeomColumnTable );

    if( !mysql_query( poDS->GetConn(), szCommand ) )
        hResultSet = mysql_store_result( poDS->GetConn() );

    papszRow = NULL;
    if( hResultSet != NULL )
        papszRow = mysql_fetch_row( hResultSet );
        

    if( papszRow != NULL && papszRow[0] != NULL )
    {
        nSRSId = atoi(papszRow[0]);
    }

    // make sure to free our results
    if( hResultSet != NULL )
        mysql_free_result( hResultSet );
		hResultSet = NULL;
        
	return nSRSId;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRMySQLLayer::GetSpatialRef()

{


    if( poSRS == NULL && nSRSId > -1 )
    {
        poSRS = poDS->FetchSRS( nSRSId );
        if( poSRS != NULL )
            poSRS->Reference();
        else
            nSRSId = -1;
    }

    return poSRS;

}
