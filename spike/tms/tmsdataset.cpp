/******************************************************************************
 *
 * Project:  TMS Driver
 * Purpose:  Implement TMS related dataset class.
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

#include "cpl_string.h"
#include "cpl_port.h"
#include "cpl_vsi.h"

#include <cstdlib>
#include <cmath>
#include <cfloat>


/*
 *  These values provide valid ranges to some attributes of TileMap resource.
 */

#define MAX_WIDTH           (1024 * 1024)
#define MAX_HEIGHT          MAX_WIDTH

#define MAX_TILE_WIDTH      2048
#define MAX_TILE_HEIGHT     MAX_TILE_WIDTH

#define MAX_NUM_LEVELS      1024

#define MIN_UNITS_PPX       DBL_EPSILON
#define MAX_UNITS_PPX       DBL_MAX

#define MAX_ORDER           1024 /* Zero is considered minimal. */

/*
 *  These are for sanity checking. 
 */

#define MAX_NUM_BANDS           128

#define MAX_NUM_TILES_WIDTH     1024
#define MAX_NUM_TILES_HEIGHT    MAX_NUM_TILES_WIDTH

#define MAX_CACHE_BYTES         (16 * 1024 * 1024)
#define MIN_CACHED_TILES        8
#define MAX_CACHED_TILES        128


static GDALDriver *GetDriverByName (const char *name);


TMSDataset::TMSDataset (const char *dirname)
{ 
    extension = NULL;
    basedir   = CPLStrdup (dirname);
    xmlfn     = NULL;
    levels    = NULL;
    cache     = NULL;
    pixbuf    = NULL;
    tiles     = NULL;    
    outTile   = NULL;
    bandInfo  = NULL;
    
    haveGeoTrans = false;
    GDALDataset::GetGeoTransform (geotrans);
}

TMSDataset::~TMSDataset ()
{
    if (extension) CPLFree (extension);
    if (basedir)   CPLFree (basedir);
    if (xmlfn)     CPLFree (xmlfn);
    if (cache) { 
        FlushCache ();
        forall (l, nLevels) {
            forall (x, levels[l].widthInBlocks) CPLFree (cache[l][x]);
            CPLFree (cache[l]);
        }
        CPLFree (cache);
    }
    if (pixbuf) {
        forall (i, nBands) CPLFree (pixbuf[i]);
        CPLFree (pixbuf);    
    }
    if (outTile) delete outTile;
    if (levels) {
        forall (i, nLevels) CPLFree (levels[i].dirName);
        CPLFree (levels);
    }
    if (tiles)    CPLFree (tiles);
    if (bandInfo) CPLFree (bandInfo);
}

/*
        Initialization.
*/

GDALDataset *TMSDataset::Open (GDALOpenInfo *info)
{
    CPLXMLNode *config, *tileMap;
    TMSDataset *ds;
    
    if (!info->fp) return NULL;

    if( info->pabyHeader == NULL 
        || strstr((const char *) info->pabyHeader, "<TileMap") == NULL )
        return NULL;

    if (!(config = CPLParseXMLFile (info->pszFilename)))
        return NULL;

    if (!(tileMap = CPLGetXMLNode (config, "=TileMap")))
    {
        CPLDestroyXMLNode( config );
        return NULL;
    }
        
    ds = new TMSDataset (CPLGetDirname (info->pszFilename));
    ds->xmlfn = CPLStrdup (info->pszFilename);

    if (!ds->InitFromXML (tileMap)) {
        delete ds;
        ds = NULL;
    } else {
        ds->DebugInfo ();
    }
    
    CPLDestroyXMLNode (config);

    return ds;
}

GDALDataset *TMSDataset::Create (const char *fn, int xsize, int ysize, int nbands, 
                                 GDALDataType dataType, char **parlist)
{
    GDALDriver  *drv;
    GDALDataset *tile;
    TMSDataset  *ds;

    drv = GetDriverByName ("MEM");
    tile = drv->Create ("", 256, 256, nbands, dataType, NULL);
    if (!tile)
        return NULL;

    ds = new TMSDataset (fn);
    if (!ds->WriteNew (xsize, ysize, tile, parlist)) {
        delete tile;
        delete ds;
        return NULL;
    }
        
    delete tile;
    return ds;
}

