/******************************************************************************
 * $Id$
 *
 * Project:  Oracle Spatial Driver
 * Purpose:  Implementation of the OGROCIDataSource class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.26  2005/02/10 15:46:02  fwarmerdam
 * added GEOMETRY_NAME layer creation option
 *
 * Revision 1.25  2004/11/22 19:24:15  fwarmerdam
 * added support for a list of tables in the datasource name
 *
 * Revision 1.24  2004/11/09 02:49:12  fwarmerdam
 * Don't add user_sdo_geom_metadata record till finalize (layer load complete)
 * time as that is when we have all the diminfo details.  Inserts of metadata
 * records with NULL diminfos fails in Oracle 10 due to new constraints.
 *
 * Revision 1.23  2004/07/09 07:07:39  warmerda
 * Added OGRSQL dialect support.
 *
 * Revision 1.22  2003/09/18 14:45:47  warmerda
 * return -1 from FetchSRSId() for SRS if not GEOGCS or PROJCS
 *
 * Revision 1.21  2003/09/13 04:52:38  warmerda
 * Added logic to mapping some EPSG codes to and from Oracle codes by table.
 * Added logic to recognise an Oracle authority in WKT as a direct mapping
 * request.  Return authority information from MDSYS.CS_SRS where available.
 *
 * Revision 1.20  2003/05/21 03:54:01  warmerda
 * expand tabs
 *
 * Revision 1.19  2003/04/22 19:37:50  warmerda
 * Added sync to disk
 *
 * Revision 1.18  2003/04/04 06:18:08  warmerda
 * first pass implementation of loader support
 *
 * Revision 1.17  2003/03/18 18:34:17  warmerda
 * added reason code 13349
 *
 * Revision 1.16  2003/02/06 21:14:43  warmerda
 * cleanup some memory leaks
 *
 * Revision 1.15  2003/01/15 06:10:04  warmerda
 * Added logic to hack angular unit name to "Decimal Degree".
 *
 * Revision 1.14  2003/01/15 05:35:32  warmerda
 * allow override of SRID, pass to table constructor
 *
 * Revision 1.13  2003/01/14 22:15:13  warmerda
 * added pseudo-sql commands DELLAYER and VALLAYER
 *
 * Revision 1.12  2003/01/14 15:09:44  warmerda
 * set layer creation options on OGROCITableLayer
 *
 * Revision 1.11  2003/01/13 13:49:43  warmerda
 * add multi-user table support (ALL_SDO_GEOM_METADATA)
 *
 * Revision 1.10  2003/01/10 22:31:06  warmerda
 * use OGROCISession::CleanName() to fixup names
 *
 * Revision 1.9  2003/01/10 19:30:14  warmerda
 * Another related initialization fix.
 *
 * Revision 1.8  2003/01/10 16:40:33  warmerda
 * initialize SRS table
 *
 * Revision 1.7  2003/01/07 22:24:35  warmerda
 * added SRS support
 *
 * Revision 1.6  2003/01/06 17:58:20  warmerda
 * Fix FID support, add DIM creation keyword
 *
 * Revision 1.5  2003/01/02 21:48:52  warmerda
 * uppercaseify layer names when creating tables
 *
 * Revision 1.4  2002/12/29 19:43:59  warmerda
 * avoid some warnings
 *
 * Revision 1.3  2002/12/29 03:19:48  warmerda
 * fixed extraction of database name
 *
 * Revision 1.2  2002/12/28 04:38:36  warmerda
 * converted to unix file conventions
 *
 * Revision 1.1  2002/12/28 04:07:27  warmerda
 * New
 *
 */

#include "ogr_oci.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

static int anEPSGOracleMapping[] = 
{
    /* Oracle SRID, EPSG GCS/PCS Code */
    
    8192, 4326, // WGS84
    8306, 4322, // WGS72
    8267, 4269, // NAD83
    8274, 4277, // OSGB 36
         // NAD27 isn't easily mapped since there are many Oracle NAD27 codes.

    81989, 27700, // UK National Grid

    0, 0 // end marker
};

/************************************************************************/
/*                          OGROCIDataSource()                          */
/************************************************************************/

OGROCIDataSource::OGROCIDataSource()

