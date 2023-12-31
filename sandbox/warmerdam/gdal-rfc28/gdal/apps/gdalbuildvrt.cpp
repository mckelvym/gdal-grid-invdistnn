/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Utilities
 * Purpose:  Commandline application to build VRT datasets from raster products or content of SHP tile index
 * Author:   Even Rouault, even.rouault at mines-paris.org
 *
 ******************************************************************************
 * Copyright (c) 2007, Even Rouault
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
 ****************************************************************************/

#include "gdal_proxy.h"
#include "cpl_string.h"
#include "vrt/gdal_vrt.h"

#ifdef OGR_ENABLED
#include "ogr_api.h"
#endif

CPL_CVSID("$Id$");

#define GEOTRSFRM_TOPLEFT_X            0
#define GEOTRSFRM_WE_RES               1
#define GEOTRSFRM_ROTATION_PARAM1      2
#define GEOTRSFRM_TOPLEFT_Y            3
#define GEOTRSFRM_ROTATION_PARAM2      4
#define GEOTRSFRM_NS_RES               5

typedef enum
{
    LOWEST_RESOLUTION,
    HIGHEST_RESOLUTION,
    AVERAGE_RESOLUTION,
    USER_RESOLUTION
} ResolutionStrategy;

typedef struct
{
    int    isFileOK;
    int    nRasterXSize;
    int    nRasterYSize;
    double adfGeoTransform[6];
    int    nBlockXSize;
    int    nBlockYSize;
    GDALDataType firstBandType;
    int*         panHasNoData;
    double*      padfNoDataValues;
} DatasetProperty;

typedef struct
{
    GDALColorInterp        colorInterpretation;
    GDALDataType           dataType;
    GDALColorTableH        colorTable;
    int                    bHasNoData;
    double                 noDataValue;
} BandProperty;

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    fprintf(stdout, "%s", 
            "Usage: gdalbuildvrt [-tileindex field_name] [-resolution {highest|lowest|average|user}]\n"
            "                    [-tr xres yres] [-separate] [-allow_projection_difference] [-q]\n"
            "                    [-te xmin ymin xmax ymax] [-addalpha] [-hidenodata] \n"
            "                    [-srcnodata \"value [value...]\"] [-vrtnodata \"value [value...]\"] \n"
            "                    [-input_file_list my_liste.txt] output.vrt [gdalfile]*\n"
            "\n"
            "eg.\n"
            "  % gdalbuildvrt doq_index.vrt doq/*.tif\n"
            "  % gdalbuildvrt -input_file_list my_liste.txt doq_index.vrt\n"
            "\n"
            "NOTES:\n"
            "  o With -separate, each files goes into a separate band in the VRT band. Otherwise,\n"
            "    the files are considered as tiles of a larger mosaic.\n"
            "  o The default tile index field is 'location' unless otherwise specified by -tileindex.\n"
            "  o In case the resolution of all input files is not the same, the -resolution flag.\n"
            "    enable the user to control the way the output resolution is computed. average is the default.\n"
            "  o Input files may be any valid GDAL dataset or a GDAL raster tile index.\n"
            "  o For a GDAL raster tile index, all entries will be added to the VRT.\n"
            "  o If one GDAL dataset is made of several subdatasets and has 0 raster bands, its\n"
            "    datasets will be added to the VRT rather than the dataset itself.\n"
            "  o By default, only datasets of same projection and band characteristics may be added to the VRT.\n"
            );
    exit( 1 );
}


/************************************************************************/
/*                         GetSrcDstWin()                               */
/************************************************************************/

int  GetSrcDstWin(DatasetProperty* psDP,
                  double we_res, double ns_res,
                  double minX, double minY, double maxX, double maxY,
                  int* pnSrcXOff, int* pnSrcYOff, int* pnSrcXSize, int* pnSrcYSize,
                  int* pnDstXOff, int* pnDstYOff, int* pnDstXSize, int* pnDstYSize)
{
    /* Check that the destination bounding box intersects the source bounding box */
    if ( psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_X] +
         psDP->nRasterXSize *
         psDP->adfGeoTransform[GEOTRSFRM_WE_RES] < minX )
         return FALSE;
    if ( psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_X] > maxX )
         return FALSE;
    if ( psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_Y] +
         psDP->nRasterYSize *
         psDP->adfGeoTransform[GEOTRSFRM_NS_RES] > maxY )
         return FALSE;
    if ( psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_Y] < minY )
         return FALSE;

    *pnSrcXSize = psDP->nRasterXSize;
    *pnSrcYSize = psDP->nRasterYSize;
    if ( psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_X] < minX )
    {
        *pnSrcXOff = (int)((minX - psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_X]) /
            psDP->adfGeoTransform[GEOTRSFRM_WE_RES] + 0.5);
        *pnDstXOff = 0;
    }
    else
    {
        *pnSrcXOff = 0;
        *pnDstXOff = (int)
            (0.5 + (psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_X] - minX) / we_res);
    }
    if ( maxY < psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_Y])
    {
        *pnSrcYOff = (int)((psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_Y] - maxY) /
            -psDP->adfGeoTransform[GEOTRSFRM_NS_RES] + 0.5);
        *pnDstYOff = 0;
    }
    else
    {
        *pnSrcYOff = 0;
        *pnDstYOff = (int)
            (0.5 + (maxY - psDP->adfGeoTransform[GEOTRSFRM_TOPLEFT_Y]) / -ns_res);
    }
    *pnDstXSize = (int)
        (0.5 + psDP->nRasterXSize *
         psDP->adfGeoTransform[GEOTRSFRM_WE_RES] / we_res);
    *pnDstYSize = (int)
        (0.5 + psDP->nRasterYSize *
         psDP->adfGeoTransform[GEOTRSFRM_NS_RES] / ns_res);
         
    return TRUE;
}

/************************************************************************/
/*                            VRTBuilder                                */
/************************************************************************/

