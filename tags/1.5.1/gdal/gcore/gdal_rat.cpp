/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Implementation of GDALRasterAttributeTable and related classes.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2005, Frank Warmerdam
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

#include "gdal_priv.h"
#include "gdal_rat.h"

CPL_CVSID("$Id$");

/**
 * \class GDALRasterAttributeTable
 *
 * The GDALRasterAttributeTable (or RAT) class is used to encapsulate a table
 * used to provide attribute information about pixel values.  Each row
 * in the table applies to a range of pixel values (or a single value in
 * some cases), and might have attributes such as the histogram count for
 * that range, the color pixels of that range should be drawn names of classes
 * or any other generic information. 
 *
 * Raster attribute tables can be used to represent histograms, color tables,
 * and classification information.  
 *
 * Each column in a raster attribute table has a name, a type (integer,
 * floating point or string), and a GDALRATFieldUsage.  The usage distinguishes 
 * columns with particular understood purposes (such as color, histogram 
 * count, name) and columns that have specific purposes not understood by 
 * the library (long label, suitability_for_growing_wheat, etc).  
 *
 * In the general case each row has a column indicating the minimum pixel
 * values falling into that category, and a column indicating the maximum
 * pixel value.  These are indicated with usage values of GFU_Min, and
 * GFU_Max.  In other cases where each row is a discrete pixel value, one
 * column of usage GFU_MinMax can be used.  
 * 
 * In other cases all the categories are of equal size and regularly spaced 
 * and the categorization information can be determine just by knowing the
 * value at which the categories start, and the size of a category.  This
 * is called "Linear Binning" and the information is kept specially on 
 * the raster attribute table as a whole.
 *
 * RATs are normally associated with GDALRasterBands and be be queried
 * using the GDALRasterBand::GetDefaultRAT() method.  
 */

/************************************************************************/
/*                      GDALRasterAttributeTable()                      */
/*                                                                      */
/*      Simple initialization constructor.                              */
/************************************************************************/

//! Construct empty table.

GDALRasterAttributeTable::GDALRasterAttributeTable()

{
    bColumnsAnalysed = FALSE;
    nMinCol = -1;
    nMaxCol = -1;
    bLinearBinning = FALSE;
    dfRow0Min = -0.5;
    dfBinSize = 1.0;
    nRowCount = 0;
}

/************************************************************************/
/*                   GDALCreateRasterAttributeTable()                   */
/************************************************************************/

GDALRasterAttributeTableH CPL_STDCALL GDALCreateRasterAttributeTable()

{
    return (GDALRasterAttributeTableH) (new GDALRasterAttributeTable());
}

/************************************************************************/
/*                      GDALRasterAttributeTable()                      */
/************************************************************************/

//! Copy constructor.

GDALRasterAttributeTable::GDALRasterAttributeTable( 
    const GDALRasterAttributeTable &oOther )

{
    // We have tried to be careful to allow wholesale assignment
    *this = oOther;
}

/************************************************************************/
/*                     ~GDALRasterAttributeTable()                      */
/*                                                                      */
/*      All magic done by magic by the container destructors.           */
/************************************************************************/

GDALRasterAttributeTable::~GDALRasterAttributeTable()

{
}

/************************************************************************/
/*                  GDALDestroyRasterAttributeTable()                   */
/************************************************************************/

void CPL_STDCALL 
GDALDestroyRasterAttributeTable( GDALRasterAttributeTableH hRAT )

{
    if( hRAT != NULL )
        delete static_cast<GDALRasterAttributeTable *>(hRAT);
}

/************************************************************************/
/*                           AnalyseColumns()                           */
/*                                                                      */
/*      Internal method to work out which column to use for various     */
/*      tasks.                                                          */
/************************************************************************/

void GDALRasterAttributeTable::AnalyseColumns()

{
    bColumnsAnalysed = TRUE;

    nMinCol = GetColOfUsage( GFU_Min );
    if( nMinCol == -1 )
        nMinCol = GetColOfUsage( GFU_MinMax );

    nMaxCol = GetColOfUsage( GFU_Max );
    if( nMaxCol == -1 )
        nMaxCol = GetColOfUsage( GFU_MinMax );
}

/************************************************************************/
/*                           GetColumnCount()                           */
/************************************************************************/

/**
 * \brief Fetch table column count.
 *
 * This method is the same as the C function GDALRATGetColumnCount().
 *
 * @return the number of columns.
 */

int GDALRasterAttributeTable::GetColumnCount() const

{
    return aoFields.size();
}

/************************************************************************/
/*                       GDALRATGetColumnCount()                        */
/************************************************************************/

int CPL_STDCALL GDALRATGetColumnCount( GDALRasterAttributeTableH hRAT )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetColumnCount", 0 );

    return ((GDALRasterAttributeTable *) hRAT)->GetColumnCount();
}

/************************************************************************/
/*                            GetNameOfCol()                            */
/************************************************************************/

/**
 * \brief Fetch name of indicated column.
 *
 * This method is the same as the C function GDALRATGetNameOfCol(), except
 * that the C function returns "const char *".
 *
 * @param iCol the column index (zero based). 
 *
 * @return the column name or an empty string for invalid column numbers.
 */

const char *GDALRasterAttributeTable::GetNameOfCol( int iCol ) const

{
    if( iCol < 0 || iCol >= (int) aoFields.size() )
        return "";

    else
        return aoFields[iCol].sName;
}

