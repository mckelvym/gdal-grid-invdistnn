/******************************************************************************
 * $Id$
 *
 * Project:  OGR
 * Purpose:  Implements OGRGMLDataSource class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.15  2004/01/29 15:30:40  warmerda
 * cleanup layer and field names
 *
 * Revision 1.14  2004/01/15 20:53:16  warmerda
 * Ensure that ogr namespace is defined.
 *
 * Revision 1.13  2003/05/21 03:48:35  warmerda
 * Expand tabs
 *
 * Revision 1.12  2003/04/03 16:23:49  warmerda
 * Added support for XSISCHEMA creation option which may be INTERNAL, EXTERNAL
 * or OFF.  EXTERNAL (write an associated .xsd file) is the default.
 *
 * Revision 1.11  2003/03/13 14:40:00  warmerda
 * added missing xs:sequence
 *
 * Revision 1.10  2003/03/11 01:26:35  warmerda
 * Fixed redeclared variable.
 *
 * Revision 1.9  2003/03/07 14:53:21  warmerda
 * implement preliminary BXFS write support
 *
 * Revision 1.8  2003/01/22 17:20:24  warmerda
 * fixed xsi:schemaLocation output
 *
 * Revision 1.7  2003/01/17 20:40:06  warmerda
 * added bounding rectangle support and XSISCHEMAURI option
 *
 * Revision 1.6  2003/01/07 22:30:18  warmerda
 * Added special support for output filename "stdout".
 *
 * Revision 1.5  2002/08/15 15:26:12  warmerda
 * Properly close file if test open fails.
 *
 * Revision 1.4  2002/03/06 20:08:47  warmerda
 * save/use .gfs file by default
 *
 * Revision 1.3  2002/01/25 20:54:23  warmerda
 * fixed last fix
 *
 * Revision 1.2  2002/01/25 20:46:26  warmerda
 * recover if CreateGMLReader() fails
 *
 * Revision 1.1  2002/01/25 20:37:02  warmerda
 * New
 *
 */

#include "ogr_gml.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                         OGRGMLDataSource()                         */
/************************************************************************/

OGRGMLDataSource::OGRGMLDataSource()

{
    pszName = NULL;
    papoLayers = NULL;
    nLayers = 0;

    poReader = NULL;
    fpOutput = NULL;

    papszCreateOptions = NULL;
}

/************************************************************************/
/*                        ~OGRGMLDataSource()                         */
/************************************************************************/

OGRGMLDataSource::~OGRGMLDataSource()

