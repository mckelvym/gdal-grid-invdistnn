/******************************************************************************
 * $Id: wcsdataset.cpp 10645 2007-01-18 02:22:39Z warmerdam $
 *
 * Project:  WCS Client Driver
 * Purpose:  Implementation of an HTTP fetching driver.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2007, Frank Warmerdam
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
#include "cpl_http.h"

CPL_CVSID("$Id: wcsdataset.cpp 10645 2007-01-18 02:22:39Z warmerdam $");

/************************************************************************/
/*                              HTTPOpen()                              */
/************************************************************************/

static GDALDataset *HTTPOpen( GDALOpenInfo * poOpenInfo )

{
    if( poOpenInfo->nHeaderBytes != 0 )
        return NULL;

    if( !EQUALN(poOpenInfo->pszFilename,"http:",5)
        && !EQUALN(poOpenInfo->pszFilename,"https:",6)
        && !EQUALN(poOpenInfo->pszFilename,"ftp:",4) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Fetch the result.                                               */
/* -------------------------------------------------------------------- */
    CPLErrorReset();
    
    CPLHTTPResult *psResult = CPLHTTPFetch( poOpenInfo->pszFilename, NULL );

/* -------------------------------------------------------------------- */
/*      Try to handle errors.                                           */
/* -------------------------------------------------------------------- */
    if( psResult == NULL || psResult->nDataLen == 0 
        || CPLGetLastErrorNo() != 0 )
    {
        CPLHTTPDestroyResult( psResult );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a memory file from the result.                           */
/* -------------------------------------------------------------------- */
    // Eventually we should be looking at mime info and stuff to figure
    // out an optimal filename, but for now we just use a fixed one.

    CPLString osResultFilename;

    osResultFilename.Printf( "/vsimem/http/%p.dat", 
                             psResult->pabyData );

    VSILFILE *fp = VSIFileFromMemBuffer( osResultFilename,
                                     psResult->pabyData, 
                                     psResult->nDataLen, 
                                     TRUE );

    if( fp == NULL )
        return NULL;

    VSIFCloseL( fp );

/* -------------------------------------------------------------------- */
/*      Steal the memory buffer from HTTP result before destroying      */
/*      it.                                                             */
/* -------------------------------------------------------------------- */
    psResult->pabyData = NULL;
    psResult->nDataLen = psResult->nDataAlloc = 0;

    CPLHTTPDestroyResult( psResult );

/* -------------------------------------------------------------------- */
/*      Try opening this result as a gdaldataset.                       */
/* -------------------------------------------------------------------- */
    GDALDataset *poDS = (GDALDataset *) 
        GDALOpen( osResultFilename, GA_ReadOnly );

/* -------------------------------------------------------------------- */
/*      If opening it in memory didn't work, perhaps we need to         */
/*      write to a temp file on disk?                                   */
/* -------------------------------------------------------------------- */
    if( poDS == NULL )
    {
        CPLString osTempFilename;
        
        osTempFilename.Printf( "/tmp/%s", CPLGetFilename(osResultFilename) );
        if( CPLCopyFile( osTempFilename, osResultFilename ) != 0 )
        {
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "Failed to create temporary file:%s", 
                      osTempFilename.c_str() );
        }
        else
        {
            poDS =  (GDALDataset *) 
                GDALOpen( osTempFilename, GA_ReadOnly );
            VSIUnlink( osTempFilename ); /* this may not work on windows */
        }
    }

/* -------------------------------------------------------------------- */
/*      Release our hold on the vsi memory file, though if it is        */
/*      held open by a dataset it will continue to exist till that      */
/*      lets it go.                                                     */
/* -------------------------------------------------------------------- */
    VSIUnlink( osResultFilename );

    return poDS;
}

/************************************************************************/
/*                         GDALRegister_HTTP()                          */
/************************************************************************/

void GDALRegister_HTTP()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "HTTP" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "HTTP" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "HTTP Fetching Wrapper" );
        
        poDriver->pfnOpen = HTTPOpen;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
