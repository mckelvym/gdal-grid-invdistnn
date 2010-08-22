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

#ifndef HAVE_OGRNETCDF_H
#define HAVE_OGRNETCDF_H

#include "ogrsf_frmts.h"

/******************************************************************************
 Layer
******************************************************************************/

class OGRNETCDFLayer : public OGRLayer
{
    OGRFeatureDefn     *poFeatureDefn;

    FILE               *fp;

    int                 nNextFID;

  public:
    OGRNETCDFLayer( const char *pszFilename, int iNCid );
   ~OGRNETCDFLayer();

    void                ResetReading();
    OGRFeature *        GetNextFeature();

    OGRFeatureDefn *    GetLayerDefn() { return poFeatureDefn; }

    int                 TestCapability( const char * ) { return FALSE; }
};

/******************************************************************************
 Datasource
******************************************************************************/

class OGRNETCDFDataSource : public OGRDataSource
{
    char                *pszName;
    
    OGRNETCDFLayer       **papoLayers;
    int                 nLayers;

  public:
                        OGRNETCDFDataSource();
                        ~OGRNETCDFDataSource();

    int                 Open( const char * pszFilename, int bUpdate );
    
    const char          *GetName() { return pszName; }

    int                 GetLayerCount() { return nLayers; }
    OGRLayer            *GetLayer( int );

    int                 TestCapability( const char * ) { return FALSE; }
};

/******************************************************************************
 Datasource
******************************************************************************/

class OGRNETCDFDriver : public OGRSFDriver
{
  public:
                ~OGRNETCDFDriver();
                
    const char    *GetName();
    OGRDataSource *Open( const char *, int );
//    OGRDataSource *CreateDataSource( const char *, char ** );
//    OGRErr         DeleteDataSource( const char *pszName );
    int            TestCapability( const char * );
};

#endif
