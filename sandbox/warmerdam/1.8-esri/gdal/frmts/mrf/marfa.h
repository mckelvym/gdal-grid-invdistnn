/******************************************************************************
 * $Id$
 *
 * Project:  Meta Raster Format
 * Purpose:  MRF structures
 * Author:   Lucian Plesea
 *
 ******************************************************************************
 * 
 *
 *
 ****************************************************************************/

#ifndef GDAL_FRMTS_MRF_MARFA_H_INCLUDED
#define GDAL_FRMTS_MRF_MARFA_H_INCLUDED

#include <gdal_pam.h>
#include <ogr_srs_api.h>
#include <ogr_spatialref.h>

// Utility, for printing values
#include <ostream>
#include <iostream>
#include <sstream>

// Force LERC to be included, normally off, detected in the makefile
// #define LERC

// These are a pain to maintain in sync.  They should be replaced with 
// some hash of objects solution, or C++11 initialized structures.  The externs reside in util.cpp
enum ILCompression { IL_PNG=0, IL_PPNG, IL_JPEG, IL_NONE , IL_ZLIB, IL_TIF, 
#if defined(LERC)
	IL_LERC, 
#endif
	IL_ERR_COMP} ;
enum ILOrder { IL_Interleaved=0, IL_Separate, IL_Sequential , IL_ERR_ORD} ;
extern char const **ILComp_Name;
extern char const **ILComp_Ext;
extern char const **ILOrder_Name;

class GDALMRFDataset;
class GDALMRFRasterBand;

typedef struct {
    char   *buffer;
    size_t size;
} buf_mgr;

// Size of an image, also used as a tile or pixel location
struct ILSize {
    GInt32 x,y,z,c,l;
    ILSize(const int x_=-1, const int y_=-1, const int z_=-1, 
        const int c_=-1, const int l_=-1) 

    { x=x_;y=y_;z=z_;c=c_;l=l_; }

    bool operator==(const ILSize& other)
    {
        return ((x==other.x) && (y==other.y) && (z==other.z) &&
            (c==other.c) && (l==other.l)); 
    }

    bool operator!=(const ILSize& other) { return !(*this==other); }
};

bool is_Endianess_Dependent(GDALDataType dt, ILCompression comp);

#define PPMW
#ifdef PPMW
void ppmWrite(const char *fname, const char *data, const ILSize &sz);
#endif

typedef struct {
    GIntBig offset;
    GIntBig size;
} ILIdx;

std::ostream& operator<<(std::ostream &out, const ILSize& sz);
std::ostream& operator<<(std::ostream &out, const ILIdx& t);

/**
 * Collects information pertaining to a single raster
 * This structure is being shallow copied, no pointers allowed
 *
 */

typedef struct {
    GIntBig dataoffset;
    GIntBig idxoffset;
    GInt32 quality;
    GInt32 pageSizeBytes;
    ILSize size;
    ILSize pagesize;
    ILSize pcount;
    ILCompression comp;
    ILOrder order;
    int nbo;
    int hasNoData;
    double NoDataValue;
    CPLString datfname;
    CPLString idxfname;
    GDALDataType dt;
} ILImage;

// Delarations of utility functions

/**
 *
 *\brief  Converters beween endianess, if needed.
 *  Call netXX() once before using the value and once again before sending it.
 * 
 * These might not be needed, use CPL_MSBWORD16(x), CPL_MSBWORD32(x) 
 * and CPL_MSBPTR64(*x) defined in cpl_port.h
 *
 */

static inline unsigned short int swab16(const unsigned short int val)
{
    return (val << 8) | (val >> 8);
}

static inline unsigned int swab32(unsigned int val)
{   
    return (unsigned int) (swab16((unsigned short int) val)) << 16
        | swab16( (unsigned short int) (val >> 16));
}

static inline unsigned long long int swab64(const unsigned long long int val)
{
    return (unsigned long long int) (swab32((unsigned int)val)) << 32 
        |  swab32( (unsigned int) (val >> 32));
}

// NET_ORDER is true if machine is BE, false otherwise
// Call netxx() if network (big) order is needed