class VRTBuilder
{
    /* Input parameters */
    char               *pszOutputFilename;
    int                 nInputFiles;
    char              **ppszInputFilenames;
    ResolutionStrategy  resolutionStrategy;
    double              we_res;
    double              ns_res;
    double              minX;
    double              minY;
    double              maxX;
    double              maxY;
    int                 bSeparate;
    int                 bAllowProjectionDifference;
    int                 bAddAlpha;
    int                 bHideNoData;
    char               *pszSrcNoData;
    char               *pszVRTNoData;

    /* Internal variables */
    char               *pszProjectionRef;
    int                 nBands;
    BandProperty       *pasBandProperties;
    int                 bFirst;
    int                 bHasGeoTransform;
    int                 nRasterXSize;
    int                 nRasterYSize;
    DatasetProperty    *pasDatasetProperties;
    int                 bUserExtent;
    int                 bAllowSrcNoData;
    double             *padfSrcNoData;
    int                 nSrcNoDataCount;
    int                 bAllowVRTNoData;
    double             *padfVRTNoData;
    int                 nVRTNoDataCount;
    int                 bHasRunBuild;

    int         AnalyseRaster(GDALDatasetH hDS, const char* dsFileName,
                              DatasetProperty* psDatasetProperties);

    void        CreateVRTSeparate(VRTDatasetH hVRTDS);
    void        CreateVRTNonSeparate(VRTDatasetH hVRTDS);

    public:
                VRTBuilder(const char* pszOutputFilename,
                           int nInputFiles, const char* const * ppszInputFilenames,
                           ResolutionStrategy resolutionStrategy,
                           double we_res, double ns_res,
                           double minX, double minY, double maxX, double maxY,
                           int bSeparate, int bAllowProjectionDifference,
                           int bAddAlpha, int bHideNoData,
                           const char* pszSrcNoData, const char* pszVRTNoData);

               ~VRTBuilder();

        int     Build(GDALProgressFunc pfnProgress, void * pProgressData);
};


/************************************************************************/
/*                          VRTBuilder()                                */
/************************************************************************/

VRTBuilder::VRTBuilder(const char* pszOutputFilename,
                       int nInputFiles, const char* const * ppszInputFilenames,
                       ResolutionStrategy resolutionStrategy,
                       double we_res, double ns_res,
                       double minX, double minY, double maxX, double maxY,
                       int bSeparate, int bAllowProjectionDifference,
                       int bAddAlpha, int bHideNoData,
                       const char* pszSrcNoData, const char* pszVRTNoData)
{
    this->pszOutputFilename = CPLStrdup(pszOutputFilename);
    this->nInputFiles = nInputFiles;

    this->ppszInputFilenames = (char**) CPLMalloc(nInputFiles * sizeof(char*));
    int i;
    for(i=0;i<nInputFiles;i++)
    {
        this->ppszInputFilenames[i] = CPLStrdup(ppszInputFilenames[i]);
    }

    this->resolutionStrategy = resolutionStrategy;
    this->we_res = we_res;
    this->ns_res = ns_res;
    this->minX = minX;
    this->minY = minY;
    this->maxX = maxX;
    this->maxY = maxY;
    this->bSeparate = bSeparate;
    this->bAllowProjectionDifference = bAllowProjectionDifference;
    this->bAddAlpha = bAddAlpha;
    this->bHideNoData = bHideNoData;
    this->pszSrcNoData = (pszSrcNoData) ? CPLStrdup(pszSrcNoData) : NULL;
    this->pszVRTNoData = (pszVRTNoData) ? CPLStrdup(pszVRTNoData) : NULL;

    bUserExtent = FALSE;
    pszProjectionRef = NULL;
    nBands = 0;
    pasBandProperties = NULL;
    bFirst = TRUE;
    bHasGeoTransform = FALSE;
    nRasterXSize = 0;
    nRasterYSize = 0;
    pasDatasetProperties = NULL;
    bAllowSrcNoData = TRUE;
    padfSrcNoData = NULL;
    nSrcNoDataCount = 0;
    bAllowVRTNoData = TRUE;
    padfVRTNoData = NULL;
    nVRTNoDataCount = 0;
    bHasRunBuild = FALSE;
}

/************************************************************************/
/*                         ~VRTBuilder()                                */
/************************************************************************/

VRTBuilder::~VRTBuilder()
{
    CPLFree(pszOutputFilename);
    CPLFree(pszSrcNoData);
    CPLFree(pszVRTNoData);

    int i;
    for(i=0;i<nInputFiles;i++)
    {
        CPLFree(ppszInputFilenames[i]);
    }
    CPLFree(ppszInputFilenames);

    if (pasDatasetProperties != NULL)
    {
        for(i=0;i<nInputFiles;i++)
        {
            CPLFree(pasDatasetProperties[i].padfNoDataValues);
            CPLFree(pasDatasetProperties[i].panHasNoData);
        }
    }
    CPLFree(pasDatasetProperties);

    if (!bSeparate && pasBandProperties != NULL)
    {
        int j;
        for(j=0;j<nBands;j++)
        {
            GDALDestroyColorTable(pasBandProperties[j].colorTable);
        }
    }
    CPLFree(pasBandProperties);

    CPLFree(pszProjectionRef);
    CPLFree(padfSrcNoData);
    CPLFree(padfVRTNoData);
}

/************************************************************************/
/*                           AnalyseRaster()                            */
/************************************************************************/

