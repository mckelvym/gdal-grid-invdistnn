/******************************************************************************
 * $Id$
 *
 * Project:  GDAL DEM Utilities
 * Purpose:  
 * Authors:  Matthew Perry, perrygeo at gmail.com
 *           Even Rouault, even dot rouault at mines dash paris dot org
 *           Howard Butler, hobu.inc at gmail.com
 *           Chris Yesson, chris dot yesson at ioz dot ac dot uk
 *
 ******************************************************************************
 * Copyright (c) 2006, 2009 Matthew Perry 
 * Copyright (c) 2009 Even Rouault
 * Portions derived from GRASS 4.1 (public domain) See 
 * http://trac.osgeo.org/gdal/ticket/2975 for more information regarding 
 * history of this code
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
 ****************************************************************************
 *
 * Slope and aspect calculations based on original method for GRASS GIS 4.1
 * by Michael Shapiro, U.S.Army Construction Engineering Research Laboratory
 *    Olga Waupotitsch, U.S.Army Construction Engineering Research Laboratory
 *    Marjorie Larson, U.S.Army Construction Engineering Research Laboratory
 * as found in GRASS's r.slope.aspect module.
 *
 * Horn's formula is used to find the first order derivatives in x and y directions
 * for slope and aspect calculations: Horn, B. K. P. (1981).
 * "Hill Shading and the Reflectance Map", Proceedings of the IEEE, 69(1):14-47. 
 *
 * Other reference :
 * Burrough, P.A. and McDonell, R.A., 1998. Principles of Geographical Information
 * Systems. p. 190.
 *
 * Shaded relief based on original method for GRASS GIS 4.1 by Jim Westervelt,
 * U.S. Army Construction Engineering Research Laboratory
 * as found in GRASS's r.shaded.relief (formerly shade.rel.sh) module.
 * ref: "r.mapcalc: An Algebra for GIS and Image Processing",
 * by Michael Shapiro and Jim Westervelt, U.S. Army Construction Engineering
 * Research Laboratory (March/1991)
 *
 * Color table of named colors and lookup code derived from src/libes/gis/named_colr.c
 * of GRASS 4.1
 *
 * TRI - Terrain Ruggedness Index is as descibed in Wilson et al (2007)
 * this is based on the method of Valentine et al. (2004)  
 * 
 * TPI - Topographic Position Index follows the description in Wilson et al (2007), following Weiss (2001)
 * The radius is fixed at 1 cell width/height
 * 
 * Roughness - follows the definition in Wilson et al. (2007), which follows Dartnell (2000)
 *
 * References for TRI/TPI/Roughness:
 * Dartnell, P. 2000. Applying Remote Sensing Techniques to map Seafloor 
 *  Geology/Habitat Relationships. Masters Thesis, San Francisco State 
 *  University, pp. 108.
 * Valentine, P. C., S. J. Fuller, L. A. Scully. 2004. Terrain Ruggedness 
 *  Analysis and Distribution of Boulder Ridges in the Stellwagen Bank National
 *  Marine Sanctuary Region (poster). Galway, Ireland: 5th International 
 *  Symposium on Marine Geological and Biological Habitat Mapping (GeoHAB), 
 *  May 2004.
 * Weiss, A. D. 2001. Topographic Positions and Landforms Analysis (poster), 
 *  ESRI International User Conference, July 2001. San Diego, CA: ESRI.
 * Wilson, M. F. J.; O'Connell, B.; Brown, C.; Guinan, J. C. & Grehan, A. J. 
 *  Multiscale terrain analysis of multibeam bathymetry data for habitat mapping
 *  on the continental slope Marine Geodesy, 2007, 30, 3-35
 ****************************************************************************/

#include <stdlib.h>
#include <math.h>

#include "cpl_conv.h"
#include "cpl_string.h"
#include "gdal.h"
#include "gdal_priv.h"

CPL_CVSID("$Id$");

#ifndef M_PI
# define M_PI  3.1415926535897932384626433832795
#endif

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( " Usage: \n"
            " - To generate a shaded relief map from any GDAL-supported elevation raster : \n\n"
            "     gdaldem hillshade input_dem output_hillshade \n"
            "                 [-z ZFactor (default=1)] [-s scale* (default=1)] \n"
            "                 [-az Azimuth (default=315)] [-alt Altitude (default=45)]\n"
            "                 [-b Band (default=1)] [-of format] [-co \"NAME=VALUE\"]* [-q]\n"
            "\n"
            " - To generates a slope map from any GDAL-supported elevation raster :\n\n"
            "     gdaldem slope input_dem output_slope_map \n"
            "                 [-p use percent slope (default=degrees)] [-s scale* (default=1)]\n"
            "                 [-b Band (default=1)] [-of format] [-co \"NAME=VALUE\"]* [-q]\n"
            "\n"
            " - To generate an aspect map from any GDAL-supported elevation raster\n"
            "   Outputs a 32-bit float tiff with pixel values from 0-360 indicating azimuth :\n\n"
            "     gdaldem aspect input_dem output_aspect_map \n"
            "                 [-trigonometric] [-zero_for_flat]\n"
            "                 [-b Band (default=1)] [-of format] [-co \"NAME=VALUE\"]* [-q]\n"
            "\n"
            " - To generate a color relief map from any GDAL-supported elevation raster\n"
            "     gdaldem color-relief input_dem color_text_file output_color_relief_map\n"
            "                 [-alpha] [-exact_color_entry | -nearest_color_entry]\n"
            "                 [-b Band (default=1)] [-of format] [-co \"NAME=VALUE\"]* [-q]\n"
            "     where color_text_file contains lines of the format \"elevation_value red green blue\"\n"
            "\n"
            " - To generate a Terrain Ruggedness Index (TRI) map from any GDAL-supported elevation raster\n"
            "     gdaldem TRI input_dem output_TRI_map\n"
            "                 [-b Band (default=1)] [-of format] [-co \"NAME=VALUE\"]* [-q]\n"
            "\n"
            " - To generate a Topographic Position Index (TPI) map from any GDAL-supported elevation raster\n"
            "     gdaldem TPI input_dem output_TPI_map\n"
            "                 [-b Band (default=1)] [-of format] [-co \"NAME=VALUE\"]* [-q]\n"
            "\n"
            " - To generate a roughness map from any GDAL-supported elevation raster\n"
            "     gdaldem roughness input_dem output_roughness_map\n"
            "                 [-b Band (default=1)] [-of format] [-co \"NAME=VALUE\"]* [-q]\n"
            "\n"
            " Notes : \n"
            "   Scale is the ratio of vertical units to horizontal\n"
            "    for Feet:Latlong use scale=370400, for Meters:LatLong use scale=111120 \n\n");
    exit( 1 );
}

/************************************************************************/
/*                  GDALGeneric3x3Processing()                          */
/************************************************************************/

typedef float (*GDALGeneric3x3ProcessingAlg) (float* pafWindow, float fDstNoDataValue, void* pData);

