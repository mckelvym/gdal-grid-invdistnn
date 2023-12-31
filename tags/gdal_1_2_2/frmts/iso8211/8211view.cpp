/* ****************************************************************************
 * $Id$
 *
 * Project:  SDTS Translator
 * Purpose:  Example program dumping data in 8211 data to stdout.
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
 * Revision 1.7  2003/08/21 21:22:46  warmerda
 * Special reporting facilities for the S-57 LNAM and NAME bitfields (provided
 * by Rodney Jensen).
 * Report DDFInt fields as unsigned if they come from an unsigned binary
 * field.
 *
 * Revision 1.6  2001/07/18 04:51:57  warmerda
 * added CPL_CVSID
 *
 * Revision 1.5  2000/06/16 18:02:08  warmerda
 * added SetRepeatingFlag hack support
 *
 * Revision 1.4  1999/11/18 19:03:04  warmerda
 * expanded tabs
 *
 * Revision 1.3  1999/05/06 14:25:15  warmerda
 * added support for binary strings
 *
 * Revision 1.2  1999/04/28 05:16:47  warmerda
 * added usage
 *
 * Revision 1.1  1999/04/27 22:09:30  warmerda
 * New
 *
 */

#include <stdio.h>
#include "iso8211.h"

CPL_CVSID("$Id$");

static void ViewRecordField( DDFField * poField );
static int ViewSubfield( DDFSubfieldDefn *poSFDefn,
                         const char * pachFieldData,
                         int nBytesRemaining );

/* **********************************************************************/
/*                                main()                                */
/* **********************************************************************/

int main( int nArgc, char ** papszArgv )