int VRTBuilder::AnalyseRaster( GDALDatasetH hDS, const char* dsFileName,
                                  DatasetProperty* psDatasetProperties)
{
    char** papszMetadata = GDALGetMetadata( hDS, "SUBDATASETS" );
    if( CSLCount(papszMetadata) > 0 && GDALGetRasterCount(hDS) == 0 )
    {
        pasDatasetProperties =
            (DatasetProperty*) CPLRealloc(pasDatasetProperties,
                            (nInputFiles+CSLCount(papszMetadata))*sizeof(DatasetProperty));

        ppszInputFilenames = (char**)CPLRealloc(ppszInputFilenames,
                                sizeof(char*) * (nInputFiles+CSLCount(papszMetadata)));
        int count = 1;
        char subdatasetNameKey[256];
        sprintf(subdatasetNameKey, "SUBDATASET_%d_NAME", count);
        while(*papszMetadata != NULL)
        {
            if (EQUALN(*papszMetadata, subdatasetNameKey, strlen(subdatasetNameKey)))
            {
                memset(&pasDatasetProperties[nInputFiles], 0, sizeof(DatasetProperty));
                ppszInputFilenames[nInputFiles++] =
                        CPLStrdup(*papszMetadata+strlen(subdatasetNameKey)+1);
                count++;
                sprintf(subdatasetNameKey, "SUBDATASET_%d_NAME", count);
            }
            papszMetadata++;
        }
        return FALSE;
    }

    const char* proj = GDALGetProjectionRef(hDS);
    double* padfGeoTransform = psDatasetProperties->adfGeoTransform;
    int bGotGeoTransform = GDALGetGeoTransform(hDS, padfGeoTransform) == CE_None;
    if (bSeparate)
    {
        if (bFirst)
        {
            bHasGeoTransform = bGotGeoTransform;
            if (!bHasGeoTransform)
            {
                if (bUserExtent)
                {
                    CPLError(CE_Warning, CPLE_NotSupported,
                        "User extent ignored by gdalbuildvrt -separate with ungeoreferenced images.");
                }
                if (resolutionStrategy == USER_RESOLUTION)
                {
                    CPLError(CE_Warning, CPLE_NotSupported,
                        "User resolution ignored by gdalbuildvrt -separate with ungeoreferenced images.");
                }
            }
        }
        else if (bHasGeoTransform != bGotGeoTransform)
        {
            CPLError(CE_Warning, CPLE_NotSupported,
                    "gdalbuildvrt -separate cannot stack ungeoreferenced and georeferenced images. Skipping %s",
                    dsFileName);
            return FALSE;
        }
        else if (!bHasGeoTransform &&
                    (nRasterXSize != GDALGetRasterXSize(hDS) ||
                    nRasterYSize != GDALGetRasterYSize(hDS)))
        {
            CPLError(CE_Warning, CPLE_NotSupported,
                    "gdalbuildvrt -separate cannot stack ungeoreferenced images that have not the same dimensions. Skipping %s",
                    dsFileName);
            return FALSE;
        }
    }
    else
    {
        if (!bGotGeoTransform)
        {
            CPLError(CE_Warning, CPLE_NotSupported,
                    "gdalbuildvrt does not support ungeoreferenced image. Skipping %s",
                    dsFileName);
            return FALSE;
        }
        bHasGeoTransform = TRUE;
    }

    if (bGotGeoTransform)
    {
        if (padfGeoTransform[GEOTRSFRM_ROTATION_PARAM1] != 0 ||
            padfGeoTransform[GEOTRSFRM_ROTATION_PARAM2] != 0)
        {
            CPLError(CE_Warning, CPLE_NotSupported,
                    "gdalbuildvrt does not support rotated geo transforms. Skipping %s",
                    dsFileName);
            return FALSE;
        }
        if (padfGeoTransform[GEOTRSFRM_NS_RES] >= 0)
        {
            CPLError(CE_Warning, CPLE_NotSupported,
                    "gdalbuildvrt does not support positive NS resolution. Skipping %s",
                    dsFileName);
            return FALSE;
        }
    }

    psDatasetProperties->nRasterXSize = GDALGetRasterXSize(hDS);
    psDatasetProperties->nRasterYSize = GDALGetRasterYSize(hDS);
    if (bFirst && bSeparate && !bGotGeoTransform)
    {
        nRasterXSize = GDALGetRasterXSize(hDS);
        nRasterYSize = GDALGetRasterYSize(hDS);
    }

    double ds_minX = padfGeoTransform[GEOTRSFRM_TOPLEFT_X];
    double ds_maxY = padfGeoTransform[GEOTRSFRM_TOPLEFT_Y];
    double ds_maxX = ds_minX +
                GDALGetRasterXSize(hDS) *
                padfGeoTransform[GEOTRSFRM_WE_RES];
    double ds_minY = ds_maxY +
                GDALGetRasterYSize(hDS) *
                padfGeoTransform[GEOTRSFRM_NS_RES];

    GDALGetBlockSize(GDALGetRasterBand( hDS, 1 ),
                        &psDatasetProperties->nBlockXSize,
                        &psDatasetProperties->nBlockYSize);

    int _nBands = GDALGetRasterCount(hDS);
    if (_nBands == 0)
    {
        CPLError(CE_Warning, CPLE_AppDefined,
                    "Skipping %s as it has no bands", dsFileName);
        return FALSE;
    }
    else if (_nBands > 1 && bSeparate)
    {
        CPLError(CE_Warning, CPLE_AppDefined, "%s has %d bands. Only the first one will "
                    "be taken into account in the -separate case",
                    dsFileName, _nBands);
        _nBands = 1;
    }

    /* For the -separate case */
    psDatasetProperties->firstBandType = GDALGetRasterDataType(GDALGetRasterBand(hDS, 1));

    psDatasetProperties->padfNoDataValues = (double*)CPLCalloc(sizeof(double), _nBands);
    psDatasetProperties->panHasNoData = (int*)CPLCalloc(sizeof(int), _nBands);
    int j;
    for(j=0;j<_nBands;j++)
    {
        if (nSrcNoDataCount > 0)
        {
            psDatasetProperties->panHasNoData[j] = TRUE;
            if (j < nSrcNoDataCount)
                psDatasetProperties->padfNoDataValues[j] = padfSrcNoData[j];
            else
                psDatasetProperties->padfNoDataValues[j] = padfSrcNoData[nSrcNoDataCount - 1];
        }
        else
        {
            psDatasetProperties->padfNoDataValues[j]  =
                GDALGetRasterNoDataValue(GDALGetRasterBand(hDS, j+1),
                                        &psDatasetProperties->panHasNoData[j]);
        }
    }

    if (bFirst)
    {
        if (proj)
            pszProjectionRef = CPLStrdup(proj);
        if (!bUserExtent)
        {
            minX = ds_minX;
            minY = ds_minY;
            maxX = ds_maxX;
            maxY = ds_maxY;
        }
        nBands = _nBands;

        if (!bSeparate)
        {
            pasBandProperties = (BandProperty*)CPLMalloc(nBands*sizeof(BandProperty));
            for(j=0;j<nBands;j++)
            {
                GDALRasterBandH hRasterBand = GDALGetRasterBand( hDS, j+1 );
                pasBandProperties[j].colorInterpretation =
                        GDALGetRasterColorInterpretation(hRasterBand);
                pasBandProperties[j].dataType = GDALGetRasterDataType(hRasterBand);
                if (pasBandProperties[j].colorInterpretation == GCI_PaletteIndex)
                {
                    pasBandProperties[j].colorTable =
                            GDALGetRasterColorTable( hRasterBand );
                    if (pasBandProperties[j].colorTable)
                    {
                        pasBandProperties[j].colorTable =
                                GDALCloneColorTable(pasBandProperties[j].colorTable);
                    }
                }
                else
                    pasBandProperties[j].colorTable = 0;

                if (nVRTNoDataCount > 0)
                {
                    pasBandProperties[j].bHasNoData = TRUE;
                    if (j < nVRTNoDataCount)
                        pasBandProperties[j].noDataValue = padfVRTNoData[j];
                    else
                        pasBandProperties[j].noDataValue = padfVRTNoData[nVRTNoDataCount - 1];
                }
                else
                {
                    pasBandProperties[j].noDataValue =
                            GDALGetRasterNoDataValue(hRasterBand, &pasBandProperties[j].bHasNoData);
                }
            }
        }
    }
    else
    {
        if ((proj != NULL && pszProjectionRef == NULL) ||
            (proj == NULL && pszProjectionRef != NULL) ||
            (proj != NULL && pszProjectionRef != NULL && EQUAL(proj, pszProjectionRef) == FALSE))
        {
            if (!bAllowProjectionDifference)
            {
                CPLError(CE_Warning, CPLE_NotSupported,
                            "gdalbuildvrt does not support heterogenous projection. Skipping %s",
                            dsFileName);
                return FALSE;
            }
        }
        if (!bSeparate)
        {
            if (nBands != _nBands)
            {
                CPLError(CE_Warning, CPLE_NotSupported,
                            "gdalbuildvrt does not support heterogenous band numbers. Skipping %s",
                        dsFileName);
                return FALSE;
            }
            for(j=0;j<nBands;j++)
            {
                GDALRasterBandH hRasterBand = GDALGetRasterBand( hDS, j+1 );
                if (pasBandProperties[j].colorInterpretation != GDALGetRasterColorInterpretation(hRasterBand) ||
                    pasBandProperties[j].dataType != GDALGetRasterDataType(hRasterBand))
                {
                    CPLError(CE_Warning, CPLE_NotSupported,
                                "gdalbuildvrt does not support heterogenous band characteristics. Skipping %s",
                                dsFileName);
                    return FALSE;
                }
                if (pasBandProperties[j].colorTable)
                {
                    GDALColorTableH colorTable = GDALGetRasterColorTable( hRasterBand );
                    int nRefColorEntryCount = GDALGetColorEntryCount(pasBandProperties[j].colorTable);
                    int i;
                    if (colorTable == NULL ||
                        GDALGetColorEntryCount(colorTable) != nRefColorEntryCount)
                    {
                        CPLError(CE_Warning, CPLE_NotSupported,
                                    "gdalbuildvrt does not support rasters with different color tables (different number of color table entries). Skipping %s",
                                dsFileName);
                        return FALSE;
                    }

                    /* Check that the palette are the same too */
                    /* We just warn and still process the file. It is not a technical no-go, but the user */
                    /* should check that the end result is OK for him. */
                    for(i=0;i<nRefColorEntryCount;i++)
                    {
                        const GDALColorEntry* psEntry = GDALGetColorEntry(colorTable, i);
                        const GDALColorEntry* psEntryRef = GDALGetColorEntry(pasBandProperties[j].colorTable, i);
                        if (psEntry->c1 != psEntryRef->c1 || psEntry->c2 != psEntryRef->c2 ||
                            psEntry->c3 != psEntryRef->c3 || psEntry->c4 != psEntryRef->c4)
                        {
                            static int bFirstWarningPCT = TRUE;
                            if (bFirstWarningPCT)
                                CPLError(CE_Warning, CPLE_NotSupported,
                                        "%s has different values than the first raster for some entries in the color table.\n"
                                        "The end result might produce weird colors.\n"
                                        "You're advised to preprocess your rasters with other tools, such as pct2rgb.py or gdal_translate -expand RGB\n"
                                        "to operate gdalbuildvrt on RGB rasters instead", dsFileName);
                            else
                                CPLError(CE_Warning, CPLE_NotSupported,
                                            "%s has different values than the first raster for some entries in the color table.",
                                            dsFileName);
                            bFirstWarningPCT = FALSE;
                            break;
                        }
                    }
                }
            }

        }
        if (!bUserExtent)
        {
            if (ds_minX < minX) minX = ds_minX;
            if (ds_minY < minY) minY = ds_minY;
            if (ds_maxX > maxX) maxX = ds_maxX;
            if (ds_maxY > maxY) maxY = ds_maxY;
        }
    }

    if (resolutionStrategy == AVERAGE_RESOLUTION)
    {
        we_res += padfGeoTransform[GEOTRSFRM_WE_RES];
        ns_res += padfGeoTransform[GEOTRSFRM_NS_RES];
    }
    else if (resolutionStrategy != USER_RESOLUTION)
    {
        if (bFirst)
        {
            we_res = padfGeoTransform[GEOTRSFRM_WE_RES];
            ns_res = padfGeoTransform[GEOTRSFRM_NS_RES];
        }
        else if (resolutionStrategy == HIGHEST_RESOLUTION)
        {
            we_res = MIN(we_res, padfGeoTransform[GEOTRSFRM_WE_RES]);
            /* Yes : as ns_res is negative, the highest resolution is the max value */
            ns_res = MAX(ns_res, padfGeoTransform[GEOTRSFRM_NS_RES]);
        }
        else
        {
            we_res = MAX(we_res, padfGeoTransform[GEOTRSFRM_WE_RES]);
            /* Yes : as ns_res is negative, the lowest resolution is the min value */
            ns_res = MIN(ns_res, padfGeoTransform[GEOTRSFRM_NS_RES]);
        }
    }

    return TRUE;
}