CPLErr GDALGeneric3x3Processing  ( GDALRasterBandH hSrcBand,
                                   GDALRasterBandH hDstBand,
                                   GDALGeneric3x3ProcessingAlg pfnAlg,
                                   void* pData,
                                   GDALProgressFunc pfnProgress,
                                   void * pProgressData)
{
    CPLErr eErr;
    float *pafThreeLineWin; /* 3 line rotating source buffer */
    float *pafOutputBuf;     /* 1 line destination buffer */
    int i, j;

    int bSrcHasNoData, bDstHasNoData;
    float fSrcNoDataValue = 0.0, fDstNoDataValue = 0.0;

    int nXSize = GDALGetRasterBandXSize(hSrcBand);
    int nYSize = GDALGetRasterBandYSize(hSrcBand);

    if (pfnProgress == NULL)
        pfnProgress = GDALDummyProgress;

/* -------------------------------------------------------------------- */
/*      Initialize progress counter.                                    */
/* -------------------------------------------------------------------- */
    if( !pfnProgress( 0.0, NULL, pProgressData ) )
    {
        CPLError( CE_Failure, CPLE_UserInterrupt, "User terminated" );
        return CE_Failure;
    }

    pafOutputBuf = (float *) CPLMalloc(sizeof(float)*nXSize);
    pafThreeLineWin  = (float *) CPLMalloc(3*sizeof(float)*(nXSize+1));

    fSrcNoDataValue = (float) GDALGetRasterNoDataValue(hSrcBand, &bSrcHasNoData);
    fDstNoDataValue = (float) GDALGetRasterNoDataValue(hDstBand, &bDstHasNoData);
    if (!bDstHasNoData)
        fDstNoDataValue = 0.0;

    // Move a 3x3 pafWindow over each cell 
    // (where the cell in question is #4)
    // 
    //      0 1 2
    //      3 4 5
    //      6 7 8

    /* Preload the first 2 lines */
    for ( i = 0; i < 2 && i < nYSize; i++)
    {
        GDALRasterIO(   hSrcBand,
                        GF_Read,
                        0, i,
                        nXSize, 1,
                        pafThreeLineWin + i * nXSize,
                        nXSize, 1,
                        GDT_Float32,
                        0, 0);
    }
    
    // Exclude the edges
    for (j = 0; j < nXSize; j++)
    {
        pafOutputBuf[j] = fDstNoDataValue;
    }
    GDALRasterIO(hDstBand, GF_Write,
                 0, 0, nXSize, 1,
                 pafOutputBuf, nXSize, 1, GDT_Float32, 0, 0);

    if (nYSize > 1)
    {
        GDALRasterIO(hDstBand, GF_Write,
                     0, nYSize - 1, nXSize, 1,
                     pafOutputBuf, nXSize, 1, GDT_Float32, 0, 0);
    }
    
    int nLine1Off = 0*nXSize;
    int nLine2Off = 1*nXSize;
    int nLine3Off = 2*nXSize;

    for ( i = 1; i < nYSize-1; i++)
    {
        /* Read third line of the line buffer */
        eErr = GDALRasterIO(   hSrcBand,
                        GF_Read,
                        0, i+1,
                        nXSize, 1,
                        pafThreeLineWin + nLine3Off,
                        nXSize, 1,
                        GDT_Float32,
                        0, 0);
        if (eErr != CE_None)
            goto end;

        // Exclude the edges
        pafOutputBuf[0] = fDstNoDataValue;
        if (nXSize > 1)
            pafOutputBuf[nXSize - 1] = fDstNoDataValue;
        
        for (j = 1; j < nXSize - 1; j++)
        {
            float afWin[9];
            afWin[0] = pafThreeLineWin[nLine1Off + j-1];
            afWin[1] = pafThreeLineWin[nLine1Off + j];
            afWin[2] = pafThreeLineWin[nLine1Off + j+1];
            afWin[3] = pafThreeLineWin[nLine2Off + j-1];
            afWin[4] = pafThreeLineWin[nLine2Off + j];
            afWin[5] = pafThreeLineWin[nLine2Off + j+1];
            afWin[6] = pafThreeLineWin[nLine3Off + j-1];
            afWin[7] = pafThreeLineWin[nLine3Off + j];
            afWin[8] = pafThreeLineWin[nLine3Off + j+1];

            if (bSrcHasNoData && (
                   afWin[0] == fSrcNoDataValue ||
                   afWin[1] == fSrcNoDataValue ||
                   afWin[2] == fSrcNoDataValue ||
                   afWin[3] == fSrcNoDataValue ||
                   afWin[4] == fSrcNoDataValue ||
                   afWin[5] == fSrcNoDataValue ||
                   afWin[6] == fSrcNoDataValue ||
                   afWin[7] == fSrcNoDataValue ||
                   afWin[8] == fSrcNoDataValue))
            {
                // We have nulls so write nullValue and move on
                pafOutputBuf[j] = fDstNoDataValue;
            }
            else
            {
                // We have a valid 3x3 window.
                pafOutputBuf[j] = pfnAlg(afWin, fDstNoDataValue, pData);
            }
        }

        /* -----------------------------------------
         * Write Line to Raster
         */
        eErr = GDALRasterIO(hDstBand, GF_Write, 0, i, nXSize, 1,
                     pafOutputBuf, nXSize, 1, GDT_Float32, 0, 0);
        if (eErr != CE_None)
            goto end;

        if( !pfnProgress( 1.0 * (i+1) / nYSize, NULL, pProgressData ) )
        {
            CPLError( CE_Failure, CPLE_UserInterrupt, "User terminated" );
            eErr = CE_Failure;
            goto end;
        }
        
        int nTemp = nLine1Off;
        nLine1Off = nLine2Off;
        nLine2Off = nLine3Off;
        nLine3Off = nTemp;
    }

    pfnProgress( 1.0, NULL, pProgressData );
    eErr = CE_None;

end:
    CPLFree(pafOutputBuf);
    CPLFree(pafThreeLineWin);

    return eErr;
}


/************************************************************************/
/*                         GDALHillshade()                              */
/************************************************************************/

typedef struct
{
    double nsres;
    double ewres;
    double sin_altRadians;
    double cos_altRadians_mul_z_scale_factor;
    double azRadians;
    double square_z_scale_factor;
} GDALHillshadeAlgData;

/* Unoptimized formulas are :
    x = psData->z*((afWin[0] + afWin[3] + afWin[3] + afWin[6]) -
        (afWin[2] + afWin[5] + afWin[5] + afWin[8])) /
        (8.0 * psData->ewres * psData->scale);

    y = psData->z*((afWin[6] + afWin[7] + afWin[7] + afWin[8]) -
        (afWin[0] + afWin[1] + afWin[1] + afWin[2])) /
        (8.0 * psData->nsres * psData->scale);

    slope = M_PI / 2 - atan(sqrt(x*x + y*y));

    aspect = atan2(x,y);

    cang = sin(alt * degreesToRadians) * sin(slope) +
           cos(alt * degreesToRadians) * cos(slope) *
           cos(az * degreesToRadians - M_PI/2 - aspect);
*/

float GDALHillshadeAlg (float* afWin, float fDstNoDataValue, void* pData)
{
    GDALHillshadeAlgData* psData = (GDALHillshadeAlgData*)pData;
    double x, y, aspect, xx_plus_yy, cang;
    
    // First Slope ...
    x = ((afWin[0] + afWin[3] + afWin[3] + afWin[6]) -
        (afWin[2] + afWin[5] + afWin[5] + afWin[8])) / psData->ewres;

    y = ((afWin[6] + afWin[7] + afWin[7] + afWin[8]) -
        (afWin[0] + afWin[1] + afWin[1] + afWin[2])) / psData->nsres;

    xx_plus_yy = x * x + y * y;

    // ... then aspect...
    aspect = atan2(x,y);

    // ... then the shade value
    cang = (psData->sin_altRadians -
           psData->cos_altRadians_mul_z_scale_factor * sqrt(xx_plus_yy) *
           sin(aspect - psData->azRadians)) /
           sqrt(1 + psData->square_z_scale_factor * xx_plus_yy);

    if (cang <= 0.0) 
        cang = 1.0;
    else
        cang = 1.0 + (254.0 * cang);
        
    return cang;
}

void*  GDALCreateHillshadeData(double* adfGeoTransform,
                               double z,
                               double scale,
                               double alt,
                               double az)
{
    GDALHillshadeAlgData* pData =
        (GDALHillshadeAlgData*)CPLMalloc(sizeof(GDALHillshadeAlgData));
        
    const double degreesToRadians = M_PI / 180.0;
    pData->nsres = adfGeoTransform[5];
    pData->ewres = adfGeoTransform[1];
    pData->sin_altRadians = sin(alt * degreesToRadians);
    pData->azRadians = az * degreesToRadians;
    double z_scale_factor = z / (8 * scale);
    pData->cos_altRadians_mul_z_scale_factor =
        cos(alt * degreesToRadians) * z_scale_factor;
    pData->square_z_scale_factor = z_scale_factor * z_scale_factor;
    return pData;
}

/************************************************************************/
/*                         GDALSlope()                                  */
/************************************************************************/

typedef struct
{
    double nsres;
    double ewres;
    double scale;
    int    slopeFormat;
} GDALSlopeAlgData;

float GDALSlopeAlg (float* afWin, float fDstNoDataValue, void* pData)
{
    const double radiansToDegrees = 180.0 / M_PI;
    GDALSlopeAlgData* psData = (GDALSlopeAlgData*)pData;
    double dx, dy, key;
    
    dx = ((afWin[0] + afWin[3] + afWin[3] + afWin[6]) - 
          (afWin[2] + afWin[5] + afWin[5] + afWin[8]))/psData->ewres;

    dy = ((afWin[6] + afWin[7] + afWin[7] + afWin[8]) - 
          (afWin[0] + afWin[1] + afWin[1] + afWin[2]))/psData->nsres;

    key = (dx * dx + dy * dy);

    if (psData->slopeFormat == 1) 
        return atan(sqrt(key) / (8*psData->scale)) * radiansToDegrees;
    else
        return 100*(sqrt(key) / (8*psData->scale));
}

void*  GDALCreateSlopeData(double* adfGeoTransform,
                           double scale,
                           int slopeFormat)
{
    GDALSlopeAlgData* pData =
        (GDALSlopeAlgData*)CPLMalloc(sizeof(GDALSlopeAlgData));
        
    pData->nsres = adfGeoTransform[5];
    pData->ewres = adfGeoTransform[1];
    pData->scale = scale;
    pData->slopeFormat = slopeFormat;
    return pData;
}

/************************************************************************/
/*                         GDALAspect()                                 */
/************************************************************************/

