/******************************************************************************
 * $Id$
 *
 * Project:  Microsoft Windows Bitmap
 * Purpose:  Read/write MS Windows Device Independent Bitmap (DIB) files
 *           and OS/2 Presentation Manager bitmaps v. 1.x and v. 2.x
 * Author:   Andrey Kiselev, dron@remotesensing.org
 *
 ******************************************************************************
 * Copyright (c) 2002, Andrey Kiselev <dron@remotesensing.org>
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.32  2004/09/28 14:18:06  fwarmerdam
 * A few more additions to improve error handling.  The "create" write
 * error checks don't work (at least on Linux) because of the buffering.
 *
 * Revision 1.31  2004/09/28 14:05:21  fwarmerdam
 * Try to strategically check the success of VSIFWriteL() calls in
 * the Create() method as per:
 * http://bugzilla.remotesensing.org/show_bug.cgi?id=619
 *
 * Revision 1.30  2004/06/08 15:53:08  dron
 * Even more fixes.
 *
 * Revision 1.29  2004/06/08 13:38:16  dron
 * Few minor fixes.
 *
 * Revision 1.28  2004/01/06 10:56:54  dron
 * Few optimizations in IReadBlock().
 *
 * Revision 1.27  2003/11/07 13:12:38  dron
 * Handle 32-bit BMPs properly.
 *
 * Revision 1.26  2003/10/24 20:31:53  warmerda
 * Added extension metadata.
 *
 * Revision 1.25  2003/09/22 17:17:50  warmerda
 * added support for .bpw and .bmpw worldfiles
 *
 * Revision 1.24  2003/09/11 19:55:15  warmerda
 * avoid warnings
 *
 * Revision 1.23  2003/06/17 17:59:03  dron
 * Do not close GDALOpenInfo::fp handler before reopening.
 *
 * Revision 1.22  2003/05/30 05:26:56  dron
 * Fixed problem with scanline numbering.
 *
 * Revision 1.21  2003/05/29 14:58:01  dron
 * Workaround for files with extra bytes at the end of data stream.
 *
 * Revision 1.20  2003/05/20 08:59:10  dron
 * BMPDataset::IRasterIO() method added.
 *
 * Revision 1.19  2003/05/01 17:40:43  dron
 * Report colour interpretation GCI_PaletteIndex for 1-bit images.
 *
 * Revision 1.18  2003/03/27 15:51:06  dron
 * Improvements in update state handling in IReadBlock().
 *
 * Revision 1.17  2003/03/27 13:24:53  dron
 * Fixes for large file support.
 *
 * Revision 1.16  2003/02/15 14:23:09  dron
 * Fix in GetGeoTransform().
 *
 * Revision 1.15  2003/02/03 18:40:15  dron
 * Type of input data checked in Create() method now.
 *
 * Revision 1.14  2002/12/15 15:24:41  dron
 * Typos fixed.
 *
 * Revision 1.13  2002/12/15 15:13:33  dron
 * BMP structures extended.
 *
 * Revision 1.12  2002/12/13 14:15:50  dron
 * IWriteBlock() fixed, CreateCopy() removed, added RLE4 decoding and OS/2 BMP
 * support.
 *
 * Revision 1.11  2002/12/11 22:09:13  warmerda
 * Re-enable createcopy.
 *
 * Revision 1.10  2002/12/11 22:08:20  warmerda
 * fixed multiband support for writing ... open wit wb+ in create
 *
 * Revision 1.9  2002/12/11 16:13:04  dron
 * Support for 16-bit RGB images (untested).
 *
 * Revision 1.8  2002/12/10 22:12:59  dron
 * RLE8 decoding added.
 *
 * Revision 1.7  2002/12/09 19:31:44  dron
 * Switched to CPL_LSBWORD32 macro.
 *
 * Revision 1.6  2002/12/07 15:20:23  dron
 * SetColorTable() added. Create() really works now.
 *
 * Revision 1.5  2002/12/06 20:50:13  warmerda
 * fixed type warning
 *
 * Revision 1.4  2002/12/06 18:37:05  dron
 * Create() method added, 1- and 4-bpp images readed now.
 *
 * Revision 1.3  2002/12/05 19:25:35  dron
 * Preliminary CreateCopy() function.
 *
 * Revision 1.2  2002/12/04 18:37:49  dron
 * Support 32-bit, 24-bit True Color images and 8-bit pseudocolor ones.
 *
 * Revision 1.1  2002/12/03 19:04:18  dron
 * Initial version.
 *
 */

#include "gdal_priv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

CPL_C_START
void    GDALRegister_BMP(void);
CPL_C_END

enum BMPType
{
    BMPT_WIN4,      // BMP used in Windows 3.0/NT 3.51/95
    BMPT_WIN5,      // BMP used in Windows NT 4.0/98/Me/2000/XP
    BMPT_OS21,      // BMP used in OS/2 PM 1.x
    BMPT_OS22       // BMP used in OS/2 PM 2.x
};

// Bitmap file consists of a BMPFileHeader structure followed by a
// BMPInfoHeader structure. An array of BMPColorEntry structures (also called
// a colour table) follows the bitmap information header structure. The colour
// table is followed by a second array of indexes into the colour table (the
// actual bitmap data). Data may be comressed, for 4-bpp and 8-bpp used RLE
// compression.
//
// +---------------------+
// | BMPFileHeader       |
// +---------------------+
// | BMPInfoHeader       |
// +---------------------+
// | BMPColorEntry array |
// +---------------------+
// | Colour-index array  |
// +---------------------+
//
// All numbers stored in Intel order with least significant byte first.

enum BMPComprMethod
{
    BMPC_RGB = 0L,              // Uncompressed
    BMPC_RLE8 = 1L,             // RLE for 8 bpp images
    BMPC_RLE4 = 2L,             // RLE for 4 bpp images
    BMPC_BITFIELDS = 3L,        // Bitmap is not compressed and the colour table
                                // consists of three DWORD color masks that specify
                                // the red, green, and blue components of each pixel.
                                // This is valid when used with 16- and 32-bpp bitmaps.
    BMPC_JPEG = 4L,             // Indicates that the image is a JPEG image.
    BMPC_PNG = 5L               // Indicates that the image is a PNG image.
};