/************************************************************************/
/*                         CreateVRTSeparate()                          */
/************************************************************************/

void VRTBuilder::CreateVRTSeparate(VRTDatasetH hVRTDS)
{
    int i;
    int iBand = 1;
    for(i=0;i<nInputFiles;i++)
    {
        DatasetProperty* psDatasetProperties = &pasDatasetProperties[i];

        if (psDatasetProperties->isFileOK == FALSE)
            continue;

        int nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize,
            nDstXOff, nDstYOff, nDstXSize, nDstYSize;
        if (bHasGeoTransform)
        {
            if ( ! GetSrcDstWin(psDatasetProperties,
                        we_res, ns_res, minX, minY, maxX, maxY,
                        &nSrcXOff, &nSrcYOff, &nSrcXSize, &nSrcYSize,
                        &nDstXOff, &nDstYOff, &nDstXSize, &nDstYSize) )
                continue;
        }
        else
        {
            nSrcXOff = nSrcYOff = nDstXOff = nDstYOff = 0;
            nSrcXSize = nDstXSize = nRasterXSize;
            nSrcYSize = nDstYSize = nRasterYSize;
        }

        const char* dsFileName = ppszInputFilenames[i];

        GDALAddBand(hVRTDS, psDatasetProperties->firstBandType, NULL);

        GDALProxyPoolDatasetH hProxyDS =
            GDALProxyPoolDatasetCreate(dsFileName,
                                        psDatasetProperties->nRasterXSize,
                                        psDatasetProperties->nRasterYSize,
                                        GA_ReadOnly, TRUE, pszProjectionRef,
                                        psDatasetProperties->adfGeoTransform);
        GDALProxyPoolDatasetAddSrcBandDescription(hProxyDS,
                                            psDatasetProperties->firstBandType,
                                            psDatasetProperties->nBlockXSize,
                                            psDatasetProperties->nBlockYSize);

        VRTSourcedRasterBandH hVRTBand =
                (VRTSourcedRasterBandH)GDALGetRasterBand(hVRTDS, iBand);

        if (bHideNoData)
            GDALSetMetadataItem(hVRTBand,"HideNoDataValue","1",NULL);

        if (bAllowSrcNoData && psDatasetProperties->panHasNoData[0])
        {
            GDALSetRasterNoDataValue(hVRTBand, psDatasetProperties->padfNoDataValues[0]);
            VRTAddComplexSource(hVRTBand, GDALGetRasterBand((GDALDatasetH)hProxyDS, 1),
                            nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize,
                            nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                            0, 1, psDatasetProperties->padfNoDataValues[0]);
        }
        else
            /* Place the raster band at the right position in the VRT */
            VRTAddSimpleSource(hVRTBand, GDALGetRasterBand((GDALDatasetH)hProxyDS, 1),
                            nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize,
                            nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                            "near", VRT_NODATA_UNSET);

        GDALDereferenceDataset(hProxyDS);

        iBand ++;
    }
}