typedef struct
{
    int bAngleAsAzimuth;
} GDALAspectAlgData;

float GDALAspectAlg (float* afWin, float fDstNoDataValue, void* pData)
{
    const double degreesToRadians = M_PI / 180.0;
    GDALAspectAlgData* psData = (GDALAspectAlgData*)pData;
    double dx, dy;
    float aspect;
    
    dx = ((afWin[2] + afWin[5] + afWin[5] + afWin[8]) -
          (afWin[0] + afWin[3] + afWin[3] + afWin[6]));

    dy = ((afWin[6] + afWin[7] + afWin[7] + afWin[8]) - 
          (afWin[0] + afWin[1] + afWin[1] + afWin[2]));

    aspect = atan2(dy,-dx) / degreesToRadians;

    if (dx == 0 && dy == 0)
    {
        /* Flat area */
        aspect = fDstNoDataValue;
    } 
    else if ( psData->bAngleAsAzimuth )
    {
        if (aspect > 90.0) 
            aspect = 450.0 - aspect;
        else
            aspect = 90.0 - aspect;
    }
    else
    {
        if (aspect < 0)
            aspect += 360.0;
    }

    if (aspect == 360.0) 
        aspect = 0.0;

    return aspect;
}

void*  GDALCreateAspectData(int bAngleAsAzimuth)
{
    GDALAspectAlgData* pData =
        (GDALAspectAlgData*)CPLMalloc(sizeof(GDALAspectAlgData));
        
    pData->bAngleAsAzimuth = bAngleAsAzimuth;
    return pData;
}

/************************************************************************/
/*                      GDALColorRelief()                               */
/************************************************************************/

typedef struct
{
    double dfVal;
    int nR;
    int nG;
    int nB;
    int nA;
} ColorAssociation;

static int GDALColorReliefSortColors(const void* pA, const void* pB)
{
    ColorAssociation* pC1 = (ColorAssociation*)pA;
    ColorAssociation* pC2 = (ColorAssociation*)pB;
    return (pC1->dfVal < pC2->dfVal) ? -1 :
           (pC1->dfVal == pC2->dfVal) ? 0 : 1;
}

typedef enum
{
    COLOR_SELECTION_INTERPOLATE,
    COLOR_SELECTION_NEAREST_ENTRY,
    COLOR_SELECTION_EXACT_ENTRY
} ColorSelectionMode;

static int GDALColorReliefGetRGBA (ColorAssociation* pasColorAssociation,
                                   int nColorAssociation,
                                   double dfVal,
                                   ColorSelectionMode eColorSelectionMode,
                                   int* pnR,
                                   int* pnG,
                                   int* pnB,
                                   int* pnA)
{
    int i;
    int lower = 0;
    int upper = nColorAssociation - 1;
    int mid;

    /* Find the index of the first element in the LUT input array that */
    /* is not smaller than the dfVal value. */
    while(TRUE)
    {
        mid = (lower + upper) / 2;
        if (upper - lower <= 1)
        {
            if (dfVal < pasColorAssociation[lower].dfVal)
                i = lower;
            else if (dfVal < pasColorAssociation[upper].dfVal)
                i = upper;
            else
                i = upper + 1;
            break;
        }
        else if (pasColorAssociation[mid].dfVal >= dfVal)
        {
            upper = mid;
        }
        else
        {
            lower = mid;
        }
    }

    if (i == 0)
    {
        if (eColorSelectionMode == COLOR_SELECTION_EXACT_ENTRY &&
            pasColorAssociation[0].dfVal != dfVal)
        {
            *pnR = 0;
            *pnG = 0;
            *pnB = 0;
            *pnA = 0;
            return FALSE;
        }
        else
        {
            *pnR = pasColorAssociation[0].nR;
            *pnG = pasColorAssociation[0].nG;
            *pnB = pasColorAssociation[0].nB;
            *pnA = pasColorAssociation[0].nA;
            return TRUE;
        }
    }
    else if (i == nColorAssociation)
    {
        if (eColorSelectionMode == COLOR_SELECTION_EXACT_ENTRY &&
            pasColorAssociation[i-1].dfVal != dfVal)
        {
            *pnR = 0;
            *pnG = 0;
            *pnB = 0;
            *pnA = 0;
            return FALSE;
        }
        else
        {
            *pnR = pasColorAssociation[i-1].nR;
            *pnG = pasColorAssociation[i-1].nG;
            *pnB = pasColorAssociation[i-1].nB;
            *pnA = pasColorAssociation[i-1].nA;
            return TRUE;
        }
    }
    else
    {
        if (eColorSelectionMode == COLOR_SELECTION_EXACT_ENTRY &&
            pasColorAssociation[i-1].dfVal != dfVal)
        {
            *pnR = 0;
            *pnG = 0;
            *pnB = 0;
            *pnA = 0;
            return FALSE;
        }
        
        if (eColorSelectionMode == COLOR_SELECTION_NEAREST_ENTRY &&
            pasColorAssociation[i-1].dfVal != dfVal)
        {
            int index;
            if (dfVal - pasColorAssociation[i-1].dfVal <
                pasColorAssociation[i].dfVal - dfVal)
                index = i -1;
            else
                index = i;

            *pnR = pasColorAssociation[index].nR;
            *pnG = pasColorAssociation[index].nG;
            *pnB = pasColorAssociation[index].nB;
            *pnA = pasColorAssociation[index].nA;
            return TRUE;
        }
        
        double dfRatio = (dfVal - pasColorAssociation[i-1].dfVal) /
            (pasColorAssociation[i].dfVal - pasColorAssociation[i-1].dfVal);
        *pnR = (int)(0.45 + pasColorAssociation[i-1].nR + dfRatio *
                (pasColorAssociation[i].nR - pasColorAssociation[i-1].nR));
        if (*pnR < 0) *pnR = 0;
        else if (*pnR > 255) *pnR = 255;
        *pnG = (int)(0.45 + pasColorAssociation[i-1].nG + dfRatio *
                (pasColorAssociation[i].nG - pasColorAssociation[i-1].nG));
        if (*pnG < 0) *pnG = 0;
        else if (*pnG > 255) *pnG = 255;
        *pnB = (int)(0.45 + pasColorAssociation[i-1].nB + dfRatio *
                (pasColorAssociation[i].nB - pasColorAssociation[i-1].nB));
        if (*pnB < 0) *pnB = 0;
        else if (*pnB > 255) *pnB = 255;
        *pnA = (int)(0.45 + pasColorAssociation[i-1].nA + dfRatio *
                (pasColorAssociation[i].nA - pasColorAssociation[i-1].nA));
        if (*pnA < 0) *pnA = 0;
        else if (*pnA > 255) *pnA = 255;
        
        return TRUE;
    }
}

/* dfPct : percentage between 0 and 1 */
static double GDALColorReliefGetAbsoluteValFromPct(GDALRasterBandH hSrcBand,
                                                   double dfPct)
{
    double dfMin, dfMax;
    int bSuccessMin, bSuccessMax;
    dfMin = GDALGetRasterMinimum(hSrcBand, &bSuccessMin);
    dfMax = GDALGetRasterMaximum(hSrcBand, &bSuccessMax);
    if (!bSuccessMin || !bSuccessMax)
    {
        double dfMean, dfStdDev;
        fprintf(stderr, "Computing source raster statistics...\n");
        GDALComputeRasterStatistics(hSrcBand, FALSE, &dfMin, &dfMax,
                                    &dfMean, &dfStdDev, NULL, NULL);
    }
    return dfMin + dfPct * (dfMax - dfMin);
}

typedef struct
{
    const char *name;
    float r, g, b;
} NamedColor;

static const NamedColor namedColors[] = {
    { "white",  1.00, 1.00, 1.00 },
    { "black",  0.00, 0.00, 0.00 },
    { "red",    1.00, 0.00, 0.00 },
    { "green",  0.00, 1.00, 0.00 },
    { "blue",   0.00, 0.00, 1.00 },
    { "yellow", 1.00, 1.00, 0.00 },
    { "magenta",1.00, 0.00, 1.00 },
    { "cyan",   0.00, 1.00, 1.00 },
    { "aqua",   0.00, 0.75, 0.75 },
    { "grey",   0.75, 0.75, 0.75 },
    { "gray",   0.75, 0.75, 0.75 },
    { "orange", 1.00, 0.50, 0.00 },
    { "brown",  0.75, 0.50, 0.25 },
    { "purple", 0.50, 0.00, 1.00 },
    { "violet", 0.50, 0.00, 1.00 },
    { "indigo", 0.00, 0.50, 1.00 },
};