/************************************************************************/
/*                        GDALRATGetNameOfCol()                         */
/************************************************************************/

const char *CPL_STDCALL GDALRATGetNameOfCol( GDALRasterAttributeTableH hRAT,
                                             int iCol )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetNameOfCol", NULL );

    // we don't just wrap the normal operator because we don't want to
    // return a temporary string, we want to return a pointer to the
    // internal column name. 

    GDALRasterAttributeTable *poRAT = (GDALRasterAttributeTable *) hRAT;

    if( iCol < 0 || iCol >= (int) poRAT->aoFields.size() )
        return "";

    else
        return poRAT->aoFields[iCol].sName.c_str();
}

/************************************************************************/
/*                           GetUsageOfCol()                            */
/************************************************************************/

/**
 * \brief Fetch column usage value. 
 *
 * This method is the same as the C function GDALRATGetUsageOfCol().
 *
 * @param iCol the column index (zero based).
 *
 * @return the column usage, or GFU_Generic for improper column numbers.
 */

GDALRATFieldUsage GDALRasterAttributeTable::GetUsageOfCol( int iCol ) const

{
    if( iCol < 0 || iCol >= (int) aoFields.size() )
        return GFU_Generic;

    else
        return aoFields[iCol].eUsage;
}

/************************************************************************/
/*                        GDALRATGetUsageOfCol()                        */
/************************************************************************/

GDALRATFieldUsage CPL_STDCALL 
GDALRATGetUsageOfCol( GDALRasterAttributeTableH hRAT, int iCol )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetUsageOfCol", GFU_Generic );

    return ((GDALRasterAttributeTable *) hRAT)->GetUsageOfCol( iCol );
}

/************************************************************************/
/*                            GetTypeOfCol()                            */
/************************************************************************/

/**
 * \brief Fetch color type.
 *
 * This method is the same as the C function GDALRATGetTypeOfCol().
 *
 * @param iCol the column index (zero based).
 *
 * @return color type or GFT_Integer if the column index is illegal.
 */

GDALRATFieldType GDALRasterAttributeTable::GetTypeOfCol( int iCol ) const

{
    if( iCol < 0 || iCol >= (int) aoFields.size() )
        return GFT_Integer;

    else
        return aoFields[iCol].eType;
}

/************************************************************************/
/*                        GDALRATGetTypeOfCol()                         */
/************************************************************************/

GDALRATFieldType CPL_STDCALL 
GDALRATGetTypeOfCol( GDALRasterAttributeTableH hRAT, int iCol )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetTypeOfCol", GFT_Integer );

    return ((GDALRasterAttributeTable *) hRAT)->GetTypeOfCol( iCol );
}

/************************************************************************/
/*                           GetColOfUsage()                            */
/************************************************************************/

/**
 * \brief Fetch column index for given usage.
 *
 * Returns the index of the first column of the requested usage type, or -1 
 * if no match is found. 
 *
 * This method is the same as the C function GDALRATGetUsageOfCol().
 *
 * @param eUsage usage type to search for.
 *
 * @return column index, or -1 on failure. 
 */

int GDALRasterAttributeTable::GetColOfUsage( GDALRATFieldUsage eUsage ) const

{
    unsigned int i;

    for( i = 0; i < aoFields.size(); i++ )
    {
        if( aoFields[i].eUsage == eUsage )
            return i;
    }

    return -1;
}

/************************************************************************/
/*                        GDALRATGetColOfUsage()                        */
/************************************************************************/

int CPL_STDCALL 
GDALRATGetColOfUsage( GDALRasterAttributeTableH hRAT, 
                      GDALRATFieldUsage eUsage )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetColOfUsage", 0 );

    return ((GDALRasterAttributeTable *) hRAT)->GetColOfUsage( eUsage );
}

/************************************************************************/
/*                            GetRowCount()                             */
/************************************************************************/

/**
 * \brief Fetch row count.
 * 
 * This method is the same as the C function GDALRATGetRowCount().
 *
 * @return the number of rows. 
 */

int GDALRasterAttributeTable::GetRowCount() const

{
    return (int) nRowCount;
}

/************************************************************************/
/*                        GDALRATGetUsageOfCol()                        */
/************************************************************************/

int CPL_STDCALL 
GDALRATGetRowCount( GDALRasterAttributeTableH hRAT )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetRowCount", 0 );

    return ((GDALRasterAttributeTable *) hRAT)->GetRowCount();
}

/************************************************************************/
/*                          GetValueAsString()                          */
/************************************************************************/

/**
 * \brief Fetch field value as a string.
 *
 * The value of the requested column in the requested row is returned
 * as a string.  If the field is numeric, it is formatted as a string
 * using default rules, so some precision may be lost.
 *
 * This method is the same as the C function GDALRATGetValueAsString(), 
 * except it returns a "const char *" result.
 *
 * @param iRow row to fetch (zero based).
 * @param iField column to fetch (zero based).
 * 
 * @return field value
 */

const char *
GDALRasterAttributeTable::GetValueAsString( int iRow, int iField ) const

