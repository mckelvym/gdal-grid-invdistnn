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
#include "ogrsf_frmts/shape/shapefil.h"
#include "vrt/gdal_vrt.h"

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
    AVERAGE_RESOLUTION
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
    int          bFirstBandHasNoData;
    double       dfFirstBandNoDataValue;
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
            "Usage: gdalbuildvrt [-tileindex field_name] [-resolution {highest|lowest|average}]\n"
            "                    [-separate] [-allow_projection_difference]\n"
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
/*                           build_vrt()                                */
/************************************************************************/

void build_vrt(const char* pszOutputFilename,
               int* pnInputFiles, char*** pppszInputFilenames,
               ResolutionStrategy resolutionStrategy,
               int bSeparate, int bAllowProjectionDifference)
{
    char* projectionRef = NULL;
    int nBands = 0;
    BandProperty* bandProperties = NULL;
    double minX = 0, minY = 0, maxX = 0, maxY = 0;
    int i,j;
    double we_res = 0;
    double ns_res = 0;
    int rasterXSize;
    int rasterYSize;
    int nCount = 0;
    int bFirst = TRUE;
    VRTDatasetH hVRTDS = NULL;

    char** ppszInputFilenames = *pppszInputFilenames;
    int nInputFiles = *pnInputFiles;

    DatasetProperty* psDatasetProperties =
            (DatasetProperty*) CPLMalloc(nInputFiles*sizeof(DatasetProperty));

    for(i=0;i<nInputFiles;i++)
    {
        const char* dsFileName = ppszInputFilenames[i];

        GDALTermProgress( 1.0 * (i+1) / nInputFiles, NULL, NULL);

        GDALDatasetH hDS = GDALOpen(ppszInputFilenames[i], GA_ReadOnly );
        psDatasetProperties[i].isFileOK = FALSE;

        if (hDS)
        {
            char** papszMetadata = GDALGetMetadata( hDS, "SUBDATASETS" );
            if( CSLCount(papszMetadata) > 0 && GDALGetRasterCount(hDS) == 0 )
            {
                psDatasetProperties =
                    (DatasetProperty*) CPLMalloc((nInputFiles+CSLCount(papszMetadata))*sizeof(DatasetProperty));

                ppszInputFilenames = (char**)CPLRealloc(ppszInputFilenames,
                                        sizeof(char*) * (nInputFiles+CSLCount(papszMetadata)));
                int count = 1;
                char subdatasetNameKey[256];
                sprintf(subdatasetNameKey, "SUBDATASET_%d_NAME", count);
                while(*papszMetadata != NULL)
                {
                    if (EQUALN(*papszMetadata, subdatasetNameKey, strlen(subdatasetNameKey)))
                    {
                        ppszInputFilenames[nInputFiles++] =
                                CPLStrdup(*papszMetadata+strlen(subdatasetNameKey)+1);
                        count++;
                        sprintf(subdatasetNameKey, "SUBDATASET_%d_NAME", count);
                    }
                    papszMetadata++;
                }
                GDALClose(hDS);
                continue;
            }

            const char* proj = GDALGetProjectionRef(hDS);
            GDALGetGeoTransform(hDS, psDatasetProperties[i].adfGeoTransform);
            if (psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_ROTATION_PARAM1] != 0 ||
                psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_ROTATION_PARAM2] != 0)
            {
                fprintf( stderr, "gdalbuildvrt does not support rotated geo transforms. Skipping %s\n",
                             dsFileName);
                GDALClose(hDS);
                continue;
            }
            if (psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_NS_RES] >= 0)
            {
                fprintf( stderr, "gdalbuildvrt does not support positive NS resolution. Skipping %s\n",
                             dsFileName);
                GDALClose(hDS);
                continue;
            }
            psDatasetProperties[i].nRasterXSize = GDALGetRasterXSize(hDS);
            psDatasetProperties[i].nRasterYSize = GDALGetRasterYSize(hDS);
            double product_minX = psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_TOPLEFT_X];
            double product_maxY = psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_TOPLEFT_Y];
            double product_maxX = product_minX +
                        GDALGetRasterXSize(hDS) *
                        psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_WE_RES];
            double product_minY = product_maxY +
                        GDALGetRasterYSize(hDS) *
                        psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_NS_RES];

            GDALGetBlockSize(GDALGetRasterBand( hDS, 1 ),
                             &psDatasetProperties[i].nBlockXSize,
                             &psDatasetProperties[i].nBlockYSize);

            int _nBands = GDALGetRasterCount(hDS);
            if (_nBands == 0)
            {
                fprintf( stderr, "Skipping %s as it has no bands\n", dsFileName);
                GDALClose(hDS);
                continue;
            }
            else if (_nBands > 1 && bSeparate)
            {
                fprintf( stderr, "%s has %d bands. Only the first one will be taken into account in the -separate case\n",
                         dsFileName, _nBands);
                _nBands = 1;
            }

            /* For the -separate case */
            psDatasetProperties[i].firstBandType = GDALGetRasterDataType(GDALGetRasterBand(hDS, 1));
            psDatasetProperties[i].dfFirstBandNoDataValue  =
                 GDALGetRasterNoDataValue(GDALGetRasterBand(hDS, 1),
                                          &psDatasetProperties[i].bFirstBandHasNoData);

            if (bFirst)
            {
                if (proj)
                    projectionRef = CPLStrdup(proj);
                minX = product_minX;
                minY = product_minY;
                maxX = product_maxX;
                maxY = product_maxY;
                nBands = _nBands;

                if (!bSeparate)
                {
                    bandProperties = (BandProperty*)CPLMalloc(nBands*sizeof(BandProperty));
                    for(j=0;j<nBands;j++)
                    {
                        GDALRasterBandH hRasterBand = GDALGetRasterBand( hDS, j+1 );
                        bandProperties[j].colorInterpretation =
                                GDALGetRasterColorInterpretation(hRasterBand);
                        bandProperties[j].dataType = GDALGetRasterDataType(hRasterBand);
                        if (bandProperties[j].colorInterpretation == GCI_PaletteIndex)
                        {
                            bandProperties[j].colorTable =
                                    GDALGetRasterColorTable( hRasterBand );
                            if (bandProperties[j].colorTable)
                            {
                                bandProperties[j].colorTable =
                                        GDALCloneColorTable(bandProperties[j].colorTable);
                            }
                        }
                        else
                            bandProperties[j].colorTable = 0;
                        bandProperties[j].noDataValue =
                                GDALGetRasterNoDataValue(hRasterBand, &bandProperties[j].bHasNoData);
                    }
                }
            }
            else
            {
                if ((proj != NULL && projectionRef == NULL) ||
                    (proj == NULL && projectionRef != NULL) ||
                    (proj != NULL && projectionRef != NULL && EQUAL(proj, projectionRef) == FALSE))
                {
                    if (!bAllowProjectionDifference)
                    {
                        fprintf( stderr, "gdalbuildvrt does not support heterogenous projection. Skipping %s\n",
                                dsFileName);
                        GDALClose(hDS);
                        continue;
                    }
                }
                if (!bSeparate)
                {
                    if (nBands != _nBands)
                    {
                        fprintf( stderr, "gdalbuildvrt does not support heterogenous band numbers. Skipping %s\n",
                                dsFileName);
                        GDALClose(hDS);
                        continue;
                    }
                    for(j=0;j<nBands;j++)
                    {
                        GDALRasterBandH hRasterBand = GDALGetRasterBand( hDS, j+1 );
                        if (bandProperties[j].colorInterpretation != GDALGetRasterColorInterpretation(hRasterBand) ||
                            bandProperties[j].dataType != GDALGetRasterDataType(hRasterBand))
                        {
                            fprintf( stderr, "gdalbuildvrt does not support heterogenous band characteristics. Skipping %s\n",
                                dsFileName);
                            GDALClose(hDS);
                        }
                        if (bandProperties[j].colorTable)
                        {
                            GDALColorTableH colorTable = GDALGetRasterColorTable( hRasterBand );
                            if (colorTable == NULL ||
                                GDALGetColorEntryCount(colorTable) != GDALGetColorEntryCount(bandProperties[j].colorTable))
                            {
                                fprintf( stderr, "gdalbuildvrt does not support heterogenous band characteristics. Skipping %s\n",
                                dsFileName);
                                GDALClose(hDS);
                                break;
                            }
                            /* We should check that the palette are the same too ! */
                        }
                    }
                    if (j != nBands)
                        continue;
                }
                if (product_minX < minX) minX = product_minX;
                if (product_minY < minY) minY = product_minY;
                if (product_maxX > maxX) maxX = product_maxX;
                if (product_maxY > maxY) maxY = product_maxY;
            }
            if (resolutionStrategy == AVERAGE_RESOLUTION)
            {
                we_res += psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_WE_RES];
                ns_res += psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_NS_RES];
            }
            else
            {
                if (bFirst)
                {
                    we_res = psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_WE_RES];
                    ns_res = psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_NS_RES];
                }
                else if (resolutionStrategy == HIGHEST_RESOLUTION)
                {
                    we_res = MIN(we_res, psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_WE_RES]);
                    ns_res = MIN(ns_res, psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_NS_RES]);
                }
                else
                {
                    we_res = MAX(we_res, psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_WE_RES]);
                    ns_res = MAX(ns_res, psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_NS_RES]);
                }
            }

            psDatasetProperties[i].isFileOK = 1;
            nCount ++;
            bFirst = FALSE;
            GDALClose(hDS);
        }
        else
        {
            fprintf( stderr, "Warning : can't open %s. Skipping it\n", dsFileName);
        }
    }
    
    *pppszInputFilenames = ppszInputFilenames;
    *pnInputFiles = nInputFiles;
    
    if (nCount == 0)
        goto end;
    
    if (resolutionStrategy == AVERAGE_RESOLUTION)
    {
        we_res /= nCount;
        ns_res /= nCount;
    }
    
    rasterXSize = (int)(0.5 + (maxX - minX) / we_res);
    rasterYSize = (int)(0.5 + (maxY - minY) / -ns_res);
    
    hVRTDS = VRTCreate(rasterXSize, rasterYSize);
    GDALSetDescription(hVRTDS, pszOutputFilename);
    
    if (projectionRef)
    {
        GDALSetProjection(hVRTDS, projectionRef);
    }

    double adfGeoTransform[6];
    adfGeoTransform[GEOTRSFRM_TOPLEFT_X] = minX;
    adfGeoTransform[GEOTRSFRM_WE_RES] = we_res;
    adfGeoTransform[GEOTRSFRM_ROTATION_PARAM1] = 0;
    adfGeoTransform[GEOTRSFRM_TOPLEFT_Y] = maxY;
    adfGeoTransform[GEOTRSFRM_ROTATION_PARAM2] = 0;
    adfGeoTransform[GEOTRSFRM_NS_RES] = ns_res;
    GDALSetGeoTransform(hVRTDS, adfGeoTransform);
    
    if (bSeparate)
    {
        int iBand = 1;
        for(i=0;i<nInputFiles;i++)
        {
            if (psDatasetProperties[i].isFileOK == 0)
                continue;
            const char* dsFileName = ppszInputFilenames[i];

            GDALAddBand(hVRTDS, psDatasetProperties[i].firstBandType, NULL);

            GDALProxyPoolDatasetH hProxyDS =
                GDALProxyPoolDatasetCreate(dsFileName,
                                            psDatasetProperties[i].nRasterXSize,
                                            psDatasetProperties[i].nRasterYSize,
                                            GA_ReadOnly, TRUE, projectionRef,
                                            psDatasetProperties[i].adfGeoTransform);
            GDALProxyPoolDatasetAddSrcBandDescription(hProxyDS,
                                                psDatasetProperties[i].firstBandType,
                                                psDatasetProperties[i].nBlockXSize,
                                                psDatasetProperties[i].nBlockYSize);

            int xoffset = (int)
                    (0.5 + (psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_TOPLEFT_X] - minX) / we_res);
            int yoffset = (int)
                    (0.5 + (maxY - psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_TOPLEFT_Y]) / -ns_res);
            int dest_width = (int)
                    (0.5 + psDatasetProperties[i].nRasterXSize *
                        psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_WE_RES] / we_res);
            int dest_height = (int)
                    (0.5 + psDatasetProperties[i].nRasterYSize *
                        psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_NS_RES] / ns_res);

            VRTSourcedRasterBandH hVRTBand =
                    (VRTSourcedRasterBandH)GDALGetRasterBand(hVRTDS, iBand);
            if (psDatasetProperties[i].bFirstBandHasNoData)
                GDALSetRasterNoDataValue(hVRTBand, psDatasetProperties[i].dfFirstBandNoDataValue);

            /* Place the raster band at the right position in the VRT */
            VRTAddSimpleSource(hVRTBand, GDALGetRasterBand((GDALDatasetH)hProxyDS, 1),
                            0, 0,
                            psDatasetProperties[i].nRasterXSize,
                            psDatasetProperties[i].nRasterYSize,
                            xoffset, yoffset,
                            dest_width, dest_height, "near",
                            VRT_NODATA_UNSET);

            GDALDereferenceDataset(hProxyDS);

            iBand ++;
        }
    }
    else
    {
        for(j=0;j<nBands;j++)
        {
            GDALRasterBandH hBand;
            GDALAddBand(hVRTDS, bandProperties[j].dataType, NULL);
            hBand = GDALGetRasterBand(hVRTDS, j+1);
            GDALSetRasterColorInterpretation(hBand, bandProperties[j].colorInterpretation);
            if (bandProperties[j].colorInterpretation == GCI_PaletteIndex)
            {
                GDALSetRasterColorTable(hBand, bandProperties[j].colorTable);
            }
            if (bandProperties[j].bHasNoData)
                GDALSetRasterNoDataValue(hBand, bandProperties[j].noDataValue);
        }
    
        for(i=0;i<nInputFiles;i++)
        {
            if (psDatasetProperties[i].isFileOK == 0)
                continue;
            const char* dsFileName = ppszInputFilenames[i];
    
            GDALProxyPoolDatasetH hProxyDS =
                GDALProxyPoolDatasetCreate(dsFileName,
                                            psDatasetProperties[i].nRasterXSize,
                                            psDatasetProperties[i].nRasterYSize,
                                            GA_ReadOnly, TRUE, projectionRef,
                                            psDatasetProperties[i].adfGeoTransform);
    
            for(j=0;j<nBands;j++)
            {
                GDALProxyPoolDatasetAddSrcBandDescription(hProxyDS,
                                                bandProperties[j].dataType,
                                                psDatasetProperties[i].nBlockXSize,
                                                psDatasetProperties[i].nBlockYSize);
            }
    
            int xoffset = (int)
                    (0.5 + (psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_TOPLEFT_X] - minX) / we_res);
            int yoffset = (int)
                    (0.5 + (maxY - psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_TOPLEFT_Y]) / -ns_res);
            int dest_width = (int)
                    (0.5 + psDatasetProperties[i].nRasterXSize *
                        psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_WE_RES] / we_res);
            int dest_height = (int)
                    (0.5 + psDatasetProperties[i].nRasterYSize *
                        psDatasetProperties[i].adfGeoTransform[GEOTRSFRM_NS_RES] / ns_res);
    
            for(j=0;j<nBands;j++)
            {
                VRTSourcedRasterBandH hVRTBand =
                        (VRTSourcedRasterBandH)GDALGetRasterBand(hVRTDS, j + 1);
    
                /* Place the raster band at the right position in the VRT */
                VRTAddSimpleSource(hVRTBand, GDALGetRasterBand((GDALDatasetH)hProxyDS, j + 1),
                                0, 0,
                                psDatasetProperties[i].nRasterXSize,
                                psDatasetProperties[i].nRasterYSize,
                                xoffset, yoffset,
                                dest_width, dest_height, "near",
                                VRT_NODATA_UNSET);
            }
            GDALDereferenceDataset(hProxyDS);
        }
    }
    GDALClose(hVRTDS);

