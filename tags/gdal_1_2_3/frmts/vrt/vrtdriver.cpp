/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Implementation of VRTDriver
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.7  2004/08/11 18:45:56  warmerda
 * Use CopyCommonInfoFrom method
 *
 * Revision 1.6  2004/07/28 16:56:36  warmerda
 * updated to use VRTSourcedRasterBand
 *
 * Revision 1.5  2004/04/15 18:54:38  warmerda
 * added UnitType, Offset, Scale and CategoryNames support
 *
 * Revision 1.4  2004/04/02 17:20:06  warmerda
 * Added defaulte extension, and point help topic to VRT tutorial.
 *
 * Revision 1.3  2004/03/16 18:34:35  warmerda
 * added support for relativeToVRT attribute on SourceFilename
 *
 * Revision 1.2  2003/07/27 11:16:06  dron
 * Check NULL pointer in GetMetadata()/SetMetadata().
 *
 * Revision 1.1  2003/07/17 20:27:18  warmerda
 * New
 *
 */

#include "vrtdataset.h"
#include "cpl_minixml.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                             VRTDriver()                              */
/************************************************************************/

VRTDriver::VRTDriver()

{
    papszSourceParsers = NULL;
}

/************************************************************************/
/*                             ~VRTDriver()                             */
/************************************************************************/

VRTDriver::~VRTDriver()

{
    CSLDestroy( papszSourceParsers );
}

/************************************************************************/
/*                            GetMetadata()                             */
/************************************************************************/

char **VRTDriver::GetMetadata( const char *pszDomain )

{
    if( pszDomain && EQUAL(pszDomain,"SourceParsers") )
        return papszSourceParsers;
    else
        return GDALDriver::GetMetadata( pszDomain );
}

/************************************************************************/
/*                            SetMetadata()                             */
/************************************************************************/

CPLErr VRTDriver::SetMetadata( char **papszMetadata, const char *pszDomain )

{
    if( pszDomain && EQUAL(pszDomain,"SourceParsers") )
    {
        CSLDestroy( papszSourceParsers );
        papszSourceParsers = CSLDuplicate( papszMetadata );
        return CE_None;
    }
    else
        return GDALDriver::SetMetadata( papszMetadata, pszDomain );
}

/************************************************************************/
/*                          AddSourceParser()                           */
/************************************************************************/

void VRTDriver::AddSourceParser( const char *pszElementName, 
                                 VRTSourceParser pfnParser )

{
    char szPtrValue[128];

    sprintf( szPtrValue, "%p", pfnParser );
    papszSourceParsers = CSLSetNameValue( papszSourceParsers, 
                                          pszElementName, szPtrValue );
}

/************************************************************************/
/*                            ParseSource()                             */
/************************************************************************/

VRTSource *VRTDriver::ParseSource( CPLXMLNode *psSrc, const char *pszVRTPath )

{
    const char *pszParserFunc;

    if( psSrc == NULL || psSrc->eType != CXT_Element )
        return NULL;

    pszParserFunc = CSLFetchNameValue( papszSourceParsers, psSrc->pszValue );
    if( pszParserFunc == NULL )
        return NULL;

    VRTSourceParser pfnParser = NULL;

    sscanf( pszParserFunc, "%p", &pfnParser );

    if( pfnParser == NULL )
        return NULL;
    else
        return pfnParser( psSrc, pszVRTPath );
}

/************************************************************************/
/*                           VRTCreateCopy()                            */
/************************************************************************/

static GDALDataset *
VRTCreateCopy( const char * pszFilename, GDALDataset *poSrcDS, 
               int bStrict, char ** papszOptions, 
               GDALProgressFunc pfnProgress, void * pProgressData )