{
    if( iField < 0 || iField >= (int) aoFields.size() )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iField (%d) out of range.", iField );

        return "";
    }

    if( iRow < 0 || iRow >= nRowCount )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iRow (%d) out of range.", iRow );

        return "";
    }

    switch( aoFields[iField].eType )
    {
      case GFT_Integer:
      {
          ((GDALRasterAttributeTable *) this)->
              osWorkingResult.Printf( "%d", aoFields[iField].anValues[iRow] );
          return osWorkingResult;
      }

      case GFT_Real:
      {
          ((GDALRasterAttributeTable *) this)->
              osWorkingResult.Printf( "%.16g", aoFields[iField].adfValues[iRow]);
          return osWorkingResult;
      }

      case GFT_String:
      {
          return aoFields[iField].aosValues[iRow];
      }
    }

    return "";
}

/************************************************************************/
/*                      GDALRATGetValueAsString()                       */
/************************************************************************/

const char * CPL_STDCALL 
GDALRATGetValueAsString( GDALRasterAttributeTableH hRAT, int iRow, int iField )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetValueAsString", NULL );

    GDALRasterAttributeTable *poRAT = (GDALRasterAttributeTable *) hRAT;

    poRAT->osWorkingResult = poRAT->GetValueAsString( iRow, iField );
    
    return poRAT->osWorkingResult.c_str();
}

/************************************************************************/
/*                           GetValueAsInt()                            */
/************************************************************************/

/**
 * \brief Fetch field value as a integer.
 *
 * The value of the requested column in the requested row is returned
 * as an integer.  Non-integer fields will be converted to integer with
 * the possibility of data loss.
 *
 * This method is the same as the C function GDALRATGetValueAsInt().
 *
 * @param iRow row to fetch (zero based).
 * @param iField column to fetch (zero based).
 * 
 * @return field value
 */

int 
GDALRasterAttributeTable::GetValueAsInt( int iRow, int iField ) const

{
    if( iField < 0 || iField >= (int) aoFields.size() )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iField (%d) out of range.", iField );

        return 0;
    }

    if( iRow < 0 || iRow >= nRowCount )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iRow (%d) out of range.", iRow );

        return 0;
    }

    switch( aoFields[iField].eType )
    {
      case GFT_Integer:
        return aoFields[iField].anValues[iRow];

      case GFT_Real:
        return (int)  aoFields[iField].adfValues[iRow];

      case GFT_String:
        return atoi( aoFields[iField].aosValues[iRow].c_str() );
    }

    return 0;
}

/************************************************************************/
/*                        GDALRATGetValueAsInt()                        */
/************************************************************************/

int CPL_STDCALL 
GDALRATGetValueAsInt( GDALRasterAttributeTableH hRAT, int iRow, int iField )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetValueAsInt", 0 );

    return ((GDALRasterAttributeTable *) hRAT)->GetValueAsInt( iRow, iField );
}

/************************************************************************/
/*                          GetValueAsDouble()                          */
/************************************************************************/

/**
 * \brief Fetch field value as a double.
 *
 * The value of the requested column in the requested row is returned
 * as a double.   Non double fields will be converted to double with
 * the possibility of data loss.
 * 
 * This method is the same as the C function GDALRATGetValueAsDouble().
 *
 * @param iRow row to fetch (zero based).
 * @param iField column to fetch (zero based).
 * 
 * @return field value
 */

double
GDALRasterAttributeTable::GetValueAsDouble( int iRow, int iField ) const

{
    if( iField < 0 || iField >= (int) aoFields.size() )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iField (%d) out of range.", iField );

        return 0;
    }

    if( iRow < 0 || iRow >= nRowCount )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iRow (%d) out of range.", iRow );

        return 0;
    }

    switch( aoFields[iField].eType )
    {
      case GFT_Integer:
        return aoFields[iField].anValues[iRow];

      case GFT_Real:
        return aoFields[iField].adfValues[iRow];

      case GFT_String:
        return atof( aoFields[iField].aosValues[iRow].c_str() );
    }

    return 0;
}

/************************************************************************/
/*                      GDALRATGetValueAsDouble()                       */
/************************************************************************/

double CPL_STDCALL 
GDALRATGetValueAsDouble( GDALRasterAttributeTableH hRAT, int iRow, int iField )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetValueAsDouble", 0 );

    return ((GDALRasterAttributeTable *) hRAT)->GetValueAsDouble(iRow,iField);
}

/************************************************************************/
/*                            SetRowCount()                             */
/************************************************************************/

/**
 * \brief Set row count.
 *
 * Resizes the table to include the indicated number of rows.  Newly created
 * rows will be initialized to their default values - "" for strings, 
 * and zero for numeric fields. 
 *
 * This method is the same as the C function GDALRATSetRowCount().
 *
 * @param nNewCount the new number of rows.
 */

void GDALRasterAttributeTable::SetRowCount( int nNewCount )

{
    if( nNewCount == nRowCount )
        return;

    unsigned int iField;
    for( iField = 0; iField < aoFields.size(); iField++ )
    {
        switch( aoFields[iField].eType )
        {
          case GFT_Integer:
            aoFields[iField].anValues.resize( nNewCount );
            break;

          case GFT_Real:
            aoFields[iField].adfValues.resize( nNewCount );
            break;

          case GFT_String:
            aoFields[iField].aosValues.resize( nNewCount );
            break;
        }
    }

    nRowCount = nNewCount;
}

/************************************************************************/
/*                         GDALRATSetRowCount()                         */
/************************************************************************/

void CPL_STDCALL 
GDALRATSetRowCount( GDALRasterAttributeTableH hRAT, int nNewCount )

