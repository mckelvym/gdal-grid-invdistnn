/******************************************************************************
 * $Id$
 *
 * Project:  Hierarchical Data Format Release 4 (HDF4)
 * Purpose:  HDF4 Datasets. Open HDF4 file, fetch metadata and list of
 *           subdatasets.
 *           This driver initially based on code supplied by Markus Neteler
 * Author:   Andrey Kiselev, dron@ak4719.spb.edu
 *
 ******************************************************************************
 * Copyright (c) 2002, Andrey Kiselev <dron@ak4719.spb.edu>
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

#include "hdf.h"
#include "mfhdf.h"

#include "HdfEosDef.h"

#include "gdal_priv.h"
#include "cpl_string.h"
#include "cpl_multiproc.h"

#include "hdf4compat.h"
#include "hdf4dataset.h"

#include <algorithm>

CPL_CVSID("$Id$");

CPL_C_START
void	GDALRegister_HDF4(void);
CPL_C_END

extern const char *pszGDALSignature;

void *hHDF4Mutex = NULL;

class HDFVdataField
{
public:
    HDFVdataField() : iType( 0 ),
                      oName()
    {
    }

    HDFVdataField( int32 iVdataID, int32 iField )
    {
        iType = VFfieldtype( iVdataID, iField );
        if ( iType == FAIL )
            iType = 0;
        char *pszName = VFfieldname( iVdataID, iField );
        if ( pszName )
            oName = pszName;
    }

    HDFVdataField( const HDFVdataField &other ) : iType( other.iType ),
                                                  oName( other.oName )
    {
    }

    HDFVdataField& operator=( const HDFVdataField &other )
    {
        iType = other.iType;
        oName = other.oName;
        return (*this);
    }

    int32 iType;
    CPLString oName;
};

bool NotPrintableChar( char c )
{
    return !isprint( (unsigned)c );
}

CPLString CleanHDFNameString( const char *pszName )
{
    CPLString osReturn = "";
    if ( !pszName )
        return osReturn;
    osReturn = pszName;
    size_t nStart = osReturn.find_first_not_of( '/' );
    osReturn.erase( 0, nStart );
    for( CPLString::iterator oIterator = osReturn.begin(); oIterator != osReturn.end(); ++oIterator )
    {
        if ( *oIterator == ' ' )
            *oIterator = '_';
        else if ( *oIterator == '/' )
            *oIterator = '_';
    }
    osReturn.erase( std::remove_if( osReturn.begin(), osReturn.end(), NotPrintableChar ), osReturn.end() );
    return osReturn;
}

/************************************************************************/
/* ==================================================================== */
/*				HDF4Dataset				*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           HDF4Dataset()                              */
/************************************************************************/

HDF4Dataset::HDF4Dataset()

{
    fp = NULL;
    hSD = 0;
    hGR = 0;
    nImages = 0;
    iSubdatasetType = H4ST_UNKNOWN;
    pszSubdatasetType = NULL;
    papszGlobalMetadata = NULL;
    papszSubDatasets = NULL;
    bIsHDFEOS = 0;
}

/************************************************************************/
/*                            ~HDF4Dataset()                            */
/************************************************************************/

HDF4Dataset::~HDF4Dataset()

{
    CPLMutexHolderD(&hHDF4Mutex);

    if ( hSD )
	SDend( hSD );
    if ( hGR )
	GRend( hGR );
    if ( papszSubDatasets )
	CSLDestroy( papszSubDatasets );
    if ( papszGlobalMetadata )
	CSLDestroy( papszGlobalMetadata );
    if( fp != NULL )
        VSIFClose( fp );
}

/************************************************************************/
/*                            GetMetadata()                             */
/************************************************************************/

char **HDF4Dataset::GetMetadata( const char *pszDomain )

{
    if( pszDomain != NULL && EQUALN( pszDomain, "SUBDATASETS", 11 ) )
        return papszSubDatasets;
    else
        return GDALDataset::GetMetadata( pszDomain );
}

/************************************************************************/
/*                           SPrintArray()                              */
/*	Prints numerical arrays in string buffer.			*/
/*	This function takes pfaDataArray as a pointer to printed array,	*/
/*	nValues as a number of values to print and pszDelimiter as a	*/
/*	field delimiting strings.					*/
/*	Pointer to filled buffer will be returned.			*/
/************************************************************************/

char *SPrintArray( GDALDataType eDataType, const void *paDataArray,
                          int nValues, const char *pszDelimiter )
{
    char        *pszString, *pszField;
    int         i, iFieldSize, iStringSize;

    iFieldSize = 32 + strlen( pszDelimiter );
    pszField = (char *)CPLMalloc( iFieldSize + 1 );
    iStringSize = nValues * iFieldSize + 1;
    pszString = (char *)CPLMalloc( iStringSize );
    memset( pszString, 0, iStringSize );
    for ( i = 0; i < nValues; i++ )
    {
        switch ( eDataType )
        {
            case GDT_Byte:
                sprintf( pszField, "%d%s", ((GByte *)paDataArray)[i],
                     (i < nValues - 1)?pszDelimiter:"" );
                break;
            case GDT_UInt16:
                sprintf( pszField, "%u%s", ((GUInt16 *)paDataArray)[i],
                     (i < nValues - 1)?pszDelimiter:"" );
                break;
            case GDT_Int16:
            default:
                sprintf( pszField, "%d%s", ((GInt16 *)paDataArray)[i],
                     (i < nValues - 1)?pszDelimiter:"" );
                break;
            case GDT_UInt32:
                sprintf( pszField, "%u%s", ((GUInt32 *)paDataArray)[i],
                     (i < nValues - 1)?pszDelimiter:"" );
                break;
            case GDT_Int32:
                sprintf( pszField, "%d%s", ((GInt32 *)paDataArray)[i],
                     (i < nValues - 1)?pszDelimiter:"" );
                break;
            case GDT_Float32:
                sprintf( pszField, "%.10g%s", ((float *)paDataArray)[i],
                     (i < nValues - 1)?pszDelimiter:"" );
                break;
            case GDT_Float64:
                sprintf( pszField, "%.15g%s", ((double *)paDataArray)[i],
                     (i < nValues - 1)?pszDelimiter:"" );
                break;
        }
        strcat( pszString, pszField );
    }
    
    CPLFree( pszField );
    return pszString;
}

/************************************************************************/
/*              Translate HDF4 data type into GDAL data type            */
/************************************************************************/
GDALDataType HDF4Dataset::GetDataType( int32 iNumType )
{
    switch (iNumType)
    {
        case DFNT_CHAR8: // The same as DFNT_CHAR
        case DFNT_UCHAR8: // The same as DFNT_UCHAR
        case DFNT_INT8:
        case DFNT_UINT8:
            return GDT_Byte;
        case DFNT_INT16:
            return GDT_Int16;
        case DFNT_UINT16:
            return GDT_UInt16;
        case DFNT_INT32:
            return GDT_Int32;
        case DFNT_UINT32:
            return GDT_UInt32;
        case DFNT_INT64:
            return GDT_Unknown;
        case DFNT_UINT64:
            return GDT_Unknown;
        case DFNT_FLOAT32:
            return GDT_Float32;
        case DFNT_FLOAT64:
            return GDT_Float64;
        default:
            return GDT_Unknown;
    }
}

/************************************************************************/
/*		Return the human readable name of data type		*/
/************************************************************************/

const char *HDF4Dataset::GetDataTypeName( int32 iNumType )
{
    switch (iNumType)
    {
        case DFNT_CHAR8: // The same as DFNT_CHAR
	    return "8-bit character";
	case DFNT_UCHAR8: // The same as DFNT_UCHAR
	    return "8-bit unsigned character";
        case DFNT_INT8:
	    return "8-bit integer";
        case DFNT_UINT8:
	    return "8-bit unsigned integer";
        case DFNT_INT16:
	    return "16-bit integer";
        case DFNT_UINT16:
	    return "16-bit unsigned integer";
        case DFNT_INT32:
	    return "32-bit integer";
        case DFNT_UINT32:
	    return "32-bit unsigned integer";
        case DFNT_INT64:
	    return "64-bit integer";
        case DFNT_UINT64:
	    return "64-bit unsigned integer";
        case DFNT_FLOAT32:
	    return "32-bit floating-point";
        case DFNT_FLOAT64:
	    return "64-bit floating-point";
	default:
	    return "unknown type";
    }
}

/************************************************************************/
/*  Return the size of data type in bytes	                        */
/************************************************************************/

