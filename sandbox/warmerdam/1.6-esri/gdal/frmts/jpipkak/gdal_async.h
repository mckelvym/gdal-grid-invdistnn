/* ******************************************************************** */
/*                          GDALAsyncReader                             */
/* ******************************************************************** */

/**
* status of the asynchronous stream
*/
typedef enum 
{	
	GARIO_PENDING = 0,
	GARIO_UPDATE = 1,
	GARIO_ERROR = 2,
	GARIO_COMPLETE = 3,
	GARIO_TypeCount = 4
} GDALAsyncStatusType;

/**
 * Class used as a session object for asynchronous requests.  They are
 * created with GDALDataset::BeginAsyncReader(), and destroyed with
 * GDALDataset::EndAsyncReader().
 */
class CPL_DLL GDALAsyncReader
{
  protected:
    GDALDataset* poDS;
    int          nXOff;
    int          nYOff;
    int          nXSize;
    int          nYSize;
    void *       pBuf;
    int          nBufXSize;
    int          nBufYSize;
    GDALDataType eBufType;
    int          nBandCount;
    int*         panBandMap;
    int          nPixelSpace;
    int          nLineSpace;
    int          nBandSpace;

  public:
    GDALAsyncReader() {}
    virtual ~GDALAsyncReader() {}

    GDALDataset* GetGDALDataset() {return poDS;}
    int GetXOffset() {return nXOff;}
    int GetYOffset() {return nYOff;}
    int GetXSize() {return nXSize;}
    int GetYSize() {return nYSize;}
    void * GetBuffer() {return pBuf;}
    int GetBufferXSize() {return nBufXSize;}
    int GetBufferYSize() {return nBufYSize;}
    GDALDataType GetBufferType() {return eBufType;}
    int GetBandCount() {return nBandCount;}
    int* GetBandMap() {return panBandMap;}
    int GetPixelSpace() {return nPixelSpace;}
    int GetLineSpace() {return nLineSpace;}
    int GetBandSpace() {return nBandSpace;}

    virtual GDALAsyncStatusType 
        GetNextUpdatedRegion(double dfTimeout,
                             int* pnBufXOff, int* pnBufYOff,
                             int* pnBufXSize, int* pnBufYSize) = 0;
    virtual int LockBuffer( double dfTimeout = -1.0 ) { return TRUE; }
    virtual void UnlockBuffer() {}
};
