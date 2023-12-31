/******************************************************************************
 * $Id: jpipkakdataset.cpp 2008-10-01 nbarker $
 *
 * Project:  jpip read driver
 * Purpose:  GDAL bindings for JPIP.  
 * Author:   Norman Barker, ITT VIS, norman.barker@gmail.com
 *
 ******************************************************************************
 * ITT Visual Information Systems grants you use of this code, under the 
 * following license:
 * 
 * Copyright (c) 2000-2007, ITT Visual Information Solutions 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions: 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software. 

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
**/

#include "jpipkakdataset.h"

/************************************************************************/
/* ==================================================================== */
/*                     Set up messaging services                        */
/* ==================================================================== */
/************************************************************************/

class kdu_cpl_error_message : public kdu_message 
{
public: // Member classes
    kdu_cpl_error_message( CPLErr eErrClass ) 
    {
        m_eErrClass = eErrClass;
        m_pszError = NULL;
    }

    void put_text(const char *string)
    {
        if( m_pszError == NULL )
            m_pszError = CPLStrdup( string );
        else
        {
            m_pszError = (char *) 
                CPLRealloc(m_pszError, strlen(m_pszError) + strlen(string)+1 );
            strcat( m_pszError, string );
        }
    }

    class JP2KAKException
    {
    };

    void flush(bool end_of_message=false) 
    {
        if( m_pszError == NULL )
            return;
        if( m_pszError[strlen(m_pszError)-1] == '\n' )
            m_pszError[strlen(m_pszError)-1] = '\0';

        CPLError( m_eErrClass, CPLE_AppDefined, "%s", m_pszError );
        CPLFree( m_pszError );
        m_pszError = NULL;

        if( end_of_message && m_eErrClass == CE_Failure )
        {
            throw new JP2KAKException();
        }
    }

private:
    CPLErr m_eErrClass;
    char *m_pszError;
};

/************************************************************************/
/* ==================================================================== */
/*                            JPIPKAKRasterBand                         */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                         JPIPKAKRasterBand()                          */
/************************************************************************/

JPIPKAKRasterBand::JPIPKAKRasterBand( int nBand, int nDiscardLevels,
                                      kdu_codestream *oCodeStream,
                                      int nResCount,
                                      JPIPKAKDataset *poBaseDSIn )

{
    this->nBand = nBand;
    poBaseDS = poBaseDSIn;

    eDataType = poBaseDSIn->eDT;

    this->nDiscardLevels = nDiscardLevels;
    this->oCodeStream = oCodeStream;

    oCodeStream->apply_input_restrictions( 0, 0, nDiscardLevels, 0, NULL );
    oCodeStream->get_dims( 0, band_dims );

    nRasterXSize = band_dims.size.x;
    nRasterYSize = band_dims.size.y;

/* -------------------------------------------------------------------- */
/*      Use a 2048x128 "virtual" block size unless the file is small.    */
/* -------------------------------------------------------------------- */
    if( nRasterXSize >= 2048 )
        nBlockXSize = 2048;
    else
        nBlockXSize = nRasterXSize;
    
    if( nRasterYSize >= 256 )
        nBlockYSize = 128;
    else
        nBlockYSize = nRasterYSize;

/* -------------------------------------------------------------------- */
/*      Figure out the color interpretation for this band.              */
/* -------------------------------------------------------------------- */
    
    eInterp = GCI_Undefined;
        
/* -------------------------------------------------------------------- */
/*      Do we have any overviews?  Only check if we are the full res    */
/*      image.                                                          */
/* -------------------------------------------------------------------- */
    nOverviewCount = 0;
    papoOverviewBand = 0;

    if( nDiscardLevels == 0 )
    {
        int  nXSize = nRasterXSize, nYSize = nRasterYSize;

        for( int nDiscard = 1; nDiscard < nResCount; nDiscard++ )
        {
            kdu_dims  dims;

            nXSize = (nXSize+1) / 2;
            nYSize = (nYSize+1) / 2;

            if( (nXSize+nYSize) < 128 || nXSize < 4 || nYSize < 4 )
                continue; /* skip super reduced resolution layers */

            oCodeStream->apply_input_restrictions( 0, 0, nDiscard, 0, NULL );
            oCodeStream->get_dims( 0, dims );

            if( (dims.size.x == nXSize || dims.size.x == nXSize-1)
                && (dims.size.y == nYSize || dims.size.y == nYSize-1) )
            {
                nOverviewCount++;
                papoOverviewBand = (JPIPKAKRasterBand **) 
                    CPLRealloc( papoOverviewBand, 
                                sizeof(void*) * nOverviewCount );
                papoOverviewBand[nOverviewCount-1] = 
                    new JPIPKAKRasterBand( nBand, nDiscard, oCodeStream, 0,
                                           poBaseDS );
            }
            else
            {
                CPLDebug( "GDAL", "Discard %dx%d JPEG2000 overview layer,\n"
                          "expected %dx%d.", 
                          dims.size.x, dims.size.y, nXSize, nYSize );
            }
        }
    }
}

/************************************************************************/
/*                         ~JPIPKAKRasterBand()                          */
/************************************************************************/

JPIPKAKRasterBand::~JPIPKAKRasterBand()

{
	//if (ario != NULL) poBaseDS->EndAsyncReader(ario);

    for( int i = 0; i < nOverviewCount; i++ )
        delete papoOverviewBand[i];

    CPLFree( papoOverviewBand );
}

/************************************************************************/
/*                          GetOverviewCount()                          */
/************************************************************************/

int JPIPKAKRasterBand::GetOverviewCount()

{
    return nOverviewCount;
}

/************************************************************************/
/*                            GetOverview()                             */
/************************************************************************/

GDALRasterBand *JPIPKAKRasterBand::GetOverview( int iOverviewIndex )

{
    if( iOverviewIndex < 0 || iOverviewIndex >= nOverviewCount )
        return NULL;
    else
        return papoOverviewBand[iOverviewIndex];
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr JPIPKAKRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                      void * pImage )
{
    CPLDebug( "JPIPKAK", "IReadBlock(%d,%d) on band %d.", 
              nBlockXOff, nBlockYOff, nBand );

    int xOff = nBlockXOff * nBlockXSize;
    int yOff = nBlockYOff * nBlockYSize;

    // TODO: Handle overviews properly.

    GDALAsyncReader* ario = poBaseDS->
        BeginAsyncReader(xOff, yOff, nBlockXSize, nBlockYSize, 
                           pImage, nBlockXSize, nBlockYSize, 
                           eDataType, 1, &nBand, 0, 0, 0, NULL);

    if( ario == NULL )
        return CE_Failure;

    int nXBufOff; // absolute x image offset
    int nYBufOff; // abolute y image offset
    int nXBufSize;
    int nYBufSize;

    GDALAsyncStatusType status;

    do 
    {
        status = ario->GetNextUpdatedRegion(-1.0, 
                                            &nXBufOff, &nYBufOff, 
                                            &nXBufSize, &nYBufSize );
    } while (status != GARIO_ERROR && status != GARIO_COMPLETE );

    poBaseDS->EndAsyncReader(ario);

    if (status == GARIO_ERROR)
        return CE_Failure;
    else
        return CE_None;
}