/************************************************************************/
/*                       CreateVRTNonSeparate()                         */
/************************************************************************/

void VRTBuilder::CreateVRTNonSeparate(VRTDatasetH hVRTDS)
{
    int i, j;

    for(j=0;j<nBands;j++)
    {
        GDALRasterBandH hBand;
        GDALAddBand(hVRTDS, pasBandProperties[j].dataType, NULL);
        hBand = GDALGetRasterBand(hVRTDS, j+1);
        GDALSetRasterColorInterpretation(hBand, pasBandProperties[j].colorInterpretation);
        if (pasBandProperties[j].colorInterpretation == GCI_PaletteIndex)
        {
            GDALSetRasterColorTable(hBand, pasBandProperties[j].colorTable);
        }
        if (bAllowVRTNoData && pasBandProperties[j].bHasNoData)
            GDALSetRasterNoDataValue(hBand, pasBandProperties[j].noDataValue);
        if ( bHideNoData )
            GDALSetMetadataItem(hBand,"HideNoDataValue","1",NULL);
    }

    if (bAddAlpha)
    {
        GDALRasterBandH hBand;
        GDALAddBand(hVRTDS, GDT_Byte, NULL);
        hBand = GDALGetRasterBand(hVRTDS, nBands + 1);
        GDALSetRasterColorInterpretation(hBand, GCI_AlphaBand);
    }

    for(i=0;i<nInputFiles;i++)
    {
        DatasetProperty* psDatasetProperties = &pasDatasetProperties[i];

        if (psDatasetProperties->isFileOK == FALSE)
            continue;

        int nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize,
            nDstXOff, nDstYOff, nDstXSize, nDstYSize;
        if ( ! GetSrcDstWin(psDatasetProperties,
                        we_res, ns_res, minX, minY, maxX, maxY,
                        &nSrcXOff, &nSrcYOff, &nSrcXSize, &nSrcYSize,
                        &nDstXOff, &nDstYOff, &nDstXSize, &nDstYSize) )
            continue;

        const char* dsFileName = ppszInputFilenames[i];

        GDALProxyPoolDatasetH hProxyDS =
            GDALProxyPoolDatasetCreate(dsFileName,
                                        psDatasetProperties->nRasterXSize,
                                        psDatasetProperties->nRasterYSize,
                                        GA_ReadOnly, TRUE, pszProjectionRef,
                                        psDatasetProperties->adfGeoTransform);

        for(j=0;j<nBands;j++)
        {
            GDALProxyPoolDatasetAddSrcBandDescription(hProxyDS,
                                            pasBandProperties[j].dataType,
                                            psDatasetProperties->nBlockXSize,
                                            psDatasetProperties->nBlockYSize);
        }

        for(j=0;j<nBands;j++)
        {
            VRTSourcedRasterBandH hVRTBand =
                    (VRTSourcedRasterBandH)GDALGetRasterBand(hVRTDS, j + 1);

            /* Place the raster band at the right position in the VRT */
            if (bAllowSrcNoData && psDatasetProperties->panHasNoData[j])
                VRTAddComplexSource(hVRTBand, GDALGetRasterBand((GDALDatasetH)hProxyDS, j + 1),
                                nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize,
                                nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                                0, 1, psDatasetProperties->padfNoDataValues[j]);
            else
                VRTAddSimpleSource(hVRTBand, GDALGetRasterBand((GDALDatasetH)hProxyDS, j + 1),
                                nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize,
                                nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                                "near", VRT_NODATA_UNSET);
        }

        if (bAddAlpha)
        {
            VRTSourcedRasterBandH hVRTBand =
                    (VRTSourcedRasterBandH)GDALGetRasterBand(hVRTDS, nBands + 1);
            /* Little trick : we use an offset of 255 and a scaling of 0, so that in areas covered */
            /* by the source, the value of the alpha band will be 255, otherwise it will be 0 */
            VRTAddComplexSource(hVRTBand, GDALGetRasterBand((GDALDatasetH)hProxyDS, 1),
                                nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize,
                                nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                                255, 0, VRT_NODATA_UNSET);
        }

        GDALDereferenceDataset(hProxyDS);
    }
}

