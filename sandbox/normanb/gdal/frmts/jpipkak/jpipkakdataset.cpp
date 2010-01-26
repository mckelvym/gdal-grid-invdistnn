/******************************************************************************
 * $Id: jpipkakdataset.cpp 2008-10-01 nbarker $
 *
 * Project:  jpip read driver
 * Purpose:  GDAL bindings for JPIP.  
 * Author:   Norman Barker, ITT VIS, norman.barker@gmail.com
 *
 ******************************************************************************
 * ITT Visual Information Systems grants you use of this code, under the following license:
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

static void *pGlobalMutex = NULL;
static void JPIPWorkerFunc(void *);

// support two communication threads to the server, a main and an overview thread
static volatile int bHighThreadRunning = 0;
static volatile int bLowThreadRunning = 0;
static volatile int bHighThreadFinished = 0;
static volatile int bLowThreadFinished = 0;

// transmission counts
static volatile long nHighThreadByteCount = 0;
static volatile long nLowThreadByteCount = 0;

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


/*****************************************/
/*         JPIPKAKDataset()              */
/*****************************************/
JPIPKAKDataset::JPIPKAKDataset()         
{
	this->pszTid = NULL;
	this->pszPath = NULL;
	this->pszCid = NULL;
	this->pszProjection = NULL;
	
	this->oCache = NULL;
	this->oCodestream = NULL;
	this->oDecompressor = NULL;

	this->nPos = 0;
	this->nVBASLen = 0;
	this->nVBASFirstByte = 0;

	this->nClassId = 0;
	this->nCodestream = 0;
	this->nDatabins = 0;
	this->bWindowDone = FALSE;
	this->bGeoTransformValid = FALSE;

	this->adfGeoTransform[0] = 0.0;
    this->adfGeoTransform[1] = 1.0;
    this->adfGeoTransform[2] = 0.0;
    this->adfGeoTransform[3] = 0.0;
    this->adfGeoTransform[4] = 0.0;
    this->adfGeoTransform[5] = 1.0;

	this->nGCPCount = 0;
    this->pasGCPList = NULL;
}