enum BMPLCSType                 // Type of logical color space.
{
    BMPLT_CALIBRATED_RGB = 0,   // This value indicates that endpoints and gamma
                                // values are given in the appropriate fields.
    BMPLT_DEVICE_RGB = 1,
    BMPLT_DEVICE_CMYK = 2,
};

typedef struct
{
    GInt32      iCIEX;
    GInt32      iCIEY;
    GInt32      iCIEZ;
} BMPCIEXYZ;

typedef struct                  // This structure contains the x, y, and z
{                               // coordinates of the three colors that correspond
    BMPCIEXYZ   iCIERed;        // to the red, green, and blue endpoints for
    BMPCIEXYZ   iCIEGreen;      // a specified logical color space.
    BMPCIEXYZ   iCIEBlue;
} BMPCIEXYZTriple;

typedef struct
{
    GByte       bType[2];       // Signature "BM"
    GUInt32     iSize;          // Size in bytes of the bitmap file. Should always
                                // be ignored while reading because of error
                                // in Windows 3.0 SDK's description of this field
    GUInt16     iReserved1;     // Reserved, set as 0
    GUInt16     iReserved2;     // Reserved, set as 0
    GUInt32     iOffBits;       // Offset of the image from file start in bytes
} BMPFileHeader;

// File header size in bytes:
const int       BFH_SIZE = 14;

typedef struct
{
    GUInt32     iSize;          // Size of BMPInfoHeader structure in bytes.
                                // Should be used to determine start of the
                                // colour table
    GInt32      iWidth;         // Image width
    GInt32      iHeight;        // Image height. If positive, image has bottom left
                                // origin, if negative --- top left.
    GUInt16     iPlanes;        // Number of image planes (must be set to 1)
    GUInt16     iBitCount;      // Number of bits per pixel (1, 4, 8, 16, 24 or 32).
                                // If 0 then the number of bits per pixel is
                                // specified or is implied by the JPEG or PNG format.
    BMPComprMethod iCompression; // Compression method
    GUInt32     iSizeImage;     // Size of uncomressed image in bytes. May be 0
                                // for BMPC_RGB bitmaps. If iCompression is BI_JPEG
                                // or BI_PNG, iSizeImage indicates the size
                                // of the JPEG or PNG image buffer.
    GInt32      iXPelsPerMeter; // X resolution, pixels per meter (0 if not used)
    GInt32      iYPelsPerMeter; // Y resolution, pixels per meter (0 if not used)
    GUInt32     iClrUsed;       // Size of colour table. If 0, iBitCount should
                                // be used to calculate this value (1<<iBitCount)
    GUInt32     iClrImportant;  // Number of important colours. If 0, all
                                // colours are required

    // Fields above should be used for bitmaps, compatible with Windows NT 3.51
    // and earlier. Windows 98/Me, Windows 2000/XP introduces additional fields:

    GUInt32     iRedMask;       // Colour mask that specifies the red component
                                // of each pixel, valid only if iCompression
                                // is set to BI_BITFIELDS.
    GUInt32     iGreenMask;     // The same for green component
    GUInt32     iBlueMask;      // The same for blue component
    GUInt32     iAlphaMask;     // Colour mask that specifies the alpha
                                // component of each pixel.
    BMPLCSType  iCSType;        // Colour space of the DIB.
    BMPCIEXYZTriple sEndpoints; // This member is ignored unless the iCSType member
                                // specifies BMPLT_CALIBRATED_RGB.
    GUInt32     iGammaRed;      // Toned response curve for red. This member
                                // is ignored unless color values are calibrated
                                // RGB values and iCSType is set to
                                // BMPLT_CALIBRATED_RGB. Specified in 16^16 format.
    GUInt32     iGammaGreen;    // Toned response curve for green.
    GUInt32     iGammaBlue;     // Toned response curve for blue.
} BMPInfoHeader;

// Info header size in bytes:
const unsigned int  BIH_WIN4SIZE = 40; // for BMPT_WIN4
const unsigned int  BIH_WIN5SIZE = 57; // for BMPT_WIN5
const unsigned int  BIH_OS21SIZE = 12; // for BMPT_OS21
const unsigned int  BIH_OS22SIZE = 64; // for BMPT_OS22

// We will use plain byte array instead of this structure, but declaration
// provided for reference
typedef struct
{
    GByte       bBlue;
    GByte       bGreen;
    GByte       bRed;
    GByte       bReserved;      // Must be 0
} BMPColorEntry;

/************************************************************************/
/* ==================================================================== */
/*                              BMPDataset                              */
/* ==================================================================== */
/************************************************************************/

class BMPDataset : public GDALDataset
{
    friend class BMPRasterBand;
    friend class BMPComprRasterBand;

    BMPFileHeader       sFileHeader;
    BMPInfoHeader       sInfoHeader;
    int                 nColorTableSize, nColorElems;
    GByte               *pabyColorTable;
    GDALColorTable      *poColorTable;
    double              adfGeoTransform[6];
    int                 bGeoTransformValid;
    char                *pszProjection;

    const char          *pszFilename;
    FILE                *fp;

  protected:
    virtual CPLErr      IRasterIO( GDALRWFlag, int, int, int, int,
                                   void *, int, int, GDALDataType,
                                   int, int *, int, int, int );

  public:
                BMPDataset();
                ~BMPDataset();

    static GDALDataset  *Open( GDALOpenInfo * );
    static GDALDataset  *Create( const char * pszFilename,
                                int nXSize, int nYSize, int nBands,
                                GDALDataType eType, char ** papszParmList );
    virtual void        FlushCache( void );

    CPLErr              GetGeoTransform( double * padfTransform );
    virtual CPLErr      SetGeoTransform( double * );
    const char          *GetProjectionRef();
};

/************************************************************************/
/* ==================================================================== */
/*                            BMPRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class BMPRasterBand : public GDALRasterBand
{
    friend class BMPDataset;

  protected:

    GUInt32         nScanSize;
    unsigned int    iBytesPerPixel;
    GByte           *pabyScan;

  public:

                BMPRasterBand( BMPDataset *, int );
                ~BMPRasterBand();

    virtual CPLErr          IReadBlock( int, int, void * );
    virtual CPLErr          IWriteBlock( int, int, void * );
    virtual GDALColorInterp GetColorInterpretation();
    virtual GDALColorTable  *GetColorTable();
    CPLErr                  SetColorTable( GDALColorTable * );
};

/************************************************************************/
/*                           BMPRasterBand()                            */
/************************************************************************/

