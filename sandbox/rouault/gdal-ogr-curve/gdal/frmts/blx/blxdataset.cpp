/******************************************************************************
 * $Id$
 *
 * Project:  BLX Driver
 * Purpose:  GDAL BLX support.
 * Author:   Henrik Johansson, henrik@johome.net
 *
 ******************************************************************************
 * Copyright (c) 2006, Henrik Johansson <henrik@johome.net>
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
*/

#include "gdal_pam.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

CPL_C_START
#include <blx.h>
CPL_C_END

CPL_C_START
void    GDALRegister_BLX(void);
CPL_C_END

class BLXDataset : public GDALDataset
{
    friend class BLXRasterBand;

    CPLErr      GetGeoTransform( double * padfTransform );
    const char *GetProjectionRef();

    blxcontext_t *blxcontext;

    int	nOverviewCount;
    int	bIsOverview;
    BLXDataset *papoOverviewDS[BLX_OVERVIEWLEVELS];

  public:
    BLXDataset();
    ~BLXDataset();

    static GDALDataset *Open( GDALOpenInfo * );
};

class BLXRasterBand : public GDALRasterBand
{
    int overviewLevel;
  public:
    BLXRasterBand( BLXDataset *, int, int overviewLevel=0 );

    virtual double  GetNoDataValue( int *pbSuccess = NULL );
    virtual GDALColorInterp GetColorInterpretation(void);
    virtual int GetOverviewCount();
    virtual GDALRasterBand *GetOverview( int );

    virtual CPLErr IReadBlock( int, int, void * );
};

GDALDataset *BLXDataset::Open( GDALOpenInfo * poOpenInfo )

{
    // -------------------------------------------------------------------- 
    //      First that the header looks like a BLX header
    // -------------------------------------------------------------------- 
    if( poOpenInfo->fp == NULL || poOpenInfo->nHeaderBytes < 102 )
        return NULL;

    if(!blx_checkheader((char *)poOpenInfo->pabyHeader))
	return NULL;
    
    // -------------------------------------------------------------------- 
    //      Create a corresponding GDALDataset.                             
    // -------------------------------------------------------------------- 
    BLXDataset 	*poDS;

    poDS = new BLXDataset();

    // -------------------------------------------------------------------- 
    //      Open BLX file
    // -------------------------------------------------------------------- 
    poDS->blxcontext = blx_create_context();
    blxopen(poDS->blxcontext, poOpenInfo->pszFilename, "rb");

    if(poDS->blxcontext==NULL)
	return NULL;
    
    if ((poDS->blxcontext->cell_xsize % (1 << (1+BLX_OVERVIEWLEVELS))) != 0 ||
        (poDS->blxcontext->cell_ysize % (1 << (1+BLX_OVERVIEWLEVELS))) != 0)
    {
        delete poDS;
        return NULL;
    }

    // Update dataset header from BLX context
    poDS->nRasterXSize = poDS->blxcontext->xsize;
    poDS->nRasterYSize = poDS->blxcontext->ysize;

    // -------------------------------------------------------------------- 
    //      Create band information objects.                                
    // -------------------------------------------------------------------- 
    poDS->nBands = 1;
    poDS->SetBand( 1, new BLXRasterBand( poDS, 1 ));

    // Create overview bands
    poDS->nOverviewCount = BLX_OVERVIEWLEVELS;	
    for(int i=0; i < poDS->nOverviewCount; i++) {
	poDS->papoOverviewDS[i] = new BLXDataset();
	poDS->papoOverviewDS[i]->blxcontext = poDS->blxcontext;
	poDS->papoOverviewDS[i]->bIsOverview = TRUE;
	poDS->papoOverviewDS[i]->nRasterXSize = poDS->nRasterXSize >> (i+1);
	poDS->papoOverviewDS[i]->nRasterYSize = poDS->nRasterYSize >> (i+1);
	poDS->nBands = 1;
	poDS->papoOverviewDS[i]->SetBand(1, new BLXRasterBand( poDS->papoOverviewDS[i], 1, i+1));
    }
    
/* -------------------------------------------------------------------- */
/*      Confirm the requested access is supported.                      */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->eAccess == GA_Update )
    {
        delete poDS;
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "The BLX driver does not support update access to existing"
                  " datasets.\n" );
        return NULL;
    }

    return( poDS );
}

BLXDataset::BLXDataset() {
    blxcontext = NULL;
    bIsOverview = FALSE;
    nOverviewCount = 0;
    
    for(int i=0; i < nOverviewCount; i++)
	papoOverviewDS[i]=NULL;
}