int HDF4Dataset::GetDataTypeSize( int32 iNumType )
{
    switch (iNumType)
    {
        case DFNT_CHAR8: // The same as DFNT_CHAR
        case DFNT_UCHAR8: // The same as DFNT_UCHAR
        case DFNT_INT8:
        case DFNT_UINT8:
            return 1;
        case DFNT_INT16:
        case DFNT_UINT16:
            return 2;
        case DFNT_INT32:
        case DFNT_UINT32:
        case DFNT_FLOAT32:
            return 4;
        case DFNT_INT64:
        case DFNT_UINT64:
        case DFNT_FLOAT64:
            return 8;
        default:
            return 0;
    }
}

/************************************************************************/
/*  Convert value stored in the input buffer to double value.           */
/************************************************************************/

double HDF4Dataset::AnyTypeToDouble( int32 iNumType, void *pData )
{
    switch ( iNumType )
    {
        case DFNT_INT8:
            return (double)*(char *)pData;
        case DFNT_UINT8:
            return (double)*(unsigned char *)pData;
        case DFNT_INT16:
            return (double)*(short *)pData;
        case DFNT_UINT16:
            return (double)*(unsigned short *)pData;
        case DFNT_INT32:
            return (double)*(long *)pData;
        case DFNT_UINT32:
            return (double)*(unsigned long *)pData;
        case DFNT_INT64:
            return (double)*(char *)pData;
        case DFNT_UINT64:
            return (double)*(GIntBig *)pData;
        case DFNT_FLOAT32:
            return (double)*(float *)pData;
        case DFNT_FLOAT64:
            return (double)*(double *)pData;
        default:
            return 0.0;
    }
}

/************************************************************************/
/*         Tokenize HDF-EOS attributes.                                 */
/************************************************************************/

char **HDF4Dataset::HDF4EOSTokenizeAttrs( const char * pszString ) 

{
    const char  *pszDelimiters = " \t\n\r";
    char        **papszRetList = NULL;
    char        *pszToken;
    int         nTokenMax, nTokenLen;

    pszToken = (char *) CPLCalloc( 10, 1 );
    nTokenMax = 10;
    
    while( pszString != NULL && *pszString != '\0' )
    {
        int     bInString = FALSE, bInBracket = FALSE;

        nTokenLen = 0;
        
        // Try to find the next delimeter, marking end of token
        for( ; *pszString != '\0'; pszString++ )
        {

            // End if this is a delimeter skip it and break.
            if ( !bInBracket && !bInString
                 && strchr(pszDelimiters, *pszString) != NULL )
            {
                pszString++;
                break;
            }

            // Sometimes in bracketed tokens we may found a sort of
            // paragraph formatting. We will remove unneeded spaces and new
            // lines. 
            if ( bInBracket )
                if ( strchr("\r\n", *pszString) != NULL
                     || ( *pszString == ' '
                          && strchr(" \r\n", *(pszString - 1)) != NULL ) )
                continue;
            
            if ( *pszString == '"' )
            {
                if ( bInString )
                {
                    bInString = FALSE;
                    continue;
                }
                else
                {
                    bInString = TRUE;
                    continue;
                }
            }
            else if ( *pszString == '(' )
	    {
                bInBracket = TRUE;
		continue;
	    }
	    else if ( *pszString == ')' )
	    {
		bInBracket = FALSE;
		continue;
	    }

	    if( nTokenLen >= nTokenMax - 2 )
            {
                nTokenMax = nTokenMax * 2 + 10;
                pszToken = (char *) CPLRealloc( pszToken, nTokenMax );
            }

            pszToken[nTokenLen] = *pszString;
            nTokenLen++;
        }

        pszToken[nTokenLen] = '\0';

        if( pszToken[0] != '\0' )
        {
            papszRetList = CSLAddString( papszRetList, pszToken );
        }

        // If the last token is an empty token, then we have to catch
        // it now, otherwise we won't reenter the loop and it will be lost. 
        if ( *pszString == '\0' && strchr(pszDelimiters, *(pszString-1)) )
        {
            papszRetList = CSLAddString( papszRetList, "" );
        }
    }

    if( papszRetList == NULL )
        papszRetList = (char **) CPLCalloc( sizeof(char *), 1 );

    CPLFree( pszToken );

    return papszRetList;
}

/************************************************************************/
/*     Find object name and its value in HDF-EOS attributes.            */
/*     Function returns pointer to the string in list next behind       */
/*     recognized object.                                               */
/************************************************************************/

char **HDF4Dataset::HDF4EOSGetObject( char **papszAttrList, char **ppszAttrName,
                                      char **ppszAttrValue )
{
    int	    iCount, i, j;
    *ppszAttrName = NULL;
    *ppszAttrValue = NULL;

    iCount = CSLCount( papszAttrList );
    for ( i = 0; i < iCount - 2; i++ )
    {
	if ( EQUAL( papszAttrList[i], "OBJECT" ) )
	{
	    i += 2;
	    for ( j = 1; i + j < iCount - 2; j++ )
	    {
	        if ( EQUAL( papszAttrList[i + j], "END_OBJECT" ) ||
		     EQUAL( papszAttrList[i + j], "OBJECT" ) )
	            return &papszAttrList[i + j];
	        else if ( EQUAL( papszAttrList[i + j], "VALUE" ) )
	        {
		    *ppszAttrName = papszAttrList[i];
	            *ppszAttrValue = papszAttrList[i + j + 2];

		    return &papszAttrList[i + j + 2];
	        }
	    }
	}
    }

    return NULL;
}

/************************************************************************/
/*         Translate HDF4-EOS attributes in GDAL metadata items         */
/************************************************************************/

char** HDF4Dataset::TranslateHDF4EOSAttributes( int32 iHandle,
    int32 iAttribute, int32 nValues, char **papszMetadata )
{
    char	*pszData;
    
    pszData = (char *)CPLMalloc( (nValues + 1) * sizeof(char) );
    pszData[nValues] = '\0';
    SDreadattr( iHandle, iAttribute, pszData );
    // HDF4-EOS attributes has followed structure:
    // 
    // GROUP = <name>
    //   GROUPTYPE = <name>
    //
    //   GROUP = <name>
    //   
    //     OBJECT = <name>
    //       CLASS = <string>
    //       NUM_VAL = <number>
    //       VALUE = <string> or <number>
    //     END_OBJECT = <name>
    //     
    //     .......
    //     .......
    //     .......
    //     
    //   END_GROUP = <name>
    //   
    // .......
    // .......
    // .......
    //
    // END_GROUP = <name>
    // END
    //
    // Used data types:
    // <name>   --- unquoted character strings
    // <string> --- quoted character strings
    // <number> --- numerical value
    // If NUM_VAL != 1 then values combined in lists:
    // (<string>,<string>,...)
    // or
    // (<number>,<number>,...)
    //
    // Records within objects may follows in any order, objects may contains
    // other objects (and lacks VALUE record), groups contains other groups
    // and objects. Names of groups and objects are not unique and may repeat.
    // Objects may contains other types of records.
    //
    // We are interested in OBJECTS structures only.

    char *pszAttrName, *pszAttrValue;
    char *pszAddAttrName = NULL;
    char **papszAttrList, **papszAttrs;
    
    papszAttrList = HDF4EOSTokenizeAttrs( pszData );
    papszAttrs = papszAttrList;
    while ( papszAttrs )
    {
	papszAttrs =
	    HDF4EOSGetObject( papszAttrs, &pszAttrName, &pszAttrValue );
	if ( pszAttrName && pszAttrValue )
	{
	    // Now we should recognize special type of HDF EOS metastructures:
	    // ADDITIONALATTRIBUTENAME = <name>
	    // PARAMETERVALUE = <value>
	    if ( EQUAL( pszAttrName, "ADDITIONALATTRIBUTENAME" ) )
		pszAddAttrName = pszAttrValue;
	    else if ( pszAddAttrName && EQUAL( pszAttrName, "PARAMETERVALUE" ) )
	    {
		papszMetadata =
		    CSLAddNameValue( papszMetadata, pszAddAttrName, pszAttrValue );
		pszAddAttrName = NULL;
	    }
	    else
	    {
		papszMetadata =
		    CSLAddNameValue( papszMetadata, pszAttrName, pszAttrValue );
	    }
	}
    }
    
    CSLDestroy( papszAttrList );
    CPLFree( pszData );

    return papszMetadata;
}

/************************************************************************/
/*         Translate HDF4 attributes in GDAL metadata items             */
/************************************************************************/