static int LevelsCmpDetail (const void *_a, const void *_b);

#define VALIDATION_HEADER       const char *curr
#define DO_VAL(name)            curr = name;
#define FIND_ELEMENT(name)      element = CPLGetXMLNode (xml, name)
#define DO_ATTR(ename, aname)               \
    DO_VAL (ename " attribute '" aname "'");\
    attrName = aname;                       \
    must (GetXMLValue (curr, element, attrName, &val, CPLE_OpenFailed))
#define PARSE_DOUBLE(d) ParseDouble (curr, val, d, CPLE_OpenFailed)
#define PARSE_LONG(d)   ParseLong (curr, val, d, CPLE_OpenFailed)
#define IN_RANGE_DOUBLE(v,min,max)  InRangeDouble (curr, v, min, max, CPLE_OpenFailed)
#define IN_RANGE_LONG(v,min,max)    InRangeLong (curr, v, min, max, CPLE_OpenFailed)

bool TMSDataset::InitFromXML (CPLXMLNode *xml)
{
    VALIDATION_HEADER;
    CPLXMLNode *element;
    const char *val, *attrName, *p;
    double d;
    long l;

    /* TileMap dimensions. */

    double minx, miny, maxx, maxy;
    FIND_ELEMENT ("BoundingBox");
    DO_ATTR ("BoundingBox", "minx"); must (PARSE_DOUBLE (&minx));
    DO_ATTR ("BoundingBox", "maxx"); must (PARSE_DOUBLE (&maxx));
    DO_ATTR ("BoundingBox", "miny"); must (PARSE_DOUBLE (&miny));
    DO_ATTR ("BoundingBox", "maxy"); must (PARSE_DOUBLE (&maxy));

    must (SetDimensions (minx, maxx, miny, maxy));

    /* Tile specification. */
    
    FIND_ELEMENT ("TileFormat"); 

    DO_ATTR ("TileFormat", "width"); 
    must (PARSE_LONG (&l) && IN_RANGE_LONG (l, 1, MAX_TILE_WIDTH)); 
    tileWidth = l;
    
    DO_ATTR ("TileFormat", "height");
    must (PARSE_LONG (&l) && IN_RANGE_LONG (l, 1, MAX_TILE_HEIGHT));
    tileHeight = l;

    DO_ATTR ("TileFormat", "extension");
    extension = CPLStrdup (val);
        
    /* Levels. */
     
    CPLXMLNode *tileSets = CPLGetXMLNode (xml, "TileSets");

    int allocked = 4;
    nLevels = 0;
    levels = ( Level * ) CPLMalloc (sizeof (Level) * allocked);    

    for (element = tileSets->psChild; element != NULL; element = element->psNext)
        if (element->eType == CXT_Element && EQUAL (element->pszValue, "TileSet")) {
            /* This check is to ensure that we don't allocate too big array. */
            DO_VAL ("Number of levels");
            must (IN_RANGE_LONG (nLevels + 1, 1, MAX_NUM_LEVELS));
            
            if (nLevels >= allocked) {
                allocked *= 2;
                levels = ( Level * ) CPLRealloc (levels, sizeof (Level) * allocked);
            }

            DO_ATTR ("TileSet", "href");
            p = CPLGetBasename (val);
            if (*p == '\0') {
                CPLError (CE_Failure, CPLE_OpenFailed,
                    "%s[%s] doesn't denote a directory.", curr, val);
                fail;
            }
            levels[nLevels].dirName = CPLStrdup (p);
            
            DO_ATTR ("TileSet", "units-per-pixel");
            must (PARSE_DOUBLE (&d));
            must (IN_RANGE_DOUBLE (d, MIN_UNITS_PPX, MAX_UNITS_PPX));
            levels[nLevels].unitsppx = d;
            
            DO_ATTR ("TileSet", "order");
            must (PARSE_LONG (&l) && IN_RANGE_LONG (l, 0, MAX_ORDER));
            levels[nLevels].order = l;
           
            nLevels++;
        }
    
    /* This ensures that we have at least one level. */
    DO_VAL ("Number of levels");
    must (IN_RANGE_LONG (nLevels, 1, MAX_NUM_LEVELS));

    must (CalculateLevels ());
    
    nRasterXSize = levels[0].width;
    nRasterYSize = levels[0].height; 
    
    must (CreateBandInfo ());
    must (CreateCache ());
    succes;
}

