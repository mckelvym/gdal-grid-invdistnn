/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Simple client for translating between formats.
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
 * Revision 1.16  2002/05/09 16:32:29  warmerda
 * added -sql, -update and -append options
 *
 * Revision 1.15  2002/03/05 14:25:14  warmerda
 * expand tabs
 *
 * Revision 1.14  2002/01/18 04:46:38  warmerda
 * added -s_srs support
 *
 * Revision 1.13  2001/11/15 21:18:59  warmerda
 * added transaction grouping on the write side
 *
 * Revision 1.12  2001/10/25 22:33:16  danmo
 * Added support for -select option
 *
 * Revision 1.11  2001/09/27 14:50:10  warmerda
 * added untested -spat and -where support
 *
 * Revision 1.10  2001/09/21 16:19:50  warmerda
 * added support for -a_srs and -t_srs options
 *
 * Revision 1.9  2001/07/18 05:03:05  warmerda
 * added CPL_CVSID
 *
 * Revision 1.8  2001/06/26 20:58:20  warmerda
 * added -nln switch
 *
 * Revision 1.7  2000/12/05 23:09:05  warmerda
 * improved error testing, added lots of CPLResetError calls
 *
 * Revision 1.6  2000/10/17 02:23:33  warmerda
 * improve error reporting
 *
 * Revision 1.5  2000/06/19 18:46:59  warmerda
 * added -skipfailures
 *
 * Revision 1.4  2000/06/09 21:15:19  warmerda
 * made CreateField and SetFrom forgiving
 *
 * Revision 1.3  2000/03/14 21:37:35  warmerda
 * added support for passing options to create methods
 *
 * Revision 1.2  1999/11/18 19:02:19  warmerda
 * expanded tabs
 *
 * Revision 1.1  1999/11/04 21:07:53  warmerda
 * New
 *
 */

#include "ogrsf_frmts.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

static void Usage();

static int TranslateLayer( OGRDataSource *poSrcDS, 
                           OGRLayer * poSrcLayer,
                           OGRDataSource *poDstDS,
                           char ** papszLSCO,
                           const char *pszNewLayerName,
                           int bTransform, 
                           OGRSpatialReference *poOutputSRS,
                           OGRSpatialReference *poSourceSRS,
                           char **papszSelFields,
                           int bAppend );

static int bSkipFailures = FALSE;
static int nGroupTransactions = 200;

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc, char ** papszArgv )

