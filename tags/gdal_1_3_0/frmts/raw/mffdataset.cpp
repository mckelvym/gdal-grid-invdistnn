/******************************************************************************
 * $Id$
 *
 * Project:  GView
 * Purpose:  Implementation of Atlantis MFF Support
 * Author:   Frank Warmerdam, warmerda@home.com
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.28  2005/05/05 13:55:42  fwarmerdam
 * PAM Enable
 *
 * Revision 1.27  2003/04/02 19:05:59  dron
 * Large file support.
 *
 * Revision 1.26  2003/03/18 19:03:44  gwalter
 * Add flushing, fix coordinate interpretation.
 *
 * Revision 1.25  2003/03/13 18:34:02  gwalter
 * Fix data type determination in CreateCopy.
 *
 * Revision 1.24  2003/03/12 16:02:28  gwalter
 * Fix spheroid support.
 *
 * Revision 1.22  2003/03/03 20:10:06  gwalter
 * Updated MFF and HKV (MFF2) georeferencing support.
 *
 * Revision 1.21  2002/11/23 18:54:17  warmerda
 * added CREATIONDATATYPES metadata for drivers
 *
 * Revision 1.20  2002/09/04 06:50:37  warmerda
 * avoid static driver pointers
 *
 * Revision 1.19  2002/06/12 21:12:25  warmerda
 * update to metadata based driver info
 *
 * Revision 1.18  2001/11/11 23:51:00  warmerda
 * added required class keyword to friend declarations
 *
 * Revision 1.17  2001/07/18 04:51:57  warmerda
 * added CPL_CVSID
 *
 * Revision 1.16  2001/07/11 18:08:14  warmerda
 * Fixed pixel interpretation again.  TOP_LEFT is really 0.0,0.0 but
 * BOTTOM_RIGHT is the top left corner of the bottom right pixel!
 *
 * Revision 1.15  2001/07/11 18:00:57  warmerda
 * Fixed off-by-half-pixel issues with MFF GCPs.
 *
 * Revision 1.14  2000/10/11 16:06:51  warmerda
 * added read support for GCPn and NUM_GCPs keywords
 *
 * Revision 1.13  2000/09/25 21:20:13  warmerda
 * avoid initialization warnings
 *
 * Revision 1.12  2000/08/24 14:26:09  warmerda
 * improved handling of read-only file sets
 *
 * Revision 1.11  2000/08/15 19:28:26  warmerda
 * added help topic
 *
 * Revision 1.10  2000/07/26 21:05:21  warmerda
 * added support for reading tiled MFF files
 *
 * Revision 1.9  2000/07/12 19:21:34  warmerda
 * fixed cleanup of GCPs
 *
 * Revision 1.8  2000/06/05 17:24:06  warmerda
 * added real complex support
 *
 * Revision 1.7  2000/05/18 22:06:03  warmerda
 * added gcp and metadata support
 *
 * Revision 1.6  2000/05/15 14:18:27  warmerda
 * added COMPLEX_INTERPRETATION metadata
 *
 * Revision 1.5  2000/04/21 21:59:26  warmerda
 * added overview support
 *
 * Revision 1.4  2000/03/24 20:00:46  warmerda
 * Don't require IMAGE_FILE_FORMAT.
 *
 * Revision 1.3  2000/03/13 14:34:42  warmerda
 * avoid const problem on write
 *
 * Revision 1.2  2000/03/06 21:51:32  warmerda
 * fixed docs
 *
 * Revision 1.1  2000/03/06 19:23:20  warmerda
 * New
 *
 */

#include "rawdataset.h"
#include "cpl_string.h"
#include <ctype.h>
#include "ogr_spatialref.h"
#include "atlsci_spheroid.h"

CPL_CVSID("$Id$");

CPL_C_START
void	GDALRegister_MFF(void);
CPL_C_END

typedef enum {
  MFFPRJ_NONE,
  MFFPRJ_LL,
  MFFPRJ_UTM,
  MFFPRJ_UNRECOGNIZED
} ;

static int         GetMFFProjectionType(const char * pszNewProjection);

/************************************************************************/
/* ==================================================================== */
/*				MFFDataset				*/
/* ==================================================================== */
/************************************************************************/

class MFFDataset : public RawDataset
{
    FILE	*fpImage;	// image data file.
    
    int         nGCPCount;
    GDAL_GCP    *pasGCPList;

    char *pszProjection;
    char *pszGCPProjection;
    double adfGeoTransform[6]; 

    void        ScanForGCPs();
    void        ScanForProjectionInfo();
 

  public:
    		MFFDataset();
    	        ~MFFDataset();
    
    char	**papszHdrLines;
    
    FILE        **pafpBandFiles;
    
    virtual int    GetGCPCount();
    virtual const char *GetGCPProjection();
    virtual const GDAL_GCP *GetGCPs();

    virtual const char *GetProjectionRef();
    virtual CPLErr GetGeoTransform( double * );

    static GDALDataset *Open( GDALOpenInfo * );
    static GDALDataset *Create( const char * pszFilename,
                                int nXSize, int nYSize, int nBands,
                                GDALDataType eType, char ** papszParmList );
    static GDALDataset *CreateCopy( const char * pszFilename, 
                                    GDALDataset *poSrcDS, 
                                    int bStrict, char ** papszOptions, 
                                    GDALProgressFunc pfnProgress, 
                                    void * pProgressData );

};

/************************************************************************/
/* ==================================================================== */
/*                            MFFTiledBand                              */
/* ==================================================================== */
/************************************************************************/

class MFFTiledBand : public GDALRasterBand
{
    friend class MFFDataset;

    FILE        *fpRaw;
    int         bNative;

  public:

                   MFFTiledBand( MFFDataset *, int, FILE *, int, int, 
                                 GDALDataType, int );
                   ~MFFTiledBand();

    virtual CPLErr IReadBlock( int, int, void * );
};


/************************************************************************/
/*                            MFFTiledBand()                            */
/************************************************************************/

MFFTiledBand::MFFTiledBand( MFFDataset *poDS, int nBand, FILE *fp, 
                            int nTileXSize, int nTileYSize, 
                            GDALDataType eDataType, int bNative )

{
    this->poDS = poDS;
    this->nBand = nBand;

    this->eDataType = eDataType; 

    this->bNative = bNative;

    this->nBlockXSize = nTileXSize;
    this->nBlockYSize = nTileYSize;

    this->fpRaw = fp;
}

/************************************************************************/
/*                           ~MFFTiledBand()                            */
/************************************************************************/

MFFTiledBand::~MFFTiledBand()

{
    VSIFCloseL( fpRaw );
}


/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr MFFTiledBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                 void * pImage )

