/******************************************************************************
 * $Id: ehdrdataset.cpp 12350 2007-10-08 17:41:32Z rouault $
 *
 * Project:  GDAL
 * Purpose:  Generic Binary format driver (.hdr but not ESRI .hdr!)
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2007, Frank Warmerdam <warmerdam@pobox.com>
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

#include "rawdataset.h"
#include "ogr_spatialref.h"
#include "cpl_string.h"

CPL_CVSID("$Id: ehdrdataset.cpp 12350 2007-10-08 17:41:32Z rouault $");

CPL_C_START
void	GDALRegister_GenBin(void);
CPL_C_END

/* ==================================================================== */
/*      Table relating USGS and ESRI state plane zones.                 */
/* ==================================================================== */
static const int anUsgsEsriZones[] =
{
  101, 3101,
  102, 3126,
  201, 3151,
  202, 3176,
  203, 3201,
  301, 3226,
  302, 3251,
  401, 3276,
  402, 3301,
  403, 3326,
  404, 3351,
  405, 3376,
  406, 3401,
  407, 3426,
  501, 3451,
  502, 3476,
  503, 3501,
  600, 3526,
  700, 3551,
  901, 3601,
  902, 3626,
  903, 3576,
 1001, 3651,
 1002, 3676,
 1101, 3701,
 1102, 3726,
 1103, 3751,
 1201, 3776,
 1202, 3801,
 1301, 3826,
 1302, 3851,
 1401, 3876,
 1402, 3901,
 1501, 3926,
 1502, 3951,
 1601, 3976,
 1602, 4001,
 1701, 4026,
 1702, 4051,
 1703, 6426,
 1801, 4076,
 1802, 4101,
 1900, 4126,
 2001, 4151,
 2002, 4176,
 2101, 4201,
 2102, 4226,
 2103, 4251,
 2111, 6351,
 2112, 6376,
 2113, 6401,
 2201, 4276,
 2202, 4301,
 2203, 4326,
 2301, 4351,
 2302, 4376,
 2401, 4401,
 2402, 4426,
 2403, 4451,
 2500,    0,
 2501, 4476,
 2502, 4501,
 2503, 4526,
 2600,    0,
 2601, 4551,
 2602, 4576,
 2701, 4601,
 2702, 4626,
 2703, 4651,
 2800, 4676,
 2900, 4701,
 3001, 4726,
 3002, 4751,
 3003, 4776,
 3101, 4801,
 3102, 4826,
 3103, 4851,
 3104, 4876,
 3200, 4901,
 3301, 4926,
 3302, 4951,
 3401, 4976,
 3402, 5001,
 3501, 5026,
 3502, 5051,
 3601, 5076,
 3602, 5101,
 3701, 5126,
 3702, 5151,
 3800, 5176,
 3900,    0,
 3901, 5201,
 3902, 5226,
 4001, 5251,
 4002, 5276,
 4100, 5301,
 4201, 5326,
 4202, 5351,
 4203, 5376,
 4204, 5401,
 4205, 5426,
 4301, 5451,
 4302, 5476,
 4303, 5501,
 4400, 5526,
 4501, 5551,
 4502, 5576,
 4601, 5601,
 4602, 5626,
 4701, 5651,
 4702, 5676,
 4801, 5701,
 4802, 5726,
 4803, 5751,
 4901, 5776,
 4902, 5801,
 4903, 5826,
 4904, 5851,
 5001, 6101,
 5002, 6126,
 5003, 6151,
 5004, 6176,
 5005, 6201,
 5006, 6226,
 5007, 6251,
 5008, 6276,
 5009, 6301,
 5010, 6326,
 5101, 5876,
 5102, 5901,
 5103, 5926,
 5104, 5951,
 5105, 5976,
 5201, 6001,
 5200, 6026,
 5200, 6076,
 5201, 6051,
 5202, 6051,
 5300,    0,
 5400,    0
};

/************************************************************************/
/* ==================================================================== */
/*				GenBinDataset				*/
/* ==================================================================== */
/************************************************************************/

class GenBinDataset : public RawDataset
{
    friend class GenBinBitRasterBand;

    FILE	*fpImage;	// image data file.

    int         bGotTransform;
    double      adfGeoTransform[6];
    char       *pszProjection;

    int         bHDRDirty;
    char      **papszHDR;

    void        ParseCoordinateSystem( char ** );

  public:
    GenBinDataset();
    ~GenBinDataset();
    
