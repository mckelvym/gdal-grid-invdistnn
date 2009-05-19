/******************************************************************************
 * $Id$
 *
 * Project:  GeoTIFF Driver
 * Purpose:  Implements translation between GeoTIFF normalized projection
 *           definitions and OpenGIS WKT SRS format.  This code is
 *           deliberately GDAL free, and it is intended to be moved into
 *           libgeotiff someday if possible.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
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
 ****************************************************************************/

#include "cpl_port.h"
#include "cpl_serv.h"
#include "geo_tiffp.h"
#define CPL_ERROR_H_INCLUDED

#include "geo_normalize.h"
#include "geovalues.h"
#include "ogr_spatialref.h"
#include "gdal.h"
#include "xtiffio.h"
#include "cpl_multiproc.h"

CPL_CVSID("$Id$");

CPL_C_START
void GTiffOneTimeInit();
int CPL_DLL VSIFCloseL( FILE * );
int CPL_DLL VSIUnlink( const char * );
FILE CPL_DLL *VSIFileFromMemBuffer( const char *pszFilename, 
                                    GByte *pabyData, 
                                    GUIntBig nDataLength,
                                    int bTakeOwnership );
GByte CPL_DLL *VSIGetMemFileBuffer( const char *pszFilename, 
                                    GUIntBig *pnDataLength, 
                                    int bUnlinkAndSeize );

char CPL_DLL *  GTIFGetOGISDefn( GTIF *, GTIFDefn * );
int  CPL_DLL   GTIFSetFromOGISDefn( GTIF *, const char * );

CPLErr CPL_DLL GTIFMemBufFromWkt( const char *pszWKT, 
                                  const double *padfGeoTransform,
                                  int nGCPCount, const GDAL_GCP *pasGCPList,
                                  int *pnSize, unsigned char **ppabyBuffer );
CPLErr CPL_DLL GTIFWktFromMemBuf( int nSize, unsigned char *pabyBuffer, 
                          char **ppszWKT, double *padfGeoTransform,
                          int *pnGCPCount, GDAL_GCP **ppasGCPList );
CPL_C_END

TIFF* VSI_TIFFOpen(const char* name, const char* mode);

static const char *papszDatumEquiv[] =
{
    "Militar_Geographische_Institut",
    "Militar_Geographische_Institute",
    "World_Geodetic_System_1984",
    "WGS_1984",
    "WGS_72_Transit_Broadcast_Ephemeris",
    "WGS_1972_Transit_Broadcast_Ephemeris",
    "World_Geodetic_System_1972",
    "WGS_1972",
    "European_Terrestrial_Reference_System_89",
    "European_Reference_System_1989",
    NULL
};

// older libgeotiff's won't list this.
#ifndef CT_CylindricalEqualArea
# define CT_CylindricalEqualArea 28
#endif

// to support SR written in different citation 
#define nCitationNameTypes 9
typedef enum 
{
  CitPcsName = 0,
  CitPrjName = 1,
  CitNadName = 2,
  CitLUnitsName = 3,
  CitGcsName = 4,
  CitDatumName = 5,
  CitEllipsoidName = 6,
  CitPrimemName = 7,
  CitAUnitsName = 8
} CitationNameType;

static const char *apszUnitMap[] = {
    "meters", "1.0",
    "meter", "1.0",
    "m", "1.0",
    "centimeters", "0.01",
    "centimeter", "0.01",
    "cm", "0.01", 
    "millimeters", "0.001",
    "millimeter", "0.001",
    "mm", "0.001",
    "kilometers", "1000.0",
    "kilometer", "1000.0",
    "km", "1000.0",
    "us_survey_feet", "0.3048006096012192",
    "us_survey_foot", "0.3048006096012192",
    "feet", "0.3048006096012192", 
    "foot", "0.3048006096012192",
    "ft", "0.3048006096012192",
    "international_feet", "0.3048",
    "international_foot", "0.3048",
    "inches", "0.0254000508001",
    "inch", "0.0254000508001",
    "in", "0.0254000508001",
    "yards", "0.9144",
    "yard", "0.9144",
    "yd", "0.9144",
    "miles", "1304.544",
    "mile", "1304.544",
    "mi", "1304.544",
    "modified_american_feet", "0.3048122530",
    "modified_american_foot", "0.3048122530",
    "clarke_feet", "0.3047972651",
    "clarke_foot", "0.3047972651",
    "indian_feet", "0.3047995142",
    "indian_foot", "0.3047995142",
    "Yard_Indian", "0.9143985307444408", 
    "Foot_Clarke", "0.304797265",
    "Foot_Gold_Coast", "0.3047997101815088",
    "Link_Clarke", "0.2011661949", 
    "Yard_Sears", "0.9143984146160287", 
    "50_Kilometers", "50000.0", 
    "150_Kilometers", "150000.0", 
    NULL, NULL
};

OGRBoolean CheckCitationKeyForStatePlaneUTM(GTIF* hGTIF, GTIFDefn* psDefn, OGRSpatialReference* poSRS, OGRBoolean* pLinearUnitIsSet);
char* ImagineCitationTranslation(char* psCitation, geokey_t keyID);
char** CitationStringParse(char* psCitation, geokey_t keyID);
void SetLinearUnitCitation(GTIF* psGTIF, char* pszLinearUOMName);
void SetGeogCSCitation(GTIF * psGTIF, OGRSpatialReference *poSRS, char* angUnitName, int nDatum, short nSpheroid);
OGRBoolean SetCitationToSRS(GTIF* hGTIF, char* szCTString, geokey_t geoKey, 
              OGRSpatialReference* poSRS, OGRBoolean* pLinearUnitIsSet);
void GetGeogCSFromCitation(char* szGCSName, geokey_t geoKey, 
                          char	**ppszGeogName,
                          char	**ppszDatumName,
                          char	**ppszPMName,
                          char	**ppszSpheroidName,
                          char	**ppszAngularUnits);
void CheckUTM( GTIFDefn * psDefn, char * pszCtString );

/************************************************************************/
/*                          WKTMassageDatum()                           */
/*                                                                      */
/*      Massage an EPSG datum name into WMT format.  Also transform     */
/*      specific exception cases into WKT versions.                     */
/************************************************************************/

static void WKTMassageDatum( char ** ppszDatum )

{
    int		i, j;
    char	*pszDatum;

/* -------------------------------------------------------------------- */
/*      First copy string and allocate with our CPLStrdup() to so we    */
/*      know when we are done this function we will have a CPL          */
/*      string, not a GTIF one.                                         */
/* -------------------------------------------------------------------- */
    pszDatum = CPLStrdup(*ppszDatum);
    GTIFFreeMemory( *ppszDatum );
    *ppszDatum = pszDatum;

/* -------------------------------------------------------------------- */
/*      Translate non-alphanumeric values to underscores.               */
/* -------------------------------------------------------------------- */
    for( i = 0; pszDatum[i] != '\0'; i++ )
    {
        if( pszDatum[i] != '+'
            && !(pszDatum[i] >= 'A' && pszDatum[i] <= 'Z')
            && !(pszDatum[i] >= 'a' && pszDatum[i] <= 'z')
            && !(pszDatum[i] >= '0' && pszDatum[i] <= '9') )
        {
            pszDatum[i] = '_';
        }
    }

/* -------------------------------------------------------------------- */
/*      Remove repeated and trailing underscores.                       */
/* -------------------------------------------------------------------- */
    for( i = 1, j = 0; pszDatum[i] != '\0'; i++ )
    {
        if( pszDatum[j] == '_' && pszDatum[i] == '_' )
            continue;

        pszDatum[++j] = pszDatum[i];
    }
    if( pszDatum[j] == '_' )
        pszDatum[j] = '\0';
    else
        pszDatum[j+1] = '\0';
    
/* -------------------------------------------------------------------- */
/*      Search for datum equivelences.  Specific massaged names get     */
/*      mapped to OpenGIS specified names.                              */
/* -------------------------------------------------------------------- */
    for( i = 0; papszDatumEquiv[i] != NULL; i += 2 )
    {
        if( EQUAL(*ppszDatum,papszDatumEquiv[i]) )
        {
            CPLFree( *ppszDatum );
            *ppszDatum = CPLStrdup( papszDatumEquiv[i+1] );
            return;
        }
    }
}

/************************************************************************/
/*                      GTIFCleanupImageineNames()                      */
/*                                                                      */
/*      Erdas Imagine sometimes emits big copyright messages, and       */
/*      other stuff into citations.  These can be pretty messy when     */
/*      turned into WKT, so we try to trim and clean the strings        */
/*      somewhat.                                                       */
/************************************************************************/

/* For example:
   GTCitationGeoKey (Ascii,215): "IMAGINE GeoTIFF Support\nCopyright 1991 - 2001 by ERDAS, Inc. All Rights Reserved\n@(#)$RCSfile$ $Revision$ $Date$\nProjection Name = UTM\nUnits = meters\nGeoTIFF Units = meters"

   GeogCitationGeoKey (Ascii,267): "IMAGINE GeoTIFF Support\nCopyright 1991 - 2001 by ERDAS, Inc. All Rights Reserved\n@(#)$RCSfile$ $Revision$ $Date$\nUnable to match Ellipsoid (Datum) to a GeographicTypeGeoKey value\nEllipsoid = Clarke 1866\nDatum = NAD27 (CONUS)"

   PCSCitationGeoKey (Ascii,214): "IMAGINE GeoTIFF Support\nCopyright 1991 - 2001 by ERDAS, Inc. All Rights Reserved\n@(#)$RCSfile$ $Revision$ $Date$\nUTM Zone 10N\nEllipsoid = Clarke 1866\nDatum = NAD27 (CONUS)"
 
*/

static void GTIFCleanupImagineNames( char *pszCitation )