{
    pszName = NULL;
    pszDBName = NULL;
    papoLayers = NULL;
    nLayers = 0;
    poSession = NULL;
    papoSRS = NULL;
    panSRID = NULL;
    nKnownSRID = 0;
}

/************************************************************************/
/*                         ~OGROCIDataSource()                          */
/************************************************************************/

OGROCIDataSource::~OGROCIDataSource()

{
    int         i;

    CPLFree( pszName );
    CPLFree( pszDBName );

    for( i = 0; i < nLayers; i++ )
        delete papoLayers[i];
    
    CPLFree( papoLayers );

    for( i = 0; i < nKnownSRID; i++ )
    {
        delete papoSRS[i];
    }
    CPLFree( papoSRS );
    CPLFree( panSRID );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

int OGROCIDataSource::Open( const char * pszNewName, int bUpdate,
                            int bTestOpen )

{
    CPLAssert( nLayers == 0 && poSession == NULL );

/* -------------------------------------------------------------------- */
/*      Verify postgresql prefix.                                       */
/* -------------------------------------------------------------------- */
    if( !EQUALN(pszNewName,"OCI:",3) )
    {
        if( !bTestOpen )
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "%s does not conform to Oracle OCI driver naming convention,"
                      " OCI:*\n" );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Try to parse out name, password and database name.              */
/* -------------------------------------------------------------------- */
    char *pszUserid;
    const char *pszPassword = "";
    const char *pszDatabase = "";
    char **papszTableList = NULL;
    int   i;

    pszUserid = CPLStrdup( pszNewName + 4 );

    // Is there a table list? 
    for( i = strlen(pszUserid)-1; i > 1; i-- )
    {
        if( pszUserid[i] == ':' )
        {
            papszTableList = CSLTokenizeStringComplex( pszUserid+i+1, ",",
                                                       TRUE, FALSE );
            pszUserid[i] = '\0';
            break;
        }

        if( pszUserid[i] == '/' || pszUserid[i] == '@' )
            break;
    }

    for( i = 0; 
         pszUserid[i] != '\0' && pszUserid[i] != '/' && pszUserid[i] != '@';
         i++ ) {}

    if( pszUserid[i] == '/' )
    {
        pszUserid[i++] = '\0';
        pszPassword = pszUserid + i;
        for( ; pszUserid[i] != '\0' && pszUserid[i] != '@'; i++ ) {}
    }

    if( pszUserid[i] == '@' )
    {
        pszUserid[i++] = '\0';
        pszDatabase = pszUserid + i;
    }

/* -------------------------------------------------------------------- */
/*      Try to establish connection.                                    */
/* -------------------------------------------------------------------- */
    CPLDebug( "OCI", "Userid=%s, Password=%s, Database=%s", 
              pszUserid, pszPassword, pszDatabase );

    poSession = OGRGetOCISession( pszUserid, pszPassword, pszDatabase );
    if( poSession == NULL )
        return FALSE;

    pszName = CPLStrdup( pszNewName );
    
    bDSUpdate = bUpdate;

/* -------------------------------------------------------------------- */
/*      If no list of target tables was provided, collect a list of     */
/*      spatial tables now.                                             */
/* -------------------------------------------------------------------- */
    if( papszTableList == NULL )
    {
        OGROCIStatement oGetTables( poSession );

        if( oGetTables.Execute( 
            "SELECT TABLE_NAME, OWNER FROM ALL_SDO_GEOM_METADATA" ) 
            == CE_None )
        {
            char **papszRow;

            while( (papszRow = oGetTables.SimpleFetchRow()) != NULL )
            {
                char szFullTableName[100];

                if( EQUAL(papszRow[1],pszUserid) )
                    strcpy( szFullTableName, papszRow[0] );
                else
                    sprintf( szFullTableName, "%s.%s", 
                             papszRow[1], papszRow[0] );

                if( CSLFindString( papszTableList, szFullTableName ) == -1 )
                    papszTableList = CSLAddString( papszTableList, 
                                                   szFullTableName );
            }
        }
    }
    CPLFree( pszUserid );

/* -------------------------------------------------------------------- */
/*      Open all the selected tables or views.                          */
/* -------------------------------------------------------------------- */
    for( i = 0; papszTableList != NULL && papszTableList[i] != NULL; i++ )
    {
        OpenTable( papszTableList[i], -1, bUpdate, FALSE );
    }

    return TRUE;
}

/************************************************************************/
/*                             OpenTable()                              */
/************************************************************************/

int OGROCIDataSource::OpenTable( const char *pszNewName, 
                                 int nSRID, int bUpdate, int bTestOpen )

{
/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    OGROCITableLayer    *poLayer;

    poLayer = new OGROCITableLayer( this, pszNewName, nSRID, 
                                    bUpdate, FALSE );

    if( !poLayer->IsValid() )
    {
        delete poLayer;
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGROCILayer **)
        CPLRealloc( papoLayers,  sizeof(OGROCILayer *) * (nLayers+1) );
    papoLayers[nLayers++] = poLayer;

    return TRUE;
}

/************************************************************************/
/*                           ValidateLayer()                            */
/************************************************************************/

void OGROCIDataSource::ValidateLayer( const char *pszLayerName )

{
    int iLayer;

/* -------------------------------------------------------------------- */
/*      Try to find layer.                                              */
/* -------------------------------------------------------------------- */
    for( iLayer = 0; iLayer < nLayers; iLayer++ )
    {
        if( EQUAL(pszLayerName,papoLayers[iLayer]->GetLayerDefn()->GetName()) )
            break;
    }

    if( iLayer == nLayers )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "ValidateLayer(): %s is not a recognised layer.", 
                  pszLayerName );
        return;
    }