{
    VALIDATE_POINTER0( hRAT, "GDALRATSetRowCount" );

    ((GDALRasterAttributeTable *) hRAT)->SetRowCount( nNewCount );
}

/************************************************************************/
/*                              SetValue()                              */
/************************************************************************/

/**
 * \brief Set field value from string.
 *
 * The indicated field (column) on the indicated row is set from the
 * passed value.  The value will be automatically converted for other field
 * types, with a possible loss of precision.
 *
 * This method is the same as the C function GDALRATSetValueAsString().
 *
 * @param iRow row to fetch (zero based).
 * @param iField column to fetch (zero based).
 * @param pszValue the value to assign.
 */

void GDALRasterAttributeTable::SetValue( int iRow, int iField, 
                                         const char *pszValue )

{
    if( iField < 0 || iField >= (int) aoFields.size() )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iField (%d) out of range.", iField );

        return;
    }

    if( iRow == nRowCount )
        SetRowCount( nRowCount+1 );
    
    if( iRow < 0 || iRow >= nRowCount )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iRow (%d) out of range.", iRow );

        return;
    }

    switch( aoFields[iField].eType )
    {
      case GFT_Integer:
        aoFields[iField].anValues[iRow] = atoi(pszValue);
        break;
        
      case GFT_Real:
        aoFields[iField].adfValues[iRow] = atof(pszValue);
        break;
        
      case GFT_String:
        aoFields[iField].aosValues[iRow] = pszValue;
        break;
    }
}

/************************************************************************/
/*                      GDALRATSetValueAsString()                       */
/************************************************************************/

void CPL_STDCALL 
GDALRATSetValueAsString( GDALRasterAttributeTableH hRAT, int iRow, int iField,
                         const char *pszValue )

{
    VALIDATE_POINTER0( hRAT, "GDALRATSetValueAsString" );

    ((GDALRasterAttributeTable *) hRAT)->SetValue( iRow, iField, pszValue );
}

/************************************************************************/
/*                              SetValue()                              */
/************************************************************************/

/**
 * \brief Set field value from integer.
 *
 * The indicated field (column) on the indicated row is set from the
 * passed value.  The value will be automatically converted for other field
 * types, with a possible loss of precision.
 *
 * This method is the same as the C function GDALRATSetValueAsInteger().
 *
 * @param iRow row to fetch (zero based).
 * @param iField column to fetch (zero based).
 * @param nValue the value to assign.
 */

void GDALRasterAttributeTable::SetValue( int iRow, int iField, 
                                         int nValue )

{
    if( iField < 0 || iField >= (int) aoFields.size() )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iField (%d) out of range.", iField );

        return;
    }

    if( iRow == nRowCount )
        SetRowCount( nRowCount+1 );
    
    if( iRow < 0 || iRow >= nRowCount )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iRow (%d) out of range.", iRow );

        return;
    }

    switch( aoFields[iField].eType )
    {
      case GFT_Integer:
        aoFields[iField].anValues[iRow] = nValue;
        break;
        
      case GFT_Real:
        aoFields[iField].adfValues[iRow] = nValue;
        break;
        
      case GFT_String:
      {
          char szValue[100];

          sprintf( szValue, "%d", nValue );
          aoFields[iField].aosValues[iRow] = szValue;
      }
      break;
    }
}

/************************************************************************/
/*                        GDALRATSetValueAsInt()                        */
/************************************************************************/

void CPL_STDCALL 
GDALRATSetValueAsInt( GDALRasterAttributeTableH hRAT, int iRow, int iField,
                      int nValue )

{
    VALIDATE_POINTER0( hRAT, "GDALRATSetValueAsInt" );

    ((GDALRasterAttributeTable *) hRAT)->SetValue( iRow, iField, nValue);
}

/************************************************************************/
/*                              SetValue()                              */
/************************************************************************/

/**
 * \brief Set field value from double.
 *
 * The indicated field (column) on the indicated row is set from the
 * passed value.  The value will be automatically converted for other field
 * types, with a possible loss of precision.
 *
 * This method is the same as the C function GDALRATSetValueAsDouble().
 *
 * @param iRow row to fetch (zero based).
 * @param iField column to fetch (zero based).
 * @param dfValue the value to assign.
 */

void GDALRasterAttributeTable::SetValue( int iRow, int iField, 
                                         double dfValue )

{
    if( iField < 0 || iField >= (int) aoFields.size() )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iField (%d) out of range.", iField );

        return;
    }

    if( iRow == nRowCount )
        SetRowCount( nRowCount+1 );
    
    if( iRow < 0 || iRow >= nRowCount )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "iRow (%d) out of range.", iRow );

        return;
    }

    switch( aoFields[iField].eType )
    {
      case GFT_Integer:
        aoFields[iField].anValues[iRow] = (int) dfValue;
        break;
        
      case GFT_Real:
        aoFields[iField].adfValues[iRow] = dfValue;
        break;
        
      case GFT_String:
      {
          char szValue[100];

          sprintf( szValue, "%.15g", dfValue );
          aoFields[iField].aosValues[iRow] = szValue;
      }
      break;
    }
}

/************************************************************************/
/*                      GDALRATSetValueAsDouble()                       */
/************************************************************************/

void CPL_STDCALL 
GDALRATSetValueAsDouble( GDALRasterAttributeTableH hRAT, int iRow, int iField,
                         double dfValue )

{
    VALIDATE_POINTER0( hRAT, "GDALRATSetValueAsDouble" );

    ((GDALRasterAttributeTable *) hRAT)->SetValue( iRow, iField, dfValue );
}

