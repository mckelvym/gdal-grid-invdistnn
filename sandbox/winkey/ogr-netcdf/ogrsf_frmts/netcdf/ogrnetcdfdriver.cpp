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
#include "cpl_conv.h"
#include "cpl_error.h"

/******************************************************************************
 Driver destructor
******************************************************************************/

OGRNETCDFDriver::~OGRNETCDFDriver()

{
}

/******************************************************************************
 Driver GetName method
******************************************************************************/

const char *OGRNETCDFDriver::GetName()
{
    return "NETCDF";
}

/******************************************************************************
 Driver Open method
******************************************************************************/

OGRDataSource *OGRNETCDFDriver::Open( const char * pszFilename, int bUpdate )
{
    OGRNETCDFDataSource   *poDS = new OGRNETCDFDataSource();

    if( !poDS->Open( pszFilename, bUpdate ) )
    {
        delete poDS;
	return NULL;
    }
    else
        return poDS;
}

/******************************************************************************
 Driver TestCapability method
******************************************************************************/

int OGRNETCDFDriver::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,ODrCCreateDataSource) )
        return FALSE;
    else if( EQUAL(pszCap,ODrCDeleteDataSource) )
        return FALSE;
    else
        return FALSE;
}

/******************************************************************************
 Driver Register function
******************************************************************************/

void RegisterOGRNETCDF()

{
    OGRSFDriverRegistrar::GetRegistrar()->
        RegisterDriver( new OGRNETCDFDriver );
    printf("gotcha\n");
}
