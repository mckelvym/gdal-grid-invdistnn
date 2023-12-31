/******************************************************************************
 * $Id$
 *
 * Project:  ISO 8211 Access
 * Purpose:  Main declarations for ISO 8211.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * Revision 1.14  2001/08/29 17:47:33  warmerda
 * added GetInstanceData
 *
 * Revision 1.13  2001/08/24 19:41:19  warmerda
 * fixed cloning problems
 *
 * Revision 1.12  2001/08/24 16:30:55  warmerda
 * added DDFRecord update in place methods for S57 updating
 *
 * Revision 1.11  2000/09/19 14:08:51  warmerda
 * keep and report _extendedCharSet
 *
 * Revision 1.10  2000/06/16 18:02:08  warmerda
 * added SetRepeatingFlag hack support
 *
 * Revision 1.9  2000/01/31 18:03:39  warmerda
 * completely rewrote format expansion to make more general
 *
 * Revision 1.8  1999/11/18 19:03:04  warmerda
 * expanded tabs
 *
 * Revision 1.7  1999/11/18 19:02:38  warmerda
 * added failquietly to open
 *
 * Revision 1.6  1999/09/20 19:29:30  warmerda
 * make forgiving of UNIT/FIELD terminator mixup in Tiger SDTS files
 *
 * Revision 1.5  1999/08/13 03:26:29  warmerda
 * added Rewind()
 *
 * Revision 1.4  1999/05/07 14:11:22  warmerda
 * added subfield value fetches on record, and other odds and ends.
 *
 * Revision 1.3  1999/05/06 14:23:32  warmerda
 * added DDFBinaryString
 *
 * Revision 1.2  1999/04/27 22:09:50  warmerda
 * updated docs
 *
 * Revision 1.1  1999/04/27 18:45:09  warmerda
 * New
 *
 */

#ifndef _ISO8211_H_INCLUDED
#define _ISO8211_H_INCLUDED

#include "cpl_port.h"

/**
  General data type
    */
typedef enum {
    DDFInt,
    DDFFloat,
    DDFString,
    DDFBinaryString
} DDFDataType;
  
/************************************************************************/
/*      These should really be private to the library ... they are      */
/*      mostly conveniences.                                            */
/************************************************************************/

long DDFScanInt( const char *pszString, int nMaxChars );
int  DDFScanVariable( const char * pszString, int nMaxChars, int nDelimChar );
char *DDFFetchVariable( const char *pszString, int nMaxChars,
                        int nDelimChar1, int nDelimChar2,
                        int *pnConsumedChars );

#define DDF_FIELD_TERMINATOR    30
#define DDF_UNIT_TERMINATOR     31

/************************************************************************/
/*                           Predeclarations                            */
/************************************************************************/

class DDFFieldDefn;
class DDFSubfieldDefn;
class DDFRecord;
class DDFField;

/************************************************************************/
/*                              DDFModule                               */
/************************************************************************/

/**
  The primary class for reading ISO 8211 files.  This class contains all
  the information read from the DDR record, and is used to read records
  from the file.           

*/  

class DDFModule
{
  public:
                DDFModule();
                ~DDFModule();
                
    int         Open( const char * pszFilename, int bFailQuietly = FALSE );
    void        Close();

    void        Dump( FILE * fp );

    DDFRecord   *ReadRecord( void );
    void        Rewind( long nOffset = -1 );

    DDFFieldDefn *FindFieldDefn( const char * );

    /** Fetch the number of defined fields. */

    int         GetFieldCount() { return nFieldDefnCount; }
    DDFFieldDefn *GetField(int);
    
    // This is really just for internal use.
    int         GetFieldControlLength() { return _fieldControlLength; }
    void        AddCloneRecord( DDFRecord * );
    void        RemoveCloneRecord( DDFRecord * );
    
    // This is just for DDFRecord.
    FILE        *GetFP() { return fpDDF; }
    
  private:
    FILE        *fpDDF;
    long        nFirstRecordOffset;

    char        _interchangeLevel;
    char        _inlineCodeExtensionIndicator;
    char        _versionNumber;
    char        _appIndicator;
    int         _fieldControlLength;
    char        _extendedCharSet[4];

    long _recLength;
    char _leaderIden;
    long _fieldAreaStart;
    long _sizeFieldLength;
    long _sizeFieldPos;
    long _sizeFieldTag;

    // One DirEntry per field.  
    int         nFieldDefnCount;
    DDFFieldDefn *paoFieldDefns;

    DDFRecord   *poRecord;

    int         nCloneCount;
    int         nMaxCloneCount;
    DDFRecord   **papoClones;
};