{

    if( fpOutput != NULL )
    {
        VSIFPrintf( fpOutput, "%s", 
                    "</ogr:FeatureCollection>\n" );

        InsertHeader();

        if( nBoundedByLocation != -1 
            && sBoundingRect.IsInit() 
            && VSIFSeek( fpOutput, nBoundedByLocation, SEEK_SET ) == 0 )
        {
            VSIFPrintf( fpOutput, "  <gml:boundedBy>\n" );
            VSIFPrintf( fpOutput, "    <gml:Box>\n" );
            VSIFPrintf( fpOutput, 
                        "      <gml:coord><gml:X>%.16g</gml:X>"
                        "<gml:Y>%.16g</gml:Y></gml:coord>\n",
                        sBoundingRect.MinX, sBoundingRect.MinY );
            VSIFPrintf( fpOutput, 
                        "      <gml:coord><gml:X>%.16g</gml:X>"
                        "<gml:Y>%.16g</gml:Y></gml:coord>\n",
                        sBoundingRect.MaxX, sBoundingRect.MaxY );
            VSIFPrintf( fpOutput, "    </gml:Box>\n" );
            VSIFPrintf( fpOutput, "  </gml:boundedBy>" );
        }

        if( fpOutput != stdout )
            VSIFClose( fpOutput );
    }

    CSLDestroy( papszCreateOptions );
    CPLFree( pszName );

    for( int i = 0; i < nLayers; i++ )
        delete papoLayers[i];
    
    CPLFree( papoLayers );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

int OGRGMLDataSource::Open( const char * pszNewName, int bTestOpen )

{
    FILE        *fp;
    char        szHeader[1000];

/* -------------------------------------------------------------------- */
/*      Open the source file.                                           */
/* -------------------------------------------------------------------- */
    fp = VSIFOpen( pszNewName, "r" );
    if( fp == NULL )
    {
        if( !bTestOpen )
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "Failed to open GML file `%s'.", 
                      pszNewName );

        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      If we aren't sure it is GML, load a header chunk and check      */
/*      for signs it is GML                                             */
/* -------------------------------------------------------------------- */
    if( bTestOpen )
    {
        VSIFRead( szHeader, 1, sizeof(szHeader), fp );
        szHeader[sizeof(szHeader)-1] = '\0';

        if( szHeader[0] != '<' 
            && strstr(szHeader,"opengis.net/gml") == NULL )
        {
            VSIFClose( fp );
            return FALSE;
        }
    }
    
/* -------------------------------------------------------------------- */
/*      We assume now that it is GML.  Close and instantiate a          */
/*      GMLReader on it.                                                */
/* -------------------------------------------------------------------- */
    VSIFClose( fp );
    
    poReader = CreateGMLReader();
    if( poReader == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "File %s appears to be GML but the GML reader can't\n"
                  "be instantiated, likely because Xerces support wasn't\n"
                  "configured in.", 
                  pszNewName );
        return FALSE;
    }

    poReader->SetSourceFile( pszNewName );
    
    pszName = CPLStrdup( pszNewName );

/* -------------------------------------------------------------------- */
/*      Can we find a GML Feature Schema (.gfs) for the input file?     */
/* -------------------------------------------------------------------- */
    const char *pszGFSFilename;
    VSIStatBuf sGFSStatBuf, sGMLStatBuf;
    int        bHaveSchema = FALSE;

    pszGFSFilename = CPLResetExtension( pszNewName, "gfs" );
    if( CPLStat( pszGFSFilename, &sGFSStatBuf ) == 0 )
    {
        CPLStat( pszNewName, &sGMLStatBuf );

        if( sGMLStatBuf.st_mtime > sGFSStatBuf.st_mtime )
        {
            CPLDebug( "GML", 
                      "Found %s but ignoring because it appears\n"
                      "be older than the associated GML file.", 
                      pszGFSFilename );
        }
        else
        {
            bHaveSchema = poReader->LoadClasses( pszGFSFilename );
        }
    }

/* -------------------------------------------------------------------- */
/*      Force a first pass to establish the schema.  Eventually we      */
/*      will have mechanisms for remembering the schema and related     */
/*      information.                                                    */
/* -------------------------------------------------------------------- */
    if( !bHaveSchema && !poReader->PrescanForSchema( TRUE ) )
    {
        // we assume an errors have been reported.
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Save the schema file if possible.  Don't make a fuss if we      */
/*      can't ... could be read-only directory or something.            */
/* -------------------------------------------------------------------- */
    if( !bHaveSchema )
    {
        FILE    *fp = NULL;

        pszGFSFilename = CPLResetExtension( pszNewName, "gfs" );
        if( CPLStat( pszGFSFilename, &sGFSStatBuf ) != 0 
            && (fp = VSIFOpen( pszGFSFilename, "wt" )) != NULL )
        {
            VSIFClose( fp );
            poReader->SaveClasses( pszGFSFilename );
        }
        else
        {
            CPLDebug("GML", 
                     "Not saving %s files already exists or can't be created.",
                     pszGFSFilename );
        }
    }

/* -------------------------------------------------------------------- */
/*      Translate the GMLFeatureClasses into layers.                    */
/* -------------------------------------------------------------------- */
    papoLayers = (OGRGMLLayer **)
        CPLCalloc( sizeof(OGRGMLLayer *), poReader->GetClassCount());
    nLayers = 0;

    while( nLayers < poReader->GetClassCount() )
    {
        papoLayers[nLayers] = TranslateGMLSchema(poReader->GetClass(nLayers));
        nLayers++;
    }
    

    
    return TRUE;
}

/************************************************************************/
/*                         TranslateGMLSchema()                         */
/************************************************************************/

OGRGMLLayer *OGRGMLDataSource::TranslateGMLSchema( GMLFeatureClass *poClass )

{
    OGRGMLLayer *poLayer;

/* -------------------------------------------------------------------- */
/*      Create an empty layer.                                          */
/* -------------------------------------------------------------------- */
    poLayer = new OGRGMLLayer( poClass->GetName(), NULL, FALSE, 
                               wkbUnknown, this );

/* -------------------------------------------------------------------- */
/*      Added attributes (properties).                                  */
/* -------------------------------------------------------------------- */
    for( int iField = 0; iField < poClass->GetPropertyCount(); iField++ )
    {
        GMLPropertyDefn *poProperty = poClass->GetProperty( iField );
        OGRFieldType eFType;

        if( poProperty->GetType() == GMLPT_Untyped )
            eFType = OFTString;
        else if( poProperty->GetType() == GMLPT_String )
            eFType = OFTString;
        else if( poProperty->GetType() == GMLPT_Integer )
            eFType = OFTInteger;
        else if( poProperty->GetType() == GMLPT_Real )
            eFType = OFTReal;
        else
            eFType = OFTString;
        
        OGRFieldDefn oField( poProperty->GetName(), eFType );
        poLayer->GetLayerDefn()->AddFieldDefn( &oField );
    }

    return poLayer;
}

/************************************************************************/
/*                               Create()                               */
/************************************************************************/

int OGRGMLDataSource::Create( const char *pszFilename, 
                              char **papszOptions )

{
    if( fpOutput != NULL || poReader != NULL )
    {
        CPLAssert( FALSE );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Create the output file.                                         */
/* -------------------------------------------------------------------- */
    pszName = CPLStrdup( pszFilename );

    if( EQUAL(pszFilename,"stdout") )
        fpOutput = stdout;
    else
        fpOutput = VSIFOpen( pszFilename, "wt+" );
    if( fpOutput == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Failed to create GML file %s.", 
                  pszFilename );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Write out "standard" header.                                    */
/* -------------------------------------------------------------------- */
    VSIFPrintf( fpOutput, "%s", 
                "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" );

    nSchemaInsertLocation = VSIFTell( fpOutput );

    VSIFPrintf( fpOutput, "%s", 
                "<ogr:FeatureCollection\n" );

/* -------------------------------------------------------------------- */
/*      Write out schema info if provided in creation options.          */
/* -------------------------------------------------------------------- */
    const char *pszSchemaURI = CSLFetchNameValue(papszOptions,"XSISCHEMAURI");
    const char *pszSchemaOpt = CSLFetchNameValue( papszOptions, "XSISCHEMA" );

    if( pszSchemaURI != NULL )
    {
        VSIFPrintf( fpOutput, 
              "     xmlns:xsi=\"http://www.w3c.org/2001/XMLSchema-instance\"\n"
              "     xsi:schemaLocation=\"%s\"\n", 
                    CSLFetchNameValue( papszOptions, "XSISCHEMAURI" ) );
    }
    else if( pszSchemaOpt == NULL || EQUAL(pszSchemaOpt,"EXTERNAL") )
    {
        char *pszBasename = CPLStrdup(CPLGetBasename( pszName ));

        VSIFPrintf( fpOutput, 
              "     xmlns:xsi=\"http://www.w3c.org/2001/XMLSchema-instance\"\n"
              "     xsi:schemaLocation=\". %s\"\n", 
                    CPLResetExtension( pszBasename, "xsd" ) );
        CPLFree( pszBasename );
    }

    VSIFPrintf( fpOutput, "%s", 
                "     xmlns:ogr=\"http://ogr.maptools.org/\"\n" );
    VSIFPrintf( fpOutput, "%s", 
                "     xmlns:gml=\"http://www.opengis.net/gml\">\n" );

/* -------------------------------------------------------------------- */
/*      Should we initialize an area to place the boundedBy element?    */
/*      We will need to seek back to fill it in.                        */
/* -------------------------------------------------------------------- */
    if( CSLFetchBoolean( papszOptions, "BOUNDEDBY", TRUE ) )
    {
        nBoundedByLocation = VSIFTell( fpOutput );

        if( nBoundedByLocation != -1 )
            VSIFPrintf( fpOutput, "%280s\n", "" );
    }
    else
        nBoundedByLocation = -1;

    return TRUE;
}

/************************************************************************/
/*                            CreateLayer()                             */
/************************************************************************/

OGRLayer *
OGRGMLDataSource::CreateLayer( const char * pszLayerName,
                               OGRSpatialReference *poSRS,
                               OGRwkbGeometryType eType,
                               char ** papszOptions )

{
/* -------------------------------------------------------------------- */
/*      Verify we are in update mode.                                   */
/* -------------------------------------------------------------------- */
    if( fpOutput == NULL )
    {
        CPLError( CE_Failure, CPLE_NoWriteAccess,
                  "Data source %s opened for read access.\n"
                  "New layer %s cannot be created.\n",
                  pszName, pszLayerName );

        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Ensure name is safe as an element name.                         */
/* -------------------------------------------------------------------- */
    char *pszCleanLayerName = CPLStrdup( pszLayerName );

    CPLCleanXMLElementName( pszCleanLayerName );
    if( strcmp(pszCleanLayerName,pszLayerName) != 0 )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Layer name '%s' adjusted to '%s' for XML validity.",
                  pszLayerName, pszCleanLayerName );
    }

/* -------------------------------------------------------------------- */
/*      Create the layer object.                                        */
/* -------------------------------------------------------------------- */
    OGRGMLLayer *poLayer;

    poLayer = new OGRGMLLayer( pszCleanLayerName, poSRS, TRUE, eType, this );

    CPLFree( pszCleanLayerName );

/* -------------------------------------------------------------------- */
/*      Add layer to data source layer list.                            */
/* -------------------------------------------------------------------- */
    papoLayers = (OGRGMLLayer **)
        CPLRealloc( papoLayers,  sizeof(OGRGMLLayer *) * (nLayers+1) );
    
    papoLayers[nLayers++] = poLayer;

    return poLayer;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRGMLDataSource::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,ODsCCreateLayer) )
        return TRUE;
    else
        return FALSE;
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/