char** HDF4Dataset::TranslateHDF4Attributes( int32 iHandle,
    int32 iAttribute, char *pszAttrName, int32 iNumType, int32 nValues,
    char **papszMetadata )
{
    void	*pData = NULL;
    char	*pszTemp = NULL;
    
/* -------------------------------------------------------------------- */
/*     Allocate a buffer to hold the attribute data.                    */
/* -------------------------------------------------------------------- */
    if ( iNumType == DFNT_CHAR8 || iNumType == DFNT_UCHAR8 )
        pData = CPLMalloc( (nValues + 1) * GetDataTypeSize(iNumType) );
    else
        pData = CPLMalloc( nValues * GetDataTypeSize(iNumType) );

/* -------------------------------------------------------------------- */
/*     Read the attribute data.                                         */
/* -------------------------------------------------------------------- */
    SDreadattr( iHandle, iAttribute, pData );
    if ( iNumType == DFNT_CHAR8 || iNumType == DFNT_UCHAR8 )
    {
        ((char *)pData)[nValues] = '\0';
        papszMetadata = CSLAddNameValue( papszMetadata, pszAttrName, 
                                         (const char *) pData );
    }
    else
    {
        pszTemp = SPrintArray( GetDataType(iNumType), pData, nValues, ", " );
        papszMetadata = CSLAddNameValue( papszMetadata, pszAttrName, pszTemp );
        if ( pszTemp )
	    CPLFree( pszTemp );
    }
    
    if ( pData )
	CPLFree( pData );

    return papszMetadata;
}

/************************************************************************/
/*      Translate HDF-EOS attributes into Esri MULTIDIMENSION format    */
/************************************************************************/

void HDF4Dataset::TranslateHDF4EOSAttributesMD( int32 iHandle, int32 iAttribute,
                                                int32 nValues, const char *pszVarName )
{
    char	   *pszData;
    char       *pszAttrName, *pszAttrValue;
    char       *pszAddAttrName = NULL;
    char       **papszAttrList, **papszAttrs;
    CPLString  osName;

    pszData = (char *)CPLMalloc( (nValues + 1) * sizeof(char) );
    pszData[nValues] = '\0';
    SDreadattr( iHandle, iAttribute, pszData );

    papszAttrList = HDF4EOSTokenizeAttrs( pszData );
    papszAttrs = papszAttrList;
    while ( papszAttrs )
    {
        papszAttrs =
            HDF4EOSGetObject( papszAttrs, &pszAttrName, &pszAttrValue );
        if ( pszAttrName && pszAttrValue )
        {
            // Now we should recognize special type of HDF EOS metastructures:
            // ADDITIONALATTRIBUTENAME = <name>
            // PARAMETERVALUE = <value>
            if ( EQUAL( pszAttrName, "ADDITIONALATTRIBUTENAME" ) )
                pszAddAttrName = pszAttrValue;
            else if ( pszAddAttrName && EQUAL( pszAttrName, "PARAMETERVALUE" ) )
            {
                if ( pszVarName )
                    osName.Printf( "%s#%s", pszVarName, pszAddAttrName );
                else
                    osName.Printf( "GLOBAL#%s", pszAddAttrName );
                SetMetadataItem( osName.c_str(), pszAttrValue, ESRI_DMD_MD );
                pszAddAttrName = NULL;
            }
            else
            {
                if ( pszVarName )
                    osName.Printf( "%s#%s", pszVarName, pszAttrName );
                else
                    osName.Printf( "GLOBAL#%s", pszAttrName );
                SetMetadataItem( osName.c_str(), pszAttrValue, ESRI_DMD_MD );
            }
        }
    }

    CSLDestroy( papszAttrList );
    CPLFree( pszData );
}

/************************************************************************/
/*         Translate HDF4 attributes into Esri MULTIDIMENSION format    */
/************************************************************************/

void HDF4Dataset::TranslateHDF4AttributesMD( int32 iHandle, int32 iAttribute, 
                                             char *pszAttrName, int32 iNumType, 
                                             int32 nValues, const char *pszVarName )
{
    void        *pData = NULL;
    char        *pszTemp = NULL;
    CPLString    osName;
    
    if ( pszVarName )
        osName.Printf( "%s#%s", pszVarName, pszAttrName );
    else
        osName.Printf( "GLOBAL#%s", pszAttrName );
    
    /* -------------------------------------------------------------------- */
    /*     Allocate a buffer to hold the attribute data.                    */
    /* -------------------------------------------------------------------- */
    if ( iNumType == DFNT_CHAR8 || iNumType == DFNT_UCHAR8 )
        pData = CPLMalloc( (nValues + 1) * GetDataTypeSize(iNumType) );
    else
        pData = CPLMalloc( nValues * GetDataTypeSize(iNumType) );
    if ( !pData )
    {
        return;
    }
    
    /* -------------------------------------------------------------------- */
    /*     Read the attribute data.                                         */
    /* -------------------------------------------------------------------- */
    if ( SDreadattr( iHandle, iAttribute, pData ) != SUCCEED )
    {
        CPLFree( pData );
        return;
    }
    
    if ( iNumType == DFNT_CHAR8 || iNumType == DFNT_UCHAR8 )
    {
        ((char *)pData)[nValues] = '\0';
        SetMetadataItem( osName.c_str(), (const char *) pData, ESRI_DMD_MD );
    }
    else
    {
        pszTemp = SPrintArray( GetDataType(iNumType), pData, nValues, ", " );
        if ( !pszTemp )
        {
            CPLFree( pData );
            return;
        }
        SetMetadataItem( osName.c_str(), (const char *) pszTemp, ESRI_DMD_MD );
        CPLFree( pszTemp );
    }
    
    CPLFree( pData );
    
    return;
}

/************************************************************************/
/*                       ReadGlobalAttributes()                         */
/************************************************************************/

CPLErr HDF4Dataset::ReadGlobalAttributes( int32 iHandler )
{
    int32	iAttribute, nValues, iNumType, nDatasets, nAttributes;
    char	szAttrName[H4_MAX_NC_NAME];

/* -------------------------------------------------------------------- */
/*     Obtain number of SDSs and global attributes in input file.       */
/* -------------------------------------------------------------------- */
    if ( SDfileinfo( iHandler, &nDatasets, &nAttributes ) != 0 )
	return CE_Failure;

    // Loop through the all attributes
    for ( iAttribute = 0; iAttribute < nAttributes; iAttribute++ )
    {
        // Get information about the attribute. Note that the first
        // parameter is an SD interface identifier.
        SDattrinfo( iHandler, iAttribute, szAttrName, &iNumType, &nValues );

        if ( EQUALN( szAttrName, "coremetadata", 12 )    ||
	     EQUALN( szAttrName, "archivemetadata.", 16 ) ||
	     EQUALN( szAttrName, "productmetadata.", 16 ) ||
             EQUALN( szAttrName, "badpixelinformation", 19 ) ||
	     EQUALN( szAttrName, "product_summary", 15 ) ||
	     EQUALN( szAttrName, "dem_specific", 12 ) ||
	     EQUALN( szAttrName, "bts_specific", 12 ) ||
	     EQUALN( szAttrName, "etse_specific", 13 ) ||
	     EQUALN( szAttrName, "dst_specific", 12 ) ||
	     EQUALN( szAttrName, "acv_specific", 12 ) ||
	     EQUALN( szAttrName, "act_specific", 12 ) ||
	     EQUALN( szAttrName, "etst_specific", 13 ) ||
	     EQUALN( szAttrName, "level_1_carryover", 17 ) )
        {
            bIsHDFEOS = 1;
            papszGlobalMetadata = TranslateHDF4EOSAttributes( iHandler,
		iAttribute, nValues, papszGlobalMetadata );
        }

        // Skip "StructMetadata.N" records. We will fetch information
        // from them using HDF-EOS API
	else if ( EQUALN( szAttrName, "structmetadata.", 15 ) )
        {
            bIsHDFEOS = 1;
            continue;
        }

        else
        {
	    papszGlobalMetadata = TranslateHDF4Attributes( iHandler,
		iAttribute, szAttrName,	iNumType, nValues, papszGlobalMetadata );
        }
    }

    return CE_None;
}