BMPRasterBand::BMPRasterBand( BMPDataset *poDS, int nBand )
{
    this->poDS = poDS;
    this->nBand = nBand;
    eDataType = GDT_Byte;
    iBytesPerPixel = poDS->sInfoHeader.iBitCount / 8;

    // We will read one scanline per time. Scanlines in BMP aligned at 4-byte
    // boundary
    nBlockXSize = poDS->GetRasterXSize();
    nScanSize =
        ((poDS->GetRasterXSize() * poDS->sInfoHeader.iBitCount + 31) & ~31) / 8;
    nBlockYSize = 1;

    CPLDebug( "BMP",
              "Band %d: set nBlockXSize=%d, nBlockYSize=%d, nScanSize=%d",
              nBand, nBlockXSize, nBlockYSize, nScanSize );

    pabyScan = (GByte *) CPLMalloc( nScanSize );
}

/************************************************************************/
/*                           ~BMPRasterBand()                           */
/************************************************************************/

BMPRasterBand::~BMPRasterBand()
{
    CPLFree( pabyScan );
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr BMPRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )
{
    BMPDataset  *poGDS = (BMPDataset *) poDS;
    GUInt32     iScanOffset;
    int         i;

    if ( poGDS->sInfoHeader.iHeight > 0 )
        iScanOffset = poGDS->sFileHeader.iOffBits +
            ( poGDS->GetRasterYSize() - nBlockYOff - 1 ) * nScanSize;
    else
        iScanOffset = poGDS->sFileHeader.iOffBits + nBlockYOff * nScanSize;

    if ( VSIFSeekL( poGDS->fp, iScanOffset, SEEK_SET ) < 0 )
    {
        // XXX: We will not report error here, because file just may be
	// in update state and data for this block will be available later
        if( poGDS->eAccess == GA_Update )
        {
            memset( pImage, 0, nBlockXSize );
            return CE_None;
        }
        else
        {
            CPLError( CE_Failure, CPLE_FileIO,
                      "Can't seek to offset %ld in input file to read data.",
                      iScanOffset );
            return CE_Failure;
        }
    }
    if ( VSIFReadL( pabyScan, 1, nScanSize, poGDS->fp ) < nScanSize )
    {
        // XXX
        if( poGDS->eAccess == GA_Update )
        {
            memset( pImage, 0, nBlockXSize );
            return CE_None;
        }
        else
        {
            CPLError( CE_Failure, CPLE_FileIO,
                      "Can't read from offset %ld in input file.", iScanOffset );
            return CE_Failure;
        }
    }

    if ( poGDS->sInfoHeader.iBitCount == 24 ||
         poGDS->sInfoHeader.iBitCount == 32 )
    {
        GByte *pabyTemp = pabyScan + 3 - nBand;

        for ( i = 0; i < nBlockXSize; i++ )
        {
            // Colour triplets in BMP file organized in reverse order:
            // blue, green, red. When we have 32-bit BMP the forth byte
            // in quadriplet should be discarded as it has no meaning.
            // That is why we always use 3 byte count in the following
            // pabyTemp index.
            ((GByte *) pImage)[i] = *pabyTemp;
            pabyTemp += iBytesPerPixel;
        }
    }
    else if ( poGDS->sInfoHeader.iBitCount == 8 )
    {
        memcpy( pImage, pabyScan, nBlockXSize );
    }
    else if ( poGDS->sInfoHeader.iBitCount == 16 )
    {
        for ( i = 0; i < nBlockXSize; i++ )
        {
            switch ( nBand )
            {
                case 1:
                ((GByte *) pImage)[i] = pabyScan[i + 1] & 0x1F;
                break;
                case 2:
                ((GByte *) pImage)[i] = ((pabyScan[i] & 0x03) << 3) |
                                        ((pabyScan[i + 1] & 0xE0) >> 5);
                break;
                case 3:
                ((GByte *) pImage)[i] = (pabyScan[i] & 0x7c) >> 2;
                break;
                default:
                break;
            }
        }
    }
    else if ( poGDS->sInfoHeader.iBitCount == 4 )
    {
        GByte *pabyTemp = pabyScan;

        for ( i = 0; i < nBlockXSize; i++ )
        {
            // Most significant part of the byte represents leftmost pixel
            if ( i & 0x01 )
                ((GByte *) pImage)[i] = *pabyTemp++ & 0x0F;
            else
                ((GByte *) pImage)[i] = (*pabyTemp & 0xF0) >> 4;
        }
    }
    else if ( poGDS->sInfoHeader.iBitCount == 1 )
    {
        GByte *pabyTemp = pabyScan;

        for ( i = 0; i < nBlockXSize; i++ )
        {
            switch ( i & 0x7 )
            {
                case 0:
                ((GByte *) pImage)[i] = (*pabyTemp & 0x80) >> 7;
                break;
                case 1:
                ((GByte *) pImage)[i] = (*pabyTemp & 0x40) >> 6;
                break;
                case 2:
                ((GByte *) pImage)[i] = (*pabyTemp & 0x20) >> 5;
                break;
                case 3:
                ((GByte *) pImage)[i] = (*pabyTemp & 0x10) >> 4;
                break;
                case 4:
                ((GByte *) pImage)[i] = (*pabyTemp & 0x08) >> 3;
                break;
                case 5:
                ((GByte *) pImage)[i] = (*pabyTemp & 0x04) >> 2;
                break;
                case 6:
                ((GByte *) pImage)[i] = (*pabyTemp & 0x02) >> 1;
                break;
                case 7:
                ((GByte *) pImage)[i] = *pabyTemp++ & 0x01;
                break;
                default:
                break;
            }
        }
    }

    return CE_None;
}

/************************************************************************/
/*                            IWriteBlock()                             */
/************************************************************************/