/************************************************************************/
/*                           GetRowOfValue()                            */
/************************************************************************/

/**
 * \brief Get row for pixel value.
 *
 * Given a raw pixel value, the raster attribute table is scanned to 
 * determine which row in the table applies to the pixel value.  The
 * row index is returned. 
 *
 * This method is the same as the C function GDALRATGetRowOfValue().
 *
 * @param dfValue the pixel value. 
 *
 * @return the row index or -1 if no row is appropriate. 
 */

int GDALRasterAttributeTable::GetRowOfValue( double dfValue ) const

{
/* -------------------------------------------------------------------- */
/*      Handle case of regular binning.                                 */
/* -------------------------------------------------------------------- */
    if( bLinearBinning )
    {
        int iBin = (int) floor((dfValue - dfRow0Min) / dfBinSize);
        if( iBin < 0 || iBin >= nRowCount )
            return -1;
        else
            return iBin;
    }

/* -------------------------------------------------------------------- */
/*      Do we have any information?                                     */
/* -------------------------------------------------------------------- */
    const GDALRasterAttributeField *poMin, *poMax;

    if( !bColumnsAnalysed )
        ((GDALRasterAttributeTable *) this)->AnalyseColumns();

    if( nMinCol == -1 && nMaxCol == -1 )
        return -1;

    if( nMinCol != -1 )
        poMin = &(aoFields[nMinCol]);
    else
        poMin = NULL;
    
    if( nMaxCol != -1 )
        poMax = &(aoFields[nMaxCol]);
    else
        poMax = NULL;
    
/* -------------------------------------------------------------------- */
/*      Search through rows for match.                                  */
/* -------------------------------------------------------------------- */
    int   iRow;

    for( iRow = 0; iRow < nRowCount; iRow++ )
    {
        if( poMin != NULL )
        {
            if( poMin->eType == GFT_Integer )
            {
                while( iRow < nRowCount && dfValue < poMin->anValues[iRow] ) 
                    iRow++;
            }
            else if( poMin->eType == GFT_Real )
            {
                while( iRow < nRowCount && dfValue < poMin->adfValues[iRow] )
                    iRow++;
            }

            if( iRow == nRowCount )
                break;
        }

        if( poMax != NULL )
        {
            if( (poMax->eType == GFT_Integer 
                 && dfValue > poMax->anValues[iRow] ) 
                || (poMax->eType == GFT_Real 
                    && dfValue > poMax->adfValues[iRow] ) )
                continue;
        }

        return iRow;
    }

    return -1;
}

/************************************************************************/
/*                           GetRowOfValue()                            */
/*                                                                      */
/*      Int arg for now just converted to double.  Perhaps we will      */
/*      handle this in a special way some day?                          */
/************************************************************************/

int GDALRasterAttributeTable::GetRowOfValue( int nValue ) const

{
    return GetRowOfValue( (double) nValue );
}

/************************************************************************/
/*                        GDALRATGetRowOfValue()                        */
/************************************************************************/

int CPL_STDCALL 
GDALRATGetRowOfValue( GDALRasterAttributeTableH hRAT, double dfValue )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetRowOfValue", 0 );

    return ((GDALRasterAttributeTable *) hRAT)->GetRowOfValue( dfValue );
}

/************************************************************************/
/*                          SetLinearBinning()                          */
/************************************************************************/

/**
 * \brief Set linear binning information.
 *
 * For RATs with equal sized categories (in pixel value space) that are
 * evenly spaced, this method may be used to associate the linear binning
 * information with the table.
 *
 * This method is the same as the C function GDALRATSetLinearBinning().
 *
 * @param dfRow0MinIn the lower bound (pixel value) of the first category.
 * @param dfBinSizeIn the width of each category (in pixel value units). 
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr GDALRasterAttributeTable::SetLinearBinning( double dfRow0MinIn, 
                                                   double dfBinSizeIn )

{
    bLinearBinning = TRUE;
    dfRow0Min = dfRow0MinIn;
    dfBinSize = dfBinSizeIn;

    return CE_None;
}

/************************************************************************/
/*                      GDALRATSetLinearBinning()                       */
/************************************************************************/

CPLErr CPL_STDCALL 
GDALRATSetLinearBinning( GDALRasterAttributeTableH hRAT, 
                         double dfRow0Min, double dfBinSize )

{
    VALIDATE_POINTER1( hRAT, "GDALRATSetLinearBinning", CE_Failure );

    return ((GDALRasterAttributeTable *) hRAT)->SetLinearBinning(
        dfRow0Min, dfBinSize );
}

/************************************************************************/
/*                          GetLinearBinning()                          */
/************************************************************************/

/**
 * \brief Get linear binning information.
 *
 * Returns linear binning information if any is associated with the RAT.
 *
 * This method is the same as the C function GDALRATGetLinearBinning().
 *
 * @param pdfRow0MinIn (out) the lower bound (pixel value) of the first category.
 * @param pdfBinSizeIn (out) the width of each category (in pixel value units).
 *
 * @return TRUE if linear binning information exists or FALSE if there is none.
 */

int GDALRasterAttributeTable::GetLinearBinning( double *pdfRow0Min,
                                                double *pdfBinSize ) const

{
    if( !bLinearBinning )
        return FALSE;

    *pdfRow0Min = dfRow0Min;
    *pdfBinSize = dfBinSize;

    return TRUE;
}