void HDF4Dataset::ExtractGlobalMetadataMD(int32 iHandle )
{
    int32       iAttribute, nValues, iNumType, nDatasets, nAttributes;
    char        szAttrName[H4_MAX_NC_NAME];
    
    /* -------------------------------------------------------------------- */
    /*     Obtain number of SDSs and global attributes in input file.       */
    /* -------------------------------------------------------------------- */
    if ( SDfileinfo( iHandle, &nDatasets, &nAttributes ) != 0 )
        return;
    
    // Loop through the all attributes
    for ( iAttribute = 0; iAttribute < nAttributes; iAttribute++ )
    {
        // Get information about the attribute. Note that the first
        // parameter is an SD interface identifier.
        SDattrinfo( iHandle, iAttribute, szAttrName, &iNumType, &nValues );
        
        if ( EQUALN( szAttrName, "coremetadata.", 13 )    ||
            EQUALN( szAttrName, "archivemetadata.", 16 ) ||
            EQUALN( szAttrName, "productmetadata.", 16 ) ||
            EQUALN( szAttrName, "badpixelinformation", 19 ) ||
            EQUALN( szAttrName, "product_summary", 15 ) ||
            EQUALN( szAttrName, "dem_specific", 12 ) ||
            EQUALN( szAttrName, "bts_specific", 12 ) ||
            EQUALN( szAttrName, "etse_specific", 13 ) ||
            EQUALN( szAttrName, "dst_specific", 12 ) ||
            EQUALN( szAttrName, "acv_specific", 12 ) ||
            EQUALN( szAttrName, "act_specific", 12 ) ||
            EQUALN( szAttrName, "etst_specific", 13 ) ||
            EQUALN( szAttrName, "level_1_carryover", 17 ) )
        {
            TranslateHDF4EOSAttributesMD( iHandle, iAttribute, nValues, "GLOBAL" );
        }        
        else if ( EQUALN( szAttrName, "structmetadata.", 15 ) )
        {
            continue;
        }        
        else
        {
            TranslateHDF4AttributesMD( iHandle, iAttribute, szAttrName, iNumType, nValues, "GLOBAL" );
        }
    }
}

void HDF4Dataset::ExtractFieldMetadataMD( const char *pszParent, const char *pszField, int32 iRank, int32 aiDimSizes[],
                                          const CPLStringList& oDimList, const CPLString& osVarName )
{
    CPLString osName, osTemp, osDimNames;
    char szTemp[256];
    int iXDim, iYDim;

    osName.Printf( "%s#description", osVarName.c_str() );
    osTemp.Printf( "//%s/%s", pszParent, pszField );
    SetMetadataItem( osName.c_str(), osTemp.c_str(), ESRI_DMD_MD );

    iXDim = HDF4Dataset::GetXIndex( iRank, oDimList );
    iYDim = HDF4Dataset::GetYIndex( iRank, oDimList );

    if (iRank == 3)
    {
        int iBandIndex = 0;
        osTemp = "";
        for( int i = 0; i < iRank; i++ )
        {
            sprintf(szTemp, "%d", (int) aiDimSizes[i]);
            if( i < iRank - 1 )
              strcat(szTemp,  "x" );
            osTemp += szTemp;
            if ( i != iXDim && i != iYDim )
                iBandIndex = i;
        }
        if( !osTemp.empty() )
        {
            sprintf( szTemp, "%s#MD_DIM_SIZES", osVarName.c_str() );
            SetMetadataItem( szTemp, osTemp.c_str(), ESRI_DMD_MD );
        }

        osName.Printf( "%s#MD_DIM_%s_DEF", osVarName.c_str(), oDimList[iBandIndex] );
        osTemp.Printf( "{%d,9}", aiDimSizes[iBandIndex] );
        SetMetadataItem( osName.c_str(), osTemp.c_str(), ESRI_DMD_MD );

        osDimNames.Printf("{%s}", oDimList[iBandIndex] );
        sprintf( szTemp, "%s#MD_DIMS", osVarName.c_str() );
        SetMetadataItem( szTemp, osDimNames.c_str(), ESRI_DMD_MD );
    }
}

void HDF4Dataset::ExtractSDMetadataMD( const char *pszField, int32 iRank, int32 aiDimSizes[],
                                       const CPLString& osVarName, int32 iSDS, int32 nAttrs )
{
    CPLString                 osName, osTemp, osDimNames;
    char                      szTemp[8192];
    int32                     nSize, iDataType;
    std::vector< CPLString >  oDimList;

    osName.Printf( "%s#description", osVarName.c_str() );
    SetMetadataItem( osName.c_str(), pszField, ESRI_DMD_MD );

    if (iRank == 3)
    {
        osTemp = "";
        for( int i = 0; i < iRank; i++ )
        {
            sprintf(szTemp, "%d", (int) aiDimSizes[i]);
            if( i < iRank - 1 )
              strcat(szTemp,  "x" );
            osTemp += szTemp;
        }
        if( !osTemp.empty() )
        {
            sprintf( szTemp, "%s#MD_DIM_SIZES", osVarName.c_str() );
            SetMetadataItem( szTemp, osTemp.c_str(), ESRI_DMD_MD );
        }

        for( int j = 0; j < iRank; j++ )
        {
            CPLString osDimName;
            osDimName.Printf( "dim%d", j );
            oDimList.push_back( osDimName );

            int32 iDimID = SDgetdimid( iSDS, j );
            if ( iDimID != -1 )
            {
                int32 nDimAttrs;
                memset( szTemp, 0, 8192 );
                if ( SDdiminfo( iDimID, szTemp, &nSize, &iDataType, &nDimAttrs ) == 0 )
                {
                    if ( strlen( szTemp ) )
                    {
                        osTemp.Printf( "%s#description", osDimName.c_str() );
                        SetMetadataItem( osTemp.c_str(), szTemp, ESRI_DMD_MD );
                    }

                    for ( int32 k = 0; k < nDimAttrs; k++ )
                    {
                        if ( SDattrinfo( iDimID, k, szTemp, &iDataType, &nSize ) == 0 )
                            TranslateHDF4AttributesMD( iDimID, k, szTemp, iDataType, nSize, osDimName.c_str() );
                    }
                }
            }
        }

        osDimNames = "{";
        for( int j = iRank - 1; j >= 2; j-- )
        {
            osDimNames += oDimList[j];
            if ( j > 2 )
                osDimNames += ",";
        }
        osDimNames += "}";
        sprintf( szTemp, "%s#MD_DIMS", osVarName.c_str() );
        SetMetadataItem( szTemp, osDimNames.c_str(), ESRI_DMD_MD );
    }

    for ( int32 k = 0; k < nAttrs; k++ )
    {
        if ( SDattrinfo( iSDS, k, szTemp, &iDataType, &nSize ) == 0 )
            TranslateHDF4AttributesMD( iSDS, k, szTemp, iDataType, nSize, osVarName.c_str() );
    }
}

void HDF4Dataset::ExtractGRMetadataMD( const char *pszField, const CPLString& osVarName, int32 iGR, int32 nAttrs )
{
    CPLString osName;
    char szTemp[8192];
    int32 nSize, iDataType;

    osName.Printf( "%s#description", osVarName.c_str() );
    SetMetadataItem( osName.c_str(), pszField, ESRI_DMD_MD );

    for ( int32 k = 0; k < nAttrs; k++ )
    {
        if ( SDattrinfo( iGR, k, szTemp, &iDataType, &nSize ) == 0 )
            TranslateHDF4AttributesMD( iGR, k, szTemp, iDataType, nSize, osVarName.c_str() );
    }
}

void HDF4Dataset::ExtractAllVdata( int32 iHandle )
{
    int32 iVdataRef = -1;

    int status = Vstart( iHandle );
    while ( ( iVdataRef = VSgetid (iHandle, iVdataRef) ) != FAIL )
    {
        int32 iVdataID = VSattach( iHandle, iVdataRef, "r" );
        if ( iVdataID == FAIL )
        {
            CPLError( CE_Warning, CPLE_AppDefined,
                      "VSattach failed on Vdata %d, skipping.", iVdataRef );
            continue;
        }
        if( VSisattr( iVdataID ) != TRUE )
            ExtractVdata( iHandle, iVdataID );
        status = VSdetach( iVdataID );
    }
    status = Vend( iHandle );
}

void HDF4Dataset::ExtractVdata( int32 iHandle, int32 iVdataID )
{
//    int32 nRecords, iVdataSize, iMode, nFields;
//    char pszFieldList[4096], pszVdataName[VSNAMELENMAX];
//    uint8 *pData = NULL;

//    memset( pszVdataName, 0, VSNAMELENMAX );
//    memset( pszFieldList, 0, 4096 );

//    nFields = VFnfields( iVdataID );
//    int status = VSinquire( iVdataID, &nRecords, &iMode,
//                            pszFieldList, &iVdataSize, pszVdataName );
//    if ( status == FAIL )
//    {
//        CPLError( CE_Warning, CPLE_AppDefined,
//                  "VSinquire failed on Vdata %d, skipping.", iVdataID );
//        return;
//    }

//    CPLStringList oFieldList( CSLTokenizeString2( pszFieldList, ",", CSLT_STRIPENDSPACES | CSLT_STRIPLEADSPACES ) );
//    if ( !oFieldList.size() )
//    {
//        CPLError( CE_Warning, CPLE_AppDefined,
//                  "Unable to get field names from Vdata %d, skipping.", iVdataID );
//        return;
//    }

//    pData = (uint8 *)VSIMalloc( iVdataSize );
//    for ( int i = 0; i < nRecords; ++i )
//    {
//        for (int j = 0; j < oFieldList.size(); ++j)
//        {
//            VSsetfields( iVdataID, oFieldList[j] );
//            VSseek( iVdataID, i );
//            VSread( iVdataID, pData, 1, NO_INTERLACE );
//        }
//    }
//    if ( pData )
//        VSIFree( pData );
}

