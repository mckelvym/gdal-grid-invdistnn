/******************************************************************************
 *
 * Project:  KML Translator
 * Purpose:  Implements OGRLIBKMLDriver
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

#include "ogr_libkml.h"
#include "cpl_conv.h"
#include "cpl_error.h"


/******************************************************************************
 OGRLIBKMLDriver()
******************************************************************************/

OGRLIBKMLDriver::OGRLIBKMLDriver() : bUpdate(0)
{
}

/******************************************************************************
 ~OGRLIBKMLDriver()
******************************************************************************/

OGRLIBKMLDriver::~OGRLIBKMLDriver()
{
}

/******************************************************************************
 GetName()
******************************************************************************/

const char *OGRLIBKMLDriver::GetName()
{
    return "LIBKML";
}

/******************************************************************************
 Open()
******************************************************************************/

OGRDataSource *OGRLIBKMLDriver::Open(const char *pszFilename, int bUpdate)
{
    OGRLIBKMLDataSource *poDS = new OGRLIBKMLDataSource();

    if (!poDS->Open(pszFilename, bUpdate))
    {
        delete poDS;
        poDS = NULL;
    }

    return poDS;
}

/******************************************************************************
 CreateDataSource()
******************************************************************************/

OGRDataSource *OGRLIBKMLDriver::CreateDataSource(const char* pszName, char** papszOptions)
{
    CPLAssert( NULL != pszName );
    CPLDebug( "LIBKML", "Attempt to create: %s", pszName );

    OGRLIBKMLDataSource *poDS = new OGRLIBKMLDataSource();

    if (!poDS->Create(pszName, papszOptions))
    {
        delete poDS;
        poDS = NULL;
    }

    return poDS;
}

/******************************************************************************
 DeleteDataSource()
******************************************************************************/

OGRErr OGRLIBKMLDriver::DeleteDataSource(const char *pszName)
{
#warning implement DeleteDataSource()

    return OGRERR_NONE;
}

/******************************************************************************
 TestCapability()
******************************************************************************/

int OGRLIBKMLDriver::TestCapability(const char* pszCap)
{
  if( EQUAL(pszCap,ODrCCreateDataSource) )
    return bUpdate;
  else if( EQUAL(pszCap,ODrCDeleteDataSource) )
    return bUpdate;
  else
    return FALSE;
}

/******************************************************************************
 RegisterOGRLIBKML()
******************************************************************************/

void RegisterOGRLIBKML()
{
  OGRSFDriverRegistrar::GetRegistrar()->RegisterDriver(new OGRLIBKMLDriver());
}