/************************************************************************/
/*                             IRasterIO()                              */
/************************************************************************/

CPLErr 
JPIPKAKRasterBand::IRasterIO( GDALRWFlag eRWFlag,
                              int nXOff, int nYOff, int nXSize, int nYSize,
                              void * pData, int nBufXSize, int nBufYSize,
                              GDALDataType eBufType, 
                              int nPixelSpace,int nLineSpace )
    
{
/* -------------------------------------------------------------------- */
/*      We need various criteria to skip out to block based methods.    */
/* -------------------------------------------------------------------- */
    if( poBaseDS->TestUseBlockIO( nXOff, nYOff, nXSize, nYSize, 
                                  nBufXSize, nBufYSize,
                                  eBufType, 1, &nBand ) )
        return GDALPamRasterBand::IRasterIO( 
            eRWFlag, nXOff, nYOff, nXSize, nYSize,
            pData, nBufXSize, nBufYSize, eBufType, 
            nPixelSpace, nLineSpace );

/* -------------------------------------------------------------------- */
/*      Otherwise do this as a single uncached async rasterio.          */
/* -------------------------------------------------------------------- */
    GDALAsyncReader* ario = 
        poBaseDS->BeginAsyncReader(nXOff, nYOff, nXSize, nYSize,
                                   pData, nBufXSize, nBufYSize, eBufType, 
                                   1, &nBand,
                                   nPixelSpace, nLineSpace, 0, NULL);
    
    if( ario == NULL )
        return CE_Failure;
    
    GDALAsyncStatusType status;

    do 
    {
        int nXBufOff,nYBufOff,nXBufSize,nYBufSize;

        status = ario->GetNextUpdatedRegion(-1.0, 
                                            &nXBufOff, &nYBufOff, 
                                            &nXBufSize, &nYBufSize );
    } while (status != GARIO_ERROR && status != GARIO_COMPLETE );

    poBaseDS->EndAsyncReader(ario);

    if (status == GARIO_ERROR)
        return CE_Failure;
    else
        return CE_None;
}

/*****************************************/
/*         JPIPKAKDataset()              */
/*****************************************/
JPIPKAKDataset::JPIPKAKDataset()         
{
    pszTid = NULL;
    pszPath = NULL;
    pszCid = NULL;
    pszProjection = NULL;
	
    poCache = NULL;
    poCodestream = NULL;
    poDecompressor = NULL;

    nPos = 0;
    nVBASLen = 0;
    nVBASFirstByte = 0;

    nClassId = 0;
    nCodestream = 0;
    nDatabins = 0;
    bWindowDone = FALSE;
    bGeoTransformValid = FALSE;

    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;

    nGCPCount = 0;
    pasGCPList = NULL;

    bHighThreadRunning = 0;
    bLowThreadRunning = 0;
    bHighThreadFinished = 0;
    bLowThreadFinished = 0;
    nHighThreadByteCount = 0;
    nLowThreadByteCount = 0;

    pGlobalMutex = CPLCreateMutex();
    CPLReleaseMutex(pGlobalMutex);
}

/*****************************************/
/*         ~JPIPKAKDataset()             */
/*****************************************/
JPIPKAKDataset::~JPIPKAKDataset()
{
    CPLHTTPCleanup();

    if (pszTid)
        CPLFree(pszTid);

    if (pszPath)
        CPLFree(pszPath);

    if (pszCid)
        CPLFree(pszCid);

    if (pszProjection)
        CPLFree(pszProjection);

    if (nGCPCount > 0 )
    {
        GDALDeinitGCPs( nGCPCount, pasGCPList );
        CPLFree( pasGCPList );
    }

    // frees decompressor as well
    if (poCodestream)
    {
        poCodestream->destroy();
        delete poCodestream;
    }
    delete poDecompressor;

    if (poCache)
        delete poCache;
}

