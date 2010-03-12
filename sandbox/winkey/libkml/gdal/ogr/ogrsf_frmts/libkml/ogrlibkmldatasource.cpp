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

//#include "cpl_conv.h"
//#include "cpl_string.h"
//#include "cpl_error.h"

#include <kml/dom.h>
#include <kml/base/file.h>

using kmldom::KmlFactory;
using kmldom::DocumentPtr;
using kmldom::Document;
using kmldom::NetworkLinkPtr;
using kmldom::LinkPtr;
using kmlbase::File;
using kmldom::KmlPtr;

#include "ogr_libkml.h"
#include "ogrlibkmldatasetstyle.h"

/******************************************************************************
 OGRLIBKMLDataSource()
******************************************************************************/

OGRLIBKMLDataSource::OGRLIBKMLDataSource (  )
{
    pszName = NULL;
    papoLayers = NULL;
    nLayers = 0;
    m_poKmlKmzfile = NULL;

}

/******************************************************************************
 ~OGRLIBKMLDataSource()
******************************************************************************/

OGRLIBKMLDataSource::~OGRLIBKMLDataSource (  )
{

    CPLFree ( pszName );
    
    delete m_poKmlKmzfile;

    for ( int i = 0; i < nLayers; i++ )
        delete papoLayers[i];

    CPLFree ( papoLayers );

}

/******************************************************************************
 method to open a kmz file
******************************************************************************/


int OGRLIBKMLDataSource::OpenKmz (
    const char *pszFilename,
    int bUpdate )
{
    if ( !( m_poKmlKmzfile = KmzFile::OpenFromFile ( pszFilename ) ) ) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "%s is not a valid kmz file", pszFilename );
        return FALSE;
    }

    /***** read the doc.kml *****/

    std::string oKmlKml;
    std::string oKmlKmlPath;
    if ( !m_poKmlKmzfile->ReadKmlAndGetPath ( &oKmlKml, &oKmlKmlPath ) ) {

#warning we should probably do some cleanup here
        return FALSE;
    }

    /***** create a SRS                                    *****/
    /***** this was shamelessly swiped from the kml driver *****/
    
    OGRSpatialReference *poOgrSRS =
        new OGRSpatialReference ( "GEOGCS[\"WGS 84\", "
                                  "   DATUM[\"WGS_1984\","
                                  "       SPHEROID[\"WGS 84\",6378137,298.257223563,"
                                  "           AUTHORITY[\"EPSG\",\"7030\"]],"
                                  "           AUTHORITY[\"EPSG\",\"6326\"]],"
                                  "       PRIMEM[\"Greenwich\",0,"
                                  "           AUTHORITY[\"EPSG\",\"8901\"]],"
                                  "       UNIT[\"degree\",0.01745329251994328,"
                                  "           AUTHORITY[\"EPSG\",\"9122\"]],"
                                  "           AUTHORITY[\"EPSG\",\"4326\"]]" );

    /***** fetch all the links *****/

    kmlengine::href_vector_t oKmlLinksVector;
    if ( !kmlengine::GetLinks ( oKmlKml, &oKmlLinksVector ) ) {
        printf("gotcha\n");
#warning this does not work, i think mayby it finds style links
        /***** there is no links *****/
        /***** we will concider the Doc.kml a layer *****/

        /***** alocate memory for the layer array *****/

        papoLayers =
            ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) );

        /***** parse the kml into the DOM *****/

        std::string oKmlErrors;
        ElementPtr poKmlRoot = kmldom::Parse ( oKmlKml, &oKmlErrors );

        if ( !poKmlRoot ) {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "ERROR Parseing kml layer %s from %s :%s",
                       oKmlKmlPath.c_str (  ),
                       pszFilename, oKmlErrors.c_str (  ) );