int HDF4Dataset::GetYIndex( int32 iRank, const CPLStringList& oDimList )
{
    int iY = iRank - 2;

    if ( oDimList.Count() )
    {
        for ( int i = 0; i < oDimList.Count(); i++ )
        {
            if ( strstr( oDimList[i], "band" ) )
                continue;

            if ( EQUALN( oDimList[i], "Y", 1 ) ||
                 EQUALN( oDimList[i], "nlat", 4 ) ||
                 EQUALN( oDimList[i], "ydim", 4 ) ||
                 EQUALN( oDimList[i], "cell_across_swath", 17 ) )
            {
                iY = i;
                break;
            }
        }
    }
    return iY;
}

int HDF4Dataset::GetXIndex( int32 iRank, const CPLStringList& oDimList )
{
    int iX = iRank - 1;

    if ( oDimList.Count() )
    {
        for ( int i = 0; i < oDimList.Count(); i++ )
        {
            if ( strstr( oDimList[i], "band" ) )
                continue;

            if ( EQUALN( oDimList[i], "X", 1 ) ||
                 EQUALN( oDimList[i], "nlon", 4 ) ||
                 EQUALN( oDimList[i], "xdim", 4 ) ||
                 EQUALN( oDimList[i], "cell_along_swath", 16 ) )
            {
                iX = i;
                break;
            }
        }
    }
    return iX;
}

int HDF4Dataset::GetBandIndex( int iRank, const CPLStringList& oDimList )
{
    int iX = HDF4Dataset::GetXIndex( iRank, oDimList );
    int iY = HDF4Dataset::GetYIndex( iRank, oDimList );

    for ( int i = 0; i < iRank; i++ )
    {
        if ( i != iX && i != iY )
            return i;
    }
    return 0;
}

/************************************************************************/
/*                              Identify()                              */
/************************************************************************/

int HDF4Dataset::Identify( GDALOpenInfo * poOpenInfo )

{
    if( poOpenInfo->nHeaderBytes < 4 )
        return FALSE;

    if( memcmp(poOpenInfo->pabyHeader,"\016\003\023\001",4) != 0 )
        return FALSE;

    return TRUE;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *HDF4Dataset::Open( GDALOpenInfo * poOpenInfo )

{
    int32	                   i;
    long int                   nVarCount = 0;
    std::vector< CPLString >   aosVarList;

    if( !Identify( poOpenInfo ) )
        return NULL;

    CPLMutexHolderD(&hHDF4Mutex);

/* -------------------------------------------------------------------- */
/*      Try opening the dataset.                                        */
/* -------------------------------------------------------------------- */
    int32	hHDF4;
    
    // Attempt to increase maximum number of opened HDF files
#ifdef HDF4_HAS_MAXOPENFILES
    intn        nCurrMax, nSysLimit;

    if ( SDget_maxopenfiles(&nCurrMax, &nSysLimit) >= 0
         && nCurrMax < nSysLimit )
    {
        intn res = SDreset_maxopenfiles( nSysLimit );
    }
#endif /* HDF4_HAS_MAXOPENFILES */

    hHDF4 = Hopen(poOpenInfo->pszFilename, DFACC_READ, 0);
    
    if( hHDF4 <= 0 )
        return( NULL );

    Hclose( hHDF4 );

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    HDF4Dataset *poDS;

    CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
    poDS = new HDF4Dataset();
    CPLAcquireMutex(hHDF4Mutex, 1000.0);

    poDS->fp = poOpenInfo->fp;
    poOpenInfo->fp = NULL;
    
/* -------------------------------------------------------------------- */
/*          Open HDF SDS Interface.                                     */
/* -------------------------------------------------------------------- */
    poDS->hSD = SDstart( poOpenInfo->pszFilename, DFACC_READ );

    if ( poDS->hSD == -1 )
    {
        CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
        delete poDS;
        CPLAcquireMutex(hHDF4Mutex, 1000.0);
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Failed to open HDF4 file \"%s\" for SDS reading.\n",
                  poOpenInfo->pszFilename );
        return NULL;
    }
   
/* -------------------------------------------------------------------- */
/*		Now read Global Attributes.				*/
/* -------------------------------------------------------------------- */
    if ( poDS->ReadGlobalAttributes( poDS->hSD ) != CE_None )
    {
        CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
        delete poDS;
        CPLAcquireMutex(hHDF4Mutex, 1000.0);
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Failed to read global attributes from HDF4 file \"%s\".\n",
                  poOpenInfo->pszFilename );
        return NULL;
    }

    poDS->SetMetadata( poDS->papszGlobalMetadata, "" );

    poDS->ExtractGlobalMetadataMD( poDS->hSD );

/* -------------------------------------------------------------------- */
/*		Determine type of file we read.				*/
/* -------------------------------------------------------------------- */
    const char	*pszValue;
    
    if ( (pszValue = CSLFetchNameValue(poDS->papszGlobalMetadata,
                                       "Signature"))
	 && EQUAL( pszValue, pszGDALSignature ) )
    {
	poDS->iSubdatasetType = H4ST_GDAL;
	poDS->pszSubdatasetType = "GDAL_HDF4";
    }

    else if ( (pszValue = CSLFetchNameValue(poDS->papszGlobalMetadata, "Title"))
	 && EQUAL( pszValue, "SeaWiFS Level-1A Data" ) )
    {
	poDS->iSubdatasetType = H4ST_SEAWIFS_L1A;
	poDS->pszSubdatasetType = "SEAWIFS_L1A";
    }

    else if ( (pszValue = CSLFetchNameValue(poDS->papszGlobalMetadata, "Title"))
	&& EQUAL( pszValue, "SeaWiFS Level-2 Data" ) )
    {
	poDS->iSubdatasetType = H4ST_SEAWIFS_L2;
	poDS->pszSubdatasetType = "SEAWIFS_L2";
    }

    else if ( (pszValue = CSLFetchNameValue(poDS->papszGlobalMetadata, "Title"))
	&& EQUAL( pszValue, "SeaWiFS Level-3 Standard Mapped Image" ) )
    {
	poDS->iSubdatasetType = H4ST_SEAWIFS_L3;
	poDS->pszSubdatasetType = "SEAWIFS_L3";
    }

    else if ( (pszValue = CSLFetchNameValue(poDS->papszGlobalMetadata,
                                            "L1 File Generated By"))
	&& EQUALN( pszValue, "HYP version ", 12 ) )
    {
	poDS->iSubdatasetType = H4ST_HYPERION_L1;
	poDS->pszSubdatasetType = "HYPERION_L1";
    }

    else
    {
	poDS->iSubdatasetType = H4ST_UNKNOWN;
	poDS->pszSubdatasetType = "UNKNOWN";
    }