{
    long    nOffset;
    int     nTilesPerRow;
    int     nWordSize, nBlockSize;

    nTilesPerRow = (nRasterXSize + nBlockXSize - 1) / nBlockXSize;
    nWordSize = GDALGetDataTypeSize( eDataType ) / 8;
    nBlockSize = nWordSize * nBlockXSize * nBlockYSize;

    nOffset = nBlockSize * (nBlockXOff + nBlockYOff*nTilesPerRow);

    if( VSIFSeekL( fpRaw, nOffset, SEEK_SET ) == -1 
        || VSIFReadL( pImage, 1, nBlockSize, fpRaw ) < 1 )
    {
        CPLError( CE_Failure, CPLE_FileIO, 
                  "Read of tile %d/%d failed with fseek or fread error.", 
                  nBlockXOff, nBlockYOff );
        return CE_Failure;
    }
    
    if( !bNative && nWordSize > 1 )
    {
        if( GDALDataTypeIsComplex( eDataType ) )
        {
            GDALSwapWords( pImage, nWordSize/2, nBlockXSize*nBlockYSize, 
                           nWordSize );
            GDALSwapWords( ((GByte *) pImage)+nWordSize/2, 
                           nWordSize/2, nBlockXSize*nBlockYSize, nWordSize );
        }
        else
            GDALSwapWords( pImage, nWordSize,
                           nBlockXSize * nBlockYSize, nWordSize );
    }
    
    return CE_None;
}

/************************************************************************/
/*                      MFF Spheroids                                   */
/************************************************************************/

class MFFSpheroidList : public SpheroidList
{

public:

  MFFSpheroidList();
  ~MFFSpheroidList();

};

MFFSpheroidList :: MFFSpheroidList()
{
  num_spheroids = 18;

  epsilonR = 0.1;
  epsilonI = 0.000001;   

  spheroids[0].SetValuesByRadii("SPHERE",6371007.0,6371007.0);
  spheroids[1].SetValuesByRadii("EVEREST",6377304.0,6356103.0);
  spheroids[2].SetValuesByRadii("BESSEL",6377397.0,6356082.0);
  spheroids[3].SetValuesByRadii("AIRY",6377563.0,6356300.0);
  spheroids[4].SetValuesByRadii("CLARKE_1858",6378294.0,6356621.0);
  spheroids[5].SetValuesByRadii("CLARKE_1866",6378206.4,6356583.8);
  spheroids[6].SetValuesByRadii("CLARKE_1880",6378249.0,6356517.0);
  spheroids[7].SetValuesByRadii("HAYFORD",6378388.0,6356915.0);
  spheroids[8].SetValuesByRadii("KRASOVSKI",6378245.0,6356863.0);
  spheroids[9].SetValuesByRadii("HOUGH",6378270.0,6356794.0);
  spheroids[10].SetValuesByRadii("FISHER_60",6378166.0,6356784.0);
  spheroids[11].SetValuesByRadii("KAULA",6378165.0,6356345.0);
  spheroids[12].SetValuesByRadii("IUGG_67",6378160.0,6356775.0);
  spheroids[13].SetValuesByRadii("FISHER_68",6378150.0,6356330.0);
  spheroids[14].SetValuesByRadii("WGS_72",6378135.0,6356751.0);
  spheroids[15].SetValuesByRadii("IUGG_75",6378140.0,6356755.0);
  spheroids[16].SetValuesByRadii("WGS_84",6378137.0,6356752.0);
  spheroids[17].SetValuesByRadii("HUGHES",6378273.0,6356889.4); 
}

MFFSpheroidList::~MFFSpheroidList()

{
}

/************************************************************************/
/* ==================================================================== */
/*				MFFDataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                            MFFDataset()                             */
/************************************************************************/

MFFDataset::MFFDataset()
{
    papszHdrLines = NULL;
    pafpBandFiles = NULL;
    nGCPCount = 0;
    pasGCPList = NULL;

    pszProjection = CPLStrdup("");
    pszGCPProjection = CPLStrdup("");
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;
}

/************************************************************************/
/*                            ~MFFDataset()                            */
/************************************************************************/

MFFDataset::~MFFDataset()

{
    FlushCache();
    CSLDestroy( papszHdrLines );
    if( pafpBandFiles != NULL )
    {
        for( int i = 0; i < GetRasterCount(); i++ )
        {
            if( pafpBandFiles[i] != NULL )
                VSIFCloseL( pafpBandFiles[i] );
        }
        CPLFree( pafpBandFiles );
    }

    if( nGCPCount > 0 )
    {
        GDALDeinitGCPs( nGCPCount, pasGCPList );
        CPLFree( pasGCPList );
    }
    CPLFree( pszProjection );
    CPLFree( pszGCPProjection );

}

/************************************************************************/
/*                            GetGCPCount()                             */
/************************************************************************/

int MFFDataset::GetGCPCount()

{
    return nGCPCount;
}

/************************************************************************/
/*                          GetGCPProjection()                          */
/************************************************************************/

const char *MFFDataset::GetGCPProjection()