static
int GDALColorReliefFindNamedColor(const char *pszColorName, int *pnR, int *pnG, int *pnB)
{
    unsigned int i;

    *pnR = *pnG = *pnB = 0;
    for (i = 0; i < sizeof(namedColors) / sizeof(namedColors[0]); i++)
    {
        if (EQUAL(pszColorName, namedColors[i].name))
        {
            *pnR = (int)(255. * namedColors[i].r);
            *pnG = (int)(255. * namedColors[i].g);
            *pnB = (int)(255. * namedColors[i].b);
            return TRUE;
        }
    }
    return FALSE;
}

static
ColorAssociation* GDALColorReliefParseColorFile(GDALRasterBandH hSrcBand,
                                                const char* pszColorFilename,
                                                int* pnColors)
{
    FILE* fpColorFile = VSIFOpenL(pszColorFilename, "rt");
    if (fpColorFile == NULL)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Cannot find %s", pszColorFilename);
        *pnColors = 0;
        return NULL;
    }

    ColorAssociation* pasColorAssociation = NULL;
    int nColorAssociation = 0;

    int bSrcHasNoData = FALSE;
    double dfSrcNoDataValue = GDALGetRasterNoDataValue(hSrcBand, &bSrcHasNoData);

    const char* pszLine;
    while ((pszLine = CPLReadLineL(fpColorFile)) != NULL)
    {
        char** papszFields = CSLTokenizeStringComplex(pszLine, " ,\t:", 
                                                      FALSE, FALSE );
        /* Skip comment lines */
        int nTokens = CSLCount(papszFields);
        if (nTokens >= 2 &&
            papszFields[0][0] != '#' &&
            papszFields[0][0] != '/')
        {
            pasColorAssociation =
                    (ColorAssociation*)CPLRealloc(pasColorAssociation,
                           (nColorAssociation + 1) * sizeof(ColorAssociation));
            if (EQUAL(papszFields[0], "nv") && bSrcHasNoData)
                pasColorAssociation[nColorAssociation].dfVal = dfSrcNoDataValue;
            else if (strlen(papszFields[0]) > 1 && papszFields[0][strlen(papszFields[0])-1] == '%')
            {
                double dfPct = atof(papszFields[0]) / 100.;
                if (dfPct < 0.0 || dfPct > 1.0)
                {
                    CPLError(CE_Failure, CPLE_AppDefined,
                             "Wrong value for a percentage : %s", papszFields[0]);
                    CSLDestroy(papszFields);
                    VSIFCloseL(fpColorFile);
                    CPLFree(pasColorAssociation);
                    *pnColors = 0;
                    return NULL;
                }
                pasColorAssociation[nColorAssociation].dfVal =
                        GDALColorReliefGetAbsoluteValFromPct(hSrcBand, dfPct);
            }
            else
                pasColorAssociation[nColorAssociation].dfVal = atof(papszFields[0]);

            if (nTokens >= 4)
            {
                pasColorAssociation[nColorAssociation].nR = atoi(papszFields[1]);
                pasColorAssociation[nColorAssociation].nG = atoi(papszFields[2]);
                pasColorAssociation[nColorAssociation].nB = atoi(papszFields[3]);
                pasColorAssociation[nColorAssociation].nA =
                        (CSLCount(papszFields) >= 5 ) ? atoi(papszFields[4]) : 255;
            }
            else
            {
                int nR, nG, nB;
                if (!GDALColorReliefFindNamedColor(papszFields[1], &nR, &nG, &nB))
                {
                    CPLError(CE_Failure, CPLE_AppDefined,
                             "Unknown color : %s", papszFields[1]);
                    CSLDestroy(papszFields);
                    VSIFCloseL(fpColorFile);
                    CPLFree(pasColorAssociation);
                    *pnColors = 0;
                    return NULL;
                }
                pasColorAssociation[nColorAssociation].nR = nR;
                pasColorAssociation[nColorAssociation].nG = nG;
                pasColorAssociation[nColorAssociation].nB = nB;
                            pasColorAssociation[nColorAssociation].nA =
                    (CSLCount(papszFields) >= 3 ) ? atoi(papszFields[2]) : 255;
            }

            nColorAssociation ++;
        }
        CSLDestroy(papszFields);
    }
    VSIFCloseL(fpColorFile);

    if (nColorAssociation == 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "No color association found in %s", pszColorFilename);
        *pnColors = 0;
        return NULL;
    }

    qsort(pasColorAssociation, nColorAssociation,
          sizeof(ColorAssociation), GDALColorReliefSortColors);

    *pnColors = nColorAssociation;
    return pasColorAssociation;
}

static
GByte* GDALColorReliefPrecompute(GDALRasterBandH hSrcBand,
                                 ColorAssociation* pasColorAssociation,
                                 int nColorAssociation,
                                 ColorSelectionMode eColorSelectionMode,
                                 int* pnIndexOffset)
{
    GDALDataType eDT = GDALGetRasterDataType(hSrcBand);
    GByte* pabyPrecomputed = NULL;
    int nIndexOffset = (eDT == GDT_Int16) ? 32768 : 0;
    *pnIndexOffset = nIndexOffset;
    int nXSize = GDALGetRasterBandXSize(hSrcBand);
    int nYSize = GDALGetRasterBandXSize(hSrcBand);
    if (eDT == GDT_Byte ||
        ((eDT == GDT_Int16 || eDT == GDT_UInt16) && nXSize * nYSize > 65536))
    {
        int iMax = (eDT == GDT_Byte) ? 256: 65536;
        pabyPrecomputed = (GByte*) VSIMalloc(4 * iMax);
        if (pabyPrecomputed)
        {
            int i;
            for(i=0;i<iMax;i++)
            {
                int nR, nG, nB, nA;
                GDALColorReliefGetRGBA  (pasColorAssociation,
                                         nColorAssociation,
                                         i - nIndexOffset,
                                         eColorSelectionMode,
                                         &nR, &nG, &nB, &nA);
                pabyPrecomputed[4 * i] = (GByte) nR;
                pabyPrecomputed[4 * i + 1] = (GByte) nG;
                pabyPrecomputed[4 * i + 2] = (GByte) nB;
                pabyPrecomputed[4 * i + 3] = (GByte) nA;
            }
        }
    }
    return pabyPrecomputed;
}

/************************************************************************/
/* ==================================================================== */
/*                       GDALColorReliefDataset                        */
/* ==================================================================== */
/************************************************************************/

class GDALColorReliefRasterBand;

class GDALColorReliefDataset : public GDALDataset
{
    friend class GDALColorReliefRasterBand;

    GDALDatasetH       hSrcDS;
    GDALRasterBandH    hSrcBand;
    int                nColorAssociation;
    ColorAssociation*  pasColorAssociation;
    ColorSelectionMode eColorSelectionMode;
    GByte*             pabyPrecomputed;
    int                nIndexOffset;
    float*             pafSourceBuf;
    int*               panSourceBuf;
    int                nCurBlockXOff;
    int                nCurBlockYOff;

  public:
                        GDALColorReliefDataset(GDALDatasetH hSrcDS,
                                            GDALRasterBandH hSrcBand,
                                            const char* pszColorFilename,
                                            ColorSelectionMode eColorSelectionMode,
                                            int bAlpha);
                       ~GDALColorReliefDataset();

    CPLErr      GetGeoTransform( double * padfGeoTransform );
    const char *GetProjectionRef();
};

/************************************************************************/
/* ==================================================================== */
/*                    GDALColorReliefRasterBand                       */
/* ==================================================================== */
/************************************************************************/

class GDALColorReliefRasterBand : public GDALRasterBand
{
    friend class GDALColorReliefDataset;

    
  public:
                 GDALColorReliefRasterBand( GDALColorReliefDataset *, int );
    
    virtual CPLErr          IReadBlock( int, int, void * );
    virtual GDALColorInterp GetColorInterpretation();
};

GDALColorReliefDataset::GDALColorReliefDataset(
                                     GDALDatasetH hSrcDS,
                                     GDALRasterBandH hSrcBand,
                                     const char* pszColorFilename,
                                     ColorSelectionMode eColorSelectionMode,
                                     int bAlpha)
{
    this->hSrcDS = hSrcDS;
    this->hSrcBand = hSrcBand;
    nColorAssociation = 0;
    pasColorAssociation =
            GDALColorReliefParseColorFile(hSrcBand, pszColorFilename,
                                          &nColorAssociation);
    this->eColorSelectionMode = eColorSelectionMode;
    
    nRasterXSize = GDALGetRasterXSize(hSrcDS);
    nRasterYSize = GDALGetRasterYSize(hSrcDS);
    
    int nBlockXSize, nBlockYSize;
    GDALGetBlockSize( hSrcBand, &nBlockXSize, &nBlockYSize);
    
    nIndexOffset = 0;
    pabyPrecomputed =
        GDALColorReliefPrecompute(hSrcBand,
                                  pasColorAssociation,
                                  nColorAssociation,
                                  eColorSelectionMode,
                                  &nIndexOffset);
    
    int i;
    for(i=0;i<((bAlpha) ? 4 : 3);i++)
    {
        SetBand(i + 1, new GDALColorReliefRasterBand(this, i+1));
    }
    
    pafSourceBuf = NULL;
    panSourceBuf = NULL;
    if (pabyPrecomputed)
        panSourceBuf = (int *) CPLMalloc(sizeof(int)*nBlockXSize*nBlockYSize);
    else
        pafSourceBuf = (float *) CPLMalloc(sizeof(float)*nBlockXSize*nBlockYSize);
    nCurBlockXOff = -1;
    nCurBlockYOff = -1;
}