/* -------------------------------------------------------------------- */
/*  If we have HDF-EOS dataset, process it here.	                */
/* -------------------------------------------------------------------- */
    char	szName[VSNAMELENMAX + 1], szTemp[8192];
    char	*pszString;
    const char  *pszName;
    int		nCount;
    int32	aiDimSizes[H4_MAX_VAR_DIMS];
    int32	iRank, iNumType, nAttrs;
    bool    bIsHDF = true;
    bool    bGotSwathOrGrid = false;
    
    // Sometimes "HDFEOSVersion" attribute is not defined and we will
    // determine HDF-EOS datasets using other records
    // (see ReadGlobalAttributes() method).
    if ( poDS->bIsHDFEOS
         || CSLFetchNameValue(poDS->papszGlobalMetadata, "HDFEOSVersion") )
    {
        bIsHDF  = false;

        int32   nSubDatasets, nStrBufSize;

/* -------------------------------------------------------------------- */
/*  Process swath layers.                                               */
/* -------------------------------------------------------------------- */
        hHDF4 = SWopen( poOpenInfo->pszFilename, DFACC_READ );
        if( hHDF4 < 0)
        {
            CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
            delete poDS;
            CPLAcquireMutex(hHDF4Mutex, 1000.0);
            CPLError( CE_Failure, CPLE_OpenFailed,
                      "Failed to open HDF-EOS file \"%s\" for swath reading.\n",
                      poOpenInfo->pszFilename );
            return NULL;
        } 
        nSubDatasets = SWinqswath(poOpenInfo->pszFilename, NULL, &nStrBufSize);
#ifdef DEBUG
        CPLDebug( "HDF4", "Number of HDF-EOS swaths: %d", (int)nSubDatasets );
#endif
        if ( nSubDatasets > 0 && nStrBufSize > 0 )
        {
            char    *pszSwathList;
            char    **papszSwaths;

            bGotSwathOrGrid = true;
            pszSwathList = (char *)CPLMalloc( nStrBufSize + 1 );
            SWinqswath( poOpenInfo->pszFilename, pszSwathList, &nStrBufSize );
            pszSwathList[nStrBufSize] = '\0';

#ifdef DEBUG
            CPLDebug( "HDF4", "List of HDF-EOS swaths: %s", pszSwathList );
#endif

            papszSwaths =
                CSLTokenizeString2( pszSwathList, ",", CSLT_HONOURSTRINGS );
            CPLFree( pszSwathList );

            if ( nSubDatasets != CSLCount(papszSwaths) )
            {
                CSLDestroy( papszSwaths );
                CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
                delete poDS;
                CPLAcquireMutex(hHDF4Mutex, 1000.0);
                CPLDebug( "HDF4", "Can not parse list of HDF-EOS grids." );
                return NULL;
            }

            for ( i = 0; i < nSubDatasets; i++)
            {
                char    *pszFieldList;
                char    **papszFields;
                int32   *paiRank, *paiNumType;
                int32   hSW, nFields, j;

                hSW = SWattach( hHDF4, papszSwaths[i] );

                nFields = SWnentries( hSW, HDFE_NENTDFLD, &nStrBufSize );
                pszFieldList = (char *)CPLMalloc( nStrBufSize + 1 );
                paiRank = (int32 *)CPLMalloc( nFields * sizeof(int32) );
                paiNumType = (int32 *)CPLMalloc( nFields * sizeof(int32) );

                SWinqdatafields( hSW, pszFieldList, paiRank, paiNumType );

#ifdef DEBUG
                {
                    char *pszTmp =
                        SPrintArray( GDT_UInt32, paiRank, nFields, "," );

                    CPLDebug( "HDF4", "Number of data fields in swath %d: %d",
                              (int) i, (int) nFields );
                    CPLDebug( "HDF4", "List of data fields in swath %d: %s",
                              (int) i, pszFieldList );
                    CPLDebug( "HDF4", "Data fields ranks: %s", pszTmp );

                    CPLFree( pszTmp );
                }
#endif

                papszFields = CSLTokenizeString2( pszFieldList, ",",
                                                  CSLT_HONOURSTRINGS );
                
                for ( j = 0; j < nFields; j++ )
                {
                    int nXSize, nYSize;
                    CPLString osDescription, osVarName;
                    CPLStringList oDimList;

                    SWfieldinfo( hSW, papszFields[j], &iRank, aiDimSizes,
                                 &iNumType, szTemp );

                    if ( iRank < 2 )
                        continue;

                    if ( strlen( szTemp ) )
                       oDimList.Assign( CSLTokenizeString2( szTemp, ",",
                                                            CSLT_HONOURSTRINGS ) );
                    if ( oDimList.Count() != iRank )
                    {
                        CPLString osTemp;
                        for ( int j = 0; j < iRank; j++ )
                        {
                            osTemp.Printf( "dim%d", j );
                            oDimList.AddString( osTemp.c_str() );
                        }
                    }

                    osVarName = CleanHDFNameString( papszFields[j] );
                    if (osVarName.empty() )
                        osVarName.Printf( "var%ld", nVarCount );
                    aosVarList.push_back( osVarName );
                    poDS->ExtractFieldMetadataMD( papszSwaths[i], papszFields[j], iRank, aiDimSizes, oDimList, osVarName );

                    if ( iRank == 3 )
                    {
                        nXSize = aiDimSizes[HDF4Dataset::GetXIndex( iRank, oDimList )];
                        nYSize = aiDimSizes[HDF4Dataset::GetYIndex( iRank, oDimList )];
                        int iBandIndex = HDF4Dataset::GetBandIndex( iRank, oDimList );


                        int32 nDimCount = aiDimSizes[iBandIndex];
                        CPLString osDimName;

                        if ( oDimList.Count() )
                            osDimName = oDimList[iBandIndex];

                        for ( long nDim = 0; nDim < nDimCount; nDim ++ )
                        {
                            if ( osDimName.empty() )
                                osDimName.Printf( "dim%d", (int)nDim );
                            // Add field to the list of GDAL subdatasets
                                nCount = CSLCount( poDS->papszSubDatasets ) / 2;
                                sprintf( szTemp, "SUBDATASET_%d_NAME", nCount + 1 );
                            // We will use the field index as an identificator.
                                poDS->papszSubDatasets =
                                    CSLSetNameValue(poDS->papszSubDatasets, szTemp,
                                            CPLSPrintf( "HDF4_EOS:EOS_SWATH:\"%s\":%s:%s:@%ld",
                                                        poOpenInfo->pszFilename,
                                                        papszSwaths[i], papszFields[j], nDim));

                                sprintf( szTemp, "SUBDATASET_%d_DESC", nCount + 1 );
                                osDescription.Printf( "variable:\"%s\"; size:\"%dx%d\"; datatype:\"%s\"; %s:\"%ld\";",
                                                      osVarName.c_str(), nXSize, nYSize, poDS->GetDataTypeName( iNumType ),
                                                      osDimName.c_str(), nDim );
                                poDS->papszSubDatasets =
                                    CSLSetNameValue( poDS->papszSubDatasets, szTemp, osDescription.c_str() );
                        }
                    }
                    else if ( iRank == 2 )
                    {
                        nXSize = aiDimSizes[0];
                        nYSize = aiDimSizes[1];

                        // Add field to the list of GDAL subdatasets
                        nCount = CSLCount( poDS->papszSubDatasets ) / 2;
                        sprintf( szTemp, "SUBDATASET_%d_NAME", nCount + 1 );
                        // We will use the field index as an identificator.
                        poDS->papszSubDatasets =
                            CSLSetNameValue( poDS->papszSubDatasets, szTemp,
                                    CPLSPrintf("HDF4_EOS:EOS_SWATH:\"%s\":%s:%s",
                                               poOpenInfo->pszFilename,
                                               papszSwaths[i], papszFields[j]) );

                        sprintf( szTemp, "SUBDATASET_%d_DESC", nCount + 1 );
                        osDescription.Printf( "variable:\"%s\"; size:\"%dx%d\"; datatype:\"%s\";",
                                              osVarName.c_str(), nXSize, nYSize, poDS->GetDataTypeName( iNumType ) );
                        poDS->papszSubDatasets =
                            CSLSetNameValue( poDS->papszSubDatasets, szTemp, osDescription.c_str() );
                    }
                    nVarCount++;
                }

                CSLDestroy( papszFields );
                CPLFree( paiNumType );
                CPLFree( paiRank );
                CPLFree( pszFieldList );
                SWdetach( hSW );
            }

            CSLDestroy( papszSwaths );
        }
        SWclose( hHDF4 );

/* -------------------------------------------------------------------- */
/*  Process grid layers.                                                */
/* -------------------------------------------------------------------- */
        hHDF4 = GDopen( poOpenInfo->pszFilename, DFACC_READ );
        if( hHDF4 < 0)
        {
            delete poDS;
            CPLError( CE_Failure, CPLE_OpenFailed, "Failed to open HDF4 `%s'.\n", poOpenInfo->pszFilename );
            return NULL;
        }

        nSubDatasets = GDinqgrid( poOpenInfo->pszFilename, NULL, &nStrBufSize );
#ifdef DEBUG
        CPLDebug( "HDF4", "Number of HDF-EOS grids: %d", (int)nSubDatasets );
#endif
        if ( nSubDatasets > 0 && nStrBufSize > 0 )
        {
            char    *pszGridList;
            char    **papszGrids;

            bGotSwathOrGrid = true;
            pszGridList = (char *)CPLMalloc( nStrBufSize + 1 );
            GDinqgrid( poOpenInfo->pszFilename, pszGridList, &nStrBufSize );

#ifdef DEBUG
            CPLDebug( "HDF4", "List of HDF-EOS grids: %s", pszGridList );
#endif

            papszGrids =
                CSLTokenizeString2( pszGridList, ",", CSLT_HONOURSTRINGS );
            CPLFree( pszGridList );

            if ( nSubDatasets != CSLCount(papszGrids) )
            {
                CSLDestroy( papszGrids );
                GDclose( hHDF4 ); 
                CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
                delete poDS;
                CPLAcquireMutex(hHDF4Mutex, 1000.0);
                CPLDebug( "HDF4", "Can not parse list of HDF-EOS grids." );
                return NULL;
            }

            for ( i = 0; i < nSubDatasets; i++)
            {
                char    *pszFieldList;
                char    **papszFields;
                int32   *paiRank, *paiNumType;
                int32   hGD, nFields, j;

                hGD = GDattach( hHDF4, papszGrids[i] );

                nFields = GDnentries( hGD, HDFE_NENTDFLD, &nStrBufSize );
                pszFieldList = (char *)CPLMalloc( nStrBufSize + 1 );
                paiRank = (int32 *)CPLMalloc( nFields * sizeof(int32) );
                paiNumType = (int32 *)CPLMalloc( nFields * sizeof(int32) );

                GDinqfields( hGD, pszFieldList, paiRank, paiNumType );

#ifdef DEBUG
                {
                    char* pszTmp =
                            SPrintArray( GDT_UInt32, paiRank, nFields, "," );
                    CPLDebug( "HDF4", "Number of fields in grid %d: %d",
                            (int) i, (int) nFields );
                    CPLDebug( "HDF4", "List of fields in grid %d: %s",
                            (int) i, pszFieldList );
                    CPLDebug( "HDF4", "Fields ranks: %s",
                            pszTmp );
                    CPLFree( pszTmp );
                }
#endif

                papszFields = CSLTokenizeString2( pszFieldList, ",",
                                                  CSLT_HONOURSTRINGS );
                
                for ( j = 0; j < nFields; j++ )
                {
                    int nXSize, nYSize;
                    CPLString osDescription, osVarName;
                    CPLStringList oDimList;

                    GDfieldinfo( hGD, papszFields[j], &iRank, aiDimSizes,
                                 &iNumType, szTemp );

                    if ( iRank < 2 )
                        continue;

                    if ( strlen( szTemp ) )
                       oDimList.Assign( CSLTokenizeString2( szTemp, ",",
                                                            CSLT_HONOURSTRINGS ) );
                    if ( oDimList.Count() != iRank )
                    {
                        CPLString osTemp;
                        for ( int j = 0; j < iRank; j++ )
                        {
                            osTemp.Printf( "dim%d", j );
                            oDimList.AddString( osTemp.c_str() );
                        }
                    }

                    osVarName = CleanHDFNameString( papszFields[j] );
                    if (osVarName.empty() )
                        osVarName.Printf( "var%ld", nVarCount );
                    aosVarList.push_back( osVarName );
                    poDS->ExtractFieldMetadataMD( papszGrids[i], papszFields[j], iRank, aiDimSizes, oDimList, osVarName );

                    if ( iRank == 3 )
                    {
                        nXSize = aiDimSizes[HDF4Dataset::GetXIndex( iRank, oDimList )];
                        nYSize = aiDimSizes[HDF4Dataset::GetYIndex( iRank, oDimList )];
                        int iBandIndex = HDF4Dataset::GetBandIndex( iRank, oDimList );

                        int32 nDimCount = aiDimSizes[iBandIndex];
                        CPLString osDimName;

                        if ( strlen( szTemp ) )
                        {
                            CPLStringList oDimList( CSLTokenizeString2( szTemp, ",",
                                                                        CSLT_HONOURSTRINGS ) );
                            if ( oDimList.Count() )
                                osDimName = oDimList[iBandIndex];
                        }
                        else
                        {
                            osDimName.Printf( "dim%d", iRank );
                        }

                        for ( long nDim = 0; nDim < nDimCount; nDim ++ )
                        {
                            if ( osDimName.empty() )
                                osDimName.Printf( "dim%d", (int)nDim );
                            // Add field to the list of GDAL subdatasets
                                nCount = CSLCount( poDS->papszSubDatasets ) / 2;
                                sprintf( szTemp, "SUBDATASET_%d_NAME", nCount + 1 );
                            // We will use the field index as an identificator.
                                poDS->papszSubDatasets =
                                    CSLSetNameValue(poDS->papszSubDatasets, szTemp,
                                            CPLSPrintf( "HDF4_EOS:EOS_GRID:\"%s\":%s:%s:@%ld",
                                                        poOpenInfo->pszFilename,
                                                        papszGrids[i], papszFields[j], nDim));

                                sprintf( szTemp, "SUBDATASET_%d_DESC", nCount + 1 );
                                osDescription.Printf( "variable:\"%s\"; size:\"%dx%d\"; datatype:\"%s\"; %s:\"%ld\";",
                                                      osVarName.c_str(), nXSize, nYSize, poDS->GetDataTypeName( iNumType ),
                                                      osDimName.c_str(), nDim );
                                poDS->papszSubDatasets =
                                    CSLSetNameValue( poDS->papszSubDatasets, szTemp, osDescription.c_str() );
                        }
                    }
                    else if ( iRank == 2 )
                    {
                        nXSize = aiDimSizes[HDF4Dataset::GetXIndex( iRank, oDimList )];
                        nYSize = aiDimSizes[HDF4Dataset::GetYIndex( iRank, oDimList )];

                        // Add field to the list of GDAL subdatasets
                        nCount = CSLCount( poDS->papszSubDatasets ) / 2;
                        sprintf( szTemp, "SUBDATASET_%d_NAME", nCount + 1 );
                        // We will use the field index as an identificator.
                        poDS->papszSubDatasets =
                            CSLSetNameValue(poDS->papszSubDatasets, szTemp,
                                    CPLSPrintf( "HDF4_EOS:EOS_GRID:\"%s\":%s:%s",
                                                poOpenInfo->pszFilename,
                                                papszGrids[i], papszFields[j]));

                        sprintf( szTemp, "SUBDATASET_%d_DESC", nCount + 1 );
                        osDescription.Printf( "variable:\"%s\"; size:\"%dx%d\"; datatype:\"%s\";",
                                              osVarName.c_str(), nXSize, nYSize, poDS->GetDataTypeName( iNumType ) );
                        poDS->papszSubDatasets =
                            CSLSetNameValue( poDS->papszSubDatasets, szTemp, osDescription.c_str() );
                    }
                    nVarCount++;
                }

                CSLDestroy( papszFields );
                CPLFree( paiNumType );
                CPLFree( paiRank );
                CPLFree( pszFieldList );
                GDdetach( hGD );
            }

            CSLDestroy( papszGrids );
        }
        GDclose( hHDF4 );

        bIsHDF = !bGotSwathOrGrid;         // Try to read as HDF if we haven't already
                                           // processed swaths or grids.
    }

    if( bIsHDF )
    {

/* -------------------------------------------------------------------- */
/*  Make a list of subdatasets from SDSs contained in input HDF file.	*/
/* -------------------------------------------------------------------- */
        int32   nDatasets;
        CPLString osDescription;

        if ( SDfileinfo( poDS->hSD, &nDatasets, &nAttrs ) != 0 )
	    return NULL;

        for ( i = 0; i < nDatasets; i++ )
        {
            int32	      iSDS;
            int           nXSize, nYSize;
            CPLString     osDescription, osVarName;

            iSDS = SDselect( poDS->hSD, i );
            if ( SDgetinfo( iSDS, szName, &iRank, aiDimSizes, &iNumType, &nAttrs) != 0 )
                return NULL;
            
            if ( iRank == 1 )		// Skip 1D datsets
                    continue;

            osVarName = CleanHDFNameString( szName );
            if (osVarName.empty() )
                osVarName.Printf( "var%ld", nVarCount );
            aosVarList.push_back( osVarName );
            poDS->ExtractSDMetadataMD( szName, iRank, aiDimSizes, osVarName, iSDS, nAttrs );

            // Do sort of known datasets. We will display only image bands
            if ( (poDS->iSubdatasetType == H4ST_SEAWIFS_L1A ) &&
                      !EQUALN( szName, "l1a_data", 8 ) )
                    continue;
            else
                pszName = szName;

            // TODO: Support 4D.
            if ( iRank == 3 )
            {
                int32 nDimCount = aiDimSizes[0];
                CPLString osDescription;

                nXSize = aiDimSizes[1];
                nYSize = aiDimSizes[0];

                for ( long nDim = 0; nDim < nDimCount; nDim ++ )
                {
                    // Add field to the list of GDAL subdatasets
                        nCount = CSLCount( poDS->papszSubDatasets ) / 2;
                        sprintf( szTemp, "SUBDATASET_%d_NAME", nCount + 1 );
                    // We will use the field index as an identificator.
                        poDS->papszSubDatasets =
                            CSLSetNameValue(poDS->papszSubDatasets, szTemp,
                                    CPLSPrintf( "HDF4_SDS:%s:\"%s\":%ld:@%ld", poDS->pszSubdatasetType,
                                                poOpenInfo->pszFilename, (long)i, nDim));

                        sprintf( szTemp, "SUBDATASET_%d_DESC", nCount + 1 );
                        osDescription.Printf( "variable:\"%s\"; size:\"%dx%d\"; datatype:\"%s\"; dim0:\"%ld\";",
                                              osVarName.c_str(), nXSize, nYSize, poDS->GetDataTypeName( iNumType ),
                                              nDim );
                        poDS->papszSubDatasets =
                            CSLSetNameValue( poDS->papszSubDatasets, szTemp, osDescription.c_str() );
                }
            }
            else if ( iRank == 2 )
            {

                nXSize = aiDimSizes[0];
                nYSize = aiDimSizes[1];

                // Add datasets with multiple dimensions to the list of GDAL subdatasets
                nCount = CSLCount( poDS->papszSubDatasets ) / 2;
                sprintf( szTemp, "SUBDATASET_%d_NAME", nCount + 1 );
                // We will use SDS index as an identificator, because SDS names
                // are not unique. Filename also needed for further file opening
                poDS->papszSubDatasets = CSLSetNameValue(poDS->papszSubDatasets, szTemp,
                      CPLSPrintf( "HDF4_SDS:%s:\"%s\":%ld", poDS->pszSubdatasetType,
                                  poOpenInfo->pszFilename, (long)i) );
                sprintf( szTemp, "SUBDATASET_%d_DESC", nCount + 1 );
                osDescription.Printf( "variable:\"%s\"; size:\"%dx%d\"; datatype:\"%s\";",
                                      osVarName.c_str(), nXSize, nYSize, poDS->GetDataTypeName( iNumType ) );
                poDS->papszSubDatasets =
                    CSLSetNameValue( poDS->papszSubDatasets, szTemp, osDescription.c_str() );
            }

            nVarCount++;
            SDendaccess( iSDS );
        }

        SDend( poDS->hSD );
        poDS->hSD = 0;
    }