/************************************************************************/
/*                      GDALRATGetLinearBinning()                       */
/************************************************************************/

int CPL_STDCALL 
GDALRATGetLinearBinning( GDALRasterAttributeTableH hRAT, 
                         double *pdfRow0Min, double *pdfBinSize )

{
    VALIDATE_POINTER1( hRAT, "GDALRATGetLinearBinning", 0 );

    return ((GDALRasterAttributeTable *) hRAT)->GetLinearBinning(
        pdfRow0Min, pdfBinSize );
}

/************************************************************************/
/*                            CreateColumn()                            */
/************************************************************************/

/**
 * \brief Create new column.
 *
 * If the table already has rows, all row values for the new column will
 * be initialized to the default value ("", or zero).  The new column is
 * always created as the last column, can will be column (field) 
 * "GetColumnCount()-1" after CreateColumn() has completed successfully.
 * 
 * This method is the same as the C function GDALRATCreateColumn().
 *
 * @param pszFieldName the name of the field to create.
 * @param eFieldType the field type (integer, double or string).
 * @param eFieldUsage the field usage, GFU_Generic if not known.
 *
 * @return CE_None on success or CE_Failure if something goes wrong.
 */

CPLErr GDALRasterAttributeTable::CreateColumn( const char *pszFieldName, 
                                               GDALRATFieldType eFieldType,
                                               GDALRATFieldUsage eFieldUsage )

{
    int iNewField = aoFields.size();

    aoFields.resize( iNewField+1 );

    aoFields[iNewField].sName = pszFieldName;
    aoFields[iNewField].eType = eFieldType;
    aoFields[iNewField].eUsage = eFieldUsage;

    if( eFieldType == GFT_Integer )
        aoFields[iNewField].anValues.resize( nRowCount );
    else if( eFieldType == GFT_Real )
        aoFields[iNewField].adfValues.resize( nRowCount );
    else if( eFieldType == GFT_String )
        aoFields[iNewField].aosValues.resize( nRowCount );

    return CE_None;
}

/************************************************************************/
/*                        GDALRATCreateColumn()                         */
/************************************************************************/

CPLErr CPL_STDCALL GDALRATCreateColumn( GDALRasterAttributeTableH hRAT, 
                                        const char *pszFieldName, 
                                        GDALRATFieldType eFieldType,
                                        GDALRATFieldUsage eFieldUsage )

{
    VALIDATE_POINTER1( hRAT, "GDALRATCreateColumn", CE_Failure );

    return ((GDALRasterAttributeTable *) hRAT)->CreateColumn( pszFieldName, 
                                                              eFieldType,
                                                              eFieldUsage );
}

/************************************************************************/
/*                      InitializeFromColorTable()                      */
/************************************************************************/

/**
 * \brief Initialize from color table.
 *
 * This method will setup a whole raster attribute table based on the
 * contents of the passed color table.  The Value (GFU_MinMax), 
 * Red (GFU_Red), Green (GFU_Green), Blue (GFU_Blue), and Alpha (GFU_Alpha)
 * fields are created, and a row is set for each entry in the color table. 
 * 
 * The raster attribute table must be empty before calling 
 * InitializeFromColorTable(). 
 *
 * The Value fields are set based on the implicit assumption with color
 * tables that entry 0 applies to pixel value 0, 1 to 1, etc. 
 *
 * This method is the same as the C function GDALRATInitializeFromColorTable().
 *
 * @param poTable the color table to copy from.
 *
 * @param CE_None on success or CE_Failure if something goes wrong.
 */

CPLErr GDALRasterAttributeTable::InitializeFromColorTable( 
    const GDALColorTable *poTable )

{
    int iRow;

    if( GetRowCount() > 0 || GetColumnCount() > 0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Raster Attribute Table not empty in InitializeFromColorTable()" );
        return CE_Failure;
    }

    SetLinearBinning( 0.0, 1.0 );
    CreateColumn( "Value", GFT_Integer, GFU_MinMax );
    CreateColumn( "Red", GFT_Integer, GFU_Red );
    CreateColumn( "Green", GFT_Integer, GFU_Green );
    CreateColumn( "Blue", GFT_Integer, GFU_Blue );
    CreateColumn( "Alpha", GFT_Integer, GFU_Alpha );

    SetRowCount( poTable->GetColorEntryCount() );

    for( iRow = 0; iRow < poTable->GetColorEntryCount(); iRow++ )
    {
        GDALColorEntry sEntry;

        poTable->GetColorEntryAsRGB( iRow, &sEntry );

        SetValue( iRow, 0, iRow );
        SetValue( iRow, 1, sEntry.c1 );
        SetValue( iRow, 2, sEntry.c2 );
        SetValue( iRow, 3, sEntry.c3 );
        SetValue( iRow, 4, sEntry.c4 );
    }

    return CE_None;
}

/************************************************************************/
/*                  GDALRATInitializeFromColorTable()                   */
/************************************************************************/

CPLErr CPL_STDCALL 
GDALRATInitializeFromColorTable( GDALRasterAttributeTableH hRAT,
                                 GDALColorTableH hCT )
                                 

{
    VALIDATE_POINTER1( hRAT, "GDALRATInitializeFromColorTable", CE_Failure );

    return ((GDALRasterAttributeTable *) hRAT)->
        InitializeFromColorTable( (GDALColorTable *) hCT );
}

