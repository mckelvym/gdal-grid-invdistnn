/******************************************************************************
 * $Id$
 *
 * Project:  TIGER/Line Translator
 * Purpose:  Implements TigerOverUnder, providing access to .RTU files.
 * Author:   Mark Phillips, mbp@geomtech.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam, Mark Phillips
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
 * Revision 1.2  2003/01/11 15:29:55  warmerda
 * expanded tabs
 *
 * Revision 1.1  2002/12/26 00:20:19  mbp
 * re-organized code to hold TIGER-version details in TigerRecordInfo structs;
 * first round implementation of TIGER_2002 support
 *
 */

#include "ogr_tiger.h"
#include "cpl_conv.h"

CPL_CVSID("$Id$");

#define FILE_CODE       "U"

static TigerFieldInfo rtU_fields[] = {
  // fieldname    fmt  type OFTType      beg  end  len  bDefine bSet bWrite
  { "MODULE",     ' ', ' ', OFTString,     0,   0,   8,       1,   0,     0 },
  { "FILE",       'L', 'N', OFTInteger,    6,  10,   5,       1,   1,     1 },
  { "TZID",       'R', 'N', OFTInteger,   11,  20,  10,       1,   1,     1 },
  { "RTSQ",       'R', 'N', OFTInteger,   21,  21,   1,       1,   1,     1 },
  { "TLIDOV1",    'R', 'N', OFTInteger,   22,  31,  10,       1,   1,     1 },
  { "TLIDOV2",    'R', 'N', OFTInteger,   32,  41,  10,       1,   1,     1 },
  { "TLIDUN1",    'R', 'N', OFTInteger,   42,  51,  10,       1,   1,     1 },
  { "TLIDUN2",    'R', 'N', OFTInteger,   52,  61,  10,       1,   1,     1 },
  { "FRLONG",     'R', 'N', OFTInteger,   62,  71,  10,       1,   1,     1 },
  { "FRLAT",      'R', 'N', OFTInteger,   72,  80,   9,       1,   1,     1 },
};
static TigerRecordInfo rtU_info =
  {
    rtU_fields,
    sizeof(rtU_fields) / sizeof(TigerFieldInfo),
    80
  };


/************************************************************************/
/*                           TigerOverUnder()                           */
/************************************************************************/

TigerOverUnder::TigerOverUnder( OGRTigerDataSource * poDSIn,
                              const char * pszPrototypeModule )
  : TigerPoint(TRUE)
{
    OGRFieldDefn        oField("",OFTInteger);

    poDS = poDSIn;
    poFeatureDefn = new OGRFeatureDefn( "OverUnder" );
    poFeatureDefn->SetGeomType( wkbNone );

    psRTUInfo = &rtU_info;

    AddFieldDefns( psRTUInfo, poFeatureDefn );

}

/************************************************************************/
TigerOverUnder::~TigerOverUnder()
{}

int TigerOverUnder::SetModule( const char * pszModule )
{
  return TigerPoint::SetModule( pszModule, FILE_CODE );
}

OGRFeature *TigerOverUnder::GetFeature( int nRecordId )
{
  return TigerPoint::GetFeature( nRecordId,
                                 psRTUInfo,
                                 62, 71,
                                 72, 80 );
}

OGRErr TigerOverUnder::CreateFeature( OGRFeature *poFeature )
{
  return TigerPoint::CreateFeature( poFeature, 
                                    psRTUInfo,
                                    62,
                                    FILE_CODE );
}
