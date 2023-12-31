/******************************************************************************
 * $Id$
 *
 * Project:  REC Translator
 * Purpose:  Implements EpiInfo .REC driver.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.1  2003/02/03 21:11:35  warmerda
 * New
 *
 */

#include "ogr_rec.h"
#include "cpl_conv.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           ~OGRRECDriver()                            */
/************************************************************************/

OGRRECDriver::~OGRRECDriver()

{
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRRECDriver::TestCapability( const char * )

{
    return FALSE;
}

/************************************************************************/
/*                              GetName()                               */
/************************************************************************/

const char *OGRRECDriver::GetName()

{
    return "REC";
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

OGRDataSource *OGRRECDriver::Open( const char * pszFilename, int bUpdate )

{
    OGRRECDataSource   *poDS = new OGRRECDataSource();

    if( !poDS->Open( pszFilename ) )
    {
        delete poDS;
        poDS = NULL;
    }

    if( poDS != NULL && bUpdate )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "REC Driver doesn't support update." );
        delete poDS;
        poDS = NULL;
    }
    
    return poDS;
}

/************************************************************************/
/*                           RegisterOGRREC()                          */
/************************************************************************/

void RegisterOGRREC()

{
    OGRSFDriverRegistrar::GetRegistrar()->RegisterDriver( new OGRRECDriver );
}