#ifdef WORDS_BIGENDIAN
#define NET_ORDER true
// These could be macros, but for the side effects related to type
static inline unsigned short net16(const unsigned short x)
{
	return (x);
}
static inline unsigned int net32(const unsigned int x)
{
	return (x);
}
static inline unsigned long long net64(const unsigned long long x)
{
	return (x);
}
#else
#define NET_ORDER false
#define net16(x) swab16(x)
#define net32(x) swab32(x)
#define net64(x) swab64(x)
#endif

const char *CompName(ILCompression comp);
const char *OrderName(ILOrder val);
ILCompression CompToken(const char *, ILCompression def=IL_ERR_COMP);
ILOrder OrderToken(const char *, ILOrder def=IL_ERR_ORD);
GDALColorInterp BandInterp(int ,int);
CPLString getFname(CPLXMLNode *,const char *, const CPLString &, const char *);
CPLString getFname(const CPLString &, const char *);
double getXMLNum(CPLXMLNode *, const char *, double);
GIntBig IdxOffset(const ILSize &, const ILImage &);
double logb(double val, double base);
int IsPower(double value,double base);
CPLXMLNode *SearchXMLSiblings( CPLXMLNode *psRoot, const char *pszElement );
void XMLSetAttributeVal(CPLXMLNode *parent,const char* pszName,
    const double val, const char *frmt=NULL);
CPLXMLNode *XMLSetAttributeVal(CPLXMLNode *parent,
        const char*pszName,const ILSize &sz,const char *frmt=NULL);
GDALColorEntry GetXMLColorEntry(CPLXMLNode *p);
GDALColorEntry HSVSwap(const GDALColorEntry& cein);
GIntBig IdxSize(const ILImage &full, const int scale=0);

// checks that the file exists and is at least sz, if access is update it extends it
int CheckFileSize(const char *fname, GIntBig sz, GDALAccess eAccess);

// Number of pages of size psz needed to hold n elements
inline int pcount(const int n, const int sz) {
    return (n-1) / sz +1;
}

// Total page count for a given image, pos is in pages
inline int pcount(const ILSize &pages) {
    return pages.x*pages.y*pages.z*pages.c;
}

// Set up page count
inline void pcount(ILSize &pages, const ILSize &size, const ILSize &psz) {
    pages.x = pcount(size.x,psz.x);
    pages.y = pcount(size.y,psz.y);
    pages.c = pcount(size.c,psz.c);
    pages.z = pcount(size.z,psz.z);
    // since pages don't have overviews, use it to hold total number of pages
    pages.l = pcount(pages);
}

// Wrapper around the VISFile, remembers how the file was opened
typedef struct {
    VSILFILE *FP;
    GDALRWFlag acc;
} VF;

// Offset of index, pos is in pages
GIntBig IdxOffset(const ILSize &pos,const ILImage &img);

class GDALMRFDataset : public GDALPamDataset {
    friend class GDALMRFRasterBand;
    friend GDALMRFRasterBand *newMRFRasterBand(GDALMRFDataset *, const ILImage &, int, int level=0);

public:
    GDALMRFDataset();
    virtual ~GDALMRFDataset();

    static GDALDataset *Open(GDALOpenInfo *);
    static int Identify(GDALOpenInfo *);
    static GDALDataset *CreateCopy(const char *pszFilename, GDALDataset *poSrcDS,
            int bStrict, char **papszOptions, GDALProgressFunc pfnProgress,
            void *pProgressData);

    virtual const char *GetProjectionRef();
    virtual CPLErr SetProjection(const char *proj);

    virtual CPLErr GetGeoTransform(double *gt);
    virtual CPLErr SetGeoTransform(double *gt);

    virtual CPLErr AdviseRead( int nXOff, int nYOff, int nXSize, int nYSize,
	int nBufXSize, int nBufYSize, 
	GDALDataType eDT, 
	int nBandCount, int *panBandList,
	char **papszOptions );

    virtual char **GetFileList();

