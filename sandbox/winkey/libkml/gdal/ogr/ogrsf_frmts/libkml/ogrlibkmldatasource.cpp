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
//#include "cpl_conv.h"
//#include "cpl_string.h"
//#include "cpl_error.h"

#include <kml/engine.h>

/******************************************************************************
 OGRLIBKMLDataSource()
******************************************************************************/

OGRLIBKMLDataSource::OGRLIBKMLDataSource()
    : pszName(0)
    , papoLayers(0)
    , nLayers(0)
    , bUpdate(0)
    , kmzfile(0)
    , kmlfile(0)
    , poKmlFactory(0)
{  
}

/******************************************************************************
 ~OGRLIBKMLDataSource()
******************************************************************************/

OGRLIBKMLDataSource::~OGRLIBKMLDataSource() {

  CPLFree( pszName );

  for( int i = 0; i < nLayers; i++ )
    delete papoLayers[i];

  CPLFree( papoLayers );
  
}


/******************************************************************************
 Open()
******************************************************************************/

int OGRLIBKMLDataSource::Open(const char *pszFilename, int bUpdate )
{

    /***** kml *****/

    if (EQUAL( CPLGetExtension(pszFilename), "kml" ))
    {
        if( bUpdate )
        {
            //static bool kmlbase::File::ReadFileToString   (   const string &       filename,
            //      string *    output   
            //  ) 
        }
    }
    /***** kmz *****/

    else if (EQUAL( CPLGetExtension(pszFilename), "kmz" ))
    {

        if (!(kmzfile = kmlengine::KmzFile::OpenFromFile(pszFilename)))
        {
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "%s is not a valid kmz file", pszFilename );
            return FALSE;
        }

        if( bUpdate )
        {
        }

    }
    else
    {
        return FALSE;

        if( bUpdate )
        {
        }
    }

    //TODO: what ret value here, seems TRUE ? mloskot
    return TRUE;
}

/******************************************************************************
 Create()


 env vars:
  LIBKML_USE_DOC.KML         default: yes
 
******************************************************************************/

int OGRLIBKMLDataSource::Create(const char *pszFilename, char **papszOptions)
{
    /***** kml *****/
  
    /*  if (EQUAL( CPLGetExtension(pszFilename), "kml" )) {

        if (!(kmlfile =
        kmlengine::KmlFile::CreateFromParse(
        kmlengine::KmlFile::CreateXmlHeader() ))) {
        CPLError( CE_Failure, CPLE_OpenFailed, 
        "Failed to create kml file %s.", 
        pszFilename );
        return false;
        }

        }
    */  
    /***** kmz *****/

    /*else*/
    if (EQUAL( CPLGetExtension(pszFilename), "kmz" ))
    {    
        if (!(kmzfile = kmlengine::KmzFile::Create (pszFilename)))
        {
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "Failed to create kmz file %s.", 
                      pszFilename );
            return false;
        }

        nLayers = 0;
    
        /***** set up the factory everything will use *****/
    
        poKmlFactory = kmldom::KmlFactory::GetFactory();

        /***** create the doc.kml it has to be the first file in *****/
    
        const char *namefield = CPLGetConfigOption("LIBKML_USE_DOC.KML", "yes");
        if (!strcmp(namefield, "yes"))
        {
            kmldom::DocumentPtr poKmlDoc(poKmlFactory->CreateDocument());
            kmldom::KmlPtr poKmlKml(poKmlFactory->CreateKml());
            poKmlKml->set_feature(poKmlDoc);

            std::string strKmlOut(kmldom::SerializePretty(poKmlKml));
            kmzfile->AddFile(strKmlOut, "Doc.Kml");
        }
    }

    return true;
}

/******************************************************************************
 GetLayer()
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::GetLayer(int iLayer)
{
  if( iLayer < 0 || iLayer >= nLayers )
    return NULL;
  else
    return papoLayers[iLayer];

}

/******************************************************************************
 CreateLayer()
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::CreateLayer(const char *pszLayerName,
                                           OGRSpatialReference *poSpatialRef,
                                           OGRwkbGeometryType eGType,
                                           char** papszOptions)
{
    OGRLayer *poLayer = new OGRLIBKMLLayer(pszLayerName, poSpatialRef, eGType);

    return poLayer;
}
/******************************************************************************
 TestCapability()
******************************************************************************/

int OGRLIBKMLDataSource::TestCapability(
  const char * pszCap)
{
  
  if( EQUAL(pszCap,ODsCCreateLayer) )
    return bUpdate;
  else if( EQUAL(pszCap,ODsCDeleteLayer) )
    return bUpdate;
  else
    return FALSE;
  
}
