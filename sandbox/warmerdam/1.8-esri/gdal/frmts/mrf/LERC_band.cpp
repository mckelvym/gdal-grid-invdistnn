/*
* Copyright (c) 2013 - 2014 Esri
*
* LERC band
* LERC page compression and decompression functions
* These functions are not methods, they reside in the global space
*
* Author:   Lucian Plesea, lplesea@esri.com
*
*/

#include "marfa.h"
#include "CntZImage.h"

//
// RGB to/from YUV and YCbCr conversions
// Y stays positive, but U and V do not
// The main difference between UV and CbCr is the normalization, 
// CbCr have [-0.5,0.5] range
//

inline double Yuv(double red, double green, double blue) {
    return .299*red + .587*green + .114*blue;
}

inline double yUv(double red, double green, double blue) {
    return -0.14713*red -0.28886*green +0.436*blue;
}

inline double yuV(double red, double green, double blue) {
    return +0.615*red -0.51499*green -0.10001*blue;
}

inline double yCBCr(double red, double green, double blue) {
    return -0.169*red -0.331*green + 0.499*blue;
}

inline double yCbCR(double red, double green, double blue) {
    return 0.499*red -0.418*green - 0.0813*blue;
}

// YUV to RGB
inline double Rgb(double y, double u, double v) {
    return y + 1.13583*v;
}

inline double rGb(double y, double u, double v) {
    return y -0.39465*u -0.58060*v;
}

inline double rgB(double y, double u, double v) {
    return y +2.03211*u;
}

// YCbCr to RGB
inline double RgbFromYCC(double y, double Cb, double Cr) {
    return y + 1.402*Cr;
}

inline double rGbFromYCC(double y, double Cb, double Cr) {
    return y -.344*Cb -0.714*Cr;
}

inline double rgBFromYCC(double y, double Cb, double Cr) {
    return y + 1.772*Cb;
}