/*****************************************/
/*         ~JPIPKAKDataset()             */
/*****************************************/
JPIPKAKDataset::~JPIPKAKDataset()
{
	CPLHTTPCleanup();

	if (this->pszTid)
		CPLFree(this->pszTid);

	if (this->pszPath)
		CPLFree(this->pszPath);

	if (this->pszCid)
		CPLFree(this->pszCid);

	if (this->pszProjection)
		CPLFree(this->pszProjection);

	if (this->nGCPCount > 0 )
    {
        GDALDeinitGCPs( nGCPCount, pasGCPList );
        CPLFree( pasGCPList );
    }

	// frees decompressor as well
	if (this->oCodestream)
		this->oCodestream->destroy();

	if (this->oCache)
		delete this->oCache;

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
    CPLString osHeaders = "HEADERS=";
    osHeaders += "Accept: jpp-stream";
	CPLString osPersistent = "PERSISTENT=1";

    char *apszOptions[] = { 
        (char *) osHeaders.c_str(),
		(char *) osPersistent.c_str(),
        NULL 
    };
        
	// make initial request to the server for a session, we are going to assume that 
	// the jpip communication is stateful, rather than one-shot stateless requests
	// append pszUrl with jpip request parameters for a stateful session (multi-shot communications)
	// "cnew=http&type=jpp-stream&stream=0&tid=0&len="
	CPLString osRequest;
	osRequest.Printf("%s?%s%i", pszUrl, "cnew=http&type=jpp-stream&stream=0&tid=0&len=", 2000);
	
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

	// parse the response headers, and the initial data until we get to the codestream definition
	char** pszHdrs = psResult->papszHeaders;
	const char* pszTid = CSLFetchNameValue(pszHdrs, "JPIP-tid");
	const char* pszCnew = CSLFetchNameValue(pszHdrs, "JPIP-cnew");

	if (pszTid && pszCnew)
	{
		//CPLDebug("JPIPKAK", "tid - %s", pszTid);
		//CPLDebug("JPIPKAK", "cnew - %s", pszCnew);

		this->pszTid = CPLStrdup(pszTid);
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
				this->pszCid = CPLStrdup(pszValue);
			}

			if (EQUALN(papszTokens[i], "path", 4))
			{
				char *pszKey = NULL;
				const char *pszValue = CPLParseNameValue(papszTokens[i], &pszKey );
				this->pszPath = CPLStrdup(pszValue);
			}
		}

		CSLDestroy(papszTokens);

		if (pszPath && pszCid)
		{
			// ok, good to go with jpip, get to the codestream before returning successful initialisation
			// of the driver
			this->oCache = new kdu_cache();
			this->oCodestream = new kdu_codestream();
			this->oDecompressor = new kdu_region_decompressor();

			int bFinished = FALSE;
			bFinished = this->ReadFromInput(psResult->pabyData, psResult->nDataLen);
			CPLHTTPDestroyResult(psResult);

			// continue making requests in the main thread to get all the available metadata
			// for data bin 0, and reach the codestream

			// format the new request
			// and set as this->pszRequestUrl;
			// get the protocol from the original request
			size_t found = osRequest.find_first_of("/");
			CPLString osProtocol = osRequest.substr(0, found + 2);
			osRequest.erase(0, found + 2);
			// find context path
			found = osRequest.find_first_of("/");
			osRequest.erase(found);
			
			this->osRequestUrl.Printf("%s%s/%s?cid=%s&stream=0&len=%i", osProtocol.c_str(), osRequest.c_str(), this->pszPath, this->pszCid, 2000);

			while (!bFinished)
			{
				CPLHTTPResult *psResult = CPLHTTPFetch(osRequestUrl, apszOptions);
				bFinished = this->ReadFromInput(psResult->pabyData, psResult->nDataLen);
				CPLHTTPDestroyResult(psResult);
			}

			// clean up osRequest, remove variable len= parameter
			size_t pos = osRequestUrl.find_last_of("&");	
			osRequestUrl.erase(pos);

			// create codestream
			this->oCache->set_read_scope(KDU_MAIN_HEADER_DATABIN, 0, 0);
			this->oCodestream->create(this->oCache);
			this->oCodestream->set_persistent();

			kdu_channel_mapping* oChannels = new kdu_channel_mapping();
			oChannels->configure(*this->oCodestream);
			int nRef_Comp = oChannels->get_source_component(0);
			kdu_coords* ref_expansion = new kdu_coords(1, 1);
					
			// get available resolutions, image width / height etc.
			kdu_dims view_dims = this->oDecompressor->get_rendered_image_dims(*this->oCodestream, oChannels, -1, 0,
									*ref_expansion, *ref_expansion, KDU_WANT_OUTPUT_COMPONENTS);

			this->nRasterXSize = view_dims.size.x;
			this->nRasterYSize = view_dims.size.y;

			// TODO add color interpretation

			// calculate overviews
			siz_params* siz_in = this->oCodestream->access_siz();
			kdu_params* cod_in = siz_in->access_cluster("COD");

			CPLFree(&view_dims);
			delete oChannels;
			delete ref_expansion;

			cod_in->get("Clayers", 0, 0, this->nQualityLayers);
			cod_in->get("Clevels", 0, 0, this->nResLevels);

			// jpipkak does not support rasterband access, 
			// when this is added this->nComps can be replaced by this->nBands
			siz_in->get("Scomponents", 0, 0, this->nComps);
			siz_in->get("Sprecision", 0, 0, this->nBitDepth);

			// set specific metadata items
	    	CPLString osNQualityLayers;
			osNQualityLayers.Printf("%i", this->nQualityLayers);
	    	CPLString osNResolutionLevels;
			osNResolutionLevels.Printf("%i", this->nResLevels);
	    	CPLString osNComps;
			osNComps.Printf("%i", this->nComps);
			CPLString osBitDepth;
			osBitDepth.Printf("%i", this->nBitDepth);

			this->SetMetadataItem("JPIP_NQUALITYLAYERS", osNQualityLayers.c_str(), "JPIP");
			this->SetMetadataItem("JPIP_NRESOLUTIONLEVELS", osNResolutionLevels.c_str(), "JPIP");
			this->SetMetadataItem("JPIP_NCOMPS", osNComps.c_str(), "JPIP");
			this->SetMetadataItem("JPIP_SPRECISION", osBitDepth.c_str(), "JPIP");
	
			// parse geojp2, or gmljp2, we will assume that the core metadata of gml or a geojp2 uuid
			// have been sent in the initial metadata response
			// if the server has used placeholder boxes for this information then the image will
			// be interpreted as x,y
			GDALJP2Metadata oJP2Geo;
			int nLen = this->oCache->get_databin_length(KDU_META_DATABIN, this->nCodestream, 0);
			if (nLen > 0)
			{
				// create in memory file using vsimem
				CPLString osFileBoxName;
				osFileBoxName.Printf("/vsimem/jpip/%s.dat", this->pszCid);
				FILE *fpLL = VSIFOpenL(osFileBoxName.c_str(), "w+");
				this->oCache->set_read_scope(KDU_META_DATABIN, this->nCodestream, 0);
				kdu_byte* pabyBuffer = (kdu_byte *)CPLMalloc(nLen);
				this->oCache->read(pabyBuffer, nLen);
				VSIFWriteL(pabyBuffer, nLen, 1, fpLL);
				VSIFFlushL(fpLL);
				VSIFSeekL(fpLL, 0, SEEK_SET);

				this->nPamFlags |= GPF_NOSAVE;

				try
				{
					oJP2Geo.ReadBoxes(fpLL);
					// parse gml first, followed by geojp2 as a fallback
					if (oJP2Geo.ParseGMLCoverageDesc() || oJP2Geo.ParseJP2GeoTIFF())
					{
						this->pszProjection = CPLStrdup(oJP2Geo.pszProjection);
						this->bGeoTransformValid = TRUE;

						memcpy(this->adfGeoTransform, oJP2Geo.adfGeoTransform, 
								sizeof(double) * 6 );
						this->nGCPCount = oJP2Geo.nGCPCount;
						this->pasGCPList = oJP2Geo.pasGCPList;

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

							this->GDALPamDataset::SetMetadata( apszMDList, osDomain );
							CPLFree( pszName );
						}
				

						this->SetDescription( CPLStrdup(pszUrl));
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

			}
			else
			{
				CPLError(CE_Failure, CPLE_AppDefined, 
                  "Unable to open stream to parse metadata boxes" );
			}

			return TRUE;
		}
		else
		{
			CPLHTTPDestroyResult(psResult);
			CPLError(CE_Failure, CPLE_AppDefined, "Error parsing path and cid from cnew - %s", pszCnew);
			return FALSE;
		}
	}
	else
	{
        CPLHTTPDestroyResult( psResult );
		CPLError(CE_Failure, CPLE_AppDefined, 
                  "Unable to parse required cnew and tid response headers" );
        return FALSE;    
	}
}