/************************************************************************/
/*                       TranslateToColorTable()                        */
/************************************************************************/

/**
 * \brief Translate to a color table.
 *
 * This method will attempt to create a corresponding GDALColorTable from
 * this raster attribute table. 
 * 
 * This method is the same as the C function GDALRATTranslateToColorTable().
 *
 * @param nEntryCount The number of entries to produce (0 to nEntryCount-1), or -1 to auto-determine the number of entries.
 *
 * @return the generated color table or NULL on failure.
 */

GDALColorTable *GDALRasterAttributeTable::TranslateToColorTable( 
    int nEntryCount )

{
/* -------------------------------------------------------------------- */
/*      Establish which fields are red, green, blue and alpha.          */
/* -------------------------------------------------------------------- */
    int iRed, iGreen, iBlue, iAlpha;

    iRed = GetColOfUsage( GFU_Red );
    iGreen = GetColOfUsage( GFU_Green );
    iBlue = GetColOfUsage( GFU_Blue );
    iAlpha = GetColOfUsage( GFU_Alpha );

    if( iRed == -1 || iGreen == -1 || iBlue == -1 )
        return NULL;

/* -------------------------------------------------------------------- */
/*      If we aren't given an explicit number of values to scan for,    */
/*      search for the maximum "max" value.                             */
/* -------------------------------------------------------------------- */
    if( nEntryCount == -1 )
    {
        int  iRow;
        int  iMaxCol;

        iMaxCol = GetColOfUsage( GFU_Max );
        if( iMaxCol == -1 )
            iMaxCol = GetColOfUsage( GFU_MinMax );

        if( iMaxCol == -1 || nRowCount == 0 )
            return NULL;
    
        for( iRow = 0; iRow < nRowCount; iRow++ )
            nEntryCount = MAX(nEntryCount,GetValueAsInt(iRow,iMaxCol)+1);

        if( nEntryCount < 0 )
            return NULL;

        // restrict our number of entries to something vaguely sensible
        nEntryCount = MIN(65535,nEntryCount);
    }

/* -------------------------------------------------------------------- */
/*      Assign values to color table.                                   */
/* -------------------------------------------------------------------- */
    GDALColorTable *poCT = new GDALColorTable();
    int iEntry;

    for( iEntry = 0; iEntry < nEntryCount; iEntry++ )
    {
        GDALColorEntry sColor;
        int iRow = GetRowOfValue( iEntry );

        if( iRow == -1 )
        {
            sColor.c1 = sColor.c2 = sColor.c3 = sColor.c4 = 0;
        }
        else
        {
            sColor.c1 = GetValueAsInt( iRow, iRed );
            sColor.c2 = GetValueAsInt( iRow, iGreen );
            sColor.c3 = GetValueAsInt( iRow, iBlue );
            if( iAlpha == -1 )
                sColor.c4 = 255;
            else
                sColor.c4 = GetValueAsInt( iRow, iAlpha );
        }
        
        poCT->SetColorEntry( iEntry, &sColor );
    }

    return poCT;
}

/************************************************************************/
/*                  GDALRATInitializeFromColorTable()                   */
/************************************************************************/

GDALColorTableH CPL_STDCALL 
GDALRATTranslateToColorTable( GDALRasterAttributeTableH hRAT,
                              int nEntryCount )
                                 

{
    VALIDATE_POINTER1( hRAT, "GDALRATTranslateToColorTable", NULL );

    return ((GDALRasterAttributeTable *) hRAT)->
        TranslateToColorTable( nEntryCount );
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr GDALRasterAttributeTable::XMLInit( CPLXMLNode *psTree, 
                                          const char * /*pszVRTPath*/ )

{
    CPLAssert( GetRowCount() == 0 && GetColumnCount() == 0 );
    
/* -------------------------------------------------------------------- */
/*      Linear binning.                                                 */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLValue( psTree, "Row0Min", NULL ) 
        && CPLGetXMLValue( psTree, "BinSize", NULL ) )
    {
        SetLinearBinning( atof(CPLGetXMLValue( psTree, "Row0Min","" )), 
                          atof(CPLGetXMLValue( psTree, "BinSize","" )) );
    }

/* -------------------------------------------------------------------- */
/*      Column definitions                                              */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psChild;

    for( psChild = psTree->psChild; psChild != NULL; psChild = psChild->psNext)
    {
        if( psChild->eType == CXT_Element 
            && EQUAL(psChild->pszValue,"FieldDefn") )
        {
            CreateColumn( 
              CPLGetXMLValue( psChild, "Name", "" ), 
              (GDALRATFieldType) atoi(CPLGetXMLValue( psChild, "Type", "1" )),
              (GDALRATFieldUsage) atoi(CPLGetXMLValue( psChild, "Usage","0")));
        }
    }
    
/* -------------------------------------------------------------------- */
/*      Row data.                                                       */
/* -------------------------------------------------------------------- */
    for( psChild = psTree->psChild; psChild != NULL; psChild = psChild->psNext)
    {
        if( psChild->eType == CXT_Element 
            && EQUAL(psChild->pszValue,"Row") )
        {
            int iRow = atoi(CPLGetXMLValue(psChild,"index","0"));
            int iField = 0;
            CPLXMLNode *psF;

            for( psF = psChild->psChild; psF != NULL; psF = psF->psNext )
            {
                if( psF->eType != CXT_Element || !EQUAL(psF->pszValue,"F") )
                    continue;

                if( psF->psChild != NULL && psF->psChild->eType == CXT_Text )
                    SetValue( iRow, iField++, psF->psChild->pszValue );
                else
                    SetValue( iRow, iField++, "" );
            }
        }
    }

    return CE_None;
}