{
    if( strstr(pszCitation,"IMAGINE GeoTIFF") == NULL )
        return;

/* -------------------------------------------------------------------- */
/*      First, we skip past all the copyright, and RCS stuff.  We       */
/*      assume that this will have a "$" at the end of it all.          */
/* -------------------------------------------------------------------- */
    char *pszSkip;
    
    for( pszSkip = pszCitation + strlen(pszCitation) - 1;
         pszSkip != pszCitation && *pszSkip != '$'; 
         pszSkip-- ) {}

    if( *pszSkip == '$' )
        pszSkip++;

    memmove( pszCitation, pszSkip, strlen(pszSkip)+1 );

/* -------------------------------------------------------------------- */
/*      Convert any newlines into spaces, they really gum up the        */
/*      WKT.                                                            */
/* -------------------------------------------------------------------- */
    int i;

    for( i = 0; pszCitation[i] != '\0'; i++ )
    {
        if( pszCitation[i] == '\n' )
            pszCitation[i] = ' ';
    }
}

/************************************************************************/
/*                          GTIFGetOGISDefn()                           */
/************************************************************************/

char *GTIFGetOGISDefn( GTIF *hGTIF, GTIFDefn * psDefn )

{
    OGRSpatialReference	oSRS;

/* -------------------------------------------------------------------- */
/*      Handle non-standard coordinate systems as LOCAL_CS.             */
/* -------------------------------------------------------------------- */
    if( psDefn->Model != ModelTypeProjected 
        && psDefn->Model != ModelTypeGeographic )
    {
        char	*pszWKT;
        char  szPeStr[2400];

        /** check if there is a pe string citation key **/
        if( GTIFKeyGet( hGTIF, PCSCitationGeoKey, szPeStr, 0, sizeof(szPeStr) ) &&
            strstr(szPeStr, "ESRI PE String = " ) )
        {
          pszWKT = CPLStrdup( szPeStr + strlen("ESRI PE String = ") );
          return pszWKT;
        }
        else
        {
          char	*pszUnitsName = NULL;
          char    szPCSName[300];
          int     nKeyCount = 0;
          int     anVersion[3];

          if( hGTIF != NULL )
              GTIFDirectoryInfo( hGTIF, anVersion, &nKeyCount );

          if( nKeyCount > 0 ) // Use LOCAL_CS if we have any geokeys at all.
          {
              // Handle citation.
              strcpy( szPCSName, "unnamed" );
              if( !GTIFKeyGet( hGTIF, GTCitationGeoKey, szPCSName, 
                               0, sizeof(szPCSName) ) )
                  GTIFKeyGet( hGTIF, GeogCitationGeoKey, szPCSName, 
                              0, sizeof(szPCSName) );

              GTIFCleanupImagineNames( szPCSName );
              oSRS.SetLocalCS( szPCSName );

              // Handle units
              GTIFGetUOMLengthInfo( psDefn->UOMLength, &pszUnitsName, NULL );
              
              if( pszUnitsName != NULL && psDefn->UOMLength != KvUserDefined )
              {
                  oSRS.SetLinearUnits( pszUnitsName, psDefn->UOMLengthInMeters );
                  oSRS.SetAuthority( "LOCAL_CS|UNIT", "EPSG", psDefn->UOMLength);
              }
              else
                  oSRS.SetLinearUnits( "unknown", psDefn->UOMLengthInMeters );

              GTIFFreeMemory( pszUnitsName );
          }

          oSRS.exportToWkt( &pszWKT );

          return pszWKT;
       }
    }
    
/* -------------------------------------------------------------------- */
/*      If this is a projected SRS we set the PROJCS keyword first      */
/*      to ensure that the GEOGCS will be a child.                      */
/* -------------------------------------------------------------------- */
    OGRBoolean linearUnitIsSet = FALSE;
    if( psDefn->Model == ModelTypeProjected )
    {
        oSRS.SetNode( "PROJCS", "unnamed" );
        psDefn->PCS = psDefn->PCS < 0 ? KvUserDefined : psDefn->PCS;
        if( psDefn->PCS != KvUserDefined )
        {
            char    *pszPCSName = NULL;

            GTIFGetPCSInfo( psDefn->PCS, &pszPCSName, NULL, NULL, NULL );
            
            oSRS.SetNode( "PROJCS", pszPCSName ? pszPCSName : "unnamed" );
            if ( pszPCSName )
                GTIFFreeMemory( pszPCSName );

            oSRS.SetAuthority( "PROJCS", "EPSG", psDefn->PCS );
        }
        
        /* Handle state plane and UTM in citation key */
        if( CheckCitationKeyForStatePlaneUTM(hGTIF, psDefn, &oSRS, &linearUnitIsSet) )
        {
          char	*pszWKT;
          oSRS.morphFromESRI();
          oSRS.FixupOrdering();
          if( oSRS.exportToWkt( &pszWKT ) == OGRERR_NONE )
              return pszWKT;
        }

        // check GTCitationGeoKey if it exists
        char  szCTString[512];
        szCTString[0] = '\0';
        if( hGTIF && GTIFKeyGet( hGTIF, GTCitationGeoKey, szCTString, 0, sizeof(szCTString) ) )
          SetCitationToSRS(hGTIF, szCTString, GTCitationGeoKey, &oSRS, &linearUnitIsSet);
    }
    
/* ==================================================================== */
/*      Setup the GeogCS                                                */
/* ==================================================================== */
    char	*pszGeogName = NULL;
    char	*pszDatumName = NULL;
    char	*pszPMName = NULL;
    char	*pszSpheroidName = NULL;
    char	*pszAngularUnits = NULL;
    double	dfInvFlattening, dfSemiMajor;
    char  szGCSName[512];
    OGRBoolean aUnitGot = FALSE;
    
    if( !GTIFGetGCSInfo( psDefn->GCS, &pszGeogName, NULL, NULL, NULL )
        && hGTIF != NULL 
        && GTIFKeyGet( hGTIF, GeogCitationGeoKey, szGCSName, 0, 
                       sizeof(szGCSName)) )
      GetGeogCSFromCitation(szGCSName, GeogCitationGeoKey, 
                          &pszGeogName, &pszDatumName,
                          &pszPMName, &pszSpheroidName,
                          &pszAngularUnits);

    if( !pszDatumName )
      GTIFGetDatumInfo( psDefn->Datum, &pszDatumName, NULL );
    if( !pszSpheroidName )
      GTIFGetEllipsoidInfo( psDefn->Ellipsoid, &pszSpheroidName, NULL, NULL );
    else
    {
      GTIFKeyGet(hGTIF, GeogSemiMajorAxisGeoKey, &(psDefn->SemiMajor), 0, 1 );
      GTIFKeyGet(hGTIF, GeogInvFlatteningGeoKey, &(psDefn->invFlattening), 0, 1 );
    }
    if( !pszPMName )
      GTIFGetPMInfo( psDefn->PM, &pszPMName, NULL );
    else
      GTIFKeyGet(hGTIF, GeogPrimeMeridianLongGeoKey, &(psDefn->PMLongToGreenwich), 0, 1 );
    
    if( !pszAngularUnits )
    {
      GTIFGetUOMAngleInfo( psDefn->UOMAngle, &pszAngularUnits, NULL );
      if( pszAngularUnits == NULL )
          pszAngularUnits = CPLStrdup("unknown");
    }
    else
    {
      GTIFKeyGet(hGTIF, GeogAngularUnitSizeGeoKey, &(psDefn->UOMAngleInDegrees), 0, 1 );
      aUnitGot = TRUE;
    }

    if( pszDatumName != NULL )
        WKTMassageDatum( &pszDatumName );

    dfSemiMajor = psDefn->SemiMajor;
    dfInvFlattening = psDefn->invFlattening;
    if( dfSemiMajor == 0.0 )
    {
        pszSpheroidName = CPLStrdup("unretrievable - using WGS84");
        dfSemiMajor = SRS_WGS84_SEMIMAJOR;
        dfInvFlattening = SRS_WGS84_INVFLATTENING;
    }
    else if( dfInvFlattening == 0.0 && ((psDefn->SemiMinor / psDefn->SemiMajor) < 0.99999999999999999
             || (psDefn->SemiMinor / psDefn->SemiMajor) > 1.00000000000000001 ) )
          dfInvFlattening = -1.0 / (psDefn->SemiMinor/psDefn->SemiMajor - 1.0);
    if(!pszSpheroidName || strlen(pszSpheroidName) == 0)
    {
      if(pszGeogName && strlen(pszGeogName) > 0)
      {
        if(pszSpheroidName)
          CPLFree( pszSpheroidName );
        if(strstr(pszGeogName, "GCS_"))
          pszSpheroidName = CPLStrdup(pszGeogName + 4);
        else
          pszSpheroidName = CPLStrdup(pszGeogName);
      }
    }
    if(!pszGeogName || strlen(pszGeogName) == 0)
      pszGeogName = CPLStrdup( pszDatumName );

    if(aUnitGot)
      oSRS.SetGeogCS( pszGeogName, pszDatumName, 
                      pszSpheroidName, dfSemiMajor, dfInvFlattening,
                      pszPMName,
                      psDefn->PMLongToGreenwich / psDefn->UOMAngleInDegrees,
                      pszAngularUnits,
                      psDefn->UOMAngleInDegrees );
    else
      oSRS.SetGeogCS( pszGeogName, pszDatumName, 
                      pszSpheroidName, dfSemiMajor, dfInvFlattening,
                      pszPMName,
                      psDefn->PMLongToGreenwich / psDefn->UOMAngleInDegrees,
                      pszAngularUnits,
                      psDefn->UOMAngleInDegrees * 0.0174532925199433 );

    if( psDefn->GCS != KvUserDefined && psDefn->GCS > 0 )
        oSRS.SetAuthority( "GEOGCS", "EPSG", psDefn->GCS );

    if( psDefn->Datum != KvUserDefined )
        oSRS.SetAuthority( "DATUM", "EPSG", psDefn->Datum );

    if( psDefn->Ellipsoid != KvUserDefined )
        oSRS.SetAuthority( "SPHEROID", "EPSG", psDefn->Ellipsoid );

    CPLFree( pszGeogName );
    CPLFree( pszDatumName );
    GTIFFreeMemory( pszPMName );
    GTIFFreeMemory( pszSpheroidName );
    GTIFFreeMemory( pszAngularUnits );
        
/* ==================================================================== */
/*      Handle projection parameters.                                   */
/* ==================================================================== */
    if( psDefn->Model == ModelTypeProjected )
    {
/* -------------------------------------------------------------------- */
/*      Make a local copy of parms, and convert back into the           */
/*      angular units of the GEOGCS and the linear units of the         */
/*      projection.                                                     */
/* -------------------------------------------------------------------- */
        double		adfParm[10];
        int		i;

        for( i = 0; i < MIN(10,psDefn->nParms); i++ )
            adfParm[i] = psDefn->ProjParm[i];

        for( ; i < 10; i++ )
            adfParm[i] = 0.0;

        if(!aUnitGot)
        {
          adfParm[0] *= psDefn->UOMAngleInDegrees;
          adfParm[1] *= psDefn->UOMAngleInDegrees;
          adfParm[2] *= psDefn->UOMAngleInDegrees;
          adfParm[3] *= psDefn->UOMAngleInDegrees; 
        }
        int unitCode = 0;
        GTIFKeyGet(hGTIF, ProjLinearUnitsGeoKey, &unitCode, 0, 1  );
        if(unitCode != KvUserDefined)
        {
          adfParm[5] /= psDefn->UOMLengthInMeters;
          adfParm[6] /= psDefn->UOMLengthInMeters;
        }
        
/* -------------------------------------------------------------------- */
/*      Translation the fundamental projection.                         */
/* -------------------------------------------------------------------- */
        switch( psDefn->CTProjection )
        {
          case CT_TransverseMercator:
            oSRS.SetTM( adfParm[0], adfParm[1],
                        adfParm[4],
                        adfParm[5], adfParm[6] );
            break;

          case CT_TransvMercator_SouthOriented:
            oSRS.SetTMSO( adfParm[0], adfParm[1],
                          adfParm[4],
                          adfParm[5], adfParm[6] );
            break;

          case CT_Mercator:
            if(adfParm[0] == 0.0)
            {
              double v;
              if( GTIFKeyGet(hGTIF, ProjStdParallel1GeoKey, &v, 0, 1 ) )
                adfParm[0] = v;
            }
            oSRS.SetMercator( adfParm[0], adfParm[1],
                              adfParm[4],
                              adfParm[5], adfParm[6] );
            break;

          case CT_ObliqueStereographic:
            oSRS.SetOS( adfParm[0], adfParm[1],
                        adfParm[4],
                        adfParm[5], adfParm[6] );
            break;

          case CT_Stereographic:
            oSRS.SetOS( adfParm[0], adfParm[1],
                        adfParm[4],
                        adfParm[5], adfParm[6] );
            break;

          case CT_ObliqueMercator: /* hotine */
            oSRS.SetHOM( adfParm[0], adfParm[1],
                         adfParm[2], adfParm[3],
                         adfParm[4],
                         adfParm[5], adfParm[6] );
            break;
        
          case CT_EquidistantConic: 
            oSRS.SetEC( adfParm[0], adfParm[1],
                        adfParm[2], adfParm[3],
                        adfParm[5], adfParm[6] );
            break;
        
          case CT_CassiniSoldner:
            oSRS.SetCS( adfParm[0], adfParm[1],
                        adfParm[5], adfParm[6] );
            break;
        
          case CT_Polyconic:
            oSRS.SetPolyconic( adfParm[0], adfParm[1],
                               adfParm[5], adfParm[6] );
            break;

          case CT_AzimuthalEquidistant:
            oSRS.SetAE( adfParm[0], adfParm[1],
                        adfParm[5], adfParm[6] );
            break;
        
          case CT_MillerCylindrical:
            oSRS.SetMC( adfParm[0], adfParm[1],
                        adfParm[5], adfParm[6] );
            break;
        
          case CT_Equirectangular:
            oSRS.SetEquirectangular( adfParm[0], adfParm[1],
                                     adfParm[5], adfParm[6] );
            break;
        
          case CT_Gnomonic:
            oSRS.SetGnomonic( adfParm[0], adfParm[1],
                              adfParm[5], adfParm[6] );
            break;
        
          case CT_LambertAzimEqualArea:
            oSRS.SetLAEA( adfParm[0], adfParm[1],
                          adfParm[5], adfParm[6] );
            break;
        
          case CT_Orthographic:
            oSRS.SetOrthographic( adfParm[0], adfParm[1],
                                  adfParm[5], adfParm[6] );
            break;
        
          case CT_Robinson:
            oSRS.SetRobinson( adfParm[1],
                              adfParm[5], adfParm[6] );
            break;
        
          case CT_Sinusoidal:
            oSRS.SetSinusoidal( adfParm[1],
                                adfParm[5], adfParm[6] );
            break;
        
          case CT_VanDerGrinten:
            oSRS.SetVDG( adfParm[1],
                         adfParm[5], adfParm[6] );
            break;

          case CT_PolarStereographic:
            oSRS.SetPS( adfParm[0], adfParm[1],
                        adfParm[4],
                        adfParm[5], adfParm[6] );
            break;
        
          case CT_LambertConfConic_2SP:
            oSRS.SetLCC( adfParm[2], adfParm[3],
                         adfParm[0], adfParm[1],
                         adfParm[5], adfParm[6] );
            break;

          case CT_LambertConfConic_1SP:
            oSRS.SetLCC1SP( adfParm[0], adfParm[1],
                            adfParm[4],
                            adfParm[5], adfParm[6] );
            break;
        
          case CT_AlbersEqualArea:
            oSRS.SetACEA( adfParm[0], adfParm[1],
                          adfParm[2], adfParm[3],
                          adfParm[5], adfParm[6] );
            break;

          case CT_NewZealandMapGrid:
            oSRS.SetNZMG( adfParm[0], adfParm[1],
                          adfParm[5], adfParm[6] );
            break;

          case CT_CylindricalEqualArea:
            oSRS.SetCEA( adfParm[0], adfParm[1],
                         adfParm[5], adfParm[6] );
            break;

          default:
            if( oSRS.IsProjected() )
                oSRS.GetRoot()->SetValue( "LOCAL_CS" );
            break;
        }

/* -------------------------------------------------------------------- */
/*      Set projection units.                                           */
/* -------------------------------------------------------------------- */
       if(!linearUnitIsSet)
       {
          char	*pszUnitsName = NULL;
          
          GTIFGetUOMLengthInfo( psDefn->UOMLength, &pszUnitsName, NULL );

          if( pszUnitsName != NULL && psDefn->UOMLength != KvUserDefined )
          {
              oSRS.SetLinearUnits( pszUnitsName, psDefn->UOMLengthInMeters );
              oSRS.SetAuthority( "PROJCS|UNIT", "EPSG", psDefn->UOMLength );
          }
          else
              oSRS.SetLinearUnits( "unknown", psDefn->UOMLengthInMeters );

          GTIFFreeMemory( pszUnitsName );
       }
    }
    
/* -------------------------------------------------------------------- */
/*      Return the WKT serialization of the object.                     */
/* -------------------------------------------------------------------- */
    char	*pszWKT;

    oSRS.FixupOrdering();

    if( oSRS.exportToWkt( &pszWKT ) == OGRERR_NONE )
        return pszWKT;
    else
        return NULL;
}