end:
    CPLFree(psDatasetProperties);
    if (!bSeparate)
    {
        for(j=0;j<nBands;j++)
        {
            GDALDestroyColorTable(bandProperties[j].colorTable);
        }
    }
    CPLFree(bandProperties);
    CPLFree(projectionRef);

}

/************************************************************************/
/*                        add_file_to_list()                            */
/************************************************************************/

static void add_file_to_list(const char* filename, const char* tile_index,
                             int* pnInputFiles, char*** pppszInputFilenames)
{
    SHPHandle   hSHP;
    DBFHandle   hDBF;
    int j, ti_field;
    
    int nInputFiles = *pnInputFiles;
    char** ppszInputFilenames = *pppszInputFilenames;
    
    if (EQUAL(CPLGetExtension(filename), "SHP"))
    {
        /* Handle GDALTIndex Shapefile as a special case */
        hSHP = SHPOpen( filename, "r" );
        if( hSHP == NULL )
        {
            fprintf( stderr, "Unable to open shapefile `%s'.\n", 
                    filename );
            exit(2);
        }
    
        hDBF = DBFOpen( filename, "r" );
        if( hDBF == NULL )
        {
            fprintf( stderr, "Unable to open DBF file `%s'.\n", 
                        filename );
            exit(2);
        }
    
        for( ti_field = 0; ti_field < DBFGetFieldCount(hDBF); ti_field++ )
        {
            char	field_name[16];
    
            DBFGetFieldInfo( hDBF, ti_field, field_name, NULL, NULL );
            if (strcmp(field_name, "LOCATION") == 0 && strcmp("LOCATION", tile_index) != 0 )
            {
                fprintf( stderr, "This shapefile seems to be a tile index of "
                                "OGR features and not GDAL products.\n");
            }
            if( strcmp(field_name, tile_index) == 0 )
                break;
        }
    
        if( ti_field == DBFGetFieldCount(hDBF) )
        {
            fprintf( stderr, "Unable to find field `%s' in DBF file `%s'.\n", 
                    tile_index, filename );
            return;
        }
    
        /* Load in memory existing file names in SHP */
        int nTileIndexFiles = DBFGetRecordCount(hDBF);
        if (nTileIndexFiles == 0)
        {
            fprintf( stderr, "Tile index %s is empty. Skipping it.\n", filename);
            return;
        }
        
        ppszInputFilenames = (char**)CPLRealloc(ppszInputFilenames,
                              sizeof(char*) * (nInputFiles+nTileIndexFiles));
        for(j=0;j<nTileIndexFiles;j++)
        {
            ppszInputFilenames[nInputFiles++] =
                    CPLStrdup(DBFReadStringAttribute( hDBF, j, ti_field ));
        }

        DBFClose( hDBF );
        SHPClose( hSHP );
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
    const char *resolution = "average";
    int nInputFiles = 0;
    char ** ppszInputFilenames = NULL;
    const char * pszOutputFilename = NULL;
    int i, iArg;
    int bSeparate = FALSE;
    int bAllowProjectionDifference = FALSE;

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
        else if( strcmp(papszArgv[iArg],"-tileindex") == 0 &&
                 iArg + 1 < nArgc)
        {
            tile_index = papszArgv[++iArg];
        }
        else if( strcmp(papszArgv[iArg],"-resolution") == 0 &&
                 iArg + 1 < nArgc)
        {
            resolution = papszArgv[++iArg];
        }
        else if( strcmp(papszArgv[iArg],"-input_file_list") == 0 &&
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
        else if ( strcmp(papszArgv[iArg],"-separate") == 0 )
        {
            bSeparate = TRUE;
        }
        else if ( strcmp(papszArgv[iArg],"-allow_projection_difference") == 0 )
        {
            bAllowProjectionDifference = TRUE;
        }
        /* Alternate syntax for output file */
        else if( strcmp(papszArgv[iArg],"-o") == 0 &&
                 iArg + 1 < nArgc)
        {
            pszOutputFilename = papszArgv[++iArg];
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

    
    build_vrt(pszOutputFilename, &nInputFiles, &ppszInputFilenames,
              EQUAL(resolution, "highest") ? HIGHEST_RESOLUTION :
              EQUAL(resolution, "lowest") ? LOWEST_RESOLUTION : AVERAGE_RESOLUTION,
              bSeparate, bAllowProjectionDifference);
    
    for(i=0;i<nInputFiles;i++)
    {
        CPLFree(ppszInputFilenames[i]);
    }
    CPLFree(ppszInputFilenames);


    CSLDestroy( papszArgv );
    GDALDumpOpenDatasets( stderr );
    GDALDestroyDriverManager();

    return 0;
}