{
    VRTDataset *poVRTDS;

    (void) bStrict;
    (void) papszOptions;

/* -------------------------------------------------------------------- */
/*      If the source dataset is a virtual dataset then just write      */
/*      it to disk as a special case to avoid extra layers of           */
/*      indirection.                                                    */
/* -------------------------------------------------------------------- */
    if( EQUAL(poSrcDS->GetDriver()->GetDescription(),"VRT") )
    {
        FILE *fpVRT;

        fpVRT = VSIFOpen( pszFilename, "w" );

    /* -------------------------------------------------------------------- */
    /*      Convert tree to a single block of XML text.                     */
    /* -------------------------------------------------------------------- */
        char *pszVRTPath = CPLStrdup(CPLGetPath(pszFilename));
        CPLXMLNode *psDSTree = ((VRTDataset *) poSrcDS)->SerializeToXML( pszVRTPath );
        char *pszXML;
        
        pszXML = CPLSerializeXMLTree( psDSTree );
        
        CPLDestroyXMLNode( psDSTree );

        CPLFree( pszVRTPath );
        
    /* -------------------------------------------------------------------- */
    /*      Write to disk.                                                  */
    /* -------------------------------------------------------------------- */
        VSIFWrite( pszXML, 1, strlen(pszXML), fpVRT );
        VSIFClose( fpVRT );

        CPLFree( pszXML );
        
        return (GDALDataset *) GDALOpen( pszFilename, GA_Update );
    }

/* -------------------------------------------------------------------- */
/*      Create the virtual dataset.                                     */
/* -------------------------------------------------------------------- */
    poVRTDS = (VRTDataset *) 
        VRTDataset::Create( pszFilename, 
                            poSrcDS->GetRasterXSize(),
                            poSrcDS->GetRasterYSize(),
                            0, GDT_Byte, NULL );

/* -------------------------------------------------------------------- */
/*      Do we have a geotransform?                                      */
/* -------------------------------------------------------------------- */
    double adfGeoTransform[6];

    if( poSrcDS->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        poVRTDS->SetGeoTransform( adfGeoTransform );
    }

/* -------------------------------------------------------------------- */
/*      Copy projection                                                 */
/* -------------------------------------------------------------------- */
    poVRTDS->SetProjection( poSrcDS->GetProjectionRef() );

/* -------------------------------------------------------------------- */
/*      Emit dataset level metadata.                                    */
/* -------------------------------------------------------------------- */
    poVRTDS->SetMetadata( poSrcDS->GetMetadata() );

/* -------------------------------------------------------------------- */
/*      GCPs                                                            */
/* -------------------------------------------------------------------- */
    if( poSrcDS->GetGCPCount() > 0 )
    {
        poVRTDS->SetGCPs( poSrcDS->GetGCPCount(), 
                          poSrcDS->GetGCPs(),
                          poSrcDS->GetGCPProjection() );
    }

/* -------------------------------------------------------------------- */
/*      Loop over all the bands.					*/
/* -------------------------------------------------------------------- */
    for( int iBand = 0; iBand < poSrcDS->GetRasterCount(); iBand++ )
    {
        GDALRasterBand *poSrcBand = poSrcDS->GetRasterBand( iBand+1 );

/* -------------------------------------------------------------------- */
/*      Create the band with the appropriate band type.                 */
/* -------------------------------------------------------------------- */
        poVRTDS->AddBand( poSrcBand->GetRasterDataType(), NULL );

        VRTSourcedRasterBand *poVRTBand = 
            (VRTSourcedRasterBand *) poVRTDS->GetRasterBand( iBand+1 );

/* -------------------------------------------------------------------- */
/*      Setup source mapping.                                           */
/* -------------------------------------------------------------------- */
        poVRTBand->AddSimpleSource( poSrcBand );

/* -------------------------------------------------------------------- */
/*      Emit various band level metadata.                               */
/* -------------------------------------------------------------------- */
        poVRTBand->CopyCommonInfoFrom( poSrcBand );
    }

    poVRTDS->FlushCache();

    return poVRTDS;
}

/************************************************************************/
/*                          GDALRegister_VRT()                          */
/************************************************************************/

void GDALRegister_VRT()

{
    VRTDriver	*poDriver;

    if( GDALGetDriverByName( "VRT" ) == NULL )
    {
        poDriver = new VRTDriver();
        
        poDriver->SetDescription( "VRT" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Virtual Raster" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "vrt" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, "gdal_vrttut.html" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES, 
                                   "Byte Int16 UInt16 Int32 UInt32 Float32 Float64 CInt16 CInt32 CFloat32 CFloat64" );
        
        poDriver->pfnOpen = VRTDataset::Open;
        poDriver->pfnCreateCopy = VRTCreateCopy;
        poDriver->pfnCreate = VRTDataset::Create;

        poDriver->AddSourceParser( "SimpleSource", 
                                   VRTParseCoreSources );
        poDriver->AddSourceParser( "ComplexSource", 
                                   VRTParseCoreSources );
        poDriver->AddSourceParser( "AveragedSource", 
                                   VRTParseCoreSources );

        poDriver->AddSourceParser( "KernelFilteredSource", 
                                   VRTParseFilterSources );

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