    void SetColorTable(GDALColorTable *pct) {poColorTable=pct;};
    const GDALColorTable *GetColorTable() {return poColorTable;};
    void SetNoDataValue(const char*);
    void SetMinValue(const char*);
    void SetMaxValue(const char*);
    void SetPBuffer(unsigned int sz);
    unsigned int GetPBufferSize() {return pbsize;};
    CPLErr SetVersion(int version);

    const CPLString GetFname() {return fname;};
    // Patches a region of all the next overview, argument counts are in blocks
    virtual CPLErr PatchOverview(int BlockX,int BlockY,int Width,int Height, 
        int srcLevel=0, int recursive=false);

protected:
    CPLErr LevelInit(const int l);
    CPLXMLNode *ReadConfig ();
    int WriteConfig(CPLXMLNode *);
    CPLErr Initialize(CPLXMLNode *);
    CPLErr CleanOverviews(void);
    // Add uniform scaled overlays, returns the size of the index file
    GIntBig AddOverviews(int scale);

    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
	void *, int, int, GDALDataType,
	int, int *, int, int, int );

    virtual CPLErr IBuildOverviews( const char*, int, int*, int, int*, 
	GDALProgressFunc, void* );

    // Write a tile, the infooffset is the relative position in the index file
    virtual CPLErr WriteTile(void *buff, GUIntBig infooffset, GUIntBig size=0);

    // For versioned MRFs, add a version
    CPLErr GDALMRFDataset::AddVersion();

    VSILFILE *IdxFP();
    VSILFILE *DataFP();
    GDALRWFlag IdxMode() { return ifp.acc; };
    GDALRWFlag DataMode() { return dfp.acc; };
    GDALDataset *GetSrcDS();

    /*
     *  There are two images defined to allow for morphing on use, in the future
     *  For example storing a multispectral image and opening it as RGB
     *
     *  Support for this feature is not implemented.
     *
     */

    // What the image really is
    ILImage full;
    // How we use it currently
    ILImage current;

    // MRF file name
    CPLString fname;

    // The source to be cached in this MRF
    CPLString source;
    int clonedSource; // Is it a cloned source

    int hasVersions; // Does it support versions
    int verCount;    // The last version
    GIntBig verIdxSize;// The size of each version index

    // Freeform sticky dataset options
    CPLString options;
    char **optlist;

    // If caching data, the parent dataset
    GDALDataset *poSrcDS;

    // Level picked, or -1 for native
    int level;

    // Child dataset, if picking a specific level
    GDALMRFDataset *cds;
    double scale;

    // A place to keep an uncompressed block, to keep from allocating it all the time
    unsigned int pbsize;
    void *pbuffer;

    ILSize tile; // ID of tile present in buffer
    // Holds bits, to be used in pixel interleaved (up to 64 bands)
    GIntBig bdirty;

    // GeoTransform support
    double GeoTransform[6];
    int bGeoTransformValid;

    char *pszProjection;
    int bProjectionValid;

    GDALColorTable *poColorTable;
    int Quality;

    // Default data file and access
    VF dfp;
    // Default index file and access
    VF ifp;

    std::vector<double> vNoData,vMin,vMax;
};

class GDALMRFRasterBand : public GDALPamRasterBand {
    friend class GDALMRFDataset;
public:
    GDALMRFRasterBand(GDALMRFDataset *, const ILImage &, int, int);
    virtual ~GDALMRFRasterBand();
    virtual CPLErr IReadBlock(int xblk, int yblk, void *buffer);
    virtual CPLErr IWriteBlock(int xblk, int yblk, void *buffer);
    virtual GDALColorTable *GetColorTable() {return poDS->poColorTable;}
    virtual GDALColorInterp GetColorInterpretation();
    virtual double  GetNoDataValue(int * pbSuccess);
    virtual double  GetMinimum(int *);
    virtual double  GetMaximum(int *);

    // MRF specific, fetch is from a remote source
    CPLErr FetchBlock(int xblk, int yblk, void *buffer = NULL);
    // Fetch a block from a cloned MRF
    CPLErr FetchClonedBlock(int xblk, int yblk, void *buffer = NULL);