OGRLayer *OGRGMLDataSource::GetLayer( int iLayer )

{
    if( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];
}

/************************************************************************/
/*                            GrowExtents()                             */
/************************************************************************/

void OGRGMLDataSource::GrowExtents( OGREnvelope *psGeomBounds )

{
    sBoundingRect.Merge( *psGeomBounds );
}

/************************************************************************/
/*                            InsertHeader()                            */
/*                                                                      */
/*      This method is used to update boundedby info for a              */
/*      dataset, and insert schema descriptions depending on            */
/*      selection options in effect.                                    */
/************************************************************************/

void OGRGMLDataSource::InsertHeader()

{
    FILE        *fpSchema;
    int         nSchemaStart;

    if( fpOutput == NULL || fpOutput == stdout )
        return;

/* -------------------------------------------------------------------- */
/*      Do we want to write the schema within the GML instance doc      */
/*      or to a separate file?  For now we only support external.       */
/* -------------------------------------------------------------------- */
    const char *pszSchemaURI = CSLFetchNameValue(papszCreateOptions,
                                                 "XSISCHEMAURI");
    const char *pszSchemaOpt = CSLFetchNameValue( papszCreateOptions, 
                                                  "XSISCHEMA" );

    if( pszSchemaURI != NULL )
        return;

    if( pszSchemaOpt == NULL || EQUAL(pszSchemaOpt,"EXTERNAL") )
    {
        const char *pszXSDFilename = CPLResetExtension( pszName, "xsd" );

        fpSchema = VSIFOpen( pszXSDFilename, "wt" );
        if( fpSchema == NULL )
        {
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "Failed to open file %.500s for schema output.", 
                      pszXSDFilename );
            return;
        }
        fprintf( fpSchema, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    }
    else if( EQUAL(pszSchemaOpt,"INTERNAL") )
    {
        nSchemaStart = VSIFTell( fpOutput );
        fpSchema = fpOutput;
    }
    else                                                               
        return;

/* ==================================================================== */
/*      Write the schema section at the end of the file.  Once          */
/*      complete, we will read it back in, and then move the whole      */
/*      file "down" enough to insert the schema at the beginning.       */
/* ==================================================================== */

/* -------------------------------------------------------------------- */
/*      Emit the start of the schema section.                           */
/* -------------------------------------------------------------------- */
    const char *pszTargetNameSpace = "http://gdal.velocet.ca/ogr";
    const char *pszPrefix = "ogr";

    VSIFPrintf( fpSchema, 
                "<xs:schema targetNamespace=\"%s\" xmlns:%s=\"%s\" xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" xmlns:gml=\"http://www.opengis.net/gml\" elementFormDefault=\"qualified\" version=\"1.0\">\n", 
                pszTargetNameSpace, pszPrefix, pszTargetNameSpace );
    
    VSIFPrintf( fpSchema, 
                "<xs:import namespace=\"http://www.opengis.net/gml\" schemaLocation=\"http://schemas.cubewerx.com/schemas/gml/2.1.2/feature.xsd\"/>" );

/* -------------------------------------------------------------------- */
/*      Define the FeatureCollection                                    */
/* -------------------------------------------------------------------- */
    VSIFPrintf( fpSchema, 
                "<xs:element name=\"FeatureCollection\" type=\"%s:FeatureCollectionType\" substitutionGroup=\"gml:_FeatureCollection\"/>\n", 
                pszPrefix );

    VSIFPrintf( 
        fpSchema, 
        "<xs:complexType name=\"FeatureCollectionType\">\n"
        "  <xs:complexContent>\n"
        "    <xs:extension base=\"gml:AbstractFeatureCollectionType\">\n"
        "      <xs:attribute name=\"lockId\" type=\"xs:string\" use=\"optional\"/>\n"
        "      <xs:attribute name=\"scope\" type=\"xs:string\" use=\"optional\"/>\n"
        "    </xs:extension>\n"
        "  </xs:complexContent>\n"
        "</xs:complexType>\n" );

/* ==================================================================== */
/*      Define the schema for each layer.                               */
/* ==================================================================== */
    int iLayer;

    for( iLayer = 0; iLayer < GetLayerCount(); iLayer++ )
    {
        OGRFeatureDefn *poFDefn = GetLayer(iLayer)->GetLayerDefn();
        
/* -------------------------------------------------------------------- */
/*      Emit initial stuff for a feature type.                          */
/* -------------------------------------------------------------------- */
        VSIFPrintf( 
            fpSchema,
            "<xs:element name=\"%s\" type=\"%s:%s_Type\" substitutionGroup=\"gml:_Feature\"/>\n",
            poFDefn->GetName(), pszPrefix, poFDefn->GetName() );

        VSIFPrintf( 
            fpSchema, 
            "<xs:complexType name=\"%s_Type\">\n"
            "  <xs:complexContent>\n"
            "    <xs:extension base=\"gml:AbstractFeatureType\">\n"
            "      <xs:sequence>\n",
            poFDefn->GetName() );

/* -------------------------------------------------------------------- */
/*      Define the geometry attribute.  For now I always use the        */
/*      generic geometry type, but eventually we should express         */
/*      particulars if available.                                       */
/* -------------------------------------------------------------------- */
        VSIFPrintf( 
            fpSchema,
            "<xs:element name=\"geometryProperty\" type=\"gml:geometryPropertyType\" nillable=\"true\" minOccurs=\"1\" maxOccurs=\"1\"/>\n" );
            
/* -------------------------------------------------------------------- */
/*      Emit each of the attributes.                                    */
/* -------------------------------------------------------------------- */
        for( int iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
        {
            OGRFieldDefn *poFieldDefn = poFDefn->GetFieldDefn(iField);

            if( poFieldDefn->GetType() == OFTInteger )
            {
                int nWidth;

                if( poFieldDefn->GetWidth() > 0 )
                    nWidth = poFieldDefn->GetWidth();
                else
                    nWidth = 16;

                VSIFPrintf( fpSchema, 
                            "    <xs:element name=\"%s\" nillable=\"true\" minOccurs=\"0\" maxOccurs=\"1\">\n"
                            "      <xs:simpleType>\n"
                            "        <xs:restriction base=\"xs:integer\">\n"
                            "          <xs:totalDigits value=\"%d\"/>\n"
                            "        </xs:restriction>\n"
                            "      </xs:simpleType>\n"
                            "    </xs:element>\n",
                            poFieldDefn->GetNameRef(), nWidth );
            }
            else if( poFieldDefn->GetType() == OFTReal )
            {
                int nWidth, nDecimals;

                if( poFieldDefn->GetPrecision() == 0 )
                    nDecimals = 16;
                else
                    nDecimals = poFieldDefn->GetPrecision();

                if( poFieldDefn->GetWidth() > 0 )
                    nWidth = poFieldDefn->GetWidth();
                else
                    nWidth = 33;

                VSIFPrintf( fpSchema, 
                            "    <xs:element name=\"%s\" nillable=\"true\" minOccurs=\"0\" maxOccurs=\"1\">\n"
                            "      <xs:simpleType>\n"
                            "        <xs:restriction base=\"xs:decimal\">\n"
                            "          <xs:totalDigits value=\"%d\"/>\n"
                            "          <xs:fractionDigits value=\"%d\"/>\n"
                            "        </xs:restriction>\n"
                            "      </xs:simpleType>\n"
                            "    </xs:element>\n",
                            poFieldDefn->GetNameRef(), nWidth, nDecimals );
            }
            else if( poFieldDefn->GetType() == OFTString )
            {
                char szMaxLength[48];

                if( poFieldDefn->GetWidth() == 0 )
                    sprintf( szMaxLength, "unbounded" );
                else
                    sprintf( szMaxLength, "%d", poFieldDefn->GetWidth() );

                VSIFPrintf( fpSchema, 
                            "    <xs:element name=\"%s\" nillable=\"true\" minOccurs=\"0\" maxOccurs=\"1\">\n"
                            "      <xs:simpleType>\n"
                            "        <xs:restriction base=\"xs:string\">\n"
                            "          <xs:maxLength value=\"%s\"/>\n"
                            "        </xs:restriction>\n"
                            "      </xs:simpleType>\n"
                            "    </xs:element>\n",
                            poFieldDefn->GetNameRef(), szMaxLength );
            }
            else
            {
                /* TODO */
            }
        } /* next field */

/* -------------------------------------------------------------------- */
/*      Finish off feature type.                                        */
/* -------------------------------------------------------------------- */
        VSIFPrintf( fpSchema, 
                    "      </xs:sequence>\n"
                    "    </xs:extension>\n"
                    "  </xs:complexContent>\n"
                    "</xs:complexType>\n" );
    } /* next layer */

    VSIFPrintf( fpSchema, "</xs:schema>\n" );

/* ==================================================================== */
/*      Move schema to the start of the file.                           */
/* ==================================================================== */
    if( fpSchema == fpOutput )
    {
/* -------------------------------------------------------------------- */
/*      Read the schema into memory.                                    */
/* -------------------------------------------------------------------- */
        int nSchemaSize = VSIFTell( fpOutput ) - nSchemaStart;
        char *pszSchema = (char *) CPLMalloc(nSchemaSize+1);
    
        VSIFSeek( fpOutput, nSchemaStart, SEEK_SET );

        VSIFRead( pszSchema, 1, nSchemaSize, fpOutput );
        pszSchema[nSchemaSize] = '\0';
    
/* -------------------------------------------------------------------- */
/*      Move file data down by "schema size" bytes from after <?xml>    */
/*      header so we have room insert the schema.  Move in pretty       */
/*      big chunks.                                                     */
/* -------------------------------------------------------------------- */
        int nChunkSize = MIN(nSchemaStart-nSchemaInsertLocation,250000);
        char *pszChunk = (char *) CPLMalloc(nChunkSize);
        int nEndOfUnmovedData = nSchemaStart;

        for( nEndOfUnmovedData = nSchemaStart;
             nEndOfUnmovedData > nSchemaInsertLocation; )
        {
            int nBytesToMove = 
                MIN(nChunkSize, nEndOfUnmovedData - nSchemaInsertLocation );

            VSIFSeek( fpOutput, nEndOfUnmovedData - nBytesToMove, SEEK_SET );
            VSIFRead( pszChunk, 1, nBytesToMove, fpOutput );
            VSIFSeek( fpOutput, nEndOfUnmovedData - nBytesToMove + nSchemaSize, 
                      SEEK_SET );
            VSIFWrite( pszChunk, 1, nBytesToMove, fpOutput );
        
            nEndOfUnmovedData -= nBytesToMove;
        }

        CPLFree( pszChunk );

/* -------------------------------------------------------------------- */
/*      Write the schema in the opened slot.                            */
/* -------------------------------------------------------------------- */
        VSIFSeek( fpOutput, nSchemaInsertLocation, SEEK_SET );
        VSIFWrite( pszSchema, 1, nSchemaSize, fpOutput );

        VSIFSeek( fpOutput, 0, SEEK_END );

        nBoundedByLocation += nSchemaSize;
    }
/* -------------------------------------------------------------------- */
/*      Close external schema files.                                    */
/* -------------------------------------------------------------------- */
    else
        VSIFClose( fpSchema );
}
