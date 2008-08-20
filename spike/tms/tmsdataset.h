/******************************************************************************
 *
 * Project:  TMS Driver
 * Purpose:  Declare TMS related dataset class.
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

#ifndef TMSDATASET_H_INCLUDED
#define TMSDATASET_H_INCLUDED

#include "gdal_pam.h"
#include "cpl_minixml.h"

#include "tmsutil.h"

class TMSDataset;
class TMSTopRasterBand;
class TMSRasterBand;

#define TMSDATASET(ds) (( TMSDataset * ) ds)

struct Level 
{
    char  *dirName;
    double unitsppx;
    int    order;
    int    width, height;
    int    widthInBlocks, heightInBlocks;
};

struct BandInfo
{
    GDALDataType dataType;
    int          dataTypeSize;
    int          tileSizeBytes;
};

struct Tile
{
    int  level, xoff, yoff;
    bool used, dirty, locked;
};

class TMSDataset : public GDALDataset
{
private:

    friend class TMSTopRasterBand;
    friend class TMSRasterBand;
    friend class WorkSet;
    
    double geotrans[6];
    bool   haveGeoTrans;
    
    char *basedir;
    char *xmlfn;

    int tileWidth, tileHeight;
    char *extension;
    
    int width, height;
    
    int nLevels;
    Level *levels;

    Tile        *tiles;
    int       ***cache;
    GByte      **pixbuf;
    BandInfo    *bandInfo;
    GDALDataset *outTile;
    int          numCachedTiles;


    bool InitFromXML (CPLXMLNode *xml);
    bool WriteNew    (int xsize, int ysize, GDALDataset *tile, char **parlist);

    bool SetDimensions   (double minx, double maxx, double miny, double maxy);
    bool CreateCache     (void);
    bool CalculateLevels (void);
    bool CreateBandInfo  (void);
    bool WriteXML        (void);
    
    bool GetTileBand (int level, int xoff, int yoff, int nband, GByte **result);
    void ReleaseTile (int level, int xoff, int yoff, bool dirty);
    void FlushTile   (int id); 
    
    bool  ReadTile  (int level, int xoff, int yoff, GDALDataset **result);
    void  WriteTile (GDALDataset *tile, int level, int xoff, int yoff);
    char *GetTileFilename (int level, int xoff, int yoff);
    
    void GetNSEW (double *vect, double *north, double *south, double *east, double *west);
    
    void DebugInfo (void);
    
public:

    TMSDataset  (const char *fn);
    ~TMSDataset ();

    static GDALDataset *Open (GDALOpenInfo *info);
    static GDALDataset *Create (const char *fn, int xsize, int ysize, int nbands, 
                                 GDALDataType dataType, char **parlist);

    virtual CPLErr SetGeoTransform (double *vect);
    virtual CPLErr GetGeoTransform (double *vect);
                              
    virtual void FlushCache (void); 
};

#define assert_valid_level(obj, level) \
    assert_in_range (level, (obj)->nLevels)

#define assert_valid_coordinates(obj, level, xoff, yoff)      \
    assert_valid_level (obj, level);                          \
    assert_in_range (xoff, (obj)->levels[level].widthInBlocks); \
    assert_in_range (yoff, (obj)->levels[level].heightInBlocks)
    
#define assert_valid_band(obj, band) \
    assert ((band) >= 1 && (band) <= (obj)->nBands)

#endif