    // Block not stored on disk
    CPLErr FillBlock(void *buffer);

    // de-interlace a buffer in pixel blocks
    CPLErr RB(int xblk, int yblk, buf_mgr src, void *buffer);

    const char *GetOptionValue(const char *opt, const char *def);
    const ILImage *GetImage();
    void SetAccess( GDALAccess eA) { 
        eAccess=eA; 
    }

protected:
    // Pointer to my DS (of GDALMRFDataset type)
    GDALMRFDataset *poDS;
    // 0 based
    GInt32 m_band;
    // Level of this band
    GInt32 m_l;
    // The info about the current image, to enable R-sets
    ILImage img;
//    VSILFILE *dfp;
//    VSILFILE *ifp;
    VF dfp;
    VF ifp;
    std::vector<GDALMRFRasterBand *> overviews;
    int overview;

    VSILFILE *IdxFP() {
        if (ifp.FP != NULL) 
	    return ifp.FP;
    // Return and keep track of the parent one
	ifp.FP = poDS->IdxFP();
	ifp.acc = poDS->IdxMode();
        return ifp.FP;
    }

    VSILFILE *DataFP() {
        if (dfp.FP != NULL) 
            return dfp.FP;
	dfp.FP  = poDS->DataFP();
	dfp.acc = poDS->DataMode();
        return dfp.FP;
    }

    GDALRWFlag IdxMode() { return ifp.acc; };
    GDALRWFlag DataMode() { return dfp.acc; };

    // How many bytes are in a page
    GUInt32 pageSizeBytes() { 
        return poDS->current.pageSizeBytes;
    }

    // How many bytes are in a band block (not a page, a single band block)
    // Easiest is to calculate it from the pageSizeBytes
    GUInt32 blockSizeBytes() { 
        return poDS->current.pageSizeBytes / poDS->current.pagesize.c;
    }

    // Compresion and decompression functions.  To be overwritten by specific implementations
    virtual CPLErr Compress(buf_mgr &dst, buf_mgr &src, const ILImage &) =0;
    virtual CPLErr Decompress(buf_mgr &dst, buf_mgr &src) =0;
    
    // Read the index record itself, can be overwritten
    virtual CPLErr ReadTileIdx(const ILSize &, ILIdx &, GIntBig bias = 0);

    GIntBig bandbit() {
        return ((GIntBig)1) << m_band; 
    }
    GIntBig bandbit(int b) {
        return ((GIntBig)1) << b; 
    }
    GIntBig AllBandMask() { 
        return (((GIntBig)1) << poDS->nBands)-1; 
    }

    // Overview Support
    // Inherited from GDALRasterBand
    // These are called only in the base level RasterBand
    virtual int GetOverviewCount() {return static_cast<int>(overviews.size());}
    virtual GDALRasterBand *GetOverview(int n) {
        if (n<(int)overviews.size()) return overviews[n];
        return 0;
    }
    void AddOverview(GDALMRFRasterBand *b) {
        overviews.push_back(b);
    }
};

/**
 * Each type of compression needs to define at least two methods, a compress and a
 * decompress, which take as arguments a dest and a source buffer, plus an image structure
 * that holds the information about the compression type.
 * Filtering is needed, probably in the form of pack and unpack functions
 * 
 */

class PNG_Band : public GDALMRFRasterBand {
    friend class GDALMRFDataset;
public:
    PNG_Band(GDALMRFDataset *pDS, const ILImage &image, int b, int level);
    virtual ~PNG_Band();
protected:
    virtual CPLErr Decompress(buf_mgr &dst, buf_mgr &src);
    virtual CPLErr Compress(buf_mgr &dst, buf_mgr &src,const ILImage &img);

    CPLErr CompressPNG(buf_mgr &dst, buf_mgr &src);
    CPLErr DecompressPNG(buf_mgr &dst, buf_mgr &src);
    void *PNGColors;
    void *PNGAlpha;
    int PalSize, TransSize;
};