/************************************************************************/
/*                     OGCDatumName2EPSGDatumCode()                     */
/************************************************************************/

static int OGCDatumName2EPSGDatumCode( const char * pszOGCName )

{
    FILE	*fp;
    char	**papszTokens;
    int		nReturn = KvUserDefined;

/* -------------------------------------------------------------------- */
/*      Do we know it as a built in?                                    */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszOGCName,"NAD27") 
        || EQUAL(pszOGCName,"North_American_Datum_1927") )
        return Datum_North_American_Datum_1927;
    else if( EQUAL(pszOGCName,"NAD83") 
        || EQUAL(pszOGCName,"North_American_Datum_1983") )
        return Datum_North_American_Datum_1983;
    else if( EQUAL(pszOGCName,"WGS84") || EQUAL(pszOGCName,"WGS_1984")
             || EQUAL(pszOGCName,"WGS 84"))
        return Datum_WGS84;
    else if( EQUAL(pszOGCName,"WGS72") || EQUAL(pszOGCName,"WGS_1972") )
        return Datum_WGS72;
    
/* -------------------------------------------------------------------- */
/*      Open the table if possible.                                     */
/* -------------------------------------------------------------------- */
    fp = VSIFOpen( CSVFilename("gdal_datum.csv"), "r" );
    if( fp == NULL )
        fp = VSIFOpen( CSVFilename("datum.csv"), "r" );

    if( fp == NULL )
        return nReturn;

/* -------------------------------------------------------------------- */
/*	Discard the first line with field names.			*/
/* -------------------------------------------------------------------- */
    CSLDestroy( CSVReadParseLine( fp ) );

/* -------------------------------------------------------------------- */
/*      Read lines looking for our datum.                               */
/* -------------------------------------------------------------------- */
    for( papszTokens = CSVReadParseLine( fp );
         CSLCount(papszTokens) > 2 && nReturn == KvUserDefined;
         papszTokens = CSVReadParseLine( fp ) )
    {
        WKTMassageDatum( papszTokens + 1 );

        if( EQUAL(papszTokens[1], pszOGCName) )
            nReturn = atoi(papszTokens[0]);

        CSLDestroy( papszTokens );
    }

    CSLDestroy( papszTokens );
    VSIFClose( fp );

    return nReturn;
}

/************************************************************************/
/*                        GTIFSetFromOGISDefn()                         */
/*                                                                      */
/*      Write GeoTIFF projection tags from an OGC WKT definition.       */
/************************************************************************/

int GTIFSetFromOGISDefn( GTIF * psGTIF, const char *pszOGCWKT )