{
    if( nGCPCount > 0 )
        return pszGCPProjection;
    else
        return "";
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *MFFDataset::GetProjectionRef()

{
   return ( pszProjection );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr MFFDataset::GetGeoTransform( double * padfTransform )

{
    memcpy( padfTransform,  adfGeoTransform, sizeof(double) * 6 ); 
    return( CE_None );
}

/************************************************************************/
/*                               GetGCP()                               */
/************************************************************************/

const GDAL_GCP *MFFDataset::GetGCPs()

{
    return pasGCPList;
}

/************************************************************************/
/*                            ScanForGCPs()                             */
/************************************************************************/

void MFFDataset::ScanForGCPs()

{
    int     nCorner;
    int	    NUM_GCPS = 0;
    
    if( CSLFetchNameValue(papszHdrLines, "NUM_GCPS") != NULL )
        NUM_GCPS = atoi(CSLFetchNameValue(papszHdrLines, "NUM_GCPS"));

    nGCPCount = 0;
    pasGCPList = (GDAL_GCP *) CPLCalloc(sizeof(GDAL_GCP),5+NUM_GCPS);

    for( nCorner = 0; nCorner < 5; nCorner++ )
    {
        const char * pszBase=NULL;
        double       dfRasterX=0.0, dfRasterY=0.0;
        char         szLatName[40], szLongName[40];

        if( nCorner == 0 )
        {
            dfRasterX = 0.5;
            dfRasterY = 0.5;
            pszBase = "TOP_LEFT_CORNER";
        }
        else if( nCorner == 1 )
        {
            dfRasterX = GetRasterXSize()-0.5;
            dfRasterY = 0.5;
            pszBase = "TOP_RIGHT_CORNER";
        }
        else if( nCorner == 2 )
        {
            dfRasterX = GetRasterXSize()-0.5;
            dfRasterY = GetRasterYSize()-0.5;
            pszBase = "BOTTOM_RIGHT_CORNER";
        }
        else if( nCorner == 3 )
        {
            dfRasterX = 0.5;
            dfRasterY = GetRasterYSize()-0.5;
            pszBase = "BOTTOM_LEFT_CORNER";
        }
        else if( nCorner == 4 )
        {
            dfRasterX = GetRasterXSize()/2.0;
            dfRasterY = GetRasterYSize()/2.0;
            pszBase = "CENTRE";
        }

        sprintf( szLatName, "%s_LATITUDE", pszBase );
        sprintf( szLongName, "%s_LONGITUDE", pszBase );
        
        if( CSLFetchNameValue(papszHdrLines, szLatName) != NULL
            && CSLFetchNameValue(papszHdrLines, szLongName) != NULL )
        {
            GDALInitGCPs( 1, pasGCPList + nGCPCount );
            
            CPLFree( pasGCPList[nGCPCount].pszId );

            pasGCPList[nGCPCount].pszId = CPLStrdup( pszBase );
                
            pasGCPList[nGCPCount].dfGCPX = 
                atof(CSLFetchNameValue(papszHdrLines, szLongName));
            pasGCPList[nGCPCount].dfGCPY = 
                atof(CSLFetchNameValue(papszHdrLines, szLatName));
            pasGCPList[nGCPCount].dfGCPZ = 0.0;

            pasGCPList[nGCPCount].dfGCPPixel = dfRasterX;
            pasGCPList[nGCPCount].dfGCPLine = dfRasterY;

            nGCPCount++;
        }
       
    }

/* -------------------------------------------------------------------- */
/*      Collect standalone GCPs.  They look like:                       */
/*                                                                      */
/*      GCPn = row, col, lat, long                                      */
/*      GCP1 = 1, 1, 45.0, -75.0                                        */
/* -------------------------------------------------------------------- */
    int i;

    for( i = 0; i < NUM_GCPS; i++ )
    {
        char	szName[25];
        char    **papszTokens;

        sprintf( szName, "GCP%d", i+1 );
        if( CSLFetchNameValue( papszHdrLines, szName ) == NULL )
            continue;

        papszTokens = CSLTokenizeStringComplex( 
            CSLFetchNameValue( papszHdrLines, szName ), 
            ",", FALSE, FALSE );
        if( CSLCount(papszTokens) == 4 )
        {
            GDALInitGCPs( 1, pasGCPList + nGCPCount );

            CPLFree( pasGCPList[nGCPCount].pszId );
            pasGCPList[nGCPCount].pszId = CPLStrdup( szName );

            pasGCPList[nGCPCount].dfGCPX = atof(papszTokens[3]);
            pasGCPList[nGCPCount].dfGCPY = atof(papszTokens[2]);
            pasGCPList[nGCPCount].dfGCPZ = 0.0;
            pasGCPList[nGCPCount].dfGCPPixel = atof(papszTokens[1])+0.5;
            pasGCPList[nGCPCount].dfGCPLine = atof(papszTokens[0])+0.5;

            nGCPCount++;
        }
    }
}

/************************************************************************/
/*                        ScanForProjectionInfo                         */
/************************************************************************/

void MFFDataset::ScanForProjectionInfo()
{
    const char *pszProjName, *pszOriginLong, *pszSpheroidName;
    const char *pszSpheroidEqRadius, *pszSpheroidPolarRadius;
    double eq_radius, polar_radius;
    OGRSpatialReference oProj;
    OGRSpatialReference oLL;
    MFFSpheroidList *mffEllipsoids;

    pszProjName = CSLFetchNameValue(papszHdrLines, 
                                    "PROJECTION_NAME");
    pszOriginLong = CSLFetchNameValue(papszHdrLines, 
                                      "PROJECTION_ORIGIN_LONGITUDE");
    pszSpheroidName = CSLFetchNameValue(papszHdrLines, 
                                      "SPHEROID_NAME");

    if (pszProjName == NULL)
    {
        CPLFree( pszProjection );
        CPLFree( pszGCPProjection );
        pszProjection=CPLStrdup("");
        pszGCPProjection=CPLStrdup("");
        return;
    }
    else if ((!EQUAL(pszProjName,"utm")) && (!EQUAL(pszProjName,"ll")))
    {
        CPLError(CE_Warning,CPLE_AppDefined,
                 "Warning- only utm and lat/long projections are currently supported.");
        CPLFree( pszProjection );
        CPLFree( pszGCPProjection );
        pszProjection=CPLStrdup("");
        pszGCPProjection=CPLStrdup("");
        return;
    }
    mffEllipsoids = new MFFSpheroidList;

    if( EQUAL(pszProjName,"utm") )
    {
        int nZone;

        if (pszOriginLong == NULL)
        {
            /* If origin not specified, assume 0.0 */
            CPLError(CE_Warning,CPLE_AppDefined,
                   "Warning- no projection origin longitude specified.  Assuming 0.0.");
            nZone = 31;
        }
        else
            nZone = 31 + (int) floor(atof(pszOriginLong)/6.0);


        if( pasGCPList[4].dfGCPY < 0 )
            oProj.SetUTM( nZone, 0 );
        else
            oProj.SetUTM( nZone, 1 );
     
        if (pszOriginLong != NULL)
            oProj.SetProjParm(SRS_PP_CENTRAL_MERIDIAN,atof(pszOriginLong));
        
    }

    if (pszOriginLong != NULL)
        oLL.SetProjParm(SRS_PP_LONGITUDE_OF_ORIGIN,atof(pszOriginLong));

    if ((pszSpheroidName == NULL))
    {
        CPLError(CE_Warning,CPLE_AppDefined,
            "Warning- unspecified ellipsoid.  Using wgs-84 parameters.\n");

        oProj.SetWellKnownGeogCS( "WGS84" );
        oLL.SetWellKnownGeogCS( "WGS84" );
    }
    else
    {
      if (mffEllipsoids->SpheroidInList(pszSpheroidName))
      { 
         oProj.SetGeogCS( "unknown","unknown",pszSpheroidName,
                         mffEllipsoids->GetSpheroidEqRadius(pszSpheroidName), 
                         mffEllipsoids->GetSpheroidInverseFlattening(pszSpheroidName)
                       );
         oLL.SetGeogCS( "unknown","unknown",pszSpheroidName,
                         mffEllipsoids->GetSpheroidEqRadius(pszSpheroidName), 
                         mffEllipsoids->GetSpheroidInverseFlattening(pszSpheroidName)
                      );
      }
      else if (EQUAL(pszSpheroidName,"USER_DEFINED"))
      {
          pszSpheroidEqRadius = CSLFetchNameValue(papszHdrLines,
                                                  "SPHEROID_EQUATORIAL_RADIUS");
          pszSpheroidPolarRadius = CSLFetchNameValue(papszHdrLines,
                                                  "SPHEROID_POLAR_RADIUS");
          if ((pszSpheroidEqRadius != NULL) && (pszSpheroidPolarRadius != NULL))
          {
            eq_radius = atof( pszSpheroidEqRadius );
            polar_radius = atof( pszSpheroidPolarRadius );
            oProj.SetGeogCS( "unknown","unknown","unknown",
                         eq_radius, eq_radius/(eq_radius - polar_radius));
            oLL.SetGeogCS( "unknown","unknown","unknown",
                         eq_radius, eq_radius/(eq_radius - polar_radius));          
          }
          else
          {
              CPLError(CE_Warning,CPLE_AppDefined,
                "Warning- radii not specified for user-defined ellipsoid. Using wgs-84 parameters. \n");
              oProj.SetWellKnownGeogCS( "WGS84" );
              oLL.SetWellKnownGeogCS( "WGS84" );
          }
      }
      else
      {
         CPLError(CE_Warning,CPLE_AppDefined,
            "Warning- unrecognized ellipsoid.  Using wgs-84 parameters.\n");
         oProj.SetWellKnownGeogCS( "WGS84" );
         oLL.SetWellKnownGeogCS( "WGS84" );
      }
    }  

    /* If a geotransform is sufficient to represent the GCP's (ie. each  */
    /* estimated gcp is within 0.25*pixel size of the actual value- this */
    /* is the test applied by GDALGCPsToGeoTransform), store the         */
    /* geotransform.                                                     */
    int transform_ok = FALSE;

    if (EQUAL(pszProjName,"LL"))
    {
        transform_ok = GDALGCPsToGeoTransform(nGCPCount,pasGCPList,adfGeoTransform,0);
    }
    else
    {
        OGRCoordinateTransformation *poTransform = NULL;
        double *dfPrjX, *dfPrjY; 
        int gcp_index;
        int    bSuccess = TRUE;

        dfPrjX = (double *) CPLMalloc(nGCPCount*sizeof(double));
        dfPrjY = (double *) CPLMalloc(nGCPCount*sizeof(double));


        poTransform = OGRCreateCoordinateTransformation( &oLL, &oProj );
        if( poTransform == NULL )
            bSuccess = FALSE;

        for(gcp_index=0;gcp_index<nGCPCount;gcp_index++)
        {
            dfPrjX[gcp_index] = pasGCPList[gcp_index].dfGCPX;
            dfPrjY[gcp_index] = pasGCPList[gcp_index].dfGCPY;

            if( bSuccess && !poTransform->Transform( 1, &(dfPrjX[gcp_index]), &(dfPrjY[gcp_index]) ) )
                bSuccess = FALSE;
 
        }

        if( bSuccess )
        {

            for(gcp_index=0;gcp_index<nGCPCount;gcp_index++)
            {
                pasGCPList[gcp_index].dfGCPX = dfPrjX[gcp_index];
                pasGCPList[gcp_index].dfGCPY = dfPrjY[gcp_index];

            }
            transform_ok = GDALGCPsToGeoTransform(nGCPCount,pasGCPList,adfGeoTransform,0);

        }
        CPLFree(dfPrjX);
        CPLFree(dfPrjY);

    }

    CPLFree( pszProjection );
    CPLFree( pszGCPProjection );
    pszProjection = NULL;
    pszGCPProjection = NULL;
    oProj.exportToWkt( &pszProjection );
    oProj.exportToWkt( &pszGCPProjection );

    if (transform_ok == FALSE)
    {
    /* transform is sufficient in some cases (slant range, standalone gcps) */
        adfGeoTransform[0] = 0.0;
        adfGeoTransform[1] = 1.0;
        adfGeoTransform[2] = 0.0;
        adfGeoTransform[3] = 0.0;
        adfGeoTransform[4] = 0.0;
        adfGeoTransform[5] = 1.0;
        CPLFree( pszProjection );
        pszProjection = CPLStrdup("");
    }

    delete mffEllipsoids;
  
}


/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *MFFDataset::Open( GDALOpenInfo * poOpenInfo )

{
    int		i, bNative = TRUE;
    char        **papszHdrLines;

/* -------------------------------------------------------------------- */
/*      We assume the user is pointing to the header file.              */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->nHeaderBytes < 17 || poOpenInfo->fp == NULL )
        return NULL;

    if( !EQUAL(CPLGetExtension(poOpenInfo->pszFilename),"hdr") )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Load the .hdr file, and compress white space out around the     */
/*      equal sign.                                                     */
/* -------------------------------------------------------------------- */
    papszHdrLines = CSLLoad( poOpenInfo->pszFilename );
    if( papszHdrLines == NULL )
        return NULL;

    for( i = 0; papszHdrLines[i] != NULL; i++ )
    {
        int       bAfterEqual = FALSE;
        int       iSrc, iDst;
        char     *pszLine = papszHdrLines[i];

        for( iSrc=0, iDst=0; pszLine[iSrc] != '\0'; iSrc++ )
        {
            if( bAfterEqual || pszLine[iSrc] != ' ' )
            {
                pszLine[iDst++] = pszLine[iSrc];
            }

            if( iDst > 0 && pszLine[iDst-1] == '=' )
                bAfterEqual = FALSE;
        }
        pszLine[iDst] = '\0';
    }

/* -------------------------------------------------------------------- */
/*      Verify it is an MFF file.                                       */
/* -------------------------------------------------------------------- */
    if( CSLFetchNameValue( papszHdrLines, "IMAGE_FILE_FORMAT" ) != NULL
        && !EQUAL(CSLFetchNameValue(papszHdrLines,"IMAGE_FILE_FORMAT"),"MFF") )
    {
        CSLDestroy( papszHdrLines );
        return NULL;
    }

    if( (CSLFetchNameValue( papszHdrLines, "IMAGE_LINES" ) == NULL 
         || CSLFetchNameValue(papszHdrLines,"LINE_SAMPLES") == NULL)
        && (CSLFetchNameValue( papszHdrLines, "no_rows" ) == NULL 
            || CSLFetchNameValue(papszHdrLines,"no_columns") == NULL) )
    {
        CSLDestroy( papszHdrLines );
        return NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    MFFDataset 	*poDS;

    poDS = new MFFDataset();

    poDS->papszHdrLines = papszHdrLines;
    
/* -------------------------------------------------------------------- */
/*      Set some dataset wide information.                              */
/* -------------------------------------------------------------------- */
    if( CSLFetchNameValue(papszHdrLines,"no_rows") != NULL
        && CSLFetchNameValue(papszHdrLines,"no_columns") != NULL )
    {
        poDS->RasterInitialize( 
            atoi(CSLFetchNameValue(papszHdrLines,"no_columns")),
            atoi(CSLFetchNameValue(papszHdrLines,"no_rows")) );
    }
    else
    {
        poDS->RasterInitialize( 
            atoi(CSLFetchNameValue(papszHdrLines,"LINE_SAMPLES")),
            atoi(CSLFetchNameValue(papszHdrLines,"IMAGE_LINES")) );
    }

    if( CSLFetchNameValue( papszHdrLines, "BYTE_ORDER" ) != NULL )
    {
#ifdef CPL_MSB
        bNative = EQUAL(CSLFetchNameValue(papszHdrLines,"BYTE_ORDER"),"MSB");
#else
        bNative = EQUAL(CSLFetchNameValue(papszHdrLines,"BYTE_ORDER"),"LSB");
#endif
    }

/* -------------------------------------------------------------------- */
/*      Get some information specific to APP tiled files.               */
/* -------------------------------------------------------------------- */
    int bTiled, nTileXSize=0, nTileYSize=0;
    const char *pszRefinedType = NULL;

    pszRefinedType = CSLFetchNameValue(papszHdrLines, "type" );

    bTiled = CSLFetchNameValue(papszHdrLines,"no_rows") != NULL;
    if( bTiled )
    {
        if( CSLFetchNameValue(papszHdrLines,"tile_size_rows") )
            nTileYSize = 
                atoi(CSLFetchNameValue(papszHdrLines,"tile_size_rows"));
        if( CSLFetchNameValue(papszHdrLines,"tile_size_columns") )
            nTileXSize = 
                atoi(CSLFetchNameValue(papszHdrLines,"tile_size_columns"));
    }

/* -------------------------------------------------------------------- */
/*      Read the directory to find matching band files.                 */
/* -------------------------------------------------------------------- */
    char       **papszDirFiles;
    char       *pszTargetBase, *pszTargetPath;
    int        nRawBand, nSkipped=0;

    pszTargetPath = CPLStrdup(CPLGetPath(poOpenInfo->pszFilename));
    pszTargetBase = CPLStrdup(CPLGetBasename( poOpenInfo->pszFilename ));
    papszDirFiles = CPLReadDir( CPLGetPath( poOpenInfo->pszFilename ) );
    if( papszDirFiles == NULL )
        return NULL;

    for( nRawBand = 0; TRUE; nRawBand++ )
    {
        const char  *pszExtension;
        int          nBand;
        GDALDataType eDataType;

        /* Find the next raw band file. */
        for( i = 0; papszDirFiles[i] != NULL; i++ )
        {
            if( !EQUAL(CPLGetBasename(papszDirFiles[i]),pszTargetBase) )
                continue;

            pszExtension = CPLGetExtension(papszDirFiles[i]);
            if( isdigit(pszExtension[1])
                && atoi(pszExtension+1) == nRawBand 
                && strchr("bBcCiIjJrRxXzZ",pszExtension[0]) != NULL )
                break;
        }

        if( papszDirFiles[i] == NULL  )
            break;

        /* open the file for required level of access */
        FILE     *fpRaw;
        const char *pszRawFilename = CPLFormFilename(pszTargetPath, 
                                                     papszDirFiles[i], NULL );

        if( poOpenInfo->eAccess == GA_Update )
            fpRaw = VSIFOpenL( pszRawFilename, "rb+" );
        else
            fpRaw = VSIFOpenL( pszRawFilename, "rb" );
        
        if( fpRaw == NULL )
        {
            CPLError( CE_Warning, CPLE_OpenFailed, 
                      "Unable to open %s ... skipping.\n", 
                      pszRawFilename );
            nSkipped++;
            continue;
        }

        pszExtension = CPLGetExtension(papszDirFiles[i]);
        if( pszRefinedType != NULL )
        {
            if( EQUAL(pszRefinedType,"C*4") )
                eDataType = GDT_CFloat32;
            else if( EQUAL(pszRefinedType,"C*8") )
                eDataType = GDT_CFloat64;
            else if( EQUAL(pszRefinedType,"R*4") )
                eDataType = GDT_Float32;
            else if( EQUAL(pszRefinedType,"R*8") )
                eDataType = GDT_Float64;
            else if( EQUAL(pszRefinedType,"I*1") )
                eDataType = GDT_Byte;
            else if( EQUAL(pszRefinedType,"I*2") )
                eDataType = GDT_Int16;
            else if( EQUAL(pszRefinedType,"I*4") )
                eDataType = GDT_Int32;
            else if( EQUAL(pszRefinedType,"U*2") )
                eDataType = GDT_UInt16;
            else if( EQUAL(pszRefinedType,"U*4") )
                eDataType = GDT_UInt32;
            else if( EQUAL(pszRefinedType,"J*1") )
                continue; /* we don't support 1 byte complex */
            else if( EQUAL(pszRefinedType,"J*2") )
                eDataType = GDT_CInt16;
            else if( EQUAL(pszRefinedType,"K*4") )
                eDataType = GDT_CInt32;
            else
                continue;
        }
        else if( EQUALN(pszExtension,"b",1) )
        {
            eDataType = GDT_Byte;
        }
        else if( EQUALN(pszExtension,"i",1) )
        {
            eDataType = GDT_UInt16;
        }
        else if( EQUALN(pszExtension,"j",1) )
        {
            eDataType = GDT_CInt16;
        }
        else if( EQUALN(pszExtension,"r",1) )
        {
            eDataType = GDT_Float32;
        }
        else if( EQUALN(pszExtension,"x",1) )
        {
            eDataType = GDT_CFloat32;
        }
        else
            continue;

        nBand = poDS->GetRasterCount() + 1;

        int nPixelOffset = GDALGetDataTypeSize(eDataType)/8;
        GDALRasterBand *poBand = NULL;
        
        if( bTiled )
        {
            poBand = 
                new MFFTiledBand( poDS, nBand, fpRaw, nTileXSize, nTileYSize,
                                  eDataType, bNative );
        }
        else
        {
            poBand = 
                new RawRasterBand( poDS, nBand, fpRaw, 0, nPixelOffset,
                                   nPixelOffset * poDS->GetRasterXSize(),
                                   eDataType, bNative, TRUE );
        }

        poDS->SetBand( nBand, poBand );
    }

    CPLFree(pszTargetPath);
    CPLFree(pszTargetBase);
    CSLDestroy(papszDirFiles);

/* -------------------------------------------------------------------- */
/*      Check if we have bands.                                         */
/* -------------------------------------------------------------------- */
    if( poDS->GetRasterCount() == 0 )
    {
        if( nSkipped > 0 && poOpenInfo->eAccess )
        {
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "Failed to open %d files that were apparently bands.\n"
                      "Perhaps this dataset is readonly?\n", 
                      nSkipped );
            delete poDS;
            return NULL;
        }
        else
        {
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "MFF header file read successfully, but no bands\n"
                      "were successfully found and opened." );
            delete poDS;
            return NULL;
        }
    }
    
/* -------------------------------------------------------------------- */
/*      Check for overviews.                                            */
/* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );

/* -------------------------------------------------------------------- */
/*      Initialize any PAM information.                                 */
/* -------------------------------------------------------------------- */
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML();

/* -------------------------------------------------------------------- */
/*      Set all information from the .hdr that isn't well know to be    */
/*      metadata.                                                       */
/* -------------------------------------------------------------------- */
    for( i = 0; papszHdrLines[i] != NULL; i++ )
    {
        const char *pszValue;
        char       *pszName;

        pszValue = CPLParseNameValue(papszHdrLines[i], &pszName);
        if( pszName == NULL || pszValue == NULL )
            continue;

        if( !EQUAL(pszName,"END") 
            && !EQUAL(pszName,"FILE_TYPE") 
            && !EQUAL(pszName,"BYTE_ORDER") 
            && !EQUAL(pszName,"no_columns") 
            && !EQUAL(pszName,"no_rows") 
            && !EQUAL(pszName,"type") 
            && !EQUAL(pszName,"tile_size_rows") 
            && !EQUAL(pszName,"tile_size_columns") 
            && !EQUAL(pszName,"IMAGE_FILE_FORMAT") 
            && !EQUAL(pszName,"IMAGE_LINES") 
            && !EQUAL(pszName,"LINE_SAMPLES") )
        {
            poDS->SetMetadataItem( pszName, pszValue );
        }

        CPLFree( pszName );
    }

/* -------------------------------------------------------------------- */
/*      Any GCPs in header file?                                        */
/* -------------------------------------------------------------------- */
    poDS->ScanForGCPs();
    poDS->ScanForProjectionInfo();

    return( poDS );
}

