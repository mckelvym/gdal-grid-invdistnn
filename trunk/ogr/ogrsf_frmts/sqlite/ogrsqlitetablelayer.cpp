/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRSQLiteTableLayer class, access to an existing table.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam
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
 * Revision 1.2  2004/07/11 19:23:51  warmerda
 * read implementation working well
 *
 * Revision 1.1  2004/07/09 06:25:05  warmerda
 * New
 *
 */

#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_sqlite.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                        OGRSQLiteTableLayer()                         */
/************************************************************************/

OGRSQLiteTableLayer::OGRSQLiteTableLayer( OGRSQLiteDataSource *poDSIn )

{
    poDS = poDSIn;

    pszQuery = NULL;

    bUpdateAccess = TRUE;

    iNextShapeId = 0;

    nSRSId = -1;

    poFeatureDefn = NULL;
}

/************************************************************************/
/*                        ~OGRSQLiteTableLayer()                        */
/************************************************************************/

OGRSQLiteTableLayer::~OGRSQLiteTableLayer()

{
    CPLFree( pszQuery );
    ClearStatement();
}

/************************************************************************/
/*                             Initialize()                             */
/************************************************************************/

CPLErr OGRSQLiteTableLayer::Initialize( const char *pszTableName, 
                                      const char *pszGeomCol )

{
    sqlite3 *hDB = poDS->GetDB();

    CPLFree( pszGeomColumn );
    if( pszGeomCol == NULL )
        pszGeomColumn = NULL;
    else
        pszGeomColumn = CPLStrdup( pszGeomCol );

    CPLFree( pszFIDColumn );
    pszFIDColumn = NULL;

/* -------------------------------------------------------------------- */
/*      Do we have a simple primary key?                                */
/*                                                                      */
/*      I don't know a way to determine it, so we will just             */
/*      explicitly use the rowid.                                       */
/* -------------------------------------------------------------------- */

    pszFIDColumn = CPLStrdup("_rowid_");

/* -------------------------------------------------------------------- */
/*      Get the column definitions for this table.                      */
/* -------------------------------------------------------------------- */
    CPLErr eErr;
    sqlite3_stmt *hColStmt = NULL;
    const char *pszSQL = CPLSPrintf( "SELECT _rowid_, * FROM %s LIMIT 1",
                                     pszTableName );

    if( sqlite3_prepare( hDB, pszSQL, strlen(pszSQL), &hColStmt, NULL )
        != SQLITE_OK )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Unable to query table %s for column definitions.",
                  pszTableName );
        
        return CE_Failure;
    }
    
    sqlite3_step( hColStmt );

    eErr = BuildFeatureDefn( pszTableName, hColStmt );
    if( eErr != CE_None )
        return eErr;

    if( poFeatureDefn->GetFieldCount() == 0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "No column definitions found for table '%s', layer not usable.", 
                  pszTableName );
        return CE_Failure;
    }

    sqlite3_finalize( hColStmt );

    return CE_None;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRSQLiteTableLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        poFilterGeom = poGeomIn->clone();

    ResetReading();
}

/************************************************************************/
/*                           ClearStatement()                           */
/************************************************************************/

void OGRSQLiteTableLayer::ClearStatement()

{
    if( hStmt != NULL )
    {
	sqlite3_finalize( hStmt );
        hStmt = NULL;
    }
}

/************************************************************************/
/*                            GetStatement()                            */
/************************************************************************/

sqlite3_stmt *OGRSQLiteTableLayer::GetStatement()

{
    if( hStmt == NULL )
        ResetStatement();

    return hStmt;
}

/************************************************************************/
/*                           ResetStatement()                           */
/************************************************************************/

OGRErr OGRSQLiteTableLayer::ResetStatement()