/************************************************************************/
/*                             Build()                                  */
/************************************************************************/

int VRTBuilder::Build(GDALProgressFunc pfnProgress, void * pProgressData)
{
    int i;

    if (bHasRunBuild)
        return CE_Failure;
    bHasRunBuild = TRUE;

    if( pfnProgress == NULL )
        pfnProgress = GDALDummyProgress;

    bUserExtent = (minX != 0 || minY != 0 || maxX != 0 || maxY != 0);
    if (bUserExtent)
    {
        if (minX >= maxX || minY >= maxY )
        {
            CPLError(CE_Failure, CPLE_IllegalArg, "Invalid user extent");
            return CE_Failure;
        }
    }

    if (resolutionStrategy == USER_RESOLUTION)
    {
        if (we_res <= 0 || ns_res <= 0)
        {
            CPLError(CE_Failure, CPLE_IllegalArg, "Invalid user resolution");
            return CE_Failure;
        }

        /* We work with negative north-south resolution in all the following code */
        ns_res = -ns_res;
    }
    else
    {
        we_res = ns_res = 0;
    }

    pasDatasetProperties =
            (DatasetProperty*) CPLCalloc(nInputFiles, sizeof(DatasetProperty));

    if (pszSrcNoData != NULL)
    {
        if (EQUAL(pszSrcNoData, "none"))
        {
            bAllowSrcNoData = FALSE;
        }
        else
        {
            char **papszTokens = CSLTokenizeString( pszSrcNoData );
            nSrcNoDataCount = CSLCount(papszTokens);
            padfSrcNoData = (double *) CPLMalloc(sizeof(double) * nSrcNoDataCount);
            for(i=0;i<nSrcNoDataCount;i++)
                padfSrcNoData[i] = CPLAtofM(papszTokens[i]);
            CSLDestroy(papszTokens);
        }
    }

    if (pszVRTNoData != NULL)
    {
        if (EQUAL(pszVRTNoData, "none"))
        {
            bAllowVRTNoData = FALSE;
        }
        else
        {
            char **papszTokens = CSLTokenizeString( pszVRTNoData );
            nVRTNoDataCount = CSLCount(papszTokens);
            padfVRTNoData = (double *) CPLMalloc(sizeof(double) * nVRTNoDataCount);
            for(i=0;i<nVRTNoDataCount;i++)
                padfVRTNoData[i] = CPLAtofM(papszTokens[i]);
            CSLDestroy(papszTokens);
        }
    }

    int nCountValid = 0;
    for(i=0;i<nInputFiles;i++)
    {
        const char* dsFileName = ppszInputFilenames[i];

        if (!pfnProgress( 1.0 * (i+1) / nInputFiles, NULL, pProgressData))
        {
            return CE_Failure;
        }

        GDALDatasetH hDS = GDALOpen(ppszInputFilenames[i], GA_ReadOnly );
        pasDatasetProperties[i].isFileOK = FALSE;

        if (hDS)
        {
            if (AnalyseRaster( hDS, dsFileName, &pasDatasetProperties[i] ))
            {
                pasDatasetProperties[i].isFileOK = TRUE;
                nCountValid ++;
                bFirst = FALSE;
            }
            GDALClose(hDS);
        }
        else
        {
            CPLError(CE_Warning, CPLE_AppDefined, 
                     "Can't open %s. Skipping it", dsFileName);
        }
    }

    if (nCountValid == 0)
        return CE_None;

    if (bHasGeoTransform)
    {
        if (resolutionStrategy == AVERAGE_RESOLUTION)
        {
            we_res /= nCountValid;
            ns_res /= nCountValid;
        }

        nRasterXSize = (int)(0.5 + (maxX - minX) / we_res);
        nRasterYSize = (int)(0.5 + (maxY - minY) / -ns_res);
    }

    if (nRasterXSize == 0 || nRasterYSize == 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                  "Computed VRT dimension is invalid. You've probably specified unappropriate resolution.");
        return CE_Failure;
    }

    VRTDatasetH hVRTDS = VRTCreate(nRasterXSize, nRasterYSize);
    GDALSetDescription(hVRTDS, pszOutputFilename);

    if (pszProjectionRef)
    {
        GDALSetProjection(hVRTDS, pszProjectionRef);
    }

    if (bHasGeoTransform)
    {
        double adfGeoTransform[6];
        adfGeoTransform[GEOTRSFRM_TOPLEFT_X] = minX;
        adfGeoTransform[GEOTRSFRM_WE_RES] = we_res;
        adfGeoTransform[GEOTRSFRM_ROTATION_PARAM1] = 0;
        adfGeoTransform[GEOTRSFRM_TOPLEFT_Y] = maxY;
        adfGeoTransform[GEOTRSFRM_ROTATION_PARAM2] = 0;
        adfGeoTransform[GEOTRSFRM_NS_RES] = ns_res;
        GDALSetGeoTransform(hVRTDS, adfGeoTransform);
    }

    if (bSeparate)
    {
        CreateVRTSeparate(hVRTDS);
    }
    else
    {
        CreateVRTNonSeparate(hVRTDS);
    }

    GDALClose(hVRTDS);

    return CE_None;
}