/*****************************************/
/*         Initialise()                  */
/*****************************************/
int JPIPKAKDataset::Initialise(char* pszUrl)
{
    // set up message handlers
    kdu_cpl_error_message oErrHandler( CE_Failure );
    kdu_cpl_error_message oWarningHandler( CE_Warning );
    kdu_customize_warnings(new kdu_cpl_error_message( CE_Warning ) );
    kdu_customize_errors(new kdu_cpl_error_message( CE_Failure ) );

    // create necessary http headers
    CPLString osHeaders = "HEADERS=Accept: jpp-stream";
    CPLString osPersistent;

    osPersistent.Printf( "PERSISTENT=JPIPKAK:%p", this );

    char *apszOptions[] = { 
        (char *) osHeaders.c_str(),
        (char *) osPersistent.c_str(),
        NULL 
    };
        
    // make initial request to the server for a session, we are going to 
    // assume that the jpip communication is stateful, rather than one-shot
    // stateless requests append pszUrl with jpip request parameters for a 
    // stateful session (multi-shot communications)
    // "cnew=http&type=jpp-stream&stream=0&tid=0&len="
    CPLString osRequest;
    osRequest.Printf("%s?%s%i", pszUrl, 
                     "cnew=http&type=jpp-stream&stream=0&tid=0&len=", 2000);
	
    CPLHTTPResult *psResult = CPLHTTPFetch(osRequest, apszOptions);
	
    if ( psResult == NULL)
        return FALSE;

    if( psResult->nDataLen == 0 || psResult->pabyData == NULL  )
    {

        CPLError(CE_Failure, CPLE_AppDefined, 
                 "No data was returned from the given URL" );
        CPLHTTPDestroyResult( psResult );
        return FALSE;
    }

    if (psResult->nStatus != 0) 
    {
        CPLHTTPDestroyResult( psResult );
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Curl reports error: %d: %s", psResult->nStatus, psResult->pszErrBuf );
        return FALSE;        
    }

    // parse the response headers, and the initial data until we get to the 
    // codestream definition
    char** pszHdrs = psResult->papszHeaders;
    const char* pszTid = CSLFetchNameValue(pszHdrs, "JPIP-tid");
    const char* pszCnew = CSLFetchNameValue(pszHdrs, "JPIP-cnew");

    if( pszTid == NULL || pszCnew == NULL )
    {
        CPLHTTPDestroyResult( psResult );
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Unable to parse required cnew and tid response headers" );
        return FALSE;    
    }

    pszTid = CPLStrdup(pszTid);
    // parse cnew response
    // JPIP-cnew:
    // cid=DC69DF980A641A4BBDEB50E484A66578,path=MyPath,transport=http
    char **papszTokens = CSLTokenizeString2( pszCnew, ",", CSLT_HONOURSTRINGS);
    for (int i = 0; i < CSLCount(papszTokens); i++)
    {
        // looking for cid, path
        if (EQUALN(papszTokens[i], "cid", 3))
        {
            char *pszKey = NULL;
            const char *pszValue = CPLParseNameValue(papszTokens[i], &pszKey );
            pszCid = CPLStrdup(pszValue);
            CPLFree( pszKey );
        }

        if (EQUALN(papszTokens[i], "path", 4))
        {
            char *pszKey = NULL;
            const char *pszValue = CPLParseNameValue(papszTokens[i], &pszKey );
            pszPath = CPLStrdup(pszValue);
            CPLFree( pszKey );
        }
    }

    CSLDestroy(papszTokens);

    if( pszPath == NULL || pszCid == NULL )
    {
        CPLHTTPDestroyResult(psResult);
        CPLError(CE_Failure, CPLE_AppDefined, "Error parsing path and cid from cnew - %s", pszCnew);
        return FALSE;
    }

    // ok, good to go with jpip, get to the codestream before returning 
    // successful initialisation of the driver
    poCache = new kdu_cache();
    poCodestream = new kdu_codestream();
    poDecompressor = new kdu_region_decompressor();

    int bFinished = FALSE;
    bFinished = ReadFromInput(psResult->pabyData, psResult->nDataLen);
    CPLHTTPDestroyResult(psResult);

    // continue making requests in the main thread to get all the available 
    // metadata for data bin 0, and reach the codestream

    // format the new request
    // and set as pszRequestUrl;
    // get the protocol from the original request
    size_t found = osRequest.find_first_of("/");
    CPLString osProtocol = osRequest.substr(0, found + 2);
    osRequest.erase(0, found + 2);
    // find context path
    found = osRequest.find_first_of("/");
    osRequest.erase(found);
			
    osRequestUrl.Printf("%s%s/%s?cid=%s&stream=0&len=%i", osProtocol.c_str(), osRequest.c_str(), pszPath, pszCid, 2000);

    while (!bFinished)
    {
        CPLHTTPResult *psResult = CPLHTTPFetch(osRequestUrl, apszOptions);
        bFinished = ReadFromInput(psResult->pabyData, psResult->nDataLen);
        CPLHTTPDestroyResult(psResult);
    }

    // clean up osRequest, remove variable len= parameter
    size_t pos = osRequestUrl.find_last_of("&");	
    osRequestUrl.erase(pos);

    // create codestream
    poCache->set_read_scope(KDU_MAIN_HEADER_DATABIN, 0, 0);
    poCodestream->create(poCache);
    poCodestream->set_persistent();

    kdu_channel_mapping oChannels;
    oChannels.configure(*poCodestream);
    kdu_coords* ref_expansion = new kdu_coords(1, 1);
					
    // get available resolutions, image width / height etc.
    kdu_dims view_dims = poDecompressor->
        get_rendered_image_dims(*poCodestream, &oChannels, -1, 0,
                                *ref_expansion, *ref_expansion, 
                                KDU_WANT_OUTPUT_COMPONENTS);

    nRasterXSize = view_dims.size.x;
    nRasterYSize = view_dims.size.y;

    // The oChannels.num_channels better represents what we get from process.
    nBands = oChannels.num_channels;
    //nBands = poCodestream->get_num_components();

    // Establish the datatype - we will use the same datatype for
    // all bands based on the first.
    if( poCodestream->get_bit_depth(0) == 16
        && poCodestream->get_signed(0) )
        eDT = GDT_Int16;
    else if( poCodestream->get_bit_depth(0) == 16
        && !poCodestream->get_signed(0) )
        eDT = GDT_UInt16;
//    else if( poCodestream->get_bit_depth(0) == 32 )
//        eDT = GDT_Float32;
    else
        this->eDT = GDT_Byte;

    // TODO add color interpretation

    // calculate overviews
    siz_params* siz_in = poCodestream->access_siz();
    kdu_params* cod_in = siz_in->access_cluster("COD");

    delete ref_expansion;

    cod_in->get("Clayers", 0, 0, nQualityLayers);
    cod_in->get("Clevels", 0, 0, nResLevels);

    // setup band objects
    int iBand;
    
    for( iBand = 1; iBand <= nBands; iBand++ )
    {
        JPIPKAKRasterBand *poBand = 
            new JPIPKAKRasterBand(iBand,0,poCodestream,nResLevels,
                                  this );
	
        SetBand( iBand, poBand );
    }

    siz_in->get("Scomponents", 0, 0, nComps);
    siz_in->get("Sprecision", 0, 0, nBitDepth);

    // set specific metadata items
    CPLString osNQualityLayers;
    osNQualityLayers.Printf("%i", nQualityLayers);
    CPLString osNResolutionLevels;
    osNResolutionLevels.Printf("%i", nResLevels);
    CPLString osNComps;
    osNComps.Printf("%i", nComps);
    CPLString osBitDepth;
    osBitDepth.Printf("%i", nBitDepth);

    SetMetadataItem("JPIP_NQUALITYLAYERS", osNQualityLayers.c_str(), "JPIP");
    SetMetadataItem("JPIP_NRESOLUTIONLEVELS", osNResolutionLevels.c_str(), "JPIP");
    SetMetadataItem("JPIP_NCOMPS", osNComps.c_str(), "JPIP");
    SetMetadataItem("JPIP_SPRECISION", osBitDepth.c_str(), "JPIP");
	
/* ==================================================================== */
/*      Parse geojp2, or gmljp2, we will assume that the core           */
/*      metadata  of gml or a geojp2 uuid have been sent in the         */
/*      initial metadata response.                                      */
/*      If the server has used placeholder boxes for this               */
/*      information then the image will be interpreted as x,y           */
/* ==================================================================== */
    GDALJP2Metadata oJP2Geo;
    int nLen = poCache->get_databin_length(KDU_META_DATABIN, nCodestream, 0);

    if( nLen == 0 )
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Unable to open stream to parse metadata boxes" );
        return FALSE;
    }

    // create in memory file using vsimem
    CPLString osFileBoxName;
    osFileBoxName.Printf("/vsimem/jpip/%s.dat", pszCid);
    FILE *fpLL = VSIFOpenL(osFileBoxName.c_str(), "w+");
    poCache->set_read_scope(KDU_META_DATABIN, nCodestream, 0);
    kdu_byte* pabyBuffer = (kdu_byte *)CPLMalloc(nLen);
    poCache->read(pabyBuffer, nLen);
    VSIFWriteL(pabyBuffer, nLen, 1, fpLL);
    CPLFree( pabyBuffer );

    VSIFFlushL(fpLL);
    VSIFSeekL(fpLL, 0, SEEK_SET);

    nPamFlags |= GPF_NOSAVE;

    try
    {
        oJP2Geo.ReadBoxes(fpLL);
        // parse gml first, followed by geojp2 as a fallback
        if (oJP2Geo.ParseGMLCoverageDesc() || oJP2Geo.ParseJP2GeoTIFF())
        {
            pszProjection = CPLStrdup(oJP2Geo.pszProjection);
            bGeoTransformValid = TRUE;

            memcpy(adfGeoTransform, oJP2Geo.adfGeoTransform, 
                   sizeof(double) * 6 );
            nGCPCount = oJP2Geo.nGCPCount;
            pasGCPList = oJP2Geo.pasGCPList;

            oJP2Geo.pasGCPList = NULL;
            oJP2Geo.nGCPCount = 0;
						
            int iBox;

            for( iBox = 0; 
                 oJP2Geo.papszGMLMetadata
                     && oJP2Geo.papszGMLMetadata[iBox] != NULL; 
                 iBox++ )
            {
                char *pszName = NULL;
                const char *pszXML = 
                    CPLParseNameValue( oJP2Geo.papszGMLMetadata[iBox], 
                                       &pszName );
                CPLString osDomain;
                char *apszMDList[2];

                osDomain.Printf( "xml:%s", pszName );
                apszMDList[0] = (char *) pszXML;
                apszMDList[1] = NULL;

                GDALPamDataset::SetMetadata( apszMDList, osDomain );
                CPLFree( pszName );
            }
				

            SetDescription( CPLStrdup(pszUrl));
        }
        else
        {
            // treat as cartesian, no geo metadata
            CPLError(CE_Warning, CPLE_AppDefined, 
                     "Parsed metadata boxes from jpip stream, geographic metadata not found - is the server using placeholders for this data?" );
						
        }
    }
    catch(...)
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Unable to parse geographic metadata boxes from jpip stream" );
    }

    VSIFCloseL(fpLL);
    VSIUnlink( osFileBoxName.c_str());

    return TRUE;
}

