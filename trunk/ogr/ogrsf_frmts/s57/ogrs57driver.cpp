/******************************************************************************
 * $Id$
 *
 * Project:  S-57 Translator
 * Purpose:  Implements OGRS57Driver
 * Author:   Frank Warmerdam, warmerda@home.com
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.1  1999/11/03 22:12:43  warmerda
 * New
 *
 */

#include "ogr_s57.h"
#include "cpl_conv.h"

/************************************************************************/
/*                           ~OGRS57Driver()                            */
/************************************************************************/

OGRS57Driver::~OGRS57Driver()

{
}

/************************************************************************/
/*                              GetName()                               */
/************************************************************************/

const char *OGRS57Driver::GetName()

{
    return "IHO S-57 (ENC)";
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

OGRDataSource *OGRS57Driver::Open( const char * pszFilename, int bUpdate )

{
    OGRS57DataSource	*poDS = new OGRS57DataSource;

    if( !poDS->Open( pszFilename, TRUE ) )
    {
        delete poDS;
        poDS = NULL;
    }

    if( poDS != NULL && bUpdate )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "S57 Driver doesn't support update." );
        delete poDS;
        poDS = NULL;
    }
    
    return poDS;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRS57Driver::TestCapability( const char * )

{
    return FALSE;
}

/************************************************************************/
/*                           RegisterOGRS57()                           */
/************************************************************************/

void RegisterOGRS57()

{
    OGRSFDriverRegistrar::GetRegistrar()->RegisterDriver( new OGRS57Driver );
}