{
    int rc;
    char *pszSQL;

    ClearStatement();

    iNextShapeId = 0;

    if( pszQuery != NULL )
        pszSQL = CPLStrdup( CPLSPrintf( "SELECT _rowid_, * FROM %s WHERE %s", 
				        poFeatureDefn->GetName(), 
					pszQuery ) );
    else
        pszSQL = CPLStrdup( CPLSPrintf( "SELECT _rowid_, * FROM %s", 
				        poFeatureDefn->GetName() ) );

    rc = sqlite3_prepare( poDS->GetDB(), pszSQL, strlen(pszSQL), 
	    	          &hStmt, NULL );

    if( rc == SQLITE_OK )
    	return OGRERR_NONE;
    else
    {
        hStmt = NULL;
        return OGRERR_FAILURE;
    }
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRSQLiteTableLayer::ResetReading()

{
    ClearStatement();
    OGRSQLiteLayer::ResetReading();
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRSQLiteTableLayer::GetFeature( long nFeatureId )

{
/* -------------------------------------------------------------------- */
/*      If we don't have an explicit FID column, just read through      */
/*      the result set iteratively to find our target.                  */
/* -------------------------------------------------------------------- */
    if( pszFIDColumn == NULL )
        return OGRSQLiteLayer::GetFeature( nFeatureId );

/* -------------------------------------------------------------------- */
/*      Setup explicit query statement to fetch the record we want.     */
/* -------------------------------------------------------------------- */
    const char *pszSQL;
    int rc;

    ClearStatement();

    iNextShapeId = nFeatureId;

    pszSQL =
        CPLSPrintf( "SELECT _rowid_, * FROM %s WHERE %s = %d", 
                    poFeatureDefn->GetName(), 
                    pszFIDColumn, nFeatureId );

    rc = sqlite3_prepare( poDS->GetDB(), pszSQL, strlen(pszSQL), 
	    	          &hStmt, NULL );

/* -------------------------------------------------------------------- */
/*      Get the feature if possible.                                    */
/* -------------------------------------------------------------------- */
    OGRFeature *poFeature = NULL;

    if( rc == SQLITE_OK )
        poFeature = GetNextRawFeature();

    ClearStatement();

    return poFeature;
}

/************************************************************************/
/*                         SetAttributeFilter()                         */
/************************************************************************/

OGRErr OGRSQLiteTableLayer::SetAttributeFilter( const char *pszQuery )

{
    if( (pszQuery == NULL && this->pszQuery == NULL)
        || (pszQuery != NULL && this->pszQuery != NULL 
            && EQUAL(pszQuery,this->pszQuery)) )
        return OGRERR_NONE;

    CPLFree( this->pszQuery );
    this->pszQuery = CPLStrdup( pszQuery );

    ClearStatement();

    return OGRERR_NONE;
}


/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRSQLiteTableLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return bUpdateAccess;

    else if( EQUAL(pszCap,OLCCreateField) )
        return bUpdateAccess;

    else 
        return OGRSQLiteLayer::TestCapability( pszCap );
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/*                                                                      */
/*      If a spatial filter is in effect, we turn control over to       */
/*      the generic counter.  Otherwise we return the total count.      */
/*      Eventually we should consider implementing a more efficient     */
/*      way of counting features matching a spatial query.              */
/************************************************************************/

int OGRSQLiteTableLayer::GetFeatureCount( int bForce )

{
    if( poFilterGeom != NULL && pszGeomColumn != NULL )
        return OGRSQLiteLayer::GetFeatureCount( bForce );

/* -------------------------------------------------------------------- */
/*      Form count SQL.                                                 */
/* -------------------------------------------------------------------- */
    const char *pszSQL;

    if( pszQuery != NULL )
        pszSQL = CPLSPrintf( "SELECT count(*) FROM %s WHERE %s",
                             poFeatureDefn->GetName(), pszQuery );
    else
        pszSQL = CPLSPrintf( "SELECT count(*) FROM %s",
                             poFeatureDefn->GetName() );

/* -------------------------------------------------------------------- */
/*      Execute.                                                        */
/* -------------------------------------------------------------------- */
    char **papszResult, *pszErrMsg;
    int nRowCount, nColCount;
    int nResult = -1;

    if( sqlite3_get_table( poDS->GetDB(), pszSQL, &papszResult, 
                           &nColCount, &nRowCount, &pszErrMsg ) != SQLITE_OK )
        return -1;

    if( nRowCount == 1 && nColCount == 1 )
        nResult = atoi(papszResult[1]);

    sqlite3_free_table( papszResult );

    return nResult;
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/*                                                                      */
/*      We override this to try and fetch the table SRID from the       */
/*      geometry_columns table if the srsid is -2 (meaning we           */
/*      haven't yet even looked for it).                                */
/************************************************************************/

OGRSpatialReference *OGRSQLiteTableLayer::GetSpatialRef()

{
#ifdef notdef
    if( nSRSId == -2 )
    {
        PGconn          *hPGConn = poDS->GetPGConn();
        PGresult        *hResult;
        char            szCommand[1024];

        nSRSId = -1;

        poDS->SoftStartTransaction();

        sprintf( szCommand, 
                 "SELECT srid FROM geometry_columns "
                 "WHERE f_table_name = '%s'",
                 poFeatureDefn->GetName() );
        hResult = PQexec(hPGConn, szCommand );

        if( hResult 
            && PQresultStatus(hResult) == PGRES_TUPLES_OK 
            && PQntuples(hResult) == 1 )
        {
            nSRSId = atoi(PQgetvalue(hResult,0,0));
        }

        poDS->SoftCommit();
    }
#endif

    return OGRSQLiteLayer::GetSpatialRef();
}
