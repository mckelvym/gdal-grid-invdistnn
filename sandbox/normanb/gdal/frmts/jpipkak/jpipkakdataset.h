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

#include "gdal_pam.h"
#include "gdaljp2metadata.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "cpl_http.h"
#include "cpl_vsi.h"
#include "cpl_multiproc.h"

#include "kdu_cache.h"
#include "kdu_region_decompressor.h"
#include "kdu_file_io.h"

#include <time.h>

class JPIPDataSegment
{
private:
	long nId;
	long nAux;
	long nClassId;
	long nCodestream;
	long nOffset;
	long nLen;
	GByte* pabyData;
	int bIsFinal;
	int bIsEOR;
public:
	int GetId(){return nId;}
	int GetAux(){return nAux;}
	int GetClassId(){return nClassId;}
	int GetCodestreamIdx(){return nCodestream;}
	int GetOffset(){return nOffset;}
	int GetLen(){return nLen;}
	GByte* GetData(){return pabyData;}
	int IsFinal(){return bIsFinal;}
	int IsEOR(){return bIsEOR;}

	void SetId(long nId){this->nId = nId;}
	void SetAux(long nAux){this->nAux = nAux;}
	void SetClassId(long nClassId){this->nClassId = nClassId;}
	void SetCodestreamIdx(long nCodestream){this->nCodestream = nCodestream;}
	void SetOffset(long nOffset){this->nOffset = nOffset;}
	void SetLen(long nLen){this->nLen = nLen;}
	void SetData(GByte* pabyData){this->pabyData = pabyData;}
	void SetFinal(int bIsFinal){this->bIsFinal = bIsFinal;}
	void SetEOR(int bIsEOR){this->bIsEOR = bIsEOR;}
	JPIPDataSegment();
	~JPIPDataSegment();
};

class JPIPKAKDataset: public GDALPamDataset
{
private:
	CPLString osRequestUrl;
	char* pszTid;
	char* pszPath;
	char* pszCid;
	char* pszProjection;

	int nPos;
	int nVBASLen;
	int nVBASFirstByte;
	int nClassId;
	int nQualityLayers;
	int nResLevels;
	int nComps;
	int nBitDepth;

	int nCodestream;
	long nDatabins;

	double adfGeoTransform[6];

	int bWindowDone;
	int bGeoTransformValid;

	int nGCPCount;
    GDAL_GCP *pasGCPList;

	// kakadu
	kdu_codestream *oCodestream;
	kdu_region_decompressor *oDecompressor;
	kdu_cache *oCache;
    long ReadVBAS(GByte* pabyData, int nLen);
	JPIPDataSegment* ReadSegment(GByte* pabyData, int nLen);
	int Initialise(char* url);
	int KakaduClassId(int nClassId);
public:
	JPIPKAKDataset();
	virtual ~JPIPKAKDataset();
	// progressive methods
	virtual GDALAsyncRasterIO* BeginAsyncRasterIO(int xOff, int yOff,
									int xSize, int ySize, 
									void *pBuf,
									int bufXSize, int bufYSize,
									GDALDataType bufType,
									int nBandCount, int* bandMap,
									int nPixelSpace, int nLineSpace,
									int nBandSpace,
									char **papszOptions);

	virtual void EndAsyncRasterIO(GDALAsyncRasterIO *);
	int GetNQualityLayers(){return nQualityLayers;}
	int GetNResolutionLevels(){return nResLevels;}
	int GetNComponents(){return nComps;}

	int ReadFromInput(GByte* pabyData, int nLen);

	//gdaldataset methods
	virtual CPLErr GetGeoTransform( double * );
    virtual const char *GetProjectionRef(void);
    virtual int    GetGCPCount();
    virtual const char *GetGCPProjection();
    virtual const GDAL_GCP *GetGCPs();

	static GDALDataset *Open(GDALOpenInfo *);
	static const GByte JPIP_EOR_IMAGE_DONE = 1;
	static const GByte JPIP_EOR_WINDOW_DONE = 2;
	static const GByte MAIN_HEADER_DATA_BIN_CLASS = 6;
	static const GByte META_DATA_BIN_CLASS = 8;
	static const GByte PRECINCT_DATA_BIN_CLASS = 0;
	static const GByte TILE_HEADER_DATA_BIN_CLASS = 2;
	static const GByte TILE_DATA_BIN_CLASS = 4;

	
	friend class JPIPKAKAsyncRasterIO;
};

class JPIPKAKAsyncRasterIO : public GDALAsyncRasterIO
{
private:
	int nLevel;
	int nQualityLayers;
	int bHighPriority;
	int bComplete;
    kdu_channel_mapping channels;
	void Start();
	void Stop();
public:
	JPIPKAKAsyncRasterIO(GDALDataset *poDS);
	virtual ~JPIPKAKAsyncRasterIO();

	/* Returns GARIO_UPDATE, GARIO_NO_MESSAGE (if pending==false and nothing in the queue
	or if pending==true && timeout != 0 and nothing in the queue at the end of the timeout), 
	GARIO_COMPLETE, GARIO_ERROR */
	virtual GDALAsyncStatusType GetNextUpdatedRegion(bool wait, int timeout,
                                                   int* pnxbufoff,
                                                   int* pnybufoff,
                                                   int* pnxbufsize,
                                                   int* pnybufsize);
	/* if wait = true, we wait forever if timeout=0, for the timeout time otherwise */
	/* if wait = false, we return immediately */
	/* the int* are output values */

	// lock a whole buffer.
	virtual void LockBuffer();

	// lock only a block
    // the caller must relax a previous lock before asking for a new one
    virtual void LockBuffer(int xbufoff, int ybufoff, int xbufsize, int ybufsize);
    virtual void UnlockBuffer(); 

	void SetComplete(int bFinished){this->bComplete = bFinished;};

	friend class JPIPKAKDataset;
};

struct JPIPRequest
{
	int bPriority;
	CPLString osRequest;
	JPIPKAKAsyncRasterIO* poARIO;
    ~JPIPRequest()
	{
		// poDS is owned elsewhere
	}

};