{
    OGRSpatialReference *poSRS;
    int		nPCS = KvUserDefined;
    OGRErr      eErr;
    OGRBoolean peStrStored = FALSE;    

    GTIFKeySet(psGTIF, GTRasterTypeGeoKey, TYPE_SHORT, 1,
               RasterPixelIsArea);

/* -------------------------------------------------------------------- */
/*      Create an OGRSpatialReference object corresponding to the       */
/*      string.                                                         */
/* -------------------------------------------------------------------- */
    poSRS = new OGRSpatialReference();
    if( poSRS->importFromWkt((char **) &pszOGCWKT) != OGRERR_NONE )
    {
        delete poSRS;
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Get the ellipsoid definition.                                   */
/* -------------------------------------------------------------------- */
    short nSpheroid = KvUserDefined;
    double dfSemiMajor, dfInvFlattening;

    if( poSRS->GetAuthorityName("PROJCS|GEOGCS|DATUM|SPHEROID") != NULL
        && EQUAL(poSRS->GetAuthorityName("PROJCS|GEOGCS|DATUM|SPHEROID"),
                 "EPSG")) 
    {
        nSpheroid = (short)
            atoi(poSRS->GetAuthorityCode("PROJCS|GEOGCS|DATUM|SPHEROID"));
    }
    else if( poSRS->GetAuthorityName("GEOGCS|DATUM|SPHEROID") != NULL
            && EQUAL(poSRS->GetAuthorityName("GEOGCS|DATUM|SPHEROID"),"EPSG")) 
    {
        nSpheroid = (short)
            atoi(poSRS->GetAuthorityCode("GEOGCS|DATUM|SPHEROID"));
    }
    
    dfSemiMajor = poSRS->GetSemiMajor( &eErr );
    dfInvFlattening = poSRS->GetInvFlattening( &eErr );
    if( eErr != OGRERR_NONE )
    {
        dfSemiMajor = 0.0;
        dfInvFlattening = 0.0;
    }
    
/* -------------------------------------------------------------------- */
/*      Get the Datum so we can special case a few PCS codes.           */
/* -------------------------------------------------------------------- */
    int		nDatum = KvUserDefined;

    if( poSRS->GetAuthorityName("PROJCS|GEOGCS|DATUM") != NULL 
        && EQUAL(poSRS->GetAuthorityName("PROJCS|GEOGCS|DATUM"),"EPSG") )
        nDatum = atoi(poSRS->GetAuthorityCode("PROJCS|GEOGCS|DATUM"));
    else if( poSRS->GetAuthorityName("GEOGCS|DATUM") != NULL 
             && EQUAL(poSRS->GetAuthorityName("GEOGCS|DATUM"),"EPSG") )
        nDatum = atoi(poSRS->GetAuthorityCode("GEOGCS|DATUM"));
    else if( poSRS->GetAttrValue("DATUM") != NULL )
        nDatum = OGCDatumName2EPSGDatumCode( poSRS->GetAttrValue("DATUM") );

/* -------------------------------------------------------------------- */
/*      Get the GCS if possible.                                        */
/* -------------------------------------------------------------------- */
    int         nGCS = KvUserDefined;

    if( poSRS->GetAuthorityName("PROJCS|GEOGCS") != NULL 
        && EQUAL(poSRS->GetAuthorityName("PROJCS|GEOGCS"),"EPSG") )
        nGCS = atoi(poSRS->GetAuthorityCode("PROJCS|GEOGCS"));
    else if( poSRS->GetAuthorityName("GEOGCS") != NULL 
             && EQUAL(poSRS->GetAuthorityName("GEOGCS"),"EPSG") )
        nGCS = atoi(poSRS->GetAuthorityCode("GEOGCS"));

    if( nGCS > 32767 )
        nGCS = KvUserDefined;

/* -------------------------------------------------------------------- */
/*      Get the linear units.                                           */
/* -------------------------------------------------------------------- */
    char        *pszLinearUOMName = NULL;
    double	dfLinearUOM = poSRS->GetLinearUnits( &pszLinearUOMName );
    int         nUOMLengthCode = 9001; /* meters */

    if( (pszLinearUOMName != NULL
         && EQUAL(pszLinearUOMName,SRS_UL_FOOT))
        || dfLinearUOM == atof(SRS_UL_FOOT_CONV) )
        nUOMLengthCode = 9002; /* international foot */
    else if( (pszLinearUOMName != NULL
         && EQUAL(pszLinearUOMName,SRS_UL_US_FOOT))
             || ABS(dfLinearUOM-atof(SRS_UL_US_FOOT_CONV)) < 0.0000001 )
        nUOMLengthCode = 9003; /* us survey foot */
    else if( dfLinearUOM != 1.0 )
        nUOMLengthCode = KvUserDefined;

/* -------------------------------------------------------------------- */
/*      Get some authority values.                                      */
/* -------------------------------------------------------------------- */
    if( poSRS->GetAuthorityName("PROJCS") != NULL 
        && EQUAL(poSRS->GetAuthorityName("PROJCS"),"EPSG") )
    {
        nPCS = atoi(poSRS->GetAuthorityCode("PROJCS"));
        if( nPCS > 32767 )
            nPCS = KvUserDefined;
    }

/* -------------------------------------------------------------------- */
/*      Handle the projection transformation.                           */
/* -------------------------------------------------------------------- */
    const char *pszProjection = poSRS->GetAttrValue( "PROJECTION" );

    if( nPCS != KvUserDefined )
    {
	      GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
        GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1, nPCS );
    }
    else if( pszProjection == NULL )
    {
        if( poSRS->IsGeographic() )
            GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                       ModelTypeGeographic);
        // otherwise, presumably something like LOCAL_CS.
    }
    else if( EQUAL(pszProjection,SRS_PT_ALBERS_CONIC_EQUAL_AREA) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_AlbersEqualArea );

        GTIFKeySet(psGTIF, ProjStdParallelGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_1, 0.0 ) );

        GTIFKeySet(psGTIF, ProjStdParallel2GeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_2, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_CENTER, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }

    else if( poSRS->GetUTMZone() != 0 )
    {
        int		bNorth, nZone, nProjection;

	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);

        nZone = poSRS->GetUTMZone( &bNorth );

        if( nDatum == Datum_North_American_Datum_1983 && nZone >= 3
            && nZone <= 22 && bNorth && nUOMLengthCode == 9001 )
        {
            nPCS = 26900 + nZone;

            GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1, nPCS );
        }
        else if( nDatum == Datum_North_American_Datum_1927 && nZone >= 3
            && nZone <= 22 && bNorth && nUOMLengthCode == 9001 )
        {
            nPCS = 26700 + nZone;

            GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1, nPCS );
        }
        else if( nDatum == Datum_WGS84 && nUOMLengthCode == 9001 )
        {
            if( bNorth )
                nPCS = 32600 + nZone;
            else
                nPCS = 32700 + nZone;

            GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1, nPCS );
        }
        else
        {
            if( bNorth )
                nProjection = 16000 + nZone;
            else
                nProjection = 16100 + nZone;

        
            GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                       KvUserDefined );
            
            GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1, nProjection );
        }
    }

    else if( EQUAL(pszProjection,SRS_PT_TRANSVERSE_MERCATOR) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_TransverseMercator );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtNatOriginGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_TRANSVERSE_MERCATOR_SOUTH_ORIENTED) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_TransvMercator_SouthOriented );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtNatOriginGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_MERCATOR_2SP) 
             || EQUAL(pszProjection,SRS_PT_MERCATOR_1SP) )

    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_Mercator );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtNatOriginGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_OBLIQUE_STEREOGRAPHIC) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_ObliqueStereographic );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtNatOriginGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_STEREOGRAPHIC) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_Stereographic );

        GTIFKeySet(psGTIF, ProjCenterLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtNatOriginGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_POLAR_STEREOGRAPHIC) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_PolarStereographic );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjStraightVertPoleLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtNatOriginGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_HOTINE_OBLIQUE_MERCATOR) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_ObliqueMercator );

        GTIFKeySet(psGTIF, ProjCenterLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_CENTER, 0.0 ) );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjAzimuthAngleGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_AZIMUTH, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjRectifiedGridAngleGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_RECTIFIED_GRID_ANGLE, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtCenterGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_CASSINI_SOLDNER) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_CassiniSoldner );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_EQUIDISTANT_CONIC) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_EquidistantConic );

        GTIFKeySet(psGTIF, ProjStdParallel1GeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_1, 0.0 ) );

        GTIFKeySet(psGTIF, ProjStdParallel2GeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_2, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_CENTER, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_POLYCONIC) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_Polyconic );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtNatOriginGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_AZIMUTHAL_EQUIDISTANT) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_AzimuthalEquidistant );

        GTIFKeySet(psGTIF, ProjCenterLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_CENTER, 0.0 ) );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_MILLER_CYLINDRICAL) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_MillerCylindrical );

        GTIFKeySet(psGTIF, ProjCenterLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_CENTER, 0.0 ) );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_EQUIRECTANGULAR) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_Equirectangular );

        GTIFKeySet(psGTIF, ProjCenterLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_GNOMONIC) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_Gnomonic );

        GTIFKeySet(psGTIF, ProjCenterLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_LAMBERT_AZIMUTHAL_EQUAL_AREA) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_LambertAzimEqualArea );

        GTIFKeySet(psGTIF, ProjCenterLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_CENTER, 0.0 ) );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_ORTHOGRAPHIC) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_Orthographic );

        GTIFKeySet(psGTIF, ProjCenterLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_NEW_ZEALAND_MAP_GRID) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_NewZealandMapGrid );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_ROBINSON) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_Robinson );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_SINUSOIDAL) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_Sinusoidal );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_VANDERGRINTEN) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_VanDerGrinten );

        GTIFKeySet(psGTIF, ProjCenterLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_ALBERS_CONIC_EQUAL_AREA) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_AlbersEqualArea );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_CENTER, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LONGITUDE_OF_CENTER, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjStdParallel1GeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_1, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjStdParallel2GeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_2, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_LambertConfConic_2SP );

        GTIFKeySet(psGTIF, ProjFalseOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjFalseOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjStdParallel1GeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_1, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjStdParallel2GeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_2, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseOriginEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseOriginNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else if( EQUAL(pszProjection,SRS_PT_LAMBERT_CONFORMAL_CONIC_1SP) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_LambertConfConic_1SP );

        GTIFKeySet(psGTIF, ProjNatOriginLatGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_LATITUDE_OF_ORIGIN, 0.0 ) );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjScaleAtNatOriginGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_SCALE_FACTOR, 1.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }

    else if( EQUAL(pszProjection,SRS_PT_CYLINDRICAL_EQUAL_AREA) )
    {
	GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   ModelTypeProjected);
	GTIFKeySet(psGTIF, ProjectedCSTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
	GTIFKeySet(psGTIF, ProjectionGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );

	GTIFKeySet(psGTIF, ProjCoordTransGeoKey, TYPE_SHORT, 1, 
		   CT_CylindricalEqualArea );

        GTIFKeySet(psGTIF, ProjNatOriginLongGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_CENTRAL_MERIDIAN, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjStdParallel1GeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_STANDARD_PARALLEL_1, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseEastingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_EASTING, 0.0 ) );
        
        GTIFKeySet(psGTIF, ProjFalseNorthingGeoKey, TYPE_DOUBLE, 1,
                   poSRS->GetNormProjParm( SRS_PP_FALSE_NORTHING, 0.0 ) );
    }
    
    else
    {
        /* Anyhing we can't map, we store as an ESRI PE string with a citation key */
        char *pszPEString = NULL;
        poSRS->morphToESRI();
        poSRS->exportToWkt( &pszPEString );
        int peStrLen = strlen(pszPEString);
        if(peStrLen > 0)
        {
          char *outPeStr = new char[peStrLen + strlen("ESRI PE String = ")+1];
          strcpy(outPeStr, "ESRI PE String = ");
          strcat(outPeStr, pszPEString);
          GTIFKeySet( psGTIF, PCSCitationGeoKey, TYPE_ASCII, 0, outPeStr );
          peStrStored = TRUE;
          delete outPeStr;
        }
        if(pszPEString)
          CPLFree( pszPEString );
        GTIFKeySet(psGTIF, GTModelTypeGeoKey, TYPE_SHORT, 1,
                   KvUserDefined );
    }
    