#warning do cleanup here
            return FALSE;
        }

        /***** create the layer *****/

        OGRLIBKMLLayer *poOgrLayer =
            new OGRLIBKMLLayer ( oKmlKmlPath.c_str (  ),
                                 poOgrSRS, wkbUnknown, this,
                                 poKmlRoot );


        papoLayers[nLayers++] = poOgrLayer;
    }

    else {
        int iKmlLink;
        int nKmlLinks = oKmlLinksVector.size (  );

        /***** alocate memory for the layer array *****/

        papoLayers =
            ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) *
                                              nKmlLinks );

        /***** loop over the links *****/

        for ( iKmlLink = 0; iKmlLink < nKmlLinks; iKmlLink++ ) {
            fprintf ( stderr, "%s\n", pszFilename );

            /***** if its a relative path lets add it as a layer *****/

            kmlengine::Href oKmlHref ( oKmlLinksVector[iKmlLink] );
            if ( oKmlHref.IsRelativePath (  ) ) {
                std::string oKml;
                if ( m_poKmlKmzfile->
                     ReadFile ( oKmlHref.get_path (  ).c_str (  ), &oKml ) ) {


#warning we should parse the kml and get a geom type
                    /***** parse the kml into the DOM *****/

                    std::string oKmlErrors;
                    ElementPtr poKmlRoot = kmldom::Parse ( oKml, &oKmlErrors );

                    if ( !poKmlRoot ) {
                        CPLError ( CE_Failure, CPLE_OpenFailed,
                                   "ERROR Parseing kml layer %s from %s :%s",
                                   oKmlHref.get_path (  ).c_str (  ),
                                   pszFilename, oKmlErrors.c_str (  ) );
                        return FALSE;
                    }

                    /***** create the layer *****/

                    OGRLIBKMLLayer *poOgrLayer =
                        new OGRLIBKMLLayer ( oKmlHref.get_path (  ).c_str (  ),
                                             poOgrSRS, wkbUnknown, this,
                                             poKmlRoot );


                    papoLayers[nLayers++] = poOgrLayer;
                }
            }
        }
    }

    delete poOgrSRS;

    return TRUE;
}

/******************************************************************************
 method to open a kml file
******************************************************************************/

int OGRLIBKMLDataSource::OpenKml (
    const char *pszFilename,
    int bUpdate )
{
    std::string oKmlKml;

    if ( !kmlbase::File::ReadFileToString( pszFilename, &oKmlKml )) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "%s is not a valid kml file", pszFilename );
        return FALSE;
    }

    /***** create a SRS                                    *****/
    /***** this was shamelessly swiped from the kml driver *****/
    
    OGRSpatialReference *poOgrSRS =
        new OGRSpatialReference ( "GEOGCS[\"WGS 84\", "
                                  "   DATUM[\"WGS_1984\","
                                  "       SPHEROID[\"WGS 84\",6378137,298.257223563,"
                                  "           AUTHORITY[\"EPSG\",\"7030\"]],"
                                  "           AUTHORITY[\"EPSG\",\"6326\"]],"
                                  "       PRIMEM[\"Greenwich\",0,"
                                  "           AUTHORITY[\"EPSG\",\"8901\"]],"
                                  "       UNIT[\"degree\",0.01745329251994328,"
                                  "           AUTHORITY[\"EPSG\",\"9122\"]],"
                                  "           AUTHORITY[\"EPSG\",\"4326\"]]" );
    
        /***** alocate memory for the layer array *****/

        papoLayers =
            ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) );

        /***** parse the kml into the DOM *****/

        std::string oKmlErrors;
        ElementPtr poKmlRoot = kmldom::Parse ( oKmlKml, &oKmlErrors );

        if ( !poKmlRoot ) {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "ERROR Parseing kml layer %s from %s :%s",
                       pszFilename,
                       pszFilename, oKmlErrors.c_str (  ) );
#warning do cleanup here
            return FALSE;
        }

        /***** create the layer *****/

        OGRLIBKMLLayer *poOgrLayer =
            new OGRLIBKMLLayer ( pszFilename,
                                 poOgrSRS, wkbUnknown, this,
                                 poKmlRoot );


        papoLayers[nLayers++] = poOgrLayer;
    
    delete poOgrSRS;
    
    return TRUE;
}

/******************************************************************************
 method to open a dir
******************************************************************************/