/************************************************************************/
/*                             Serialize()                              */
/************************************************************************/

CPLXMLNode *GDALRasterAttributeTable::Serialize() const

{
    CPLXMLNode *psTree = NULL;
    CPLXMLNode *psRow = NULL;

    psTree = CPLCreateXMLNode( NULL, CXT_Element, "GDALRasterAttributeTable" );

/* -------------------------------------------------------------------- */
/*      Add attributes with regular binning info if appropriate.        */
/* -------------------------------------------------------------------- */
    char szValue[128];

    if( bLinearBinning )
    {
        sprintf( szValue, "%.16g", dfRow0Min );
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psTree, CXT_Attribute, "Row0Min" ), 
            CXT_Text, szValue );

        sprintf( szValue, "%.16g", dfBinSize );
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psTree, CXT_Attribute, "BinSize" ), 
            CXT_Text, szValue );
    }

/* -------------------------------------------------------------------- */
/*      Define each column.                                             */
/* -------------------------------------------------------------------- */
    int iCol;

    for( iCol = 0; iCol < (int) aoFields.size(); iCol++ )
    {
        CPLXMLNode *psCol;

        psCol = CPLCreateXMLNode( psTree, CXT_Element, "FieldDefn" );
        
        sprintf( szValue, "%d", iCol );
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psCol, CXT_Attribute, "index" ), 
            CXT_Text, szValue );

        CPLCreateXMLElementAndValue( psCol, "Name", 
                                     aoFields[iCol].sName.c_str() );

        sprintf( szValue, "%d", (int) aoFields[iCol].eType );
        CPLCreateXMLElementAndValue( psCol, "Type", szValue );

        sprintf( szValue, "%d", (int) aoFields[iCol].eUsage );
        CPLCreateXMLElementAndValue( psCol, "Usage", szValue );
    }

/* -------------------------------------------------------------------- */
/*      Write out each row.                                             */
/* -------------------------------------------------------------------- */
    int iRow;
    CPLXMLNode *psTail = NULL;

    for( iRow = 0; iRow < nRowCount; iRow++ )
    {
        psRow = CPLCreateXMLNode( NULL, CXT_Element, "Row" );
        if( psTail == NULL )
            CPLAddXMLChild( psTree, psRow );
        else
            psTail->psNext = psRow;
        psTail = psRow;

        sprintf( szValue, "%d", iRow );
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psRow, CXT_Attribute, "index" ), 
            CXT_Text, szValue );

        for( iCol = 0; iCol < (int) aoFields.size(); iCol++ )
        {
            const char *pszValue = szValue;

            if( aoFields[iCol].eType == GFT_Integer )
                sprintf( szValue, "%d", aoFields[iCol].anValues[iRow] );
            else if( aoFields[iCol].eType == GFT_Real )
                sprintf( szValue, "%.16g", aoFields[iCol].adfValues[iRow] );
            else
                pszValue = aoFields[iCol].aosValues[iRow].c_str();

            CPLCreateXMLElementAndValue( psRow, "F", pszValue );
        }
    }

    return psTree;
}

/************************************************************************/
/*                            DumpReadable()                            */
/************************************************************************/

/**
 * \brief Dump RAT in readable form.
 *
 * Currently the readable form is the XML encoding ... only barely 
 * readable. 
 *
 * This method is the same as the C function GDALRATDumpReadable().
 *
 * @param fp file to dump to or NULL for stdout. 
 */

void GDALRasterAttributeTable::DumpReadable( FILE * fp )

{
    CPLXMLNode *psTree = Serialize();
    char *pszXMLText = CPLSerializeXMLTree( psTree );

    CPLDestroyXMLNode( psTree );

    if( fp == NULL )
        fp = stdout;

    fprintf( fp, "%s\n", pszXMLText );

    CPLFree( pszXMLText );
}

/************************************************************************/
/*                        GDALRATDumpReadable()                         */
/************************************************************************/

void CPL_STDCALL 
GDALRATDumpReadable( GDALRasterAttributeTableH hRAT, FILE *fp )

{
    VALIDATE_POINTER0( hRAT, "GDALRATDumpReadable" );

    ((GDALRasterAttributeTable *) hRAT)->DumpReadable( fp );
}

/************************************************************************/
/*                               Clone()                                */
/************************************************************************/

/**
 * \brief Copy Raster Attribute Table
 *
 * Creates a new copy of an existing raster attribute table.  The new copy
 * becomes the responsibility of the caller to destroy.
 *
 * This method is the same as the C function GDALRATClone().
 *
 * @return new copy of the RAT. 
 */

GDALRasterAttributeTable *GDALRasterAttributeTable::Clone() const

{
    return new GDALRasterAttributeTable( *this );
}

/************************************************************************/
/*                            GDALRATClone()                            */
/************************************************************************/

GDALRasterAttributeTableH CPL_STDCALL 
GDALRATClone( GDALRasterAttributeTableH hRAT )

{
    VALIDATE_POINTER1( hRAT, "GDALRATClone", NULL );

    return ((GDALRasterAttributeTable *) hRAT)->Clone();
}