int GetMFFProjectionType(const char *pszNewProjection)
{
    OGRSpatialReference *oSRS;
    char *modifiableProjection = NULL;

    if( !EQUALN(pszNewProjection,"GEOGCS",6)
       && !EQUALN(pszNewProjection,"PROJCS",6)
       && !EQUAL(pszNewProjection,"") )
      {
          return MFFPRJ_UNRECOGNIZED;       
      }
      else if (EQUAL(pszNewProjection,""))
      { 
          return MFFPRJ_NONE;  
      }
      else
      {
          /* importFromWkt updates the pointer, so don't use pszNewProjection directly */
             modifiableProjection=CPLStrdup(pszNewProjection);

             oSRS = new OGRSpatialReference;
             oSRS->importFromWkt(&modifiableProjection);

             if ((oSRS->GetAttrValue("PROJECTION") != NULL) && 
                 (EQUAL(oSRS->GetAttrValue("PROJECTION"),SRS_PT_TRANSVERSE_MERCATOR)))
             {
               return MFFPRJ_UTM;
             }
             else if ((oSRS->GetAttrValue("PROJECTION") == NULL) && (oSRS->IsGeographic()))
             {
                  return MFFPRJ_LL;
             }
             else
             {
                  return MFFPRJ_UNRECOGNIZED;
             }
      }
}

