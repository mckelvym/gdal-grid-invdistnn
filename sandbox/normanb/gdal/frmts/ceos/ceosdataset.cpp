/******************************************************************************
 * $Id$
 *
 * Project:  CEOS Translator
 * Purpose:  GDALDataset driver for CEOS translator.
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

#include "ceosopen.h"
#include "gdal_pam.h"

CPL_CVSID("$Id$");

CPL_C_START
void	GDALRegister_CEOS(void);
CPL_C_END

/************************************************************************/
/* ==================================================================== */
/*				CEOSDataset				*/
/* ==================================================================== */
/************************************************************************/

class CEOSRasterBand;

class CEOSDataset : public GDALPamDataset
{
    friend class CEOSRasterBand;
    
    CEOSImage	*psCEOS;

  public:
                 CEOSDataset();
                ~CEOSDataset();
    static GDALDataset *Open( GDALOpenInfo * );
};

/************************************************************************/
/* ==================================================================== */
/*                            CEOSRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class CEOSRasterBand : public GDALPamRasterBand
{
    friend class CEOSDataset;
    
  public:

    		CEOSRasterBand( CEOSDataset *, int );
    
    virtual CPLErr IReadBlock( int, int, void * );
};


/************************************************************************/
/*                           CEOSRasterBand()                            */
/************************************************************************/

CEOSRasterBand::CEOSRasterBand( CEOSDataset *poDS, int nBand )

{
    this->poDS = poDS;
    this->nBand = nBand;
    
    eDataType = GDT_Byte;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr CEOSRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )

{
    CEOSDataset	*poCEOS_DS = (CEOSDataset *) poDS;

    CPLAssert( nBlockXOff == 0 );

    return( CEOSReadScanline(poCEOS_DS->psCEOS, nBand, nBlockYOff+1, pImage) );
}

/************************************************************************/
/* ==================================================================== */
/*                             CEOSDataset                              */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                            CEOSDataset()                             */
/************************************************************************/

CEOSDataset::CEOSDataset()

{
    psCEOS = NULL;
}

/************************************************************************/
/*                            ~CEOSDataset()                            */
/************************************************************************/

CEOSDataset::~CEOSDataset()

{
    FlushCache();
    if( psCEOS )
        CEOSClose( psCEOS );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *CEOSDataset::Open( GDALOpenInfo * poOpenInfo )

{
    CEOSImage	*psCEOS;
    int		i;
    
/* -------------------------------------------------------------------- */
/*      Before trying CEOSOpen() we first verify that the first         */
/*      record is in fact a CEOS file descriptor record.                */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->nHeaderBytes < 100 )
        return NULL;

    if( poOpenInfo->pabyHeader[4] != 0x3f
        || poOpenInfo->pabyHeader[5] != 0xc0
        || poOpenInfo->pabyHeader[6] != 0x12
        || poOpenInfo->pabyHeader[7] != 0x12 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Try opening the dataset.                                        */
/* -------------------------------------------------------------------- */
    psCEOS = CEOSOpen( poOpenInfo->pszFilename, "rb" );
    
    if( psCEOS == NULL )
        return( NULL );

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    CEOSDataset 	*poDS;

    poDS = new CEOSDataset();

    poDS->psCEOS = psCEOS;
    
/* -------------------------------------------------------------------- */
/*      Capture some information from the file that is of interest.     */
/* -------------------------------------------------------------------- */
    poDS->nRasterXSize = psCEOS->nPixels;
    poDS->nRasterYSize = psCEOS->nLines;
    
/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->nBands = psCEOS->nBands;;

    for( i = 0; i < poDS->nBands; i++ )
        poDS->SetBand( i+1, new CEOSRasterBand( poDS, i+1 ) );

/* -------------------------------------------------------------------- */
/*      Initialize any PAM information.                                 */
/* -------------------------------------------------------------------- */
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML();

/* -------------------------------------------------------------------- */
/*      Check for overviews.                                            */
/* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );

    return( poDS );
}

/************************************************************************/
/*                          GDALRegister_GTiff()                        */
/************************************************************************/

void GDALRegister_CEOS()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "CEOS" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "CEOS" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "CEOS Image" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#CEOS" );
        
        poDriver->pfnOpen = CEOSDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

