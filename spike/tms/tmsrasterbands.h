/******************************************************************************
 *
 * Project:  TMS Driver
 * Purpose:  Declare TMS related raster band classes.
 * Author:   Vaclav Klusak (keo at keo.cz)
 *
 ******************************************************************************
 * Copyright (c) 2008, Vaclav Klusak
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