{
    const char  *pszFormat = "ESRI Shapefile";
    const char  *pszDataSource = NULL;
    const char  *pszDestDataSource = NULL;
    char        **papszLayers = NULL;
    char        **papszDSCO = NULL, **papszLCO = NULL;
    int         bTransform = FALSE;
    int         bAppend = FALSE, bUpdate = FALSE;
    const char  *pszOutputSRSDef = NULL;
    const char  *pszSourceSRSDef = NULL;
    OGRSpatialReference *poOutputSRS = NULL;
    OGRSpatialReference *poSourceSRS = NULL;
    const char  *pszNewLayerName = NULL;
    const char  *pszWHERE = NULL;
    OGRGeometry *poSpatialFilter = NULL;
    const char  *pszSelect;
    char        **papszSelFields = NULL;
    const char  *pszSQLStatement = NULL;

/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    OGRRegisterAll();

/* -------------------------------------------------------------------- */
/*      Processing command line arguments.                              */
/* -------------------------------------------------------------------- */
    for( int iArg = 1; iArg < nArgc; iArg++ )
    {
        if( EQUAL(papszArgv[iArg],"-f") && iArg < nArgc-1 )
        {
            pszFormat = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-dsco") && iArg < nArgc-1 )
        {
            papszDSCO = CSLAddString(papszDSCO, papszArgv[++iArg] );
        }
        else if( EQUAL(papszArgv[iArg],"-lco") && iArg < nArgc-1 )
        {
            papszLCO = CSLAddString(papszLCO, papszArgv[++iArg] );
        }
        else if( EQUALN(papszArgv[iArg],"-skip",5) )
        {
            bSkipFailures = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-append") )
        {
            bAppend = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-update") )
        {
            bUpdate = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-sql") && papszArgv[iArg+1] != NULL )
        {
            pszSQLStatement = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-nln") && iArg < nArgc-1 )
        {
            pszNewLayerName = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-tg") && iArg < nArgc-1 )
        {
            nGroupTransactions = atoi(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-s_srs") && iArg < nArgc-1 )
        {
            pszSourceSRSDef = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-a_srs") && iArg < nArgc-1 )
        {
            pszOutputSRSDef = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-t_srs") && iArg < nArgc-1 )
        {
            pszOutputSRSDef = papszArgv[++iArg];
            bTransform = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-spat") 
                 && papszArgv[iArg+1] != NULL 
                 && papszArgv[iArg+2] != NULL 
                 && papszArgv[iArg+3] != NULL 
                 && papszArgv[iArg+4] != NULL )
        {
            OGRLinearRing  oRing;

            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+2]) );
            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+4]) );
            oRing.addPoint( atof(papszArgv[iArg+3]), atof(papszArgv[iArg+4]) );
            oRing.addPoint( atof(papszArgv[iArg+3]), atof(papszArgv[iArg+2]) );
            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+2]) );

            poSpatialFilter = new OGRPolygon();
            ((OGRPolygon *) poSpatialFilter)->addRing( &oRing );
            iArg += 4;
        }
        else if( EQUAL(papszArgv[iArg],"-where") && papszArgv[iArg+1] != NULL )
        {
            pszWHERE = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-select") && papszArgv[iArg+1] != NULL)
        {
            pszSelect = papszArgv[++iArg];
            papszSelFields = CSLTokenizeStringComplex(pszSelect, " ,", 
                                                      FALSE, FALSE );
        }
        else if( papszArgv[iArg][0] == '-' )
        {
            Usage();
        }
        else if( pszDestDataSource == NULL )
            pszDestDataSource = papszArgv[iArg];
        else if( pszDataSource == NULL )
            pszDataSource = papszArgv[iArg];
        else
            papszLayers = CSLAddString( papszLayers, papszArgv[iArg] );
    }

    if( pszDataSource == NULL )
        Usage();

/* -------------------------------------------------------------------- */
/*      Open data source.                                               */
/* -------------------------------------------------------------------- */
    OGRDataSource       *poDS;
        
    poDS = OGRSFDriverRegistrar::Open( pszDataSource, FALSE );

/* -------------------------------------------------------------------- */
/*      Report failure                                                  */
/* -------------------------------------------------------------------- */
    if( poDS == NULL )
    {
        OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
        
        printf( "FAILURE:\n"
                "Unable to open datasource `%s' with the following drivers.\n",
                pszDataSource );

        for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
        {
            printf( "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
        }

        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Try opening the output datasource as an existing, writable      */
/* -------------------------------------------------------------------- */
    OGRDataSource       *poODS;
    
    if( bUpdate )
    {
        poODS = OGRSFDriverRegistrar::Open( pszDestDataSource, TRUE );
        if( poODS == NULL )
        {
            printf( "FAILURE:\n"
                    "Unable to open existing output datasource `%s'.\n",
                    pszDestDataSource );
            exit( 1 );
        }
    }

/* -------------------------------------------------------------------- */
/*      Find the output driver.                                         */
/* -------------------------------------------------------------------- */
    else
    {
        OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();
        OGRSFDriver          *poDriver = NULL;
        int                  iDriver;

        for( iDriver = 0;
             iDriver < poR->GetDriverCount() && poDriver == NULL;
             iDriver++ )
        {
            if( EQUAL(poR->GetDriver(iDriver)->GetName(),pszFormat) )
            {
                poDriver = poR->GetDriver(iDriver);
            }
        }

        if( poDriver == NULL )
        {
            printf( "Unable to find driver `%s'.\n", pszFormat );
            printf( "The following drivers are available:\n" );
        
            for( iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
            {
                printf( "  -> `%s'\n", poR->GetDriver(iDriver)->GetName() );
            }
            exit( 1 );
        }

        if( !poDriver->TestCapability( ODrCCreateDataSource ) )
        {
            printf( "%s driver does not support data source creation.\n",
                    pszFormat );
            exit( 1 );
        }

/* -------------------------------------------------------------------- */
/*      Create the output data source.                                  */
/* -------------------------------------------------------------------- */
        poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
        if( poODS == NULL )
        {
            printf( "%s driver failed to create %s\n", 
                    pszFormat, pszDestDataSource );
            exit( 1 );
        }
    }

/* -------------------------------------------------------------------- */
/*      Parse the output SRS definition if possible.                    */
/* -------------------------------------------------------------------- */
    if( pszOutputSRSDef != NULL )
    {
        poOutputSRS = new OGRSpatialReference();
        if( poOutputSRS->SetFromUserInput( pszOutputSRSDef ) != OGRERR_NONE )
        {
            printf( "Failed to process SRS definition: %s\n", 
                    pszOutputSRSDef );
            exit( 1 );
        }
    }

/* -------------------------------------------------------------------- */
/*      Parse the source SRS definition if possible.                    */
/* -------------------------------------------------------------------- */
    if( pszSourceSRSDef != NULL )
    {
        poSourceSRS = new OGRSpatialReference();
        if( poSourceSRS->SetFromUserInput( pszSourceSRSDef ) != OGRERR_NONE )
        {
            printf( "Failed to process SRS definition: %s\n", 
                    pszSourceSRSDef );
            exit( 1 );
        }
    }

/* -------------------------------------------------------------------- */
/*      Special case for -sql clause.  No source layers required.       */
/* -------------------------------------------------------------------- */
    if( pszSQLStatement != NULL )
    {
        OGRLayer *poResultSet;

        if( pszWHERE != NULL )
            printf( "-where clause ignored in combination with -sql.\n" );
        if( CSLCount(papszLayers) > 0 )
            printf( "layer names ignored in combination with -sql.\n" );
        
        poResultSet = poDS->ExecuteSQL( pszSQLStatement, poSpatialFilter, 
                                        NULL );

        if( poResultSet != NULL )
        {
            if( !TranslateLayer( poDS, poResultSet, poODS, papszLCO, 
                                 pszNewLayerName, bTransform, poOutputSRS,
                                 poSourceSRS, papszSelFields, bAppend ) )
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "Terminating translation prematurely after failed\n"
                          "translation from sql statement." );

                exit( 1 );
            }
            poDS->ReleaseResultSet( poResultSet );
        }
    }

/* -------------------------------------------------------------------- */
/*      Process each data source layer.                                 */
/* -------------------------------------------------------------------- */
    for( int iLayer = 0; 
         pszSQLStatement == NULL && iLayer < poDS->GetLayerCount(); 
         iLayer++ )
    {
        OGRLayer        *poLayer = poDS->GetLayer(iLayer);

        if( poLayer == NULL )
        {
            printf( "FAILURE: Couldn't fetch advertised layer %d!\n",
                    iLayer );
            exit( 1 );
        }

        if( CSLCount(papszLayers) == 0
            || CSLFindString( papszLayers,
                              poLayer->GetLayerDefn()->GetName() ) != -1 )
        {
            if( pszWHERE != NULL )
                poLayer->SetAttributeFilter( pszWHERE );
            
            if( poSpatialFilter != NULL )
                poLayer->SetSpatialFilter( poSpatialFilter );
            
            if( !TranslateLayer( poDS, poLayer, poODS, papszLCO, 
                                 pszNewLayerName, bTransform, poOutputSRS,
                                 poSourceSRS, papszSelFields, bAppend ) 
                && !bSkipFailures )
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "Terminating translation prematurely after failed\n"
                          "translation of layer %s\n", 
                          poLayer->GetLayerDefn()->GetName() );

                exit( 1 );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Close down.                                                     */
/* -------------------------------------------------------------------- */
    delete poODS;
    delete poDS;

    CSLDestroy(papszSelFields);

#ifdef DBMALLOC
    malloc_dump(1);
#endif
    
    return 0;
}

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    OGRSFDriverRegistrar        *poR = OGRSFDriverRegistrar::GetRegistrar();

    printf( "Usage: ogr2ogr [-skipfailures] [-append] [-update] [-f format_name]\n"
            "               [-select field_list] [-where restricted_where]\n"
            "               [-spat xmin ymin xmax ymax]\n"
            "               [-t_srs srs_def] [-a_srs srs_def]\n"
            "               [[-dsco NAME=VALUE] ...] dst_datasource_name\n"
            "               src_datasource_name\n"
            "               [-lco NAME=VALUE] [-nln name] layer [layer ...]]\n"
            "\n"
            " -f format_name: output file format name, possible values are:\n");
    
    for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
    {
        OGRSFDriver *poDriver = poR->GetDriver(iDriver);

        if( poDriver->TestCapability( ODrCCreateDataSource ) )
            printf( "     -f \"%s\"\n", poDriver->GetName() );
    }

    printf( " -select field_list: Comma-delimited list of fields from input layer to\n"
            "                     copy to the new layer (defaults to all)\n" 
            " -where restricted_where: Attribute query (like SQL WHERE)\n" 
            " -spat xmin ymin xmax ymax: spatial query extents\n"
            " -dsco NAME=VALUE: Dataset creation option (format specific)\n"
            " -lco  NAME=VALUE: Layer creation option (format specific)\n"
            " -nln name: Assign an alternate name to the new layer\n" );

    printf(" -a_srs srs_def: Assign an output SRS\n"
           " -t_srs srs_def: Reproject/transform to this SRS on output\n"
           " -s_srs srs_def: Override source SRS\n"
           "\n" 
           " Srs_def can be a full WKT definition (hard to escape properly),\n"
           " or a well known definition (ie. EPSG:4326) or a file with a WKT\n"
           " definition.\n" );

    exit( 1 );
}

/************************************************************************/
/*                           TranslateLayer()                           */
/************************************************************************/

static int TranslateLayer( OGRDataSource *poSrcDS, 
                           OGRLayer * poSrcLayer,
                           OGRDataSource *poDstDS,
                           char **papszLCO,
                           const char *pszNewLayerName,
                           int bTransform, 
                           OGRSpatialReference *poOutputSRS,
                           OGRSpatialReference *poSourceSRS,
                           char **papszSelFields,
                           int bAppend)

{
    OGRLayer    *poDstLayer;
    OGRFeatureDefn *poFDefn;
    OGRErr      eErr;

    if( pszNewLayerName == NULL )
        pszNewLayerName = poSrcLayer->GetLayerDefn()->GetName();

/* -------------------------------------------------------------------- */
/*      Setup coordinate transformation if we need it.                  */
/* -------------------------------------------------------------------- */
    OGRCoordinateTransformation *poCT = NULL;

    if( bTransform )
    {
        if( poSourceSRS == NULL )
            poSourceSRS = poSrcLayer->GetSpatialRef();

        if( poSourceSRS == NULL )
        {
            printf( "Can't transform coordinates, source layer has no\n"
                    "coordinate system.  Use -s_srs to set one.\n" );
            exit( 1 );
        }

        poCT = OGRCreateCoordinateTransformation( poSourceSRS, poOutputSRS );
        if( poCT == NULL )
        {
            char        *pszWKT = NULL;

            printf("Failed to create coordinate transformation between the\n"
                   "following coordinate systems.  This may be because they\n"
                   "are not transformable, or because projection services\n"
                   "(PROJ.4 DLL/.so) could not be loaded.\n" );
            
            poSrcLayer->GetSpatialRef()->exportToPrettyWkt( &pszWKT, FALSE );
            printf( "Source:\n%s\n", pszWKT );
            
            poOutputSRS->exportToPrettyWkt( &pszWKT, FALSE );
            printf( "Target:\n%s\n", pszWKT );
            exit( 1 );
        }
    }
    
/* -------------------------------------------------------------------- */
/*      Get other info.                                                 */
/* -------------------------------------------------------------------- */
    poFDefn = poSrcLayer->GetLayerDefn();
    
    if( poOutputSRS == NULL )
        poOutputSRS = poSrcLayer->GetSpatialRef();
    
/* -------------------------------------------------------------------- */
/*      Create the layer.                                               */
/* -------------------------------------------------------------------- */
    if( !bAppend )
    {
        CPLAssert( poDstDS->TestCapability( ODsCCreateLayer ) );
        CPLErrorReset();

        poDstLayer = poDstDS->CreateLayer( pszNewLayerName, poOutputSRS,
                                           poFDefn->GetGeomType(), papszLCO );

        if( poDstLayer == NULL )
            return FALSE;
    }
/* -------------------------------------------------------------------- */
/*      Or find existing layer to append to.                            */
/* -------------------------------------------------------------------- */
    else
    {
        poDstLayer = NULL;

        for( int iLayer = 0; iLayer < poDstDS->GetLayerCount(); iLayer++ )
        {
            OGRLayer        *poLayer = poDstDS->GetLayer(iLayer);

            if( poLayer != NULL 
                && EQUAL(poLayer->GetLayerDefn()->GetName(),pszNewLayerName) )
            {
                poDstLayer = poLayer;
                break;
            }
        }

        if( poDstLayer == NULL )
        {
            printf( "FAILED: To find existing layer `%s' on output dataset\n"
                    "        to append new features to.\n",
                    pszNewLayerName );
        }
    }

/* -------------------------------------------------------------------- */
/*      Add fields.  Default to copy all field.                         */
/*      If only a subset of all fields requested, then output only      */
/*      the selected fields, and in the order that they were            */
/*      selected.                                                       */
/* -------------------------------------------------------------------- */
    int         iField;

    if (papszSelFields && !bAppend )
    {
        for( iField=0; papszSelFields[iField] != NULL; iField++)
        {
            int iSrcField = poFDefn->GetFieldIndex(papszSelFields[iField]);
            if (iSrcField >= 0)
                poDstLayer->CreateField( poFDefn->GetFieldDefn(iSrcField) );
            else
            {
                printf( "Field '%s' not found in source layer.\n", 
                         papszSelFields[iField] );
                if( !bSkipFailures )
                    return FALSE;
            }
        }
    }
    else if( !bAppend )
    {
        for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
            poDstLayer->CreateField( poFDefn->GetFieldDefn(iField) );
    }

/* -------------------------------------------------------------------- */
/*      Transfer features.                                              */
/* -------------------------------------------------------------------- */
    OGRFeature  *poFeature;
    int         nFeaturesInTransaction = 0;
    
    poSrcLayer->ResetReading();

    if( nGroupTransactions )
        poDstLayer->StartTransaction();

    while( (poFeature = poSrcLayer->GetNextFeature()) != NULL )
    {
        OGRFeature      *poDstFeature;

        if( ++nFeaturesInTransaction == nGroupTransactions )
        {
            poDstLayer->CommitTransaction();
            poDstLayer->StartTransaction();
            nFeaturesInTransaction = 0;
        }

        CPLErrorReset();
        poDstFeature = new OGRFeature( poDstLayer->GetLayerDefn() );

        if( poDstFeature->SetFrom( poFeature, TRUE ) != OGRERR_NONE )
        {
            if( nGroupTransactions )
                poDstLayer->CommitTransaction();
            
            delete poFeature;
            
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Unable to translate feature %d from layer %s.\n",
                      poFeature->GetFID(), poFDefn->GetName() );
            return FALSE;
        }
        
        if( poCT && poDstFeature->GetGeometryRef() != NULL )
        {
            eErr = poDstFeature->GetGeometryRef()->transform( poCT );
            if( eErr != OGRERR_NONE )
            {
                if( nGroupTransactions )
                    poDstLayer->CommitTransaction();

                printf( "Failed to transform feature %d.\n", 
                        (int) poFeature->GetFID() );
                if( !bSkipFailures )
                    return FALSE;
            }
        }
        
        delete poFeature;

        CPLErrorReset();
        if( poDstLayer->CreateFeature( poDstFeature ) != OGRERR_NONE 
            && !bSkipFailures )
        {
            if( nGroupTransactions )
                poDstLayer->RollbackTransaction();

            delete poDstFeature;
            return FALSE;
        }

        delete poDstFeature;
    }

    if( nGroupTransactions )
        poDstLayer->CommitTransaction();

    return TRUE;
}