/* -------------------------------------------------------------------- */
/*      Verify we have an FID and geometry column for this table.       */
/* -------------------------------------------------------------------- */
    OGROCITableLayer *poLayer = (OGROCITableLayer *) papoLayers[iLayer];

    if( poLayer->GetFIDName() == NULL || poLayer->GetGeomName() == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "ValidateLayer(): %s lacks a geometry or fid column.", 
                  pszLayerName );

        return;
    }

/* -------------------------------------------------------------------- */
/*      Prepare and execute the geometry validation.                    */
/* -------------------------------------------------------------------- */
    OGROCIStringBuf oValidateCmd;
    OGROCIStatement oValidateStmt( GetSession() );

    oValidateCmd.Append( "SELECT c." );
    oValidateCmd.Append( poLayer->GetFIDName() );
    oValidateCmd.Append( ", SDO_GEOM.VALIDATE_GEOMETRY(c." );
    oValidateCmd.Append( poLayer->GetGeomName() );
    oValidateCmd.Append( ", m.diminfo) from " );
    oValidateCmd.Append( poLayer->GetLayerDefn()->GetName() );
    oValidateCmd.Append( " c, user_sdo_geom_metadata m WHERE m.table_name= '");
    oValidateCmd.Append( poLayer->GetLayerDefn()->GetName() );
    oValidateCmd.Append( "' AND m.column_name = '" );
    oValidateCmd.Append( poLayer->GetGeomName() );
    oValidateCmd.Append( "' AND SDO_GEOM.VALIDATE_GEOMETRY(c." );
    oValidateCmd.Append( poLayer->GetGeomName() );
    oValidateCmd.Append( ", m.diminfo ) <> 'TRUE'" );

    oValidateStmt.Execute( oValidateCmd.GetString() );

/* -------------------------------------------------------------------- */
/*      Report results to debug stream.                                 */
/* -------------------------------------------------------------------- */
    char **papszRow;

    while( (papszRow = oValidateStmt.SimpleFetchRow()) != NULL )
    {
        const char *pszReason = papszRow[1];

        if( EQUAL(pszReason,"13011") )
            pszReason = "13011: value is out of range";
        else if( EQUAL(pszReason,"13050") )
            pszReason = "13050: unable to construct spatial object";
        else if( EQUAL(pszReason,"13349") )
            pszReason = "13349: polygon boundary crosses itself";

        CPLDebug( "OCI", "Validation failure for FID=%s: %s", 
                  papszRow[0], pszReason );
    }
}

/************************************************************************/
/*                            DeleteLayer()                             */
/************************************************************************/

void OGROCIDataSource::DeleteLayer( const char *pszLayerName )