{
    DDFModule   oModule;
    const char  *pszFilename = NULL;
    int         bFSPTHack = FALSE;

    for( int iArg = 1; iArg < nArgc; iArg++ )
    {
        if( EQUAL(papszArgv[iArg],"-fspt_repeating") )
            bFSPTHack = TRUE;
        else
            pszFilename = papszArgv[iArg];
    }

    if( pszFilename == NULL )
    {
        printf( "Usage: 8211view filename\n" );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the file.  Note that by default errors are reported to     */
/*      stderr, so we don't bother doing it ourselves.                  */
/* -------------------------------------------------------------------- */
    if( !oModule.Open( pszFilename ) )
    {
        exit( 1 );
    }

    if( bFSPTHack )
    {
        DDFFieldDefn *poFSPT = oModule.FindFieldDefn( "FSPT" );

        if( poFSPT == NULL )
            fprintf( stderr,
                     "unable to find FSPT field to set repeating flag.\n" );
        else
            poFSPT->SetRepeatingFlag( TRUE );
    }

/* -------------------------------------------------------------------- */
/*      Loop reading records till there are none left.                  */
/* -------------------------------------------------------------------- */
    DDFRecord   *poRecord;
    int         iRecord = 0;

    while( (poRecord = oModule.ReadRecord()) != NULL )
    {
        printf( "Record %d (%d bytes)\n",
                ++iRecord, poRecord->GetDataSize() );

        /* ------------------------------------------------------------ */
        /*      Loop over each field in this particular record.         */
        /* ------------------------------------------------------------ */
        for( int iField = 0; iField < poRecord->GetFieldCount(); iField++ )
        {
            DDFField    *poField = poRecord->GetField( iField );

            ViewRecordField( poField );
        }
    }
}

/* **********************************************************************/
/*                          ViewRecordField()                           */
/*                                                                      */
/*      Dump the contents of a field instance in a record.              */
/* **********************************************************************/

static void ViewRecordField( DDFField * poField )

{
    int         nBytesRemaining;
    const char  *pachFieldData;
    DDFFieldDefn *poFieldDefn = poField->GetFieldDefn();

    // Report general information about the field.
    printf( "    Field %s: %s\n",
            poFieldDefn->GetName(), poFieldDefn->GetDescription() );

    // Get pointer to this fields raw data.  We will move through
    // it consuming data as we report subfield values.

    pachFieldData = poField->GetData();
    nBytesRemaining = poField->GetDataSize();

    /* -------------------------------------------------------- */
    /*      Loop over the repeat count for this fields          */
    /*      subfields.  The repeat count will almost            */
    /*      always be one.                                      */
    /* -------------------------------------------------------- */
    int         iRepeat;

    for( iRepeat = 0; iRepeat < poField->GetRepeatCount(); iRepeat++ )
    {

        /* -------------------------------------------------------- */
        /*   Loop over all the subfields of this field, advancing   */
        /*   the data pointer as we consume data.                   */
        /* -------------------------------------------------------- */
        int     iSF;

        for( iSF = 0; iSF < poFieldDefn->GetSubfieldCount(); iSF++ )
        {
            DDFSubfieldDefn *poSFDefn = poFieldDefn->GetSubfield( iSF );
            int         nBytesConsumed;

            nBytesConsumed = ViewSubfield( poSFDefn, pachFieldData,
                                           nBytesRemaining );

            nBytesRemaining -= nBytesConsumed;
            pachFieldData += nBytesConsumed;
        }
    }
}

/* **********************************************************************/
/*                            ViewSubfield()                            */
/* **********************************************************************/

static int ViewSubfield( DDFSubfieldDefn *poSFDefn,
                         const char * pachFieldData,
                         int nBytesRemaining )

{
    int         nBytesConsumed = 0;

    switch( poSFDefn->GetType() )
    {
      case DDFInt:
        if( poSFDefn->GetBinaryFormat() == DDFSubfieldDefn::UInt )
            printf( "        %s = %u\n",
                    poSFDefn->GetName(),
                    poSFDefn->ExtractIntData( pachFieldData, nBytesRemaining,
                                              &nBytesConsumed ) );
        else
            printf( "        %s = %d\n",
                    poSFDefn->GetName(),
                    poSFDefn->ExtractIntData( pachFieldData, nBytesRemaining,
                                              &nBytesConsumed ) );
        break;

      case DDFFloat:
        printf( "        %s = %f\n",
                poSFDefn->GetName(),
                poSFDefn->ExtractFloatData( pachFieldData, nBytesRemaining,
                                            &nBytesConsumed ) );
        break;

      case DDFString:
        printf( "        %s = `%s'\n",
                poSFDefn->GetName(),
                poSFDefn->ExtractStringData( pachFieldData, nBytesRemaining,
                                             &nBytesConsumed ) );
        break;

      case DDFBinaryString:
      {
          int   i;
          //rjensen 19-Feb-2002 5 integer variables to decode NAME and LNAM
          int vrid_rcnm=0;
          int vrid_rcid=0;
          int foid_agen=0;
          int foid_find=0;
          int foid_fids=0;

          GByte *pabyBString = (GByte *)
              poSFDefn->ExtractStringData( pachFieldData, nBytesRemaining,
                                           &nBytesConsumed );

          printf( "        %s = 0x", poSFDefn->GetName() );
          for( i = 0; i < MIN(nBytesConsumed,24); i++ )
              printf( "%02X", pabyBString[i] );

          if( nBytesConsumed > 24 )
              printf( "%s", "..." );

          // rjensen 19-Feb-2002 S57 quick hack. decode NAME and LNAM bitfields
          if ( EQUAL(poSFDefn->GetName(),"NAME") )
          {
              vrid_rcnm=pabyBString[0];
              vrid_rcid=pabyBString[1] + (pabyBString[2]*256)+
                  (pabyBString[3]*65536)+ (pabyBString[4]*16777216);
              printf("\tVRID RCNM = %d,RCID = %u",vrid_rcnm,vrid_rcid);
          }
          else if ( EQUAL(poSFDefn->GetName(),"LNAM") )
          {
              foid_agen=pabyBString[0] + (pabyBString[1]*256);
              foid_find=pabyBString[2] + (pabyBString[3]*256)+
                  (pabyBString[4]*65536)+ (pabyBString[5]*16777216);
              foid_fids=pabyBString[6] + (pabyBString[7]*256);
              printf("\tFOID AGEN = %u,FIDN = %u,FIDS = %u",
                     foid_agen,foid_find,foid_fids);
          }

          printf( "\n" );
      }
      break;

    }

    return nBytesConsumed;
}