/* -------------------------------------------------------------------- */
/*      Write linear units information.                                 */
/* -------------------------------------------------------------------- */
    if( !poSRS->IsGeographic() )
    {
        GTIFKeySet(psGTIF, ProjLinearUnitsGeoKey, TYPE_SHORT, 1, 
                   nUOMLengthCode );
        if( nUOMLengthCode == KvUserDefined )
            GTIFKeySet( psGTIF, ProjLinearUnitSizeGeoKey, TYPE_DOUBLE, 1, 
                        dfLinearUOM);

        /* if linear units name is available and user defined, store it as citation */
        if(!peStrStored && nUOMLengthCode == KvUserDefined && pszLinearUOMName && strlen(pszLinearUOMName)>0)
          SetLinearUnitCitation(psGTIF, pszLinearUOMName);
    }
    
/* -------------------------------------------------------------------- */
/*      Write angular units.  Always Degrees for now.                   */
/*   Changed to support different angular units                         */
/* -------------------------------------------------------------------- */

    char* angUnitName = NULL;
    double angUnitValue = poSRS->GetAngularUnits(&angUnitName);
    if(EQUAL(angUnitName, "Degree"))
      GTIFKeySet(psGTIF, GeogAngularUnitsGeoKey, TYPE_SHORT, 1, 
               Angular_Degree );
    else if(angUnitName)
    {
      GTIFKeySet(psGTIF, GeogCitationGeoKey, TYPE_ASCII, 0, 
                    angUnitName ); // it may be rewritten if the gcs is userdefined 
      GTIFKeySet(psGTIF, GeogAngularUnitSizeGeoKey, TYPE_DOUBLE, 1, 
               angUnitValue );
    }

/* -------------------------------------------------------------------- */
/*      Try to write a citation from the main coordinate system         */
/*      name.                                                           */
/* -------------------------------------------------------------------- */
    if( poSRS->GetRoot() != NULL
        && poSRS->GetRoot()->GetChild(0) != NULL 
        && (poSRS->IsProjected() || poSRS->IsLocal()) )
    {
        GTIFKeySet( psGTIF, GTCitationGeoKey, TYPE_ASCII, 0, 
                    poSRS->GetRoot()->GetChild(0)->GetValue() );
    }

/* -------------------------------------------------------------------- */
/*      Try to write a GCS citation.                                    */
/* -------------------------------------------------------------------- */
    OGR_SRSNode *poGCS = poSRS->GetAttrNode( "GEOGCS" );

    if( poGCS != NULL && poGCS->GetChild(0) != NULL )
    {
        GTIFKeySet( psGTIF, GeogCitationGeoKey, TYPE_ASCII, 0, 
                    poGCS->GetChild(0)->GetValue() );
    }

/* -------------------------------------------------------------------- */
/*      Try to identify the GCS/datum, scanning the EPSG datum file for */
/*      a match.                                                        */
/* -------------------------------------------------------------------- */
    if( nPCS == KvUserDefined )
    {
        if( nGCS == KvUserDefined )
        {
            if( nDatum == Datum_North_American_Datum_1927 )
                nGCS = GCS_NAD27;
            else if( nDatum == Datum_North_American_Datum_1983 )
                nGCS = GCS_NAD83;
            else if( nDatum == Datum_WGS84 || nDatum == DatumE_WGS84 )
                nGCS = GCS_WGS_84;
        }
            
        if( nGCS != KvUserDefined )
        {
            GTIFKeySet( psGTIF, GeographicTypeGeoKey, TYPE_SHORT,
                        1, nGCS );
        }
        else if( nDatum != KvUserDefined )
        {
            GTIFKeySet( psGTIF, GeographicTypeGeoKey, TYPE_SHORT, 1, 
                        KvUserDefined );
            GTIFKeySet( psGTIF, GeogGeodeticDatumGeoKey, TYPE_SHORT,
                        1, nDatum );
        }
        else if( nSpheroid != KvUserDefined )
        {
            GTIFKeySet( psGTIF, GeographicTypeGeoKey, TYPE_SHORT, 1, 
                        KvUserDefined );
            GTIFKeySet( psGTIF, GeogGeodeticDatumGeoKey, TYPE_SHORT,
                        1, KvUserDefined );
            GTIFKeySet( psGTIF, GeogEllipsoidGeoKey, TYPE_SHORT, 1, 
                        nSpheroid );
        }
        else if( dfSemiMajor != 0.0 )
        {
            GTIFKeySet( psGTIF, GeographicTypeGeoKey, TYPE_SHORT, 1, 
                        KvUserDefined );
            GTIFKeySet( psGTIF, GeogGeodeticDatumGeoKey, TYPE_SHORT,
                        1, KvUserDefined );
            GTIFKeySet( psGTIF, GeogEllipsoidGeoKey, TYPE_SHORT, 1, 
                        KvUserDefined );
            GTIFKeySet( psGTIF, GeogSemiMajorAxisGeoKey, TYPE_DOUBLE, 1,
                        dfSemiMajor );
            if( dfInvFlattening == 0.0 )
                GTIFKeySet( psGTIF, GeogSemiMinorAxisGeoKey, TYPE_DOUBLE, 1,
                            dfSemiMajor );
            else
                GTIFKeySet( psGTIF, GeogInvFlatteningGeoKey, TYPE_DOUBLE, 1,
                            dfInvFlattening );
        }
        else if( poSRS->GetAttrValue("DATUM") != NULL
                 && strstr(poSRS->GetAttrValue("DATUM"),"unknown") == NULL
                 && strstr(poSRS->GetAttrValue("DATUM"),"unnamed") == NULL )
                 
        {
            CPLError( CE_Warning, CPLE_AppDefined,
                      "Couldn't translate `%s' to a GeoTIFF datum.\n",
                      poSRS->GetAttrValue("DATUM") );
        }

      /* Always set InvFlattening if it is avaliable.  */
      /* So that it doesn'tneed to calculate from SemiMinor */
      if( dfInvFlattening != 0.0 )
          GTIFKeySet( psGTIF, GeogInvFlatteningGeoKey, TYPE_DOUBLE, 1,
                      dfInvFlattening );
      /* Always set SemiMajor to keep the precision and in case of editing */
      if( dfSemiMajor != 0.0 )
          GTIFKeySet( psGTIF, GeogSemiMajorAxisGeoKey, TYPE_DOUBLE, 1,
                        dfSemiMajor );

      if( nGCS == KvUserDefined )
        SetGeogCSCitation(psGTIF, poSRS, angUnitName, nDatum, nSpheroid);
    }
 
/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
    delete poSRS;
    return TRUE;
}

/************************************************************************/
/*                         GTIFWktFromMemBuf()                          */
/************************************************************************/

CPLErr GTIFWktFromMemBuf( int nSize, unsigned char *pabyBuffer, 
                          char **ppszWKT, double *padfGeoTransform,
                          int *pnGCPCount, GDAL_GCP **ppasGCPList )

{
    TIFF        *hTIFF;
    GTIF 	*hGTIF;
    GTIFDefn	sGTIFDefn;
    char        szFilename[100];

    sprintf( szFilename, "/vsimem/wkt_from_mem_buf_%ld.tif", 
             (long) CPLGetPID() );

/* -------------------------------------------------------------------- */
/*      Create a memory file from the buffer.                           */
/* -------------------------------------------------------------------- */
    FILE *fp = VSIFileFromMemBuffer( szFilename, pabyBuffer, nSize, FALSE );
    if( fp == NULL )
        return CE_Failure;
    VSIFCloseL( fp );

/* -------------------------------------------------------------------- */
/*      Initialize access to the memory geotiff structure.              */
/* -------------------------------------------------------------------- */
    hTIFF = VSI_TIFFOpen( szFilename, "r" );

    if( hTIFF == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "TIFF/GeoTIFF structure is corrupt." );
        return CE_Failure;
    }
    
/* -------------------------------------------------------------------- */
/*      Get the projection definition.                                  */
/* -------------------------------------------------------------------- */
    hGTIF = GTIFNew(hTIFF);

    if( hGTIF != NULL && GTIFGetDefn( hGTIF, &sGTIFDefn ) )
        *ppszWKT = GTIFGetOGISDefn( hGTIF, &sGTIFDefn );
    else
        *ppszWKT = NULL;
    
    if( hGTIF )
        GTIFFree( hGTIF );