bool TMSDataset::WriteNew (int xsize, int ysize, GDALDataset *tile, char **parlist)
{    
    char str[50], *dir;
    const char *dir2;
    double u;

    must (CreateDir (basedir));
        
    nLevels = 1 + ( int ) (MAX (ceil (log2 (xsize / ( float ) 256)), 
                                ceil (log2 (ysize / ( float) 256))));
    levels = mkarray (Level, nLevels);
    u = 1.0;
    forall (i, nLevels) {
        Level *l = &levels[i];
                
        snprintf (str, 50, "%d", nLevels - i - 1);
        l->dirName = CPLStrdup (str);
        l->unitsppx = u;
        l->order = nLevels - i - 1;
        
        u *= 2.0;
    }
    
    must (SetDimensions (0.0, xsize, 0.0, ysize));
    tileWidth = 256;
    tileHeight = 256;
    extension = CPLStrdup ("png");
    must (CalculateLevels ());
    nRasterXSize = width;
    nRasterYSize = height;
        
    forall (i, nLevels) {
        Level *l = &levels[i];
        
        dir = CPLStrdup (CPLFormFilename (basedir, l->dirName, NULL));
        must (CreateDir (dir));
       
        forall (j, l->widthInBlocks) {
            snprintf (str, 50, "%d", j);
            dir2 = CPLFormFilename (dir, str, NULL);
            must (CreateDir (dir2));
            
            forall (k, l->heightInBlocks)
                WriteTile (tile, i, j, k);
        }
                
        CPLFree (dir);
    }

    xmlfn = CPLStrdup (CPLFormFilename (basedir, "tilemapresource", "xml"));
    
    must (WriteXML ());
    must (CreateBandInfo ());
    must (CreateCache ());
    succes;
}

bool TMSDataset::SetDimensions (double minx, double maxx, double miny, double maxy)
{
    VALIDATION_HEADER;
    double w, h;
    
    DO_VAL ("Maximum map width");
    w = floor (maxx - minx); 
    must (IN_RANGE_DOUBLE (w, 1.0, MAX_WIDTH));
    
    DO_VAL ("Maximum map height");
    h = floor (maxy - miny); 
    must (IN_RANGE_DOUBLE (h, 1.0, MAX_HEIGHT));
    
    width  = ( int ) w;
    height = ( int ) h;

    succes;
}    

bool TMSDataset::CalculateLevels (void)
{
    VALIDATION_HEADER;
    
    qsort (levels, nLevels, sizeof (Level), LevelsCmpDetail);
    
    forall (i, nLevels) {
        levels[i].width = width / levels[i].unitsppx; 
        levels[i].height = height / levels[i].unitsppx;
        
        long w = ceil (( double ) levels[i].width / ( double ) tileWidth);
        long h = ceil (( double ) levels[i].height / ( double ) tileHeight);
  
        DO_VAL ("Maximum number of tiles in dimension X");
        must (IN_RANGE_LONG (w, 1, MAX_NUM_TILES_WIDTH));
 
        DO_VAL ("Maximum number of tiles in dimension Y");
        must (IN_RANGE_LONG (h, 1, MAX_NUM_TILES_HEIGHT));
 
        levels[i].widthInBlocks = w;
        levels[i].heightInBlocks = h;
    }
    succes;
}    

