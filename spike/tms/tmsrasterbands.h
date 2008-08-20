
#ifndef TMSRASTERBANDS_H_INCLUDED
#define TMSRASTERBANDS_H_INCLUDED

#include "gdal_pam.h"

class TMSDataset;
class TMSRasterBand;


/* Raster band wrapping all levels of detail. */

class TMSTopRasterBand : public GDALRasterBand
{
private:

    int nOverviews;
    TMSRasterBand **overviews;

public:

    TMSTopRasterBand  (TMSDataset *ds, int _nband);
    ~TMSTopRasterBand ();
    
    virtual int             GetOverviewCount (void);
    virtual GDALRasterBand *GetOverview      (int num);
    
    CPLErr RasterIO (GDALRWFlag eRWFlag, int nXOff, int nYOff, int nXSize, int nYSize,
		void *pData, int nBufXSize, int	nBufYSize, GDALDataType eBufType, int nPixelSpace,
		int nLineSpace);
    
    virtual CPLErr IReadBlock  (int xoff, int yoff, void *buf);
    virtual CPLErr IWriteBlock (int xoff, int yoff, void *buf);
};


/* Raster band representing one individual level of detail. */

class TMSRasterBand : public GDALRasterBand
{
private:

    int level;
    int gap;
    int heightInBlocks;
    int dataTypeSize;
    
    bool BlockIO (GDALRWFlag mode, int xoff, int yoff, void *buf);
    void CopyBuf (GDALRWFlag mode, void *a, int ayoff, void *b, int byoffset, int numlines);

public:
    
    TMSRasterBand (TMSDataset *ds, int _nband, int _level);

    virtual CPLErr IReadBlock  (int xoff, int yoff, void *buf);
    virtual CPLErr IWriteBlock (int xoff, int yoff, void *buf);    
};

#endif
