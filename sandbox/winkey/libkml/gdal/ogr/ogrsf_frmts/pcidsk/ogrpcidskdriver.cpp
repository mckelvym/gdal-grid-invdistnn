/******************************************************************************
 * $Id: ogrcsvdriver.cpp 10645 2007-01-18 02:22:39Z warmerdam $
 *
 * Project:  PCIDSK Translator
 * Purpose:  Implements OGRPCIDSKDriver.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2009, Frank Warmerdam <warmerdam@pobox.com>
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

#include "ogr_pcidsk.h"
#include "cpl_conv.h"

CPL_CVSID("$Id: ogrcsvdriver.cpp 10645 2007-01-18 02:22:39Z warmerdam $");

/************************************************************************/
/*                          ~OGRPCIDSKDriver()                          */
/************************************************************************/

OGRPCIDSKDriver::~OGRPCIDSKDriver()

{
}

/************************************************************************/
/*                              GetName()                               */
/************************************************************************/

const char *OGRPCIDSKDriver::GetName()

{
    return "PCIDSK";
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

OGRDataSource *OGRPCIDSKDriver::Open( const char * pszFilename, int bUpdate )

{
    OGRPCIDSKDataSource   *poDS = new OGRPCIDSKDataSource();

    if( !poDS->Open( pszFilename, bUpdate ) )
    {
        delete poDS;
        poDS = NULL;
    }

    return poDS;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRPCIDSKDriver::TestCapability( const char * pszCap )

{
    return FALSE;
}

/************************************************************************/
/*                         RegisterOGRPCIDSK()                          */
/************************************************************************/

void RegisterOGRPCIDSK()

{
    OGRSFDriverRegistrar::GetRegistrar()->RegisterDriver( new OGRPCIDSKDriver );
}