/************************************************************************/
/*                               Create()                               */
/************************************************************************/

GDALDataset *MFFDataset::Create( const char * pszFilenameIn,
                                 int nXSize, int nYSize, int nBands,
                                 GDALDataType eType,
                                 char ** papszParmList )

{
/* -------------------------------------------------------------------- */
/*      Verify input options.                                           */
/* -------------------------------------------------------------------- */
    if( eType != GDT_Byte && eType != GDT_Float32 && eType != GDT_UInt16 
        && eType != GDT_CInt16 && eType != GDT_CFloat32 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
              "Attempt to create MFF file with currently unsupported\n"
              "data type (%s).\n",
              GDALGetDataTypeName(eType) );

        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Establish the base filename (path+filename, less extension).    */
/* -------------------------------------------------------------------- */
    char	*pszBaseFilename;
    int         i;

    pszBaseFilename = (char *) CPLMalloc(strlen(pszFilenameIn)+5);
    strcpy( pszBaseFilename, pszFilenameIn );
    
    for( i = strlen(pszBaseFilename)-1; i > 0; i-- )
    {
        if( pszBaseFilename[i] == '.' )
        {
            pszBaseFilename[i] = '\0';
            break;
        }

        if( pszBaseFilename[i] == '/' || pszBaseFilename[i] == '\\' )
            break;
    }
    
/* -------------------------------------------------------------------- */
/*      Create the header file.                                         */
/* -------------------------------------------------------------------- */
    FILE       *fp;
    const char *pszFilename;

    pszFilename = CPLFormFilename( NULL, pszBaseFilename, "hdr" );

    fp = VSIFOpen( pszFilename, "wt" );
    if( fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Couldn't create %s.\n", pszFilename );
        return NULL;
    }

    fprintf( fp, "IMAGE_FILE_FORMAT = MFF\n" );
    fprintf( fp, "FILE_TYPE = IMAGE\n" );
    fprintf( fp, "IMAGE_LINES = %d\n", nYSize );
    fprintf( fp, "LINE_SAMPLES = %d\n", nXSize );
#ifdef CPL_MSB     
    fprintf( fp, "BYTE_ORDER = MSB\n" );
#else
    fprintf( fp, "BYTE_ORDER = LSB\n" );
#endif

    if (CSLFetchNameValue(papszParmList,"NO_END") == NULL)
        fprintf( fp, "END\n" );
    
    VSIFClose( fp );
   
/* -------------------------------------------------------------------- */
/*      Create the data files, but don't bother writing any data to them.*/
/* -------------------------------------------------------------------- */
    for( int iBand = 0; iBand < nBands; iBand++ )
    {
        char       szExtension[4];

        if( eType == GDT_Byte )
            sprintf( szExtension, "b%02d", iBand );
        else if( eType == GDT_UInt16 )
            sprintf( szExtension, "i%02d", iBand );
        else if( eType == GDT_Float32 )
            sprintf( szExtension, "r%02d", iBand );
        else if( eType == GDT_CInt16 )
            sprintf( szExtension, "j%02d", iBand );
        else if( eType == GDT_CFloat32 )
            sprintf( szExtension, "x%02d", iBand );

        pszFilename = CPLFormFilename( NULL, pszBaseFilename, szExtension );
        fp = VSIFOpen( pszFilename, "wb" );
        if( fp == NULL )
        {
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "Couldn't create %s.\n", pszFilename );
            return NULL;
        }

        VSIFWrite( (void *) "", 1, 1, fp );
        VSIFClose( fp );
    }

