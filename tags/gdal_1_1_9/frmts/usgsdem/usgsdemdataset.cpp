/******************************************************************************
 * $Id$
 *
 * Project:  USGS DEM Reader
 * Purpose:  All code for USGS DEM Reader
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 * Portions of this module derived from the VTP USGS DEM driver by Ben
 * Discoe, see http://www.vterrain.org
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.7  2002/11/25 15:27:00  warmerda
 * relax testopen restrictions to work with adams.dem - PA NED data
 *
 * Revision 1.6  2002/09/04 06:50:37  warmerda
 * avoid static driver pointers
 *
 * Revision 1.5  2002/08/26 06:45:54  warmerda
 * removed use of bool
 *
 * Revision 1.4  2002/06/12 21:12:25  warmerda
 * update to metadata based driver info
 *
 * Revision 1.3  2001/11/27 16:09:26  warmerda
 * Added credit notes
 *
 * Revision 1.2  2001/11/27 15:16:53  warmerda
 * Added header check before trying to open.
 *
 * Revision 1.1  2001/11/27 14:45:18  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "ogr_spatialref.h"

CPL_CVSID("$Id$");

CPL_C_START
void	GDALRegister_USGSDEM(void);
CPL_C_END

typedef struct {
    double	x;
    double	y;
} DPoint2;

#define USGSDEM_NODATA	-32000

/************************************************************************/
/*                              DConvert()                              */
/************************************************************************/

static double DConvert( FILE *fp, int nCharCount )

{
    char	szBuffer[100];
    int		i;

    VSIFRead( szBuffer, nCharCount, 1, fp );
    szBuffer[nCharCount] = '\0';

    for( i = 0; i < nCharCount; i++ )
    {
        if( szBuffer[i] == 'D' )
            szBuffer[i] = 'E';
    }

    return atof(szBuffer);
}

/************************************************************************/
/* ==================================================================== */
/*				USGSDEMDataset				*/
/* ==================================================================== */
/************************************************************************/

class USGSDEMRasterBand;

class USGSDEMDataset : public GDALDataset
{
    friend class USGSDEMRasterBand;

    int         nDataStartOffset;
    GDALDataType eNaturalDataFormat;

    double      adfGeoTransform[6];
    char        *pszProjection; 

    double      fVRes;

    const char  *pszUnits; 

    int         LoadFromFile( FILE * );

    FILE	*fp;

  public:
                USGSDEMDataset();
		~USGSDEMDataset();
    
    static GDALDataset *Open( GDALOpenInfo * );

    CPLErr 	GetGeoTransform( double * padfTransform );
    const char *GetProjectionRef();

};

/************************************************************************/
/* ==================================================================== */
/*                            USGSDEMRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class USGSDEMRasterBand : public GDALRasterBand
{
    friend class USGSDEMDataset;

  public:

    		USGSDEMRasterBand( USGSDEMDataset * );
    
    virtual const char *GetUnitType();
    virtual double GetNoDataValue( int *pbSuccess = NULL );
    virtual CPLErr IReadBlock( int, int, void * );
};


/************************************************************************/
/*                           USGSDEMRasterBand()                            */
/************************************************************************/

USGSDEMRasterBand::USGSDEMRasterBand( USGSDEMDataset *poDS )

{
    this->poDS = poDS;
    this->nBand = 1;

    eDataType = poDS->eNaturalDataFormat;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = poDS->GetRasterYSize();

}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr USGSDEMRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                      void * pImage )