bool TMSDataset::CreateBandInfo (void)
{
    VALIDATION_HEADER;
    long l;
    
    /* TODO: the driver should try other tiles if this one is unavailable. */
    GDALDataset *tile;
    
    if (!ReadTile (0, 0, 0, &tile)) {
        CPLError (CE_Failure, CPLE_OpenFailed, "Could not read the exemplary tile.");
        fail;
    }

    DO_VAL ("Exemplary tile's number of bands");
    l = tile->GetRasterCount (); must (IN_RANGE_LONG (l, 1, MAX_NUM_BANDS));
    nBands = l;
    
    bandInfo = mkarray (BandInfo, nBands);
    forall (i, nBands) {
        GDALRasterBand *rb = tile->GetRasterBand (i + 1);
        
        BandInfo *b = &bandInfo[i];
        
        b->dataType = rb->GetRasterDataType ();
        b->dataTypeSize = GDALGetDataTypeSize (bandInfo[i].dataType) / 8;
        b->tileSizeBytes = tileWidth * tileHeight * bandInfo[i].dataTypeSize;
        
        SetBand (i + 1, new TMSTopRasterBand (this, i));
    }

    delete tile;
    succes;
}    

bool TMSDataset::CreateCache (void)
{
    int w, h, tileSizeBytes;
    GDALDriver *drv;
    
    tileSizeBytes = 0;
    forall (i, nBands)
        tileSizeBytes += bandInfo[i].tileSizeBytes;
    
    numCachedTiles = MIN (MAX_CACHE_BYTES / tileSizeBytes, MAX_CACHED_TILES);
    if (numCachedTiles < MIN_CACHED_TILES) {
        CPLError (CE_Failure, CPLE_OpenFailed,
            "Driver must allocate at least %d cached tiles, but these would not "
            "fit into the allowed memory consumption %dB.", 
            MIN_CACHED_TILES, MAX_CACHE_BYTES);
        fail;
    }

    tiles = mkarray (Tile, numCachedTiles);
    forall (i, numCachedTiles)
        tiles[i].used = tiles[i].dirty = tiles[i].locked = false;

    cache = mkarray (int **, nLevels);
    forall (l, nLevels) {
        w = levels[l].widthInBlocks;
        h = levels[l].heightInBlocks;
        
        cache[l] = mkarray (int *, w);
        forall (x, w) {
            cache[l][x] = mkarray (int, h);
            forall (y, h) cache[l][x][y] = -1;
        }
    }
    
    pixbuf = mkarray (GByte *, nBands);
    forall (i, nBands)
    	pixbuf[i] = mkarray (GByte, numCachedTiles * bandInfo[i].tileSizeBytes);

    drv = GetDriverByName ("MEM");
    outTile = drv->Create ("", tileWidth, tileHeight, nBands, bandInfo[0].dataType, NULL);

    succes;
}    

static char xmlTemplateStart[] = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<TileMap version=\"1.0.0\" tilemapservice=\"http://tms.osgeo.org/1.0.0\">\n"
"  <Title>%s</Title>\n"
"  <Abstract></Abstract>\n"
"  <SRS>%s</SRS>\n"
"  <BoundingBox minx=\"%.20lf\" miny=\"%.20lf\" maxx=\"%.20lf\" maxy=\"%.20lf\"/>\n"
"  <Origin x=\"%.20lf\" y=\"%.20lf\"/>\n"
"  <TileFormat width=\"%d\" height=\"%d\" mime-type=\"image/%s\" extension=\"%s\"/>\n"
"  <TileSets profile=\"%s\">\n";

static char xmlTemplateTileSet[] = 
    "    <TileSet href=\"%s%s\" units-per-pixel=\"%.20f\" order=\"%d\"/>\n"; 
    
static char xmlTemplateEnd[] = "</TileSets>\n</TileMap>\n";

