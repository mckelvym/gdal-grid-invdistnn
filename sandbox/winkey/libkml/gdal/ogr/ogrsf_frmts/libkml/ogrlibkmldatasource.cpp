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

//#include <sstream>
#include <kml/dom.h>
#include <kml/base/file.h>

using kmldom::KmlFactory;
using kmldom::DocumentPtr;
using kmldom::FolderPtr;
using kmldom::FeaturePtr;
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
    nAlloced = 0;
    
    bUpdated = FALSE;
    
    m_isKml = FALSE;
    m_poKmlDSKml = NULL;
    m_poKmlDSContainer = NULL;

    m_isKmz = FALSE;
    m_poKmlDocKml = NULL;
    
    m_poStyleTable = NULL;

}

/******************************************************************************
 method to write a single file ds .kml at ds destroy
******************************************************************************/

void OGRLIBKMLDataSource::WriteKml (
     )
{
    std::string oKmlFilename = pszName;
    if ( m_poKmlDSKml ) {
        std::string oKmlOut = kmldom::SerializePretty ( m_poKmlDSKml );
#warning check the return code
        kmlbase::File::WriteStringToFile ( oKmlOut, oKmlFilename );
    }
    else if ( m_poKmlDSContainer ) {
        std::string oKmlOut = kmldom::SerializePretty ( m_poKmlDSContainer );
#warning check the return code
        kmlbase::File::WriteStringToFile ( oKmlOut, oKmlFilename );
    }

    return;
}

/******************************************************************************
 method to write a ds .kmz at ds destroy
******************************************************************************/

void OGRLIBKMLDataSource::WriteKmz (
     )
{

    KmzFile *poKmlKmzfile = kmlengine::KmzFile::Create ( pszName );

    if ( !poKmlKmzfile ) {
        CPLError ( CE_Failure, CPLE_NoWriteAccess, "ERROR creating %s",
                   pszName );
        return;
    }

    /***** write out the doc.kml ****/

    const char *pszUseDocKml =
        CPLGetConfigOption ( "LIBKML_USE_DOC.KML", "yes" );

    if ( EQUAL ( pszUseDocKml, "yes" ) && m_poKmlDocKml ) {

        KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );
        poKmlKml->set_feature ( m_poKmlDocKml );
        std::string oKmlOut = kmldom::SerializePretty ( poKmlKml );
        poKmlKmzfile->AddFile ( oKmlOut, "doc.kml" );

    }

    /***** loop though the layers and write them *****/

    int iLayer;

    for ( iLayer = 0; iLayer < nLayers; iLayer++ ) {
        ContainerPtr poKlmContainer = papoLayers[iLayer]->GetKmlLayer (  );

        KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );
        poKmlKml->set_feature ( poKlmContainer );
        
        std::string oKmlOut = kmldom::SerializePretty ( poKmlKml );
        std::string oName = papoLayers[iLayer]->GetName (  );
        oName.append(".kml");

        poKmlKmzfile->AddFile ( oKmlOut, oName.c_str() );
    }

    /***** write the style table *****/

#warning implement write out of style table to kmz
    
    return;
}

/******************************************************************************
 ~OGRLIBKMLDataSource()
******************************************************************************/

OGRLIBKMLDataSource::~OGRLIBKMLDataSource (  )
{

    /***** some types of output require the write to happen at the very end *****/

    /***** kml *****/

    if ( bUpdate && bUpdated && IsKml (  ) )
        WriteKml (  );

    /***** kmz *****/

    else if ( bUpdate && bUpdated && IsKmz (  ) ) {

    }




    CPLFree ( pszName );


    for ( int i = 0; i < nLayers; i++ )
        delete papoLayers[i];

    CPLFree ( papoLayers );

    delete m_poStyleTable;

}

/******************************************************************************
 method to open a kml file
******************************************************************************/