/******************************************/
/*           ReadVBAS()                   */
/******************************************/
long JPIPKAKDataset::ReadVBAS(GByte* pabyData, int nLen)
{
    int c = -1;
    long val = 0;
    nVBASLen = 0;

    while ((c & 0x80) != 0)
    {
        if (nVBASLen >= 9)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "VBAS Length not supported");
            break;
        }

        if (nPos > nLen)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "EOF reached before completing VBAS");
            break;
        }

        c = pabyData[nPos];
        nPos++;

        val = (val << 7) | (long)(c & 0x7F);

        if (nVBASLen == 0)
            nVBASFirstByte = c;

        nVBASLen++;
    }

    return val;
}

/******************************************/
/*            ReadSegment()               */
/******************************************/
JPIPDataSegment* JPIPKAKDataset::ReadSegment(GByte* pabyData, int nLen)
{
    long nId = ReadVBAS(pabyData, nLen);
    if (nId < 0)
        return NULL;
    else
    {
        JPIPDataSegment* segment = new JPIPDataSegment();
        segment->SetId(nId);

        if (nVBASFirstByte == 0)
        {
            segment->SetEOR(TRUE);
            segment->SetId(pabyData[nPos]);
        }
        else
        {
            segment->SetEOR(FALSE);
            nId &= ~(0x70 << ((nVBASLen -1) * 7));
            segment->SetId(nId);
            segment->SetFinal((nVBASFirstByte & 0x10) != 0);

            int m = (nVBASFirstByte & 0x7F) >> 5;

            if (m == 0)
            {
                CPLError(CE_Failure, CPLE_AppDefined,
                         "Invalid Bin-ID value format");
            }
            else if (m >= 2) {
                nClassId = ReadVBAS(pabyData, nLen);
                if (m > 2)
                    nCodestream = ReadVBAS(pabyData, nLen);
            }

            segment->SetClassId(nClassId);
            segment->SetCodestreamIdx(nCodestream);
            segment->SetOffset(ReadVBAS(pabyData, nLen));
            segment->SetLen(ReadVBAS(pabyData, nLen));
        }

        if ((segment->GetLen() > 0) && (!segment->IsEOR()))
        {
            GByte* pabyDataSegment = (GByte *) CPLCalloc(1,segment->GetLen());
			
            // copy data from input array pabyData to the data segment
            memcpy(pabyDataSegment, 
                   pabyData + nPos,
                   segment->GetLen());

            segment->SetData(pabyDataSegment);
        }

        nPos += segment->GetLen();

        if (!segment->IsEOR())
            nDatabins++;

        if ((segment->GetId() == JPIPKAKDataset::JPIP_EOR_WINDOW_DONE) && (segment->IsEOR()))
            bWindowDone = TRUE;

        return segment;
    }
}

/******************************************/
/*           KakaduClassId()              */
/******************************************/
int JPIPKAKDataset::KakaduClassId(int nClassId)
{
	if (nClassId == 0)
        return KDU_PRECINCT_DATABIN;
    else if (nClassId == 2)
        return KDU_TILE_HEADER_DATABIN;
	else if (nClassId == 6)
        return KDU_MAIN_HEADER_DATABIN;
    else if (nClassId == 8)  
        return KDU_META_DATABIN;
    else if (nClassId == 4)
        return KDU_TILE_DATABIN;
	else
		return -1;
}

/******************************************/
/*            ReadFromInput()             */
/******************************************/
int JPIPKAKDataset::ReadFromInput(GByte* pabyData, int nLen)
{
    int res = FALSE;
    if (nLen > 0)
    {
        // parse the data stream, reading the vbas and adding to the kakadu cache
        // we could parse all the boxes by hand, and just add data to the kakadu cache
        // we will do it the easy way and retrieve the metadata through the kakadu query api
        nPos = 0;
        JPIPDataSegment* pSegment = NULL;
        while ((pSegment = ReadSegment(pabyData, nLen)) != NULL)
        {
			
            if (pSegment->IsEOR())
            {		
                if ((pSegment->GetId() == JPIPKAKDataset::JPIP_EOR_IMAGE_DONE) || 
                    (pSegment->GetId() == JPIPKAKDataset::JPIP_EOR_WINDOW_DONE))
                    res = TRUE;
				
                delete pSegment;
                break;
            }
            else
            {
                // add data to kakadu
                //CPLDebug("JPIPKAK", "Parsed JPIP Segment class=%i stream=%i id=%i offset=%i len=%i isFinal=%i isEOR=%i", pSegment->GetClassId(), pSegment->GetCodestreamIdx(), pSegment->GetId(), pSegment->GetOffset(), pSegment->GetLen(), pSegment->IsFinal(), pSegment->IsEOR());
                poCache->add_to_databin(KakaduClassId(pSegment->GetClassId()), pSegment->GetCodestreamIdx(),
                                             pSegment->GetId(), pSegment->GetData(), pSegment->GetOffset(), pSegment->GetLen(), pSegment->IsFinal());

                delete pSegment;
            }

        }
    }
    return res;
}


/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *JPIPKAKDataset::GetProjectionRef()