    virtual CPLErr GetGeoTransform( double * padfTransform );
    virtual const char *GetProjectionRef(void);
    
    virtual char **GetFileList();

    static GDALDataset *Open( GDALOpenInfo * );
};

/************************************************************************/
/* ==================================================================== */
/*                       GenBinBitRasterBand                            */
/* ==================================================================== */
/************************************************************************/

class GenBinBitRasterBand : public GDALPamRasterBand
{
    int            nBits;
    long           nStartBit;
    int            nPixelOffsetBits;
    int            nLineOffsetBits;

  public:
    GenBinBitRasterBand( GenBinDataset *poDS, int nBits );

    virtual CPLErr IReadBlock( int, int, void * );
};

/************************************************************************/
/*                        GenBinBitRasterBand()                         */
/************************************************************************/

GenBinBitRasterBand::GenBinBitRasterBand( GenBinDataset *poDS, int nBitsIn )
{
    SetMetadataItem( "NBITS", 
                     CPLString().Printf("%d",nBitsIn), 
                     "IMAGE_STRUCTURE" );

    this->poDS = poDS;
    nBits = nBitsIn;
    nBand = 1;

    eDataType = GDT_Byte;

    nBlockXSize = poDS->nRasterXSize;
    nBlockYSize = 1;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr GenBinBitRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                        void * pImage )