BLXDataset::~BLXDataset() {
    if(!bIsOverview) {
	if(blxcontext) {
	    blxclose(blxcontext);
	    blx_free_context(blxcontext);
	}
	for(int i=0; i < nOverviewCount; i++)
	    if(papoOverviewDS[i])
		delete papoOverviewDS[i];
    }
}


CPLErr BLXDataset::GetGeoTransform( double * padfTransform )

{
    padfTransform[0] = blxcontext->lon;
    padfTransform[1] = blxcontext->pixelsize_lon;
    padfTransform[2] = 0.0;
    padfTransform[3] = blxcontext->lat;
    padfTransform[4] = 0.0;
    padfTransform[5] = blxcontext->pixelsize_lat;

    return CE_None;
}

const char *BLXDataset::GetProjectionRef()
{
    return( "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433],AUTHORITY[\"EPSG\",\"4326\"]]" );
}

BLXRasterBand::BLXRasterBand( BLXDataset *poDS, int nBand, int overviewLevel )

{
    BLXDataset *poGDS = (BLXDataset *) poDS;

    this->poDS = poDS;
    this->nBand = nBand;
    this->overviewLevel = overviewLevel;
    
    eDataType = GDT_Int16;

    nBlockXSize = poGDS->blxcontext->cell_xsize >> overviewLevel;
    nBlockYSize = poGDS->blxcontext->cell_ysize >> overviewLevel;
}

int BLXRasterBand::GetOverviewCount()
{
    BLXDataset *poGDS = (BLXDataset *) poDS;
    
    return poGDS->nOverviewCount;
}

GDALRasterBand *BLXRasterBand::GetOverview( int i )
{
    BLXDataset        *poGDS = (BLXDataset *) poDS;

    if( i < 0 || i >= poGDS->nOverviewCount )
	return NULL;
    else
    	return poGDS->papoOverviewDS[i]->GetRasterBand(nBand);
}

CPLErr BLXRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )

{
    BLXDataset *poGDS = (BLXDataset *) poDS;

    if(blx_readcell(poGDS->blxcontext, nBlockYOff, nBlockXOff, (short *)pImage, nBlockXSize*nBlockYSize*2, overviewLevel) == NULL) {
	CPLError(CE_Failure, CPLE_AppDefined,
		 "Failed to read BLX cell");
	return CE_Failure;
    }

    return CE_None;
}

double BLXRasterBand::GetNoDataValue( int * pbSuccess )
{
    if (pbSuccess)
        *pbSuccess = TRUE;
    return BLX_UNDEF;
}

GDALColorInterp BLXRasterBand::GetColorInterpretation(void) {
    return GCI_GrayIndex;
}

/* TODO: check if georeference is the same as for BLX files, WGS84
*/
static GDALDataset *
BLXCreateCopy( const char * pszFilename, GDALDataset *poSrcDS, 
                int bStrict, char ** papszOptions, 
                GDALProgressFunc pfnProgress, void * pProgressData )

{
    int  nBands = poSrcDS->GetRasterCount();
    int  nXSize = poSrcDS->GetRasterXSize();
    int  nYSize = poSrcDS->GetRasterYSize();
    
    int zscale = 1;
    int fillundef = 1, fillundefval = 0;
    int endian = LITTLEENDIAN;

// -------------------------------------------------------------------- 
//      Some rudimentary checks                                    
// -------------------------------------------------------------------- 
    if( nBands != 1 )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "BLX driver doesn't support %d bands.  Must be 1 (grey) ",
                  nBands );
        return NULL;
    }

    if( poSrcDS->GetRasterBand(1)->GetRasterDataType() != GDT_Int16 && bStrict )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "BLX driver doesn't support data type %s. "
                  "Only 16 bit byte bands supported.\n", 
                  GDALGetDataTypeName( 
                      poSrcDS->GetRasterBand(1)->GetRasterDataType()) );

        return NULL;
    }

    if( (nXSize % 128 != 0) || (nYSize % 128 != 0) ) {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "BLX driver doesn't support dimensions that are not a multiple of 128.\n");

        return NULL;
    }
    
