/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  OGRSpatialReference translation to/from ESRI .prj definitions.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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

#include "ogr_spatialref.h"
#include "ogr_p.h"
#include "cpl_csv.h"

#include "ogr_srs_esri_names.h"

CPL_CVSID("$Id$");

void  SetNewName( OGRSpatialReference* pOgr, const char* keyName, const char* newName );
int   RemapImgWGSProjcsName( OGRSpatialReference* pOgr, const char* pszProjCSName, 
                            const char* pszProgCSName );
int   RemapImgUTMNames( OGRSpatialReference* pOgr, const char* pszProjCSName, 
                       const char* pszProgCSName, char **mappingTable );
int   RemapNameBasedOnKeyName( OGRSpatialReference* pOgr, const char* pszName, 
                             const char* pszkeyName, char **mappingTable );
int   RemapNamesBasedOnTwo( OGRSpatialReference* pOgr, const char* name1, const char* name2, 
                             char **mappingTable, long nTableStepSize, 
                             const char* pszkeyName );
int   RemapPValuesBasedOnProjCSAndPName( OGRSpatialReference* pOgr, 
                             const char* pszProgCSName, char **mappingTable );
int   RemapPNamesBasedOnProjCSAndPName( OGRSpatialReference* pOgr, 
                             const char* pszProgCSName, char **mappingTable );
int   DeleteParamBasedOnPrjName( OGRSpatialReference* pOgr, 
                             const char* pszProjectionName, char **mappingTable );
int   AddParamBasedOnPrjName( OGRSpatialReference* pOgr, 
                             const char* pszProjectionName, char **mappingTable );
int   AddParam( OGRSpatialReference* pOgr, const char* paraName, const char* paraVal );
void  UnknowNameHandle( OGRSpatialReference* pOgr );
void  RemapProjCSName( OGRSpatialReference* pOgr, char *pszUTMPrefix );
void  RemapProjection( OGRSpatialReference* pOgr );
void  WellKnownGcsHandle( OGRSpatialReference* pOgr, char** pszUTMPrefix );
void  RemapDatumName( OGRSpatialReference* pOgr );
void  ReMapUnitName( OGRSpatialReference* pOgr );
int   RemapGeogCSName( OGRSpatialReference* pOgr, const char *pszGeogCSName );
int   RemapSpheroidName( OGRSpatialReference* pOgr, const char *pszSpheroidName );
int   RemapLCCScaleFactor( OGRSpatialReference* pOgr );
void   RemapSpheroidParameters( OGRSpatialReference* pOgr ); 

static const char *apszProjMapping[] = {
    "Albers", SRS_PT_ALBERS_CONIC_EQUAL_AREA,
    "Cassini", SRS_PT_CASSINI_SOLDNER,
    "Plate_Carree", SRS_PT_EQUIRECTANGULAR,
    "Hotine_Oblique_Mercator_Azimuth_Natural_Origin", 
                                        SRS_PT_HOTINE_OBLIQUE_MERCATOR,
    "Hotine_Oblique_Mercator_Azimuth_Center", 
                                        SRS_PT_HOTINE_OBLIQUE_MERCATOR,
    "Lambert_Conformal_Conic", SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP,
    "Lambert_Conformal_Conic", SRS_PT_LAMBERT_CONFORMAL_CONIC_1SP,
    "Van_der_Grinten_I", SRS_PT_VANDERGRINTEN,
    SRS_PT_TRANSVERSE_MERCATOR, SRS_PT_TRANSVERSE_MERCATOR,
    "Gauss_Kruger", SRS_PT_TRANSVERSE_MERCATOR,
    "Mercator", SRS_PT_MERCATOR_1SP,
    "Equidistant_Cylindrical", SRS_PT_EQUIRECTANGULAR,
    NULL, NULL }; 
 
static const char *apszAlbersMapping[] = {
    SRS_PP_CENTRAL_MERIDIAN, SRS_PP_LONGITUDE_OF_CENTER, 
    SRS_PP_LATITUDE_OF_ORIGIN, SRS_PP_LATITUDE_OF_CENTER,
    "Central_Parallel", SRS_PP_LATITUDE_OF_CENTER,
    NULL, NULL };

static const char *apszECMapping[] = {
    SRS_PP_CENTRAL_MERIDIAN, SRS_PP_LONGITUDE_OF_CENTER, 
    SRS_PP_LATITUDE_OF_ORIGIN, SRS_PP_LATITUDE_OF_CENTER, 
    NULL, NULL };

static const char *apszMercatorMapping[] = {
    SRS_PP_STANDARD_PARALLEL_1, SRS_PP_LATITUDE_OF_ORIGIN,
    NULL, NULL };

static const char *apszPolarStereographicMapping[] = {
    SRS_PP_STANDARD_PARALLEL_1, SRS_PP_LATITUDE_OF_ORIGIN,
    NULL, NULL };

static char **papszDatumMapping = NULL;
 
static const char *apszDefaultDatumMapping[] = {
    "6267", "North_American_1927", SRS_DN_NAD27,
    "6269", "North_American_1983", SRS_DN_NAD83,
    NULL, NULL, NULL }; 

static const char *apszUnitMapping[] = {
    "Meter", "meter",
    "Meter", "metre",
    "Foot", "foot",
    "Foot", "feet",
    "Foot", "international_feet", 
    "Foot_US", SRS_UL_US_FOOT,
    "Foot_Clarke", "clarke_feet",
    "Degree", "degree",
    "Degree", "degrees",
    "Degree", SRS_UA_DEGREE,
    "Radian", SRS_UA_RADIAN,
    NULL, NULL }; 
 
/* -------------------------------------------------------------------- */
/*      Table relating USGS and ESRI state plane zones.                 */
/* -------------------------------------------------------------------- */
static const int anUsgsEsriZones[] =
{
  101, 3101,
  102, 3126,
  201, 3151,
  202, 3176,
  203, 3201,
  301, 3226,
  302, 3251,
  401, 3276,
  402, 3301,
  403, 3326,
  404, 3351,
  405, 3376,
  406, 3401,
  407, 3426,
  501, 3451,
  502, 3476,
  503, 3501,
  600, 3526,
  700, 3551,
  901, 3601,
  902, 3626,
  903, 3576,
 1001, 3651,
 1002, 3676,
 1101, 3701,
 1102, 3726,
 1103, 3751,
 1201, 3776,
 1202, 3801,
 1301, 3826,
 1302, 3851,
 1401, 3876,
 1402, 3901,
 1501, 3926,
 1502, 3951,
 1601, 3976,
 1602, 4001,
 1701, 4026,
 1702, 4051,
 1703, 6426,
 1801, 4076,
 1802, 4101,
 1900, 4126,
 2001, 4151,
 2002, 4176,
 2101, 4201,
 2102, 4226,
 2103, 4251,
 2111, 6351,
 2112, 6376,
 2113, 6401,
 2201, 4276,
 2202, 4301,
 2203, 4326,
 2301, 4351,
 2302, 4376,
 2401, 4401,
 2402, 4426,
 2403, 4451,
 2500,    0,
 2501, 4476,
 2502, 4501,
 2503, 4526,
 2600,    0,
 2601, 4551,
 2602, 4576,
 2701, 4601,
 2702, 4626,
 2703, 4651,
 2800, 4676,
 2900, 4701,
 3001, 4726,
 3002, 4751,
 3003, 4776,
 3101, 4801,
 3102, 4826,
 3103, 4851,
 3104, 4876,
 3200, 4901,
 3301, 4926,
 3302, 4951,
 3401, 4976,
 3402, 5001,
 3501, 5026,
 3502, 5051,
 3601, 5076,
 3602, 5101,
 3701, 5126,
 3702, 5151,
 3800, 5176,
 3900,    0,
 3901, 5201,
 3902, 5226,
 4001, 5251,
 4002, 5276,
 4100, 5301,
 4201, 5326,
 4202, 5351,
 4203, 5376,
 4204, 5401,
 4205, 5426,
 4301, 5451,
 4302, 5476,
 4303, 5501,
 4400, 5526,
 4501, 5551,
 4502, 5576,
 4601, 5601,
 4602, 5626,
 4701, 5651,
 4702, 5676,
 4801, 5701,
 4802, 5726,
 4803, 5751,
 4901, 5776,
 4902, 5801,
 4903, 5826,
 4904, 5851,
 5001, 6101,
 5002, 6126,
 5003, 6151,
 5004, 6176,
 5005, 6201,
 5006, 6226,
 5007, 6251,
 5008, 6276,
 5009, 6301,
 5010, 6326,
 5101, 5876,
 5102, 5901,
 5103, 5926,
 5104, 5951,
 5105, 5976,
 5201, 6001,
 5200, 6026,
 5200, 6076,
 5201, 6051,
 5202, 6051,
 5300,    0, 
 5400,    0
};

void OGREPSGDatumNameMassage( char ** ppszDatum );

/************************************************************************/
/*                           RemapSpheroidName()                        */
/*                                                                      */
/*      Convert Spheroid name to ESRI style name                        */
/************************************************************************/

static const char* RemapSpheroidName(const char* pszName)
{
  if (strcmp(pszName, "WGS 84") == 0)
    return "WGS 1984";

  if (strcmp(pszName, "WGS 72") == 0)
    return "WGS 1972";

  return pszName;
}

/************************************************************************/
/*                           ESRIToUSGSZone()                           */
/*                                                                      */
/*      Convert ESRI style state plane zones to USGS style state        */
/*      plane zones.                                                    */
/************************************************************************/

static int ESRIToUSGSZone( int nESRIZone )

{
    int         nPairs = sizeof(anUsgsEsriZones) / (2*sizeof(int));
    int         i;
    
    if( nESRIZone < 0 )
        return ABS(nESRIZone);

    for( i = 0; i < nPairs; i++ )
    {
        if( anUsgsEsriZones[i*2+1] == nESRIZone )
            return anUsgsEsriZones[i*2];
    }

    return 0;
}

/************************************************************************/
/*                          MorphNameToESRI()                           */
/*                                                                      */
/*      Make name ESRI compatible. Convert spaces and special           */
/*      characters to underscores and then strip down.                  */
/************************************************************************/

static void MorphNameToESRI( char ** ppszName )