GDALColorReliefDataset::~GDALColorReliefDataset()
{
    CPLFree(pasColorAssociation);
    CPLFree(pabyPrecomputed);
    CPLFree(panSourceBuf);
    CPLFree(pafSourceBuf);
}

CPLErr GDALColorReliefDataset::GetGeoTransform( double * padfGeoTransform )
{
    return GDALGetGeoTransform(hSrcDS, padfGeoTransform);
}

const char *GDALColorReliefDataset::GetProjectionRef()
{
    return GDALGetProjectionRef(hSrcDS);
}

GDALColorReliefRasterBand::GDALColorReliefRasterBand(
                                    GDALColorReliefDataset * poDS, int nBand)
{
    this->poDS = poDS;
    this->nBand = nBand;
    eDataType = GDT_Byte;
    GDALGetBlockSize( poDS->hSrcBand, &nBlockXSize, &nBlockYSize);
}

CPLErr GDALColorReliefRasterBand::IReadBlock( int nBlockXOff,
                                              int nBlockYOff,
                                              void *pImage )
{
    GDALColorReliefDataset * poGDS = (GDALColorReliefDataset *) poDS;
    int nReqXSize, nReqYSize;

    if ((nBlockXOff + 1) * nBlockXSize >= nRasterXSize)
        nReqXSize = nRasterXSize - nBlockXOff * nBlockXSize;
    else
        nReqXSize = nBlockXSize;
        
    if ((nBlockYOff + 1) * nBlockYSize >= nRasterYSize)
        nReqYSize = nRasterYSize - nBlockYOff * nBlockYSize;
    else
        nReqYSize = nBlockYSize;

    int nCount = nReqXSize * nReqYSize;
    if ( poGDS->nCurBlockXOff != nBlockXOff ||
         poGDS->nCurBlockYOff != nBlockYOff )
    {
        poGDS->nCurBlockXOff = nBlockXOff;
        poGDS->nCurBlockYOff = nBlockYOff;
        
        CPLErr eErr = GDALRasterIO( poGDS->hSrcBand,
                            GF_Read,
                            nBlockXOff * nBlockXSize,
                            nBlockYOff * nBlockYSize,
                            nReqXSize, nReqYSize,
                            (poGDS->panSourceBuf) ?
                                (void*) poGDS->panSourceBuf :
                                (void* )poGDS->pafSourceBuf,
                            nReqXSize, nReqYSize,
                            (poGDS->panSourceBuf) ? GDT_Int32 : GDT_Float32,
                            0, 0);
        if (eErr != CE_None)
        {
            memset(pImage, 0, nCount);
            return eErr;
        }
    }

    int j;
    if (poGDS->panSourceBuf)
    {
        for ( j = 0; j < nCount; j++)
        {
            int nIndex = poGDS->panSourceBuf[j] + poGDS->nIndexOffset;
            ((GByte*)pImage)[j] = poGDS->pabyPrecomputed[4*nIndex + nBand-1];
        }
    }
    else
    {
        int anComponents[4];
        for ( j = 0; j < nCount; j++)
        {
            GDALColorReliefGetRGBA  (poGDS->pasColorAssociation,
                                     poGDS->nColorAssociation,
                                     poGDS->pafSourceBuf[j],
                                     poGDS->eColorSelectionMode,
                                     &anComponents[0],
                                     &anComponents[1],
                                     &anComponents[2],
                                     &anComponents[3]);
            ((GByte*)pImage)[j] = (GByte) anComponents[nBand-1];
        }
    }
    
    return CE_None;
}

GDALColorInterp GDALColorReliefRasterBand::GetColorInterpretation()
{
    return (GDALColorInterp)(GCI_RedBand + nBand - 1);
}


CPLErr GDALColorRelief (GDALRasterBandH hSrcBand,
                        GDALRasterBandH hDstBand1,
                        GDALRasterBandH hDstBand2,
                        GDALRasterBandH hDstBand3,
                        GDALRasterBandH hDstBand4,
                        const char* pszColorFilename,
                        ColorSelectionMode eColorSelectionMode,
                        GDALProgressFunc pfnProgress,
                        void * pProgressData)
{
    CPLErr eErr;
    
    if (hSrcBand == NULL || hDstBand1 == NULL || hDstBand2 == NULL ||
        hDstBand3 == NULL)
        return CE_Failure;

    int nColorAssociation = 0;
    ColorAssociation* pasColorAssociation =
            GDALColorReliefParseColorFile(hSrcBand, pszColorFilename,
                                          &nColorAssociation);
    if (pasColorAssociation == NULL)
        return CE_Failure;

    int nXSize = GDALGetRasterBandXSize(hSrcBand);
    int nYSize = GDALGetRasterBandYSize(hSrcBand);

    if (pfnProgress == NULL)
        pfnProgress = GDALDummyProgress;
        
    int nR = 0, nG = 0, nB = 0, nA = 0;
    
/* -------------------------------------------------------------------- */
/*      Precompute the map from values to RGBA quadruplets              */
/*      for GDT_Byte, GDT_Int16 or GDT_UInt16                           */
/* -------------------------------------------------------------------- */
    int nIndexOffset = 0;
    GByte* pabyPrecomputed =
        GDALColorReliefPrecompute(hSrcBand,
                                  pasColorAssociation,
                                  nColorAssociation,
                                  eColorSelectionMode,
                                  &nIndexOffset);

/* -------------------------------------------------------------------- */
/*      Initialize progress counter.                                    */
/* -------------------------------------------------------------------- */

    float* pafSourceBuf = NULL;
    int* panSourceBuf = NULL;
    if (pabyPrecomputed)
        panSourceBuf = (int *) CPLMalloc(sizeof(int)*nXSize);
    else
        pafSourceBuf = (float *) CPLMalloc(sizeof(float)*nXSize);
    GByte* pabyDestBuf1  = (GByte*) CPLMalloc( 4 * nXSize );
    GByte* pabyDestBuf2  =  pabyDestBuf1 + nXSize;
    GByte* pabyDestBuf3  =  pabyDestBuf2 + nXSize;
    GByte* pabyDestBuf4  =  pabyDestBuf3 + nXSize;
    int i, j;

    if( !pfnProgress( 0.0, NULL, pProgressData ) )
    {
        CPLError( CE_Failure, CPLE_UserInterrupt, "User terminated" );
        eErr = CE_Failure;
        goto end;
    }

    for ( i = 0; i < nYSize; i++)
    {
        /* Read source buffer */
        eErr = GDALRasterIO(   hSrcBand,
                        GF_Read,
                        0, i,
                        nXSize, 1,
                        (panSourceBuf) ? (void*) panSourceBuf : (void* )pafSourceBuf,
                        nXSize, 1,
                        (panSourceBuf) ? GDT_Int32 : GDT_Float32,
                        0, 0);
        if (eErr != CE_None)
            goto end;

        if (panSourceBuf)
        {
            for ( j = 0; j < nXSize; j++)
            {
                int nIndex = panSourceBuf[j] + nIndexOffset;
                pabyDestBuf1[j] = pabyPrecomputed[4 * nIndex];
                pabyDestBuf2[j] = pabyPrecomputed[4 * nIndex + 1];
                pabyDestBuf3[j] = pabyPrecomputed[4 * nIndex + 2];
                pabyDestBuf4[j] = pabyPrecomputed[4 * nIndex + 3];
            }
        }
        else
        {
            for ( j = 0; j < nXSize; j++)
            {
                GDALColorReliefGetRGBA  (pasColorAssociation,
                                         nColorAssociation,
                                         pafSourceBuf[j],
                                         eColorSelectionMode,
                                         &nR,
                                         &nG,
                                         &nB,
                                         &nA);
                pabyDestBuf1[j] = (GByte) nR;
                pabyDestBuf2[j] = (GByte) nG;
                pabyDestBuf3[j] = (GByte) nB;
                pabyDestBuf4[j] = (GByte) nA;
            }
        }
        
        /* -----------------------------------------
         * Write Line to Raster
         */
        eErr = GDALRasterIO(hDstBand1,
                      GF_Write,
                      0, i, nXSize,
                      1, pabyDestBuf1, nXSize, 1, GDT_Byte, 0, 0);
        if (eErr != CE_None)
            goto end;

        eErr = GDALRasterIO(hDstBand2,
                      GF_Write,
                      0, i, nXSize,
                      1, pabyDestBuf2, nXSize, 1, GDT_Byte, 0, 0);
        if (eErr != CE_None)
            goto end;
            
        eErr = GDALRasterIO(hDstBand3,
                      GF_Write,
                      0, i, nXSize,
                      1, pabyDestBuf3, nXSize, 1, GDT_Byte, 0, 0);
        if (eErr != CE_None)
            goto end;
            
        if (hDstBand4)
        {
            eErr = GDALRasterIO(hDstBand4,
                        GF_Write,
                        0, i, nXSize,
                        1, pabyDestBuf4, nXSize, 1, GDT_Byte, 0, 0);
            if (eErr != CE_None)
                goto end;
        }

        if( !pfnProgress( 1.0 * (i+1) / nYSize, NULL, pProgressData ) )
        {
            CPLError( CE_Failure, CPLE_UserInterrupt, "User terminated" );
            eErr = CE_Failure;
            goto end;
        }
    }
    pfnProgress( 1.0, NULL, pProgressData );
    eErr = CE_None;

end:
    VSIFree(pabyPrecomputed);
    CPLFree(pafSourceBuf);
    CPLFree(panSourceBuf);
    CPLFree(pabyDestBuf1);
    CPLFree(pasColorAssociation);

    return eErr;
}