/* -------------------------------------------------------------------- */
/*      Get geotransform or tiepoints.                                  */
/* -------------------------------------------------------------------- */
    double	*padfTiePoints, *padfScale, *padfMatrix;
    int16	nCount;

    padfGeoTransform[0] = 0.0;
    padfGeoTransform[1] = 1.0;
    padfGeoTransform[2] = 0.0;
    padfGeoTransform[3] = 0.0;
    padfGeoTransform[4] = 0.0;
    padfGeoTransform[5] = 1.0;

    *pnGCPCount = 0;
    *ppasGCPList = NULL;
    
    if( TIFFGetField(hTIFF,TIFFTAG_GEOPIXELSCALE,&nCount,&padfScale )
        && nCount >= 2 )
    {
        padfGeoTransform[1] = padfScale[0];
        padfGeoTransform[5] = - ABS(padfScale[1]);

        if( TIFFGetField(hTIFF,TIFFTAG_GEOTIEPOINTS,&nCount,&padfTiePoints )
            && nCount >= 6 )
        {
            padfGeoTransform[0] =
                padfTiePoints[3] - padfTiePoints[0] * padfGeoTransform[1];
            padfGeoTransform[3] =
                padfTiePoints[4] - padfTiePoints[1] * padfGeoTransform[5];
        }
    }

    else if( TIFFGetField(hTIFF,TIFFTAG_GEOTIEPOINTS,&nCount,&padfTiePoints )
            && nCount >= 6 )
    {
        *pnGCPCount = nCount / 6;
        *ppasGCPList = (GDAL_GCP *) CPLCalloc(sizeof(GDAL_GCP),*pnGCPCount);
        
        for( int iGCP = 0; iGCP < *pnGCPCount; iGCP++ )
        {
            char	szID[32];
            GDAL_GCP	*psGCP = *ppasGCPList + iGCP;

            sprintf( szID, "%d", iGCP+1 );
            psGCP->pszId = CPLStrdup( szID );
            psGCP->pszInfo = CPLStrdup("");
            psGCP->dfGCPPixel = padfTiePoints[iGCP*6+0];
            psGCP->dfGCPLine = padfTiePoints[iGCP*6+1];
            psGCP->dfGCPX = padfTiePoints[iGCP*6+3];
            psGCP->dfGCPY = padfTiePoints[iGCP*6+4];
            psGCP->dfGCPZ = padfTiePoints[iGCP*6+5];
        }
    }

    else if( TIFFGetField(hTIFF,TIFFTAG_GEOTRANSMATRIX,&nCount,&padfMatrix ) 
             && nCount == 16 )
    {
        padfGeoTransform[0] = padfMatrix[3];
        padfGeoTransform[1] = padfMatrix[0];
        padfGeoTransform[2] = padfMatrix[1];
        padfGeoTransform[3] = padfMatrix[7];
        padfGeoTransform[4] = padfMatrix[4];
        padfGeoTransform[5] = padfMatrix[5];
    }

/* -------------------------------------------------------------------- */
/*      Cleanup.                                                        */
/* -------------------------------------------------------------------- */
    XTIFFClose( hTIFF );

    VSIUnlink( szFilename );

    if( *ppszWKT == NULL )
        return CE_Failure;
    else
        return CE_None;
}

/************************************************************************/
/*                         GTIFMemBufFromWkt()                          */
/************************************************************************/

CPLErr GTIFMemBufFromWkt( const char *pszWKT, const double *padfGeoTransform,
                          int nGCPCount, const GDAL_GCP *pasGCPList,
                          int *pnSize, unsigned char **ppabyBuffer )