CPLErr BMPRasterBand::IWriteBlock( int nBlockXOff, int nBlockYOff,
                                   void * pImage )
{
    BMPDataset  *poGDS = (BMPDataset *)poDS;
    int         iInPixel, iOutPixel;
    GUInt32     iScanOffset;

    CPLAssert( poGDS != NULL
               && nBlockXOff >= 0
               && nBlockYOff >= 0
               && pImage != NULL );

    iScanOffset = poGDS->sFileHeader.iOffBits +
            ( poGDS->GetRasterYSize() - nBlockYOff - 1 ) * nScanSize;
    if ( VSIFSeekL( poGDS->fp, iScanOffset, SEEK_SET ) < 0 )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Can't seek to offset %ld in output file to write data.\n%s",
                  iScanOffset, VSIStrerror( errno ) );
        return CE_Failure;
    }

    if( poGDS->nBands != 1 )
    {
        memset( pabyScan, 0, nScanSize );
        VSIFReadL( pabyScan, 1, nScanSize, poGDS->fp );
        VSIFSeekL( poGDS->fp, iScanOffset, SEEK_SET );
    }

    for ( iInPixel = 0, iOutPixel = iBytesPerPixel - nBand;
          iInPixel < nBlockXSize; iInPixel++, iOutPixel += poGDS->nBands )
    {
        pabyScan[iOutPixel] = ((GByte *) pImage)[iInPixel];
    }

    if ( VSIFWriteL( pabyScan, 1, nScanSize, poGDS->fp ) < nScanSize )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Can't write block with X offset %d and Y offset %d.\n%s",
                  nBlockXOff, nBlockYOff,
                  VSIStrerror( errno ) );
        return CE_Failure;
    }

    return CE_None;
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

GDALColorTable *BMPRasterBand::GetColorTable()
{
    BMPDataset   *poGDS = (BMPDataset *) poDS;

    return poGDS->poColorTable;
}

/************************************************************************/
/*                           SetColorTable()                            */
/************************************************************************/

