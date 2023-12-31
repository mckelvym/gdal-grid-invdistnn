/******************************************************************************
 * $Id$
 *
 * Project:  BIGGIF Driver
 * Purpose:  Implement GDAL support for reading large GIF files in a 
 *           streaming fashion rather than the slurp-into-memory approach
 *           of the normal GIF driver.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001-2008, Frank Warmerdam <warmerdam@pobox.com>
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

#include "gdal_pam.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

CPL_C_START
#include "gif_lib.h"
CPL_C_END

CPL_C_START
void	GDALRegister_BIGGIF(void);
CPL_C_END

static const int InterlacedOffset[] = { 0, 4, 2, 1 }; 
static const int InterlacedJumps[] = { 8, 8, 4, 2 };  

static int VSIGIFReadFunc( GifFileType *, GifByteType *, int);

/************************************************************************/
/* ==================================================================== */
/*				BIGGIFDataset				*/
/* ==================================================================== */
/************************************************************************/

class BIGGifRasterBand;

class BIGGIFDataset : public GDALPamDataset
{
    friend class BIGGifRasterBand;

    FILE *fp;

    GifFileType *hGifFile;
    int         nLastLineRead;

    int	   bGeoTransformValid;
    double adfGeoTransform[6];

    GDALDataset *poWorkDS;

    CPLErr       ReOpen();

  public:
                 BIGGIFDataset();
                 ~BIGGIFDataset();

    virtual CPLErr GetGeoTransform( double * );
    static GDALDataset *Open( GDALOpenInfo * );
    static int          Identify( GDALOpenInfo * );
};

/************************************************************************/
/* ==================================================================== */
/*                            BIGGifRasterBand                          */
/* ==================================================================== */
/************************************************************************/

class BIGGifRasterBand : public GDALPamRasterBand
{
    friend class BIGGIFDataset;

    int		*panInterlaceMap;
    
    GDALColorTable *poColorTable;

  public:

                   BIGGifRasterBand( BIGGIFDataset *, int );
    virtual       ~BIGGifRasterBand();

    virtual CPLErr IReadBlock( int, int, void * );

    virtual GDALColorInterp GetColorInterpretation();
    virtual GDALColorTable *GetColorTable();
};

/************************************************************************/
/*                          BIGGifRasterBand()                          */
/************************************************************************/

BIGGifRasterBand::BIGGifRasterBand( BIGGIFDataset *poDS, int nBackground )

{
    SavedImage *psImage = poDS->hGifFile->SavedImages + 0;

    this->poDS = poDS;
    this->nBand = 1;

    eDataType = GDT_Byte;

    nBlockXSize = poDS->nRasterXSize;
    nBlockYSize = 1;

/* -------------------------------------------------------------------- */
/*      Setup interlacing map if required.                              */
/* -------------------------------------------------------------------- */
    panInterlaceMap = NULL;
    if( psImage->ImageDesc.Interlace )
    {
        int	i, j, iLine = 0;

        poDS->SetMetadataItem( "INTERLACED", "YES", "IMAGE_STRUCTURE" );

        panInterlaceMap = (int *) CPLCalloc(poDS->nRasterYSize,sizeof(int));

	for (i = 0; i < 4; i++)
        {
	    for (j = InterlacedOffset[i]; 
                 j < poDS->nRasterYSize;
                 j += InterlacedJumps[i]) 
                panInterlaceMap[j] = iLine++;
        }
    }
    else
        poDS->SetMetadataItem( "INTERLACED", "NO", "IMAGE_STRUCTURE" );

/* -------------------------------------------------------------------- */
/*      Setup colormap.                                                 */
/* -------------------------------------------------------------------- */
    ColorMapObject 	*psGifCT = psImage->ImageDesc.ColorMap;
    if( psGifCT == NULL )
        psGifCT = poDS->hGifFile->SColorMap;

    poColorTable = new GDALColorTable();
    for( int iColor = 0; iColor < psGifCT->ColorCount; iColor++ )
    {
        GDALColorEntry	oEntry;

        oEntry.c1 = psGifCT->Colors[iColor].Red;
        oEntry.c2 = psGifCT->Colors[iColor].Green;
        oEntry.c3 = psGifCT->Colors[iColor].Blue;
        oEntry.c4 = 255;

        poColorTable->SetColorEntry( iColor, &oEntry );
    }

/* -------------------------------------------------------------------- */
/*      If we have a background value, return it here.  Some            */
/*      applications might want to treat this as transparent, but in    */
/*      many uses this is inappropriate so we don't return it as        */
/*      nodata or transparent.                                          */
/* -------------------------------------------------------------------- */
    if( nBackground != 255 )
    {
        char szBackground[10];
        
        sprintf( szBackground, "%d", nBackground );
        SetMetadataItem( "GIF_BACKGROUND", szBackground );
    }
}