/************************************************************************/
/*                             DDFFieldDefn                             */
/************************************************************************/

/**
 * Information from the DDR defining one field.  Note that just because
 * a field is defined for a DDFModule doesn't mean that it actually occurs
 * on any records in the module.  DDFFieldDefns are normally just significant
 * as containers of the DDFSubfieldDefns.
 */

class DDFFieldDefn
{
  public:
                DDFFieldDefn();
                ~DDFFieldDefn();

    int         Initialize( DDFModule * poModule, const char *pszTag,
                            int nSize, const char * pachRecord );
    
    void        Dump( FILE * fp );

    /** Fetch a pointer to the field name (tag).
     * @return this is an internal copy and shouldn't be freed.
     */
    const char  *GetName() { return pszTag; }

    /** Fetch a longer descriptio of this field.
     * @return this is an internal copy and shouldn't be freed.
     */
    const char  *GetDescription() { return _fieldName; }

    /** Get the number of subfields. */
    int         GetSubfieldCount() { return nSubfieldCount; }
    
    DDFSubfieldDefn *GetSubfield( int i );
    DDFSubfieldDefn *FindSubfieldDefn( const char * );

    /**
     * Get the width of this field.  This function isn't normally used
     * by applications.
     *
     * @return The width of the field in bytes, or zero if the field is not
     * apparently of a fixed width.
     */
    int         GetFixedWidth() { return nFixedWidth; }

    /**
     * Fetch repeating flag.
     * @see DDFField::GetRepeatCount()
     * @return TRUE if the field is marked as repeating.
     */
    int         IsRepeating() { return bRepeatingSubfields; }

  typedef enum { elementary, vector, array, concatenated } data_struct_code;
  typedef enum { char_string, 
                 implicit_point, 
                 explicit_point, 
                 explicit_point_scaled, 
                 char_bit_string, 
                 bit_string, 
                 mixed_data_type } data_type_code;

    static char       *ExpandFormat( const char * );

    /** this is just for an S-57 hack for swedish data */
    void SetRepeatingFlag( int n ) { bRepeatingSubfields = n; }
    
  private:

    static char       *ExtractSubstring( const char * );

    DDFModule * poModule;
    char *      pszTag;

    char *      _fieldName;
    char *      _arrayDescr;
    char *      _formatControls;

    int         bRepeatingSubfields;
    int         nFixedWidth;    // zero if variable. 

    int         BuildSubfields();
    int         ApplyFormats();

    DDFFieldDefn::data_struct_code _data_struct_code;

    DDFFieldDefn::data_type_code   _data_type_code;

    int         nSubfieldCount;
    DDFSubfieldDefn *paoSubfields;
};

/************************************************************************/
/*                           DDFSubfieldDefn                            */
/*                                                                      */
/*      Information from the DDR record for one subfield of a           */
/*      particular field.                                               */
/************************************************************************/

/**
 * Information from the DDR record describing one subfield of a DDFFieldDefn.
 * All subfields of a field will occur in each occurance of that field
 * (as a DDFField) in a DDFRecord.  Subfield's actually contain formatted
 * data (as instances within a record).
 */

class DDFSubfieldDefn
{
public:

                DDFSubfieldDefn();
                ~DDFSubfieldDefn();

    void        SetName( const char * pszName );

    /** Get pointer to subfield name. */
    const char  *GetName() { return pszName; }
    
    /** Get pointer to subfield format string */
    const char  *GetFormat() { return pszFormatString; }
    int         SetFormat( const char * pszFormat );

    /**
     * Get the general type of the subfield.  This can be used to
     * determine which of ExtractFloatData(), ExtractIntData() or
     * ExtractStringData() should be used.
     * @return The subfield type.  One of DDFInt, DDFFloat, DDFString or
     * DDFBinaryString.
     */
      
    DDFDataType GetType() { return eType; }

    double      ExtractFloatData( const char *pachData, int nMaxBytes,
                                  int * pnConsumedBytes );
    int         ExtractIntData( const char *pachData, int nMaxBytes,
                                int * pnConsumedBytes );
    const char  *ExtractStringData( const char *pachData, int nMaxBytes,
                                    int * pnConsumedBytes );
    int         GetDataLength( const char *, int, int * );
    void        DumpData( const char *pachData, int nMaxBytes, FILE * fp );

    /** Get the subfield width (zero for variable). */
    int         GetWidth() { return nFormatWidth; } // zero for variable.
    
    void        Dump( FILE * fp );

private:

/**
  Binary format: this is the digit immediately following the B or b for
  binary formats. 
  */
typedef enum {
    NotBinary=0,
    UInt=1,
    SInt=2,
    FPReal=3,
    FloatReal=4,
    FloatComplex=5
} DDFBinaryFormat;

