/******************************************************************************
 *
 * Project:  NETCDF Translator
 * Purpose:  Implements OGRNETCDFDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2010, Brian Case
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
 *****************************************************************************/

#include "ogrnetcdf.h"

#include <netcdf.h>

/******************************************************************************
 Datasource constructor
******************************************************************************/

OGRNETCDFDataSource::OGRNETCDFDataSource()

{
    papoLayers = NULL;
    nLayers = 0;

    pszName = NULL;
}

/******************************************************************************
 Datasource destructor
******************************************************************************/

OGRNETCDFDataSource::~OGRNETCDFDataSource()

{
    for( int i = 0; i < nLayers; i++ )
        delete papoLayers[i];
    CPLFree( papoLayers );

    CPLFree( pszName );
}

/******************************************************************************
 Datasource Open method
******************************************************************************/

int  OGRNETCDFDataSource::Open( const char *pszFilename, int bUpdate )
{

    int ncid;

	if( bUpdate ) {
	    CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Update access not supported by the NETCDF driver." );
        return FALSE;
    }

    /***** Does this appear to be an .nc file *****/
	
    if (NC_NOERR != nc_open(pszFilename, NC_NOWRITE, &ncid ) )
        return FALSE;

    /***** Create a corresponding layer. *****/

	nLayers = 1;
    papoLayers = (OGRNETCDFLayer **) CPLMalloc(sizeof(void*));
    
    papoLayers[0] = new OGRNETCDFLayer ( pszFilename, ncid );

    pszName = CPLStrdup( pszFilename );

    return TRUE;
}

/******************************************************************************
 Datasource GetLayer method
******************************************************************************/

OGRLayer *OGRNETCDFDataSource::GetLayer( int iLayer )
{
    if( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];
}