/************************************************************************/
/*                        add_file_to_list()                            */
/************************************************************************/

static void add_file_to_list(const char* filename, const char* tile_index,
                             int* pnInputFiles, char*** pppszInputFilenames)
{
   
    int nInputFiles = *pnInputFiles;
    char** ppszInputFilenames = *pppszInputFilenames;
    
    if (EQUAL(CPLGetExtension(filename), "SHP"))
    {
#ifndef OGR_ENABLED
        CPLError(CE_Failure, CPLE_AppDefined, "OGR support needed to read tileindex");
        *pnInputFiles = 0;
        *pppszInputFilenames = NULL;
#else
        OGRDataSourceH hDS;
        OGRLayerH      hLayer;
        OGRFeatureDefnH hFDefn;
        int j, ti_field;

        OGRRegisterAll();
        
        /* Handle GDALTIndex Shapefile as a special case */
        hDS = OGROpen( filename, FALSE, NULL );
        if( hDS  == NULL )
        {
            fprintf( stderr, "Unable to open shapefile `%s'.\n", 
                    filename );
            exit(2);
        }
        
        hLayer = OGR_DS_GetLayer(hDS, 0);

        hFDefn = OGR_L_GetLayerDefn(hLayer);

        for( ti_field = 0; ti_field < OGR_FD_GetFieldCount(hFDefn); ti_field++ )
        {
            OGRFieldDefnH hFieldDefn = OGR_FD_GetFieldDefn( hFDefn, ti_field );
            const char* pszName = OGR_Fld_GetNameRef(hFieldDefn);

            if (strcmp(pszName, "LOCATION") == 0 && strcmp("LOCATION", tile_index) != 0 )
            {
                fprintf( stderr, "This shapefile seems to be a tile index of "
                                "OGR features and not GDAL products.\n");
            }
            if( strcmp(pszName, tile_index) == 0 )
                break;
        }
    
        if( ti_field == OGR_FD_GetFieldCount(hFDefn) )
        {
            fprintf( stderr, "Unable to find field `%s' in DBF file `%s'.\n", 
                    tile_index, filename );
            return;
        }
    
        /* Load in memory existing file names in SHP */
        int nTileIndexFiles = OGR_L_GetFeatureCount(hLayer, TRUE);
        if (nTileIndexFiles == 0)
        {
            fprintf( stderr, "Tile index %s is empty. Skipping it.\n", filename);
            return;
        }
        
        ppszInputFilenames = (char**)CPLRealloc(ppszInputFilenames,
                              sizeof(char*) * (nInputFiles+nTileIndexFiles));
        for(j=0;j<nTileIndexFiles;j++)
        {
            OGRFeatureH hFeat = OGR_L_GetNextFeature(hLayer);
            ppszInputFilenames[nInputFiles++] =
                    CPLStrdup(OGR_F_GetFieldAsString(hFeat, ti_field ));
            OGR_F_Destroy(hFeat);
        }

        OGR_DS_Destroy( hDS );
#endif
    }
    else
    {
        ppszInputFilenames = (char**)CPLRealloc(ppszInputFilenames,
                                                 sizeof(char*) * (nInputFiles+1));
        ppszInputFilenames[nInputFiles++] = CPLStrdup(filename);
    }

    *pnInputFiles = nInputFiles;
    *pppszInputFilenames = ppszInputFilenames;
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int nArgc, char ** papszArgv )

