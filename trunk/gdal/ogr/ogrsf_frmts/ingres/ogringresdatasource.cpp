/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRIngresDataSource class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2008, Frank Warmerdam <warmerdam@pobox.com>
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


#include "ogr_ingres.h"

#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");
/************************************************************************/
/*                         OGRIngresDataSource()                        */
/************************************************************************/

OGRIngresDataSource::OGRIngresDataSource()

{
    pszName = NULL;
    papoLayers = NULL;
    nLayers = 0;
    hConn = 0;

    nKnownSRID = 0;
    panSRID = NULL;
    papoSRS = NULL;
}

/************************************************************************/
/*                        ~OGRIngresDataSource()                        */
/************************************************************************/

OGRIngresDataSource::~OGRIngresDataSource()

{
    int         i;

    CPLFree( pszName );

    for( i = 0; i < nLayers; i++ )
        delete papoLayers[i];
    
    CPLFree( papoLayers );

#ifdef notdef
    if( hConn != NULL )
        ingres_close( hConn );
#endif

    for( i = 0; i < nKnownSRID; i++ )
    {
        if( papoSRS[i] != NULL )
            papoSRS[i]->Release();
    }
    CPLFree( panSRID );
    CPLFree( papoSRS );
}

/************************************************************************/
/*                            ReportError()                             */
/************************************************************************/

void OGRIngresDataSource::ReportError( const char *pszDescription )

{
#ifdef notdef
    if( pszDescription )
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Ingres error message:%s Description: %s", 
                  ingres_error( hConn ), 
                  pszDescription );
    else
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s", ingres_error( hConn ) );
#endif
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

int OGRIngresDataSource::Open( const char * pszNewName, int bUpdate )


