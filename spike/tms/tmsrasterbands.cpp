/******************************************************************************
 *
 * Project:  TMS Driver
 * Purpose:  Implement TMS raster band classes.
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

#include "tmsdataset.h"
#include "tmsrasterbands.h"
#include "tmsutil.h"

#include "cpl_port.h"


TMSTopRasterBand::TMSTopRasterBand (TMSDataset *ds, int _nband)
{
    assert_valid_band (ds, _nband + 1);
    
    poDS = ds;

    nBlockXSize = ds->tileWidth;
    nBlockYSize = ds->tileHeight;

    nBand = _nband + 1;    
    eDataType = ds->bandInfo[_nband].dataType;

    nOverviews = ds->nLevels;
    overviews = mkarray (TMSRasterBand *, nOverviews);
    forall (i, nOverviews) overviews[i] = new TMSRasterBand (ds, _nband, i);
}

TMSTopRasterBand::~TMSTopRasterBand ()
{
    forall (i, nOverviews) delete overviews[i];
    CPLFree (overviews);
}

int TMSTopRasterBand::GetOverviewCount (void)
{
    return nOverviews;
}

GDALRasterBand *TMSTopRasterBand::GetOverview (int num)
{
    assert_in_range (num, nOverviews);
    return overviews[num];
}

CPLErr TMSTopRasterBand::RasterIO (GDALRWFlag eRWFlag, 
                                   int nXOff, int nYOff, int nXSize, int nYSize,
                               	   void *pData, int nBufXSize, int	nBufYSize, 
                                   GDALDataType eBufType, 
                                   int nPixelSpace, int nLineSpace)
{
    TMSDataset *ds = TMSDATASET(poDS);
    double xfact, yfact;
    CPLErr ret;
    
    if (eRWFlag == GF_Read)
        return GDALRasterBand::RasterIO (eRWFlag, nXOff, nYOff, nXSize, nYSize,
            pData, nBufXSize, nBufYSize, eBufType, nPixelSpace, nLineSpace);
    else {
        ret = CE_None;
        
        forall (i, ds->nLevels) {
            
            xfact = ds->levels[i].width / nRasterXSize;
            yfact = ds->levels[i].height / nRasterYSize;
            
            ret = overviews[i]->RasterIO (eRWFlag, 
                nXOff * xfact, nYOff * yfact, nXSize * xfact, nYSize * yfact,
                pData, nBufXSize, nBufYSize, eBufType, nPixelSpace, nLineSpace);
        }
    }
    
    return ret;
}        

CPLErr TMSTopRasterBand::IReadBlock (int xoff, int yoff, void *buf)
{
    assert_valid_coordinates (TMSDATASET(poDS), 0, xoff, yoff);
    return overviews[0]->IReadBlock (xoff, yoff, buf);
}

CPLErr TMSTopRasterBand::IWriteBlock (int xoff, int yoff, void *buf)
{
    assert_valid_coordinates (TMSDATASET(poDS), 0, xoff, yoff);
    return overviews[0]->IWriteBlock (xoff, yoff, buf);
}

/****************************************************************************/

TMSRasterBand::TMSRasterBand (TMSDataset *ds, int _nband, int _level)
{
    assert_valid_band (ds, _nband + 1);
    assert_valid_level (ds, _level);
    
    poDS = ds;

    level = _level;
    nBand = _nband + 1;
    eDataType = ds->bandInfo[_nband].dataType;
    dataTypeSize = ds->bandInfo[_nband].dataTypeSize;
    
    nBlockXSize = ds->tileWidth;
    nBlockYSize = ds->tileHeight;
    nRasterXSize = ds->levels[level].width;
    nRasterYSize = ds->levels[level].height;
    heightInBlocks = ds->levels[level].heightInBlocks;
    gap = (heightInBlocks * nBlockYSize) - nRasterYSize;

    assert_in_range (gap, nBlockYSize); /* There was an error in this before. */
}

CPLErr TMSRasterBand::IReadBlock (int xoff, int yoff, void *buf)
{
    assert_valid_coordinates (TMSDATASET(poDS), level, xoff, yoff);
    return bool_to_CE_Error (BlockIO (GF_Read, xoff, heightInBlocks - yoff - 1, buf));    
}

CPLErr TMSRasterBand::IWriteBlock (int xoff, int yoff, void *buf)
{
    assert_valid_coordinates (TMSDATASET(poDS), level, xoff, yoff);
    return bool_to_CE_Error (BlockIO (GF_Write, xoff, heightInBlocks - yoff - 1, buf));    
}

void TMSRasterBand::CopyBuf (GDALRWFlag mode, void *a, int ayoff, void *b, int byoff, int numlines)
{
    assert (ayoff >= 0 && byoff >= 0 && numlines > 0);
    
    void *aadr = ( void * ) ((( GByte * ) a) + (nBlockXSize * ayoff));
    void *badr = ( void * ) ((( GByte * ) b) + (nBlockXSize * byoff));
    
    int sz = dataTypeSize * nBlockXSize * numlines;
    assert (sz > 0);
    
    if (mode == GF_Read) memcpy (badr, aadr, sz); else memcpy (aadr, badr, sz);
}

bool TMSRasterBand::BlockIO (GDALRWFlag mode, int xoff, int yoff, void *buf)
{
    TMSDataset *ds = TMSDATASET(poDS);
    GByte *tilebuf;
    
    assert_valid_coordinates (ds, level, xoff, yoff);
    
    must (ds->GetTileBand (level, xoff, yoff, nBand, &tilebuf));
    CopyBuf (mode, tilebuf, gap, buf, 0, nBlockYSize - gap);
    ds->ReleaseTile (level, xoff, yoff, mode == GF_Write);    

    if (gap > 0 && yoff > 0) {
        must (ds->GetTileBand (level, xoff, yoff - 1, nBand, &tilebuf));
        CopyBuf (mode, tilebuf, 0, buf, nBlockYSize - gap, gap);
        ds->ReleaseTile (level, xoff, yoff - 1, mode == GF_Write);
    }     

    succes;
}