  char      *pszName;   // a.k.a. subfield mnemonic
  char      *pszFormatString; 

  DDFDataType           eType;
  DDFBinaryFormat       eBinaryFormat;

/* -------------------------------------------------------------------- */
/*      bIsVariable determines whether we using the                     */
/*      chFormatDelimeter (TRUE), or the fixed width (FALSE).           */
/* -------------------------------------------------------------------- */
  int        bIsVariable;
  
  char       chFormatDelimeter;
  int        nFormatWidth;

/* -------------------------------------------------------------------- */
/*      Fetched string cache.  This is where we hold the values         */
/*      returned from ExtractStringData().                              */
/* -------------------------------------------------------------------- */
  int        nMaxBufChars;
  char       *pachBuffer;
};

/************************************************************************/
/*                              DDFRecord                               */
/*                                                                      */
/*      Class that contains one DR record from a file.  We read into    */
/*      the same record object repeatedly to ensure that repeated       */
/*      leaders can be easily preserved.                                */
/************************************************************************/

/**
 * Contains instance data from one data record (DR).  The data is contained
 * as a list of DDFField instances partitioning the raw data into fields.
 */

class DDFRecord
{
  public:
                DDFRecord( DDFModule * );
                ~DDFRecord();

    DDFRecord  *Clone();
    DDFRecord  *CloneOn( DDFModule * );
    
    void        Dump( FILE * );

    /** Get the number of DDFFields on this record. */
    int         GetFieldCount() { return nFieldCount; }

    DDFField    *FindField( const char *, int = 0 );
    DDFField    *GetField( int );

    int         GetIntSubfield( const char *, int, const char *, int,
                                int * = NULL );
    double      GetFloatSubfield( const char *, int, const char *, int,
                                  int * = NULL );
    const char *GetStringSubfield( const char *, int, const char *, int,
                                   int * = NULL );

    /** Fetch size of records raw data (GetData()) in bytes. */
    int         GetDataSize() { return nDataSize; }

    /**
     * Fetch the raw data for this record.  The returned pointer is effectively
     * to the data for the first field of the record, and is of size 
     * GetDataSize().
     */
    const char  *GetData() { return pachData; }

    /**
     * Fetch the DDFModule with which this record is associated.
     */

    DDFModule * GetModule() { return poModule; }

    int ResizeField( DDFField *poField, int nNewDataSize );
    int DeleteField( DDFField *poField );
    DDFField* AddField( DDFFieldDefn * );

    int SetFieldRaw( DDFField *poField, int iIndexWithinField, 
                     const char *pachRawData, int nRawDataSize );

    // This is really just for the DDFModule class.
    int         Read();
    void        Clear();
    
  private:

    int         ReadHeader();
    
    DDFModule   *poModule;

    int         nReuseHeader;   

    int         nFieldOffset;   // field data area, not dir entries.

    int         nDataSize;      // Whole record except leader with header
    char        *pachData;

    int         nFieldCount;
    DDFField    *paoFields;

    int         bIsClone;
};

/************************************************************************/
/*                               DDFField                               */
/*                                                                      */
/*      This object represents one field in a DDFRecord.                */
/************************************************************************/

/**
 * This object represents one field in a DDFRecord.  This
 * models an instance of the fields data, rather than it's data definition
 * which is handled by the DDFFieldDefn class.  Note that a DDFField
 * doesn't have DDFSubfield children as you would expect.  To extract
 * subfield values use GetSubfieldData() to find the right data pointer and
 * then use ExtractIntData(), ExtractFloatData() or ExtractStringData().
 */

class DDFField
{
  public:
    void                Initialize( DDFFieldDefn *, const char *pszData,
                                    int nSize );

    void                Dump( FILE * fp );

    const char         *GetSubfieldData( DDFSubfieldDefn *,
                                         int * = NULL, int = 0 );

    const char         *GetInstanceData( int nInstance, int *pnSize );

    /**
     * Return the pointer to the entire data block for this record. This
     * is an internal copy, and shouldn't be freed by the application.
     */
    const char         *GetData() { return pachData; }

    /** Return the number of bytes in the data block returned by GetData(). */
    int                 GetDataSize() { return nDataSize; }

    int                 GetRepeatCount();

    /** Fetch the corresponding DDFFieldDefn. */
    DDFFieldDefn        *GetFieldDefn() { return poDefn; }
    
  private:
    DDFFieldDefn        *poDefn;

    int                 nDataSize;

    const char          *pachData;
};


#endif /* ndef _ISO8211_H_INCLUDED */