/* -------------------------------------------------------------------- */
/*      Build a list of raster images. Note, that HDF-EOS dataset may   */
/*      contain a raster image as well.                                 */
/* -------------------------------------------------------------------- */

    hHDF4 = Hopen(poOpenInfo->pszFilename, DFACC_READ, 0);
    poDS->hGR = GRstart( hHDF4 );

    if ( poDS->hGR != -1 )
    {
        CPLString osDescription, osVarName;

        if ( GRfileinfo( poDS->hGR, &poDS->nImages, &nAttrs ) == -1 )
        {
            CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
            GRend( poDS->hGR );
            poDS->hGR = 0;
            Hclose( hHDF4 );
            delete poDS;
            CPLAcquireMutex(hHDF4Mutex, 1000.0);
            return NULL;
        }

        for ( i = 0; i < poDS->nImages; i++ )
        {
            int32   iInterlaceMode; 
            int32   iGR = GRselect( poDS->hGR, i );
            int     nXSize, nYSize;

            // iRank in GR interface has another meaning. It represents number
            // of samples per pixel. aiDimSizes has only two dimensions.
            if ( GRgetiminfo( iGR, szName, &iRank, &iNumType, &iInterlaceMode,
                              aiDimSizes, &nAttrs ) != 0 )
            {
                CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
                GRend( poDS->hGR );
                poDS->hGR = 0;
                Hclose( hHDF4 );
                delete poDS;
                CPLAcquireMutex(hHDF4Mutex, 1000.0);
                return NULL;
            }

            nXSize = aiDimSizes[0];
            nYSize = aiDimSizes[1];

            osVarName.Printf( "var%ld", nVarCount );
            aosVarList.push_back( osVarName );
            poDS->ExtractGRMetadataMD( szName, osVarName, iGR, nAttrs );

            nCount = CSLCount( poDS->papszSubDatasets ) / 2;
            sprintf( szTemp, "SUBDATASET_%d_NAME", nCount + 1 );
            poDS->papszSubDatasets = CSLSetNameValue(poDS->papszSubDatasets,
                szTemp,CPLSPrintf( "HDF4_GR:UNKNOWN:\"%s\":%ld",
                                   poOpenInfo->pszFilename, (long)i));
            sprintf( szTemp, "SUBDATASET_%d_DESC", nCount + 1 );
            osDescription.Printf( "variable:\"%s\"; size:\"%dx%d\"; datatype:\"%s\";",
                                  osVarName.c_str(), nXSize, nYSize, poDS->GetDataTypeName( iNumType ) );
            poDS->papszSubDatasets =
                CSLSetNameValue( poDS->papszSubDatasets, szTemp, osDescription.c_str() );

            nVarCount++;
            GRendaccess( iGR );
        }

        GRend( poDS->hGR );
        poDS->hGR = 0;
    }

    Hclose( hHDF4 );

    nVarCount = aosVarList.size();
    if ( nVarCount )
    {
        CPLString osNames = "{";
        for( int j = 0; j < nVarCount; j++ ) 
        {
            osNames += aosVarList[j];
            if ( j < nVarCount - 1 )
                osNames += ",";
        }
        osNames += "}";
        poDS->SetMetadataItem( "MD_VARIABLES", osNames.c_str(), ESRI_DMD_MD );
    }

    poDS->nRasterXSize = poDS->nRasterYSize = 512; // XXX: bogus values

    // Make sure we don't try to do any pam stuff with this dataset.
    poDS->nPamFlags |= GPF_NOSAVE;

    int bOpenSingleSubdataset = CSLTestBoolean(
        CPLGetConfigOption( "HDF_AUTO_OPEN_SUBDATASETS", "YES" ) );

