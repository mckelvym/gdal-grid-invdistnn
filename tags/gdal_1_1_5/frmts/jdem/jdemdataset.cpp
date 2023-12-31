/******************************************************************************
 * $Id$
 *
 * Project:  JDEM Reader
 * Purpose:  All code for Japanese DEM Reader
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.4  2001/07/18 04:51:57  warmerda
 * added CPL_CVSID
 *
 * Revision 1.3  2001/06/21 19:59:51  warmerda
 * added help link
 *
 * Revision 1.2  2000/11/28 02:28:54  warmerda
 * Added error checks, GetGeoTransform and GetProjection
 *
 * Revision 1.1  2000/11/27 19:03:26  warmerda
 * New
 *
 */

#include "gdal_priv.h"

CPL_CVSID("$Id$");

static GDALDriver	*poJDEMDriver = NULL;

CPL_C_START
void	GDALRegister_JDEM(void);
CPL_C_END

/************************************************************************/
/*                            JDEMGetField()                            */
/************************************************************************/

static int JDEMGetField( char *pszField, int nWidth )

{
    char	szWork[32];

    CPLAssert( nWidth < (int) sizeof(szWork) );

    strncpy( szWork, pszField, nWidth );
    szWork[nWidth] = '\0';

    return atoi(szWork);
}

/************************************************************************/
/*                            JDEMGetAngle()                            */
/************************************************************************/

static double JDEMGetAngle( char *pszField )

{
    int		nAngle = JDEMGetField( pszField, 7 );
    int		nDegree, nMin, nSec;

    // Note, this isn't very general purpose, but it would appear
    // from the field widths that angles are never negative.  Nice
    // to be a country in the "first quadrant". 

    nDegree = nAngle / 10000;
    nMin = (nAngle / 100) % 100;
    nSec = nAngle % 100;
    
    return nDegree + nMin / 60.0 + nSec / 3600.0;
}

/************************************************************************/
/* ==================================================================== */
/*				JDEMDataset				*/
/* ==================================================================== */
/************************************************************************/

class JDEMRasterBand;

class JDEMDataset : public GDALDataset
{
    friend	JDEMRasterBand;

    FILE	*fp;
    GByte	abyHeader[1012];

  public:
		~JDEMDataset();
    
    static GDALDataset *Open( GDALOpenInfo * );

    CPLErr 	GetGeoTransform( double * padfTransform );
    const char *GetProjectionRef();
};

/************************************************************************/
/* ==================================================================== */
/*                            JDEMRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class JDEMRasterBand : public GDALRasterBand
{
    friend	JDEMDataset;
    
  public:

    		JDEMRasterBand( JDEMDataset *, int );
    
    virtual CPLErr IReadBlock( int, int, void * );
};


/************************************************************************/
/*                           JDEMRasterBand()                            */
/************************************************************************/

JDEMRasterBand::JDEMRasterBand( JDEMDataset *poDS, int nBand )

{
    this->poDS = poDS;
    this->nBand = nBand;
    
    eDataType = GDT_Float32;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr JDEMRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )

{
    JDEMDataset *poGDS = (JDEMDataset *) poDS;
    char	*pszRecord;
    int		nRecordSize = nBlockXSize*5 + 9 + 2;
    int		i;

    VSIFSeek( poGDS->fp, 1011 + nRecordSize*nBlockYOff, SEEK_SET );

    pszRecord = (char *) CPLMalloc(nRecordSize);
    VSIFRead( pszRecord, 1, nRecordSize, poGDS->fp );

    if( !EQUALN((char *) poGDS->abyHeader,pszRecord,6) )
    {
        CPLFree( pszRecord );

        CPLError( CE_Failure, CPLE_AppDefined, 
                  "JDEM Scanline corrupt.  Perhaps file was not transferred\n"
                  "in binary mode?" );
        return CE_Failure;
    }
    
    if( JDEMGetField( pszRecord + 6, 3 ) != nBlockYOff + 1 )
    {
        CPLFree( pszRecord );

        CPLError( CE_Failure, CPLE_AppDefined, 
                  "JDEM scanline out of order, JDEM driver does not\n"
                  "currently support partial datasets." );
        return CE_Failure;
    }

    for( i = 0; i < nBlockXSize; i++ )
        ((float *) pImage)[i] = JDEMGetField( pszRecord + 9 + 5 * i, 5) * 0.1;

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*				JDEMDataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                            ~JDEMDataset()                             */
/************************************************************************/

JDEMDataset::~JDEMDataset()

{
    if( fp != NULL )
        VSIFClose( fp );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr JDEMDataset::GetGeoTransform( double * padfTransform )

{
    double	dfLLLat, dfLLLong, dfURLat, dfURLong;

    dfLLLat = JDEMGetAngle( (char *) abyHeader + 29 );
    dfLLLong = JDEMGetAngle( (char *) abyHeader + 36 );
    dfURLat = JDEMGetAngle( (char *) abyHeader + 43 );
    dfURLong = JDEMGetAngle( (char *) abyHeader + 50 );
    
    padfTransform[0] = dfLLLong;
    padfTransform[3] = dfURLat;
    padfTransform[1] = (dfURLong - dfLLLong) / GetRasterXSize();
    padfTransform[2] = 0.0;
        
    padfTransform[4] = 0.0;
    padfTransform[5] = -1 * (dfURLat - dfLLLat) / GetRasterYSize();


    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *JDEMDataset::GetProjectionRef()

{
    return( "GEOGCS[\"Tokyo\",DATUM[\"Tokyo\",SPHEROID[\"Bessel 1841\",6377397.155,299.1528128,AUTHORITY[\"EPSG\",7004]],TOWGS84[-148,507,685,0,0,0,0],AUTHORITY[\"EPSG\",6301]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",8901]],UNIT[\"DMSH\",0.0174532925199433,AUTHORITY[\"EPSG\",9108]],AXIS[\"Lat\",NORTH],AXIS[\"Long\",EAST],AUTHORITY[\"EPSG\",4301]]" );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *JDEMDataset::Open( GDALOpenInfo * poOpenInfo )

{
/* -------------------------------------------------------------------- */
/*      Before trying JDEMOpen() we first verify that there is at        */
/*      least one "\n#keyword" type signature in the first chunk of     */
/*      the file.                                                       */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->fp == NULL || poOpenInfo->nHeaderBytes < 50 )
        return NULL;

    /* check if century values seem reasonable */
    if( (!EQUALN((char *)poOpenInfo->pabyHeader+11,"19",2)
          && !EQUALN((char *)poOpenInfo->pabyHeader+11,"20",2))
        || (!EQUALN((char *)poOpenInfo->pabyHeader+15,"19",2)
             && !EQUALN((char *)poOpenInfo->pabyHeader+15,"20",2))
        || (!EQUALN((char *)poOpenInfo->pabyHeader+19,"19",2)
             && !EQUALN((char *)poOpenInfo->pabyHeader+19,"20",2)) )
    {
        return NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    JDEMDataset 	*poDS;

    poDS = new JDEMDataset();

    poDS->poDriver = poJDEMDriver;
    poDS->fp = poOpenInfo->fp;
    poOpenInfo->fp = NULL;
    
/* -------------------------------------------------------------------- */
/*      Read the header.                                                */
/* -------------------------------------------------------------------- */
    VSIFSeek( poDS->fp, 0, SEEK_SET );
    VSIFRead( poDS->abyHeader, 1, 1012, poDS->fp );

    poDS->nRasterXSize = JDEMGetField( (char *) poDS->abyHeader + 23, 3 );
    poDS->nRasterYSize = JDEMGetField( (char *) poDS->abyHeader + 26, 3 );

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->SetBand( 1, new JDEMRasterBand( poDS, 1 ));

    return( poDS );
}

/************************************************************************/
/*                          GDALRegister_JDEM()                          */
/************************************************************************/

void GDALRegister_JDEM()

{
    GDALDriver	*poDriver;

    if( poJDEMDriver == NULL )
    {
        poJDEMDriver = poDriver = new GDALDriver();
        
        poDriver->pszShortName = "JDEM";
        poDriver->pszLongName = "Japanese DEM (.mem)";
        poDriver->pszHelpTopic = "frmt_various.html#JDEM";
        
        poDriver->pfnOpen = JDEMDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