{
    int iLayer;

/* -------------------------------------------------------------------- */
/*      Try to find layer.                                              */
/* -------------------------------------------------------------------- */
    for( iLayer = 0; iLayer < nLayers; iLayer++ )
    {
        if( EQUAL(pszLayerName,papoLayers[iLayer]->GetLayerDefn()->GetName()) )
        {
            pszLayerName = CPLStrdup(papoLayers[iLayer]->GetLayerDefn()->GetName());
            break;
        }
    }

    if( iLayer == nLayers )
        return;

/* -------------------------------------------------------------------- */
/*      Blow away our OGR structures related to the layer.  This is     */
/*      pretty dangerous if anything has a reference to this layer!     */
/* -------------------------------------------------------------------- */
    CPLDebug( "OCI", "DeleteLayer(%s)", pszLayerName );

    delete papoLayers[iLayer];
    memmove( papoLayers + iLayer, papoLayers + iLayer + 1, 
             sizeof(void *) * (nLayers - iLayer - 1) );
    nLayers--;

/* -------------------------------------------------------------------- */
/*      Remove from the database.                                       */
/* -------------------------------------------------------------------- */
    OGROCIStatement oCommand( poSession );
    char            szCommand[1024];

    sprintf( szCommand, "DROP TABLE \"%s\"", pszLayerName );
    oCommand.Execute( szCommand );

    sprintf( szCommand, 
             "DELETE FROM USER_SDO_GEOM_METADATA WHERE TABLE_NAME = '%s'", 
             pszLayerName );
    oCommand.Execute( szCommand );

    CPLFree( (char *) pszLayerName );
}

/************************************************************************/
/*                            CreateLayer()                             */
/************************************************************************/

OGRLayer *
OGROCIDataSource::CreateLayer( const char * pszLayerName,
                               OGRSpatialReference *poSRS,
                               OGRwkbGeometryType eType,
                               char ** papszOptions )

{
    char                szCommand[1024];
    char               *pszSafeLayerName = CPLStrdup(pszLayerName);

    poSession->CleanName( pszSafeLayerName );

/* -------------------------------------------------------------------- */
/*      Do we already have this layer?  If so, should we blow it        */
/*      away?                                                           */
/* -------------------------------------------------------------------- */
    int iLayer;

    for( iLayer = 0; iLayer < nLayers; iLayer++ )
    {
        if( EQUAL(pszSafeLayerName,
                  papoLayers[iLayer]->GetLayerDefn()->GetName()) )
        {
            if( CSLFetchNameValue( papszOptions, "OVERWRITE" ) != NULL
                && !EQUAL(CSLFetchNameValue(papszOptions,"OVERWRITE"),"NO") )
            {
                DeleteLayer( pszSafeLayerName );
            }
            else
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "Layer %s already exists, CreateLayer failed.\n"
                          "Use the layer creation option OVERWRITE=YES to "
                          "replace it.",
                          pszSafeLayerName );
                CPLFree( pszSafeLayerName );
                return NULL;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Try to get the SRS Id of this spatial reference system,         */
/*      adding tot the srs table if needed.                             */
/* -------------------------------------------------------------------- */
    char szSRSId[100];

    if( CSLFetchNameValue( papszOptions, "SRID" ) != NULL )
        strcpy( szSRSId, CSLFetchNameValue( papszOptions, "SRID" ) );     
    else if( poSRS != NULL )
        sprintf( szSRSId, "%d", FetchSRSId( poSRS ) );
    else
        strcpy( szSRSId, "NULL" );

/* -------------------------------------------------------------------- */
/*      Determine name of geometry column to use.                       */
/* -------------------------------------------------------------------- */
    const char *pszGeometryName = 
        CSLFetchNameValue( papszOptions, "GEOMETRY_NAME" );
    if( pszGeometryName == NULL )
        pszGeometryName = "ORA_GEOMETRY";

/* -------------------------------------------------------------------- */
/*      Create a basic table with the FID.  Also include the            */
/*      geometry if this is not a PostGIS enabled table.                */
/* -------------------------------------------------------------------- */
   
    OGROCIStatement oStatement( poSession );
    
    sprintf( szCommand, 
             "CREATE TABLE \"%s\" ( "
             "OGR_FID INTEGER, "
             "%s %s )",
             pszSafeLayerName, pszGeometryName, SDO_GEOMETRY );

    if( oStatement.Execute( szCommand ) != CE_None )
    {
        CPLFree( pszSafeLayerName );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    const char *pszLoaderFile = CSLFetchNameValue(papszOptions,"LOADER_FILE");
    OGROCIWritableLayer *poLayer;

    if( pszLoaderFile == NULL )
        poLayer = new OGROCITableLayer( this, pszSafeLayerName, 
                                        EQUAL(szSRSId,"NULL") ? -1 : atoi(szSRSId),
                                        TRUE, TRUE );
    else
        poLayer = 
            new OGROCILoaderLayer( this, pszSafeLayerName, 
                                   pszGeometryName,
                                   EQUAL(szSRSId,"NULL") ? -1 : atoi(szSRSId),
                                   pszLoaderFile );

/* -------------------------------------------------------------------- */
/*      Set various options on the layer.                               */
/* -------------------------------------------------------------------- */
    poLayer->SetLaunderFlag( CSLFetchBoolean(papszOptions,"LAUNDER",FALSE) );
    poLayer->SetPrecisionFlag( CSLFetchBoolean(papszOptions,"PRECISION",TRUE));

    if( CSLFetchNameValue(papszOptions,"DIM") != NULL )
        poLayer->SetDimension( atoi(CSLFetchNameValue(papszOptions,"DIM")) );

    poLayer->SetOptions( papszOptions );

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGROCILayer **)
        CPLRealloc( papoLayers,  sizeof(OGROCILayer *) * (nLayers+1) );
    
    papoLayers[nLayers++] = poLayer;

    CPLFree( pszSafeLayerName );

    return poLayer;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGROCIDataSource::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,ODsCCreateLayer) && bDSUpdate )
        return TRUE;
    else
        return FALSE;
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/