/******************************************/
/*           ReadVBAS()                   */
/******************************************/
long JPIPKAKDataset::ReadVBAS(GByte* pabyData, int nLen)
{
	int c = -1;
	long val = 0;
	this->nVBASLen = 0;

	while ((c & 0x80) != 0)
	{
		if (this->nVBASLen >= 9)
		{
			CPLError(CE_Failure, CPLE_AppDefined,
				"VBAS Length not supported");
			break;
		}

		if (this->nPos > nLen)
		{
			CPLError(CE_Failure, CPLE_AppDefined,
				"EOF reached before completing VBAS");
			break;
		}

		c = pabyData[this->nPos];
		this->nPos++;

		val = (val << 7) | (long)(c & 0x7F);

		if (this -> nVBASLen == 0)
			this -> nVBASFirstByte = c;

		this->nVBASLen++;
	}

	return val;
}

/******************************************/
/*            ReadSegment()               */
/******************************************/
JPIPDataSegment* JPIPKAKDataset::ReadSegment(GByte* pabyData, int nLen)
{
	long nId = this->ReadVBAS(pabyData, nLen);
	if (nId < 0)
		return NULL;
	else
	{
		JPIPDataSegment* segment = new JPIPDataSegment();
		segment->SetId(nId);

		if (this->nVBASFirstByte == 0)
		{
			segment->SetEOR(TRUE);
			segment->SetId(pabyData[this->nPos]);
		}
		else
		{
			segment->SetEOR(FALSE);
			nId &= ~(0x70 << ((this->nVBASLen -1) * 7));
			segment->SetId(nId);
			segment->SetFinal((this->nVBASFirstByte & 0x10) != 0);

			int m = (this->nVBASFirstByte & 0x7F) >> 5;

			if (m == 0)
			{
				CPLError(CE_Failure, CPLE_AppDefined,
					"Invalid Bin-ID value format");
			}
			else if (m >= 2) {
				this->nClassId = this->ReadVBAS(pabyData, nLen);
				if (m > 2)
					this->nCodestream = this->ReadVBAS(pabyData, nLen);
			}

			segment->SetClassId(this->nClassId);
			segment->SetCodestreamIdx(this->nCodestream);
			segment->SetOffset(this->ReadVBAS(pabyData, nLen));
			segment->SetLen(this->ReadVBAS(pabyData, nLen));
		}

		if ((segment->GetLen() > 0) && (!segment->IsEOR()))
		{
			GByte* pabyDataSegment = new GByte[segment->GetLen()];
			
			// copy data from input array pabyData to the data segment
			memcpy(pabyDataSegment, 
				pabyData + this->nPos,
				segment->GetLen());

			segment->SetData(pabyDataSegment);
		}

		this->nPos += segment->GetLen();

		if (!segment->IsEOR())
			nDatabins++;

		if ((segment->GetId() == JPIPKAKDataset::JPIP_EOR_WINDOW_DONE) && (segment->IsEOR()))
			this->bWindowDone = TRUE;

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
		this->nPos = 0;
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
				this->oCache->add_to_databin(this->KakaduClassId(pSegment->GetClassId()), pSegment->GetCodestreamIdx(),
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

/*************************************************************************/
/*                     BeginAsyncRasterIO()                              */
/*                                                                       */
/*  Caller is responsible for freeing the input buffer and the band map  */
/*  Options include LEVEL=, LAYERS=, PRIORITY= for resolution level,     */
/*  number of quality layers, and thread retrieval priority              */
/*  xOff, yOff, xSize, ySize are at 1:1									 */
/*************************************************************************/
GDALAsyncRasterIO* JPIPKAKDataset::BeginAsyncRasterIO(int xOff, int yOff,
								int xSize, int ySize, 
								void *pBuf,
								int bufXSize, int bufYSize,
								GDALDataType bufType,
								int nBandCount, int* pBandMap,
								int nPixelSpace, int nLineSpace,
								int nBandSpace,
								char **papszOptions)
{
	JPIPKAKAsyncRasterIO* ario = new JPIPKAKAsyncRasterIO(this);
	ario->pBuf = pBuf;
	ario->bufXSize = bufXSize;
	ario->bufYSize = bufYSize;
	ario->bufType = bufType;
	ario->nBandCount = nBandCount;
	ario->pBandMap = pBandMap;
	ario->nPixelSpace = nPixelSpace;
	ario->nLineSpace = nLineSpace;
	ario->nBandSpace = nBandSpace;

	if (pBandMap)
	{
		ario->pBandMap = new int[nBandCount];
		for (int i = 0; i < nBandCount; i++)
			ario->pBandMap[i] = pBandMap[i];
	}

	// check we have sensible values
	if (xOff > this->GetRasterXSize())
		xOff = this->GetRasterXSize();

	if (yOff > this->GetRasterYSize())
		yOff = this->GetRasterYSize();

	if ((xOff + xSize) > this->GetRasterXSize())
		xSize = this->GetRasterXSize() - xOff;

	if ((yOff + ySize) > this->GetRasterYSize())
		ySize = this->GetRasterYSize() - yOff;

	ario->xOff = xOff;
	ario->yOff = yOff;
	ario->xSize = xSize;
	ario->ySize = ySize;

	// parse options
	const char* pszLevel = CSLFetchNameValue(papszOptions, "LEVEL");
	const char* pszLayers = CSLFetchNameValue(papszOptions, "LAYERS");
	const char* pszPriority = CSLFetchNameValue(papszOptions, "PRIORITY");
	
	if (pszLevel)
		ario->nLevel = atoi(pszLevel);
	else
	    ario->nLevel = this->nResLevels;

	if (pszLayers)
		ario->nQualityLayers = atoi(pszLayers);
	else
		ario->nQualityLayers = this->nQualityLayers;

	if (pszPriority)
	{
		if (EQUAL(pszPriority, "0"))
			ario->bHighPriority = 0;
		else
			ario->bHighPriority = 1;
	}
	else
		ario->bHighPriority = 1;

	ario->Start();

	return ario;
}

/************************************************************************/
/*                  EndAsyncRasterIO                                    */
/************************************************************************/
void JPIPKAKDataset::EndAsyncRasterIO(GDALAsyncRasterIO *poARIO)
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
/*                        GDALRegister_JPIPKAK()				*/
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

/**********************************************/
/*             JPIPKAKAsyncRasterIO           */
/**********************************************/
JPIPKAKAsyncRasterIO::JPIPKAKAsyncRasterIO(GDALDataset *poDS)
{
	this->poDS = (JPIPKAKDataset *)poDS;
	this->pBandMap = NULL;
	this->pBuf = NULL;
	this->nDataRead = 0;
}

/***********************************************/
/*              ~JPIPKAKAsyncRasterIO          */
/***********************************************/
JPIPKAKAsyncRasterIO::~JPIPKAKAsyncRasterIO()
{
	this->Stop();

	// don't own the buffer
	if (this->pBandMap)
		delete pBandMap;
}
/************************************************/
/*           Start()                            */
/************************************************/
void JPIPKAKAsyncRasterIO::Start()
{

	// stop the currently running thread
	// start making requests to the server to the server
	if (pGlobalMutex == NULL)
	{
		pGlobalMutex = CPLCreateMutex();
		CPLReleaseMutex(pGlobalMutex);
	}

	nDataRead = 0;
	bComplete = 0;

	// check a thread is not already running
	if (((this->bHighPriority) && (bHighThreadRunning)) || 
			((!this->bHighPriority) && (bLowThreadRunning)))
		CPLError(CE_Failure, CPLE_AppDefined, "JPIPKAKAsyncRasterIO supports at most two concurrent server communication threads");
	else
	{
		// calculate the url the worker function is going to retrieve
		int w = this->poDS->GetRasterXSize();
		int h = this->poDS->GetRasterYSize();
		
		// calculate the kakadu adjust image size
		channels.configure(*(((JPIPKAKDataset*)this->poDS)->oCodestream));
		// set band mapping
		if (pBandMap)
		{
			for (int i = 0; i < nBandCount; i++)
				channels.source_components[i] = pBandMap[i];
		}

		kdu_coords* reference_expansion = new kdu_coords(1, 1);

		// find current canvas width and height in the cache and check we don't
		// exceed this in our process request
		kdu_dims view_dims;	
		if (nBandCount == 1)
			view_dims = ((JPIPKAKDataset*)this->poDS)->oDecompressor->get_rendered_image_dims(*((JPIPKAKDataset*)this->poDS)->oCodestream, NULL, 
				channels.source_components[0], this->nLevel, *reference_expansion);
		else
			view_dims = ((JPIPKAKDataset*)this->poDS)->oDecompressor->get_rendered_image_dims(*((JPIPKAKDataset*)this->poDS)->oCodestream, &channels, 
				-1, this->nLevel, *reference_expansion);

		kdu_coords* view_siz = view_dims.access_size();
		
		delete reference_expansion;

		int nRes = 1;
		if (this->nLevel > 0)
			nRes = 2 << (this->nLevel - 1);

		// formulate jpip parameters and adjust offsets for current level
		int fx = view_siz->x;
		int fy = view_siz->y;

		this->xOff = ceil(this->xOff / (1.0 * nRes)); // roffx
		this->yOff = ceil(this->yOff / (1.0 * nRes)); // roffy
		this->xSize = ceil(this->xSize / (1.0 * nRes)); // rsizx
		this->ySize = ceil(this->ySize / (1.0 * nRes)); // rsizy

		if ((this->xOff + this->xSize) > fx)
			this->xSize = fx - this->xOff;

		if ((this->yOff+ this->ySize) > fy)
			this->ySize = fy - this->yOff;

		CPLString jpipUrl;
		CPLString comps;

		for (int i = 0; i < nBandCount; i++)
			comps.Printf("%s%i,", comps.c_str(), pBandMap[i]);

		comps.erase(comps.length() -1);
	
		jpipUrl.Printf("%s&type=jpp-stream&roff=%i,%i&rsiz=%i,%i&fsiz=%i,%i,closest&quality=%i&comps=%s", 
			((JPIPKAKDataset*)this->poDS)->osRequestUrl.c_str(),
			this->xOff, this->yOff,
			this->xSize, this->ySize,
			fx, fy,
			this->nQualityLayers, comps.c_str());

		JPIPRequest* pRequest = new JPIPRequest();
		pRequest->bPriority = this->bHighPriority;
		pRequest->osRequest = jpipUrl;
		pRequest->poARIO = this;

		//CPLDebug("JPIPKAKAsyncRasterIO", "THREADING TURNED OFF");
		if (CPLCreateThread(JPIPWorkerFunc, pRequest) == -1)
			CPLError(CE_Failure, CPLE_AppDefined, 
					"Unable to create worker jpip  thread" );
		// run in main thread as a test
		//JPIPWorkerFunc(pRequest);

	}

}

void JPIPKAKAsyncRasterIO::Stop()
{
	bComplete = 1;
	if (pGlobalMutex)
	{

		if (((this->bHighPriority) && (!bHighThreadFinished)) || 
			((!this->bHighPriority) && (!bLowThreadFinished)))
		{
			// stop the thread
			if (this->bHighPriority)
			{
				CPLAcquireMutex(pGlobalMutex, 100.0);
				bHighThreadRunning = 0;
				CPLReleaseMutex(pGlobalMutex);
		
				while (!bHighThreadFinished)
					CPLSleep(0.1);
			}
			else
			{
				CPLAcquireMutex(pGlobalMutex, 100.0);
				bLowThreadRunning = 0;
				CPLReleaseMutex(pGlobalMutex);
		
				while (!bLowThreadFinished)
				{
					CPLSleep(0.1);
				}
			}

		}

	}
}

/***********************************************/
/*            GetNextUpdatedRegion()           */
/***********************************************/
GDALAsyncStatusType JPIPKAKAsyncRasterIO::GetNextUpdatedRegion(bool wait, int timeout,
                                                   int* pnxbufoff,
                                                   int* pnybufoff,
                                                   int* pnxbufsize,
                                                   int* pnybufsize)
{
	GDALAsyncStatusType result = GARIO_ERROR;

	long nSize = 0;
    // take a snapshot of the volatile variables
	if (bHighPriority)
	{
		const long s = nHighThreadByteCount - nDataRead;
		nSize = s;
	}
	else
	{
		const long s = nLowThreadByteCount - nDataRead;
		nSize = s;
	}

	// wait for new data if required
	if ((nSize == 0) && (wait))
	{
		// poll for change in cache size
		clock_t end_wait = NULL;

		if (timeout > 0)
			end_wait = clock() + timeout * CLOCKS_PER_SEC; 
		
		while ((nSize == 0) && ((bHighPriority && bHighThreadRunning) ||
			(!bHighPriority && bLowThreadRunning)))
		{
			if (end_wait)
				if (clock() > end_wait)
					break;

			CPLSleep(0.1);

			if (bHighPriority)
			{
				const long s = nHighThreadByteCount - nDataRead;
				nSize = s;
			}
			else
			{
				const long s = nLowThreadByteCount - nDataRead;
				nSize = s;
			}

		}
	}
	else
	{
		*pnxbufoff = 0;
		*pnybufoff = 0;
		*pnxbufsize = 0;
		*pnybufsize = 0;		
		if (!wait & !bComplete)
			result =  GARIO_PENDING;
	}

	if (!(result == GARIO_PENDING))
	{

		kdu_coords* expansion = new kdu_coords(1, 1);

		kdu_dims* region = new kdu_dims();
		region->access_pos()->x = this->xOff;
		region->access_pos()->y = this->yOff;
		region->access_size()->x = this->xSize;
		region->access_size()->y = this->ySize;
		((JPIPKAKDataset*)this->poDS)->oCodestream->apply_input_restrictions(0, 0, this->nLevel, this->nQualityLayers, region);

		kdu_dims* incomplete_region = new kdu_dims();
		incomplete_region->assign(*region);

		kdu_coords* origin = new kdu_coords(region->access_pos()->x, region->access_pos()->y);
		int* channel_offsets = (int *)CPLCalloc((channels.num_channels), sizeof(int));


		int bIsDecompressing = FALSE;
		
		CPLAcquireMutex(pGlobalMutex, 100.0);

		if (nBandCount == 1)
			bIsDecompressing = ((JPIPKAKDataset*)this->poDS)->oDecompressor->start(*((JPIPKAKDataset*)this->poDS)->oCodestream, NULL,
				channels.get_source_component(0), this->nLevel, this->nQualityLayers, 
				*region, *expansion,
				*expansion, TRUE);	
		else
			bIsDecompressing = ((JPIPKAKDataset*)this->poDS)->oDecompressor->start(*((JPIPKAKDataset*)this->poDS)->oCodestream, &channels,
			-1, this->nLevel, this->nQualityLayers, *region, *expansion,
				*expansion, TRUE);	
		
		*pnxbufoff = region->access_pos()->x;
		*pnybufoff = region->access_pos()->y;
		*pnxbufsize = region->access_size()->x;
		*pnybufsize = region->access_size()->y;


		int nPrecisionBits = 8;

		switch(bufType)
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

        while ((bIsDecompressing == 1) || (incomplete_region->area() != 0))
        {
            if (nPrecisionBits <= 8)
            {
                bIsDecompressing = ((JPIPKAKDataset*)this->poDS)->oDecompressor->process((kdu_int32*)pBuf, 
                    *origin, *pnxbufsize, 100000, 0, *incomplete_region, *region);
            }
            else
            {
                    bIsDecompressing = ((JPIPKAKDataset*)this->poDS)->oDecompressor->process((kdu_uint16 *)pBuf, 
                        channel_offsets, 1, *origin, *pnxbufsize, 100000, 0, *incomplete_region, *region, nPrecisionBits);
            }
        }

		((JPIPKAKDataset*)this->poDS)->oDecompressor->finish();
		CPLReleaseMutex(pGlobalMutex);

		CPLFree(channel_offsets);
		delete expansion;
		delete region;
		delete incomplete_region;
		delete origin;

		// has there been any more data read whilst we have been processing?
		long size = 0;
		if (bHighPriority)
		{
			const long x = nHighThreadByteCount - nDataRead;
			size = x;
		}
		else
		{
			const long x = nLowThreadByteCount - nDataRead;
			size = x;
		}

		if ((bComplete) && (nSize == size))
			result = GARIO_COMPLETE;
		else
			result = GARIO_UPDATE;

		nDataRead += nSize;
	}

	return result;	
}

/***********************************************/
/*            LockBuffer()                     */
/***********************************************/
void JPIPKAKAsyncRasterIO::LockBuffer()
{
	// acquire the mutex
	
}

/***********************************************/
/*            LockBuffer()                     */
/***********************************************/
void JPIPKAKAsyncRasterIO::LockBuffer(int xbufoff, int ybufoff, int xbufsize, int ybufsize)
{

        CPLError(CE_Failure, CPLE_AppDefined, 
                  "Partial locks not implemented (yet)." );
}

/************************************************/
/*            UnlockBuffer()                    */
/************************************************/
void JPIPKAKAsyncRasterIO::UnlockBuffer()
{
	// release the mutex
}

/***********************************************/
/*              JPIPDataSegment                */
/***********************************************/
JPIPDataSegment::JPIPDataSegment()
{
	this -> nId = 0;
	this -> nAux = 0;
	this -> nClassId = 0;
	this -> nCodestream = 0;
	this -> nOffset = 0;
	this -> nLen = 0;
	this -> pabyData = NULL;
	this -> bIsFinal = FALSE;
	this -> bIsEOR = FALSE;
}

/***********************************************/
/*              ~JPIPDataSegment               */
/***********************************************/
JPIPDataSegment::~JPIPDataSegment()
{
	if (this->pabyData)
		CPLFree(this->pabyData);
}

static void JPIPWorkerFunc(void *req)
{
	int nCurrentTransmissionLength = 2000;
	int nMinimumTransmissionLength = 2000;

	JPIPRequest *pRequest = (JPIPRequest *)req;
	
	int bPriority = pRequest->bPriority;

	CPLAcquireMutex(pGlobalMutex, 100.0);

	// set the running status
	if (bPriority)
	{
		bHighThreadRunning = 1;
		bHighThreadFinished = 0;
	}
	else
	{
		bLowThreadRunning = 1;
		bLowThreadFinished = 0;
	}

	CPLReleaseMutex(pGlobalMutex);

	CPLString osHeaders = "HEADERS=";
    osHeaders += "Accept: jpp-stream";
	CPLString osPersistent = "PERSISTENT=1";


    char *apszOptions[] = { 
        (char *) osHeaders.c_str(),
		(char *) osPersistent.c_str(),
        NULL 
    };

	while (TRUE)
	{
		// modulate the len= parameter to use the currently available bandwidth
		long nStart = clock();

		if (((bPriority) && (!bHighThreadRunning)) || ((!bPriority) && (!bLowThreadRunning)))
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
			nCurrentTransmissionLength = max(bytes / ((1.0 * (nEnd - nStart)) / CLOCKS_PER_SEC), nMinimumTransmissionLength);
		

		CPLAcquireMutex(pGlobalMutex, 100.0);

		int bComplete = ((JPIPKAKDataset*)pRequest->poARIO->GetGDALDataset())->ReadFromInput(psResult->pabyData, psResult->nDataLen);
		if (bPriority)
			nHighThreadByteCount += psResult->nDataLen;
		else
			nLowThreadByteCount += psResult->nDataLen;

		pRequest->poARIO->SetComplete(bComplete);
		
		CPLReleaseMutex(pGlobalMutex);
		CPLHTTPDestroyResult(psResult);

		if (bComplete)
			break;
	}


	CPLAcquireMutex(pGlobalMutex, 100.0);

	if (bPriority)
	{
		bHighThreadRunning = 0;
		bHighThreadFinished = 1;
	}
	else
	{
		bLowThreadRunning = 0;
		bLowThreadFinished = 1;
	}

	CPLReleaseMutex(pGlobalMutex);

	// end of thread
	delete pRequest;


}