{
    GenBinDataset *poGDS = (GenBinDataset *) poDS;
    vsi_l_offset   nLineStart;
    unsigned int   nLineBytes;
    int            iBitOffset;
    GByte         *pabyBuffer;

/* -------------------------------------------------------------------- */
/*      Establish desired position.                                     */
/* -------------------------------------------------------------------- */
    nLineStart = (((vsi_l_offset)nBlockXSize) * nBlockYOff * nBits) / 8;
    iBitOffset = (((vsi_l_offset)nBlockXSize) * nBlockYOff * nBits) % 8;
    nLineBytes = (((vsi_l_offset)nBlockXSize) * (nBlockYOff+1) * nBits + 7) / 8
        - nLineStart;

/* -------------------------------------------------------------------- */
/*      Read data into buffer.                                          */
/* -------------------------------------------------------------------- */
    pabyBuffer = (GByte *) CPLCalloc(nLineBytes,1);

    if( VSIFSeekL( poGDS->fpImage, nLineStart, SEEK_SET ) != 0 
        || VSIFReadL( pabyBuffer, 1, nLineBytes, poGDS->fpImage) != nLineBytes )
    {
        CPLError( CE_Failure, CPLE_FileIO,
                  "Failed to read %u bytes at offset %lu.\n%s",
                  nLineBytes, (unsigned long)nLineStart, 
                  VSIStrerror( errno ) );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Copy data, promoting to 8bit.                                   */
/* -------------------------------------------------------------------- */
    int iX;

    if( nBits == 1 )
    {
        for( iX = 0; iX < nBlockXSize; iX++, iBitOffset += nBits )
        {
            if( pabyBuffer[iBitOffset>>3]  & (0x80 >>(iBitOffset & 7)) )
                ((GByte *) pImage)[iX] = 1;
            else
                ((GByte *) pImage)[iX] = 0;
        }
    }
    else if( nBits == 2 )
    {
        for( iX = 0; iX < nBlockXSize; iX++, iBitOffset += nBits )
        {
            ((GByte *) pImage)[iX] = 
                ((pabyBuffer[iBitOffset>>3]) >> (6-(iBitOffset&0x7)) & 0x3);
        }
    }
    else if( nBits == 4 )
    {
        for( iX = 0; iX < nBlockXSize; iX++, iBitOffset += nBits )
        {
            if( iBitOffset == 0 )
                ((GByte *) pImage)[iX] = (pabyBuffer[iBitOffset>>3]) >> 4;
            else
                ((GByte *) pImage)[iX] = (pabyBuffer[iBitOffset>>3]) & 0xf;
        }
    }
    else
        CPLAssert( FALSE );

    CPLFree( pabyBuffer );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*				GenBinDataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                            GenBinDataset()                             */
/************************************************************************/

GenBinDataset::GenBinDataset()
{
    fpImage = NULL;
    pszProjection = CPLStrdup("");
    bGotTransform = FALSE;
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;
    papszHDR = NULL;
}

/************************************************************************/
/*                            ~GenBinDataset()                            */
/************************************************************************/

GenBinDataset::~GenBinDataset()

{
    FlushCache();

    if( fpImage != NULL )
        VSIFCloseL( fpImage );

    CPLFree( pszProjection );
    CSLDestroy( papszHDR );
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *GenBinDataset::GetProjectionRef()

{
    if (pszProjection && strlen(pszProjection) > 0)
        return pszProjection;

    return GDALPamDataset::GetProjectionRef();
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr GenBinDataset::GetGeoTransform( double * padfTransform )

{
    if( bGotTransform )
    {
        memcpy( padfTransform, adfGeoTransform, sizeof(double) * 6 );
        return CE_None;
    }
    else
    {
        return GDALPamDataset::GetGeoTransform( padfTransform );
    }
}

/************************************************************************/
/*                            GetFileList()                             */
/************************************************************************/

char **GenBinDataset::GetFileList()

{
    CPLString osPath = CPLGetPath( GetDescription() );
    CPLString osName = CPLGetBasename( GetDescription() );
    char **papszFileList = NULL;

    // Main data file, etc. 
    papszFileList = GDALPamDataset::GetFileList();

    // Header file.
    CPLString osFilename = CPLFormCIFilename( osPath, osName, "hdr" );
    papszFileList = CSLAddString( papszFileList, osFilename );

    return papszFileList;
}

/************************************************************************/
/*                       ParseCoordinateSystem()                        */
/************************************************************************/

void GenBinDataset::ParseCoordinateSystem( char **papszHdr )

{
    const char *pszProjName = CSLFetchNameValue( papszHdr, "PROJECTION_NAME" );
    OGRSpatialReference oSRS;

    if( pszProjName == NULL )
        return;

/* -------------------------------------------------------------------- */
/*      Translate zone and parameters into numeric form.                */
/* -------------------------------------------------------------------- */
    int nZone = 0;
    double adfProjParms[15];
    const char *pszUnits = CSLFetchNameValue( papszHdr, "MAP_UNITS" );
    const char *pszDatumName = CSLFetchNameValue( papszHdr, "DATUM_NAME" );

    if( CSLFetchNameValue( papszHdr, "PROJECTION_ZONE" ) )
        nZone = atoi(CSLFetchNameValue( papszHdr, "PROJECTION_ZONE" ));

    memset( adfProjParms, 0, sizeof(adfProjParms) );
    if( CSLFetchNameValue( papszHdr, "PROJECTION_PARAMETERS" ) )
    {
        int i;
        char **papszTokens = CSLTokenizeString( 
            CSLFetchNameValue( papszHdr, "PROJECTION_PARAMETERS" ) );

        for( i = 0; i < 15 && papszTokens[i] != NULL; i++ )
            adfProjParms[i] = CPLAtofM( papszTokens[i] );

        CSLDestroy( papszTokens );
    }

/* -------------------------------------------------------------------- */
/*      Handle projections.                                             */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszProjName,"UTM") && nZone != 0 )
    {
        // honestly, I'm just getting that the negative zone for 
        // southern hemisphere is used.
        oSRS.SetUTM( ABS(nZone), nZone > 0 );
    }

    else if( EQUAL(pszProjName,"State Plane") && nZone != 0 )
    {
        int		nPairs = sizeof(anUsgsEsriZones) / (2*sizeof(int));
        int		i;
        double          dfUnits = 0.0;
        
        for( i = 0; i < nPairs; i++ )
        {
            if( anUsgsEsriZones[i*2+1] == nZone )
            {
                nZone = anUsgsEsriZones[i*2];
                break;
            }
            
        }

        if( EQUAL(pszUnits,"feet") )
            dfUnits = CPLAtofM(SRS_UL_US_FOOT_CONV);
        else if( EQUALN(pszUnits,"MET",3) )
            dfUnits = 1.0;
        else
            pszUnits = NULL;

        oSRS.SetStatePlane( ABS(nZone), 
                            pszDatumName==NULL || !EQUAL(pszDatumName,"NAD27"),
                            pszUnits, dfUnits );
    }

/* -------------------------------------------------------------------- */
/*      Setup the geographic coordinate system.                         */
/* -------------------------------------------------------------------- */
    if( oSRS.GetAttrNode( "GEOGCS" ) == NULL )
    {
        if( pszDatumName != NULL 
            && oSRS.SetWellKnownGeogCS( pszDatumName ) == OGRERR_NONE )
        {
            // good
        }
        else if( CSLFetchNameValue( papszHdr, "SPHEROID_NAME" ) 
                 && CSLFetchNameValue( papszHdr, "SEMI_MAJOR_AXIS" )
                 && CSLFetchNameValue( papszHdr, "SEMI_MINOR_AXIS" ) )
        {
            double dfSemiMajor = CPLAtofM(CSLFetchNameValue( papszHdr, "SEMI_MAJOR_AXIS"));
            double dfSemiMinor = CPLAtofM(CSLFetchNameValue( papszHdr, "SEMI_MINOR_AXIS"));
            
            oSRS.SetGeogCS( CSLFetchNameValue( papszHdr, "SPHEROID_NAME" ),
                            CSLFetchNameValue( papszHdr, "SPHEROID_NAME" ),
                            CSLFetchNameValue( papszHdr, "SPHEROID_NAME" ),
                            dfSemiMajor, 
                            1.0 / (1.0 - dfSemiMinor/dfSemiMajor) );
        }
        else // fallback default.
            oSRS.SetWellKnownGeogCS( "WGS84" );
    }

/* -------------------------------------------------------------------- */
/*      Convert to WKT.                                                 */
/* -------------------------------------------------------------------- */
    CPLFree( pszProjection );
    pszProjection = NULL;
    
    oSRS.exportToWkt( &pszProjection );
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *GenBinDataset::Open( GDALOpenInfo * poOpenInfo )

{
    int		i, bSelectedHDR;
    
/* -------------------------------------------------------------------- */
/*	We assume the user is pointing to the binary (ie. .bil) file.	*/
/* -------------------------------------------------------------------- */
    if( poOpenInfo->nHeaderBytes < 2 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Now we need to tear apart tfhe filename to form a .HDR           */
/*      filename.                                                       */
/* -------------------------------------------------------------------- */
    CPLString osPath = CPLGetPath( poOpenInfo->pszFilename );
    CPLString osName = CPLGetBasename( poOpenInfo->pszFilename );
    CPLString osHDRFilename;

    if( poOpenInfo->papszSiblingFiles )
    {
        int iFile = CSLFindString(poOpenInfo->papszSiblingFiles, 
                                  CPLFormFilename( NULL, osName, "hdr" ) );
        if( iFile < 0 ) // return if there is no corresponding .hdr file
            return NULL;

        osHDRFilename = 
            CPLFormFilename( osPath, poOpenInfo->papszSiblingFiles[iFile],
                             NULL );
    }
    else
    {
        osHDRFilename = CPLFormCIFilename( osPath, osName, "hdr" );
    }

    bSelectedHDR = EQUAL( osHDRFilename, poOpenInfo->pszFilename );

/* -------------------------------------------------------------------- */
/*      Do we have a .hdr file?                                         */
/* -------------------------------------------------------------------- */
    FILE	*fp;

    fp = VSIFOpenL( osHDRFilename, "r" );
    
    if( fp == NULL )
    {
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Read a chunk to skim for expected keywords.                     */
/* -------------------------------------------------------------------- */
    char achHeader[1000];
    
    int nRead = VSIFReadL( achHeader, 1, sizeof(achHeader) - 1, fp );
    achHeader[nRead] = '\0';
    VSIFSeekL( fp, 0, SEEK_SET );

    if( strstr( achHeader, "BANDS:" ) == NULL 
        || strstr( achHeader, "ROWS:" ) == NULL 
        || strstr( achHeader, "COLS:" ) == NULL )
    {
        VSIFCloseL( fp );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Has the user selected the .hdr file to open?                    */
/* -------------------------------------------------------------------- */
    if( bSelectedHDR )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "The selected file is an Generic Binary header file, but to\n"
                  "open Generic Binary datasets, the data file should be selected\n"
                  "instead of the .hdr file.  Please try again selecting\n"
                  "the raw data file corresponding to the header file: %s\n", 
                  poOpenInfo->pszFilename );
        VSIFCloseL( fp );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Read the .hdr file.                                             */
/* -------------------------------------------------------------------- */
    char **papszHdr = NULL;
    const char *pszLine = CPLReadLineL( fp );

    while( pszLine != NULL )
    {
        if( EQUAL(pszLine,"PROJECTION_PARAMETERS:") )
        {
            CPLString osPP = pszLine;

            pszLine = CPLReadLineL(fp);
            while( pszLine != NULL 
                   && (*pszLine == '\t' || *pszLine == ' ') )
            {
                osPP += pszLine;
                pszLine = CPLReadLineL(fp);
            }
            papszHdr = CSLAddString( papszHdr, osPP );
        }
        else
        {
            char *pszName;
            CPLString osValue;
            
            osValue = CPLParseNameValue( pszLine, &pszName );
            osValue.Trim();
            
            papszHdr = CSLSetNameValue( papszHdr, pszName, osValue );
            CPLFree( pszName );
            
            pszLine = CPLReadLineL( fp );
        }
    }

    VSIFCloseL( fp );

    if( CSLFetchNameValue( papszHdr, "COLS" ) == NULL
        || CSLFetchNameValue( papszHdr, "ROWS" ) == NULL
        || CSLFetchNameValue( papszHdr, "BANDS" ) == NULL )
    {
        CSLDestroy( papszHdr );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    GenBinDataset     *poDS;

    poDS = new GenBinDataset();

/* -------------------------------------------------------------------- */
/*      Capture some information from the file that is of interest.     */
/* -------------------------------------------------------------------- */
    int nBands = atoi(CSLFetchNameValue( papszHdr, "BANDS" ));

    poDS->nRasterXSize = atoi(CSLFetchNameValue( papszHdr, "COLS" ));
    poDS->nRasterYSize = atoi(CSLFetchNameValue( papszHdr, "ROWS" ));
    poDS->papszHDR = papszHdr;

    if (!GDALCheckDatasetDimensions(poDS->nRasterXSize, poDS->nRasterYSize) ||
        !GDALCheckBandCount(nBands, FALSE))
    {
        delete poDS;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Open target binary file.                                        */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->eAccess == GA_ReadOnly )
        poDS->fpImage = VSIFOpenL( poOpenInfo->pszFilename, "rb" );
    else
        poDS->fpImage = VSIFOpenL( poOpenInfo->pszFilename, "r+b" );

    if( poDS->fpImage == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Failed to open %s with write permission.\n%s", 
                  osName.c_str(), VSIStrerror( errno ) );
        delete poDS;
        return NULL;
    }

    poDS->eAccess = poOpenInfo->eAccess;

/* -------------------------------------------------------------------- */
/*      Figure out the data type.                                       */
/* -------------------------------------------------------------------- */
    const char *pszDataType = CSLFetchNameValue( papszHdr, "DATATYPE" );
    GDALDataType eDataType;
    int nBits = -1; // Only needed for partial byte types

    if( pszDataType == NULL )
        eDataType = GDT_Byte;
    else if( EQUAL(pszDataType,"U16") )
        eDataType = GDT_UInt16;
    else if( EQUAL(pszDataType,"S16") )
        eDataType = GDT_Int16;
    else if( EQUAL(pszDataType,"F32") )
        eDataType = GDT_Float32;
    else if( EQUAL(pszDataType,"F64") )
        eDataType = GDT_Float64;
    else if( EQUAL(pszDataType,"U8") )
        eDataType = GDT_Byte;
    else if( EQUAL(pszDataType,"U1") 
             || EQUAL(pszDataType,"U2")
             || EQUAL(pszDataType,"U4") )
    {
        nBits = atoi(pszDataType+1);
        eDataType = GDT_Byte;
        if( nBands != 1 )
        {
            CPLError( CE_Failure, CPLE_OpenFailed, 
                      "Only one band is supported for U1/U2/U4 data type" );
            delete poDS;
            return NULL;
        }
    }
    else
    {
        eDataType = GDT_Byte;
        CPLError( CE_Warning, CPLE_AppDefined,
                  "DATATYPE=%s not recognised, assuming Byte.",
                  pszDataType );
    }

/* -------------------------------------------------------------------- */
/*      Do we need byte swapping?                                       */
/* -------------------------------------------------------------------- */
    const char *pszBYTE_ORDER = CSLFetchNameValue(papszHdr,"BYTE_ORDER");
    int bNative = TRUE;
    
    if( pszBYTE_ORDER != NULL )
    {
#ifdef CPL_LSB
        bNative = EQUALN(pszBYTE_ORDER,"LSB",3);
#else
        bNative = !EQUALN(pszBYTE_ORDER,"LSB",3);
#endif        
    }

/* -------------------------------------------------------------------- */
/*	Work out interleaving info.					*/
/* -------------------------------------------------------------------- */
    int nItemSize = GDALGetDataTypeSize(eDataType)/8;
    const char *pszInterleaving = CSLFetchNameValue(papszHdr,"INTERLEAVING");
    int             nPixelOffset, nLineOffset;
    vsi_l_offset    nBandOffset;
    int bIntOverflow = FALSE;

    if( pszInterleaving == NULL )
        pszInterleaving = "BIL";
    
    if( EQUAL(pszInterleaving,"BSQ") || EQUAL(pszInterleaving,"NA") )
    {
        nPixelOffset = nItemSize;
        if (poDS->nRasterXSize > INT_MAX / nItemSize) bIntOverflow = TRUE;
        nLineOffset = nItemSize * poDS->nRasterXSize;
        nBandOffset = nLineOffset * poDS->nRasterYSize;
    }
    else if( EQUAL(pszInterleaving,"BIP") )
    {
        nPixelOffset = nItemSize * nBands;
        if (poDS->nRasterXSize > INT_MAX / nPixelOffset) bIntOverflow = TRUE;
        nLineOffset = nPixelOffset * poDS->nRasterXSize;
        nBandOffset = nItemSize;
    }
    else
    {
        if( !EQUAL(pszInterleaving,"BIL") )
            CPLError( CE_Warning, CPLE_AppDefined,
                      "INTERLEAVING:%s not recognised, assume BIL.",
                      pszInterleaving );

        nPixelOffset = nItemSize;
        if (poDS->nRasterXSize > INT_MAX / (nPixelOffset * nBands)) bIntOverflow = TRUE;
        nLineOffset = nPixelOffset * nBands * poDS->nRasterXSize;
        nBandOffset = nItemSize * poDS->nRasterXSize;
    }

    if (bIntOverflow)
    {
        delete poDS;
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Int overflow occured.");
        return NULL;
    }

    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->PamInitialize();

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    poDS->nBands = nBands;
    for( i = 0; i < poDS->nBands; i++ )
    {
        if( nBits != -1 )
        {
            poDS->SetBand( i+1, new GenBinBitRasterBand( poDS, nBits ) );
        }
        else
            poDS->SetBand( 
                i+1, 
                new RawRasterBand( poDS, i+1, poDS->fpImage,
                                   nBandOffset * i, nPixelOffset, nLineOffset,
                                   eDataType, bNative, TRUE ) );
    }

/* -------------------------------------------------------------------- */
/*      Get geotransform.                                               */
/* -------------------------------------------------------------------- */
    if( CSLFetchNameValue(papszHdr,"UL_X_COORDINATE") != NULL
        && CSLFetchNameValue(papszHdr,"UL_Y_COORDINATE") != NULL
        && CSLFetchNameValue(papszHdr,"LR_X_COORDINATE") != NULL
        && CSLFetchNameValue(papszHdr,"LR_Y_COORDINATE") != NULL )
    {
        double dfULX = CPLAtofM(CSLFetchNameValue(papszHdr,"UL_X_COORDINATE"));
        double dfULY = CPLAtofM(CSLFetchNameValue(papszHdr,"UL_Y_COORDINATE"));
        double dfLRX = CPLAtofM(CSLFetchNameValue(papszHdr,"LR_X_COORDINATE"));
        double dfLRY = CPLAtofM(CSLFetchNameValue(papszHdr,"LR_Y_COORDINATE"));

        poDS->adfGeoTransform[1] = (dfLRX - dfULX) / (poDS->nRasterXSize-1);
        poDS->adfGeoTransform[2] = 0.0;
        poDS->adfGeoTransform[4] = 0.0;
        poDS->adfGeoTransform[5] = (dfLRY - dfULY) / (poDS->nRasterYSize-1);

        poDS->adfGeoTransform[0] = dfULX - poDS->adfGeoTransform[1] * 0.5;
        poDS->adfGeoTransform[3] = dfULY - poDS->adfGeoTransform[5] * 0.5;

        poDS->bGotTransform = TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Try and parse the coordinate system.                            */
/* -------------------------------------------------------------------- */
    poDS->ParseCoordinateSystem( papszHdr );

/* -------------------------------------------------------------------- */
/*      Initialize any PAM information.                                 */
/* -------------------------------------------------------------------- */
    poDS->TryLoadXML();
    
/* -------------------------------------------------------------------- */
/*      Check for overviews.                                            */
/* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );

    return( poDS );
}

/************************************************************************/
/*                         GDALRegister_GenBin()                          */
/************************************************************************/

void GDALRegister_GenBin()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "GenBin" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "GenBin" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Generic Binary (.hdr Labelled)" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#GenBin" );
        poDriver->pfnOpen = GenBinDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