/************************************************************************/
/*                         GDALTRIAlg()                                 */
/************************************************************************/

float GDALTRIAlg (float* afWin, float fDstNoDataValue, void* pData)
{
    // Terrain Ruggedness is average difference in height
    return (fabs(afWin[0]-afWin[4]) +
            fabs(afWin[1]-afWin[4]) +
            fabs(afWin[2]-afWin[4]) +
            fabs(afWin[3]-afWin[4]) +
            fabs(afWin[5]-afWin[4]) +
            fabs(afWin[6]-afWin[4]) +
            fabs(afWin[7]-afWin[4]) +
            fabs(afWin[8]-afWin[4]))/8;
}


/************************************************************************/
/*                         GDALTPIAlg()                                 */
/************************************************************************/

float GDALTPIAlg (float* afWin, float fDstNoDataValue, void* pData)
{
    // Terrain Position is the difference between
    // The central cell and the mean of the surrounding cells
    return afWin[4] - 
            ((afWin[0]+
              afWin[1]+
              afWin[2]+
              afWin[3]+
              afWin[5]+
              afWin[6]+
              afWin[7]+
              afWin[8])/8);
}

/************************************************************************/
/*                     GDALRoughnessAlg()                               */
/************************************************************************/

float GDALRoughnessAlg (float* afWin, float fDstNoDataValue, void* pData)
{
    // Roughness is the largest difference
    //  between any two cells

    float pafRoughnessMin = afWin[0];
    float pafRoughnessMax = afWin[0];

    for ( int k = 1; k < 9; k++)
    {
        if (afWin[k] > pafRoughnessMax)
        {
            pafRoughnessMax=afWin[k];
        }
        if (afWin[k] < pafRoughnessMin)
        {
            pafRoughnessMin=afWin[k];
        }
    }
    return pafRoughnessMax - pafRoughnessMin;
}

/************************************************************************/
/* ==================================================================== */
/*                       GDALGeneric3x3Dataset                        */
/* ==================================================================== */
/************************************************************************/

class GDALGeneric3x3RasterBand;

class GDALGeneric3x3Dataset : public GDALDataset
{
    friend class GDALGeneric3x3RasterBand;

    GDALGeneric3x3ProcessingAlg pfnAlg;
    void*              pAlgData;
    GDALDatasetH       hSrcDS;
    GDALRasterBandH    hSrcBand;
    float*             apafSourceBuf[3];
    int                bDstHasNoData;
    double             dfDstNoDataValue;
    int                nCurLine;

  public:
                        GDALGeneric3x3Dataset(GDALDatasetH hSrcDS,
                                              GDALRasterBandH hSrcBand,
                                              GDALDataType eDstDataType,
                                              int bDstHasNoData,
                                              double dfDstNoDataValue,
                                              GDALGeneric3x3ProcessingAlg pfnAlg,
                                              void* pAlgData);
                       ~GDALGeneric3x3Dataset();

    CPLErr      GetGeoTransform( double * padfGeoTransform );
    const char *GetProjectionRef();
};

/************************************************************************/
/* ==================================================================== */
/*                    GDALGeneric3x3RasterBand                       */
/* ==================================================================== */
/************************************************************************/

class GDALGeneric3x3RasterBand : public GDALRasterBand
{
    friend class GDALGeneric3x3Dataset;

    
  public:
                 GDALGeneric3x3RasterBand( GDALGeneric3x3Dataset *poDS,
                                           GDALDataType eDstDataType );
    
    virtual CPLErr          IReadBlock( int, int, void * );
    virtual double          GetNoDataValue( int* pbHasNoData );
};

GDALGeneric3x3Dataset::GDALGeneric3x3Dataset(
                                     GDALDatasetH hSrcDS,
                                     GDALRasterBandH hSrcBand,
                                     GDALDataType eDstDataType,
                                     int bDstHasNoData,
                                     double dfDstNoDataValue,
                                     GDALGeneric3x3ProcessingAlg pfnAlg,
                                     void* pAlgData)
{
    this->hSrcDS = hSrcDS;
    this->hSrcBand = hSrcBand;
    this->pfnAlg = pfnAlg;
    this->pAlgData = pAlgData;
    this->bDstHasNoData = bDstHasNoData;
    this->dfDstNoDataValue = dfDstNoDataValue;
    
    CPLAssert(eDstDataType == GDT_Byte || eDstDataType == GDT_Float32);

    nRasterXSize = GDALGetRasterXSize(hSrcDS);
    nRasterYSize = GDALGetRasterYSize(hSrcDS);
    
    SetBand(1, new GDALGeneric3x3RasterBand(this, eDstDataType));
    
    apafSourceBuf[0] = (float *) CPLMalloc(sizeof(float)*nRasterXSize);
    apafSourceBuf[1] = (float *) CPLMalloc(sizeof(float)*nRasterXSize);
    apafSourceBuf[2] = (float *) CPLMalloc(sizeof(float)*nRasterXSize);
    
    nCurLine = -1;
}

GDALGeneric3x3Dataset::~GDALGeneric3x3Dataset()
{
    CPLFree(apafSourceBuf[0]);
    CPLFree(apafSourceBuf[1]);
    CPLFree(apafSourceBuf[2]);
}

CPLErr GDALGeneric3x3Dataset::GetGeoTransform( double * padfGeoTransform )
{
    return GDALGetGeoTransform(hSrcDS, padfGeoTransform);
}

const char *GDALGeneric3x3Dataset::GetProjectionRef()
{
    return GDALGetProjectionRef(hSrcDS);
}