/************************************************************************/
/*                           ~BIGGifRasterBand()                           */
/************************************************************************/

BIGGifRasterBand::~BIGGifRasterBand()

{
    if( poColorTable != NULL )
        delete poColorTable;

    CPLFree( panInterlaceMap );
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr BIGGifRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )

{
    BIGGIFDataset *poGDS = (BIGGIFDataset *) poDS;

    CPLAssert( nBlockXOff == 0 );

    if( panInterlaceMap != NULL )
        nBlockYOff = panInterlaceMap[nBlockYOff];

/* -------------------------------------------------------------------- */
/*      Do we already have this line in the work dataset?               */
/* -------------------------------------------------------------------- */
    if( poGDS->poWorkDS != NULL && nBlockYOff <= poGDS->nLastLineRead )
    {
        return poGDS->poWorkDS->
            RasterIO( GF_Read, 0, nBlockYOff, nBlockXSize, 1, 
                      pImage, nBlockXSize, 1, GDT_Byte, 
                      1, NULL, 0, 0, 0 );
    }

/* -------------------------------------------------------------------- */
/*      Do we need to restart from the start of the image?              */
/* -------------------------------------------------------------------- */
    if( nBlockYOff <= poGDS->nLastLineRead )
    {
        if( poGDS->ReOpen() == CE_Failure )
            return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Read till we get our target line.                               */
/* -------------------------------------------------------------------- */
    while( poGDS->nLastLineRead < nBlockYOff )
    {
        if( DGifGetLine( poGDS->hGifFile, (GifPixelType*)pImage, 
                         nBlockXSize ) == GIF_ERROR )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Failure decoding scanline of GIF file." );
            return CE_Failure;
        }

        poGDS->nLastLineRead++;

        if( poGDS->poWorkDS != NULL )
        {
            poGDS->poWorkDS->RasterIO( GF_Write, 
                                       0, poGDS->nLastLineRead, nBlockXSize, 1, 
                                       pImage, nBlockXSize, 1, GDT_Byte, 
                                       1, NULL, 0, 0, 0 );
        }
    }

    return CE_None;
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp BIGGifRasterBand::GetColorInterpretation()

{
    return GCI_PaletteIndex;
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

GDALColorTable *BIGGifRasterBand::GetColorTable()

{
    return poColorTable;
}

/************************************************************************/
/* ==================================================================== */
/*                             BIGGIFDataset                            */
/* ==================================================================== */
/************************************************************************/


/************************************************************************/
/*                            BIGGIFDataset()                            */
/************************************************************************/

BIGGIFDataset::BIGGIFDataset()

{
    hGifFile = NULL;
    fp = NULL;
    bGeoTransformValid = FALSE;
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;
    nLastLineRead = -1;
    poWorkDS = NULL;
}

/************************************************************************/
/*                           ~BIGGIFDataset()                            */
/************************************************************************/

BIGGIFDataset::~BIGGIFDataset()

{
    FlushCache();
    if( hGifFile )
        DGifCloseFile( hGifFile );
    if( fp != NULL )
        VSIFCloseL( fp );

    if( poWorkDS != NULL )
    {
        CPLString osTempFilename = poWorkDS->GetDescription();

        GDALClose( (GDALDatasetH) poWorkDS );
        poWorkDS = NULL;

        GDALDriver *poGTiff = (GDALDriver *) GDALGetDriverByName( "GTiff" );
        poGTiff->Delete( osTempFilename );
    }
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr BIGGIFDataset::GetGeoTransform( double * padfTransform )

{
    if( bGeoTransformValid )
    {
        memcpy( padfTransform, adfGeoTransform, sizeof(double)*6 );
        return CE_None;
    }
    else
        return GDALPamDataset::GetGeoTransform( padfTransform );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

int BIGGIFDataset::Identify( GDALOpenInfo * poOpenInfo )

{
    if( poOpenInfo->nHeaderBytes < 8 )
        return FALSE;

    if( strncmp((const char *) poOpenInfo->pabyHeader, "GIF87a",5) != 0
        && strncmp((const char *) poOpenInfo->pabyHeader, "GIF89a",5) != 0 )
        return FALSE;

    return TRUE;
}

/************************************************************************/
/*                               ReOpen()                               */
/*                                                                      */
/*      (Re)Open the gif file and process past the first image          */
/*      descriptor.                                                     */
/************************************************************************/

CPLErr BIGGIFDataset::ReOpen()

{
/* -------------------------------------------------------------------- */
/*      If the file is already open, close it so we can restart.        */
/* -------------------------------------------------------------------- */
    if( hGifFile != NULL )
        DGifCloseFile( hGifFile );

/* -------------------------------------------------------------------- */
/*      If we are actually reopening, then we assume that access to     */
/*      the image data is not strictly once through sequential, and     */
/*      we will try to create a working database in a temporary         */
/*      directory to hold the image as we read through it the second    */
/*      time.                                                           */
/* -------------------------------------------------------------------- */
    if( hGifFile != NULL )
    {
        GDALDriver *poGTiffDriver = (GDALDriver*) GDALGetDriverByName("GTiff");
        
        if( poGTiffDriver != NULL )
        {
            /* Create as a sparse file to avoid filling up the whole file */
            /* while closing and then destroying this temporary dataset */
            char *apszOptions[] = { "COMPRESS=LZW", "SPARSE_OK=YES", NULL };
            CPLString osTempFilename = CPLGenerateTempFilename("biggif");

            osTempFilename += ".tif";

            poWorkDS = poGTiffDriver->Create( osTempFilename, 
                                              nRasterXSize, nRasterYSize, 1, 
                                              GDT_Byte, apszOptions );
        }
    }

/* -------------------------------------------------------------------- */
/*      Open                                                            */
/* -------------------------------------------------------------------- */
    VSIFSeekL( fp, 0, SEEK_SET );

    nLastLineRead = -1;
    hGifFile = DGifOpen( fp, VSIGIFReadFunc );
    if( hGifFile == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "DGifOpen() failed.  Perhaps the gif file is corrupt?\n" );

        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Find the first image record.                                    */
/* -------------------------------------------------------------------- */
    GifRecordType RecordType = TERMINATE_RECORD_TYPE;

    while( DGifGetRecordType(hGifFile, &RecordType) != GIF_ERROR
           && RecordType != TERMINATE_RECORD_TYPE
           && RecordType != IMAGE_DESC_RECORD_TYPE ) {}

    if( RecordType != IMAGE_DESC_RECORD_TYPE )
    {
        DGifCloseFile( hGifFile );
        hGifFile = NULL;

        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Failed to find image description record in GIF file." );
        return CE_Failure;
    }
    
    if (DGifGetImageDesc(hGifFile) == GIF_ERROR)
    {
        DGifCloseFile( hGifFile );
        hGifFile = NULL;

        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Image description reading failed in GIF file." );
        return CE_Failure;
    }
    
    return CE_None;
}


/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *BIGGIFDataset::Open( GDALOpenInfo * poOpenInfo )

{
    if( !Identify( poOpenInfo ) )
        return NULL;

    if( poOpenInfo->eAccess == GA_Update )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "The GIF driver does not support update access to existing"
                  " files.\n" );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    FILE                *fp;

    fp = VSIFOpenL( poOpenInfo->pszFilename, "r" );
    if( fp == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    BIGGIFDataset 	*poDS;

    poDS = new BIGGIFDataset();

    poDS->fp = fp;
    poDS->eAccess = GA_ReadOnly;
    if( poDS->ReOpen() == CE_Failure )
    {
        delete poDS;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Capture some information from the file that is of interest.     */
/* -------------------------------------------------------------------- */
    
    poDS->nRasterXSize = poDS->hGifFile->SavedImages[0].ImageDesc.Width;
    poDS->nRasterYSize = poDS->hGifFile->SavedImages[0].ImageDesc.Height;

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->SetBand( 1, 
                   new BIGGifRasterBand( poDS, 
                                         poDS->hGifFile->SBackGroundColor ));

/* -------------------------------------------------------------------- */
/*      Check for world file.                                           */
/* -------------------------------------------------------------------- */
    poDS->bGeoTransformValid = 
        GDALReadWorldFile( poOpenInfo->pszFilename, NULL, 
                           poDS->adfGeoTransform )
        || GDALReadWorldFile( poOpenInfo->pszFilename, ".wld", 
                              poDS->adfGeoTransform );

/* -------------------------------------------------------------------- */
/*      Initialize any PAM information.                                 */
/* -------------------------------------------------------------------- */
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML();

/* -------------------------------------------------------------------- */
/*      Support overviews.                                              */
/* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );

    return poDS;
}

/************************************************************************/
/*                           VSIGIFReadFunc()                           */
/*                                                                      */
/*      Proxy function for reading from GIF file.                       */
/************************************************************************/

static int VSIGIFReadFunc( GifFileType *psGFile, GifByteType *pabyBuffer, 
                           int nBytesToRead )

{
    return VSIFReadL( pabyBuffer, 1, nBytesToRead, 
                      (FILE *) psGFile->UserData );
}

/************************************************************************/
/*                        GDALRegister_BIGGIF()                         */
/************************************************************************/

void GDALRegister_BIGGIF()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "BIGGIF" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "BIGGIF" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Graphics Interchange Format (.gif)" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_gif.html" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "gif" );
        poDriver->SetMetadataItem( GDAL_DMD_MIMETYPE, "image/gif" );
        poDriver->SetMetadataItem( GDAL_DCAP_VIRTUALIO, "YES" );

        poDriver->pfnOpen = BIGGIFDataset::Open;
        poDriver->pfnIdentify = BIGGIFDataset::Identify;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