{
    TIFF        *hTIFF;
    GTIF 	*hGTIF;
    char        szFilename[100];

    sprintf( szFilename, "/vsimem/wkt_from_mem_buf_%ld.tif", 
             (long) CPLGetPID() );

/* -------------------------------------------------------------------- */
/*      Initialize access to the memory geotiff structure.              */
/* -------------------------------------------------------------------- */
    hTIFF = VSI_TIFFOpen( szFilename, "w" );

    if( hTIFF == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "TIFF/GeoTIFF structure is corrupt." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Write some minimal set of image parameters.                     */
/* -------------------------------------------------------------------- */
    TIFFSetField( hTIFF, TIFFTAG_IMAGEWIDTH, 1 );
    TIFFSetField( hTIFF, TIFFTAG_IMAGELENGTH, 1 );
    TIFFSetField( hTIFF, TIFFTAG_BITSPERSAMPLE, 8 );
    TIFFSetField( hTIFF, TIFFTAG_SAMPLESPERPIXEL, 1 );
    TIFFSetField( hTIFF, TIFFTAG_ROWSPERSTRIP, 1 );
    TIFFSetField( hTIFF, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
    TIFFSetField( hTIFF, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK );
    
/* -------------------------------------------------------------------- */
/*      Get the projection definition.                                  */
/* -------------------------------------------------------------------- */

    if( pszWKT != NULL )
    {
        hGTIF = GTIFNew(hTIFF);
        GTIFSetFromOGISDefn( hGTIF, pszWKT );
        GTIFWriteKeys( hGTIF );
        GTIFFree( hGTIF );
    }

/* -------------------------------------------------------------------- */
/*      Set the geotransform, or GCPs.                                  */
/* -------------------------------------------------------------------- */
    if( padfGeoTransform[0] != 0.0 || padfGeoTransform[1] != 1.0
        || padfGeoTransform[2] != 0.0 || padfGeoTransform[3] != 0.0
        || padfGeoTransform[4] != 0.0 || ABS(padfGeoTransform[5]) != 1.0 )
    {

        if( padfGeoTransform[2] == 0.0 && padfGeoTransform[4] == 0.0 )
        {
            double	adfPixelScale[3], adfTiePoints[6];

            adfPixelScale[0] = padfGeoTransform[1];
            adfPixelScale[1] = fabs(padfGeoTransform[5]);
            adfPixelScale[2] = 0.0;

            TIFFSetField( hTIFF, TIFFTAG_GEOPIXELSCALE, 3, adfPixelScale );
            
            adfTiePoints[0] = 0.0;
            adfTiePoints[1] = 0.0;
            adfTiePoints[2] = 0.0;
            adfTiePoints[3] = padfGeoTransform[0];
            adfTiePoints[4] = padfGeoTransform[3];
            adfTiePoints[5] = 0.0;
        
            TIFFSetField( hTIFF, TIFFTAG_GEOTIEPOINTS, 6, adfTiePoints );
        }
        else
        {
            double	adfMatrix[16];

            memset(adfMatrix,0,sizeof(double) * 16);

            adfMatrix[0] = padfGeoTransform[1];
            adfMatrix[1] = padfGeoTransform[2];
            adfMatrix[3] = padfGeoTransform[0];
            adfMatrix[4] = padfGeoTransform[4];
            adfMatrix[5] = padfGeoTransform[5];
            adfMatrix[7] = padfGeoTransform[3];
            adfMatrix[15] = 1.0;

            TIFFSetField( hTIFF, TIFFTAG_GEOTRANSMATRIX, 16, adfMatrix );
        }
    }

/* -------------------------------------------------------------------- */
/*      Otherwise write tiepoints if they are available.                */
/* -------------------------------------------------------------------- */
    else if( nGCPCount > 0 )
    {
        double	*padfTiePoints;

        padfTiePoints = (double *) CPLMalloc(6*sizeof(double)*nGCPCount);

        for( int iGCP = 0; iGCP < nGCPCount; iGCP++ )
        {

            padfTiePoints[iGCP*6+0] = pasGCPList[iGCP].dfGCPPixel;
            padfTiePoints[iGCP*6+1] = pasGCPList[iGCP].dfGCPLine;
            padfTiePoints[iGCP*6+2] = 0;
            padfTiePoints[iGCP*6+3] = pasGCPList[iGCP].dfGCPX;
            padfTiePoints[iGCP*6+4] = pasGCPList[iGCP].dfGCPY;
            padfTiePoints[iGCP*6+5] = pasGCPList[iGCP].dfGCPZ;
        }

        TIFFSetField( hTIFF, TIFFTAG_GEOTIEPOINTS, 6*nGCPCount, padfTiePoints);
        CPLFree( padfTiePoints );
    } 

/* -------------------------------------------------------------------- */
/*      Cleanup and return the created memory buffer.                   */
/* -------------------------------------------------------------------- */
    GByte bySmallImage = 0;

    TIFFWriteEncodedStrip( hTIFF, 0, (char *) &bySmallImage, 1 );
    TIFFWriteCheck( hTIFF, TIFFIsTiled(hTIFF), "GTIFMemBufFromWkt");
    TIFFWriteDirectory( hTIFF );

    XTIFFClose( hTIFF );

/* -------------------------------------------------------------------- */
/*      Read back from the memory buffer.  It would be preferrable      */
/*      to be able to "steal" the memory buffer, but there isn't        */
/*      currently any support for this.                                 */
/* -------------------------------------------------------------------- */
    GUIntBig nBigLength;

    *ppabyBuffer = VSIGetMemFileBuffer( szFilename, &nBigLength, TRUE );
    *pnSize = (int) nBigLength;

    return CE_None;
}

/************************************************************************/
/*               CheckCitationKeyForStatePlaneUTM()                     */
/*                                                                      */
/*      Handle state plane and UTM in citation key                      */
/************************************************************************/
OGRBoolean CheckCitationKeyForStatePlaneUTM(GTIF* hGTIF, GTIFDefn* psDefn, OGRSpatialReference* poSRS, OGRBoolean* pLinearUnitIsSet)
{
  if( !hGTIF || !psDefn || !poSRS )
    return FALSE;
  char  szCTString[512];
  szCTString[0] = '\0';

  /* Check units */
  char units[32];
  strcpy(units, "meters"); 

  OGRBoolean hasUnits = FALSE;
  if( GTIFKeyGet( hGTIF, GTCitationGeoKey, szCTString, 0, sizeof(szCTString) ) )
  {
    if( strstr( szCTString, "international_feet")
     || strstr(szCTString, "feet") || strstr(szCTString, "foot") 
     || strstr(szCTString, "meter") )
      hasUnits = TRUE;
  }
  if( !hasUnits )
  {
    char	*pszUnitsName = NULL;
    GTIFGetUOMLengthInfo( psDefn->UOMLength, &pszUnitsName, NULL );
    if( pszUnitsName && strlen(pszUnitsName) > 0 )
    {
      strcpy( szCTString, pszUnitsName );
      GTIFFreeMemory( pszUnitsName );
      hasUnits = TRUE;
    }
  }
  if(hasUnits)
  {
    if(strstr(szCTString, "international_feet") || strstr(szCTString, "international feet") )
      strcpy(units, "international_feet");
    else if(strstr(szCTString, "feet") || strstr(szCTString, "foot"))
      strcpy(units, "us_survey_feet");
  }

  /* check PCSCitationGeoKey if it exists */
  szCTString[0] = '\0';
  if( hGTIF && GTIFKeyGet( hGTIF, PCSCitationGeoKey, szCTString, 0, sizeof(szCTString)) )  
  {
      /* For tif created by LEICA(ERDAS), ESRI state plane pe string was used and */
      /* the state plane zone is given in PCSCitation. Therefore try Esri pe string first. */
      SetCitationToSRS(hGTIF, szCTString, PCSCitationGeoKey, poSRS, pLinearUnitIsSet);
      const char *pcsName = poSRS->GetAttrValue("PROJCS");
      const char *pStr = NULL;
      if( pcsName && (pStr = strstr(pcsName, "State Plane Zone ")) != NULL
       || (pStr = strstr(szCTString, "State Plane Zone ")) != NULL )
      {
        pStr += strlen("State Plane Zone ");
        int statePlaneZone = abs(atoi(pStr));
        char nad[32];
        strcpy(nad, "HARN");
        if( strstr(szCTString, "NAD83") || strstr(szCTString, "NAD = 83") )
          strcpy(nad, "NAD83");
        else if( strstr(szCTString, "NAD27") || strstr(szCTString, "NAD = 27") )
          strcpy(nad, "NAD27");
        if( poSRS->ImportFromStatePlaneWKT(statePlaneZone, (const char*)nad, (const char*)units, psDefn->PCS) == OGRERR_NONE )
          return TRUE;
      }
      else if( pcsName && (pStr = strstr(pcsName, "UTM Zone ")) != NULL )
        CheckUTM( psDefn, szCTString );
  }

  /* check state plane again to see if a pe string is available */
  if( psDefn->PCS != KvUserDefined )
  {
    if( poSRS->ImportFromStatePlaneWKT(0, NULL, (const char*)units, psDefn->PCS) == OGRERR_NONE )
      return TRUE;
  }
  return FALSE;
}

/************************************************************************/
/*               ImagineCitationTranslation()                           */
/*                                                                      */
/*      Translate ERDAS Imagine GeoTif citation                         */
/************************************************************************/
char* ImagineCitationTranslation(char* psCitation, geokey_t keyID)
{
  static const char *keyNames[] = {
    "NAD = ", "Datum = ", "Ellipsoid = ", "Units = ", NULL
  };

  char* ret = NULL;
  int adjustLen = 0, i;
  if(!psCitation)
    return ret;
  if(EQUALN(psCitation, "IMAGINE GeoTIFF Support", strlen("IMAGINE GeoTIFF Support")))
  {
    // this is a handle IMAGING style citation
    char name[256];
    name[0] = '\0';
    char* p = NULL;
    char* p1 = NULL;

    p = strchr(psCitation, '$');
    if( p && strchr(p, '\n') )
      p = strchr(p, '\n') + 1;
    if(p)
    {
      p1 = p + strlen(p);
      char *p2 = strchr(p, '\n');
      if(p2)
        p1 = MIN(p1, p2);
      p2 = strchr(p, '\0');
      if(p2)
        p1 = MIN(p1, p2);
      for(i=0; keyNames[i]!=NULL; i++)
      {
        p2 = strstr(p, keyNames[i]);
        if(p2)
          p1 = MIN(p1, p2);
      }
    }

    // PCS name, GCS name and PRJ name
    if(p && p1)
    {
      switch (keyID)
      {
        case PCSCitationGeoKey:
          if(strstr(psCitation, "Projection = "))
            strcpy(name, "PRJ Name = ");
          else
            strcpy(name, "PCS Name = ");
        break;
        case GTCitationGeoKey:
          strcpy(name, "PCS Name = ");
        break;
        case GeogCitationGeoKey:
          if(!strstr(p, "Unable to"))
            strcpy(name, "GCS Name = ");
        break;
      }
      if(strlen(name)>0)
      {
        char* p2;
        if((p2 = strstr(psCitation, "Projection Name = ")) != 0)
          p = p2 + strlen("Projection Name = ");
        if((p2 = strstr(psCitation, "Projection = ")) != 0)
          p = p2 + strlen("Projection = ");
        if(p1[0] == '\0' || p1[0] == '\n' || p1[0] == ' ')
          p1 --;
        p2 = p1 - 1;
        while( p2>0 && (p2[0] == ' ' || p2[0] == '\0' || p2[0] == '\n') )
          p2--;
        if(p2 != p1 - 1)
          p1 = p2;
        if(p1 >= p)
        {
          strncat(name, p, p1-p+1);
          strcat(name, "|");
          name[strlen(name)] = '\0';
        }
      }
    }

    // All other parameters
    for(i=0; keyNames[i]!=NULL; i++)
    {
      p = strstr(psCitation, keyNames[i]);
      if(p)
      {
        p += strlen(keyNames[i]);
        p1 = p + strlen(p);
        char *p2 = strchr(p, '\n');
        if(p2)
          p1 = MIN(p1, p2);
        p2 = strchr(p, '\0');
        if(p2)
          p1 = MIN(p1, p2);
        for(int j=0; keyNames[j]!=NULL; j++)
        {
          p2 = strstr(p, keyNames[j]);
          if(p2)
            p1 = MIN(p1, p2);
        }
      }
      if(p && p1 && p1>p)
      {
        if(EQUAL(keyNames[i], "Units = "))
          strcat(name, "LUnits = ");
        else
          strcat(name, keyNames[i]);
        if(p1[0] == '\0' || p1[0] == '\n' || p1[0] == ' ')
          p1 --;
        char* p2 = p1 - 1;
        while( p2>0 && (p2[0] == ' ' || p2[0] == '\0' || p2[0] == '\n') )
          p2--;
        if(p2 != p1 - 1)
          p1 = p2;
        if(p1 >= p)
        {
          strncat(name, p, p1-p+1);
          strcat(name, "|");
          name[strlen(name)] = '\0';
        }
      }
    }
    if(strlen(name) > 0)
      ret = CPLStrdup(name);
  }
  return ret;
}

/************************************************************************/
/*                      CitationStringParse()                           */
/*                                                                      */
/*                      Parse a Citation string                         */
/************************************************************************/
char** CitationStringParse(char* psCitation, geokey_t keyID)
{
  char ** ret = NULL;
  if(!psCitation)
    return ret;

  ret = (char **) CPLCalloc(sizeof(char*), nCitationNameTypes); 
  char* pDelimit = NULL;
  char* pStr = psCitation;
  char name[512];
  int nameLen = strlen(psCitation);
  OGRBoolean nameFound = FALSE;
  while((pStr-psCitation+1)< nameLen)
  {
    if( pDelimit = strstr(pStr, "|"))
    {
      strncpy( name, pStr, pDelimit-pStr );
      name[pDelimit-pStr] = '\0';
      pStr = pDelimit+1;
    }
    else
    {
      strcpy (name, pStr);
      pStr += strlen(pStr);
    }
    if( strstr(name, "PCS Name = ") )
    {
      ret[CitPcsName] = CPLStrdup(name+strlen("PCS Name = "));
      nameFound = TRUE;
    }
    if(strstr(name, "PRJ Name = "))
    {
      ret[CitPrjName] = CPLStrdup(name+strlen("PRJ Name = "));
      nameFound = TRUE;
    }
    if(strstr(name, "LUnits = "))
    {
      ret[CitLUnitsName] = CPLStrdup(name+strlen("LUnits = "));
      nameFound = TRUE;
    }
    if(strstr(name, "GCS Name = "))
    {
      ret[CitGcsName] = CPLStrdup(name+strlen("GCS Name = "));
      nameFound = TRUE;
    }
    if(strstr(name, "Datum = "))
    {
      ret[CitDatumName] = CPLStrdup(name+strlen("Datum = "));
      nameFound = TRUE;
    }
    if(strstr(name, "Ellipsoid = "))
    {
      ret[CitEllipsoidName] = CPLStrdup(name+strlen("Ellipsoid = "));
      nameFound = TRUE;
    }
    if(strstr(name, "Primem = "))
    {
      ret[CitPrimemName] = CPLStrdup(name+strlen("Primem = "));    
      nameFound = TRUE;
    }
    if(strstr(name, "AUnits = "))
    {
      ret[CitAUnitsName] = CPLStrdup(name+strlen("AUnits = "));
      nameFound = TRUE;
    }
  }
  if( !nameFound && keyID == GeogCitationGeoKey )
  {
    ret[CitGcsName] = CPLStrdup(name);
    nameFound = TRUE;
  }
  if(!nameFound)
  {
    CPLFree( ret );
    ret = (char**)NULL;
  }
  return ret;
}

/************************************************************************/
/*                        SetLinearUnitCitation()                     */
/*                                                                      */
/*               Set linear unit Citation string                        */
/************************************************************************/
void SetLinearUnitCitation(GTIF* psGTIF, char* pszLinearUOMName)
{
  char szName[512];
  int n = 0;
  if( GTIFKeyGet( psGTIF, PCSCitationGeoKey, szName, 0, sizeof(szName) ) )
    n = strlen(szName);
  if(n>0)
  {
    if(szName[n-1] != '|')
      strcat(szName, "|");
    strcat(szName, "LUnits = ");
    strcat(szName, pszLinearUOMName);
    strcat(szName, "|");
  }
  else
  {
    strcpy(szName, "LUnits = ");
    strcat(szName, pszLinearUOMName);
  }
  GTIFKeySet( psGTIF, PCSCitationGeoKey, TYPE_ASCII, 0, szName );
  return;
}

/************************************************************************/
/*                        SetGeogCSCitation()                         */
/*                                                                      */
/*               Set geogcs Citation string                             */
/************************************************************************/
void SetGeogCSCitation(GTIF * psGTIF, OGRSpatialReference *poSRS, char* angUnitName, int nDatum, short nSpheroid)
{
  char szName[256];
  int n = 0;
  if( GTIFKeyGet( psGTIF, GeogCitationGeoKey, szName, 0, sizeof(szName) ) )
    n = strlen(szName);
  if(n > 0 )
  {
    if(!EQUALN(szName, "GCS Name = ", strlen("GCS Name = ")))
    {
      char newName[256];
      strcpy(newName, "GCS Name = ");
      strcat(newName, szName);
      strcpy(szName, newName);
      n += strlen("GCS Name = ");
    }
  }
  if(nDatum == KvUserDefined )
  {
    const char* datumName = poSRS->GetAttrValue( "DATUM" );
    if(n > 0 && datumName && strlen(datumName) > 0)
    {
      strcat(szName, "|Datum = ");
      strcat(szName, datumName);
      strcat(szName, "|");
      GTIFKeySet( psGTIF, GeogCitationGeoKey, TYPE_ASCII, 0, 
          szName );
    }             
  }
  if(nSpheroid == KvUserDefined )
  {
    const char* spheroidName = poSRS->GetAttrValue( "SPHEROID" );
    if(n == strlen(szName))
      strcat(szName, "|");
    if(n > 0 && spheroidName && strlen(spheroidName) > 0)
    {
      strcat(szName, "Ellipsoid = ");
      strcat(szName, spheroidName);
      strcat(szName, "|");
      GTIFKeySet( psGTIF, GeogCitationGeoKey, TYPE_ASCII, 0, 
          szName );
    }             
  }
  const char* primemName = poSRS->GetAttrValue( "PRIMEM" );
  if(n == strlen(szName))
    strcat(szName, "|");
  if(n > 0 && primemName && strlen(primemName) > 0)
  {
    strcat(szName, "Primem = ");
    strcat(szName, primemName);
    strcat(szName, "|");
    GTIFKeySet( psGTIF, GeogCitationGeoKey, TYPE_ASCII, 0, 
        szName );
    double primemValue = poSRS->GetPrimeMeridian(NULL);
    if(angUnitName && !EQUAL(angUnitName, "Degree"))
    {
      double aUnit = poSRS->GetAngularUnits(NULL);
      primemValue *= aUnit;
    }
    GTIFKeySet( psGTIF, GeogPrimeMeridianLongGeoKey, TYPE_DOUBLE, 1, 
          primemValue );
  } 
  if(angUnitName && !EQUAL(angUnitName, "Degree"))
  {
    if(n == strlen(szName))
      strcat(szName, "|");
    if(n > 0 && strlen(angUnitName) > 0)
    {
      strcat(szName, "AUnits = ");
      strcat(szName, angUnitName);
      strcat(szName, "|");
      GTIFKeySet( psGTIF, GeogCitationGeoKey, TYPE_ASCII, 0, 
          szName );
    } 
  }
  return;
}

/************************************************************************/
/*                           SetCitationToSRS()                         */
/*                                                                      */
/*              Parse and set Citation string to SRS                    */
/************************************************************************/
OGRBoolean SetCitationToSRS(GTIF* hGTIF, char* szCTString,  geokey_t geoKey,  OGRSpatialReference*	poSRS, OGRBoolean* pLinearUnitIsSet)
{
  OGRBoolean ret = FALSE;
  char* lUnitName = NULL;
  double	dfLinearUOM = poSRS->GetLinearUnits( &lUnitName );
  if(!lUnitName || strlen(lUnitName) == 0  || EQUAL(lUnitName, "unknown"))
    *pLinearUnitIsSet = FALSE;
  else
    *pLinearUnitIsSet = TRUE;

  char* imgCTName = ImagineCitationTranslation(szCTString, geoKey);
  if(imgCTName)
  {
    strcpy(szCTString, imgCTName);
    CPLFree( imgCTName );
  }
  char** ctNames = CitationStringParse(szCTString, geoKey);
  if(ctNames)
  {
    if( poSRS->GetRoot() == NULL)
      poSRS->SetNode( "PROJCS", "unnamed" );
    if(ctNames[CitPcsName])
    {
      poSRS->SetNode( "PROJCS", ctNames[CitPcsName] );
      ret = TRUE;
    }
    if(ctNames[CitPrjName])
      poSRS->SetProjection( ctNames[CitPrjName] );

    if(ctNames[CitLUnitsName])
    {
      double unitSize = 0.0;
      int size = strlen(ctNames[CitLUnitsName]);
      if(strchr(ctNames[CitLUnitsName], '\0'))
        size -= 1;
      for( int i = 0; apszUnitMap[i] != NULL; i += 2 )
      {
            if( EQUALN(apszUnitMap[i], ctNames[CitLUnitsName], size) )
            {
                unitSize = atof(apszUnitMap[i+1]);
                break;
            }
      }
      if( unitSize == 0.0 )
        GTIFKeyGet(hGTIF, ProjLinearUnitSizeGeoKey, &unitSize, 0,
                         sizeof(unitSize) );
      poSRS->SetLinearUnits( ctNames[CitLUnitsName], unitSize);
      *pLinearUnitIsSet = TRUE;
    }
    for(int i= 0; i<nCitationNameTypes; i++) 
      CPLFree( ctNames[i] );
    CPLFree( ctNames );
  }

  /* if no "PCS Name = " (from Erdas) in GTCitationGeoKey */
  if(geoKey == GTCitationGeoKey)
  {
    if(strlen(szCTString) > 0 && !strstr(szCTString, "PCS Name = "))
    {
      poSRS->SetNode( "PROJCS", szCTString );
      ret = TRUE;
    }
  }

  return ret;
}

/************************************************************************/
/*                      GetGeogCSFromCitation()                         */
/*                                                                      */
/*        Parse and get geogcs names from a Citation string             */
/************************************************************************/
void GetGeogCSFromCitation(char* szGCSName,  geokey_t geoKey, 
                          char	**ppszGeogName,
                          char	**ppszDatumName,
                          char	**ppszPMName,
                          char	**ppszSpheroidName,
                          char	**ppszAngularUnits)
{
  *ppszGeogName = *ppszDatumName = *ppszPMName = 
  *ppszSpheroidName = *ppszAngularUnits = NULL;

  char* imgCTName = ImagineCitationTranslation(szGCSName, geoKey);
  if(imgCTName)
  {
    strcpy(szGCSName, imgCTName);
    CPLFree( imgCTName );
  }
  char** ctNames = CitationStringParse(szGCSName, geoKey);
  if(ctNames)
  {
    if(ctNames[CitGcsName])
      *ppszGeogName = CPLStrdup( ctNames[CitGcsName] );

    if(ctNames[CitDatumName])
      *ppszDatumName = CPLStrdup( ctNames[CitDatumName] );

    if(ctNames[CitEllipsoidName])
      *ppszSpheroidName = CPLStrdup( ctNames[CitEllipsoidName] );

    if(ctNames[CitPrimemName])
      *ppszPMName = CPLStrdup( ctNames[CitPrimemName] );

    if(ctNames[CitAUnitsName])
       *ppszAngularUnits = CPLStrdup( ctNames[CitAUnitsName] );

    for(int i= 0; i<nCitationNameTypes; i++)
      CPLFree( ctNames[i] );
    CPLFree( ctNames );
  }
  return;
}

/************************************************************************/
/*                               CheckUTM()                             */
/*                                                                      */
/*        Check utm proj code by its name.                              */
/************************************************************************/
void CheckUTM( GTIFDefn * psDefn, char * pszCtString )
{
  if(!psDefn || !pszCtString)
    return;

  static const char *apszUtmProjCode[] = {
    "PSAD56", "17N", "16017",
    "PSAD56", "18N", "16018",
    "PSAD56", "19N", "16019",
    "PSAD56", "20N", "16020",
    "PSAD56", "21N", "16021",
    "PSAD56", "17S", "16117",
    "PSAD56", "18S", "16118",
    "PSAD56", "19S", "16119",
    "PSAD56", "20S", "16120",
    "PSAD56", "21S", "16121",
    "PSAD56", "22S", "16122",
  NULL, NULL, NULL};

  char* p = strstr(pszCtString, "Datum = ");
  char datumName[128];
  if(p)
  {
    p += strlen("Datum = ");
    char* p1 = strchr(p, '|');
    if(p1)
    {
      strncpy(datumName, p, (p1-p));
      datumName[p1-p] = '\0';
    }
    else
      strcpy(datumName, p);
  }

  char utmName[64];
  p = strstr(pszCtString, "UTM Zone ");
  if(p)
  {
    p += strlen("UTM Zone ");
    char* p1 = strchr(p, '|');
    if(p1)
    {
      strncpy(utmName, p, (p1-p));
      utmName[p1-p] = '\0';
    }
    else
      strcpy(utmName, p);
  }

  for(int i=0; apszUtmProjCode[i]!=NULL; i += 3)
  {
    if(EQUALN(utmName, apszUtmProjCode[i+1], strlen(apszUtmProjCode[i+1])) &&
       EQUAL(datumName, apszUtmProjCode[i]) )
    {
      if(psDefn->ProjCode != atoi(apszUtmProjCode[i+2]))
      {
        psDefn->ProjCode = atoi(apszUtmProjCode[i+2]);
        GTIFGetProjTRFInfo( psDefn->ProjCode, NULL, &(psDefn->Projection),
                            psDefn->ProjParm );
        break;
      }
    }
  }
  return;
}