{
    double	dfYMin;
    int		bad = FALSE;
    USGSDEMDataset *poGDS = (USGSDEMDataset *) poDS;

/* -------------------------------------------------------------------- */
/*      Initialize image buffer to nodata value.                        */
/* -------------------------------------------------------------------- */
    for( int k = GetXSize() * GetYSize() - 1; k >= 0; k-- )
    {
        if( GetRasterDataType() == GDT_Int16 )
            ((GInt16 *) pImage)[k] = USGSDEM_NODATA;
        else
            ((float *) pImage)[k] = USGSDEM_NODATA;
    }

/* -------------------------------------------------------------------- */
/*      Seek to data.                                                   */
/* -------------------------------------------------------------------- */
    VSIFSeek(poGDS->fp, poGDS->nDataStartOffset, 0);

    dfYMin = poGDS->adfGeoTransform[3] 
        + (GetYSize()-0.5) * poGDS->adfGeoTransform[5];

/* -------------------------------------------------------------------- */
/*      Read all the profiles into the image buffer.                    */
/* -------------------------------------------------------------------- */
    for( int i = 0; i < GetXSize(); i++)
    {
        int	njunk, nCPoints, lygap;
        double	djunk, dxStart, dyStart;

        fscanf(poGDS->fp, "%d", &njunk);
        fscanf(poGDS->fp, "%d", &njunk);
        fscanf(poGDS->fp, "%d", &nCPoints);
        fscanf(poGDS->fp, "%d", &njunk);

        dxStart = DConvert(poGDS->fp, 24);
        dyStart = DConvert(poGDS->fp, 24);
        djunk = DConvert(poGDS->fp, 24);
        djunk = DConvert(poGDS->fp, 24);
        djunk = DConvert(poGDS->fp, 24);

        if( strstr(poGDS->pszProjection,"PROJCS") == NULL )
            dyStart = dyStart / 3600.0;

        lygap = (int)((dfYMin - dyStart)/poGDS->adfGeoTransform[5]+ 0.5);

        for (int j=lygap; j < (nCPoints+(int)lygap); j++)
        {
            int		iY = GetYSize() - j - 1;
            int         nElev;

            fscanf(poGDS->fp, "%d", &nElev);
            if (iY < 0 || iY >= GetYSize() )
                bad = TRUE;
            else
            {
                if( GetRasterDataType() == GDT_Int16 )
                {
                    ((GInt16 *) pImage)[i + iY*GetXSize()] = 
                        (int) (nElev * poGDS->fVRes);
                }
                else
                {
                    ((float *) pImage)[i + iY*GetXSize()] = 
                        nElev * poGDS->fVRes;
                }
            }
        }
    }

    return CE_None;
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

double USGSDEMRasterBand::GetNoDataValue( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = TRUE;

    return USGSDEM_NODATA;
}

/************************************************************************/
/*                            GetUnitType()                             */
/************************************************************************/
const char *USGSDEMRasterBand::GetUnitType()
{
    USGSDEMDataset *poGDS = (USGSDEMDataset *) poDS;

    return poGDS->pszUnits;
}

/************************************************************************/
/* ==================================================================== */
/*				USGSDEMDataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           USGSDEMDataset()                           */
/************************************************************************/

USGSDEMDataset::USGSDEMDataset()

{
    fp = NULL;
    pszProjection = NULL;
}

/************************************************************************/
/*                            ~USGSDEMDataset()                         */
/************************************************************************/

USGSDEMDataset::~USGSDEMDataset()

{
    CPLFree( pszProjection );
    if( fp != NULL )
        VSIFClose( fp );
}

/************************************************************************/
/*                            LoadFromFile()                            */
/*                                                                      */
/*      If the data from DEM is in meters, then values are stored as    */
/*      shorts. If DEM data is in feet, then height data will be        */
/*      stored in float, to preserve the precision of the original      */
/*      data. returns true if the file was successfully opened and      */
/*      read.                                                           */
/************************************************************************/

int USGSDEMDataset::LoadFromFile(FILE *InDem)
{
    int		i, j;
    int		nRow, nColumn;
    int		nVUnit, nGUnit;
    double 	dxdelta, dydelta;
    double	dElevMax, dElevMin;
    int 	bNewFormat;
    int		nCoordSystem;
    int		nProfiles;
    char	szDateBuffer[5];
    DPoint2	corners[4];			// SW, NW, NE, SE
    DPoint2	extent_min, extent_max;
    int		iUTMZone;

    // check for version of DEM format
    VSIFSeek(InDem, 864, 0);

    // Read DEM into matrix
    fscanf(InDem, "%d", &nRow);
    fscanf(InDem, "%d", &nColumn);
    bNewFormat = ((nRow!=1)||(nColumn!=1));
    if (bNewFormat)
    {
        VSIFSeek(InDem, 1024, 0); 	// New Format
        fscanf(InDem, "%d", &i);
        fscanf(InDem, "%d", &j);
        if ((i!=1)||(j!=1))			// File OK?
        {
            VSIFSeek(InDem, 893, 0); 	// Undocumented Format
            fscanf(InDem, "%d", &i);
            fscanf(InDem, "%d", &j);
            if ((i!=1)||(j!=1))			// File OK?
            {
                CPLError( CE_Failure, CPLE_AppDefined,
                          "Does not appear to be a USGS DEM file." );
                return FALSE;
            }
            else
                nDataStartOffset = 893;
        }
        else
            nDataStartOffset = 1024;
    }
    else
        nDataStartOffset = 864;

    VSIFSeek(InDem, 156, 0);
    fscanf(InDem, "%d", &nCoordSystem);
    fscanf(InDem, "%d", &iUTMZone);

    VSIFSeek(InDem, 528, 0);
    fscanf(InDem, "%d", &nGUnit);
    fscanf(InDem, "%d", &nVUnit);

    // Vertical Units in meters
    if (nVUnit==1)
        pszUnits = "ft";
    else
        pszUnits = "m";

    VSIFSeek(InDem, 816, 0);
    dxdelta = DConvert(InDem, 12);
    dydelta = DConvert(InDem, 12);
    fVRes = DConvert(InDem, 12);

/* -------------------------------------------------------------------- */
/*      Should we treat this as floating point, or GInt16.              */
/* -------------------------------------------------------------------- */
    if (nVUnit==1 || fVRes < 1.0)
        eNaturalDataFormat = GDT_Float32;
    else
        eNaturalDataFormat = GDT_Int16;

/* -------------------------------------------------------------------- */
/*      Read four corner coordinates.                                   */
/* -------------------------------------------------------------------- */
    VSIFSeek(InDem, 546, 0);
    for (i = 0; i < 4; i++)
    {
        corners[i].x = DConvert(InDem, 24);
        corners[i].y = DConvert(InDem, 24);
    }
    
    // find absolute extents of raw vales
    extent_min.x = MIN(corners[0].x, corners[1].x);
    extent_max.x = MAX(corners[2].x, corners[3].x);
    extent_min.y = MIN(corners[0].y, corners[3].y);
    extent_max.y = MAX(corners[1].y, corners[2].y);

    dElevMin = DConvert(InDem, 48);
    dElevMax = DConvert(InDem, 48);

    VSIFSeek(InDem, 858, 0);
    fscanf(InDem, "%d", &nProfiles);

/* -------------------------------------------------------------------- */
/*      Collect the spatial reference system.                           */
/* -------------------------------------------------------------------- */
    OGRSpatialReference sr;

    // OLD format header ends at byte 864
    if (bNewFormat)
    {
        // year of data compilation
        VSIFSeek(InDem, 876, 0);
        fread(szDateBuffer, 4, 1, InDem);
        szDateBuffer[4] = 0;

        // Horizontal datum
        // 1=North American Datum 1927 (NAD 27)
        // 2=World Geodetic System 1972 (WGS 72)
        // 3=WGS 84
        // 4=NAD 83
        // 5=Old Hawaii Datum
        // 6=Puerto Rico Datum
        int datum;
        VSIFSeek(InDem, 890, 0);
        fscanf(InDem, "%d", &datum);
        switch (datum)
        {
          case 1:
            sr.SetWellKnownGeogCS( "NAD27" );
            break;

          case 2:
            sr.SetWellKnownGeogCS( "WGS72" );
            break;

          case 3:
            sr.SetWellKnownGeogCS( "WGS84" );
            break;

          case 4:
            sr.SetWellKnownGeogCS( "NAD83" );
            break;

          default:
            sr.SetWellKnownGeogCS( "NAD27" );
            break;
        }
    }
    else
        sr.SetWellKnownGeogCS( "NAD27" );

    if (nCoordSystem == 1)	// UTM
        sr.SetUTM( iUTMZone, TRUE );

    sr.exportToWkt( &pszProjection );

/* -------------------------------------------------------------------- */
/*      For UTM we use the extents (really the UTM coordinates of       */
/*      the lat/long corners of the quad) to determine the size in      */
/*      pixels and lines, but we have to make the anchors be modulus    */
/*      the pixel size which what really gets used.                     */
/* -------------------------------------------------------------------- */
    if (nCoordSystem == 1)	// UTM
    {
        int	njunk;
        double  dxStart;

        // expand extents modulus the pixel size.
        extent_min.y = floor(extent_min.y/dydelta) * dydelta;
        extent_max.y = ceil(extent_max.y/dydelta) * dydelta;

        // Forceably compute X extents based on first profile and pixelsize.
        VSIFSeek(InDem, nDataStartOffset, 0);
        fscanf(InDem, "%d", &njunk);
        fscanf(InDem, "%d", &njunk);
        fscanf(InDem, "%d", &njunk);
        fscanf(InDem, "%d", &njunk);
        dxStart = DConvert(InDem, 24);
        
        nRasterYSize = (int) ((extent_max.y - extent_min.y)/dydelta + 1.5);
        nRasterXSize = nProfiles;

        adfGeoTransform[0] = dxStart - dxdelta/2.0;
        adfGeoTransform[1] = dxdelta;
        adfGeoTransform[2] = 0.0;
        adfGeoTransform[3] = extent_max.y + dydelta/2.0;
        adfGeoTransform[4] = 0.0;
        adfGeoTransform[5] = -dydelta;
    }
/* -------------------------------------------------------------------- */
/*      Geographic -- use corners directly.                             */
/* -------------------------------------------------------------------- */
    else
    {
        nRasterYSize = (int) ((extent_max.y - extent_min.y)/dydelta + 1.5);
        nRasterXSize = nProfiles;

        // Translate extents from arc-seconds to decimal degrees.
        adfGeoTransform[0] = (extent_min.x - dxdelta/2.0) / 3600.0;
        adfGeoTransform[1] = dxdelta / 3600.0;
        adfGeoTransform[2] = 0.0;
        adfGeoTransform[3] = (extent_max.y + dydelta/2.0) / 3600.0;
        adfGeoTransform[4] = 0.0;
        adfGeoTransform[5] = (-dydelta) / 3600.0;
    }

    return TRUE;
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr USGSDEMDataset::GetGeoTransform( double * padfTransform )

{
    memcpy( padfTransform, adfGeoTransform, sizeof(double) * 6 );
    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *USGSDEMDataset::GetProjectionRef()

{
    return pszProjection;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *USGSDEMDataset::Open( GDALOpenInfo * poOpenInfo )

{
    if( poOpenInfo->fp == NULL || poOpenInfo->nHeaderBytes < 200 )
        return NULL;

    if( !EQUALN((const char *) poOpenInfo->pabyHeader+156, "     0",6)
        && !EQUALN((const char *) poOpenInfo->pabyHeader+156, "     1",6)
        && !EQUALN((const char *) poOpenInfo->pabyHeader+156, "     2",6) 
        && !EQUALN((const char *) poOpenInfo->pabyHeader+156, "     3",6) )
        return NULL;

    if( !EQUALN((const char *) poOpenInfo->pabyHeader+150, "     1",6) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    USGSDEMDataset 	*poDS;

    poDS = new USGSDEMDataset();

    poDS->fp = poOpenInfo->fp;
    poOpenInfo->fp = NULL;
    
/* -------------------------------------------------------------------- */
/*	Read the file.							*/
/* -------------------------------------------------------------------- */
    if( !poDS->LoadFromFile( poDS->fp ) )
    {
        delete poDS;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->SetBand( 1, new USGSDEMRasterBand( poDS ));

    return( poDS );
}

/************************************************************************/
/*                        GDALRegister_USGSDEM()                        */
/************************************************************************/

void GDALRegister_USGSDEM()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "USGSDEM" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "USGSDEM" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "USGS Optional ASCII DEM" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#USGSDEM" );
        
        poDriver->pfnOpen = USGSDEMDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