int OGRLIBKMLDataSource::OpenKml (
    const char *pszFilename,
    int bUpdate )
{
    std::string oKmlKml;

    if ( !kmlbase::File::ReadFileToString ( pszFilename, &oKmlKml ) ) {
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



    /***** parse the kml into the DOM *****/

    std::string oKmlErrors;

    ElementPtr poKmlRoot = kmldom::Parse ( oKmlKml, &oKmlErrors );

    if ( !poKmlRoot ) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "ERROR Parseing kml %s :%s",
                   pszFilename, oKmlErrors.c_str (  ) );
        delete poOgrSRS;
        return FALSE;
    }

    /***** is the root a <kml> *****/

    if ( poKmlRoot->IsA ( kmldom::Type_kml ) ) {
        m_poKmlDSKml =
            boost::static_pointer_cast < kmldom::Kml > ( poKmlRoot );

        FeaturePtr poKmlFeat = m_poKmlDSKml->get_feature (  );

        /***** is the child of root some type of container *****/

        if ( poKmlFeat->IsA ( kmldom::Type_Container ) )
            m_poKmlDSContainer =
                boost::static_pointer_cast < kmldom::Container > ( poKmlFeat );

        /***** its not we have no layers to read *****/

        else {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "ERROR Parseing kml %s :%s %s",
                       pszFilename, "This file does not fit the OGR model,",
                       "there is no container element at the root." );
            delete poOgrSRS;
            return FALSE;
        }
    }

    /***** is the root some type of container *****/

    else if ( poKmlRoot->IsA ( kmldom::Type_Container ) )
        m_poKmlDSContainer =
            boost::static_pointer_cast < kmldom::Container > ( poKmlRoot );

    /***** its not we have no layers to read *****/

    else {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "ERROR Parseing kml %s :%s %s",
                   pszFilename, "This file does not fit the OGR model,",
                   "there is no container element at the root." );
        delete poOgrSRS;
        return FALSE;
    }

    /***** get how many features the container has *****/

    size_t nKmlFeatures = m_poKmlDSContainer->get_feature_array_size (  );


    /***** alocate memory for the layer array *****/

    papoLayers =
        ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) *
                                          nKmlFeatures );

    nAlloced = nKmlFeatures;

    /***** loop over the container to seperate the style, layers, etc *****/

    int nPlacemarks = 0;
    size_t iKmlFeature;

    for ( iKmlFeature = 0; iKmlFeature < nKmlFeatures; iKmlFeature++ ) {
        FeaturePtr poKmlFeat =
            m_poKmlDSContainer->get_feature_array_at ( iKmlFeature );

        /***** style *****/

        if ( poKmlFeat->IsA ( kmldom::Type_Style ) ) {
            if ( !m_poStyleTable )
                m_poStyleTable = new OGRStyleTable (  );
            ElementPtr poKmlElement =
                boost::static_pointer_cast < kmldom::Element > ( poKmlFeat );
            kml2datasetstyletable ( m_poStyleTable,
                                    boost::static_pointer_cast <
                                    kmldom::Style > ( poKmlElement ) );
        }

        /***** schema *****/

        if ( poKmlFeat->IsA ( kmldom::Type_Schema ) ) {
        }

        /***** container *****/

        if ( poKmlFeat->IsA ( kmldom::Type_Container ) ) {

            /***** see if the container has a name *****/

            std::string oKmlFeatName;
            if ( poKmlFeat->has_name (  ) ) {
                oKmlFeatName = poKmlFeat->get_name (  );
            }

            /***** use the feature index number as the name *****/
            /***** not sure i like this c++ ich *****/

            else {
                std::stringstream oOut;
                oOut << iKmlFeature;
                oKmlFeatName = oOut.str (  );
            }

            /***** create the layer *****/

#warning we need a way to pass schema data for a layerdefn
            OGRLIBKMLLayer *poOgrLayer =
                new OGRLIBKMLLayer ( oKmlFeatName.c_str (  ),
                                     poOgrSRS, wkbUnknown, this,
                                     poKmlFeat );

            papoLayers[nLayers++] = poOgrLayer;
        }

        if ( poKmlFeat->IsA ( kmldom::Type_Placemark ) ) {
            nPlacemarks++;
        }
    }

    /***** if there is placemarks in the root its a layer *****/


    if ( nPlacemarks ) {

#warning the layer name needs to be the basename of the file
#warning we need a way to pass schema data for a layerdefn
        OGRLIBKMLLayer *poOgrLayer = new OGRLIBKMLLayer ( pszFilename,
                                                          poOgrSRS, wkbUnknown,
                                                          this,
                                                          m_poKmlDSContainer );


        papoLayers[nLayers++] = poOgrLayer;
    }

    delete poOgrSRS;

    return TRUE;
}

/******************************************************************************
 method to open a kmz file
******************************************************************************/