int OGRLIBKMLDataSource::OpenDir (
    const char *pszFilename,
    int bUpdate )
{

    char ** papszDirList = NULL;
    
    if (!( papszDirList = VSIReadDir(pszFilename)))
        return FALSE;
   
   /***** create a SRS                                    *****/
   /***** this was shamelessly swiped from the kml driver *****/
    
    OGRSpatialReference *poOgrSRS =
        new OGRSpatialReference ( "GEOGCS[\"WGS 84\", "
                                  "   DATUM[\"WGS_1984\","
                                  "       SPHEROID[\"WGS 84\",6378137,298.257223563,"
                                  "           AUTHORITY[\"EPSG\",\"7030\"]],"
                                  "           AUTHORITY[\"EPSG\",\"6326\"]],"
                                  "       PRIMEM[\"Greenwich\",0,"
                                  "           AUTHORITY[\"EPSG\",\"8901\"]],"
                                  "       UNIT[\"degree\",0.01745329251994328,"
                                  "           AUTHORITY[\"EPSG\",\"9122\"]],"
                                  "           AUTHORITY[\"EPSG\",\"4326\"]]" );

    
    
    int nFiles = CSLCount (papszDirList);

    /***** alocate memory for the layer array *****/

    papoLayers =
        ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) * nFiles);
    
    int iFile;
    for (iFile = 0; iFile < nFiles; iFile++) {

        if ( !EQUAL ( CPLGetExtension ( papszDirList[iFile] ), "kml" ) )
            continue;
        
        /***** read the file *****/
        std::string oKmlKml;
        const char *pszFilePath = CPLFormFilename(pszFilename, papszDirList[iFile], NULL);
        
        if ( !kmlbase::File::ReadFileToString( pszFilePath, &oKmlKml )) {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "%s is not a valid kml file", pszFilename );
            continue;
        }

        /***** parse the kml into the DOM *****/

        std::string oKmlErrors;
        ElementPtr poKmlRoot = kmldom::Parse ( oKmlKml, &oKmlErrors );

        if ( !poKmlRoot ) {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "ERROR Parseing kml layer %s from %s :%s",
                       pszFilePath,
                       pszFilename, oKmlErrors.c_str (  ) );

            continue;
        }

        /***** create the layer *****/

        OGRLIBKMLLayer *poOgrLayer =
            new OGRLIBKMLLayer ( pszFilePath,
                                 poOgrSRS, wkbUnknown, this,
                                 poKmlRoot );


        papoLayers[nLayers++] = poOgrLayer;
    
 

    }

    delete poOgrSRS;
    
    if (!nLayers) {
        return FALSE;
#warning we need to do cleanup here
    }
    
    return TRUE;
}

/******************************************************************************
 Open()
******************************************************************************/

int OGRLIBKMLDataSource::Open (
    const char *pszFilename,
    int bUpdate )
{

    pszName = CPLStrdup ( pszFilename );
    
    /***** dir *****/

    VSIStatBufL sStatBuf = {};
    if ( !VSIStatL (pszFilename, &sStatBuf) &&
         VSI_ISDIR(sStatBuf.st_mode) )
        return OpenDir ( pszFilename, bUpdate );
        
   /***** kml *****/

    else if ( EQUAL ( CPLGetExtension ( pszFilename ), "kml" ) )
        return OpenKml ( pszFilename, bUpdate );
    
    /***** kmz *****/

    else if ( EQUAL ( CPLGetExtension ( pszFilename ), "kmz" ) )
        return OpenKmz ( pszFilename, bUpdate );

    else
        return FALSE;

    return TRUE;
}

/******************************************************************************
 Create()


 env vars:
  LIBKML_USE_DOC.KML         default: yes
 
******************************************************************************/

int OGRLIBKMLDataSource::Create (
    const char *pszFilename,
    char **papszOptions )
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

    /*else */ if ( EQUAL ( CPLGetExtension ( pszFilename ), "kmz" ) ) {

        if ( !( m_poKmlKmzfile = KmzFile::Create ( pszFilename ) ) ) {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "Failed to create kmz file %s.", pszFilename );
            return false;
        }

        nLayers = 0;

        /***** set up the factory everything will use *****/

        m_poKmlFactory = KmlFactory::GetFactory (  );

#warning all files should be added to the kmz at dataset close
        
        /***** create the doc.kml it has to be the first file in *****/

        const char *namefield =
            CPLGetConfigOption ( "LIBKML_USE_DOC.KML", "yes" );
        if ( !strcmp ( namefield, "yes" ) ) {
            DocumentPtr poKmlDocument = m_poKmlFactory->CreateDocument (  );

            KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );

            poKmlKml->set_feature ( poKmlDocument );

            std::string strKmlOut = kmldom::SerializePretty ( poKmlKml );

            m_poKmlKmzfile->AddFile ( strKmlOut, "Doc.Kml" );

            m_poKmlDoc_kml = poKmlDocument;
        }
    }

    return true;
}