/* -------------------------------------------------------------------- */
/*      If we have single subdataset only, open it immediately          */
/* -------------------------------------------------------------------- */
    if ( bOpenSingleSubdataset && ( CSLCount( poDS->papszSubDatasets ) / 2 == 1 ) )
    {
        char *pszSDSName;
        pszSDSName = CPLStrdup( CSLFetchNameValue( poDS->papszSubDatasets,
                            "SUBDATASET_1_NAME" ));
        CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
        delete poDS;
        poDS = NULL;

        GDALDataset* poRetDS = (GDALDataset*) GDALOpen( pszSDSName, poOpenInfo->eAccess );
        CPLFree( pszSDSName );

        CPLAcquireMutex(hHDF4Mutex, 1000.0);

        if (poRetDS)
        {
            poRetDS->SetDescription(poOpenInfo->pszFilename);
        }

        return poRetDS;
    }
    else
    {
/* -------------------------------------------------------------------- */
/*      Confirm the requested access is supported.                      */
/* -------------------------------------------------------------------- */
        if( poOpenInfo->eAccess == GA_Update )
        {
            CPLReleaseMutex(hHDF4Mutex); // Release mutex otherwise we'll deadlock with GDALDataset own mutex
            delete poDS;
            CPLAcquireMutex(hHDF4Mutex, 1000.0);

            CPLError( CE_Failure, CPLE_NotSupported, 
                      "The HDF4 driver does not support update access to existing"
                      " datasets.\n" );
            return NULL;
        }
    
    }

    return( poDS );
}

/************************************************************************/
/*                           HDF4UnloadDriver()                         */
/************************************************************************/

static void HDF4UnloadDriver(GDALDriver* poDriver)
{
    if( hHDF4Mutex != NULL )
        CPLDestroyMutex(hHDF4Mutex);
    hHDF4Mutex = NULL;
}

/************************************************************************/
/*                        GDALRegister_HDF4()				*/
/************************************************************************/

void GDALRegister_HDF4()

{
    GDALDriver	*poDriver;
    
    if (! GDAL_CHECK_VERSION("HDF4 driver"))
        return;

    if( GDALGetDriverByName( "HDF4" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "HDF4" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Hierarchical Data Format Release 4" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_hdf4.html" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "hdf" );
        poDriver->SetMetadataItem( GDAL_DMD_POSSIBLEEXTENSIONS, "hdf;hdf4;h4;he2" );
        poDriver->SetMetadataItem( GDAL_DMD_SUBDATASETS, "YES" );

        poDriver->pfnOpen = HDF4Dataset::Open;
        poDriver->pfnIdentify = HDF4Dataset::Identify;
        poDriver->pfnUnloadDriver = HDF4UnloadDriver;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