{
    CPLAssert( nLayers == 0 );

/* -------------------------------------------------------------------- */
/*      Verify Ingres prefix.                                            */
/* -------------------------------------------------------------------- */
    if( !EQUALN(pszNewName,"INGRES:",7) )
    {
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      For now we assume the whole argument is the "dbname".           */
/* -------------------------------------------------------------------- */
    const char *pszDBName = pszNewName + 7;
    char **papszTableNames = NULL;

/* -------------------------------------------------------------------- */
/*      Initialize the Ingres API. Should we only do this once per      */
/*      program run?  Really we should also try to terminate the api    */
/*      on program exit.                                                */
/* -------------------------------------------------------------------- */
    IIAPI_INITPARM  initParm;

    initParm.in_version = IIAPI_VERSION_1; 
    initParm.in_timeout = -1;
    IIapi_initialize( &initParm );

/* -------------------------------------------------------------------- */
/*      Try to connect to the database.                                 */
/* -------------------------------------------------------------------- */
    IIAPI_CONNPARM	connParm;
    IIAPI_WAITPARM	waitParm = { -1 };
    
    connParm.co_genParm.gp_callback = NULL;
    connParm.co_genParm.gp_closure = NULL;
    connParm.co_target = (II_CHAR *) pszDBName;
    connParm.co_connHandle = NULL;
    connParm.co_tranHandle = NULL;
    connParm.co_username = NULL;
    connParm.co_password = NULL;
    connParm.co_timeout = -1;

    IIapi_connect( &connParm );
    
    while( connParm.co_genParm.gp_completed == FALSE )
	IIapi_wait( &waitParm );

    hConn = connParm.co_connHandle;

    if( hConn == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Attempt to connect to Ingres database '%s' failed.",
                  pszDBName );
        return FALSE;
    }

    pszName = CPLStrdup( pszNewName );
    
    bDSUpdate = bUpdate;

/* -------------------------------------------------------------------- */
/*      Get a list of available tables.                                 */
/* -------------------------------------------------------------------- */
    if( papszTableNames == NULL )
    {
        OGRIngresStatement oStmt( hConn );
        
        if( oStmt.ExecuteSQL( "select table_name from iitables where system_use = 'U'" ) )
        {
            char **papszFields;
            while( (papszFields = oStmt.GetRow()) )
            {
                CPLString osTableName = papszFields[0];
                osTableName.Trim();
                papszTableNames = CSLAddString( papszTableNames, 
                                                osTableName );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Get the schema of the available tables.                         */
/* -------------------------------------------------------------------- */
    int iRecord;

    for( iRecord = 0; 
         papszTableNames != NULL && papszTableNames[iRecord] != NULL;
         iRecord++ )
    {
        OpenTable( papszTableNames[iRecord], bUpdate );
    }

    CSLDestroy( papszTableNames );

    return TRUE;
}

/************************************************************************/
/*                             OpenTable()                              */
/************************************************************************/

int OGRIngresDataSource::OpenTable( const char *pszNewName, int bUpdate )

{
/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    OGRIngresTableLayer  *poLayer;
    OGRErr eErr;

    poLayer = new OGRIngresTableLayer( this, pszNewName, bUpdate );
    eErr = poLayer->Initialize(pszNewName);
    if (eErr == OGRERR_FAILURE)
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGRIngresLayer **)
        CPLRealloc( papoLayers,  sizeof(OGRIngresLayer *) * (nLayers+1) );
    papoLayers[nLayers++] = poLayer;

    return TRUE;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRIngresDataSource::TestCapability( const char * pszCap )

{
	
    if( EQUAL(pszCap, ODsCCreateLayer) )
        return TRUE;
	if( EQUAL(pszCap, ODsCDeleteLayer))
		return TRUE;
    else
        return FALSE;
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/

OGRLayer *OGRIngresDataSource::GetLayer( int iLayer )

{
    if( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];
}


/************************************************************************/
/*                      InitializeMetadataTables()                      */
/*                                                                      */
/*      Create the metadata tables (SPATIAL_REF_SYS and                 */
/*      GEOMETRY_COLUMNS). This method "does no harm" if the tables     */
/*      exist and can be called at will.                                */
/************************************************************************/

OGRErr OGRIngresDataSource::InitializeMetadataTables()

{
#ifdef notdef
    char            szCommand[1024];
    INGRES_RES       *hResult;
    OGRErr	    eErr = OGRERR_NONE;
 
    sprintf( szCommand, "DESCRIBE geometry_columns" );
    if( ingres_query(GetConn(), szCommand ) )
    {
        sprintf(szCommand,
                "CREATE TABLE geometry_columns "
                "( F_TABLE_CATALOG VARCHAR(256), "
                "F_TABLE_SCHEMA VARCHAR(256), "
                "F_TABLE_NAME VARCHAR(256) NOT NULL," 
                "F_GEOMETRY_COLUMN VARCHAR(256) NOT NULL, "
                "COORD_DIMENSION INT, "
                "SRID INT,"
                "TYPE VARCHAR(256) NOT NULL)");
        if( ingres_query(GetConn(), szCommand ) )
        {
            ReportError( szCommand );
            eErr = OGRERR_FAILURE;
        }
        else
            CPLDebug("INGRES","Creating geometry_columns metadata table");
 
    }
 
    // make sure to attempt to free results of successful queries
    hResult = ingres_store_result( GetConn() );
    if( hResult != NULL )
    {
        ingres_free_result( hResult );
        hResult = NULL;   
    }
 
    sprintf( szCommand, "DESCRIBE spatial_ref_sys" );
    if( ingres_query(GetConn(), szCommand ) )
    {
        sprintf(szCommand,
                "CREATE TABLE spatial_ref_sys "
                "(SRID INT NOT NULL, "
                "AUTH_NAME VARCHAR(256), "
                "AUTH_SRID INT, "
                "SRTEXT VARCHAR(2048))");
        if( ingres_query(GetConn(), szCommand ) )
        {
            ReportError( szCommand );
            eErr = OGRERR_FAILURE;
        }
        else
            CPLDebug("INGRES","Creating spatial_ref_sys metadata table");
 
    }    
 
    // make sure to attempt to free results of successful queries
    hResult = ingres_store_result( GetConn() );
    if( hResult != NULL )
    {
        ingres_free_result( hResult );
        hResult = NULL;
    }
 
    return eErr;
#endif
    return OGRERR_NONE;
}

/************************************************************************/
/*                              FetchSRS()                              */
/*                                                                      */
/*      Return a SRS corresponding to a particular id.  Note that       */
/*      reference counting should be honoured on the returned           */
/*      OGRSpatialReference, as handles may be cached.                  */
/************************************************************************/

OGRSpatialReference *OGRIngresDataSource::FetchSRS( int nId )
{
#ifdef notdef
    char         szCommand[1024];
    char           **papszRow;  
    INGRES_RES       *hResult;
            
    if( nId < 0 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      First, we look through our SRID cache, is it there?             */
/* -------------------------------------------------------------------- */
    int  i;

    for( i = 0; i < nKnownSRID; i++ )
    {
        if( panSRID[i] == nId )
            return papoSRS[i];
    }

    OGRSpatialReference *poSRS = NULL;
 
    // make sure to attempt to free any old results
    hResult = ingres_store_result( GetConn() );
    if( hResult != NULL )
        ingres_free_result( hResult );
    hResult = NULL;   
                        
    sprintf( szCommand,
         "SELECT srtext FROM spatial_ref_sys WHERE srid = %d",
         nId );
    
    if( !ingres_query( GetConn(), szCommand ) )
        hResult = ingres_store_result( GetConn() );
        
    char  *pszWKT = NULL;
    papszRow = NULL;
    

    if( hResult != NULL )
        papszRow = ingres_fetch_row( hResult );

    if( papszRow != NULL && papszRow[0] != NULL )
    {
        pszWKT =papszRow[0];
    }

    // make sure to attempt to free results of successful queries
    hResult = ingres_store_result( GetConn() );
    if( hResult != NULL )
        ingres_free_result( hResult );
	hResult = NULL;
		
     poSRS = new OGRSpatialReference();
     if( pszWKT == NULL || poSRS->importFromWkt( &pszWKT ) != OGRERR_NONE )
     {
         delete poSRS;
         poSRS = NULL;
     }

/* -------------------------------------------------------------------- */
/*      Add to the cache.                                               */
/* -------------------------------------------------------------------- */
    panSRID = (int *) CPLRealloc(panSRID,sizeof(int) * (nKnownSRID+1) );
    papoSRS = (OGRSpatialReference **) 
        CPLRealloc(papoSRS, sizeof(void*) * (nKnownSRID + 1) );
    panSRID[nKnownSRID] = nId;
    papoSRS[nKnownSRID] = poSRS;

    return poSRS;
#endif
    return NULL;
}



/************************************************************************/
/*                             FetchSRSId()                             */
/*                                                                      */
/*      Fetch the id corresponding to an SRS, and if not found, add     */
/*      it to the table.                                                */
/************************************************************************/

int OGRIngresDataSource::FetchSRSId( OGRSpatialReference * poSRS )

{
#ifdef notdef
    char           **papszRow;  
    INGRES_RES       *hResult=NULL;
    
    char                szCommand[10000];
    char                *pszWKT = NULL;
    int                 nSRSId;

    if( poSRS == NULL )
        return -1;

/* -------------------------------------------------------------------- */
/*      Translate SRS to WKT.                                           */
/* -------------------------------------------------------------------- */
    if( poSRS->exportToWkt( &pszWKT ) != OGRERR_NONE )
        return -1;
    
    CPLAssert( strlen(pszWKT) < sizeof(szCommand) - 500 );

/* -------------------------------------------------------------------- */
/*      Try to find in the existing table.                              */
/* -------------------------------------------------------------------- */
    sprintf( szCommand, 
             "SELECT srid FROM spatial_ref_sys WHERE srtext = '%s'",
             pszWKT );

    if( !ingres_query( GetConn(), szCommand ) )
        hResult = ingres_store_result( GetConn() );

    if (!ingres_num_rows(hResult))
    {
        CPLDebug("INGRES", "No rows exist currently exist in spatial_ref_sys");
        ingres_free_result( hResult );
        hResult = NULL;
    }
    papszRow = NULL;
    if( hResult != NULL )
        papszRow = ingres_fetch_row( hResult );
        
    if( papszRow != NULL && papszRow[0] != NULL )
    {
        nSRSId = atoi(papszRow[0]);
        if( hResult != NULL )
            ingres_free_result( hResult );
        hResult = NULL;
        return nSRSId;
    }

    // make sure to attempt to free results of successful queries
    hResult = ingres_store_result( GetConn() );
    if( hResult != NULL )
        ingres_free_result( hResult );
    hResult = NULL;

/* -------------------------------------------------------------------- */
/*      Get the current maximum srid in the srs table.                  */
/* -------------------------------------------------------------------- */
    sprintf( szCommand, 
             "SELECT MAX(srid) FROM spatial_ref_sys");    
    if( !ingres_query( GetConn(), szCommand ) )
    {
        hResult = ingres_store_result( GetConn() );
        papszRow = ingres_fetch_row( hResult );
    }
        
    if( papszRow != NULL && papszRow[0] != NULL )
    {
        nSRSId = atoi(papszRow[0]) + 1;
        if( hResult != NULL )
            ingres_free_result( hResult );
        hResult = NULL;
    }
    else
        nSRSId = 1;
    
/* -------------------------------------------------------------------- */
/*      Try adding the SRS to the SRS table.                            */
/* -------------------------------------------------------------------- */
    sprintf( szCommand, 
             "INSERT INTO spatial_ref_sys (srid,srtext) VALUES (%d,'%s')",
             nSRSId, pszWKT );

    if( !ingres_query( GetConn(), szCommand ) )
        hResult = ingres_store_result( GetConn() );

    // make sure to attempt to free results of successful queries
    hResult = ingres_store_result( GetConn() );
    if( hResult != NULL )
        ingres_free_result( hResult );
    hResult = NULL;
           
    return nSRSId;
#endif
    return -1;
}

/************************************************************************/
/*                             ExecuteSQL()                             */
/************************************************************************/

OGRLayer * OGRIngresDataSource::ExecuteSQL( const char *pszSQLCommand,
                                        OGRGeometry *poSpatialFilter,
                                        const char *pszDialect )

{
#ifdef notdef
    if( poSpatialFilter != NULL )
    {
        CPLDebug( "OGR_INGRES", 
          "Spatial filter ignored for now in OGRIngresDataSource::ExecuteSQL()" );
    }

/* -------------------------------------------------------------------- */
/*      Use generic implementation for OGRSQL dialect.                  */
/* -------------------------------------------------------------------- */
    if( pszDialect != NULL && EQUAL(pszDialect,"OGRSQL") )
        return OGRDataSource::ExecuteSQL( pszSQLCommand, 
                                          poSpatialFilter, 
                                          pszDialect );

/* -------------------------------------------------------------------- */
/*      Special case DELLAYER: command.                                 */
/* -------------------------------------------------------------------- */
#ifdef notdef
    if( EQUALN(pszSQLCommand,"DELLAYER:",9) )
    {
        const char *pszLayerName = pszSQLCommand + 9;

        while( *pszLayerName == ' ' )
            pszLayerName++;

        DeleteLayer( pszLayerName );
        return NULL;
    }
#endif

/* -------------------------------------------------------------------- */
/*      Make sure there isn't an active transaction already.            */
/* -------------------------------------------------------------------- */
    InterruptLongResult();

/* -------------------------------------------------------------------- */
/*      Execute the statement.                                          */
/* -------------------------------------------------------------------- */
    INGRES_RES *hResultSet;

    if( ingres_query( hConn, pszSQLCommand ) )
    {
        ReportError( pszSQLCommand );
        return NULL;
    }

    hResultSet = ingres_use_result( hConn );
    if( hResultSet == NULL )
    {
        if( ingres_field_count( hConn ) == 0 )
        {
            CPLDebug( "INGRES", "Command '%s' succeeded, %d rows affected.", 
                      pszSQLCommand, 
                      (int) ingres_affected_rows(hConn) );
            return NULL;
        }
        else
        {
            ReportError( pszSQLCommand );
            return NULL;
        }
    }

/* -------------------------------------------------------------------- */
/*      Do we have a tuple result? If so, instantiate a results         */
/*      layer for it.                                                   */
/* -------------------------------------------------------------------- */

    OGRIngresResultLayer *poLayer = NULL;

    poLayer = new OGRIngresResultLayer( this, pszSQLCommand, hResultSet );
        
    return poLayer;
#endif
    return NULL;
}

/************************************************************************/
/*                          ReleaseResultSet()                          */
/************************************************************************/

void OGRIngresDataSource::ReleaseResultSet( OGRLayer * poLayer )

{
    delete poLayer;
}

/************************************************************************/
/*                            LaunderName()                             */
/************************************************************************/

char *OGRIngresDataSource::LaunderName( const char *pszSrcName )

{
    char    *pszSafeName = CPLStrdup( pszSrcName );
    int     i;

    for( i = 0; pszSafeName[i] != '\0'; i++ )
    {
        pszSafeName[i] = (char) tolower( pszSafeName[i] );
        if( pszSafeName[i] == '-' || pszSafeName[i] == '#' )
            pszSafeName[i] = '_';
    }

    return pszSafeName;
}

/************************************************************************/
/*                            DeleteLayer()                             */
/************************************************************************/

int OGRIngresDataSource::DeleteLayer( int iLayer)

{
#ifdef notdef
    if( iLayer < 0 || iLayer >= nLayers )
        return OGRERR_FAILURE;
        
/* -------------------------------------------------------------------- */
/*      Blow away our OGR structures related to the layer.  This is     */
/*      pretty dangerous if anything has a reference to this layer!     */
/* -------------------------------------------------------------------- */
    CPLString osLayerName = papoLayers[iLayer]->GetLayerDefn()->GetName();
    
    CPLDebug( "INGRES", "DeleteLayer(%s)", osLayerName.c_str() );

    delete papoLayers[iLayer];
    memmove( papoLayers + iLayer, papoLayers + iLayer + 1,
             sizeof(void *) * (nLayers - iLayer - 1) );
    nLayers--;

/* -------------------------------------------------------------------- */
/*      Remove from the database.                                       */
/* -------------------------------------------------------------------- */
    char        		szCommand[1024];

    sprintf( szCommand,
             "DROP TABLE %s ",
             osLayerName.c_str() );

    if( !ingres_query(GetConn(), szCommand ) )
    {
        CPLDebug("INGRES","Dropped table %s.", osLayerName.c_str());
        return OGRERR_NONE;
    }
    else
    {
        ReportError( szCommand );
        return OGRERR_FAILURE;
    }
#endif
    return OGRERR_FAILURE;
}

/************************************************************************/
/*                            CreateLayer()                             */
/************************************************************************/

OGRLayer *
OGRIngresDataSource::CreateLayer( const char * pszLayerNameIn,
                              OGRSpatialReference *poSRS,
                              OGRwkbGeometryType eType,
                              char ** papszOptions )

{
#ifdef notdef

    INGRES_RES           *hResult=NULL;
    char        		szCommand[1024];
    const char          *pszGeometryType;
    const char			*pszGeomColumnName;
    const char 			*pszExpectedFIDName; 
	
    char                *pszLayerName;
    int                 nDimension = 3; // Ingres only supports 2d currently


    if( CSLFetchBoolean(papszOptions,"LAUNDER",TRUE) )
        pszLayerName = LaunderName( pszLayerNameIn );
    else
        pszLayerName = CPLStrdup( pszLayerNameIn );

    if( wkbFlatten(eType) == eType )
        nDimension = 2;

    CPLDebug("INGRES","Creating layer %s.", pszLayerName);

/* -------------------------------------------------------------------- */
/*      Do we already have this layer?  If so, should we blow it        */
/*      away?                                                           */
/* -------------------------------------------------------------------- */

    int iLayer;
    for( iLayer = 0; iLayer < nLayers; iLayer++ )
    {
        if( EQUAL(pszLayerName,papoLayers[iLayer]->GetLayerDefn()->GetName()) )
        {
			
            if( CSLFetchNameValue( papszOptions, "OVERWRITE" ) != NULL
                && !EQUAL(CSLFetchNameValue(papszOptions,"OVERWRITE"),"NO") )
            {
                DeleteLayer( iLayer );
            }
            else
            {
                CPLFree( pszLayerName );
                CPLError( CE_Failure, CPLE_AppDefined,
                          "Layer %s already exists, CreateLayer failed.\n"
                          "Use the layer creation option OVERWRITE=YES to "
                          "replace it.",
                          pszLayerName );
                return NULL;
            }
        }
    }

    pszGeomColumnName = CSLFetchNameValue( papszOptions, "GEOMETRY_NAME" );
    if (!pszGeomColumnName)
        pszGeomColumnName="SHAPE";

    pszExpectedFIDName = CSLFetchNameValue( papszOptions, "INGRES_FID" );
    if (!pszExpectedFIDName)
        pszExpectedFIDName="OGR_FID";


    CPLDebug("INGRES","Geometry Column Name %s.", pszGeomColumnName);
    CPLDebug("INGRES","FID Column Name %s.", pszExpectedFIDName);

    if( wkbFlatten(eType) == wkbNone )
    {
        sprintf( szCommand,
                 "CREATE TABLE %s ( "
                 "   %s INT UNIQUE NOT NULL AUTO_INCREMENT )",
                 pszLayerName, pszExpectedFIDName );
    }
    else
    {
        sprintf( szCommand,
                 "CREATE TABLE %s ( "
                 "   %s INT UNIQUE NOT NULL AUTO_INCREMENT, "
                 "   %s GEOMETRY NOT NULL )",
                 pszLayerName, pszExpectedFIDName, pszGeomColumnName );
    }

    if( CSLFetchNameValue( papszOptions, "ENGINE" ) != NULL )
    {
        strcat( szCommand, " ENGINE = " );
        strcat( szCommand, CSLFetchNameValue( papszOptions, "ENGINE" ) );
    }
	
    if( !ingres_query(GetConn(), szCommand ) )
    {
        if( ingres_field_count( GetConn() ) == 0 )
            CPLDebug("INGRES","Created table %s.", pszLayerName);
        else
        {
            ReportError( szCommand );
            return NULL;
        }
    }
    else
    {
        ReportError( szCommand );
        return NULL;
    }

    // make sure to attempt to free results of successful queries
    hResult = ingres_store_result( GetConn() );
    if( hResult != NULL )
        ingres_free_result( hResult );
    hResult = NULL;
    
    // Calling this does no harm
    InitializeMetadataTables();
    
/* -------------------------------------------------------------------- */
/*      Try to get the SRS Id of this spatial reference system,         */
/*      adding tot the srs table if needed.                             */
/* -------------------------------------------------------------------- */
    int nSRSId = -1;

    if( poSRS != NULL )
        nSRSId = FetchSRSId( poSRS );

/* -------------------------------------------------------------------- */
/*      Sometimes there is an old crufty entry in the geometry_columns  */
/*      table if things were not properly cleaned up before.  We make   */
/*      an effort to clean out such cruft.                              */
/*                                                                      */
/* -------------------------------------------------------------------- */
    sprintf( szCommand,
             "DELETE FROM geometry_columns WHERE f_table_name = '%s'",
             pszLayerName );

    if( ingres_query(GetConn(), szCommand ) )
    {
        ReportError( szCommand );
        return NULL;
    }

    // make sure to attempt to free results of successful queries
    hResult = ingres_store_result( GetConn() );
    if( hResult != NULL )
        ingres_free_result( hResult );
    hResult = NULL;   
        
/* -------------------------------------------------------------------- */
/*      Attempt to add this table to the geometry_columns table, if     */
/*      it is a spatial layer.                                          */
/* -------------------------------------------------------------------- */
    if( eType != wkbNone )
    {
        int nCoordDimension;
        if( eType == wkbFlatten(eType) )
            nCoordDimension = 2;
        else
            nCoordDimension = 3;

        switch( wkbFlatten(eType) )
        {
            case wkbPoint:
                pszGeometryType = "POINT";
                break;

            case wkbLineString:
                pszGeometryType = "LINESTRING";
                break;

            case wkbPolygon:
                pszGeometryType = "POLYGON";
                break;

            case wkbMultiPoint:
                pszGeometryType = "MULTIPOINT";
                break;

            case wkbMultiLineString:
                pszGeometryType = "MULTILINESTRING";
                break;

            case wkbMultiPolygon:
                pszGeometryType = "MULTIPOLYGON";
                break;

            case wkbGeometryCollection:
                pszGeometryType = "GEOMETRYCOLLECTION";
                break;

            default:
                pszGeometryType = "GEOMETRY";
                break;

        }

        if( nSRSId == -1 )
            sprintf( szCommand,
                     "INSERT INTO geometry_columns "
                     " (F_TABLE_NAME, "
                     "  F_GEOMETRY_COLUMN, "
                     "  COORD_DIMENSION, "
                     "  TYPE) values "
                     "  ('%s', '%s', %d, '%s')",
                     pszLayerName,
                     pszGeomColumnName,
                     nCoordDimension,
                     pszGeometryType );
        else
            sprintf( szCommand,
                     "INSERT INTO geometry_columns "
                     " (F_TABLE_NAME, "
                     "  F_GEOMETRY_COLUMN, "
                     "  COORD_DIMENSION, "
                     "  SRID, "
                     "  TYPE) values "
                     "  ('%s', '%s', %d, %d, '%s')",
                     pszLayerName,
                     pszGeomColumnName,
                     nCoordDimension,
                     nSRSId,
                     pszGeometryType );

        if( ingres_query(GetConn(), szCommand ) )
        {
            ReportError( szCommand );
            return NULL;
        }

        // make sure to attempt to free results of successful queries
        hResult = ingres_store_result( GetConn() );
        if( hResult != NULL )
            ingres_free_result( hResult );
        hResult = NULL;   
    }

/* -------------------------------------------------------------------- */
/*      Create the spatial index.                                       */
/*                                                                      */
/*      We're doing this before we add geometry and record to the table */
/*      so this may not be exactly the best way to do it.               */
/* -------------------------------------------------------------------- */
    const char *pszSI = CSLFetchNameValue( papszOptions, "SPATIAL_INDEX" );

    if( eType != wkbNone && (pszSI == NULL || CSLTestBoolean(pszSI)) )
    {
        sprintf( szCommand,
                 "ALTER TABLE %s ADD SPATIAL INDEX(%s) ",
                 pszLayerName,
                 pszGeomColumnName);

        if( ingres_query(GetConn(), szCommand ) )
        {
            ReportError( szCommand );
            return NULL;
        }

        // make sure to attempt to free results of successful queries
        hResult = ingres_store_result( GetConn() );
        if( hResult != NULL )
            ingres_free_result( hResult );
        hResult = NULL;   
    }
        
/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    OGRIngresTableLayer     *poLayer;
    OGRErr                  eErr;

    poLayer = new OGRIngresTableLayer( this, pszLayerName, TRUE, nSRSId );
    eErr = poLayer->Initialize(pszLayerName);
    if (eErr == OGRERR_FAILURE)
        return NULL;

    poLayer->SetLaunderFlag( CSLFetchBoolean(papszOptions,"LAUNDER",TRUE) );
    poLayer->SetPrecisionFlag( CSLFetchBoolean(papszOptions,"PRECISION",TRUE));

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGRIngresLayer **)
        CPLRealloc( papoLayers,  sizeof(OGRIngresLayer *) * (nLayers+1) );

    papoLayers[nLayers++] = poLayer;

    CPLFree( pszLayerName );

    return poLayer;
#endif
    return NULL;
}
