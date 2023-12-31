/******************************************************************************
 * Copyright (c) 1998, Frank Warmerdam
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
 * gdalopen.c
 *
 * GDALOpen() function, and supporting functions.
 *
 * 
 * $Log$
 * Revision 1.11  2001/08/20 13:40:28  warmerda
 * modified message on failure to open if not a file
 *
 * Revision 1.10  2001/07/18 04:04:30  warmerda
 * added CPL_CVSID
 *
 * Revision 1.9  2001/01/03 05:32:20  warmerda
 * avoid depending on VSIStatBuf file size for 2GB issues
 *
 * Revision 1.8  2000/04/21 21:55:53  warmerda
 * set filename as description of GDALDatasets.
 *
 * Revision 1.7  2000/01/10 15:43:06  warmerda
 * Fixed debug statement.
 *
 * Revision 1.6  2000/01/10 15:31:02  warmerda
 * Added debug statement in GDALOpen.
 *
 * Revision 1.5  1999/11/11 21:59:07  warmerda
 * added GetDriver() for datasets
 *
 * Revision 1.4  1999/10/01 14:44:02  warmerda
 * added documentation
 *
 * Revision 1.3  1999/04/21 04:00:34  warmerda
 * Initialize fp to NULL.
 *
 * Revision 1.2  1998/12/31 18:52:45  warmerda
 * Use CPL memory functions (they are safe), and fixed up header reading.
 *
 * Revision 1.1  1998/12/03 18:31:45  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "cpl_conv.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*                             GDALOpenInfo                             */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                            GDALOpenInfo()                            */
/************************************************************************/

GDALOpenInfo::GDALOpenInfo( const char * pszFilenameIn, GDALAccess eAccessIn )

{
    pszFilename = CPLStrdup( pszFilenameIn );

    nHeaderBytes = 0;
    pabyHeader = NULL;
    bStatOK = FALSE;
    eAccess = eAccessIn;
    fp = NULL;
    
/* -------------------------------------------------------------------- */
/*      Collect information about the file.                             */
/* -------------------------------------------------------------------- */
    if( VSIStat( pszFilename, &sStat ) == 0 )
    {
        bStatOK = TRUE;

        if( VSI_ISREG( sStat.st_mode ) )
        {
            pabyHeader = (GByte *) CPLCalloc(1025,1);

            fp = VSIFOpen( pszFilename, "rb" );

            if( fp != NULL )
            {
                nHeaderBytes = VSIFRead( pabyHeader, 1, 1024, fp );

                VSIRewind( fp );
            }
        }
    }
}

/************************************************************************/
/*                           ~GDALOpenInfo()                            */
/************************************************************************/

GDALOpenInfo::~GDALOpenInfo()

{
    VSIFree( pabyHeader );
    CPLFree( pszFilename );

    if( fp != NULL )
        VSIFClose( fp );
}

/************************************************************************/
/*                              GDALOpen()                              */
/************************************************************************/

/**
 * Open a raster file as a GDALDataset.
 *
 * @param pszFilename the name of the file to access.  In the case of
 * exotic drivers this may not refer to a physical file, but instead contain
 * information for the driver on how to access a dataset.
 *
 * @param eAccess the desired access, either GA_Update or GA_ReadOnly.  Many
 * drivers support only read only access.
 *
 * @return A GDALDatasetH handle or NULL on failure.  For C++ applications
 * this handle can be cast to a GDALDataset *. 
 */
 

GDALDatasetH GDALOpen( const char * pszFilename, GDALAccess eAccess )

{
    int		iDriver;
    GDALDriverManager *poDM = GetGDALDriverManager();
    GDALOpenInfo oOpenInfo( pszFilename, eAccess );

    CPLErrorReset();
    
    for( iDriver = 0; iDriver < poDM->GetDriverCount(); iDriver++ )
    {
        GDALDriver	*poDriver = poDM->GetDriver( iDriver );
        GDALDataset	*poDS;

        poDS = poDriver->pfnOpen( &oOpenInfo );
        if( poDS != NULL )
        {
            poDS->SetDescription( pszFilename );

            if( poDS->poDriver == NULL )
                poDS->poDriver = poDriver;

            CPLDebug( "GDAL", "GDALOpen(%s) succeeds as %s.\n",
                      pszFilename, poDriver->pszLongName );

            return (GDALDatasetH) poDS;
        }

        if( CPLGetLastErrorNo() != 0 )
            return NULL;
    }

    if( oOpenInfo.bStatOK )
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "`%s' not recognised as a supported file format.\n",
                  pszFilename );
    else
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "`%s' does not exist in the file system,\n"
                  "and is not recognised as a supported dataset name.\n",
                  pszFilename );
              
    return NULL;
}