// -------------------------------------------------------------------- 
//      What options has the user selected?                             
// -------------------------------------------------------------------- 
    if( CSLFetchNameValue(papszOptions,"ZSCALE") != NULL ) {
        zscale = atoi(CSLFetchNameValue(papszOptions,"ZSCALE"));
        if( zscale < 1 ) {
            CPLError( CE_Failure, CPLE_IllegalArg,
                      "ZSCALE=%s is not a legal value in the range >= 1.",
                      CSLFetchNameValue(papszOptions,"ZSCALE") );
            return NULL;
        }
    }

    if( CSLFetchNameValue(papszOptions,"FILLUNDEF") != NULL 
                && EQUAL(CSLFetchNameValue(papszOptions,"FILLUNDEF"),"NO") )
	fillundef = 0;
    else	
	fillundef = 1;

    if( CSLFetchNameValue(papszOptions,"FILLUNDEFVAL") != NULL ) {
        fillundefval = atoi(CSLFetchNameValue(papszOptions,"FILLUNDEFVAL"));
        if( (fillundefval < -32768) || (fillundefval > 32767) ) {
            CPLError( CE_Failure, CPLE_IllegalArg,
                      "FILLUNDEFVAL=%s is not a legal value in the range -32768, 32767.",
                      CSLFetchNameValue(papszOptions,"FILLUNDEFVAL") );
            return NULL;
        }
    }
    if( CSLFetchNameValue(papszOptions,"BIGENDIAN") != NULL 
	&& !EQUAL(CSLFetchNameValue(papszOptions,"BIGENDIAN"),"NO") )
	endian = BIGENDIAN;
	
    

// -------------------------------------------------------------------- 
//      Create the dataset.                                             
// -------------------------------------------------------------------- 
    blxcontext_t *ctx;

    // Create a BLX context
    ctx = blx_create_context();

    // Setup BLX parameters
    ctx->cell_rows = nYSize / ctx->cell_ysize;
    ctx->cell_cols = nXSize / ctx->cell_xsize;
    ctx->zscale = zscale;
    ctx->fillundef = fillundef;
    ctx->fillundefval = fillundefval;
    ctx->endian = endian;

    if(blxopen(ctx, pszFilename, "wb")) {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Unable to create blx file %s.\n", 
                  pszFilename );
        blx_free_context(ctx);
        return NULL;
    }

// -------------------------------------------------------------------- 
//      Loop over image, copying image data.                            
// -------------------------------------------------------------------- 
    GInt16     *pabyTile;
    CPLErr      eErr=CE_None;

    pabyTile = (GInt16 *) VSIMalloc( sizeof(GInt16)*ctx->cell_xsize*ctx->cell_ysize );
    if (pabyTile == NULL)
    {
        CPLError( CE_Failure, CPLE_OutOfMemory, "Out of memory");
        blxclose(ctx);
        blx_free_context(ctx);
        return NULL;
    }

    if( !pfnProgress( 0.0, NULL, pProgressData ) )
        eErr = CE_Failure;

    for(int i=0; (i < ctx->cell_rows) && (eErr == CE_None); i++)
	for(int j=0; j < ctx->cell_cols; j++) {
	    blxdata *celldata;
	    GDALRasterBand * poBand = poSrcDS->GetRasterBand( 1 );
	    eErr = poBand->RasterIO( GF_Read, j*ctx->cell_xsize, i*ctx->cell_ysize, 
				     ctx->cell_xsize, ctx->cell_ysize, 
				     pabyTile, ctx->cell_xsize, ctx->cell_ysize, GDT_Int16,
				     0, 0 );
	    if(eErr >= CE_Failure) 
	       break;
	    celldata = pabyTile;
	    if (blx_writecell(ctx, celldata, i, j) != 0)
            {
                eErr = CE_Failure;
                break;
            }

            if ( ! pfnProgress( 1.0 * (i * ctx->cell_cols + j) / (ctx->cell_rows * ctx->cell_cols), NULL, pProgressData ))
            {
                eErr = CE_Failure;
                break;
            }
    }

    pfnProgress( 1.0, NULL, pProgressData );

    CPLFree( pabyTile );

    double        adfGeoTransform[6];
    if( poSrcDS->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
	ctx->lon = adfGeoTransform[0];
	ctx->lat = adfGeoTransform[3];
	ctx->pixelsize_lon = adfGeoTransform[1];	
	ctx->pixelsize_lat = adfGeoTransform[5];	
    }

    blxclose(ctx);
    blx_free_context(ctx);

    if (eErr == CE_None)
        return (GDALDataset *) GDALOpen( pszFilename, GA_ReadOnly );
    else
        return NULL;
}


void GDALRegister_BLX()

{
    GDALDriver  *poDriver;

    if( GDALGetDriverByName( "BLX" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "BLX" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Magellan topo (.blx)" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#BLX" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "blx" );

        poDriver->pfnOpen = BLXDataset::Open;
	poDriver->pfnCreateCopy = BLXCreateCopy;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