OGRLayer *OGROCIDataSource::GetLayer( int iLayer )

{
    if( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];
}



/************************************************************************/
/*                             ExecuteSQL()                             */
/************************************************************************/

OGRLayer * OGROCIDataSource::ExecuteSQL( const char *pszSQLCommand,
                                        OGRGeometry *poSpatialFilter,
                                        const char *pszDialect )

{
/* -------------------------------------------------------------------- */
/*      Use generic implementation for OGRSQL dialect.                  */
/* -------------------------------------------------------------------- */
    if( pszDialect != NULL && EQUAL(pszDialect,"OGRSQL") )
        return OGRDataSource::ExecuteSQL( pszSQLCommand, 
                                          poSpatialFilter, 
                                          pszDialect );

/* -------------------------------------------------------------------- */
/*      Ensure any pending stuff is flushed to the database.            */
/* -------------------------------------------------------------------- */
    SyncToDisk();

/* -------------------------------------------------------------------- */
/*      Special case DELLAYER: command.                                 */
/* -------------------------------------------------------------------- */
    if( EQUALN(pszSQLCommand,"DELLAYER:",9) )
    {
        const char *pszLayerName = pszSQLCommand + 9;

        while( *pszLayerName == ' ' )
            pszLayerName++;

        DeleteLayer( pszLayerName );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Special case VALLAYER: command.                                 */
/* -------------------------------------------------------------------- */
    if( EQUALN(pszSQLCommand,"VALLAYER:",9) )
    {
        const char *pszLayerName = pszSQLCommand + 9;

        while( *pszLayerName == ' ' )
            pszLayerName++;

        ValidateLayer( pszLayerName );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Just execute simple command.                                    */
/* -------------------------------------------------------------------- */
    if( !EQUALN(pszSQLCommand,"SELECT",6) )
    {
        OGROCIStatement oCommand( poSession );

        oCommand.Execute( pszSQLCommand, OCI_COMMIT_ON_SUCCESS );

        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise instantiate a layer.                                  */
/* -------------------------------------------------------------------- */
    else
    {
        OGROCIStatement oCommand( poSession );
        
        if( oCommand.Execute( pszSQLCommand, OCI_DESCRIBE_ONLY ) == CE_None )
            return new OGROCISelectLayer( this, pszSQLCommand, &oCommand );
        else
            return NULL;
    }
}

/************************************************************************/
/*                          ReleaseResultSet()                          */
/************************************************************************/

void OGROCIDataSource::ReleaseResultSet( OGRLayer * poLayer )

{
    delete poLayer;
}

/************************************************************************/
/*                              FetchSRS()                              */
/*                                                                      */
/*      Return a SRS corresponding to a particular id.  Note that       */
/*      reference counting should be honoured on the returned           */
/*      OGRSpatialReference, as handles may be cached.                  */
/************************************************************************/

OGRSpatialReference *OGROCIDataSource::FetchSRS( int nId )

{
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

/* -------------------------------------------------------------------- */
/*      Try looking up in MDSYS.CS_SRS table.                           */
/* -------------------------------------------------------------------- */
    OGROCIStatement oStatement( GetSession() );
    char            szSelect[200], **papszResult;

    sprintf( szSelect, "SELECT WKTEXT, AUTH_SRID, AUTH_NAME FROM MDSYS.CS_SRS WHERE SRID = %d", nId );

    if( oStatement.Execute( szSelect ) != CE_None )
        return NULL;

    papszResult = oStatement.SimpleFetchRow();
    if( CSLCount(papszResult) < 1 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Turn into a spatial reference.                                  */
/* -------------------------------------------------------------------- */
    char *pszWKT = papszResult[0];
    OGRSpatialReference *poSRS = NULL;

    poSRS = new OGRSpatialReference();
    if( poSRS->importFromWkt( &pszWKT ) != OGRERR_NONE )
    {
        delete poSRS;
        poSRS = NULL;
    }

/* -------------------------------------------------------------------- */
/*      If we have a corresponding EPSG code for this SRID, use that    */
/*      authority.                                                      */
/* -------------------------------------------------------------------- */
    int bGotEPSGMapping = FALSE;
    for( i = 0; anEPSGOracleMapping[i] != 0; i += 2 )
    {
        if( anEPSGOracleMapping[i] == nId )
        {
            poSRS->SetAuthority( poSRS->GetRoot()->GetValue(), "EPSG",
                                 anEPSGOracleMapping[i+1] );
            bGotEPSGMapping = TRUE;
            break;
        }
    }

/* -------------------------------------------------------------------- */
/*      Insert authority information, if it is available.               */
/* -------------------------------------------------------------------- */
    if( papszResult[1] != NULL && atoi(papszResult[1]) != 0
        && papszResult[2] != NULL && strlen(papszResult[1]) != 0 
        && poSRS->GetRoot() != NULL 
        && !bGotEPSGMapping )
    {
        poSRS->SetAuthority( poSRS->GetRoot()->GetValue(), 
                             papszResult[2], atoi(papszResult[1]) );
    }

/* -------------------------------------------------------------------- */
/*      Add to the cache.                                               */
/* -------------------------------------------------------------------- */
    panSRID = (int *) CPLRealloc(panSRID,sizeof(int) * (nKnownSRID+1) );
    papoSRS = (OGRSpatialReference **) 
        CPLRealloc(papoSRS, sizeof(void*) * (nKnownSRID + 1) );
    panSRID[nKnownSRID] = nId;
    papoSRS[nKnownSRID] = poSRS;

    nKnownSRID++;

    return poSRS;
}

/************************************************************************/
/*                             FetchSRSId()                             */
/*                                                                      */
/*      Fetch the id corresponding to an SRS, and if not found, add     */
/*      it to the table.                                                */
/************************************************************************/

int OGROCIDataSource::FetchSRSId( OGRSpatialReference * poSRS )

{
    char                *pszWKT = NULL;
    int                 nSRSId;

    if( poSRS == NULL )
        return -1;

    if( !poSRS->IsProjected() && !poSRS->IsGeographic() )
        return -1;

/* ==================================================================== */
/*      The first strategy is to see if we can identify it by           */
/*      authority information within the SRS.  Either using ORACLE      */
/*      authority values directly, or check if there is a known         */
/*      translation for an EPSG authority code.                         */
/* ==================================================================== */
    const char *pszAuthName = NULL, *pszAuthCode = NULL;

    if( poSRS->IsGeographic() )
    {
        pszAuthName = poSRS->GetAuthorityName( "GEOGCS" );
        pszAuthCode = poSRS->GetAuthorityCode( "GEOGCS" );
    }
    else if( poSRS->IsProjected() )
    {
        pszAuthName = poSRS->GetAuthorityName( "PROJCS" );
        pszAuthCode = poSRS->GetAuthorityCode( "PROJCS" );
    }

    if( pszAuthName != NULL && pszAuthCode != NULL )
    {
        if( EQUAL(pszAuthName,"Oracle") 
            && atoi(pszAuthCode) != 0 )
            return atoi(pszAuthCode);

        if( EQUAL(pszAuthName,"EPSG") )
        {
            int i, nEPSGCode = atoi(pszAuthCode);

            for( i = 0; anEPSGOracleMapping[i] != 0; i += 2 )
            {
                if( nEPSGCode == anEPSGOracleMapping[i+1] )
                    return anEPSGOracleMapping[i];
            }
        }
    }

/* ==================================================================== */
/*      We need to lookup the SRS in the existing Oracle CS_SRS         */
/*      table.                                                          */
/* ==================================================================== */

/* -------------------------------------------------------------------- */
/*      Convert SRS into old style format (SF-SQL 1.0).                 */
/* -------------------------------------------------------------------- */
    OGRSpatialReference *poSRS2 = poSRS->Clone();
    
    poSRS2->StripCTParms();

/* -------------------------------------------------------------------- */
/*      Convert any degree type unit names to "Decimal Degree".         */
/* -------------------------------------------------------------------- */
    double dfAngularUnits = poSRS2->GetAngularUnits( NULL );
    if( fabs(dfAngularUnits - 0.0174532925199433) < 0.0000000000000010 )
        poSRS2->SetAngularUnits( "Decimal Degree", 0.0174532925199433 );

/* -------------------------------------------------------------------- */
/*      Translate SRS to WKT.                                           */
/* -------------------------------------------------------------------- */
    if( poSRS2->exportToWkt( &pszWKT ) != OGRERR_NONE )
    {
        delete poSRS2;
        return -1;
    }
    
    delete poSRS2;
    
/* -------------------------------------------------------------------- */
/*      Try to find in the existing table.                              */
/* -------------------------------------------------------------------- */
    OGROCIStringBuf     oCmdText;
    OGROCIStatement     oCmdStatement( GetSession() );
    char                **papszResult = NULL;

    oCmdText.Append( "SELECT SRID FROM MDSYS.CS_SRS WHERE WKTEXT = '" );
    oCmdText.Append( pszWKT );
    oCmdText.Append( "'" );

    if( oCmdStatement.Execute( oCmdText.GetString() ) == CE_None )
        papszResult = oCmdStatement.SimpleFetchRow() ;

/* -------------------------------------------------------------------- */
/*      We got it!  Return it.                                          */
/* -------------------------------------------------------------------- */
    if( CSLCount(papszResult) == 1 )
    {
        CPLFree( pszWKT );
        return atoi( papszResult[0] );
    }
    
/* ==================================================================== */
/*      We didn't find it, so we need to define it as a new SRID at     */
/*      the end of the list of known values.                            */
/* ==================================================================== */

/* -------------------------------------------------------------------- */
/*      Get the current maximum srid in the srs table.                  */
/* -------------------------------------------------------------------- */
    if( oCmdStatement.Execute("SELECT MAX(SRID) FROM MDSYS.CS_SRS") == CE_None )
        papszResult = oCmdStatement.SimpleFetchRow();
    else
        papszResult = NULL;
        
    if( CSLCount(papszResult) == 1 )
        nSRSId = atoi(papszResult[0]) + 1;
    else
        nSRSId = 1;

/* -------------------------------------------------------------------- */
/*      Try adding the SRS to the SRS table.                            */
/* -------------------------------------------------------------------- */
    oCmdText.Clear();
    oCmdText.Append( "INSERT INTO MDSYS.CS_SRS (SRID, WKTEXT, CS_NAME) " );
    oCmdText.Appendf( 100, " VALUES (%d,'", nSRSId );
    oCmdText.Append( pszWKT );
    oCmdText.Append( "', '" );
    oCmdText.Append( poSRS->GetRoot()->GetChild(0)->GetValue() );
    oCmdText.Append( "' )" );

    CPLFree( pszWKT );

    if( oCmdStatement.Execute( oCmdText.GetString() ) != CE_None )
        return -1;
    else
        return nSRSId;
}