/******************************************************************************
 GetLayer()
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::GetLayer (
    int iLayer )
{
    if ( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];

}

/******************************************************************************
 GetLayerByName()
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::GetLayerByName (
    const char *pszname )
{
    int iLayer = 0;

    for ( iLayer = 0; iLayer < nLayers; iLayer++ ) {
        if ( EQUAL ( pszname, papoLayers[iLayer]->GetName (  ) ) )
            return papoLayers[iLayer];
    }

    return NULL;
}


/******************************************************************************
 DeleteLayer()
******************************************************************************/
OGRErr OGRLIBKMLDataSource::DeleteLayer (
    int )
{

#warning we need to figure this out

    return OGRERR_UNSUPPORTED_OPERATION;
}


/******************************************************************************
 CreateLayer()
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::CreateLayer (
    const char *pszLayerName,
    OGRSpatialReference * poOgrSRS,
    OGRwkbGeometryType eGType,
    char **papszOptions )
{

    DocumentPtr poKmlDocument =
        boost::static_pointer_cast < Document > ( m_poKmlDoc_kml );
    
    OGRLayer *poOgrLayer =
        new OGRLIBKMLLayer ( pszLayerName, poOgrSRS, eGType, this,
                             poKmlDocument );

#warning we should check if the later name already exsists

#warning minizip adds a seconf file with the same name, it can not overwrite

    /***** add the layer to doc.kml *****/

    /***** is our dataset a kmz? *****/

    if ( m_poKmlKmzfile ) {

        poKmlDocument =
            boost::static_pointer_cast < Document > ( m_poKmlDoc_kml );

        NetworkLinkPtr poKmlNetworkLink =
            m_poKmlFactory->CreateNetworkLink (  );
        LinkPtr poKmlLink = m_poKmlFactory->CreateLink (  );

#warning we need to clean up poKmlKml
        KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );

        std::string oHref;
        oHref.append ( pszLayerName );
        oHref.append ( ".kml" );
        poKmlLink->set_href ( oHref );

        poKmlNetworkLink->set_link ( poKmlLink );
        poKmlDocument->add_feature ( poKmlNetworkLink );
        poKmlKml->set_feature ( poKmlDocument );

        std::string strKmlOut = kmldom::SerializePretty ( poKmlKml );

        m_poKmlKmzfile->AddFile ( strKmlOut, "Doc.Kml" );
    }


    return poOgrLayer;
}

/******************************************************************************

******************************************************************************/

OGRStyleTable *OGRLIBKMLDataSource::GetStyleTable (
     )
{

    return m_poStyleTable;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTableDirectly (
    OGRStyleTable * poStyleTable )
{

    m_poStyleTable = poStyleTable;

    DocumentPtr poKmlDocument = m_poKmlFactory->CreateDocument (  );
    KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );

    datasetstyletable2kml ( m_poStyleTable, m_poKmlFactory, poKmlDocument );


    poKmlKml->set_feature ( poKmlDocument );
    std::string strKmlOut = kmldom::SerializePretty ( poKmlKml );

    m_poKmlKmzfile->AddFile ( strKmlOut, "style/style.kml" );

    return;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTable (
    OGRStyleTable * poStyleTable )
{

    m_poStyleTable = poStyleTable->Clone (  );

    DocumentPtr poKmlDocument = m_poKmlFactory->CreateDocument (  );
    KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );

    datasetstyletable2kml ( m_poStyleTable, m_poKmlFactory, poKmlDocument );


    poKmlKml->set_feature ( poKmlDocument );
    std::string strKmlOut = kmldom::SerializePretty ( poKmlKml );

    m_poKmlKmzfile->AddFile ( strKmlOut, "style/style.kml" );

    return;
}


/******************************************************************************
 TestCapability()
******************************************************************************/

int OGRLIBKMLDataSource::TestCapability (
    const char *pszCap )
{

    if ( EQUAL ( pszCap, ODsCCreateLayer ) )
        return TRUE;
#warning minizip cannot delete a file from an archive we need to test for this case
    else if ( EQUAL ( pszCap, ODsCDeleteLayer ) )
        return bUpdate;
    else
        return FALSE;

}