int OGRLIBKMLDataSource::OpenKmz (
    const char *pszFilename,
    int bUpdate )
{

    KmzFile *poKmlKmzfile = KmzFile::OpenFromFile ( pszFilename );
    if ( !poKmlKmzfile ) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "%s is not a valid kmz file", pszFilename );
        return FALSE;
    }

    /***** read the doc.kml *****/

    std::string oKmlKml;
    std::string oKmlKmlPath;
    if ( !poKmlKmzfile->ReadKmlAndGetPath ( &oKmlKml, &oKmlKmlPath ) ) {

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
            delete poOgrSRS;
            return FALSE;
        }

        /***** create the layer *****/

        OGRLIBKMLLayer *poOgrLayer =
            new OGRLIBKMLLayer ( oKmlKmlPath.c_str (  ),
                                 poOgrSRS, wkbUnknown, this,
                                 poKmlRoot );


        papoLayers[nLayers++] = poOgrLayer;
    }

    /***** read all the layers the doc.kml points to *****/

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
                if ( poKmlKmzfile->
                     ReadFile ( oKmlHref.get_path (  ).c_str (  ), &oKml ) ) {

                    /***** parse the kml into the DOM *****/

                    std::string oKmlErrors;
                    ElementPtr poKmlRoot = kmldom::Parse ( oKml, &oKmlErrors );

                    if ( !poKmlRoot ) {
                        CPLError ( CE_Failure, CPLE_OpenFailed,
                                   "ERROR Parseing kml layer %s from %s :%s",
                                   oKmlHref.get_path (  ).c_str (  ),
                                   pszFilename, oKmlErrors.c_str (  ) );
                        delete poOgrSRS;
                        return FALSE;
                    }

                    /***** skip over the <kml> we want the container *****/

                    if ( poKmlRoot->IsA ( kmldom::Type_kml ) ) {
                        KmlPtr poKmlKml = boost::static_pointer_cast < kmldom::Kml > ( poKmlRoot );

                        poKmlRoot = poKmlKml->get_feature (  );
                    }

                    if ( poKmlRoot->IsA ( kmldom::Type_Container ) ) {
                         
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
    }

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

    char **papszDirList = NULL;

    if ( !( papszDirList = VSIReadDir ( pszFilename ) ) )
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



    int nFiles = CSLCount ( papszDirList );

    /***** alocate memory for the layer array *****/

    papoLayers =
        ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) *
                                          nFiles );

    int iFile;

    for ( iFile = 0; iFile < nFiles; iFile++ ) {

        if ( !EQUAL ( CPLGetExtension ( papszDirList[iFile] ), "kml" ) )
            continue;

        /***** read the file *****/
        std::string oKmlKml;
        const char *pszFilePath =
            CPLFormFilename ( pszFilename, papszDirList[iFile], NULL );

        if ( !kmlbase::File::ReadFileToString ( pszFilePath, &oKmlKml ) ) {
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
                       pszFilePath, pszFilename, oKmlErrors.c_str (  ) );

            continue;
        }

        /***** create the layer *****/

        OGRLIBKMLLayer *poOgrLayer = new OGRLIBKMLLayer ( pszFilePath,
                                                          poOgrSRS, wkbUnknown,
                                                          this,
                                                          poKmlRoot );


        papoLayers[nLayers++] = poOgrLayer;



    }

    delete poOgrSRS;

    if ( !nLayers ) {
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

    VSIStatBufL sStatBuf = { };
    if ( !VSIStatL ( pszFilename, &sStatBuf ) &&
         VSI_ISDIR ( sStatBuf.st_mode ) )
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
 method to create a single file .kml ds
******************************************************************************/

int OGRLIBKMLDataSource::CreateKml (
    const char *pszFilename,
    char **papszOptions )
{
    m_poKmlFactory = KmlFactory::GetFactory (  );
    m_poKmlDSKml = m_poKmlFactory->CreateKml (  );
    DocumentPtr poKmlDocument = m_poKmlFactory->CreateDocument (  );

    m_poKmlDSKml->set_feature ( poKmlDocument );
    m_poKmlDSContainer = poKmlDocument;
    m_isKml = TRUE;
    
    return true;
}

/******************************************************************************
 method to create a .kmz ds
******************************************************************************/

int OGRLIBKMLDataSource::CreateKmz (
    const char *pszFilename,
    char **papszOptions )
{


    /***** create the doc.kml  *****/

    const char *namefield = CPLGetConfigOption ( "LIBKML_USE_DOC.KML", "yes" );

    if ( !strcmp ( namefield, "yes" ) ) {
        m_poKmlDocKml = m_poKmlFactory->CreateDocument (  );
    }

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

    int bResult = FALSE;

    pszName = CPLStrdup ( pszFilename );
    bUpdate = TRUE;
    
    /***** kml *****/

    if ( EQUAL ( CPLGetExtension ( pszFilename ), "kml" ) ) {
        bResult = CreateKml ( pszFilename, papszOptions );
    }

    /***** kmz *****/

    if ( EQUAL ( CPLGetExtension ( pszFilename ), "kmz" ) ) {
        bResult = CreateKmz ( pszFilename, papszOptions );
    }

    /***** dir *****/

    else {


    }

    return bResult;
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
 method to create a layer in a single file .kml
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::CreateLayerKml (
    const char *pszLayerName,
    OGRSpatialReference * poOgrSRS,
    OGRwkbGeometryType eGType,
    char **papszOptions )
{

    OGRLIBKMLLayer *poOgrLayer = NULL;

    FolderPtr poKmlFolder = m_poKmlFactory->CreateFolder (  );

    m_poKmlDSContainer->add_feature ( poKmlFolder );

    /***** create the layer *****/

    poOgrLayer = new OGRLIBKMLLayer ( pszLayerName, poOgrSRS, eGType, this,
                                      poKmlFolder );

    /***** check to see if we have enough space to store the layer *****/

    if ( nLayers == nAlloced ) {
        void *tmp = CPLRealloc ( papoLayers,
                                 sizeof ( OGRLIBKMLLayer * ) * ++nAlloced );

        papoLayers = ( OGRLIBKMLLayer ** ) tmp;
    }

    /***** add the layer to the array of layers *****/

    papoLayers[nLayers++] = poOgrLayer;

    return ( OGRLayer * ) poOgrLayer;
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

    OGRLayer *poOgrLayer = NULL;

    /***** kml DS *****/

    if ( IsKml (  ) ) {
        poOgrLayer = CreateLayerKml ( pszLayerName, poOgrSRS,
                                      eGType, papszOptions );

    }




#warning we should check if the later name already exsists

#warning minizip adds a seconf file with the same name, it can not overwrite

    /***** add the layer to doc.kml *****/

    /***** is our dataset a kmz? *****/
/*
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

*/

    /***** mark the dataset as updated *****/

    if ( poOgrLayer )
        bUpdated = TRUE;

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
    method to write a style table to a single file .kml ds
******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTable2Kml (
    OGRStyleTable * poStyleTable )
{
            /***** go though the root features looking for styles *****/

    size_t nKmlFeatures = m_poKmlDSContainer->get_feature_array_size (  );
    size_t iKmlFeature;

    for ( iKmlFeature = 0; iKmlFeature < nKmlFeatures; iKmlFeature++ ) {
        FeaturePtr poKmlFeat =
            m_poKmlDSContainer->get_feature_array_at ( iKmlFeature );

            /***** is it a style *****/

        if ( poKmlFeat->IsA ( kmldom::Type_Style ) ) {

                /***** see if it has an id *****/

            if ( poKmlFeat->has_id (  ) ) {
                const std::string oKmlID = poKmlFeat->get_id (  );

                    /***** delete the style *****/

                m_poKmlDSContainer->DeleteFeatureById ( oKmlID );
            }
        }
    }

        /***** add the new style table to the container *****/

    datasetstyletable2kml ( poStyleTable, m_poKmlFactory, m_poKmlDSContainer );

    return;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTableDirectly (
    OGRStyleTable * poStyleTable )
{

    m_poStyleTable = poStyleTable;

    /***** a kml datasource? *****/

    if ( IsKml (  ) )
        SetStyleTable2Kml ( m_poStyleTable );






/*
   
    

    
    DocumentPtr poKmlDocument = m_poKmlFactory->CreateDocument (  );
    KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );

    datasetstyletable2kml ( m_poStyleTable, m_poKmlFactory, poKmlDocument );


    poKmlKml->set_feature ( poKmlDocument );
    std::string strKmlOut = kmldom::SerializePretty ( poKmlKml );

    m_poKmlKmzfile->AddFile ( strKmlOut, "style/style.kml" );
*/
    return;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTable (
    OGRStyleTable * poStyleTable )
{

    SetStyleTableDirectly ( poStyleTable->Clone (  ) );

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