bool TMSDataset::WriteXML (void)
{
    FILE *fd;
    double north, south, east, west;

    must (OpenFileWrite (xmlfn, &fd));
    
    if (haveGeoTrans)
        GetNSEW (geotrans, &north, &south, &east, &west);
    else {
        west  = 0.0;
        south = 0.0;
        north = nRasterXSize;
        east  = nRasterYSize;
    }

    VSIFPrintf (fd, xmlTemplateStart,
        "title", "srs", 
        west, south, east, north,
        west, south, 
        256, 256, //tilewidth, tileheight, extension, extension, profile);
        "png", "png", "profile");

    forall (i, nLevels) {
        Level *l = &levels[nLevels - i - 1];
        
        VSIFPrintf (fd, xmlTemplateTileSet,
            "", l->dirName, l->unitsppx, l->order);
    }

    VSIFPuts (xmlTemplateEnd, fd);
    VSIFClose (fd);
    succes;
}

bool TMSDataset::GetTileBand (int level, int xoff, int yoff, int nband, 
                              GByte **result)
{
    assert_valid_coordinates (this, level, xoff, yoff);
    assert_valid_band (this, nband);
    
    int id = cache[level][xoff][yoff];
    
    if (id == -1) {
        forall (i, numCachedTiles) if (!tiles[i].used) {
            id = i;
            break;
        }
        if (id == -1) {
            /* TODO: find a better way to find the least used tile. */
            forall (i, numCachedTiles) if (!tiles[i].locked) {
                id = i;
                break;
            }
        }
        assert (id != -1);
        
        if (tiles[id].used) {
            FlushTile (id);
            cache[tiles[id].level][tiles[id].xoff][tiles[id].yoff] = -1;
        }

        tiles[id].used  = true;
        tiles[id].dirty = false;
        tiles[id].level = level;
        tiles[id].xoff  = xoff;
        tiles[id].yoff  = yoff;
       
        CPLDebug ("TMS", "Reading tile %d [%d ; %d]", level, xoff, yoff);
        
        GDALDataset *p;
        must (ReadTile (level, xoff, yoff, &p));
        
        forall (i, nBands) {
            GDALRasterBand *rb = p->GetRasterBand (i + 1);
            rb->RasterIO (GF_Read, 
                0, 0, tileWidth, tileHeight,
                pixbuf[i] + (id * bandInfo[i].tileSizeBytes),
                tileWidth, tileHeight, bandInfo[i].dataType, 0, 0);
        }
        
        delete p;
        cache[level][xoff][yoff] = id;
    }
    
    assert (!tiles[id].locked);
    tiles[id].locked = true;
    
    *result = pixbuf[nband - 1] + (id * bandInfo[nband - 1].tileSizeBytes);
    succes;
}

void TMSDataset::ReleaseTile (int level, int xoff, int yoff, bool dirty)
{
    assert_valid_coordinates (this, level, xoff, yoff);
    int id = cache[level][xoff][yoff];
    assert (id != -1);

    tiles[id].locked = false;
    tiles[id].dirty = tiles[id].dirty || dirty;
}

void TMSDataset::FlushCache (void)
{
    GDALDataset::FlushCache ();
    forall (i, numCachedTiles) if (tiles[i].used) FlushTile (i);
}

void TMSDataset::FlushTile (int id)
{
    int b[1];
    
    assert (tiles[id].used);
    
    if (tiles[id].dirty) {
        forall (i, nBands) {
            b[0] = i;
            
            outTile->RasterIO (GF_Write,
                0, 0, tileWidth, tileHeight,
                pixbuf[i] + (id * bandInfo[i].tileSizeBytes),
                tileWidth, tileHeight, bandInfo[i].dataType, 
                1, b, 0, 0, 0);
        }
            
        WriteTile (outTile, tiles[id].level, tiles[id].xoff, tiles[id].yoff);
        tiles[id].dirty = false;
    }
}

void TMSDataset::GetNSEW (double *vect, double *north, double *south, double *east, double *west)
{
    *north = vect[3];
    *south = vect[3] + vect[5] * nRasterXSize;
    *east = vect[0] + vect[1] * nRasterYSize;
    *west = vect[0];
}

CPLErr TMSDataset::SetGeoTransform (double *vect)
{
    assert (vect != NULL);
    
    double oldWidth, fact, north, south, east, west;
    
    memcpy (geotrans, vect, sizeof (double) * 6);
    haveGeoTrans = true;
    
    oldWidth = width;
    GetNSEW (vect, &north, &south, &east, &west);
    SetDimensions (west, east, south, north);
    fact = width / oldWidth;
    
    forall (i, nLevels)
        levels[i].unitsppx *= fact;
  
    WriteXML ();
    
    return CE_None;
}