/* -------------------------------------------------------------------- */
/*      Open the dataset normally.                                      */
/* -------------------------------------------------------------------- */
    GDALDataset *poDS;

    strcat( pszBaseFilename, ".hdr" );
    poDS = (GDALDataset *) GDALOpen( pszBaseFilename, GA_Update );
    CPLFree( pszBaseFilename );
    
    return poDS;
}

/************************************************************************/
/*                             CreateCopy()                             */
/************************************************************************/

GDALDataset *
MFFDataset::CreateCopy( const char * pszFilename, GDALDataset *poSrcDS, 
                        int bStrict, char ** papszOptions, 
                        GDALProgressFunc pfnProgress, void * pProgressData )

{
    MFFDataset	*poDS;
    GDALDataType eType = poSrcDS->GetRasterBand(1)->GetRasterDataType();
    int          iBand;
    char **newpapszOptions=NULL;

    if( !pfnProgress( 0.0, NULL, pProgressData ) )
        return NULL;

    /* check that other bands match type- sets type */
    /* to unknown if they differ.                  */
    for( iBand = 1; iBand < poSrcDS->GetRasterCount(); iBand++ )
     {
         GDALRasterBand *poBand = poSrcDS->GetRasterBand( iBand+1 );
         eType = GDALDataTypeUnion( eType, poBand->GetRasterDataType() );
     }

    newpapszOptions=CSLDuplicate(papszOptions);
    newpapszOptions=CSLSetNameValue(newpapszOptions,"NO_END","TRUE");

    poDS = (MFFDataset *) Create( pszFilename, 
                                  poSrcDS->GetRasterXSize(), 
                                  poSrcDS->GetRasterYSize(), 
                                  poSrcDS->GetRasterCount(), 
                                  eType, newpapszOptions );
    
    CSLDestroy(newpapszOptions);
   

    /* Check that Create worked- return Null if it didn't */
    if (poDS == NULL)
        return NULL;


/* -------------------------------------------------------------------- */
/*      Copy the image data.                                            */
/* -------------------------------------------------------------------- */
    int         nXSize = poDS->GetRasterXSize();
    int         nYSize = poDS->GetRasterYSize();
    int  	nBlockXSize, nBlockYSize, nBlockTotal, nBlocksDone;

    poDS->GetRasterBand(1)->GetBlockSize( &nBlockXSize, &nBlockYSize );

    nBlockTotal = ((nXSize + nBlockXSize - 1) / nBlockXSize)
        * ((nYSize + nBlockYSize - 1) / nBlockYSize)
        * poSrcDS->GetRasterCount();

    nBlocksDone = 0;
    for( iBand = 0; iBand < poSrcDS->GetRasterCount(); iBand++ )
    {
        GDALRasterBand *poSrcBand = poSrcDS->GetRasterBand( iBand+1 );
        GDALRasterBand *poDstBand = poDS->GetRasterBand( iBand+1 );
        int	       iYOffset, iXOffset;
        void           *pData;
        CPLErr  eErr;


        pData = CPLMalloc(nBlockXSize * nBlockYSize
                          * GDALGetDataTypeSize(eType) / 8);

        for( iYOffset = 0; iYOffset < nYSize; iYOffset += nBlockYSize )
        {
            for( iXOffset = 0; iXOffset < nXSize; iXOffset += nBlockXSize )
            {
                int	nTBXSize, nTBYSize;

                if( !pfnProgress( (nBlocksDone++) / (float) nBlockTotal,
                                  NULL, pProgressData ) )
                {
                    CPLError( CE_Failure, CPLE_UserInterrupt, 
                              "User terminated" );
                    delete poDS;

                    GDALDriver *poMFFDriver = 
                        (GDALDriver *) GDALGetDriverByName( "MFF" );
                    poMFFDriver->Delete( pszFilename );
                    return NULL;
                }

                nTBXSize = MIN(nBlockXSize,nXSize-iXOffset);
                nTBYSize = MIN(nBlockYSize,nYSize-iYOffset);

                eErr = poSrcBand->RasterIO( GF_Read, 
                                            iXOffset, iYOffset, 
                                            nTBXSize, nTBYSize,
                                            pData, nTBXSize, nTBYSize,
                                            eType, 0, 0 );
                if( eErr != CE_None )
                {
                    return NULL;
                }
            
                eErr = poDstBand->RasterIO( GF_Write, 
                                            iXOffset, iYOffset, 
                                            nTBXSize, nTBYSize,
                                            pData, nTBXSize, nTBYSize,
                                            eType, 0, 0 );

                if( eErr != CE_None )
                {
                    return NULL;
                }
            }
        }

        CPLFree( pData );
    }

/* -------------------------------------------------------------------- */
/*      Copy georeferencing information, if enough is available.        */
/* -------------------------------------------------------------------- */


/* -------------------------------------------------------------------- */
/*      Establish the base filename (path+filename, less extension).    */
/* -------------------------------------------------------------------- */
    char	*pszBaseFilename;
    int         i;
    FILE       *fp;
    const char *pszFilenameGEO;

    pszBaseFilename = (char *) CPLMalloc(strlen(pszFilename)+5);
    strcpy( pszBaseFilename, pszFilename );
    
    for( i = strlen(pszBaseFilename)-1; i > 0; i-- )
    {
        if( pszBaseFilename[i] == '.' )
        {
            pszBaseFilename[i] = '\0';
            break;
        }

        if( pszBaseFilename[i] == '/' || pszBaseFilename[i] == '\\' )
            break;
    }

    pszFilenameGEO = CPLFormFilename( NULL, pszBaseFilename, "hdr" );

    fp = VSIFOpen( pszFilenameGEO, "at" );
    if( fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Couldn't open %s for appending.\n", pszFilenameGEO );
        return NULL;
    }  
    

    /* MFF requires corner and center gcps */
    double	*padfTiepoints;
    int         src_prj;
    int         georef_created = FALSE;

    padfTiepoints = (double *) CPLMalloc(2*sizeof(double)*5);

    src_prj = GetMFFProjectionType(poSrcDS->GetProjectionRef());

    if ((src_prj != MFFPRJ_NONE) && (src_prj != MFFPRJ_UNRECOGNIZED))
    {
      double    *tempGeoTransform = NULL; 

      tempGeoTransform = (double *) CPLMalloc(6*sizeof(double));

      if (( poSrcDS->GetGeoTransform( tempGeoTransform ) == CE_None)
          && (tempGeoTransform[0] != 0.0 || tempGeoTransform[1] != 1.0
          || tempGeoTransform[2] != 0.0 || tempGeoTransform[3] != 0.0
              || tempGeoTransform[4] != 0.0 || ABS(tempGeoTransform[5]) != 1.0 ))
      {
          OGRSpatialReference oUTMorLL;
          OGRSpatialReference oLL;
          OGRCoordinateTransformation *poTransform = NULL;          
          char *srcProjection=NULL;
          char *newGCPProjection=NULL;

          padfTiepoints[0]=tempGeoTransform[0] + tempGeoTransform[1]*0.5 +\
                           tempGeoTransform[2]*0.5;

          padfTiepoints[1]=tempGeoTransform[3] + tempGeoTransform[4]*0.5 +\
                           tempGeoTransform[5]*0.5;

          padfTiepoints[2]=tempGeoTransform[0] + tempGeoTransform[2]*0.5 +\
                           tempGeoTransform[1]*(poSrcDS->GetRasterXSize()-0.5);

          padfTiepoints[3]=tempGeoTransform[3] + tempGeoTransform[5]*0.5 +\
                           tempGeoTransform[4]*(poSrcDS->GetRasterXSize()-0.5);

          padfTiepoints[4]=tempGeoTransform[0] + tempGeoTransform[1]*0.5 +\
                           tempGeoTransform[2]*(poSrcDS->GetRasterYSize()-0.5);

          padfTiepoints[5]=tempGeoTransform[3] + tempGeoTransform[4]*0.5 +\
                           tempGeoTransform[5]*(poSrcDS->GetRasterYSize()-0.5);

          padfTiepoints[6]=tempGeoTransform[0] +\
                           tempGeoTransform[1]*(poSrcDS->GetRasterXSize()-0.5) +\
                           tempGeoTransform[2]*(poSrcDS->GetRasterYSize()-0.5);

          padfTiepoints[7]=tempGeoTransform[3]+\
                           tempGeoTransform[4]*(poSrcDS->GetRasterXSize()-0.5)+\
                           tempGeoTransform[5]*(poSrcDS->GetRasterYSize()-0.5);

          padfTiepoints[8]=tempGeoTransform[0]+\
                           tempGeoTransform[1]*(poSrcDS->GetRasterXSize())/2.0+\
                           tempGeoTransform[2]*(poSrcDS->GetRasterYSize())/2.0;

          padfTiepoints[9]=tempGeoTransform[3]+\
                           tempGeoTransform[4]*(poSrcDS->GetRasterXSize())/2.0+\
                           tempGeoTransform[5]*(poSrcDS->GetRasterYSize())/2.0;

          srcProjection = CPLStrdup(poSrcDS->GetProjectionRef());
          oUTMorLL.importFromWkt(&srcProjection);
          (oUTMorLL.GetAttrNode("GEOGCS"))->exportToWkt(&newGCPProjection);
          oLL.importFromWkt(&newGCPProjection);
          if EQUALN(poSrcDS->GetProjectionRef(),"PROJCS",6)
          {
            // projected coordinate system- need to translate gcps */
            int bSuccess=TRUE;
            int index;

            poTransform = OGRCreateCoordinateTransformation( &oUTMorLL, &oLL );
            if( poTransform == NULL )
                bSuccess = FALSE;

            for (index=0;index<5;index++)
            {
                if( !bSuccess || !poTransform->Transform( 1, &(padfTiepoints[index*2]), &(padfTiepoints[index*2+1]) ) )
                  bSuccess = FALSE;
            }
            if (bSuccess == TRUE)
               georef_created = TRUE;
          }
          else
          {
            georef_created = TRUE;
          }
      }
      CPLFree(tempGeoTransform);
    } 
  
    if (georef_created == TRUE)
    {
          char *szValue;
      
          szValue = (char *) CPLMalloc(255);
    /* -------------------------------------------------------------------- */
    /*      top left                                                        */
    /* -------------------------------------------------------------------- */
          sprintf( szValue, "TOP_LEFT_CORNER_LATITUDE = %.10f\n", padfTiepoints[1] );
          fprintf( fp, szValue );
          sprintf( szValue, "TOP_LEFT_CORNER_LONGITUDE = %.10f\n", padfTiepoints[0] );
          fprintf( fp, szValue );
    /* -------------------------------------------------------------------- */
    /*      top_right                                                       */
    /* -------------------------------------------------------------------- */
          sprintf( szValue, "TOP_RIGHT_CORNER_LATITUDE = %.10f\n", padfTiepoints[3] );
          fprintf( fp, szValue );
          sprintf( szValue, "TOP_RIGHT_CORNER_LONGITUDE = %.10f\n", padfTiepoints[2] );
          fprintf( fp, szValue );
    /* -------------------------------------------------------------------- */
    /*      bottom_left                                                     */
    /* -------------------------------------------------------------------- */
          sprintf( szValue, "BOTTOM_LEFT_CORNER_LATITUDE = %.10f\n", padfTiepoints[5] );
          fprintf( fp, szValue );
          sprintf( szValue, "BOTTOM_LEFT_CORNER_LONGITUDE = %.10f\n", padfTiepoints[4] );
          fprintf( fp, szValue );
    /* -------------------------------------------------------------------- */
    /*      bottom_right                                                    */
    /* -------------------------------------------------------------------- */
          sprintf( szValue, "BOTTOM_RIGHT_CORNER_LATITUDE = %.10f\n", padfTiepoints[7] );
          fprintf( fp, szValue );
          sprintf( szValue, "BOTTOM_RIGHT_CORNER_LONGITUDE = %.10f\n", padfTiepoints[6] );
          fprintf( fp, szValue );
    /* -------------------------------------------------------------------- */
    /*      Center                                                          */
    /* -------------------------------------------------------------------- */
          sprintf( szValue, "CENTRE_LATITUDE = %.10f\n", padfTiepoints[9] );
          fprintf( fp, szValue );
          sprintf( szValue, "CENTRE_LONGITUDE = %.10f\n", padfTiepoints[8] );
          fprintf( fp, szValue );


          CPLFree(szValue);
    /* ------------------------------------------------------------------- */
    /*     Ellipsoid/projection                                            */
    /* --------------------------------------------------------------------*/

          
          OGRSpatialReference *oSRS;
          MFFSpheroidList *mffEllipsoids;
          double eq_radius, inv_flattening;
          OGRErr ogrerrorEq=OGRERR_NONE;
          OGRErr ogrerrorInvf=OGRERR_NONE;
          OGRErr ogrerrorOl=OGRERR_NONE;
          char *modifiableProjection = NULL;
          char *pszNewProjection;
          char *spheroid_name = NULL;

          pszNewProjection = CPLStrdup( poSrcDS->GetProjectionRef() );
 
          if( !EQUALN(pszNewProjection,"GEOGCS",6)
           && !EQUALN(pszNewProjection,"PROJCS",6)
           && !EQUAL(pszNewProjection,"") )
          {
            CPLError( CE_Warning, CPLE_AppDefined,
                    "Only OGC WKT Projections supported for writing to MFF.\n"
                    "%s not supported.",
                      pszNewProjection );
          }
          else if (!EQUAL(pszNewProjection,""))
          {
           

          /* importFromWkt updates the pointer, so don't use pszNewProjection directly */
             modifiableProjection=CPLStrdup(pszNewProjection);

             oSRS = new OGRSpatialReference;
             oSRS->importFromWkt(&modifiableProjection);

             if ((oSRS->GetAttrValue("PROJECTION") != NULL) && 
                 (EQUAL(oSRS->GetAttrValue("PROJECTION"),SRS_PT_TRANSVERSE_MERCATOR)))
             {
                 char *ol_txt;
    
                 ol_txt=(char *) CPLMalloc(255);
                 fprintf(fp,"PROJECTION_NAME = UTM\n");
                 sprintf(ol_txt,"PROJECTION_ORIGIN_LONGITUDE = %f\n",
                         oSRS->GetProjParm(SRS_PP_CENTRAL_MERIDIAN,0.0,&ogrerrorOl));
                 fprintf(fp,ol_txt);
                 CPLFree(ol_txt);
             }
             else if ((oSRS->GetAttrValue("PROJECTION") == NULL) && (oSRS->IsGeographic()))
             {
                  fprintf(fp,"PROJECTION_NAME = LL\n");
             }
             else
             {
                  CPLError( CE_Warning, CPLE_AppDefined,
                  "Unrecognized projection- no georeferencing information transferred.");
                  fprintf(fp,"PROJECTION_NAME = LL\n");
             }
             eq_radius = oSRS->GetSemiMajor(&ogrerrorEq);
             inv_flattening = oSRS->GetInvFlattening(&ogrerrorInvf);
             if ((ogrerrorEq == OGRERR_NONE) && (ogrerrorInvf == OGRERR_NONE)) 
             {
                 char *ol_txt;
    
                 ol_txt=(char *) CPLMalloc(255);
                 mffEllipsoids = new MFFSpheroidList;
                 spheroid_name = mffEllipsoids->GetSpheroidNameByEqRadiusAndInvFlattening(eq_radius,inv_flattening);
                 if (spheroid_name != NULL)
                 {
                     sprintf(ol_txt,"SPHEROID_NAME = %s\n",spheroid_name );
                     fprintf(fp,ol_txt);
                 } 
                 else
                 {
                     sprintf(ol_txt,
       "SPHEROID_NAME = USER_DEFINED\nSPHEROID_EQUATORIAL_RADIUS = %.10f\nSPHEROID_POLAR_RADIUS = %.10f\n",
                     eq_radius,eq_radius*(1-1.0/inv_flattening) );
                     fprintf(fp,ol_txt);
                 }
                 delete mffEllipsoids;
                 CPLFree(ol_txt);
                 CPLFree(spheroid_name);
              }
          } 
          CPLFree(pszNewProjection);         
    } 
      
    CPLFree( padfTiepoints );
    fprintf( fp, "END\n" );
    VSIFClose( fp );
   
    /* End of georeferencing stuff */

    /* Make sure image data gets flushed */
    for( iBand = 0; iBand < poDS->GetRasterCount(); iBand++ )
    {
        RawRasterBand *poDstBand =  (RawRasterBand *) poDS->GetRasterBand( iBand+1 );
        poDstBand->FlushCache();
    }


    if( !pfnProgress( 1.0, NULL, pProgressData ) )
    {
        CPLError( CE_Failure, CPLE_UserInterrupt, 
                  "User terminated" );
        delete poDS;

        GDALDriver *poMFFDriver = 
            (GDALDriver *) GDALGetDriverByName( "MFF" );
        poMFFDriver->Delete( pszFilename );
        return NULL;
    }

    poDS->CloneInfo( poSrcDS, GCIF_PAM_DEFAULT );

    return poDS;
}


/************************************************************************/
/*                         GDALRegister_MFF()                          */
/************************************************************************/

void GDALRegister_MFF()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "MFF" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "MFF" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Atlantis MFF Raster" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#MFF" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "hdr" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES, 
                                   "Byte UInt16 Float32 CInt16 CFloat32" );

        poDriver->pfnOpen = MFFDataset::Open;
        poDriver->pfnCreate = MFFDataset::Create;
        poDriver->pfnCreateCopy = MFFDataset::CreateCopy;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