{
    const char *tile_index = "location";
    const char *resolution = NULL;
    int nInputFiles = 0;
    char ** ppszInputFilenames = NULL;
    const char * pszOutputFilename = NULL;
    int i, iArg;
    int bSeparate = FALSE;
    int bAllowProjectionDifference = FALSE;
    int bQuiet = FALSE;
    GDALProgressFunc pfnProgress = NULL;
    double we_res = 0, ns_res = 0;
    double xmin = 0, ymin = 0, xmax = 0, ymax = 0;
    int bAddAlpha = FALSE;
    int bForceOverwrite = FALSE;
    int bHideNoData = FALSE;
    const char* pszSrcNoData = NULL;
    const char* pszVRTNoData = NULL;

    GDALAllRegister();

    nArgc = GDALGeneralCmdLineProcessor( nArgc, &papszArgv, 0 );
    if( nArgc < 1 )
        exit( -nArgc );

/* -------------------------------------------------------------------- */
/*      Parse commandline.                                              */
/* -------------------------------------------------------------------- */
    for( iArg = 1; iArg < nArgc; iArg++ )
    {
        if( EQUAL(papszArgv[iArg], "--utility_version") )
        {
            printf("%s was compiled against GDAL %s and is running against GDAL %s\n",
                   papszArgv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }
        else if( EQUAL(papszArgv[iArg],"-tileindex") &&
                 iArg + 1 < nArgc)
        {
            tile_index = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-resolution") &&
                 iArg + 1 < nArgc)
        {
            resolution = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-input_file_list") &&
                 iArg + 1 < nArgc)
        {
            const char* input_file_list = papszArgv[++iArg];
            FILE* f = VSIFOpen(input_file_list, "r");
            if (f)
            {
                while(1)
                {
                    const char* filename = CPLReadLine(f);
                    if (filename == NULL)
                        break;
                    add_file_to_list(filename, tile_index,
                                     &nInputFiles, &ppszInputFilenames);
                }
                VSIFClose(f);
            }
        }
        else if ( EQUAL(papszArgv[iArg],"-separate") )
        {
            bSeparate = TRUE;
        }
        else if ( EQUAL(papszArgv[iArg],"-allow_projection_difference") )
        {
            bAllowProjectionDifference = TRUE;
        }
        /* Alternate syntax for output file */
        else if( EQUAL(papszArgv[iArg],"-o")  &&
                 iArg + 1 < nArgc)
        {
            pszOutputFilename = papszArgv[++iArg];
        }
        else if ( EQUAL(papszArgv[iArg],"-q") || EQUAL(papszArgv[iArg],"-quiet") )
        {
            bQuiet = TRUE;
        }
        else if ( EQUAL(papszArgv[iArg],"-tr") && iArg + 2 < nArgc)
        {
            we_res = CPLAtofM(papszArgv[++iArg]);
            ns_res = CPLAtofM(papszArgv[++iArg]);
        }
        else if ( EQUAL(papszArgv[iArg],"-te") && iArg + 4 < nArgc)
        {
            xmin = CPLAtofM(papszArgv[++iArg]);
            ymin = CPLAtofM(papszArgv[++iArg]);
            xmax = CPLAtofM(papszArgv[++iArg]);
            ymax = CPLAtofM(papszArgv[++iArg]);
        }
        else if ( EQUAL(papszArgv[iArg],"-addalpha") )
        {
            bAddAlpha = TRUE;
        }
        else if ( EQUAL(papszArgv[iArg],"-hidenodata") )
        {
            bHideNoData = TRUE;
        }
        else if ( EQUAL(papszArgv[iArg],"-overwrite") )
        {
            bForceOverwrite = TRUE;
        }
        else if ( EQUAL(papszArgv[iArg],"-srcnodata") && iArg + 1 < nArgc)
        {
            pszSrcNoData = papszArgv[++iArg];
        }
        else if ( EQUAL(papszArgv[iArg],"-vrtnodata") && iArg + 1 < nArgc)
        {
            pszVRTNoData = papszArgv[++iArg];
        }
        else if ( papszArgv[iArg][0] == '-' )
        {
            printf("Unrecognized option : %s\n", papszArgv[iArg]);
            exit(-1);
        }
        else if( pszOutputFilename == NULL )
        {
            pszOutputFilename = papszArgv[iArg];
        }
        else
        {
            add_file_to_list(papszArgv[iArg], tile_index,
                             &nInputFiles, &ppszInputFilenames);
        }
    }

    if( pszOutputFilename == NULL || nInputFiles == 0 )
        Usage();

    if (!bQuiet)
        pfnProgress = GDALTermProgress;
       
    /* Avoid overwriting a non VRT dataset if the user did not put the */
    /* filenames in the right order */
    VSIStatBuf sBuf;
    if (!bForceOverwrite)
    {
        int bExists = (VSIStat(pszOutputFilename, &sBuf) == 0);
        if (bExists)
        {
            GDALDriverH hDriver = GDALIdentifyDriver( pszOutputFilename, NULL );
            if (hDriver && !EQUAL(GDALGetDriverShortName(hDriver), "VRT"))
            {
                fprintf(stderr,
                        "'%s' is an existing GDAL dataset managed by %s driver.\n"
                        "There is an high chance you did not put filenames in the right order.\n"
                        "If you want to overwrite %s, add -overwrite option to the command line.\n\n",
                        pszOutputFilename, GDALGetDriverShortName(hDriver), pszOutputFilename);
                Usage();
            }
        }
    }
    
    if (we_res != 0 && ns_res != 0 &&
        resolution != NULL && !EQUAL(resolution, "user"))
    {
        fprintf(stderr, "-tr option is not compatible with -resolution %s\n", resolution);
        Usage();
    }
    
    if (bAddAlpha && bSeparate)
    {
        fprintf(stderr, "-addalpha option is not compatible with -separate\n");
        Usage();
    }
        
    ResolutionStrategy eStrategy = AVERAGE_RESOLUTION;
    if ( resolution == NULL || EQUAL(resolution, "user") )
    {
        if ( we_res != 0 || ns_res != 0)
            eStrategy = USER_RESOLUTION;
        else if ( resolution != NULL && EQUAL(resolution, "user") )
        {
            fprintf(stderr, "-tr option must be used with -resolution user\n");
            Usage();
        }
    }
    else if ( EQUAL(resolution, "average") )
        eStrategy = AVERAGE_RESOLUTION;
    else if ( EQUAL(resolution, "highest") )
        eStrategy = HIGHEST_RESOLUTION;
    else if ( EQUAL(resolution, "lowest") )
        eStrategy = LOWEST_RESOLUTION;
    else
    {
        fprintf(stderr, "invalid value (%s) for -resolution\n", resolution);
        Usage();
    }
    
    /* If -srcnodata is specified, use it as the -vrtnodata if the latter is not */
    /* specified */
    if (pszSrcNoData != NULL && pszVRTNoData == NULL)
        pszVRTNoData = pszSrcNoData;

    VRTBuilder oBuilder(pszOutputFilename, nInputFiles, ppszInputFilenames,
                        eStrategy, we_res, ns_res, xmin, ymin, xmax, ymax,
                        bSeparate, bAllowProjectionDifference, bAddAlpha, bHideNoData,
                        pszSrcNoData, pszVRTNoData);

    oBuilder.Build(pfnProgress, NULL);
    
    for(i=0;i<nInputFiles;i++)
    {
        CPLFree(ppszInputFilenames[i]);
    }
    CPLFree(ppszInputFilenames);


    CSLDestroy( papszArgv );
    GDALDumpOpenDatasets( stderr );
    GDALDestroyDriverManager();
#ifdef OGR_ENABLED
    OGRCleanupAll();
#endif

    return 0;
}