GDALGeneric3x3RasterBand::GDALGeneric3x3RasterBand(GDALGeneric3x3Dataset *poDS,
                                                   GDALDataType eDstDataType)
{
    this->poDS = poDS;
    this->nBand = 1;
    eDataType = eDstDataType;
    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

CPLErr GDALGeneric3x3RasterBand::IReadBlock( int nBlockXOff,
                                             int nBlockYOff,
                                             void *pImage )
{
    GDALGeneric3x3Dataset * poGDS = (GDALGeneric3x3Dataset *) poDS;
    
    if ( nBlockYOff == 0 || nBlockYOff == nRasterYSize - 1)
    {
        int j;
        if (eDataType == GDT_Byte)
        {
            for(j=0;j<nBlockXSize;j++)
                ((GByte*)pImage)[j] = (GByte) poGDS->dfDstNoDataValue;
        }
        else
        {
            for(j=0;j<nBlockXSize;j++)
                ((float*)pImage)[j] = (float) poGDS->dfDstNoDataValue;
        }
            
        return CE_None;
    }

    if ( poGDS->nCurLine != nBlockYOff )
    {
        if (nBlockYOff != 1 &&
            poGDS->nCurLine == nBlockYOff + 1)
        {
            float* pafTmp =  poGDS->apafSourceBuf[0];
            poGDS->apafSourceBuf[0] = poGDS->apafSourceBuf[1];
            poGDS->apafSourceBuf[1] = poGDS->apafSourceBuf[2];
            poGDS->apafSourceBuf[2] = pafTmp;
            
            poGDS->nCurLine = nBlockYOff;
            
            CPLErr eErr = GDALRasterIO( poGDS->hSrcBand,
                                    GF_Read,
                                    0, nBlockYOff + 1, nBlockXSize, 1,
                                    poGDS->apafSourceBuf[2],
                                    nBlockXSize, 1,
                                    GDT_Float32,
                                    0, 0);
                                    
            if (eErr != CE_None)
            {
                memset(pImage, 0, nBlockXSize * sizeof(float));
                return eErr;
            }
        }
        else
        {
            poGDS->nCurLine = nBlockYOff;
            int i;
            for(i=0;i<3;i++)
            {
                CPLErr eErr = GDALRasterIO( poGDS->hSrcBand,
                                    GF_Read,
                                    0, nBlockYOff + i - 1, nBlockXSize, 1,
                                    poGDS->apafSourceBuf[i],
                                    nBlockXSize, 1,
                                    GDT_Float32,
                                    0, 0);
                if (eErr != CE_None)
                {
                    memset(pImage, 0, nBlockXSize * sizeof(float));
                    return eErr;
                }
            }
        }
    }

    int j;
    
    if (eDataType == GDT_Byte)
    {
        ((GByte*)pImage)[0] = (GByte) poGDS->dfDstNoDataValue;
        if (nBlockXSize > 1)
            ((GByte*)pImage)[nBlockXSize - 1] = (GByte) poGDS->dfDstNoDataValue;
    }
    else
    {
        ((float*)pImage)[0] = (float) poGDS->dfDstNoDataValue;
        if (nBlockXSize > 1)
            ((float*)pImage)[nBlockXSize - 1] = (float) poGDS->dfDstNoDataValue;
    }
    
    int bSrcHasNoData;
    double dfSrcNoDataValue = GDALGetRasterNoDataValue(poGDS->hSrcBand,
                                                       &bSrcHasNoData);

    for(j=1;j<nBlockXSize - 1;j++)
    {
        float afWin[9];
        afWin[0] = poGDS->apafSourceBuf[0][j-1];
        afWin[1] = poGDS->apafSourceBuf[0][j];
        afWin[2] = poGDS->apafSourceBuf[0][j+1];
        afWin[3] = poGDS->apafSourceBuf[1][j-1];
        afWin[4] = poGDS->apafSourceBuf[1][j];
        afWin[5] = poGDS->apafSourceBuf[1][j+1];
        afWin[6] = poGDS->apafSourceBuf[2][j-1];
        afWin[7] = poGDS->apafSourceBuf[2][j];
        afWin[8] = poGDS->apafSourceBuf[2][j+1];

        // Check if afWin has null value
        int bContainsNull = FALSE;
        if (bSrcHasNoData)
        {
            for ( int n = 0; n <= 8; n++)
            {
                if(afWin[n] == dfSrcNoDataValue)
                {
                    bContainsNull = TRUE;
                    break;
                }
            }
        }

        if (bContainsNull)
        {
            // We have nulls so write nullValue and move on
            if (eDataType == GDT_Byte)
                ((GByte*)pImage)[j] = (GByte) poGDS->dfDstNoDataValue;
            else
                ((float*)pImage)[j] = (float) poGDS->dfDstNoDataValue;
        } else {
            // We have a valid 3x3 window.
            if (eDataType == GDT_Byte)
                ((GByte*)pImage)[j] = (GByte) (poGDS->pfnAlg(
                        afWin, poGDS->dfDstNoDataValue, poGDS->pAlgData) + 0.5);
            else
                ((float*)pImage)[j] = (float) poGDS->pfnAlg(
                        afWin, poGDS->dfDstNoDataValue, poGDS->pAlgData);
        }
    }
    
    return CE_None;
}

double GDALGeneric3x3RasterBand::GetNoDataValue( int* pbHasNoData )
{
    GDALGeneric3x3Dataset * poGDS = (GDALGeneric3x3Dataset *) poDS;
    if (pbHasNoData)
        *pbHasNoData = poGDS->bDstHasNoData;
    return poGDS->dfDstNoDataValue;
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

typedef enum
{
    HILL_SHADE,
    SLOPE,
    ASPECT,
    COLOR_RELIEF,
    TRI,
    TPI,
    ROUGHNESS
} Algorithm;

int main( int argc, char ** argv )

{
    Algorithm eUtilityMode;
    double z = 1.0;
    double scale = 1.0;
    double az = 315.0;
    double alt = 45.0;
    // 0 = 'percent' or 1 = 'degrees'
    int slopeFormat = 1; 
    int bAddAlpha = FALSE;
    int bZeroForFlat = FALSE;
    int bAngleAsAzimuth = TRUE;
    ColorSelectionMode eColorSelectionMode = COLOR_SELECTION_INTERPOLATE;
    
    int nBand = 1;
    double  adfGeoTransform[6];

    const char *pszSrcFilename = NULL;
    const char *pszDstFilename = NULL;
    const char *pszColorFilename = NULL;
    const char *pszFormat = "GTiff";
    char **papszCreateOptions = NULL;
    
    GDALDatasetH hSrcDataset = NULL;
    GDALDatasetH hDstDataset = NULL;
    GDALRasterBandH hSrcBand = NULL;
    GDALRasterBandH hDstBand = NULL;
    GDALDriverH hDriver = NULL;

    GDALProgressFunc pfnProgress = GDALTermProgress;
    
    int nXSize = 0;
    int nYSize = 0;
    
    /* Check strict compilation and runtime library version as we use C++ API */
    if (! GDAL_CHECK_VERSION(argv[0]))
        exit(1);

    argc = GDALGeneralCmdLineProcessor( argc, &argv, 0 );
    if( argc < 2 )
    {
        fprintf(stderr, "Not enough arguments\n");
        Usage();
        exit( 1 );
    }

    if( EQUAL(argv[1], "--utility_version") || EQUAL(argv[1], "--utility-version") )
    {
        printf("%s was compiled against GDAL %s and is running against GDAL %s\n",
                argv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
        return 0;
    }
    else if ( EQUAL(argv[1], "shade") || EQUAL(argv[1], "hillshade") )
    {
        eUtilityMode = HILL_SHADE;
    }
    else if ( EQUAL(argv[1], "slope") )
    {
        eUtilityMode = SLOPE;
    }
    else if ( EQUAL(argv[1], "aspect") )
    {
        eUtilityMode = ASPECT;
    }
    else if ( EQUAL(argv[1], "color-relief") )
    {
        eUtilityMode = COLOR_RELIEF;
    }
    else if ( EQUAL(argv[1], "TRI") )
    {
        eUtilityMode = TRI;
    }
    else if ( EQUAL(argv[1], "TPI") )
    {
        eUtilityMode = TPI;
    }
    else if ( EQUAL(argv[1], "roughness") )
    {
        eUtilityMode = ROUGHNESS;
    }
    else
    {
        fprintf(stderr, "Missing valid sub-utility mention\n");
        Usage();
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Parse arguments.                                                */
/* -------------------------------------------------------------------- */
    for(int i = 2; i < argc; i++ )
    {
        if( eUtilityMode == HILL_SHADE && i + 1 < argc &&
            (EQUAL(argv[i], "--z") || EQUAL(argv[i], "-z")))
        {
            z = atof(argv[++i]);
        }
        else if ( eUtilityMode == SLOPE && EQUAL(argv[i], "-p"))
        {
            slopeFormat = 0;
        }
        else if ( eUtilityMode == ASPECT && EQUAL(argv[i], "-trigonometric"))
        {
            bAngleAsAzimuth = FALSE;
        }
        else if ( eUtilityMode == ASPECT && EQUAL(argv[i], "-zero_for_flat"))
        {
            bZeroForFlat = TRUE;
        }
        else if ( eUtilityMode == COLOR_RELIEF && EQUAL(argv[i], "-exact_color_entry"))
        {
            eColorSelectionMode = COLOR_SELECTION_EXACT_ENTRY;
        }
        else if ( eUtilityMode == COLOR_RELIEF && EQUAL(argv[i], "-nearest_color_entry"))
        {
            eColorSelectionMode = COLOR_SELECTION_NEAREST_ENTRY;
        }
        else if( i + 1 < argc &&
            (EQUAL(argv[i], "--s") || 
             EQUAL(argv[i], "-s") ||
             EQUAL(argv[i], "--scale") ||
             EQUAL(argv[i], "-scale"))
          )
        {
            scale = atof(argv[++i]);
        }
        else if( eUtilityMode == HILL_SHADE && i + 1 < argc &&
            (EQUAL(argv[i], "--az") || 
             EQUAL(argv[i], "-az") ||
             EQUAL(argv[i], "--azimuth") ||
             EQUAL(argv[i], "-azimuth"))
          )
        {
            az = atof(argv[++i]);
        }
        else if( eUtilityMode == HILL_SHADE && i + 1 < argc &&
            (EQUAL(argv[i], "--alt") || 
             EQUAL(argv[i], "-alt") ||
             EQUAL(argv[i], "--alt") ||
             EQUAL(argv[i], "-alt"))
          )
        {
            alt = atof(argv[++i]);
        }
        else if( eUtilityMode == COLOR_RELIEF &&
                 EQUAL(argv[i], "-alpha"))
        {
            bAddAlpha = TRUE;
        }
        else if( i + 1 < argc &&
            (EQUAL(argv[i], "--b") || 
             EQUAL(argv[i], "-b"))
          )
        {
            nBand = atoi(argv[++i]);
        }
        else if ( EQUAL(argv[i], "-q") || EQUAL(argv[i], "-quiet") )
        {
            pfnProgress = GDALDummyProgress;
        }
        else if( EQUAL(argv[i],"-co") && i < argc-1 )
        {
            papszCreateOptions = CSLAddString( papszCreateOptions, argv[++i] );
        }
        else if( EQUAL(argv[i],"-of") && i < argc-1 )
        {
            pszFormat = argv[++i];
        }
        else if( argv[i][0] == '-' )
        {
            fprintf( stderr, "Option %s incomplete, or not recognised.\n\n", 
                    argv[i] );
            Usage();
            GDALDestroyDriverManager();
            exit( 2 );
        }
        else if( pszSrcFilename == NULL )
        {
            pszSrcFilename = argv[i];
        }
        else if( eUtilityMode == COLOR_RELIEF && pszColorFilename == NULL )
        {
            pszColorFilename = argv[i];
        }
        else if( pszDstFilename == NULL )
        {
            pszDstFilename = argv[i];
        }
        else
            Usage();
    }

    if( pszSrcFilename == NULL )
    {
        fprintf( stderr, "Missing source.\n\n" );
        Usage();
    }
    if ( eUtilityMode == COLOR_RELIEF && pszColorFilename == NULL )
    {
        fprintf( stderr, "Missing color file.\n\n" );
        Usage();
    }
    if( pszDstFilename == NULL )
    {
        fprintf( stderr, "Missing destination.\n\n" );
        Usage();
    }

    GDALAllRegister();

    // Open Dataset and get raster band
    hSrcDataset = GDALOpen( pszSrcFilename, GA_ReadOnly );
    
    if( hSrcDataset == NULL )
    {
        fprintf( stderr,
                 "GDALOpen failed - %d\n%s\n",
                 CPLGetLastErrorNo(), CPLGetLastErrorMsg() );
        GDALDestroyDriverManager();
        exit( 1 );
    }

    nXSize = GDALGetRasterXSize(hSrcDataset);
    nYSize = GDALGetRasterYSize(hSrcDataset);

    if( nBand <= 0 || nBand > GDALGetRasterCount(hSrcDataset) )
    {
        fprintf( stderr,
                 "Unable to fetch band #%d\n", nBand );
        GDALDestroyDriverManager();
        exit( 1 );
    }
    hSrcBand = GDALGetRasterBand( hSrcDataset, nBand );

    GDALGetGeoTransform(hSrcDataset, adfGeoTransform);

    hDriver = GDALGetDriverByName(pszFormat);
    if( hDriver == NULL 
        || (GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) == NULL &&
            GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATECOPY, NULL ) == NULL))
    {
        int	iDr;

        fprintf( stderr, "Output driver `%s' not recognised or does not support\n", 
                 pszFormat );
        fprintf( stderr, "direct output file creation.  The following format drivers are configured\n"
                "and support direct output:\n" );

        for( iDr = 0; iDr < GDALGetDriverCount(); iDr++ )
        {
            GDALDriverH hDriver = GDALGetDriver(iDr);

            if( GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) == NULL &&
                GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATECOPY, NULL ) == NULL )
            {
                printf( "  %s: %s\n",
                        GDALGetDriverShortName( hDriver  ),
                        GDALGetDriverLongName( hDriver ) );
            }
        }
        GDALDestroyDriverManager();
        exit( 1 );
    }

    double dfDstNoDataValue = 0;
    int bDstHasNoData = FALSE;
    void* pData = NULL;
    GDALGeneric3x3ProcessingAlg pfnAlg = NULL;

    if (eUtilityMode == HILL_SHADE)
    {
        dfDstNoDataValue = 0;
        bDstHasNoData = TRUE;
        pData = GDALCreateHillshadeData   (adfGeoTransform,
                                           z,
                                           scale,
                                           alt,
                                           az);
        pfnAlg = GDALHillshadeAlg;
    }
    else if (eUtilityMode == SLOPE)
    {
        dfDstNoDataValue = -9999;
        bDstHasNoData = TRUE;

        pData = GDALCreateSlopeData(adfGeoTransform, scale, slopeFormat);
        pfnAlg = GDALSlopeAlg;
    }

    else if (eUtilityMode == ASPECT)
    {
        if (!bZeroForFlat)
        {
            dfDstNoDataValue = -9999;
            bDstHasNoData = TRUE;
        }

        pData = GDALCreateAspectData(bAngleAsAzimuth);
        pfnAlg = GDALAspectAlg;
    }
    else if (eUtilityMode == TRI)
    {
        dfDstNoDataValue = -9999;
        bDstHasNoData = TRUE;
        pfnAlg = GDALTRIAlg;
    }
    else if (eUtilityMode == TPI)
    {
        dfDstNoDataValue = -9999;
        bDstHasNoData = TRUE;
        pfnAlg = GDALTPIAlg;
    }
    else if (eUtilityMode == ROUGHNESS)
    {
        dfDstNoDataValue = -9999;
        bDstHasNoData = TRUE;
        pfnAlg = GDALRoughnessAlg;
    }
    
    GDALDataType eDstDataType = (eUtilityMode == HILL_SHADE ||
                                 eUtilityMode == COLOR_RELIEF) ? GDT_Byte :
                                                               GDT_Float32;
    
    if( GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) == NULL &&
        GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATECOPY, NULL ) != NULL)
    {
        GDALDatasetH hIntermediateDataset;
        
        if (eUtilityMode == COLOR_RELIEF)
            hIntermediateDataset = (GDALDatasetH)
                new GDALColorReliefDataset (hSrcDataset,
                                            hSrcBand,
                                            pszColorFilename,
                                            eColorSelectionMode,
                                            bAddAlpha);
        else
            hIntermediateDataset = (GDALDatasetH)
                new GDALGeneric3x3Dataset(hSrcDataset, hSrcBand,
                                          eDstDataType,
                                          bDstHasNoData,
                                          dfDstNoDataValue,
                                          pfnAlg,
                                          pData);

        GDALDatasetH hOutDS = GDALCreateCopy(
                                 hDriver, pszDstFilename, hIntermediateDataset, 
                                 TRUE, papszCreateOptions, 
                                 pfnProgress, NULL );

        if( hOutDS != NULL )
            GDALClose( hOutDS );
        GDALClose(hIntermediateDataset);
        GDALClose(hSrcDataset);
        
        CPLFree(pData);

        GDALDestroyDriverManager();
        CSLDestroy( argv );
        CSLDestroy( papszCreateOptions );
        
        return 0;
    }

    int nDstBands;
    if (eUtilityMode == COLOR_RELIEF)
        nDstBands = (bAddAlpha) ? 4 : 3;
    else
        nDstBands = 1;

    hDstDataset = GDALCreate(   hDriver,
                                pszDstFilename,
                                nXSize,
                                nYSize,
                                nDstBands,
                                eDstDataType,
                                papszCreateOptions);

    if( hDstDataset == NULL )
    {
        fprintf( stderr,
                 "Unable to create dataset %s %d\n%s\n", pszDstFilename,
                 CPLGetLastErrorNo(), CPLGetLastErrorMsg() );
        GDALDestroyDriverManager();
        exit( 1 );
    }
    
    hDstBand = GDALGetRasterBand( hDstDataset, 1 );

    GDALSetGeoTransform(hDstDataset, adfGeoTransform);
    GDALSetProjection(hDstDataset, GDALGetProjectionRef(hSrcDataset));
    
    if (eUtilityMode == COLOR_RELIEF)
    {
        GDALColorRelief (hSrcBand, 
                         GDALGetRasterBand(hDstDataset, 1),
                         GDALGetRasterBand(hDstDataset, 2),
                         GDALGetRasterBand(hDstDataset, 3),
                         (bAddAlpha) ? GDALGetRasterBand(hDstDataset, 4) : NULL,
                         pszColorFilename,
                         eColorSelectionMode,
                         pfnProgress, NULL);
    }
    else
    {
        if (bDstHasNoData)
            GDALSetRasterNoDataValue(hDstBand, dfDstNoDataValue);
        
        GDALGeneric3x3Processing(hSrcBand, hDstBand,
                                 pfnAlg, pData,
                                 pfnProgress, NULL);
                                    
    }

    GDALClose(hSrcDataset);
    GDALClose(hDstDataset);
    CPLFree(pData);

    GDALDestroyDriverManager();
    CSLDestroy( argv );
    CSLDestroy( papszCreateOptions );

    return 0;
}