CPLErr BMPRasterBand::SetColorTable( GDALColorTable *poColorTable )
{
    BMPDataset  *poGDS = (BMPDataset *) poDS;

    if ( poColorTable )
    {
        GDALColorEntry  oEntry;
        GUInt32         iULong;
        unsigned int    i;

        poGDS->sInfoHeader.iClrUsed = poColorTable->GetColorEntryCount();
        if ( poGDS->sInfoHeader.iClrUsed < 1 ||
             poGDS->sInfoHeader.iClrUsed > (1U << poGDS->sInfoHeader.iBitCount) )
            return CE_Failure;

        VSIFSeekL( poGDS->fp, BFH_SIZE + 32, SEEK_SET );

        iULong = CPL_LSBWORD32( poGDS->sInfoHeader.iClrUsed );
        VSIFWriteL( &iULong, 4, 1, poGDS->fp );
        poGDS->pabyColorTable = (GByte *) CPLRealloc( poGDS->pabyColorTable,
                        poGDS->nColorElems * poGDS->sInfoHeader.iClrUsed );
        if ( !poGDS->pabyColorTable )
            return CE_Failure;

        for( i = 0; i < poGDS->sInfoHeader.iClrUsed; i++ )
        {
            poColorTable->GetColorEntryAsRGB( i, &oEntry );
            poGDS->pabyColorTable[i * poGDS->nColorElems + 3] = 0;
            poGDS->pabyColorTable[i * poGDS->nColorElems + 2] = 
                (GByte) oEntry.c1; // Red
            poGDS->pabyColorTable[i * poGDS->nColorElems + 1] = 
                (GByte) oEntry.c2; // Green
            poGDS->pabyColorTable[i * poGDS->nColorElems] = 
                (GByte) oEntry.c3;     // Blue
        }

        VSIFSeekL( poGDS->fp, BFH_SIZE + poGDS->sInfoHeader.iSize, SEEK_SET );
        if ( VSIFWriteL( poGDS->pabyColorTable, 1,
                poGDS->nColorElems * poGDS->sInfoHeader.iClrUsed, poGDS->fp ) <
             poGDS->nColorElems * (GUInt32) poGDS->sInfoHeader.iClrUsed )
        {
            return CE_Failure;
        }
    }
    else
        return CE_Failure;

    return CE_None;
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp BMPRasterBand::GetColorInterpretation()
{
    BMPDataset      *poGDS = (BMPDataset *) poDS;

    if( poGDS->sInfoHeader.iBitCount == 24 ||
        poGDS->sInfoHeader.iBitCount == 32 ||
        poGDS->sInfoHeader.iBitCount == 16 )
    {
        if( nBand == 1 )
            return GCI_RedBand;
        else if( nBand == 2 )
            return GCI_GreenBand;
        else if( nBand == 3 )
            return GCI_BlueBand;
        else
            return GCI_Undefined;
    }
    else
    {
        return GCI_PaletteIndex;
    }
}

/************************************************************************/
/* ==================================================================== */
/*                       BMPComprRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class BMPComprRasterBand : public BMPRasterBand
{
    friend class BMPDataset;

    GByte           *pabyComprBuf;
    GByte           *pabyUncomprBuf;

  public:

                BMPComprRasterBand( BMPDataset *, int );
                ~BMPComprRasterBand();

    virtual CPLErr          IReadBlock( int, int, void * );
//    virtual CPLErr        IWriteBlock( int, int, void * );
};

/************************************************************************/
/*                           BMPComprRasterBand()                       */
/************************************************************************/

BMPComprRasterBand::BMPComprRasterBand( BMPDataset *poDS, int nBand )
    : BMPRasterBand( poDS, nBand )
{
    unsigned int    i, j, k, iLength;
    GUInt32         iComprSize, iUncomprSize;

    iComprSize = poDS->sFileHeader.iSize - poDS->sFileHeader.iOffBits;
    iUncomprSize = poDS->GetRasterXSize() * poDS->GetRasterYSize();
    pabyComprBuf = (GByte *) CPLMalloc( iComprSize );
    pabyUncomprBuf = (GByte *) CPLMalloc( iUncomprSize );

    CPLDebug( "BMP", "RLE compression detected." );
    CPLDebug ( "BMP", "Size of compressed buffer %ld bytes,"
               " size of uncompressed buffer %ld bytes.",
               iComprSize, iUncomprSize );

    VSIFSeekL( poDS->fp, poDS->sFileHeader.iOffBits, SEEK_SET );
    VSIFReadL( pabyComprBuf, 1, iComprSize, poDS->fp );
    i = 0;
    j = 0;
    if ( poDS->sInfoHeader.iBitCount == 8 )         // RLE8
    {
        while( j < iUncomprSize && i < iComprSize )
        {
            if ( pabyComprBuf[i] )
            {
                iLength = pabyComprBuf[i++];
                while( iLength > 0 && j < iUncomprSize && i < iComprSize )
                {
                    pabyUncomprBuf[j++] = pabyComprBuf[i];
                    iLength--;
                }
                i++;
            }
            else
            {
                i++;
                if ( pabyComprBuf[i] == 0 )         // Next scanline
                {
                    i++;
                }
                else if ( pabyComprBuf[i] == 1 )    // End of image
                {
                    break;
                }
                else if ( pabyComprBuf[i] == 2 )    // Move to...
                {
                    i++;
                    if ( i < iComprSize - 1 )
                    {
                        j += pabyComprBuf[i] +
                             pabyComprBuf[i+1] * poDS->GetRasterXSize();
                        i += 2;
                    }
                    else
                        break;
                }
                else                                // Absolute mode
                {
                    iLength = pabyComprBuf[i++];
                    for ( k = 0; k < iLength && j < iUncomprSize && i < iComprSize; k++ )
                        pabyUncomprBuf[j++] = pabyComprBuf[i++];
                    if ( k & 0x01 )
                        i++;
                }
            }
        }
    }
    else                                            // RLE4
    {
        while( j < iUncomprSize && i < iComprSize )
        {
            if ( pabyComprBuf[i] )
            {
                iLength = pabyComprBuf[i++];
                while( iLength > 0 && j < iUncomprSize && i < iComprSize )
                {
                    if ( iLength & 0x01 )
                        pabyUncomprBuf[j++] = (pabyComprBuf[i] & 0xF0) >> 4;
                    else
                        pabyUncomprBuf[j++] = pabyComprBuf[i] & 0x0F;
                    iLength--;
                }
                i++;
            }
            else
            {
                i++;
                if ( pabyComprBuf[i] == 0 )         // Next scanline
                {
                    i++;
                }
                else if ( pabyComprBuf[i] == 1 )    // End of image
                {
                    break;
                }
                else if ( pabyComprBuf[i] == 2 )    // Move to...
                {
                    i++;
                    if ( i < iComprSize - 1 )
                    {
                        j += pabyComprBuf[i] +
                             pabyComprBuf[i+1] * poDS->GetRasterXSize();
                        i += 2;
                    }
                    else
                        break;
                }
                else                                // Absolute mode
                {
                    iLength = pabyComprBuf[i++];
                    for ( k = 0; k < iLength && j < iUncomprSize && i < iComprSize; k++ )
                    {
                        if ( k & 0x01 )
                            pabyUncomprBuf[j++] = pabyComprBuf[i++] & 0x0F;
                        else
                            pabyUncomprBuf[j++] = (pabyComprBuf[i] & 0xF0) >> 4;
                    }
                    if ( k & 0x01 )
                        i++;
                }
            }
        }
    }
}

/************************************************************************/
/*                           ~BMPComprRasterBand()                      */
/************************************************************************/

BMPComprRasterBand::~BMPComprRasterBand()
{
    if ( pabyComprBuf )
        CPLFree( pabyComprBuf );
    if ( pabyUncomprBuf )
        CPLFree( pabyUncomprBuf );
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr BMPComprRasterBand::
    IReadBlock( int nBlockXOff, int nBlockYOff, void * pImage )
{
    memcpy( pImage, pabyUncomprBuf +
            (poDS->GetRasterYSize() - nBlockYOff - 1) * poDS->GetRasterXSize(),
            nBlockXSize );

    return CE_None;
}

/************************************************************************/
/*                           BMPDataset()                               */
/************************************************************************/

BMPDataset::BMPDataset()
{
    pszFilename = NULL;
    fp = NULL;
    nBands = 0;
    pszProjection = CPLStrdup( "" );
    bGeoTransformValid = FALSE;
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;
    pabyColorTable = NULL;
    poColorTable = NULL;
    memset( &sFileHeader, 0, sizeof(sFileHeader) );
    memset( &sInfoHeader, 0, sizeof(sInfoHeader) );
}

/************************************************************************/
/*                            ~BMPDataset()                             */
/************************************************************************/

BMPDataset::~BMPDataset()
{
    FlushCache();

    if ( pszProjection )
        CPLFree( pszProjection );
    if ( pabyColorTable )
        CPLFree( pabyColorTable );
    if ( poColorTable != NULL )
        delete poColorTable;
    if( fp != NULL )
        VSIFCloseL( fp );
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr BMPDataset::GetGeoTransform( double * padfTransform )
{
    memcpy( padfTransform, adfGeoTransform, sizeof(adfGeoTransform[0]) * 6 );

    if( bGeoTransformValid )
        return CE_None;
    else
        return CE_Failure;
}

/************************************************************************/
/*                          SetGeoTransform()                           */
/************************************************************************/

CPLErr BMPDataset::SetGeoTransform( double * padfTransform )
{
    CPLErr              eErr = CE_None;

    memcpy( adfGeoTransform, padfTransform, sizeof(double) * 6 );

    if ( pszFilename && bGeoTransformValid )
        if ( GDALWriteWorldFile( pszFilename, "wld", adfGeoTransform )
             == FALSE )
        {
            CPLError( CE_Failure, CPLE_FileIO, "Can't write world file." );
            eErr = CE_Failure;
        }

    return eErr;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *BMPDataset::GetProjectionRef()
{
    if( pszProjection )
        return pszProjection;
    else
        return "";
}

/************************************************************************/
/*                             IRasterIO()                              */
/*                                                                      */
/*      Multi-band raster io handler.  We will use  block based         */
/*      loading is used for multiband BMPs.  That is because they       */
/*      are effectively pixel interleaved, so processing all bands      */
/*      for a given block together avoid extra seeks.                   */
/************************************************************************/

CPLErr BMPDataset::IRasterIO( GDALRWFlag eRWFlag, 
                              int nXOff, int nYOff, int nXSize, int nYSize,
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType,
                              int nBandCount, int *panBandMap, 
                              int nPixelSpace, int nLineSpace, int nBandSpace )

{
    if( nBandCount > 1 )
        return GDALDataset::BlockBasedRasterIO( 
            eRWFlag, nXOff, nYOff, nXSize, nYSize,
            pData, nBufXSize, nBufYSize, eBufType, 
            nBandCount, panBandMap, nPixelSpace, nLineSpace, nBandSpace );
    else
        return 
            GDALDataset::IRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize,
                                    pData, nBufXSize, nBufYSize, eBufType, 
                                    nBandCount, panBandMap, 
                                    nPixelSpace, nLineSpace, nBandSpace );
}

/************************************************************************/
/*                             FlushCache()                             */
/************************************************************************/

void BMPDataset::FlushCache()

{
    GDALDataset::FlushCache();
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *BMPDataset::Open( GDALOpenInfo * poOpenInfo )
{
    if( poOpenInfo->fp == NULL )
        return NULL;

    if( !EQUALN((const char *) poOpenInfo->pabyHeader, "BM", 2) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    BMPDataset      *poDS;
    VSIStatBuf      sStat;

    poDS = new BMPDataset();

    if( poOpenInfo->eAccess == GA_ReadOnly )
        poDS->fp = VSIFOpenL( poOpenInfo->pszFilename, "rb" );
    else
        poDS->fp = VSIFOpenL( poOpenInfo->pszFilename, "r+b" );
    if ( !poDS->fp )
        return NULL;

    CPLStat(poOpenInfo->pszFilename, &sStat);

/* -------------------------------------------------------------------- */
/*      Read the BMPFileHeader. We need iOffBits value only             */
/* -------------------------------------------------------------------- */
    VSIFSeekL( poDS->fp, 10, SEEK_SET );
    VSIFReadL( &poDS->sFileHeader.iOffBits, 1, 4, poDS->fp );
#ifdef CPL_MSB
    CPL_SWAP32PTR( &poDS->sFileHeader.iOffBits );
#endif
    poDS->sFileHeader.iSize = sStat.st_size;
    CPLDebug( "BMP", "File size %d bytes.", poDS->sFileHeader.iSize );
    CPLDebug( "BMP", "Image offset 0x%x bytes from file start.",
              poDS->sFileHeader.iOffBits );

/* -------------------------------------------------------------------- */
/*      Read the BMPInfoHeader.                                         */
/* -------------------------------------------------------------------- */
    BMPType         eBMPType;

    VSIFSeekL( poDS->fp, BFH_SIZE, SEEK_SET );
    VSIFReadL( &poDS->sInfoHeader.iSize, 1, 4, poDS->fp );
#ifdef CPL_MSB
    CPL_SWAP32PTR( &poDS->sInfoHeader.iSize );
#endif

    if ( poDS->sInfoHeader.iSize == BIH_WIN4SIZE )
        eBMPType = BMPT_WIN4;
    else if ( poDS->sInfoHeader.iSize == BIH_OS21SIZE )
        eBMPType = BMPT_OS21;
    else if ( poDS->sInfoHeader.iSize == BIH_OS22SIZE ||
              poDS->sInfoHeader.iSize == 16 )
        eBMPType = BMPT_OS22;
    else
        eBMPType = BMPT_WIN5;

    if ( eBMPType == BMPT_WIN4 || eBMPType == BMPT_WIN5 || eBMPType == BMPT_OS22 )
    {
        VSIFReadL( &poDS->sInfoHeader.iWidth, 1, 4, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iHeight, 1, 4, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iPlanes, 1, 2, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iBitCount, 1, 2, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iCompression, 1, 4, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iSizeImage, 1, 4, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iXPelsPerMeter, 1, 4, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iYPelsPerMeter, 1, 4, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iClrUsed, 1, 4, poDS->fp );
        VSIFReadL( &poDS->sInfoHeader.iClrImportant, 1, 4, poDS->fp );
#ifdef CPL_MSB
        CPL_SWAP32PTR( &poDS->sInfoHeader.iWidth );
        CPL_SWAP32PTR( &poDS->sInfoHeader.iHeight );
        CPL_SWAP16PTR( &poDS->sInfoHeader.iPlanes );
        CPL_SWAP16PTR( &poDS->sInfoHeader.iBitCount );
        CPL_SWAP32PTR( &poDS->sInfoHeader.iCompression );
        CPL_SWAP32PTR( &poDS->sInfoHeader.iSizeImage );
        CPL_SWAP32PTR( &poDS->sInfoHeader.iXPelsPerMeter );
        CPL_SWAP32PTR( &poDS->sInfoHeader.iYPelsPerMeter );
        CPL_SWAP32PTR( &poDS->sInfoHeader.iClrUsed );
        CPL_SWAP32PTR( &poDS->sInfoHeader.iClrImportant );
#endif
        poDS->nColorElems = 4;
    }
    
    if ( eBMPType == BMPT_OS22 )
    {
        poDS->nColorElems = 3; // FIXME: different info in different documents regarding this!
    }
    
    if ( eBMPType == BMPT_OS21 )
    {
        GInt16  iShort;

        VSIFReadL( &iShort, 1, 2, poDS->fp );
        poDS->sInfoHeader.iWidth = CPL_LSBWORD16( iShort );
        VSIFReadL( &iShort, 1, 2, poDS->fp );
        poDS->sInfoHeader.iHeight = CPL_LSBWORD16( iShort );
        VSIFReadL( &iShort, 1, 2, poDS->fp );
        poDS->sInfoHeader.iPlanes = CPL_LSBWORD16( iShort );
        VSIFReadL( &iShort, 1, 2, poDS->fp );
        poDS->sInfoHeader.iBitCount = CPL_LSBWORD16( iShort );
        poDS->sInfoHeader.iCompression = BMPC_RGB;
        poDS->nColorElems = 3;
    }

    if ( poDS->sInfoHeader.iBitCount != 1  &&
         poDS->sInfoHeader.iBitCount != 4  &&
         poDS->sInfoHeader.iBitCount != 8  &&
         poDS->sInfoHeader.iBitCount != 16 &&
         poDS->sInfoHeader.iBitCount != 24 &&
         poDS->sInfoHeader.iBitCount != 32 )
    {
        delete poDS;
        return NULL;
    }

    CPLDebug( "BMP", "Windows Device Independent Bitmap parameters:\n"
              " info header size: %d bytes\n"
              " width: %d\n height: %d\n planes: %d\n bpp: %d\n"
              " compression: %d\n image size: %d bytes\n X resolution: %d\n"
              " Y resolution: %d\n colours used: %d\n colours important: %d",
              poDS->sInfoHeader.iSize,
              poDS->sInfoHeader.iWidth, poDS->sInfoHeader.iHeight,
              poDS->sInfoHeader.iPlanes, poDS->sInfoHeader.iBitCount,
              poDS->sInfoHeader.iCompression, poDS->sInfoHeader.iSizeImage,
              poDS->sInfoHeader.iXPelsPerMeter, poDS->sInfoHeader.iYPelsPerMeter,
              poDS->sInfoHeader.iClrUsed, poDS->sInfoHeader.iClrImportant );

    poDS->nRasterXSize = poDS->sInfoHeader.iWidth;
    poDS->nRasterYSize = (poDS->sInfoHeader.iHeight > 0)?
        poDS->sInfoHeader.iHeight:-poDS->sInfoHeader.iHeight;
    switch ( poDS->sInfoHeader.iBitCount )
    {
        case 1:
        case 4:
        case 8:
        {
            int     i;

            poDS->nBands = 1;
            // Allocate memory for colour table and read it
            if ( poDS->sInfoHeader.iClrUsed )
                poDS->nColorTableSize = poDS->sInfoHeader.iClrUsed;
            else
                poDS->nColorTableSize = 1 << poDS->sInfoHeader.iBitCount;
            poDS->pabyColorTable =
                (GByte *)CPLMalloc( poDS->nColorElems * poDS->nColorTableSize );
            VSIFSeekL( poDS->fp, BFH_SIZE + poDS->sInfoHeader.iSize, SEEK_SET );
            VSIFReadL( poDS->pabyColorTable, poDS->nColorElems,
                      poDS->nColorTableSize, poDS->fp );

            GDALColorEntry oEntry;
            poDS->poColorTable = new GDALColorTable();
            for( i = 0; i < poDS->nColorTableSize; i++ )
            {
                oEntry.c1 = poDS->pabyColorTable[i * poDS->nColorElems + 2]; // Red
                oEntry.c2 = poDS->pabyColorTable[i * poDS->nColorElems + 1]; // Green
                oEntry.c3 = poDS->pabyColorTable[i * poDS->nColorElems];     // Blue
                oEntry.c4 = 255;

                poDS->poColorTable->SetColorEntry( i, &oEntry );
            }
        }
        break;
        case 16:
        case 24:
        case 32:
        poDS->nBands = 3;
        break;
        default:
        delete poDS;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    int             iBand;

    if ( poDS->sInfoHeader.iCompression == BMPC_RGB )
    {
        for( iBand = 1; iBand <= poDS->nBands; iBand++ )
            poDS->SetBand( iBand, new BMPRasterBand( poDS, iBand ) );
    }
    else if ( poDS->sInfoHeader.iCompression == BMPC_RLE8
              || poDS->sInfoHeader.iCompression == BMPC_RLE4 )
    {
        for( iBand = 1; iBand <= poDS->nBands; iBand++ )
            poDS->SetBand( iBand, new BMPComprRasterBand( poDS, iBand ) );
    }
    else
    {
        delete poDS;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Check for world file.                                           */
/* -------------------------------------------------------------------- */
    poDS->bGeoTransformValid =
        GDALReadWorldFile( poOpenInfo->pszFilename, ".wld",
                           poDS->adfGeoTransform );

    if( !poDS->bGeoTransformValid )
        poDS->bGeoTransformValid =
            GDALReadWorldFile( poOpenInfo->pszFilename, ".bpw",
                               poDS->adfGeoTransform );

    if( !poDS->bGeoTransformValid )
        poDS->bGeoTransformValid =
            GDALReadWorldFile( poOpenInfo->pszFilename, ".bmpw",
                               poDS->adfGeoTransform );

    return( poDS );
}

/************************************************************************/
/*                               Create()                               */
/************************************************************************/

GDALDataset *BMPDataset::Create( const char * pszFilename,
                                 int nXSize, int nYSize, int nBands,
                                 GDALDataType eType, char **papszOptions )

{
    if( eType != GDT_Byte )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
              "Attempt to create BMP dataset with an illegal\n"
              "data type (%s), only Byte supported by the format.\n",
              GDALGetDataTypeName(eType) );

        return NULL;
    }

    if( nBands != 1 && nBands != 3 )
    {
        CPLError( CE_Failure, CPLE_NotSupported,
                  "BMP driver doesn't support %d bands. Must be 1 or 3.\n",
                  nBands );

        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create the dataset.                                             */
/* -------------------------------------------------------------------- */
    BMPDataset      *poDS;

    poDS = new BMPDataset();

    poDS->fp = VSIFOpenL( pszFilename, "wb+" );
    if( poDS->fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to create file %s.\n",
                  pszFilename );
        return NULL;
    }

    poDS->pszFilename = pszFilename;

/* -------------------------------------------------------------------- */
/*      Fill the BMPInfoHeader                                          */
/* -------------------------------------------------------------------- */
    GUInt32         nScanSize;

    poDS->sInfoHeader.iSize = 40;
    poDS->sInfoHeader.iWidth = nXSize;
    poDS->sInfoHeader.iHeight = nYSize;
    poDS->sInfoHeader.iPlanes = 1;
    poDS->sInfoHeader.iBitCount = ( nBands == 3 )?24:8;
    poDS->sInfoHeader.iCompression = BMPC_RGB;
    nScanSize = ((poDS->sInfoHeader.iWidth *
                  poDS->sInfoHeader.iBitCount + 31) & ~31) / 8;
    poDS->sInfoHeader.iSizeImage = nScanSize * poDS->sInfoHeader.iHeight;
    poDS->sInfoHeader.iXPelsPerMeter = 0;
    poDS->sInfoHeader.iYPelsPerMeter = 0;
    poDS->nColorElems = 4;

/* -------------------------------------------------------------------- */
/*      Do we need colour table?                                        */
/* -------------------------------------------------------------------- */
    unsigned int    i;

    if ( nBands == 1 )
    {
        poDS->sInfoHeader.iClrUsed = 1 << poDS->sInfoHeader.iBitCount;
        poDS->pabyColorTable =
            (GByte *) CPLMalloc( poDS->nColorElems * poDS->sInfoHeader.iClrUsed );
        for ( i = 0; i < poDS->sInfoHeader.iClrUsed; i++ )
        {
            poDS->pabyColorTable[i * poDS->nColorElems] =
                poDS->pabyColorTable[i * poDS->nColorElems + 1] =
                poDS->pabyColorTable[i * poDS->nColorElems + 2] =
                poDS->pabyColorTable[i * poDS->nColorElems + 3] = (GByte) i;
        }
    }
    else
    {
        poDS->sInfoHeader.iClrUsed = 0;
    }
    poDS->sInfoHeader.iClrImportant = 0;

/* -------------------------------------------------------------------- */
/*      Fill the BMPFileHeader                                          */
/* -------------------------------------------------------------------- */
    poDS->sFileHeader.bType[0] = 'B';
    poDS->sFileHeader.bType[1] = 'M';
    poDS->sFileHeader.iSize = BFH_SIZE + poDS->sInfoHeader.iSize +
                    poDS->sInfoHeader.iClrUsed * poDS->nColorElems +
                    poDS->sInfoHeader.iSizeImage;
    poDS->sFileHeader.iReserved1 = 0;
    poDS->sFileHeader.iReserved2 = 0;
    poDS->sFileHeader.iOffBits = BFH_SIZE + poDS->sInfoHeader.iSize +
                    poDS->sInfoHeader.iClrUsed * poDS->nColorElems;

/* -------------------------------------------------------------------- */
/*      Write all structures to the file                                */
/* -------------------------------------------------------------------- */
    if( VSIFWriteL( &poDS->sFileHeader.bType, 1, 2, poDS->fp ) != 2 )
    {
        CPLError( CE_Failure, CPLE_FileIO, 
                  "Write of first 2 bytes to BMP file %s failed.\n"
                  "Is file system full?",
                  pszFilename );
        VSIFCloseL( poDS->fp );
        delete poDS;

        return NULL;
    }

    GInt32      iLong;
    GUInt32     iULong;
    GUInt16     iUShort;

    iULong = CPL_LSBWORD32( poDS->sFileHeader.iSize );
    VSIFWriteL( &iULong, 4, 1, poDS->fp );
    iUShort = CPL_LSBWORD16( poDS->sFileHeader.iReserved1 );
    VSIFWriteL( &iUShort, 2, 1, poDS->fp );
    iUShort = CPL_LSBWORD16( poDS->sFileHeader.iReserved2 );
    VSIFWriteL( &iUShort, 2, 1, poDS->fp );
    iULong = CPL_LSBWORD32( poDS->sFileHeader.iOffBits );
    VSIFWriteL( &iULong, 4, 1, poDS->fp );

    iULong = CPL_LSBWORD32( poDS->sInfoHeader.iSize );
    VSIFWriteL( &iULong, 4, 1, poDS->fp );
    iLong = CPL_LSBWORD32( poDS->sInfoHeader.iWidth );
    VSIFWriteL( &iLong, 4, 1, poDS->fp );
    iLong = CPL_LSBWORD32( poDS->sInfoHeader.iHeight );
    VSIFWriteL( &iLong, 4, 1, poDS->fp );
    iUShort = CPL_LSBWORD16( poDS->sInfoHeader.iPlanes );
    VSIFWriteL( &iUShort, 2, 1, poDS->fp );
    iUShort = CPL_LSBWORD16( poDS->sInfoHeader.iBitCount );
    VSIFWriteL( &iUShort, 2, 1, poDS->fp );
    iULong = CPL_LSBWORD32( poDS->sInfoHeader.iCompression );
    VSIFWriteL( &iULong, 4, 1, poDS->fp );
    iULong = CPL_LSBWORD32( poDS->sInfoHeader.iSizeImage );
    VSIFWriteL( &iULong, 4, 1, poDS->fp );
    iLong = CPL_LSBWORD32( poDS->sInfoHeader.iXPelsPerMeter );
    VSIFWriteL( &iLong, 4, 1, poDS->fp );
    iLong = CPL_LSBWORD32( poDS->sInfoHeader.iYPelsPerMeter );
    VSIFWriteL( &iLong, 4, 1, poDS->fp );
    iULong = CPL_LSBWORD32( poDS->sInfoHeader.iClrUsed );
    VSIFWriteL( &iULong, 4, 1, poDS->fp );
    iULong = CPL_LSBWORD32( poDS->sInfoHeader.iClrImportant );
    VSIFWriteL( &iULong, 4, 1, poDS->fp );

    if ( poDS->sInfoHeader.iClrUsed )
    {
        if( VSIFWriteL( poDS->pabyColorTable, 1,
                        poDS->nColorElems * poDS->sInfoHeader.iClrUsed, poDS->fp ) 
            != poDS->nColorElems * poDS->sInfoHeader.iClrUsed )
        {
            CPLError( CE_Failure, CPLE_FileIO, 
                      "Error writing color table.  Is disk full?" );
            VSIFCloseL( poDS->fp );
            delete poDS;

            return NULL;
        }
    }

    poDS->nRasterXSize = nXSize;
    poDS->nRasterYSize = nYSize;
    poDS->eAccess = GA_Update;
    poDS->nBands = nBands;

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    int         iBand;

    for( iBand = 1; iBand <= poDS->nBands; iBand++ )
    {
        poDS->SetBand( iBand, new BMPRasterBand( poDS, iBand ) );
    }

/* -------------------------------------------------------------------- */
/*      Do we need a world file?                                        */
/* -------------------------------------------------------------------- */
    if( CSLFetchBoolean( papszOptions, "WORLDFILE", FALSE ) )
        poDS->bGeoTransformValid = TRUE;

    return (GDALDataset *) poDS;
}

/************************************************************************/
/*                        GDALRegister_BMP()                            */
/************************************************************************/

void GDALRegister_BMP()

{
    GDALDriver  *poDriver;

    if( GDALGetDriverByName( "BMP" ) == NULL )
    {
        poDriver = new GDALDriver();

        poDriver->SetDescription( "BMP" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME,
                                   "MS Windows Device Independent Bitmap" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC,
                                   "frmt_bmp.html" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "bmp" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES, "Byte" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONOPTIONLIST,
"<CreationOptionList>"
"   <Option name='WORLDFILE' type='boolean' description='Write out world file'/>"
"</CreationOptionList>" );

        poDriver->pfnOpen = BMPDataset::Open;
        poDriver->pfnCreate = BMPDataset::Create;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