class JPEG_Band : public GDALMRFRasterBand {
    friend class GDALMRFDataset;
public:
    JPEG_Band(GDALMRFDataset *pDS, const ILImage &image, int b, int level) : 
        GDALMRFRasterBand(pDS,image,b,int(level)) {};
    virtual ~JPEG_Band() {};
protected:
    virtual CPLErr Decompress(buf_mgr &dst, buf_mgr &src);
    virtual CPLErr Compress(buf_mgr &dst, buf_mgr &src,const ILImage &img);
};


class Raw_Band : public GDALMRFRasterBand {
    friend class GDALMRFDataset;
public:
    Raw_Band(GDALMRFDataset *pDS, const ILImage &image, int b, int level) : 
        GDALMRFRasterBand(pDS,image,b,int(level)) {};
    virtual ~Raw_Band() {};
protected:
    virtual CPLErr Decompress(buf_mgr &dst, buf_mgr &src);
    virtual CPLErr Compress(buf_mgr &dst, buf_mgr &src,const ILImage &img);
};


class ZLIB_Band : public GDALMRFRasterBand {
    friend class GDALMRFDataset;
public:
    ZLIB_Band(GDALMRFDataset *pDS, const ILImage &image, int b, int level) : 
        GDALMRFRasterBand(pDS,image,b,int(level)) {};
    virtual ~ZLIB_Band() {};
protected:
    virtual CPLErr Decompress(buf_mgr &dst, buf_mgr &src);
    virtual CPLErr Compress(buf_mgr &dst, buf_mgr &src,const ILImage &img);
};


class TIF_Band : public GDALMRFRasterBand {
    friend class GDALMRFDataset;
public:
    TIF_Band(GDALMRFDataset *pDS, const ILImage &image, int b, int level);
    virtual ~TIF_Band();
protected:
    virtual CPLErr Decompress(buf_mgr &dst, buf_mgr &src);
    virtual CPLErr Compress(buf_mgr &dst, buf_mgr &src,const ILImage &img) ;

    // Create options for TIF pages
    char **papszOptions;
};

#if defined(LERC)
class LERC_Band : public GDALMRFRasterBand {
    friend class GDALMRFDataset;
public:
    LERC_Band(GDALMRFDataset *pDS, const ILImage &image, int b, int level);
    virtual ~LERC_Band();
protected:
    virtual CPLErr Decompress(buf_mgr &dst, buf_mgr &src);
    virtual CPLErr Compress(buf_mgr &dst, buf_mgr &src,const ILImage &img);
    double precision;
};
#endif

/*\brief band for level mrf
 *
 * Stand alone definition of a derived band, used in access to a specific level in an MRF
 *
 */
class GDALMRFLRasterBand : public GDALPamRasterBand {
public:
    GDALMRFLRasterBand(GDALMRFRasterBand *b) { 
        pBand=b ;
        eDataType=b->GetRasterDataType();
        b->GetBlockSize(&nBlockXSize,&nBlockYSize);
        eAccess=b->GetAccess();
        nRasterXSize=b->GetXSize();
        nRasterYSize=b->GetYSize();
    }
    virtual CPLErr IReadBlock(int xblk, int yblk, void *buffer) {
        return pBand->IReadBlock(xblk,yblk,buffer);
    }
    virtual CPLErr IWriteBlock(int xblk, int yblk, void *buffer) {
        return pBand->IWriteBlock(xblk,yblk,buffer);
    }
    virtual GDALColorTable *GetColorTable() {
        return pBand->GetColorTable();
    }
    virtual GDALColorInterp GetColorInterpretation() {
        return pBand->GetColorInterpretation();
    }
    virtual double  GetNoDataValue(int * pbSuccess) {
        return pBand->GetNoDataValue(pbSuccess);
    }
    virtual double  GetMinimum(int *b) {
        return pBand->GetMinimum(b);
    }
    virtual double  GetMaximum(int *b) {
        return pBand->GetMaximum(b);
    }
    
protected:
    virtual int GetOverviewCount() {return 0;}
    virtual GDALRasterBand *GetOverview(int n) {return 0;}

    GDALMRFRasterBand *pBand;
};

#endif // GDAL_FRMTS_MRF_MARFA_H_INCLUDED

