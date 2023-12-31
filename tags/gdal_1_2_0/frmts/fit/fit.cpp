/******************************************************************************
 * $Id$
 *
 * Project:  FIT Driver
 * Purpose:  Implement FIT Support - not using the SGI iflFIT library.
 * Author:   Philip Nemec, nemec@keyholecorp.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Keyhole, Inc.
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
 * Revision 1.4  2001/07/18 04:51:56  warmerda
 * added CPL_CVSID
 *
 * Revision 1.3  2001/07/12 00:49:18  nemec
 * Convert unknown colorInterp based on number of bands for happier .fit files
 *
 * Revision 1.2  2001/07/06 18:46:25  nemec
 * Cleanup files - improve Windows build, make proper copyright notice
 *
 *
 */

#include <limits.h>
#include "fit.h"

CPL_CVSID("$Id$");

GDALDataType fitDataType(int dtype) {
    switch (dtype) {
    case 1: // iflBit   /* single-bit */
        fprintf(stderr, 
                "GDAL unsupported data type (single-bit) in fitDataType\n");
        return GDT_Unknown;
    case 2: // iflUChar    /* unsigned character (byte) */
        return GDT_Byte;
    case 4: // iflChar     /* signed character (byte) */
        fprintf(stderr, 
                "GDAL unsupported data type (signed char) in fitDataType\n");
        return GDT_Unknown;
//         return Byte;
    case 8: // iflUShort   /* unsigned short integer (nominally 16 bits) */
        return GDT_UInt16;
    case 16: // iflShort   /* signed short integer */
        return GDT_Int16;
    case 32: // iflUInt    /* unsigned integer (nominally 32 bits) */
//     case 32: // iflULong   /* deprecated, same as iflUInt */
        return GDT_UInt32;
        break;
    case 64: // iflInt     /* integer */
//     case 64: // iflLong    /* deprecated, same as iflULong */
        return GDT_Int32;
    case 128: // iflFloat  /* floating point */
        return GDT_Float32;
    case 256: // iflDouble /* double precision floating point */
        return GDT_Float64;
    default:
        CPLError(CE_Failure, CPLE_NotSupported, 
                 "FIT - unknown data type %i in fitDataType", dtype);
        return GDT_Unknown;
    } // switch
}

int fitGetDataType(GDALDataType eDataType) {
    switch (eDataType) {
    case GDT_Byte:
        return 2; // iflUChar - unsigned character (byte)
    case GDT_UInt16:
        return 8; // iflUShort - unsigned short integer (nominally 16 bits)
    case GDT_Int16:
        return 16; // iflShort - signed short integer
    case GDT_UInt32:
        return 32; // iflUInt - unsigned integer (nominally 32 bits)
    case GDT_Int32:
        return 64; // iflInt - integer
    case GDT_Float32:
        return 128; // iflFloat - floating point
    case GDT_Float64:
        return 256; // iflDouble - double precision floating point
    default:
        CPLError(CE_Failure, CPLE_NotSupported, 
                 "FIT - unsupported GDALDataType %i in fitGetDataType",
                 eDataType);
        return 0;
    } // switch
}

#define UNSUPPORTED_COMBO() \
            CPLError(CE_Failure, CPLE_NotSupported, \
                     "FIT write - unsupported combination (band 1 = %s " \
                     "and %i bands) - ignoring color model", \
                     GDALGetColorInterpretationName(colorInterp), nBands); \
            return 0


int fitGetColorModel(GDALColorInterp colorInterp, int nBands) {
    // XXX - shoould check colorInterp for all bands, not just first one

    switch(colorInterp) {
    case GCI_GrayIndex:
        switch (nBands) {
        case 1:
            return 2; // iflLuminance - luminance
        case 2:
            return 13; // iflLuminanceAlpha - Luminance plus alpha
        default:
            UNSUPPORTED_COMBO();
        } // switch

    case GCI_PaletteIndex:
        CPLError(CE_Failure, CPLE_NotSupported, 
                 "FIT write - unsupported ColorInterp PaletteIndex\n");
        return 0;

    case GCI_RedBand:
        switch (nBands) {
        case 3:
            return 3; // iflRGB - full color (Red, Green, Blue triplets)
        case 4:
            return 5; // iflRGBA - full color with transparency (alpha channel)
        default:
            UNSUPPORTED_COMBO();
        } // switch

    case GCI_BlueBand:
        switch (nBands) {
        case 3:
            return 9; // iflBGR - full color (ordered Blue, Green, Red)
        default:
            UNSUPPORTED_COMBO();
        } // switch

    case GCI_AlphaBand:
        switch (nBands) {
        case 4:
            return 10; // iflABGR - Alpha, Blue, Green, Red (SGI frame buffers)
        default:
            UNSUPPORTED_COMBO();
        } // switch

    case GCI_HueBand:
        switch (nBands) {
        case 3:
            return 6; // iflHSV - Hue, Saturation, Value
        default:
            UNSUPPORTED_COMBO();
        } // switch

    case GCI_CyanBand:
        switch (nBands) {
        case 3:
            return 7; // iflCMY - Cyan, Magenta, Yellow
        case 4:
            return 8; // iflCMYK - Cyan, Magenta, Yellow, Black
        default:
            UNSUPPORTED_COMBO();
        } // switch
        
    case GCI_GreenBand:
    case GCI_SaturationBand:
    case GCI_LightnessBand:
    case GCI_MagentaBand:
    case GCI_YellowBand:
    case GCI_BlackBand:
        CPLError(CE_Failure, CPLE_NotSupported, 
                 "FIT write - unsupported combination (band 1 = %s) "
                 "- ignoring color model",
                 GDALGetColorInterpretationName(colorInterp), nBands);
        return 0;

    default:
        CPLDebug("FIT write", "unrecognized colorInterp %i - deriving from "
                 "number of bands (%i)", colorInterp, nBands);
        switch (nBands) {
        case 1:
            return 2; // iflLuminance - luminance
        case 2:
            return 13; // iflLuminanceAlpha - Luminance plus alpha
        case 3:
            return 3; // iflRGB - full color (Red, Green, Blue triplets)
        case 4:
            return 5; // iflRGBA - full color with transparency (alpha channel)
        } // switch

        CPLError(CE_Failure, CPLE_NotSupported, 
                 "FIT write - unrecognized colorInterp %i and "
                 "unrecognized number of bands (%i)", colorInterp, nBands);

        return 0;
    } // switch
}