{
    if( pszProjection && *pszProjection )
        return( pszProjection );
    else
        return GDALPamDataset::GetProjectionRef();
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr JPIPKAKDataset::GetGeoTransform( double * padfTransform )

{
    if( bGeoTransformValid )
    {
        memcpy( padfTransform, adfGeoTransform, sizeof(double)*6 );
    
        return CE_None;
    }
    else
        return GDALPamDataset::GetGeoTransform( padfTransform );
}

/************************************************************************/
/*                            GetGCPCount()                             */
/************************************************************************/

int JPIPKAKDataset::GetGCPCount()

{
    return nGCPCount;
}

/************************************************************************/
/*                          GetGCPProjection()                          */
/************************************************************************/

const char *JPIPKAKDataset::GetGCPProjection()

{
    if( nGCPCount > 0 )
        return pszProjection;
    else
        return "";
}

/************************************************************************/
/*                               GetGCP()                               */
/************************************************************************/

const GDAL_GCP *JPIPKAKDataset::GetGCPs()

{
    return pasGCPList;
}

/************************************************************************/
/*                             IRasterIO()                              */
/************************************************************************/

CPLErr JPIPKAKDataset::IRasterIO( GDALRWFlag eRWFlag,
                                  int nXOff, int nYOff, int nXSize, int nYSize,
                                  void * pData, int nBufXSize, int nBufYSize,
                                  GDALDataType eBufType, 
                                  int nBandCount, int *panBandMap,
                                  int nPixelSpace,int nLineSpace,int nBandSpace)

{
/* -------------------------------------------------------------------- */
/*      We need various criteria to skip out to block based methods.    */
/* -------------------------------------------------------------------- */
    if( TestUseBlockIO( nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize,
                        eBufType, nBandCount, panBandMap ) )
        return GDALPamDataset::IRasterIO( 
            eRWFlag, nXOff, nYOff, nXSize, nYSize,
            pData, nBufXSize, nBufYSize, eBufType, 
            nBandCount, panBandMap, nPixelSpace, nLineSpace, nBandSpace );

/* -------------------------------------------------------------------- */
/*      Otherwise do this as a single uncached async rasterio.          */
/* -------------------------------------------------------------------- */
    GDALAsyncReader* ario = 
        BeginAsyncReader(nXOff, nYOff, nXSize, nYSize,
                         pData, nBufXSize, nBufYSize, eBufType, 
                         nBandCount, panBandMap, 
                         nPixelSpace, nLineSpace, nBandSpace, NULL);

    if( ario == NULL )
        return CE_Failure;
    
    GDALAsyncStatusType status;

    do 
    {
        int nXBufOff,nYBufOff,nXBufSize,nYBufSize;

        status = ario->GetNextUpdatedRegion(-1.0, 
                                            &nXBufOff, &nYBufOff, 
                                            &nXBufSize, &nYBufSize );
    } while (status != GARIO_ERROR && status != GARIO_COMPLETE );

    EndAsyncReader(ario);

    if (status == GARIO_ERROR)
        return CE_Failure;
    else
        return CE_None;
}

/************************************************************************/
/*                           TestUseBlockIO()                           */
/************************************************************************/

int 
JPIPKAKDataset::TestUseBlockIO( int nXOff, int nYOff, int nXSize, int nYSize,
                                int nBufXSize, int nBufYSize,
                                GDALDataType eDataType, 
                                int nBandCount, int *panBandList )

{
/* -------------------------------------------------------------------- */
/*      Due to limitations in DirectRasterIO() we can only handle       */
/*      it no duplicates in the band list.                              */
/* -------------------------------------------------------------------- */
    int i, j; 
    
    for( i = 0; i < nBandCount; i++ )
    {
        for( j = i+1; j < nBandCount; j++ )
            if( panBandList[j] == panBandList[i] )
                return TRUE;
    }

/* -------------------------------------------------------------------- */
/*      The rest of the rules are io strategy stuff, and use            */
/*      configuration checks.                                           */
/* -------------------------------------------------------------------- */
    int bUseBlockedIO = bForceCachedIO;

    if( nYSize == 1 || nXSize * ((double) nYSize) < 100.0 )
        bUseBlockedIO = TRUE;

    if( nBufYSize == 1 || nBufXSize * ((double) nBufYSize) < 100.0 )
        bUseBlockedIO = TRUE;

    if( bUseBlockedIO
        && CSLTestBoolean( CPLGetConfigOption( "GDAL_ONE_BIG_READ", "NO") ) )
        bUseBlockedIO = FALSE;

    return bUseBlockedIO;
}

/*************************************************************************/
/*                     BeginAsyncReader()                              */
/*************************************************************************/

GDALAsyncReader* 
JPIPKAKDataset::BeginAsyncReader(int xOff, int yOff,
                                   int xSize, int ySize, 
                                   void *pBuf,
                                   int bufXSize, int bufYSize,
                                   GDALDataType bufType,
                                   int nBandCount, int* pBandMap,
                                   int nPixelSpace, int nLineSpace,
                                   int nBandSpace,
                                   char **papszOptions)
{
    CPLDebug( "JPIP", "BeginAsyncReadeR(%d,%d,%d,%d -> %dx%d)",
              xOff, yOff, xSize, ySize, bufXSize, bufYSize );

/* -------------------------------------------------------------------- */
/*      Provide default packing if needed.                              */
/* -------------------------------------------------------------------- */
    if( nPixelSpace == 0 )
        nPixelSpace = GDALGetDataTypeSize(bufType) / 8;
    if( nLineSpace == 0 )
        nLineSpace = nPixelSpace * bufXSize;
    if( nBandSpace == 0 )
        nBandSpace = nLineSpace * bufYSize;
    
/* -------------------------------------------------------------------- */
/*      Record request information.                                     */
/* -------------------------------------------------------------------- */
    JPIPKAKAsyncReader* ario = new JPIPKAKAsyncReader();
    ario->poDS = this;
    ario->nBufXSize = bufXSize;
    ario->nBufYSize = bufYSize;
    ario->eBufType = bufType;
    ario->nBandCount = nBandCount;

    ario->panBandMap = new int[nBandCount];
    if (pBandMap)
    {
        for (int i = 0; i < nBandCount; i++)
            ario->panBandMap[i] = pBandMap[i];
    }
    else
    {
        for (int i = 0; i < nBandCount; i++)
            ario->panBandMap[i] = i+1;
    }

/* -------------------------------------------------------------------- */
/*      If the buffer type is of other than images type, we need to     */
/*      allocate a private buffer the same type as the image which      */
/*      will be converted later.                                        */
/* -------------------------------------------------------------------- */
    if( bufType != eDT )
    {
        ario->nPixelSpace = GDALGetDataTypeSize(eDT) / 8;
        ario->nLineSpace = ario->nPixelSpace * bufXSize;
        ario->nBandSpace = ario->nLineSpace * bufYSize;

        ario->nAppPixelSpace = nPixelSpace;
        ario->nAppLineSpace = nLineSpace;
        ario->nAppBandSpace = nBandSpace;

        ario->pBuf = VSIMalloc3(bufXSize,bufYSize,ario->nPixelSpace*nBandCount);
        if( ario->pBuf == NULL )
        {
            delete ario;
            CPLError( CE_Failure, CPLE_OutOfMemory,
                      "Failed to allocate %d byte work buffer.",
                      bufXSize * bufYSize * ario->nPixelSpace );
            return NULL;
        }

        ario->pAppBuf = pBuf;
    }
    else
    {
        ario->pBuf = pBuf;
        ario->pAppBuf = pBuf;

        ario->nAppPixelSpace = ario->nPixelSpace = nPixelSpace;
        ario->nAppLineSpace = ario->nLineSpace = nLineSpace;
        ario->nAppBandSpace = ario->nBandSpace = nBandSpace;
    }

/* -------------------------------------------------------------------- */
/*      check we have sensible values for windowing.                    */
/* -------------------------------------------------------------------- */
    if (xOff > GetRasterXSize())
        xOff = GetRasterXSize();

    if (yOff > GetRasterYSize())
        yOff = GetRasterYSize();

    if ((xOff + xSize) > GetRasterXSize())
        xSize = GetRasterXSize() - xOff;

    if ((yOff + ySize) > GetRasterYSize())
        ySize = GetRasterYSize() - yOff;

    ario->nXOff = xOff;
    ario->nYOff = yOff;
    ario->nXSize = xSize;
    ario->nYSize = ySize;

    // parse options
    const char* pszLevel = CSLFetchNameValue(papszOptions, "LEVEL");
    const char* pszLayers = CSLFetchNameValue(papszOptions, "LAYERS");
    const char* pszPriority = CSLFetchNameValue(papszOptions, "PRIORITY");
	
    if (pszLayers)
        ario->nQualityLayers = atoi(pszLayers);
    else
        ario->nQualityLayers = nQualityLayers;

    if (pszPriority)
    {
        if (EQUAL(pszPriority, "0"))
            ario->bHighPriority = 0;
        else
            ario->bHighPriority = 1;
    }
    else
        ario->bHighPriority = 1;

/* -------------------------------------------------------------------- */
/*      Select an appropriate level based on the ratio of buffer        */
/*      size to full resolution image.  We aim for the next             */
/*      resolution *lower* than we might expect for the target          */
/*      buffer unless it falls on a power of two.  This is because      */
/*      the region decompressor only seems to support upsampling        */
/*      via the numerator/denominator magic.                            */
/* -------------------------------------------------------------------- */
    if (pszLevel)
        ario->nLevel = atoi(pszLevel);
    else
    {
        int nRXSize = xSize, nRYSize = ySize;
        ario->nLevel = 0;

        while( ario->nLevel < nResLevels
               && (nRXSize > bufXSize || nRYSize > bufYSize) )
        {
            nRXSize = (nRXSize+1) / 2;
            nRYSize = (nRYSize+1) / 2;
            ario->nLevel++;
        }
    }

    ario->Start();

    return ario;
}

/************************************************************************/
/*                  EndAsyncReader()                                  */
/************************************************************************/

void JPIPKAKDataset::EndAsyncReader(GDALAsyncReader *poARIO)
{
    delete poARIO;
}


/*****************************************/
/*             Open()                    */
/*****************************************/
GDALDataset *JPIPKAKDataset::Open(GDALOpenInfo * poOpenInfo)
{
    // test jpip and jpips, assuming jpip is using http as the transport layer
    // jpip = http, jpips = https (note SSL is allowed, but jpips is not in the ISO spec)
    if (EQUALN(poOpenInfo->pszFilename,"jpip://", 7)
        || EQUALN(poOpenInfo->pszFilename,"jpips://",8))
    {
        // swap the protocol to http / https as jpip in this implementations
        // is using http
        char* request = CPLStrdup(poOpenInfo->pszFilename);
        CPLPrintString(request, "http", 4);
        //CPLDebug("JPIPKAK", "Connecting to %s", poOpenInfo->pszFilename);

        // perform the initial connection
        // using cpl_http for the connection
        if  (CPLHTTPEnabled() == TRUE)
        {
            JPIPKAKDataset *poDS;
            poDS = new JPIPKAKDataset();
            if (poDS->Initialise(request))
            {
                CPLFree(request);			
                return poDS;
            }
            else
            {
                CPLFree(request);	
                delete poDS;
                return NULL;
            }
        }
        else
        {
            CPLError( CE_Failure, CPLE_OpenFailed,
                      "Failed to open %s within JPIPKAK driver CPL HTTP not enabled.\n",
                      poOpenInfo->pszFilename );
            CPLFree(request);
            return NULL;
        }
    }
    else
        return NULL;
}

/************************************************************************/
/*                        GDALRegister_JPIPKAK()			*/
/************************************************************************/

void GDALRegister_JPIPKAK()
{
    GDALDriver *poDriver;
	
    if (! GDAL_CHECK_VERSION("JPIPKAK driver"))
        return;

    if( GDALGetDriverByName( "JPIPKAK" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "JPIPKAK" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "JPIP (based on Kakadu)" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_jpipkak.html" );
        poDriver->SetMetadataItem( GDAL_DMD_MIMETYPE, "image/jpp-stream" );

        poDriver->pfnOpen = JPIPKAKDataset::Open;
        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

/************************************************************************/
/*                         JPIPKAKAsyncReader                         */
/************************************************************************/
JPIPKAKAsyncReader::JPIPKAKAsyncReader()
{
    panBandMap = NULL;
    pAppBuf = pBuf = NULL;
    nDataRead = 0;
}

/************************************************************************/
/*                        ~JPIPKAKAsyncReader                         */
/************************************************************************/
JPIPKAKAsyncReader::~JPIPKAKAsyncReader()
{
    Stop();

    // don't own the buffer
    delete [] panBandMap;

    if( pAppBuf != pBuf )
        CPLFree( pBuf );
}

/************************************************************************/
/*                               Start()                                */
/************************************************************************/

void JPIPKAKAsyncReader::Start()
{
    JPIPKAKDataset *poJDS = (JPIPKAKDataset*)poDS;

    // stop the currently running thread
    // start making requests to the server to the server
    nDataRead = 0;
    bComplete = 0;

    // check a thread is not already running
    if (((bHighPriority) && (poJDS->bHighThreadRunning)) || 
        ((!bHighPriority) && (poJDS->bLowThreadRunning)))
        CPLError(CE_Failure, CPLE_AppDefined, "JPIPKAKAsyncReader supports at most two concurrent server communication threads");
    else
    {
        // calculate the url the worker function is going to retrieve
        // calculate the kakadu adjust image size
        channels.configure(*(((JPIPKAKDataset*)poDS)->poCodestream));

        // find current canvas width and height in the cache and check we don't
        // exceed this in our process request
        kdu_dims view_dims;	
        kdu_coords ref_expansion;
        ref_expansion.x = 1;
        ref_expansion.y = 1;

        view_dims = ((JPIPKAKDataset*)poDS)->poDecompressor->
            get_rendered_image_dims(*((JPIPKAKDataset*)poDS)->poCodestream, &channels, 
                                    -1, nLevel, 
                                    ref_expansion );

        kdu_coords* view_siz = view_dims.access_size();
		
        // Establish the decimation implied by our resolution level.
        int nRes = 1;
        if (nLevel > 0)
            nRes = 2 << (nLevel - 1);

        // setup expansion to account for the difference between 
        // the selected level and the buffer resolution.
        exp_numerator.x = nBufXSize;
        exp_numerator.y = nBufYSize;

        exp_denominator.x = (int) ceil(nXSize / (double) nRes);
        exp_denominator.y = (int) ceil(nYSize / (double) nRes);

        // formulate jpip parameters and adjust offsets for current level
        int fx = view_siz->x;
        int fy = view_siz->y;

        rr_win.pos.x = (int) ceil(nXOff / (1.0 * nRes)); // roffx
        rr_win.pos.y = (int) ceil(nYOff / (1.0 * nRes)); // roffy
        rr_win.size.x = (int) ceil(nXSize / (1.0 * nRes)); // rsizx
        rr_win.size.y = (int) ceil(nYSize / (1.0 * nRes)); // rsizy

        if ( rr_win.pos.x + rr_win.size.x > fx)
            rr_win.size.x = fx - rr_win.pos.x;
        if ( rr_win.pos.y + rr_win.size.y > fy)
            rr_win.size.y = fy - rr_win.pos.y;

        CPLString jpipUrl;
        CPLString comps;

        for (int i = 0; i < nBandCount; i++)
            comps.Printf("%s%i,", comps.c_str(), panBandMap[i]-1);

        comps.erase(comps.length() -1);
	
        jpipUrl.Printf("%s&type=jpp-stream&roff=%i,%i&rsiz=%i,%i&fsiz=%i,%i,closest&quality=%i&comps=%s", 
                       ((JPIPKAKDataset*)poDS)->osRequestUrl.c_str(),
                       rr_win.pos.x, rr_win.pos.y,
                       rr_win.size.x, rr_win.size.y,
                       fx, fy,
                       nQualityLayers, comps.c_str());

        JPIPRequest* pRequest = new JPIPRequest();
        pRequest->bPriority = bHighPriority;
        pRequest->osRequest = jpipUrl;
        pRequest->poARIO = this;

        //CPLDebug("JPIPKAKAsyncReader", "THREADING TURNED OFF");
        if (CPLCreateThread(JPIPWorkerFunc, pRequest) == -1)
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Unable to create worker jpip  thread" );
        // run in main thread as a test
        //JPIPWorkerFunc(pRequest);
    }
}

/************************************************************************/
/*                                Stop()                                */
/************************************************************************/
void JPIPKAKAsyncReader::Stop()
{
    JPIPKAKDataset *poJDS = (JPIPKAKDataset*)poDS;

    bComplete = 1;
    if (poJDS->pGlobalMutex)
    {
        if (((bHighPriority) && (!poJDS->bHighThreadFinished)) || 
            ((!bHighPriority) && (!poJDS->bLowThreadFinished)))
        {
            // stop the thread
            if (bHighPriority)
            {
                CPLAcquireMutex(poJDS->pGlobalMutex, 100.0);
                poJDS->bHighThreadRunning = 0;
                CPLReleaseMutex(poJDS->pGlobalMutex);
		
                while (!poJDS->bHighThreadFinished)
                    CPLSleep(0.1);
            }
            else
            {
                CPLAcquireMutex(poJDS->pGlobalMutex, 100.0);
                poJDS->bLowThreadRunning = 0;
                CPLReleaseMutex(poJDS->pGlobalMutex);
		
                while (!poJDS->bLowThreadFinished)
                {
                    CPLSleep(0.1);
                }
            }
        }
    }
}

/************************************************************************/
/*                        GetNextUpdatedRegion()                        */
/************************************************************************/

GDALAsyncStatusType 
JPIPKAKAsyncReader::GetNextUpdatedRegion(double dfTimeout,
                                         int* pnxbufoff,
                                         int* pnybufoff,
                                         int* pnxbufsize,
                                         int* pnybufsize)
{
    GDALAsyncStatusType result = GARIO_ERROR;
    JPIPKAKDataset *poJDS = (JPIPKAKDataset*)poDS;

    long nSize = 0;
    // take a snapshot of the volatile variables
    if (bHighPriority)
    {
        const long s = poJDS->nHighThreadByteCount - nDataRead;
        nSize = s;
    }
    else
    {
        const long s = poJDS->nLowThreadByteCount - nDataRead;
        nSize = s;
    }

    // wait for new data if required
    if ((nSize == 0) && dfTimeout != 0 )
    {
        // poll for change in cache size
        clock_t end_wait = 0;

        end_wait = clock() + (int) (dfTimeout * CLOCKS_PER_SEC); 
		
        while ((nSize == 0) && ((bHighPriority && poJDS->bHighThreadRunning) ||
                                (!bHighPriority && poJDS->bLowThreadRunning)))
        {
            if (end_wait)
                if (clock() > end_wait && dfTimeout >= 0 )
                    break;

            CPLSleep(0.1);

            if (bHighPriority)
            {
                const long s = poJDS->nHighThreadByteCount - nDataRead;
                nSize = s;
            }
            else
            {
                const long s = poJDS->nLowThreadByteCount - nDataRead;
                nSize = s;
            }
        }
    }

    // if there is no pending data and we don't want to wait.
    if( nSize == 0 )
    {
        *pnxbufoff = 0;
        *pnybufoff = 0;
        *pnxbufsize = 0;
        *pnybufsize = 0;		
        return GARIO_PENDING;
    }

    // Establish the canvas region with the expansion factor applied,
    // and compute region from the original window cut down to the
    // target canvas.
    kdu_dims view_dims;

    view_dims = poJDS->poDecompressor->get_rendered_image_dims(
        *poJDS->poCodestream, &channels, 
        -1, nLevel, exp_numerator, exp_denominator );

    double x_ratio, y_ratio;
    x_ratio = view_dims.size.x / (double) poDS->GetRasterXSize();
    y_ratio = view_dims.size.y / (double) poDS->GetRasterYSize();

    kdu_dims region = rr_win;

    region.pos.x = (int) ceil(nXOff * x_ratio);
    region.pos.y = (int) ceil(nYOff * y_ratio);
    region.size.x = (int) ceil(nXSize * x_ratio);
    region.size.y = (int) ceil(nYSize * y_ratio);

    region.size.x = MIN(region.size.x,nBufXSize);
    region.size.y = MIN(region.size.y,nBufYSize);
        
    if( region.pos.x + region.size.x > view_dims.size.x )
        region.size.x = view_dims.size.x - region.pos.x;
    if( region.pos.y + region.size.y > view_dims.size.y )
        region.size.y = view_dims.size.y - region.pos.y;
        
    region.pos += view_dims.pos;

    // Set up channel list requested.  
    std::vector<int> component_indices;
    int i;

    for( i=0; i < nBandCount; i++ )
        component_indices.push_back( panBandMap[i]-1 );
        
    // Apply region and other restrictions.
    poJDS->poCodestream->apply_input_restrictions(
        nBandCount, &(component_indices[0]), nLevel, nQualityLayers, 
        &region, KDU_WANT_CODESTREAM_COMPONENTS);

    channels.configure(*(poJDS->poCodestream));

    if( channels.num_channels < nBandCount )
        CPLDebug( "JPIP", "channels.num_channels = %d, but nBandCount=%d!",
                  channels.num_channels, nBandCount );

    for( i=0; i < MIN(nBandCount,channels.num_channels); i++ )
        channels.source_components[i] = panBandMap[i]-1;

    kdu_dims incomplete_region = region;
    kdu_coords origin = region.pos;

    int bIsDecompressing = FALSE;
		
    CPLAcquireMutex(poJDS->pGlobalMutex, 100.0);

    bIsDecompressing = poJDS->poDecompressor->start(
        *(poJDS->poCodestream), 
        &channels, -1, nLevel, nQualityLayers, 
        region, exp_numerator, exp_denominator, TRUE);	
		
    *pnxbufoff = 0;
    *pnybufoff = 0;
    *pnxbufsize = region.access_size()->x;
    *pnybufsize = region.access_size()->y;

    int nPrecisionBits = 8;

    switch(poJDS->eDT)
    {
      case GDT_Byte:
        nPrecisionBits = 8;
        break;
      case GDT_UInt16:
      case GDT_Int16:
        nPrecisionBits = 16;
        break;
      default:
        nPrecisionBits = 8;
        break;
    }
        
    // Setup channel buffers
    std::vector<kdu_byte*> channel_bufs;

    for( i=0; i < nBandCount; i++ )
        channel_bufs.push_back( ((kdu_byte *) pBuf) + i * nBandSpace );

    int pixel_gap = nPixelSpace / (nPrecisionBits / 8);
    int row_gap = nLineSpace / (nPrecisionBits / 8);

    while ((bIsDecompressing == 1) || (incomplete_region.area() != 0))
    {
        if( nPrecisionBits == 8 )
        {
            bIsDecompressing = poJDS->poDecompressor->
                process(&(channel_bufs[0]), false, 
                        pixel_gap, origin, row_gap, 1000000, 0, 
                        incomplete_region, region,
                        8, false );
        }
        else if( nPrecisionBits == 16 )
        {
            bIsDecompressing = poJDS->poDecompressor->
                process((kdu_uint16**) &(channel_bufs[0]), false,
                        pixel_gap, origin, row_gap, 1000000, 0, 
                        incomplete_region, region,
                        16, false );
        }

        CPLDebug( "JPIPKAK",
                  "processed=%d,%d %dx%d   - incomplete=%d,%d %dx%d",
                  region.pos.x, region.pos.y, 
                  region.size.x, region.size.y,
                  incomplete_region.pos.x, incomplete_region.pos.y,
                  incomplete_region.size.x, incomplete_region.size.y );
    }

    poJDS->poDecompressor->finish();
    CPLReleaseMutex(poJDS->pGlobalMutex);

    // has there been any more data read whilst we have been processing?
    long size = 0;
    if (bHighPriority)
    {
        const long x = poJDS->nHighThreadByteCount - nDataRead;
        size = x;
    }
    else
    {
        const long x = poJDS->nLowThreadByteCount - nDataRead;
        size = x;
    }

    if ((bComplete) && (nSize == size))
        result = GARIO_COMPLETE;
    else
        result = GARIO_UPDATE;

    nDataRead += nSize;

/* -------------------------------------------------------------------- */
/*      If the application buffer is of a different type than our       */
/*      band we need to copy into the application buffer at this        */
/*      point.                                                          */
/*                                                                      */
/*      We could optimize to only update affected area, but that is     */
/*      always the whole area for the JPIP driver it seems.             */
/* -------------------------------------------------------------------- */
    if( pAppBuf != pBuf )
    {
        int iY, iBand;
        GByte *pabySrc = (GByte *) pBuf;
        GByte *pabyDst = (GByte *) pAppBuf;

        for( iBand = 0; iBand < nBandCount; iBand++ )
        {
            for( iY = 0; iY < nBufYSize; iY++ )
            {
                GDALCopyWords( pabySrc + nLineSpace * iY + nBandSpace * iBand,
                               poJDS->eDT, nPixelSpace,
                               pabyDst + nAppLineSpace*iY + nAppBandSpace*iBand,
                               eBufType, nAppPixelSpace, 
                               nBufXSize );
            }
        }
    }

    return result;	
}

/************************************************************************/
/*                          JPIPDataSegment()                           */
/************************************************************************/
JPIPDataSegment::JPIPDataSegment()
{
    nId = 0;
    nAux = 0;
    nClassId = 0;
    nCodestream = 0;
    nOffset = 0;
    nLen = 0;
    pabyData = NULL;
    bIsFinal = FALSE;
    bIsEOR = FALSE;
}

/************************************************************************/
/*                          ~JPIPDataSegment()                          */
/************************************************************************/
JPIPDataSegment::~JPIPDataSegment()
{
    CPLFree(pabyData);
}

/************************************************************************/
/*                           JPIPWorkerFunc()                           */
/************************************************************************/
static void JPIPWorkerFunc(void *req)
{
    int nCurrentTransmissionLength = 2000;
    int nMinimumTransmissionLength = 2000;

    JPIPRequest *pRequest = (JPIPRequest *)req;
    JPIPKAKDataset *poJDS = 
        (JPIPKAKDataset*)(pRequest->poARIO->GetGDALDataset());
	
    int bPriority = pRequest->bPriority;

    CPLAcquireMutex(poJDS->pGlobalMutex, 100.0);

    // set the running status
    if (bPriority)
    {
        poJDS->bHighThreadRunning = 1;
        poJDS->bHighThreadFinished = 0;
    }
    else
    {
        poJDS->bLowThreadRunning = 1;
        poJDS->bLowThreadFinished = 0;
    }

    CPLReleaseMutex(poJDS->pGlobalMutex);

    CPLString osHeaders = "HEADERS=";
    osHeaders += "Accept: jpp-stream";
    CPLString osPersistent;

    osPersistent.Printf( "PERSISTENT=JPIPKAK:%p", poJDS );

    char *apszOptions[] = { 
        (char *) osHeaders.c_str(),
        (char *) osPersistent.c_str(),
        NULL 
    };

    while (TRUE)
    {
        // modulate the len= parameter to use the currently available bandwidth
        long nStart = clock();

        if (((bPriority) && (!poJDS->bHighThreadRunning)) || ((!bPriority) && (!poJDS->bLowThreadRunning)))
            break;

        // make jpip requests
        // adjust transmission length
        CPLString osCurrentRequest;
        osCurrentRequest.Printf("%s&len=%i", pRequest->osRequest.c_str(), nCurrentTransmissionLength);
        CPLHTTPResult *psResult = CPLHTTPFetch(osCurrentRequest, apszOptions);
        if (psResult->nDataLen == 0)
        {
            // status is not being set, always zero in cpl_http
            CPLDebug("JPIPWorkerFunc", "zero data returned from server: %s", psResult->pszErrBuf);
            break;
        }

        int bytes = psResult->nDataLen;
        long nEnd = clock();
		
        if ((nEnd - nStart) > 0)
            nCurrentTransmissionLength = (int) MAX(bytes / ((1.0 * (nEnd - nStart)) / CLOCKS_PER_SEC), nMinimumTransmissionLength);
		

        CPLAcquireMutex(poJDS->pGlobalMutex, 100.0);

        int bComplete = ((JPIPKAKDataset*)pRequest->poARIO->GetGDALDataset())->ReadFromInput(psResult->pabyData, psResult->nDataLen);
        if (bPriority)
            poJDS->nHighThreadByteCount += psResult->nDataLen;
        else
            poJDS->nLowThreadByteCount += psResult->nDataLen;

        pRequest->poARIO->SetComplete(bComplete);
		
        CPLReleaseMutex(poJDS->pGlobalMutex);
        CPLHTTPDestroyResult(psResult);

        if (bComplete)
            break;
    }

    CPLAcquireMutex(poJDS->pGlobalMutex, 100.0);

    if (bPriority)
    {
        poJDS->bHighThreadRunning = 0;
        poJDS->bHighThreadFinished = 1;
    }
    else
    {
        poJDS->bLowThreadRunning = 0;
        poJDS->bLowThreadFinished = 1;
    }

    CPLReleaseMutex(poJDS->pGlobalMutex);

    // end of thread
    delete pRequest;
}