// Load a buffer into a zImg
template <typename T> void CntZImgFill(CntZImage &zImg, T *src, const ILImage &img) 
{
    int w = img.pagesize.x;
    int h = img.pagesize.y;

    // Pagesize of 3 means interleaved, we convert it to YUV2:0:0, so the vertical size will be equal
    // to 1.5 of a single channel
    int zimg_height = (img.pagesize.c == 1) ? h: h + h/2;
    zImg.resize(w,zimg_height);
    T *ptr = src;
    double ndv = img.NoDataValue;
    // use 0 if NoData is not defined
    if (!img.hasNoData) ndv=0;

    if (img.pagesize.c == 1) {
	for (int i=0; i < h; i++)
	    for (int j=0; j < w; j++) {
		zImg(i,j).z = float(*ptr++);
		zImg(i,j).cnt = !CPLIsEqual(zImg(i,j).z, ndv);
	    }
	return;
    }

    // How do we pack the image?  Y at the top, then U next to V.
    // Doing 2x2 input pixels at a time
    for (int i=0; i < h; i+=2) {
	T *odd_line = ptr + 3 * img.pagesize.x;
	for (int j=0; j < w; j+=2) {
	    double rgb[3][4]; // four pixels RGB, band separate
	    for (int c=0; c<3; c++) {
		rgb[c][0] = double(*ptr);
		rgb[c][1] = double(ptr[3]);
		rgb[c][2] = double(*odd_line);
		rgb[c][3] = double(odd_line[3]);
		ptr++;
		odd_line++;
	    }
	    // Skip the next input column too
	    ptr+=3;
	    odd_line+=3;
	    // Y is aranged as expected
	    zImg(i,j).z     = Yuv(rgb[0][0], rgb[1][0], rgb[2][0]);
	    zImg(i,j+1).z   = Yuv(rgb[0][1], rgb[1][1], rgb[2][1]);
	    zImg(i+1,j).z   = Yuv(rgb[0][2], rgb[1][2], rgb[2][2]);
	    zImg(i+1,j+1).z = Yuv(rgb[0][3], rgb[1][3], rgb[2][3]);

#if defined(LERC_YUV)
	    // The U and V are half size images, stored after the Y
	    zImg(h+i/2, j/2).z = .25*(
		yUv(rgb[0][0], rgb[1][0], rgb[2][0]) +
		yUv(rgb[0][1], rgb[1][1], rgb[2][1]) +
		yUv(rgb[0][2], rgb[1][1], rgb[2][1]) +
		yUv(rgb[0][3], rgb[1][1], rgb[2][1]));

	    zImg(h+i/2, w/2 + j/2).z = .25*(
		yuV(rgb[0][0], rgb[1][0], rgb[2][0]) +
		yuV(rgb[0][1], rgb[1][1], rgb[2][1]) +
		yuV(rgb[0][2], rgb[1][1], rgb[2][1]) +
		yuV(rgb[0][3], rgb[1][1], rgb[2][1]));

#else // YCrCb instead
	    // The Cr and Cb are half size images, stored after the Y
	    zImg(h+i/2, j/2).z = .25*(
		yCBCr(rgb[0][0], rgb[1][0], rgb[2][0]) +
		yCBCr(rgb[0][1], rgb[1][1], rgb[2][1]) +
		yCBCr(rgb[0][2], rgb[1][1], rgb[2][1]) +
		yCBCr(rgb[0][3], rgb[1][1], rgb[2][1]));

	    zImg(h+i/2, w/2 + j/2).z = .25*(
		yCbCR(rgb[0][0], rgb[1][0], rgb[2][0]) +
		yCbCR(rgb[0][1], rgb[1][1], rgb[2][1]) +
		yCbCR(rgb[0][2], rgb[1][1], rgb[2][1]) +
		yCbCR(rgb[0][3], rgb[1][1], rgb[2][1]));
#endif
	}
	// The odd line pointer is at the begining of the next even line
	ptr = odd_line;
    }

    // Done with the pixel values, fill the alpha channel
    for (int i=0; i < zimg_height; i++)
	for (int j=0; j < w; j++)
	    zImg(i,j).cnt = 1;
}

// Clip at byte range
inline unsigned char bclip(double val) {
    unsigned char ret = (unsigned char)(val + 0.5);
    if (val < 0) ret=0;
    if (val > 255) ret=255;
    return ret;
}

// Unload a zImg into a buffer
template <typename T> void CntZImgUFill(CntZImage &zImg, T *dst, const ILImage &img)
{
    int h = zImg.getHeight();
    int w = zImg.getWidth();
    T *ptr = dst;
    double ndv = img.NoDataValue;
    // Use 0 if nodata is not defined
    if (!img.hasNoData) ndv=0;
    if (h == img.pagesize.y) {
	for (int i=0; i < h; i++)
	    for (int j=0; j < w; j++)
		*ptr++ = (T)((zImg(i,j).cnt == 0)?ndv:zImg(i,j).z);
	return;
    }

    // Interleaved, with color conversion
    // This is the output image height. the zimg is half again taller
    h = img.pagesize.y;
    // Need to convert from Yuv2:0:0 to RGB1:1:1
    for (int i=0; i < h; i++)
	for (int j=0; j < w; j++) {
	    // If any of the color components is no data, can't compute value
	    if ( 0 == zImg(i,j).cnt 
		|| 0 == zImg( h + i/2, j/2).cnt 
		|| 0 == zImg( h + i/2, w/2 + j/2).cnt) {
		// Three NDVs because it is RGB
		*ptr++=ndv;
		*ptr++=ndv;
		*ptr++=ndv;
		continue;
	    }

#if defined(LERC_YUV)
	    // Pick up the YUV
	    double y = zImg(i,j).z;
	    double u = zImg(h + i/2, j/2).z;
	    double v = zImg(h + i/2, w/2 + j/2).z;
	    *ptr++ = bclip(Rgb(y,u,v));
	    *ptr++ = bclip(rGb(y,u,v));
	    *ptr++ = bclip(rgB(y,u,v));
#else
	    // Pick up the YCbCr
	    double y = zImg(i,j).z;
	    double Cb = zImg(h + i/2, j/2).z;
	    double Cr = zImg(h + i/2, w/2 + j/2).z;
	    *ptr++ = bclip(RgbFromYCC(y,Cb,Cr));
	    *ptr++ = bclip(rGbFromYCC(y,Cb,Cr));
	    *ptr++ = bclip(rgBFromYCC(y,Cb,Cr));
#endif

	}

}