CPLErr TMSDataset::GetGeoTransform (double *vect)
{
    assert (vect != NULL);

    if (haveGeoTrans) {
        memcpy (vect, geotrans, sizeof (double) * 6);    
        return CE_None;
    } else
        return GDALDataset::GetGeoTransform (vect);
}


/*
    Disc IO.
*/

static GDALDriver *GetDriverByName (const char *name)
{
    GDALDriverManager *mgr = GetGDALDriverManager ();
    return mgr->GetDriverByName (name);
}

bool TMSDataset::ReadTile (int level, int xoff, int yoff, GDALDataset **result)
{
    assert_valid_coordinates (this, level, xoff, yoff);

    char *fn = GetTileFilename (level, xoff, yoff);
    GDALDataset *tile = ( GDALDataset * ) GDALOpen (fn, GA_ReadOnly);
    CPLFree (fn);    
    if (!tile) fail;
    *result = tile;
    succes;
}

void TMSDataset::WriteTile (GDALDataset *tile, int level, int xoff, int yoff)
{
    assert_valid_coordinates (this, level, xoff, yoff);
    
    GDALDriver *drv = GetDriverByName ("PNG");
    char *fn = GetTileFilename (level, xoff, yoff);
    GDALDataset *n = drv->CreateCopy (fn, tile, FALSE, NULL, NULL, NULL);
    delete n;
    CPLFree (fn);
}

char *TMSDataset::GetTileFilename (int level, int xoff, int yoff)
{
    assert_valid_coordinates (this, level, xoff, yoff);

    /* TODO: add correction with respect to `Origin'. */
    char tileName[50];
    snprintf (tileName, 50, "%d/%d", xoff, yoff);
    
    char *dirName = levels[level].dirName;
    char *rel = CPLStrdup (CPLFormFilename (dirName, tileName, extension));
    char *ret = CPLStrdup (CPLFormFilename (basedir, rel, NULL));
    CPLFree (rel);
    return ret;
}

void TMSDataset::DebugInfo (void)
{
    CPLDebug ("TMS", "Dataset info:");
    CPLDebug ("TMS", "    Raster size: %d x %d", nRasterXSize, nRasterYSize);
    CPLDebug ("TMS", "    Tile size: %d x %d", tileWidth, tileHeight);
    CPLDebug ("TMS", "    Extension: `%s'", extension);
    CPLDebug ("TMS", "    Basedir: `%s'", basedir);
    CPLDebug ("TMS", "    Number of cached tiles: %d", numCachedTiles);
    CPLDebug ("TMS", "    Number of bands: %d", nBands);
    CPLDebug ("TMS", "    Number of levels: %d", nLevels);
    forall (i, nLevels)
        CPLDebug ("TMS", "      Level %d: %d x %d,\t`%s',\t%f units-ppx", 
            i, levels[i].width, levels[i].height, 
            levels[i].dirName, levels[i].unitsppx);
}            

/****************************************************************************/

static int LevelsCmpDetail (const void *_a, const void *_b)
{
    assert (_a != NULL && _b != NULL);
    
    Level *a = ( Level * ) _a, *b = ( Level * ) _b;
    
    if (a->unitsppx < b->unitsppx) return -1;
    else if (a->unitsppx > b->unitsppx) return 1;
    else return 0;
}

void GDALRegister_TMS ()
{
    GDALDriver *poDriver;

    if (!GDALGetDriverByName ("TMS")) {
        poDriver = new GDALDriver ();
        
        poDriver->SetDescription ("TMS");
        poDriver->SetMetadataItem (GDAL_DMD_LONGNAME, "Tile Map Service");

        poDriver->pfnOpen = TMSDataset::Open;
        poDriver->pfnCreate = TMSDataset::Create;

        GetGDALDriverManager()->RegisterDriver (poDriver);
    }
}
