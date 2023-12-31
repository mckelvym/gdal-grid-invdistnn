/******************************************************************************
 * $Id$
 *
 * Name:     gdalconst.i
 * Project:  GDAL Python Interface
 * Purpose:  GDAL constant declarations.
 * Author:   Kevin Ruland, kruland@ku.edu
 *
 ******************************************************************************
 * Copyright (c) 2005, Kevin Ruland
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

#ifdef PERL_CPAN_NAMESPACE
%module "Geo::GDAL::Const"
#elif defined(SWIGCSHARP)
%module GdalConst
#else
%module gdalconst 
#endif

#if defined(SWIGJAVA)
%include gdalconst_java.i
#endif

%{
#include "gdal.h"
#include "gdalwarper.h"
#include "cpl_string.h"
#include "cpl_minixml.h"
%}

// GDALDataType
%constant GDT_Unknown   = GDT_Unknown;
%constant GDT_Byte      = GDT_Byte;
%constant GDT_UInt16    = GDT_UInt16;
%constant GDT_Int16     = GDT_Int16;
%constant GDT_UInt32    = GDT_UInt32;
%constant GDT_Int32     = GDT_Int32;
%constant GDT_Float32   = GDT_Float32;
%constant GDT_Float64   = GDT_Float64;
%constant GDT_CInt16    = GDT_CInt16;
%constant GDT_CInt32    = GDT_CInt32;
%constant GDT_CFloat32  = GDT_CFloat32;
%constant GDT_CFloat64  = GDT_CFloat64;
%constant GDT_TypeCount = GDT_TypeCount;

// GDALAccess
%constant GA_ReadOnly = GA_ReadOnly;
%constant GA_Update   = GA_Update;

// GDALRWFlag
%constant GF_Read  = GF_Read;
%constant GF_Write = GF_Write;

// GDALColorInterp
%constant GCI_Undefined     = GCI_Undefined;
%constant GCI_GrayIndex     = GCI_GrayIndex;
%constant GCI_PaletteIndex  = GCI_PaletteIndex;
%constant GCI_RedBand       = GCI_RedBand;
%constant GCI_GreenBand     = GCI_GreenBand;
%constant GCI_BlueBand      = GCI_BlueBand;
%constant GCI_AlphaBand     = GCI_AlphaBand;
%constant GCI_HueBand       = GCI_HueBand;
%constant GCI_SaturationBand= GCI_SaturationBand;
%constant GCI_LightnessBand = GCI_LightnessBand;
%constant GCI_CyanBand      = GCI_CyanBand;
%constant GCI_MagentaBand   = GCI_MagentaBand;
%constant GCI_YellowBand    = GCI_YellowBand;
%constant GCI_BlackBand     = GCI_BlackBand;
%constant GCI_YCbCr_YBand     = GCI_YCbCr_YBand;
%constant GCI_YCbCr_CrBand     = GCI_YCbCr_CrBand;
%constant GCI_YCbCr_CbBand     = GCI_YCbCr_CbBand;

// GDALResampleAlg

%constant GRA_NearestNeighbour = GRA_NearestNeighbour;
%constant GRA_Bilinear         = GRA_Bilinear;
%constant GRA_Cubic            = GRA_Cubic;
%constant GRA_CubicSpline      = GRA_CubicSpline;
%constant GRA_Lanczos          = GRA_Lanczos;

// GDALPaletteInterp
%constant GPI_Gray  = GPI_Gray;
%constant GPI_RGB   = GPI_RGB;
%constant GPI_CMYK  = GPI_CMYK;
%constant GPI_HLS   = GPI_HLS;

%constant CXT_Element   = CXT_Element;
%constant CXT_Text      = CXT_Text;
%constant CXT_Attribute = CXT_Attribute;
%constant CXT_Comment   = CXT_Comment;
%constant CXT_Literal   = CXT_Literal;

// Error classes

%constant CE_None    = CE_None;
%constant CE_Debug   = CE_Debug;
%constant CE_Warning = CE_Warning;
%constant CE_Failure = CE_Failure;
%constant CE_Fatal   = CE_Fatal;

// Error codes

%constant CPLE_None                       = CPLE_None;
%constant CPLE_AppDefined                 = CPLE_AppDefined;
%constant CPLE_OutOfMemory                = CPLE_OutOfMemory;
%constant CPLE_FileIO                     = CPLE_FileIO;
%constant CPLE_OpenFailed                 = CPLE_OpenFailed;
%constant CPLE_IllegalArg                 = CPLE_IllegalArg;
%constant CPLE_NotSupported               = CPLE_NotSupported;
%constant CPLE_AssertionFailed            = CPLE_AssertionFailed;
%constant CPLE_NoWriteAccess              = CPLE_NoWriteAccess;
%constant CPLE_UserInterrupt              = CPLE_UserInterrupt;

%constant char *DMD_LONGNAME           = GDAL_DMD_LONGNAME;
%constant char *DMD_HELPTOPIC          = GDAL_DMD_HELPTOPIC;
%constant char *DMD_MIMETYPE           = GDAL_DMD_MIMETYPE;
%constant char *DMD_EXTENSION          = GDAL_DMD_EXTENSION;
%constant char *DMD_CREATIONOPTIONLIST = GDAL_DMD_CREATIONOPTIONLIST;
%constant char *DMD_CREATIONDATATYPES  = GDAL_DMD_CREATIONDATATYPES;

%constant char *DCAP_CREATE     = GDAL_DCAP_CREATE;
%constant char *DCAP_CREATECOPY = GDAL_DCAP_CREATECOPY;
%constant char *DCAP_VIRTUALIO = GDAL_DCAP_VIRTUALIO;

%constant CPLES_BackslashQuotable = CPLES_BackslashQuotable;
%constant CPLES_XML               = CPLES_XML;
%constant CPLES_URL               = CPLES_URL;
%constant CPLES_SQL               = CPLES_SQL;
%constant CPLES_CSV               = CPLES_CSV;

// GDALRATFieldType
%constant GFT_Integer             = GFT_Integer;
%constant GFT_Real                = GFT_Real;   
%constant GFT_String              = GFT_String;  

// GDALRATFieldUsage
%constant GFU_Generic             = GFU_Generic;
%constant GFU_PixelCount          = GFU_PixelCount;
%constant GFU_Name                = GFU_Name;
%constant GFU_Min                 = GFU_Min;
%constant GFU_Max                 = GFU_Max;
%constant GFU_MinMax              = GFU_MinMax;
%constant GFU_Red                 = GFU_Red;
%constant GFU_Green               = GFU_Green;
%constant GFU_Blue                = GFU_Blue;
%constant GFU_Alpha               = GFU_Alpha;
%constant GFU_RedMin              = GFU_RedMin;
%constant GFU_GreenMin            = GFU_GreenMin;
%constant GFU_BlueMin             = GFU_BlueMin;
%constant GFU_AlphaMin            = GFU_AlphaMin;
%constant GFU_RedMax              = GFU_RedMax;
%constant GFU_GreenMax            = GFU_GreenMax;
%constant GFU_BlueMax             = GFU_BlueMax;
%constant GFU_AlphaMax            = GFU_AlphaMax;
%constant GFU_MaxCount            = GFU_MaxCount;

%constant GMF_ALL_VALID           = 0x01;
%constant GMF_PER_DATASET         = 0x02;
%constant GMF_ALPHA               = 0x04;
%constant GMF_NODATA              = 0x08;