//
CPLErr CompressLERC(buf_mgr &dst, buf_mgr &src, const ILImage &img, double precision)
{
    CntZImage zImg;
    // Fill data into zImg
#define FILL(T) CntZImgFill<T>(zImg, (T *)(src.buffer), img)
    switch (img.dt) {
    case GDT_Byte:	FILL(GByte);	break;
    case GDT_UInt16:	FILL(GUInt16);	break;
    case GDT_Int16:	FILL(GInt16);	break;
    case GDT_Int32:	FILL(GInt32);	break;
    case GDT_UInt32:	FILL(GUInt32);	break;
    case GDT_Float32:	FILL(float);	break;
    case GDT_Float64:	FILL(double);	break;
    }
#undef FILL

    Byte *ptr = (Byte *)dst.buffer;

    // if it can't compress in output buffer it will crash
    if (!zImg.write(&ptr, precision)) {
	CPLError(CE_Failure,CPLE_AppDefined,"MRF: Error during LERC compression");
	return CE_Failure;
    }

    // write changes the value of the pointer, we can find the size by testing how far it moved
    dst.size = ptr - (Byte *)dst.buffer;
    CPLDebug("MRF_LERC","LERC Compressed to %d\n", dst.size);
    return CE_None;
}

CPLErr DecompressLERC(buf_mgr &dst, buf_mgr &src, const ILImage &img)
{
    CntZImage zImg;
    Byte *ptr = (Byte *)src.buffer;
    if (!zImg.read(&ptr, 1e12))
    {
	CPLError(CE_Failure,CPLE_AppDefined,"MRF: Error during LERC decompression");
	return CE_Failure;
    }

// Unpack from zImg to dst buffer
#define UFILL(T) CntZImgUFill<T>(zImg, (T *)(dst.buffer), img)
    switch (img.dt) {
    case GDT_Byte:	UFILL(GByte);	break;
    case GDT_UInt16:	UFILL(GUInt16); break;
    case GDT_Int16:	UFILL(GInt16);	break;
    case GDT_Int32:	UFILL(GInt32);	break;
    case GDT_UInt32:	UFILL(GUInt32); break;
    case GDT_Float32:	UFILL(float);	break;
    case GDT_Float64:	UFILL(double);	break;
    }
#undef UFILL

    return CE_None;
}

CPLErr LERC_Band::Decompress(buf_mgr &dst, buf_mgr &src) 
{
    return DecompressLERC(dst, src, img);
}

CPLErr LERC_Band::Compress(buf_mgr &dst, buf_mgr &src, const ILImage &img) 
{
    return CompressLERC(dst, src, img, precision);
}

LERC_Band::LERC_Band(GDALMRFDataset *pDS, const ILImage &image, int b, int level): 
    GDALMRFRasterBand(pDS, image, b, level) 
{
    // Pick 1/1000 for floats and 0.5 losless for integers
    if (eDataType == GDT_Float32 || eDataType == GDT_Float64 )
        precision = strtod(GetOptionValue( "LERC_PREC" , ".001" ),0);
    else
	precision = strtod(GetOptionValue( "LERC_PREC" , ".5" ),0);

    // Enlarge the page buffer in this case, LERC may expand data
    pDS->SetPBuffer( 2 * image.pageSizeBytes);
}

LERC_Band::~LERC_Band()
{
}