{
    int         i, j, k;
    char        *pszName = *ppszName;

/* -------------------------------------------------------------------- */
/*      Translate non-alphanumeric values to underscores.               */
/* -------------------------------------------------------------------- */
    for( i = 0; pszName[i] != '\0'; i++ )
    {
        if( pszName[i] != '+'
            && pszName[i] != '/'
            && !(pszName[i] >= 'A' && pszName[i] <= 'Z')
            && !(pszName[i] >= 'a' && pszName[i] <= 'z')
            && !(pszName[i] >= '0' && pszName[i] <= '9') )
        {
            pszName[i] = '_';
        }
    }

/* -------------------------------------------------------------------- */
/*      Remove repeated, heading and trailing underscores.              */
/* -------------------------------------------------------------------- */
    k = 0;
    while(pszName[k] == '_' && pszName[k] != '\0')
      k++;
    pszName[0] = pszName[k];
    for( i = k+1, j = 0; pszName[i] != '\0'; i++ )
    {
        if( pszName[j] == '_' && pszName[i] == '_' )
            continue;
        if(pszName[j] == '_' && pszName[i] == '/' )
            continue;
        if(pszName[j] == '/' && pszName[i] == '_' )
        {
            pszName[j] = pszName[i];
            continue;
        }
        pszName[++j] = pszName[i];
    }
    if( pszName[j] == '_' )
        pszName[j] = '\0';
    else
        pszName[j+1] = '\0';
}

/************************************************************************/
/*                     CleanESRIDatumMappingTable()                     */
/************************************************************************/

CPL_C_START 
void CleanupESRIDatumMappingTable()

{
    if( papszDatumMapping == NULL )
        return;

    if( papszDatumMapping != (char **) apszDefaultDatumMapping )
    {
        CSLDestroy( papszDatumMapping );
        papszDatumMapping = NULL;
    }
}
CPL_C_END

/************************************************************************/
/*                       InitDatumMappingTable()                        */
/************************************************************************/

static void InitDatumMappingTable()

{
    if( papszDatumMapping != NULL )
        return;

/* -------------------------------------------------------------------- */
/*      Try to open the datum.csv file.                                 */
/* -------------------------------------------------------------------- */
    const char  *pszFilename = CSVFilename("gdal_datum.csv");
    FILE * fp = VSIFOpen( pszFilename, "rb" );

/* -------------------------------------------------------------------- */
/*      Use simple default set if we can't find the file.               */
/* -------------------------------------------------------------------- */
    if( fp == NULL )
    {
        papszDatumMapping = (char **)apszDefaultDatumMapping;
        return;
    }

/* -------------------------------------------------------------------- */
/*      Figure out what fields we are interested in.                    */
/* -------------------------------------------------------------------- */
    char **papszFieldNames = CSVReadParseLine( fp );
    int  nDatumCodeField = CSLFindString( papszFieldNames, "DATUM_CODE" );
    int  nEPSGNameField = CSLFindString( papszFieldNames, "DATUM_NAME" );
    int  nESRINameField = CSLFindString( papszFieldNames, "ESRI_DATUM_NAME" );

    CSLDestroy( papszFieldNames );

    if( nDatumCodeField == -1 || nEPSGNameField == -1 || nESRINameField == -1 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Failed to find required field in gdal_datum.csv in InitDatumMappingTable(), using default table setup." );
        
        papszDatumMapping = (char **)apszDefaultDatumMapping;
        return;
    }
    
/* -------------------------------------------------------------------- */
/*      Read each line, adding a detail line for each.                  */
/* -------------------------------------------------------------------- */
    int nMappingCount = 0;
    const int nMaxDatumMappings = 1000;
    char **papszFields;
    papszDatumMapping = (char **)CPLCalloc(sizeof(char*),nMaxDatumMappings*3);

    for( papszFields = CSVReadParseLine( fp );
         papszFields != NULL;
         papszFields = CSVReadParseLine( fp ) )
    {
        int nFieldCount = CSLCount(papszFields);

        CPLAssert( nMappingCount+1 < nMaxDatumMappings );

        if( MAX(nEPSGNameField,MAX(nDatumCodeField,nESRINameField)) 
            < nFieldCount 
            && nMaxDatumMappings > nMappingCount+1 )
        {
            papszDatumMapping[nMappingCount*3+0] = 
                CPLStrdup( papszFields[nDatumCodeField] );
            papszDatumMapping[nMappingCount*3+1] = 
                CPLStrdup( papszFields[nESRINameField] );
            papszDatumMapping[nMappingCount*3+2] = 
                CPLStrdup( papszFields[nEPSGNameField] );
            OGREPSGDatumNameMassage( &(papszDatumMapping[nMappingCount*3+2]) );

            nMappingCount++;
        }
        CSLDestroy( papszFields );
    }

    VSIFClose( fp );

    papszDatumMapping[nMappingCount*3+0] = NULL;
    papszDatumMapping[nMappingCount*3+1] = NULL;
    papszDatumMapping[nMappingCount*3+2] = NULL;
}


/************************************************************************/
/*                         OSRImportFromESRI()                          */
/************************************************************************/

OGRErr OSRImportFromESRI( OGRSpatialReferenceH hSRS, char **papszPrj )

{
    VALIDATE_POINTER1( hSRS, "OSRImportFromESRI", CE_Failure );

    return ((OGRSpatialReference *) hSRS)->importFromESRI( papszPrj );
}

/************************************************************************/
/*                              OSR_GDV()                               */
/*                                                                      */
/*      Fetch a particular parameter out of the parameter list, or      */
/*      the indicated default if it isn't available.  This is a         */
/*      helper function for importFromESRI().                           */
/************************************************************************/

static double OSR_GDV( char **papszNV, const char * pszField, 
                       double dfDefaultValue )

{
    int         iLine;

    if( papszNV == NULL || papszNV[0] == NULL )
        return dfDefaultValue;

    if( EQUALN(pszField,"PARAM_",6) )
    {
        int     nOffset;

        for( iLine = 0; 
             papszNV[iLine] != NULL && !EQUALN(papszNV[iLine],"Paramet",7);
             iLine++ ) {}

        for( nOffset=atoi(pszField+6); 
             papszNV[iLine] != NULL && nOffset > 0; 
             iLine++ ) 
        {
            if( strlen(papszNV[iLine]) > 0 )
                nOffset--;
        }
        
        while( papszNV[iLine] != NULL && strlen(papszNV[iLine]) == 0 ) 
            iLine++;

        if( papszNV[iLine] != NULL )
        {
            char        **papszTokens, *pszLine = papszNV[iLine];
            double      dfValue;
            
            int         i;
            
            // Trim comments.
            for( i=0; pszLine[i] != '\0'; i++ )
            {
                if( pszLine[i] == '/' && pszLine[i+1] == '*' )
                    pszLine[i] = '\0';
            }

            papszTokens = CSLTokenizeString(papszNV[iLine]);
            if( CSLCount(papszTokens) == 3 )
            {
                dfValue = ABS(atof(papszTokens[0]))
                    + atof(papszTokens[1]) / 60.0
                    + atof(papszTokens[2]) / 3600.0;

                if( atof(papszTokens[0]) < 0.0 )
                    dfValue *= -1;
            }
            else if( CSLCount(papszTokens) > 0 )
                dfValue = atof(papszTokens[0]);
            else
                dfValue = dfDefaultValue;

            CSLDestroy( papszTokens );

            return dfValue;
        }
        else
            return dfDefaultValue;
    }
    else
    {
        for( iLine = 0; 
             papszNV[iLine] != NULL && 
                 !EQUALN(papszNV[iLine],pszField,strlen(pszField));
             iLine++ ) {}

        if( papszNV[iLine] == NULL )
            return dfDefaultValue;
        else
            return atof( papszNV[iLine] + strlen(pszField) );
    }
}

/************************************************************************/
/*                              OSR_GDS()                               */
/************************************************************************/

static CPLString OSR_GDS( char **papszNV, const char * pszField, 
                          const char *pszDefaultValue )

{
    int         iLine;

    if( papszNV == NULL || papszNV[0] == NULL )
        return pszDefaultValue;

    for( iLine = 0; 
         papszNV[iLine] != NULL && 
             !EQUALN(papszNV[iLine],pszField,strlen(pszField));
         iLine++ ) {}

    if( papszNV[iLine] == NULL )
        return pszDefaultValue;
    else
    {
        char     szResult[80];
        char    **papszTokens;
        
        papszTokens = CSLTokenizeString(papszNV[iLine]);

        if( CSLCount(papszTokens) > 1 )
            strncpy( szResult, papszTokens[1], sizeof(szResult));
        else
            strncpy( szResult, pszDefaultValue, sizeof(szResult));
        
        CSLDestroy( papszTokens );
        return szResult;
    }
}

/************************************************************************/
/*                          importFromESRI()                            */
/************************************************************************/

/**
 * Import coordinate system from ESRI .prj format(s).
 *
 * This function will read the text loaded from an ESRI .prj file, and
 * translate it into an OGRSpatialReference definition.  This should support
 * many (but by no means all) old style (Arc/Info 7.x) .prj files, as well
 * as the newer pseudo-OGC WKT .prj files.  Note that new style .prj files
 * are in OGC WKT format, but require some manipulation to correct datum
 * names, and units on some projection parameters.  This is addressed within
 * importFromESRI() by an automatical call to morphFromESRI(). 
 *
 * Currently only GEOGRAPHIC, UTM, STATEPLANE, GREATBRITIAN_GRID, ALBERS, 
 * EQUIDISTANT_CONIC, and TRANSVERSE (mercator) projections are supported
 * from old style files. 
 *
 * At this time there is no equivelent exportToESRI() method.  Writing old
 * style .prj files is not supported by OGRSpatialReference. However the
 * morphToESRI() and exportToWkt() methods can be used to generate output
 * suitable to write to new style (Arc 8) .prj files. 
 *
 * This function is the equilvelent of the C function OSRImportFromESRI().
 *
 * @param papszPrj NULL terminated list of strings containing the definition.
 *
 * @return OGRERR_NONE on success or an error code in case of failure. 
 */

OGRErr OGRSpatialReference::importFromESRI( char **papszPrj )

{
    if( papszPrj == NULL || papszPrj[0] == NULL )
        return OGRERR_CORRUPT_DATA;

/* -------------------------------------------------------------------- */
/*      ArcGIS and related products now use a varient of Well Known     */
/*      Text.  Try to recognise this and ingest it.  WKT is usually     */
/*      all on one line, but we will accept multi-line formats and      */
/*      concatenate.                                                    */
/* -------------------------------------------------------------------- */
    if( EQUALN(papszPrj[0],"GEOGCS",6)
        || EQUALN(papszPrj[0],"PROJCS",6)
        || EQUALN(papszPrj[0],"LOCAL_CS",8) )
    {
        char    *pszWKT, *pszWKT2;
        OGRErr  eErr;
        int     i;

        pszWKT = CPLStrdup(papszPrj[0]);
        for( i = 1; papszPrj[i] != NULL; i++ )
        {
            pszWKT = (char *) 
                CPLRealloc(pszWKT,strlen(pszWKT)+strlen(papszPrj[i])+1);
            strcat( pszWKT, papszPrj[i] );
        }
        pszWKT2 = pszWKT;
        eErr = importFromWkt( &pszWKT2 );
        CPLFree( pszWKT );

        if( eErr == OGRERR_NONE )
            eErr = morphFromESRI();
        return eErr;
    }

/* -------------------------------------------------------------------- */
/*      Operate on the basis of the projection name.                    */
/* -------------------------------------------------------------------- */
    CPLString osProj = OSR_GDS( papszPrj, "Projection", "" );

    if( EQUAL(osProj,"") )
    {
        CPLDebug( "OGR_ESRI", "Can't find Projection\n" );
        return OGRERR_CORRUPT_DATA;
    }

    else if( EQUAL(osProj,"GEOGRAPHIC") )
    {
    }
    
    else if( EQUAL(osProj,"utm") )
    {
        if( (int) OSR_GDV( papszPrj, "zone", 0.0 ) != 0 )
        {
            double      dfYShift = OSR_GDV( papszPrj, "Yshift", 0.0 );

            SetUTM( (int) OSR_GDV( papszPrj, "zone", 0.0 ),
                    dfYShift == 0.0 );
        }
        else
        {
            double      dfCentralMeridian, dfRefLat;
            int         nZone;

            dfCentralMeridian = OSR_GDV( papszPrj, "PARAM_1", 0.0 );
            dfRefLat = OSR_GDV( papszPrj, "PARAM_2", 0.0 );

            nZone = (int) ((dfCentralMeridian+183) / 6.0 + 0.0000001);
            SetUTM( nZone, dfRefLat >= 0.0 );
        }
    }

    else if( EQUAL(osProj,"STATEPLANE") )
    {
        int nZone = (int) OSR_GDV( papszPrj, "zone", 0.0 );
        if( nZone != 0 )
            nZone = ESRIToUSGSZone( nZone );
        else
            nZone = (int) OSR_GDV( papszPrj, "fipszone", 0.0 );

        if( nZone != 0 )
        {
            if( EQUAL(OSR_GDS( papszPrj, "Datum", "NAD83"),"NAD27") )
                SetStatePlane( nZone, FALSE );
            else
                SetStatePlane( nZone, TRUE );
        }
    }

    else if( EQUAL(osProj,"GREATBRITIAN_GRID") 
             || EQUAL(osProj,"GREATBRITAIN_GRID") )
    {
        const char *pszWkt = 
            "PROJCS[\"OSGB 1936 / British National Grid\",GEOGCS[\"OSGB 1936\",DATUM[\"OSGB_1936\",SPHEROID[\"Airy 1830\",6377563.396,299.3249646]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"latitude_of_origin\",49],PARAMETER[\"central_meridian\",-2],PARAMETER[\"scale_factor\",0.999601272],PARAMETER[\"false_easting\",400000],PARAMETER[\"false_northing\",-100000],UNIT[\"metre\",1]]";

        importFromWkt( (char **) &pszWkt );
    }

    else if( EQUAL(osProj,"ALBERS") )
    {
        SetACEA( OSR_GDV( papszPrj, "PARAM_1", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_4", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_5", 0.0 ), 
                 OSR_GDV( papszPrj, "PARAM_6", 0.0 ) );
    }

    else if( EQUAL(osProj,"LAMBERT") )
    {
        SetLCC( OSR_GDV( papszPrj, "PARAM_1", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_2", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_4", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_3", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_5", 0.0 ),
                OSR_GDV( papszPrj, "PARAM_6", 0.0 ) );
    }

    else if( EQUAL(osProj,"EQUIDISTANT_CONIC") )
    {
        int     nStdPCount = (int) OSR_GDV( papszPrj, "PARAM_1", 0.0 );

        if( nStdPCount == 1 )
        {
            SetEC( OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_4", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_5", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_6", 0.0 ) );
        }
        else
        {
            SetEC( OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_5", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_4", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_5", 0.0 ), 
                   OSR_GDV( papszPrj, "PARAM_7", 0.0 ) );
        }
    }

    else if( EQUAL(osProj,"TRANSVERSE") )
    {
        SetTM( OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_1", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_4", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_5", 0.0 ) );
    }

    else if( EQUAL(osProj,"POLAR") )
    {
        SetPS( OSR_GDV( papszPrj, "PARAM_2", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_1", 0.0 ), 
               1.0,
               OSR_GDV( papszPrj, "PARAM_3", 0.0 ), 
               OSR_GDV( papszPrj, "PARAM_4", 0.0 ) );
    }

    else
    {
        CPLDebug( "OGR_ESRI", "Unsupported projection: %s", osProj.c_str() );
        SetLocalCS( osProj );
    }

/* -------------------------------------------------------------------- */
/*      Try to translate the datum/spheroid.                            */
/* -------------------------------------------------------------------- */
    if( !IsLocal() && GetAttrNode( "GEOGCS" ) == NULL )
    {
        CPLString osDatum;

        osDatum = OSR_GDS( papszPrj, "Datum", "");

        if( EQUAL(osDatum,"NAD27") || EQUAL(osDatum,"NAD83")
            || EQUAL(osDatum,"WGS84") || EQUAL(osDatum,"WGS72") )
        {
            SetWellKnownGeogCS( osDatum );
        }
        else if( EQUAL( osDatum, "EUR" )
                 || EQUAL( osDatum, "ED50" ) )
        {
            SetWellKnownGeogCS( "EPSG:4230" );
        }
        else if( EQUAL( osDatum, "GDA94" ) )
        {
            SetWellKnownGeogCS( "EPSG:4283" );
        }
        else
        {
            CPLString osSpheroid;

            osSpheroid = OSR_GDS( papszPrj, "Spheroid", "");
            
            if( EQUAL(osSpheroid,"INT1909") 
                || EQUAL(osSpheroid,"INTERNATIONAL1909") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4022 );
                CopyGeogCSFrom( &oGCS );
            }
            else if( EQUAL(osSpheroid,"AIRY") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4001 );
                CopyGeogCSFrom( &oGCS );
            }
            else if( EQUAL(osSpheroid,"CLARKE1866") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4008 );
                CopyGeogCSFrom( &oGCS );
            }
            else if( EQUAL(osSpheroid,"GRS80") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4019 );
                CopyGeogCSFrom( &oGCS );
            }
            else if( EQUAL(osSpheroid,"KRASOVSKY") 
                     || EQUAL(osSpheroid,"KRASSOVSKY") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4024 );
                CopyGeogCSFrom( &oGCS );
            }
            else if( EQUAL(osSpheroid,"Bessel") )
            {
                OGRSpatialReference oGCS;
                oGCS.importFromEPSG( 4004 );
                CopyGeogCSFrom( &oGCS );
            }
            else
            {
                // If we don't know, default to WGS84 so there is something there.
                SetWellKnownGeogCS( "WGS84" );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Linear units translation                                        */
/* -------------------------------------------------------------------- */
    if( IsLocal() || IsProjected() )
    {
        CPLString osValue;
        double dfOldUnits = GetLinearUnits();

        osValue = OSR_GDS( papszPrj, "Units", "" );
        if( EQUAL(osValue, "" ) )
            SetLinearUnitsAndUpdateParameters( SRS_UL_METER, 1.0 );
        else if( EQUAL(osValue,"FEET") )
            SetLinearUnitsAndUpdateParameters( SRS_UL_US_FOOT, atof(SRS_UL_US_FOOT_CONV) );
        else if( atof(osValue) != 0.0 )
            SetLinearUnitsAndUpdateParameters( "user-defined", 
                                               1.0 / atof(osValue) );
        else
            SetLinearUnitsAndUpdateParameters( osValue, 1.0 );

        // If we have reset the linear units we should clear any authority
        // nodes on the PROJCS.  This especially applies to state plane
        // per bug 1697
        double dfNewUnits = GetLinearUnits();
        if( dfOldUnits != 0.0 
            && (dfNewUnits / dfOldUnits < 0.9999999
                || dfNewUnits / dfOldUnits > 1.0000001) )
        {
            if( GetRoot()->FindChild( "AUTHORITY" ) != -1 )
                GetRoot()->DestroyChild(GetRoot()->FindChild( "AUTHORITY" ));
        }
    }
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                            morphToESRI()                             */
/************************************************************************/

/**
 * Convert in place from ESRI WKT format.
 *
 * The value notes of this coordinate system as modified in various manners
 * to adhere more closely to the WKT standard.  This mostly involves
 * translating a variety of ESRI names for projections, arguments and
 * datums to "standard" names, as defined by Adam Gawne-Cain's reference
 * translation of EPSG to WKT for the CT specification.
 *
 * This does the same as the C function OSRMorphToESRI().
 *
 * @return OGRERR_NONE unless something goes badly wrong.
 */

OGRErr OGRSpatialReference::morphToESRI()

{
    OGRErr      eErr;

    CPLLocaleC localeC;
/* -------------------------------------------------------------------- */
/*      Fixup ordering, missing linear units, etc.                      */
/* -------------------------------------------------------------------- */
    eErr = Fixup();
    if( eErr != OGRERR_NONE )
        return eErr;

/* -------------------------------------------------------------------- */
/*      Strip all CT parameters (AXIS, AUTHORITY, TOWGS84, etc).        */
/* -------------------------------------------------------------------- */
    eErr = StripCTParms();
    if( eErr != OGRERR_NONE )
        return eErr;

    if( GetRoot() == NULL )
        return OGRERR_NONE;

/* -------------------------------------------------------------------- */
/*      Force Unnamed to Unknown for most common locations.             */
/* -------------------------------------------------------------------- */
    UnknowNameHandle(this);

/* -------------------------------------------------------------------- */
/*      Translate UNIT keywords that are misnamed, or even the wrong    */
/*      case.                                                           */
/* -------------------------------------------------------------------- */
    ReMapUnitName( this );

/* -------------------------------------------------------------------- */
/*      Very specific handling for some well known geographic           */
/*      coordinate systems.                                             */
/* -------------------------------------------------------------------- */
    char *pszUTMPrefix = NULL;
    WellKnownGcsHandle(this, &pszUTMPrefix);

/* -------------------------------------------------------------------- */
/*      Re-map projCS name to ESRI style. If the PROJCS name is unset,  */
/*      use the PROJECTION name in place of unknown, or unnamed. At the */
/*      request of Peng Gao.                                            */
/* -------------------------------------------------------------------- */
    RemapProjCSName(this, pszUTMPrefix);
    if(pszUTMPrefix)
      CPLFree( pszUTMPrefix );

/* -------------------------------------------------------------------- */
/*      Translate GEOGCS keywords that are misnamed.                    */
/* -------------------------------------------------------------------- */
    const char *pszGcsName = GetAttrValue( "GEOGCS" ); 
    if(pszGcsName && strlen(pszGcsName) > 0)
      RemapGeogCSName(this, pszGcsName);

/* -------------------------------------------------------------------- */
/*      Translate DATUM keywords that are misnamed.                     */
/* -------------------------------------------------------------------- */
    RemapDatumName( this );

/* -------------------------------------------------------------------- */
/*      Convert SPHEROID name and its values.                           */
/* -------------------------------------------------------------------- */
    RemapSpheroidParameters(this); 
    
/* -------------------------------------------------------------------- */
/*      Re-map misnamed projection name for some special cases,         */
/*      such as Hotine Oblique Mercator, Stereographic_South_Pole,      */
/*      Stereographic_North_Poleto and OBLIQUE_STEREOGRAPHIC.           */
/*      Some projection parameters are also re-mapped.                  */
/* -------------------------------------------------------------------- */
    RemapProjection( this );

    return OGRERR_NONE;
}

/************************************************************************/
/*                           OSRMorphToESRI()                           */
/************************************************************************/

OGRErr OSRMorphToESRI( OGRSpatialReferenceH hSRS )

{
    VALIDATE_POINTER1( hSRS, "OSRMorphToESRI", CE_Failure );

    return ((OGRSpatialReference *) hSRS)->morphToESRI();
}

/************************************************************************/
/*                           morphFromESRI()                            */
/*                                                                      */
/*      modify this definition from the ESRI definition of WKT to       */
/*      the "Standard" definition.                                      */
/************************************************************************/

/**
 * Convert in place to ESRI WKT format.
 *
 * The value nodes of this coordinate system as modified in various manners
 * more closely map onto the ESRI concept of WKT format.  This includes
 * renaming a variety of projections and arguments, and stripping out 
 * nodes note recognised by ESRI (like AUTHORITY and AXIS). 
 *
 * This does the same as the C function OSRMorphFromESRI().
 *
 * @return OGRERR_NONE unless something goes badly wrong.
 */

OGRErr OGRSpatialReference::morphFromESRI()

{
    OGRErr      eErr = OGRERR_NONE;

    if( GetRoot() == NULL )
        return OGRERR_NONE;

/* -------------------------------------------------------------------- */
/*      Translate DATUM keywords that are oddly named.                  */
/* -------------------------------------------------------------------- */
    InitDatumMappingTable();

    GetRoot()->applyRemapper( "DATUM", 
                              (char **)papszDatumMapping+1,
                              (char **)papszDatumMapping+2, 3 );

/* -------------------------------------------------------------------- */
/*      Try to remove any D_ in front of the datum name.                */
/* -------------------------------------------------------------------- */
    OGR_SRSNode *poDatum;

    poDatum = GetAttrNode( "DATUM" );
    if( poDatum != NULL )
        poDatum = poDatum->GetChild(0);

    if( poDatum != NULL )
    {
        if( EQUALN(poDatum->GetValue(),"D_",2) )
        {
            char *pszNewValue = CPLStrdup( poDatum->GetValue() + 2 );
            poDatum->SetValue( pszNewValue );
            CPLFree( pszNewValue );
        }
    }

/* -------------------------------------------------------------------- */
/*      Split Lambert_Conformal_Conic into 1SP or 2SP form.             */
/*                                                                      */
/*      See bugzilla.remotesensing.org/show_bug.cgi?id=187              */
/*                                                                      */
/*      We decide based on whether it has 2SPs.  We used to assume      */
/*      1SP if it had a scale factor but that turned out to be a        */
/*      poor test.                                                      */
/* -------------------------------------------------------------------- */
    const char *pszProjection = GetAttrValue("PROJECTION");
    
    if( pszProjection != NULL
        && EQUAL(pszProjection,"Lambert_Conformal_Conic") )
    {
        if( GetProjParm( SRS_PP_STANDARD_PARALLEL_1, 1000.0 ) != 1000.0 
            && GetProjParm( SRS_PP_STANDARD_PARALLEL_2, 1000.0 ) != 1000.0 )
            SetNode( "PROJCS|PROJECTION", 
                     SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP );
        else
            SetNode( "PROJCS|PROJECTION", 
                     SRS_PT_LAMBERT_CONFORMAL_CONIC_1SP );

        pszProjection = GetAttrValue("PROJECTION");
    }

/* -------------------------------------------------------------------- */
/*      Remap Albers, Mercator and Polar Stereographic parameters.      */
/* -------------------------------------------------------------------- */
    if( pszProjection != NULL && EQUAL(pszProjection,"Albers") )
        GetRoot()->applyRemapper( 
            "PARAMETER", (char **)apszAlbersMapping + 0,
            (char **)apszAlbersMapping + 1, 2 );

    if( pszProjection != NULL 
        && (EQUAL(pszProjection,SRS_PT_EQUIDISTANT_CONIC) ||
            EQUAL(pszProjection,SRS_PT_LAMBERT_AZIMUTHAL_EQUAL_AREA) ||
            EQUAL(pszProjection,SRS_PT_AZIMUTHAL_EQUIDISTANT) ||
            EQUAL(pszProjection,SRS_PT_SINUSOIDAL) ||
            EQUAL(pszProjection,SRS_PT_ROBINSON) ) )
        GetRoot()->applyRemapper( 
            "PARAMETER", (char **)apszECMapping + 0,
            (char **)apszECMapping + 1, 2 );

    if( pszProjection != NULL && EQUAL(pszProjection,"Mercator") )
        GetRoot()->applyRemapper( 
            "PARAMETER",
            (char **)apszMercatorMapping + 0,
            (char **)apszMercatorMapping + 1, 2 );

    if( pszProjection != NULL 
        && EQUALN(pszProjection,"Stereographic_",14) 
        && EQUALN(pszProjection+strlen(pszProjection)-5,"_Pole",5) )
        GetRoot()->applyRemapper( 
            "PARAMETER", 
            (char **)apszPolarStereographicMapping + 0, 
            (char **)apszPolarStereographicMapping + 1, 2 );

/* -------------------------------------------------------------------- */
/*      Remap south and north polar stereographic to one value.         */
/* -------------------------------------------------------------------- */
    if( pszProjection != NULL
        && EQUALN(pszProjection,"Stereographic_",14)
        && EQUALN(pszProjection+strlen(pszProjection)-5,"_Pole",5) )
    {
        SetNode( "PROJCS|PROJECTION", SRS_PT_POLAR_STEREOGRAPHIC );
        pszProjection = GetAttrValue("PROJECTION");
    }

/* -------------------------------------------------------------------- */
/*      Translate PROJECTION keywords that are misnamed.                */
/* -------------------------------------------------------------------- */
    GetRoot()->applyRemapper( "PROJECTION", 
                              (char **)apszProjMapping,
                              (char **)apszProjMapping+1, 2 );
    
/* -------------------------------------------------------------------- */
/*      Translate DATUM keywords that are misnamed.                     */
/* -------------------------------------------------------------------- */
    InitDatumMappingTable();

    GetRoot()->applyRemapper( "DATUM", 
                              (char **)papszDatumMapping+1,
                              (char **)papszDatumMapping+2, 3 );

    return eErr;
}

/************************************************************************/
/*                     ImportFromStatePlaneWKT()                        */
/*                                                                      */
/*      Search a ESRI State Plane WKT and import it.                    */
/************************************************************************/
OGRErr OGRSpatialReference::ImportFromStatePlaneWKT(  int code, const char* datumName, const char* unitsName, int pcsCode )
{
  int i;
  long searchCode = -1;

  /* Find state plane prj str by pcs code only */
  if( code == 0 && !datumName && pcsCode != 32767 )
  {

    int unitCode = 1;
    if( strstr(unitsName, "feet") || strstr(unitsName, "foot") )
      unitCode = 2;
    else if( EQUAL(unitsName, "international_feet") )
      unitCode = 3;
    for(i=0; statePlanePcsCodeToZoneCode[i] != 0; i+=2)
    {
      if( pcsCode == statePlanePcsCodeToZoneCode[i] )
      {
        searchCode = statePlanePcsCodeToZoneCode[i+1];
        int unitIndex =  searchCode % 10;
        if( unitCode == 1 && !(unitIndex == 0 || unitIndex == 1) 
         || unitCode == 2 && !(unitIndex == 2 || unitIndex == 3 || unitIndex == 4 )
         || unitCode == 3 && !(unitIndex == 5 || unitIndex == 6 ) )
        {
          searchCode -= unitIndex; 
          switch (unitIndex)
          {
          case 0:
          case 3:
          case 5:
            if(unitCode == 2)
              searchCode += 3;
            else if(unitCode == 3)
              searchCode += 5;
            break;
          case 1:
          case 2:
          case 6:
            if(unitCode == 1)
              searchCode += 1;
            if(unitCode == 2)
              searchCode += 2;
            else if(unitCode == 3)
              searchCode += 6;
           break;
          case 4:
            if(unitCode == 2)
              searchCode += 4;
            break;
          }
        }
        break;
      }
    }
  }
  else /* Find state plane prj str by all inputs. */
  {
    /* Need to have a specail EPSG-ESRI zone code mapping first. */
    for(i=0; statePlaneZoneMapping[i] != 0; i+=3)
    {
      if( code == statePlaneZoneMapping[i] 
       && (statePlaneZoneMapping[i+1] == -1 || pcsCode == statePlaneZoneMapping[i+1]))
      {
        code = statePlaneZoneMapping[i+2];
        break;
      }
    }
    searchCode = (long)code * 10;
    if(EQUAL(datumName, "HARN"))
    {
      if( EQUAL(unitsName, "international_feet") )
        searchCode += 5;
      else if( strstr(unitsName, "feet") || strstr(unitsName, "foot") )
        searchCode += 3;
    }
    else if(strstr(datumName, "NAD") && strstr(datumName, "83"))
    {
      if( EQUAL(unitsName, "meters") )
         searchCode += 1;
      else if( EQUAL(unitsName, "international_feet") )
        searchCode += 6;
      else if( strstr(unitsName, "feet") || strstr(unitsName, "foot") )
         searchCode += 2;
    }
    else if(strstr(datumName, "NAD") && strstr(datumName, "27") && !EQUAL(unitsName, "meters"))
    {
      searchCode += 4;
    }
    else
      searchCode = -1;
  }
  if(searchCode > 0)
  {
    char codeS[10];
    sprintf(codeS, "%d", searchCode);
    return importFromDict( "esri_StatePlane_extra.wkt", codeS);
  }
  return OGRERR_FAILURE;
}

/************************************************************************/
/*                     ImportFromWisconsinWKT()                         */
/*                                                                      */
/*      Search a ESRI State Plane WKT and import it.                    */
/************************************************************************/
OGRErr OGRSpatialReference::ImportFromWisconsinWKT( char* prjName, double centralMeridian, double latOfOrigin, const char* unitsName )
{
  double* tableWISCRS;
  if(EQUAL(prjName, SRS_PT_LAMBERT_CONFORMAL_CONIC))
    tableWISCRS = apszWISCRS_LCC_meter;
  else if(EQUAL(prjName, SRS_PT_TRANSVERSE_MERCATOR))
    tableWISCRS = apszWISCRS_TM_meter;
  else
    return OGRERR_FAILURE;
  long k = -1;
  for(int i=0; tableWISCRS[i] != 0; i+=3)
  {
    if( fabs(centralMeridian - tableWISCRS[i]) <= 0.0000000001 && fabs(latOfOrigin - tableWISCRS[i+1]) <= 0.0000000001) 
    {
      k = (long)tableWISCRS[i+2];
      break;
    }
  }
  if(k > 0)
  {
    if(!EQUAL(unitsName, "meters"))
      k += 100;
    char codeS[10];
    sprintf(codeS, "%d", k);
    return importFromDict( "esri_Wisconsin_extra.wkt", codeS);
  }
  return OGRERR_FAILURE;
}

/************************************************************************/
/*                          OSRMorphFromESRI()                          */
/************************************************************************/

OGRErr OSRMorphFromESRI( OGRSpatialReferenceH hSRS )

{
    VALIDATE_POINTER1( hSRS, "OSRMorphFromESRI", CE_Failure );

    return ((OGRSpatialReference *) hSRS)->morphFromESRI();
}

/************************************************************************/
/*                           SetNewName()                               */
/*                                                                      */
/*      Set an esri name                                                */
/************************************************************************/
void SetNewName( OGRSpatialReference* pOgr, const char* keyName, const char* newName )
{
  OGR_SRSNode *poNode = pOgr->GetAttrNode( keyName );
  OGR_SRSNode *poNodeChild = NULL;
  if(poNode)
    poNodeChild = poNode->GetChild(0);
  if(poNodeChild)
      poNodeChild->SetValue( newName );
}

/************************************************************************/
/*                           RemapImgWGSProjcsName()                    */
/*                                                                      */
/*      Convert Img projcs names to ESRI style                          */
/************************************************************************/
int RemapImgWGSProjcsName( OGRSpatialReference* pOgr, const char* pszProjCSName, const char* pszProgCSName )
{
  if(EQUAL(pszProgCSName, "WGS_1972") || EQUAL(pszProgCSName, "WGS_1984") )
  {
    char* newName = (char *) CPLMalloc(strlen(pszProjCSName) + 10);
    sprintf( newName, "%s_", pszProgCSName );
    strcat(newName, pszProjCSName);
    SetNewName( pOgr, "PROJCS", newName );
    CPLFree( newName );
    return 1;
  }
  return -1;
}

/************************************************************************/
/*                           RemapImgUTMNames()                         */
/*                                                                      */
/*      Convert Img UTM names to ESRI style                             */
/************************************************************************/

int RemapImgUTMNames( OGRSpatialReference* pOgr, const char* pszProjCSName, const char* pszProgCSName, 
                                          char **mappingTable )
{
  long i, n;
  long index = -1;
  for( i = 0; mappingTable[i] != NULL; i += 5 )
  {
    if( EQUAL(pszProjCSName, mappingTable[i]) )
    {
      long j = i;
      while(mappingTable[j] != NULL && EQUAL(mappingTable[i], mappingTable[j]))
      {
        if( EQUAL(pszProgCSName, mappingTable[j+1]) )
        {
          index = j;
          break;
        }
        j += 5;
      }
      if (index >= 0)
        break;
    }
  }
  if(index >= 0)
  {
    OGR_SRSNode *poNode = pOgr->GetAttrNode( "PROJCS" );
    OGR_SRSNode *poNodeChild = NULL;
    if(poNode)
      poNodeChild = poNode->GetChild(0);
    if( poNodeChild && strlen(poNodeChild->GetValue()) > 0 )
        poNodeChild->SetValue( mappingTable[index+2]);

    poNode = pOgr->GetAttrNode( "GEOGCS" );
    poNodeChild = NULL;
    if(poNode)
      poNodeChild = poNode->GetChild(0);
    if( poNodeChild && strlen(poNodeChild->GetValue()) > 0 )
        poNodeChild->SetValue( mappingTable[index+3]);

    poNode = pOgr->GetAttrNode( "DATUM" );
    poNodeChild = NULL;
    if(poNode)
      poNodeChild = poNode->GetChild(0);
    if( poNodeChild && strlen(poNodeChild->GetValue()) > 0 )
        poNodeChild->SetValue( mappingTable[index+4]);
  }
  return index;
}

/************************************************************************/
/*                           RemapNameBasedOnKeyName()                  */
/*                                                                      */
/*      Convert a name to ESRI style name                               */
/************************************************************************/

int RemapNameBasedOnKeyName( OGRSpatialReference* pOgr, const char* pszName, const char* pszkeyName, 
                                                 char **mappingTable )
{
  long i;
  long index = -1;
  for( i = 0; mappingTable[i] != NULL; i += 2 )
  {
    if( EQUAL(pszName, mappingTable[i]) )
    {
      index = i;
      break;
    }
  }
  if(index >= 0) 
  {
    OGR_SRSNode *poNode = pOgr->GetAttrNode( pszkeyName );
    OGR_SRSNode *poNodeChild = NULL;
    if(poNode)
      poNodeChild = poNode->GetChild(0);
    if( poNodeChild && strlen(poNodeChild->GetValue()) > 0 )
        poNodeChild->SetValue( mappingTable[index+1]);
  }
  return index;
}

/************************************************************************/
/*                     RemapNamesBasedOnTwo()                           */
/*                                                                      */
/*      Convert a name to ESRI style name                               */
/************************************************************************/

int RemapNamesBasedOnTwo( OGRSpatialReference* pOgr, const char* name1, const char* name2, 
                                              char **mappingTable, long nTableStepSize, 
                                              const char* pszkeyName)
{
  long i;
  long index = -1;
  if(!name1 || strlen(name1) == 0 || !name2 || strlen(name2) == 0)
    return index;
  for( i = 0; mappingTable[i] != NULL; i += nTableStepSize )
  {
    if( EQUAL(name1, mappingTable[i]) )
    {
      long j = i;
      while(mappingTable[j] != NULL && EQUAL(mappingTable[i], mappingTable[j]))
      {
        if( EQUAL(name2, mappingTable[j+1]) )
        {
          index = j;
          break;
        }
        j += 3;
      }
      if (index >= 0)
        break;
    }
  }
  if(index >= 0)
  {
    OGR_SRSNode *poNode = pOgr->GetAttrNode( pszkeyName );
    OGR_SRSNode *poNodeChild = NULL;
    if(poNode)
      poNodeChild = poNode->GetChild(0);
    if( poNodeChild && strlen(poNodeChild->GetValue()) > 0 )
        poNodeChild->SetValue( mappingTable[index+2]);
  }
  return index;
}

/************************************************************************/
/*                RemapPValuesBasedOnProjCSAndPName()                   */
/*                                                                      */
/*      Convert a parameters to ESRI style name                         */
/************************************************************************/

int RemapPValuesBasedOnProjCSAndPName( OGRSpatialReference* pOgr, const char* pszProgCSName, 
                                                                      char **mappingTable )
{
  long ret = 0;
  OGR_SRSNode *poPROJCS = pOgr->GetAttrNode( "PROJCS" );
  if(!poPROJCS)
    return ret;
  for( int i = 0; mappingTable[i] != NULL; i += 4 )
  {
    while( mappingTable[i] != NULL && EQUAL(pszProgCSName, mappingTable[i]) )
    {
      OGR_SRSNode *poParm;
      const char* pszParamName = mappingTable[i+1];
      const char* pszParamValue = mappingTable[i+2];
      for( int iChild = 0; iChild < poPROJCS->GetChildCount(); iChild++ )
      {
          poParm = poPROJCS->GetChild( iChild );

          if( EQUAL(poParm->GetValue(),"PARAMETER") 
              && poParm->GetChildCount() == 2 
              && EQUAL(poParm->GetChild(0)->GetValue(),pszParamName) 
              && EQUALN(poParm->GetChild(1)->GetValue(),pszParamValue, strlen(pszParamValue) ) )
          {
              poParm->GetChild(1)->SetValue( mappingTable[i+3] );
              break;
          }
      }
      ret ++;
      i += 4;
    }
    if (ret > 0)
      break;
  }
  return ret;
}

/************************************************************************/
/*                  RemapPNamesBasedOnProjCSAndPName()                  */
/*                                                                      */
/*      Convert a parameters to ESRI style name                         */
/************************************************************************/

int RemapPNamesBasedOnProjCSAndPName( OGRSpatialReference* pOgr, const char* pszProgCSName, 
                                                                     char **mappingTable )
{
  long ret = 0;
  OGR_SRSNode *poPROJCS = pOgr->GetAttrNode( "PROJCS" );
  if(!poPROJCS)
    return ret;
  for( int i = 0; mappingTable[i] != NULL; i += 3 )
  {
    while( mappingTable[i] != NULL && EQUAL(pszProgCSName, mappingTable[i]) )
    {
      OGR_SRSNode *poParm;
      const char* pszParamName = mappingTable[i+1];
      for( int iChild = 0; iChild < poPROJCS->GetChildCount(); iChild++ )
      {
          poParm = poPROJCS->GetChild( iChild );

          if( EQUAL(poParm->GetValue(),"PARAMETER") 
              && poParm->GetChildCount() == 2 
              && EQUAL(poParm->GetChild(0)->GetValue(),pszParamName) )
          {
              poParm->GetChild(0)->SetValue( mappingTable[i+2] );
              break;
          }
      }
      ret ++;
      i += 3;
    }
    if (ret > 0)
      break;
  }
  return ret;
}

/************************************************************************/
/*                        DeleteParamBasedOnPrjName                     */
/*                                                                      */
/*      Delete non-ESRI parameters                                      */
/************************************************************************/
int DeleteParamBasedOnPrjName( OGRSpatialReference* pOgr, const char* pszProjectionName, 
                                                             char **mappingTable )
{
  long index = -1, ret = -1;
  for( int i = 0; mappingTable[i] != NULL; i += 2 )
  {
    if( EQUAL(pszProjectionName, mappingTable[i]) )
    {
      OGR_SRSNode *poPROJCS = pOgr->GetAttrNode( "PROJCS" );
      OGR_SRSNode *poParm;
      const char* pszParamName = mappingTable[i+1];
      index = -1;
      for( int iChild = 0; iChild < poPROJCS->GetChildCount(); iChild++ )
      {
        poParm = poPROJCS->GetChild( iChild );

        if( EQUAL(poParm->GetValue(),"PARAMETER") 
            && poParm->GetChildCount() == 2 
            && EQUAL(poParm->GetChild(0)->GetValue(),pszParamName) )
        {
          index = iChild;
          break;
        }
      }
      if(index >= 0)
      {
        poPROJCS->DestroyChild( index );
        ret ++;
      }
    }
  }
  return ret;
}
/************************************************************************/
/*                          AddParamBasedOnPrjName()                    */
/*                                                                      */
/*      Add ESRI style parameters                                       */
/************************************************************************/
int AddParamBasedOnPrjName( OGRSpatialReference* pOgr, const char* pszProjectionName, 
                                                          char **mappingTable )
{
  long index = -1, ret = -1;
  OGR_SRSNode *poPROJCS = pOgr->GetAttrNode( "PROJCS" );
  for( int i = 0; mappingTable[i] != NULL; i += 3 )
  {
    if( EQUAL(pszProjectionName, mappingTable[i]) )
    {
      OGR_SRSNode *poParm;
      int exist = 0;
      for( int iChild = 0; iChild < poPROJCS->GetChildCount(); iChild++ )
      {
        poParm = poPROJCS->GetChild( iChild );

        if( EQUAL(poParm->GetValue(),"PARAMETER") 
            && poParm->GetChildCount() == 2 
            && EQUAL(poParm->GetChild(0)->GetValue(),mappingTable[i+1]) )
        {
          exist = 1;
          break;
        }
      }
      if(!exist)
      {
        poParm = new OGR_SRSNode( "PARAMETER" );
        poParm->AddChild( new OGR_SRSNode( mappingTable[i+1] ) );
        poParm->AddChild( new OGR_SRSNode( mappingTable[i+2] ) );
        poPROJCS->AddChild( poParm );
        ret ++;
      }
    }
  }
  return ret;
}

/************************************************************************/
/*                                    AddParam()                        */
/*                                                                      */
/*      Add ESRI style parameters                                       */
/************************************************************************/
int AddParam( OGRSpatialReference* pOgr, const char* paraName, const char* paraVal)
{
  long index = -1, ret = 0;
  OGR_SRSNode *poPROJCS = pOgr->GetAttrNode( "PROJCS" );
  OGR_SRSNode *poParm;
  int exist = 0;
  for( int iChild = 0; iChild < poPROJCS->GetChildCount(); iChild++ )
  {
    poParm = poPROJCS->GetChild( iChild );

    if( EQUAL(poParm->GetValue(),"PARAMETER") 
        && poParm->GetChildCount() == 2 
        && EQUAL(poParm->GetChild(0)->GetValue(),paraName) )
    {
      exist = 1;
      break;
    }
  }
  if(!exist)
  {
    poParm = new OGR_SRSNode( "PARAMETER" );
    poParm->AddChild( new OGR_SRSNode( paraName ) );
    poParm->AddChild( new OGR_SRSNode( paraVal ) );
    poPROJCS->AddChild( poParm );
    ret = 1;
  }
  return ret;
}

/************************************************************************/
/*                           UnknowNameHandle()                         */
/*                                                                      */
/*      Convert names to ESRI style                                     */
/************************************************************************/
void UnknowNameHandle( OGRSpatialReference* pOgr )
{
    static const char *apszUnknownMapping[] = { 
          "Unknown", "Unnamed",
          NULL, NULL 
    };

    pOgr->GetRoot()->applyRemapper( "PROJCS", 
                              (char **)apszUnknownMapping+1,
                              (char **)apszUnknownMapping+0, 2 );
    pOgr->GetRoot()->applyRemapper( "GEOGCS", 
                              (char **)apszUnknownMapping+1,
                              (char **)apszUnknownMapping+0, 2 );
    pOgr->GetRoot()->applyRemapper( "DATUM", 
                              (char **)apszUnknownMapping+1,
                              (char **)apszUnknownMapping+0, 2 );
    pOgr->GetRoot()->applyRemapper( "SPHEROID", 
                              (char **)apszUnknownMapping+1,
                              (char **)apszUnknownMapping+0, 2 );
    pOgr->GetRoot()->applyRemapper( "PRIMEM", 
                              (char **)apszUnknownMapping+1,
                              (char **)apszUnknownMapping+0, 2 );
    return;
}

/************************************************************************/
/*                           RemapProjCSName()                          */
/*                                                                      */
/*      Convert names to ESRI style                                     */
/************************************************************************/
void RemapProjCSName( OGRSpatialReference* pOgr, char *pszUTMPrefix )
{
  OGR_SRSNode *poProjCS          = NULL;
  OGR_SRSNode *poProjCSNodeChild = NULL;
  const char *pszProjCSName      = NULL;

  if( (poProjCS = pOgr->GetAttrNode( "PROJCS" )) )
    poProjCSNodeChild = poProjCS->GetChild(0);
  if( !poProjCSNodeChild )
    return;
  pszProjCSName = poProjCSNodeChild->GetValue();
  if(!pszProjCSName )
    return;

/* -------------------------------------------------------------------- */
/*      Prepare very specific PROJCS names for UTM coordinate           */
/*      systems.                                                        */
/* -------------------------------------------------------------------- */
  int bNorth = 0;
  int nZone  = 0;

  /* get zone from name first */
  if( EQUALN(pszProjCSName, "UTM Zone ", 9) )
  {
    nZone = atoi(pszProjCSName+9);
    if( strstr(pszProjCSName, "North") )
      bNorth = 1;

    /* if can not get from the name, from the parameters */
    if( nZone <= 0 ) 
      nZone = pOgr->GetUTMZone( &bNorth );

    if( nZone > 0 && pszUTMPrefix )
    {
      char szUTMName[128];
      if( bNorth )
          sprintf( szUTMName, "%s_UTM_Zone_%dN", pszUTMPrefix, nZone );
      else
          sprintf( szUTMName, "%s_UTM_Zone_%dS", pszUTMPrefix, nZone );
              
      if( poProjCSNodeChild )
          poProjCSNodeChild->SetValue( szUTMName );
    }
  }
  else if( EQUAL(pszProjCSName,"unnamed")
        || EQUAL(pszProjCSName,"unknown")
        || EQUAL(pszProjCSName,"") )
  {
    if( pOgr->GetAttrValue( "PROJECTION", 0 ) != NULL )
    {
      pszProjCSName = pOgr->GetAttrValue( "PROJECTION", 0 );
      poProjCSNodeChild->SetValue( pszProjCSName );
    }
  }

  /* re-mapping for some misnamed names */
  pszProjCSName = poProjCSNodeChild->GetValue();
  if( pszProjCSName && strlen(pszProjCSName) > 0 )
  {
    char *pszNewValue = CPLStrdup(pszProjCSName);
    MorphNameToESRI( &pszNewValue );
    poProjCSNodeChild->SetValue( pszNewValue );
    CPLFree( pszNewValue );
    pszProjCSName = poProjCSNodeChild->GetValue();
    int done = 0;
    if( EQUALN( pszProjCSName, "GDM_2000", 8 ) 
     || EQUALN( pszProjCSName, "DGN_1995", 8 ) )
      done = RemapNameBasedOnKeyName( pOgr, pszProjCSName, "PROJCS", (char**)apszProjCSNameMapping);
    if( !done
     && ( EQUALN( pszProjCSName, "WGS_1984_UTM", 12 )
       || EQUALN( pszProjCSName, "NAD_1927_UTM", 12 )
       || EQUALN( pszProjCSName, "NAD_1983_UTM", 12 ) )
     && ( strstr( pszProjCSName, "14N" )
       || strstr( pszProjCSName, "15N" )
       || strstr( pszProjCSName, "16N" )
       || strstr( pszProjCSName, "17N" ) ) )
    {
      const char* lUnitName = pOgr->GetAttrValue( "UNIT", 0 );
      RemapNamesBasedOnTwo( pOgr, pszProjCSName, lUnitName, (char**)apszProjCSNameMappingBasedOnUnit, 3, "PROJCS");
    }
  }
  return;
}

/************************************************************************/
/*                           RemapProjection()                          */
/*                                                                      */
/*      Convert names to ESRI style                                     */
/************************************************************************/
void RemapProjection( OGRSpatialReference* pOgr )
{
    const char *pszProjection = pOgr->GetAttrValue("PROJECTION");
    if(!pszProjection)
      return;
/* -------------------------------------------------------------------- */
/*      There is a special case for Hotine Oblique Mercator to split    */
/*      out the case with an angle to rectified grid.  Bug 423          */
/* -------------------------------------------------------------------- */
    
    if( EQUAL(pszProjection,SRS_PT_HOTINE_OBLIQUE_MERCATOR) 
        && (pOgr->GetProjParm(SRS_PP_AZIMUTH, 0.0 ) > 0.0 
        && pOgr->GetProjParm(SRS_PP_AZIMUTH, 0.0 ) < 90.0001) )
    {
        pOgr->SetNode( "PROJCS|PROJECTION", 
                 "Hotine_Oblique_Mercator_Azimuth_Center" );

        /* ideally we should strip out of the rectified_grid_angle */
    }

/* -------------------------------------------------------------------- */
/*      Polar_Stereographic maps to ESRI codes                          */
/*      Stereographic_South_Pole or Stereographic_North_Pole based      */
/*      on latitude.                                                    */
/* -------------------------------------------------------------------- */
    else if( EQUAL(pszProjection,SRS_PT_POLAR_STEREOGRAPHIC) )
    {
        if( pOgr->GetProjParm(SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) < 0.0 )
        {
            pOgr->SetNode( "PROJCS|PROJECTION", 
                     "Stereographic_South_Pole" );
        }
        else
        {
            pOgr->SetNode( "PROJCS|PROJECTION", 
                     "Stereographic_North_Pole" );
        }
    }

/* -------------------------------------------------------------------- */
/*      OBLIQUE_STEREOGRAPHIC maps to ESRI Stereographic                */
/* -------------------------------------------------------------------- */
    else if( EQUAL(pszProjection,SRS_PT_OBLIQUE_STEREOGRAPHIC) )
    {
        pOgr->SetNode( "PROJCS|PROJECTION", "Stereographic" );
    }

/* -------------------------------------------------------------------- */
/*      Translate PROJECTION keywords that are misnamed.                */
/* -------------------------------------------------------------------- */
    pOgr->GetRoot()->applyRemapper( "PROJECTION", 
                              (char **)apszProjMapping+1,
                              (char **)apszProjMapping, 2 );

/* -------------------------------------------------------------------- */
/*      Remap parameters used for Albers and Mercator.                  */
/* -------------------------------------------------------------------- */
    pszProjection = pOgr->GetAttrValue("PROJECTION");
    OGR_SRSNode *poProjCS = pOgr->GetAttrNode( "PROJCS" );
    if(!pszProjection)
      return;
    if( EQUAL(pszProjection,"Albers") )
        pOgr->GetRoot()->applyRemapper( 
            "PARAMETER", (char **)apszAlbersMapping + 1,
            (char **)apszAlbersMapping + 0, 2 );

    else if( EQUAL(pszProjection,SRS_PT_EQUIDISTANT_CONIC) ||
             EQUAL(pszProjection,SRS_PT_LAMBERT_AZIMUTHAL_EQUAL_AREA) ||
             EQUAL(pszProjection,SRS_PT_AZIMUTHAL_EQUIDISTANT) ||
             EQUAL(pszProjection,SRS_PT_SINUSOIDAL) ||
             EQUAL(pszProjection,SRS_PT_ROBINSON) )
        pOgr->GetRoot()->applyRemapper( 
            "PARAMETER", (char **)apszECMapping + 1,
            (char **)apszECMapping + 0, 2 );

    else if( EQUAL(pszProjection,"Mercator") )
        pOgr->GetRoot()->applyRemapper( 
            "PARAMETER",
            (char **)apszMercatorMapping + 1,
            (char **)apszMercatorMapping + 0, 2 );

    else if( EQUALN(pszProjection,"Stereographic_",14)
          && EQUALN(pszProjection+strlen(pszProjection)-5,"_Pole",5) )
        pOgr->GetRoot()->applyRemapper( 
            "PARAMETER", 
            (char **)apszPolarStereographicMapping + 1, 
            (char **)apszPolarStereographicMapping + 0, 2 );

    else if( EQUAL(pszProjection,"Plate_Carree") )
    {
        if(pOgr->FindProjParm( SRS_PP_STANDARD_PARALLEL_1, poProjCS ) < 0)
          pOgr->GetRoot()->applyRemapper( 
            "PARAMETER", 
            (char **)apszPolarStereographicMapping + 1, 
            (char **)apszPolarStereographicMapping + 0, 2 );
    }

    // special for Lambert_Conformal_Conic
    int iChild;
    if(EQUAL(pszProjection,"Lambert_Conformal_Conic"))
    {
      if(pOgr->FindProjParm( SRS_PP_STANDARD_PARALLEL_2, poProjCS ) < 0 )
      {
        int iChild1 = pOgr->FindProjParm( SRS_PP_STANDARD_PARALLEL_1, poProjCS );
        iChild = pOgr->FindProjParm( SRS_PP_LATITUDE_OF_ORIGIN, poProjCS );
        if( iChild >= 0 && iChild1 < 0 )
        {
          const OGR_SRSNode *poParameter = poProjCS->GetChild(iChild);
          if( poParameter )
          {
            OGR_SRSNode *poNewParm = new OGR_SRSNode( "PARAMETER" );
            poNewParm->AddChild( new OGR_SRSNode( "standard_parallel_1" ) );
            poNewParm->AddChild( new OGR_SRSNode( poParameter->GetChild(1)->GetValue() ) );
            poProjCS->AddChild( poNewParm );
          }
        }
      }
      RemapLCCScaleFactor(pOgr);
    }

    // special for Plate_Carree
    if(EQUAL(pszProjection,"Plate_Carree"))
    {
      iChild = pOgr->FindProjParm( SRS_PP_STANDARD_PARALLEL_1, poProjCS );
      if(iChild < 0)
        iChild = pOgr->FindProjParm( SRS_PP_PSEUDO_STD_PARALLEL_1, poProjCS );
        
      if(iChild >= 0)
      {
        const OGR_SRSNode *poParameter = poProjCS->GetChild(iChild);
        if(!EQUAL(poParameter->GetChild(1)->GetValue(), "0.0") && !EQUAL(poParameter->GetChild(1)->GetValue(), "0"))
        {
          pOgr->SetNode( "PROJCS|PROJECTION", "Equidistant_Cylindrical" );
        }
      }
    }
        
    pszProjection = pOgr->GetAttrValue("PROJECTION");
    if(pszProjection) 
    {
      DeleteParamBasedOnPrjName( pOgr, pszProjection, (char **)apszDeleteParametersBasedOnProjection);
      if( EQUAL(pszProjection, "Cassini")
       || EQUAL(pszProjection, "Mercator") )
        AddParamBasedOnPrjName( pOgr, pszProjection, (char **)apszAddParametersBasedOnProjection);
      if( EQUAL(pszProjection, "Cassini")
       || EQUAL(pszProjection, "Transverse_Mercator") 
       || EQUAL(pszProjection, "Lambert_Conformal_Conic") 
       || EQUAL(pszProjection, "Krovak") 
       || EQUAL(pszProjection, "Hotine_Oblique_Mercator_Azimuth_Cente") )
        RemapPValuesBasedOnProjCSAndPName( pOgr, pszProjection, (char **)apszParamValueMapping);
      else if( EQUAL(pszProjection, "Lambert_Azimuthal_Equal_Area")
       || EQUAL(pszProjection, "Miller_Cylindrical") 
       || EQUAL(pszProjection, "Gnomonic") 
       || EQUAL(pszProjection, "Orthographic") 
       || EQUAL(pszProjection, "New_Zealand_Map_Grid") )
        RemapPNamesBasedOnProjCSAndPName( pOgr, pszProjection, (char **)apszParamNameMapping);
    }

    // scale factor check
    if( (iChild = pOgr->FindProjParm( SRS_PP_SCALE_FACTOR, poProjCS )) >= 0 )
    {
        OGR_SRSNode *poParameter = poProjCS->GetChild(iChild);
        if(poParameter)
          poParameter = poParameter->GetChild(1);
        if( poParameter
         && (EQUAL(poParameter->GetValue(), "0.0") || EQUAL(poParameter->GetValue(), "0")) )
          poParameter->SetValue("1.0");
    }
    return;
}

/************************************************************************/
/*                           WellKnownGcsHandle()                         */
/*                                                                      */
/*      Convert names to ESRI style                                     */
/************************************************************************/
void WellKnownGcsHandle( OGRSpatialReference* pOgr, char** pszUTMPrefix )
{
  OGR_SRSNode *poGeogCS = pOgr->GetAttrNode( "GEOGCS" );
  if( poGeogCS != NULL )
  {
        const char *pszGeogCSName = poGeogCS->GetChild(0)->GetValue();
        const char *pszAuthName = pOgr->GetAuthorityName("GEOGCS");
        int nGCSCode = -1;
        
        if( pszAuthName != NULL && EQUAL(pszAuthName,"EPSG") )
            nGCSCode = atoi(pOgr->GetAuthorityCode("GEOGCS"));

        if( nGCSCode == 4326 
            || EQUAL(pszGeogCSName,"WGS84") 
            || EQUAL(pszGeogCSName,"WGS 84") 
            || EQUAL(pszGeogCSName,"GCS_WGS84") 
            || EQUAL(pszGeogCSName,"GCS_WGS_84") )
        {
            poGeogCS->GetChild(0)->SetValue( "GCS_WGS_1984" );
            *pszUTMPrefix = CPLStrdup("WGS_1984");
        }
        else if( nGCSCode == 4267
                 || EQUAL(pszGeogCSName,"NAD27") 
                 || EQUAL(pszGeogCSName,"NAD 27") 
                 || EQUAL(pszGeogCSName,"GCS_NAD27") 
                 || EQUAL(pszGeogCSName,"GCS_NAD_27") )
        {
            poGeogCS->GetChild(0)->SetValue( "GCS_North_American_1927" );
            *pszUTMPrefix = CPLStrdup("NAD_1927");
        }
        else if( nGCSCode == 4269
                 || EQUAL(pszGeogCSName,"NAD83") 
                 || EQUAL(pszGeogCSName,"NAD 83") 
                 || EQUAL(pszGeogCSName,"GCS_NAD83") 
                 || EQUAL(pszGeogCSName,"GCS_NAD_83") )
        {
            poGeogCS->GetChild(0)->SetValue( "GCS_North_American_1983" );
            *pszUTMPrefix = CPLStrdup("NAD_1983");
        }
  }
  return;
}

/************************************************************************/
/*                            RemapDatumName()                          */
/*                                                                      */
/*      Convert names to ESRI style                                     */
/************************************************************************/
void RemapDatumName( OGRSpatialReference* pOgr )
{
    InitDatumMappingTable();

    pOgr->GetRoot()->applyRemapper( "DATUM", 
                              papszDatumMapping+2, papszDatumMapping+1, 3 );
    const char *pszDatumName = pOgr->GetAttrValue( "DATUM", 0 );
    if(!pszDatumName)
      return;
    char *pszNewValue = CPLStrdup(pszDatumName);
    MorphNameToESRI( &pszNewValue );
    SetNewName( pOgr, "DATUM", pszNewValue );
    CPLFree( pszNewValue );
    const char* pszDName = pOgr->GetAttrValue( "DATUM", 0 );
    if( !EQUALN(pszDName, "D_",2) )
    {
        char *pszNewValue;

        pszNewValue = (char *) CPLMalloc(strlen(pszDName)+3);
        strcpy( pszNewValue, "D_" );
        strcat( pszNewValue, pszDName );
        SetNewName( pOgr, "DATUM", pszNewValue );
        CPLFree( pszNewValue );
        pszDName = pOgr->GetAttrValue( "DATUM", 0 );
    }

    if( EQUAL(pszDName+2, "ETRS_1989")
     || EQUAL(pszDName+2, "Kartasto_Koordinaati_Jarjestelma_1966")
     || EQUAL(pszDName+2, "Not_specified_based_on_Authalic_Sphere") )
    {
      const char *pszGcsName = pOgr->GetAttrValue("GEOGCS");
      RemapNamesBasedOnTwo( pOgr, pszDName+2, pszGcsName, 
                (char**)apszDatumNameMappingBasedOnGcs, 3, "DATUM" );
    }
    else
      RemapNameBasedOnKeyName( pOgr, pszDName+2, "DATUM", (char**)apszDatumNameMapping );
    return;
}

/************************************************************************/
/*                            ReMapUnitName()                           */
/*                                                                      */
/*      Convert names to ESRI style                                     */
/************************************************************************/
void ReMapUnitName( OGRSpatialReference* pOgr )
{
  pOgr->GetRoot()->applyRemapper( "UNIT", 
                              (char **)apszUnitMapping+1,
                              (char **)apszUnitMapping, 2 );

/* -------------------------------------------------------------------- */
/*      reset constants for decimal degrees to the exact string ESRI    */
/*      expects when encountered to ensure a matchup.                   */
/* -------------------------------------------------------------------- */
  OGR_SRSNode *poUnit = pOgr->GetAttrNode( "GEOGCS|UNIT" );
    
  if( poUnit != NULL && poUnit->GetChildCount() >= 2 
      && ABS(pOgr->GetAngularUnits()-0.0174532925199433) < 0.00000000001 )
  {
    poUnit->GetChild(0)->SetValue("Degree");
    poUnit->GetChild(1)->SetValue("0.017453292519943295");
  }

/* -------------------------------------------------------------------- */
/*      Make sure we reproduce US Feet exactly too.                     */
/* -------------------------------------------------------------------- */
  poUnit = pOgr->GetAttrNode( "PROJCS|UNIT" );
    
  if( poUnit != NULL && poUnit->GetChildCount() >= 2 
      && ABS(pOgr->GetLinearUnits()- 0.30480060960121924) < 0.000000000000001)
  {
    poUnit->GetChild(0)->SetValue("Foot_US");
    poUnit->GetChild(1)->SetValue("0.30480060960121924");
  }
  return;
}

/************************************************************************/
/*                                   RemapGeogCSName()                  */
/*                                                                      */
/*      Convert names to ESRI style                                     */
/************************************************************************/
int RemapGeogCSName( OGRSpatialReference* pOgr, const char *pszGeogCSName )
{
  int ret = -1;
  char *pszNewValue = NULL;
  if(pszGeogCSName && !EQUALN( pszGeogCSName, "GCS_", 4 ) )
  {
    char* newGcsName = (char *) CPLMalloc(strlen(pszGeogCSName) + 5);
    strcpy( newGcsName, "GCS_" );
    strcat(newGcsName, pszGeogCSName);
    SetNewName( pOgr, "GEOGCS", newGcsName );
    CPLFree( newGcsName );
    pszNewValue = CPLStrdup( pOgr->GetAttrValue( "GEOGCS" ) );
  } 
  else
    pszNewValue = CPLStrdup(pszGeogCSName);
  MorphNameToESRI( &pszNewValue );
  SetNewName( pOgr, "GEOGCS", pszNewValue );
  CPLFree( pszNewValue );

  const char* pszGcsName = pOgr->GetAttrValue( "GEOGCS" );
  if( EQUALN( pszGcsName+4, "Voirol", 6 )
   || EQUAL( pszGcsName+4, "NTF" ) )
  {
    const char* pszUnitName = pOgr->GetAttrValue( "GEOGCS|UNIT");
    if(pszUnitName)
      ret = RemapNamesBasedOnTwo( pOgr, pszGcsName+4, pszUnitName, (char**)apszGcsNameMappingBasedOnUnit, 3, "GEOGCS");
  }
  if(ret < 0)
  {
    const char* pszPrimeName = pOgr->GetAttrValue("PRIMEM");
    if(pszPrimeName)
      ret = RemapNamesBasedOnTwo( pOgr, pszGcsName+4, pszPrimeName, (char**)apszGcsNameMappingBasedPrime, 3, "GEOGCS");
    if(ret < 0)
    {
      ret = RemapNameBasedOnKeyName( pOgr, pszGcsName+4, "GEOGCS", (char**)apszGcsNameMapping);
      if(ret < 0)
      {
        const char* pszProjCS = pOgr->GetAttrValue( "PROJCS" );
        if(pszProjCS && strlen(pszProjCS) > 0)
          ret = RemapNamesBasedOnTwo( pOgr, pszProjCS, pszGcsName, (char**)apszGcsNameMappingBasedOnProjCS, 3, "GEOGCS");
      }
    }
  }
  return ret;
}

/************************************************************************/
/*                            RemapSpheroidName()                       */
/*                                                                      */
/*      Convert names to ESRI style                                     */
/************************************************************************/
int RemapSpheroidName(OGRSpatialReference* pOgr, const char *pszSpheroidName)
{
  int ret = -1;
  if(!pszSpheroidName || strlen(pszSpheroidName) == 0)
    return ret;

  char* newName = NULL;
  if( EQUAL(pszSpheroidName, "WGS 84") )
    newName = CPLStrdup( "WGS_1984" );
  else if ( EQUAL(pszSpheroidName, "WGS 72") )
    newName = CPLStrdup( "WGS_1972" );
  else
    newName = CPLStrdup( pszSpheroidName );
  MorphNameToESRI( &newName );
  SetNewName( pOgr, "SPHEROID", newName );
  CPLFree( newName );

  /* special mapping for some spheroids */
  pszSpheroidName = pOgr->GetAttrValue( "SPHEROID", 0 );
  if( EQUAL( pszSpheroidName, "Clarke_1880" )
   || EQUAL( pszSpheroidName, "Krasovsky" )
   || EQUAL( pszSpheroidName, "Modified_Everest" ) )
  {
    const char* pszDatumName = pOgr->GetAttrValue( "DATUM", 0 );
    if(pszDatumName)
      ret = RemapNamesBasedOnTwo( pOgr, pszSpheroidName, pszDatumName, (char**)apszSpheroidNameMappingBasedOnDatum, 3, "SPHEROID");
  }
  if(ret < 0)
    ret = RemapNameBasedOnKeyName( pOgr, pszSpheroidName, "SPHEROID", (char**)apszSpheroidNameMapping);
  return ret;
}

/************************************************************************/
/*                       RemapSpheroidParameters()                      */
/*                                                                      */
/*      Remapping spheroid name and its values                          */
/************************************************************************/
void RemapSpheroidParameters(OGRSpatialReference* pOgr)
{
    OGR_SRSNode *poSpheroid = NULL; 
    OGR_SRSNode *poSpheroidChild = NULL;
    poSpheroid = pOgr->GetAttrNode( "SPHEROID" );
    if( poSpheroid )
        poSpheroidChild = poSpheroid->GetChild(0);
    if(!poSpheroidChild)
      return;

    /* Name mapping */
    const char* pszSpheroidName = poSpheroidChild->GetValue();
    if(pszSpheroidName && strlen(pszSpheroidName) > 0)
    {
      RemapSpheroidName(pOgr, pszSpheroidName);
      pszSpheroidName = poSpheroidChild->GetValue();
    }

    /* inverse flattening mapping */
    poSpheroidChild = poSpheroid->GetChild(2);
    if(!poSpheroidChild)
      return;
    const char * dfValue = poSpheroidChild->GetValue();
    int i, found = 0;
    if( pszSpheroidName 
     && ( EQUAL(pszSpheroidName, "WGS_1984" )
       || EQUAL(pszSpheroidName, "GRS_1980" ) ) )
    {
      for( i = 0; apszInvFlatteningMappingBasedOnSpheroid[i] != NULL; i += 3 )
      {
        if( EQUALN(apszInvFlatteningMappingBasedOnSpheroid[i], dfValue, strlen(apszInvFlatteningMappingBasedOnSpheroid[i]))
         && EQUAL(apszInvFlatteningMappingBasedOnSpheroid[i+1], pszSpheroidName) )
        {
          poSpheroidChild->SetValue( apszInvFlatteningMappingBasedOnSpheroid[i+2] );
          found = 1;
          break;
        }
      }
    }

    if(!found)
    {
      for( i = 0; apszInvFlatteningMapping[i] != NULL; i += 2 )
      {
        if( EQUALN(apszInvFlatteningMapping[i], dfValue, strlen(apszInvFlatteningMapping[i]) ))
        {
          poSpheroidChild->SetValue( apszInvFlatteningMapping[i+1] );
          break;
        }
      }
    }

    // SemiMajor mapping 
    poSpheroidChild = poSpheroid->GetChild(1);
    if(!poSpheroidChild)
      return;
    dfValue = poSpheroidChild->GetValue();
    for( i = 0; apszSemiMajorMapping[i] != NULL; i += 2 )
    {
      if( EQUALN(apszSemiMajorMapping[i], dfValue, strlen(apszSemiMajorMapping[i]) ))
      {
        poSpheroidChild->SetValue( apszSemiMajorMapping[i+1] );
        break;
      }
    }
    return; 
}

/************************************************************************/
/*                       RemapScaleFactor()                             */
/*                                                                      */
/*      For Lambert_Conformal_Conic scale factor mapping                */
/************************************************************************/
int RemapLCCScaleFactor(OGRSpatialReference* pOgr)
{
  int ret = 0;
  const char* pszGCSName = pOgr->GetAttrValue("GEOGCS");
  const char* pszDatumName = pOgr->GetAttrValue("DATUM");
  const char* pszSpheroidName = pOgr->GetAttrValue( "SPHEROID", 0 );
  double dv1 = pOgr->GetProjParm( SRS_PP_STANDARD_PARALLEL_1, 1000.0 );
  double dv2 = pOgr->GetProjParm( SRS_PP_STANDARD_PARALLEL_2, 1000.0 );
  for( int i = 0; apszRemapLCCScaleFactor[i] != NULL; i += 6 )
  {
    if(!EQUAL(pszGCSName, apszRemapLCCScaleFactor[i]))
      continue;
    if(!EQUAL(pszDatumName, apszRemapLCCScaleFactor[i+1]))
      continue;  
    if(!EQUAL(pszSpheroidName, apszRemapLCCScaleFactor[i+2]))
      continue;
    if(dv1 == 1000.0)
      continue;
    if(atof(apszRemapLCCScaleFactor[i+3]) != dv1)
      continue;
    if(dv2 == atof(apszRemapLCCScaleFactor[i+4]))
    {
      AddParam( pOgr, "scale_factor", apszRemapLCCScaleFactor[i+5]);
      DeleteParamBasedOnPrjName( pOgr, "Lambert_Conformal_Conic", (char **)apszConditionalDeleteParameter);
      ret = 1;
      break;
    }
  }
  return ret;
}

