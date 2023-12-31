/******************************************************************************
 * $Id$
 *
 * Project:  TIGER/Line Translator
 * Purpose:  Implements OGRTigerDataSource class
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.22  2003/03/20 19:10:37  warmerda
 * fixed memory leak
 *
 * Revision 1.21  2003/02/27 16:02:46  warmerda
 * Handle case with filename without path properly in BuildFilename().
 *
 * Revision 1.20  2003/01/13 17:06:10  warmerda
 * added support for a single county as a dataset
 *
 * Revision 1.19  2003/01/11 15:29:55  warmerda
 * expanded tabs
 *
 * Revision 1.18  2003/01/04 23:21:56  mbp
 * Minor bug fixes and field definition changes.  Cleaned
 * up and commented code written for TIGER 2002 support.
 *
 * Revision 1.17  2002/12/28 05:18:59  warmerda
 * Loosen candidate file constraints in Open() to include TST* files.
 *
 * Revision 1.16  2002/12/26 00:20:19  mbp
 * re-organized code to hold TIGER-version details in TigerRecordInfo structs;
 * first round implementation of TIGER_2002 support
 *
 * Revision 1.15  2001/12/12 17:25:07  warmerda
 * Use CPLStat instead of VSIStat.
 *
 * Revision 1.14  2001/07/19 16:58:39  warmerda
 * ensure version initialized, even if not testopen
 *
 * Revision 1.13  2001/07/19 16:05:49  warmerda
 * clear out tabs
 *
 * Revision 1.12  2001/07/19 16:03:11  warmerda
 * allow VERSION override on write
 *
 * Revision 1.11  2001/07/19 13:26:32  warmerda
 * enable override of existing modules
 *
 * Revision 1.10  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.9  2001/07/04 23:25:32  warmerda
 * first round implementation of writer
 *
 * Revision 1.8  2001/07/04 05:40:35  warmerda
 * upgraded to support FILE, and Tiger2000 schema
 *
 * Revision 1.7  2001/01/19 21:15:20  warmerda
 * expanded tabs
 *
 * Revision 1.6  2000/01/13 16:39:17  warmerda
 * Use CPLGetBasename() to compare to limitedFileList.
 *
 * Revision 1.5  2000/01/13 05:18:11  warmerda
 * added support for multiple versions
 *
 * Revision 1.4  1999/12/22 15:38:15  warmerda
 * major update
 *
 * Revision 1.3  1999/12/15 19:59:52  warmerda
 * added new file types
 *
 * Revision 1.2  1999/11/04 21:14:31  warmerda
 * various improvements, and TestCapability()
 *
 * Revision 1.1  1999/10/07 18:19:21  warmerda
 * New
 *
 * Revision 1.6  1999/10/04 03:08:52  warmerda
 * added raster support
 *
 * Revision 1.5  1999/10/01 14:47:51  warmerda
 * major upgrade: generic, string feature codes, etc
 *
 * Revision 1.4  1999/09/29 16:43:43  warmerda
 * added spatial ref, improved test open for non-os files
 *
 * Revision 1.3  1999/09/08 00:58:40  warmerda
 * Added limiting list of files for FME.
 *
 * Revision 1.2  1999/08/30 16:49:26  warmerda
 * added feature class layer support
 *
 * Revision 1.1  1999/08/28 03:13:35  warmerda
 * New
 */

#include "ogr_tiger.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include <ctype.h>

CPL_CVSID("$Id$");

/************************************************************************/
/*                        TigerClassifyVersion()                        */
/************************************************************************/

TigerVersion TigerClassifyVersion( int nVersionCode )

{
    TigerVersion        nVersion;
    int                 nYear, nMonth;

/*
** TIGER Versions
**
** 0000           TIGER/Line Precensus Files, 1990 
** 0002           TIGER/Line Initial Voting District Codes Files, 1990 
** 0003           TIGER/Line Files, 1990 
** 0005           TIGER/Line Files, 1992 
** 0021           TIGER/Line Files, 1994 
** 0024           TIGER/Line Files, 1995 
** 9706 to 9810   TIGER/Line Files, 1997 
** 9812 to 9904   TIGER/Line Files, 1998 
** 0006 to 0008   TIGER/Line Files, 1999 
** 0010 to 0011   TIGER/Line Files, Redistricting Census 2000
** 0103 to 0108   TIGER/Line Files, Census 2000
**
** 0203 to 0205   TIGER/Line Files, UA 2000
** ????    ????
**
** 0206 & higher  TIGER/Line Files, 2002
** ????
*/

    nVersion = TIGER_Unknown;
    if( nVersionCode == 0 )
        nVersion = TIGER_1990_Precensus;
    else if( nVersionCode == 2 )
        nVersion = TIGER_1990;
    else if( nVersionCode == 3 )
        nVersion = TIGER_1992;
    else if( nVersionCode == 5 )
        nVersion = TIGER_1994;
    else if( nVersionCode == 21 )
        nVersion = TIGER_1994;
    else if( nVersionCode == 24 )
        nVersion = TIGER_1995;

    nYear = nVersionCode % 100;
    nMonth = nVersionCode / 100;

    nVersionCode = nYear * 100 + nMonth;

    if( nVersion != TIGER_Unknown )
        /* do nothing */;
    else if( nVersionCode >= 9706 && nVersionCode <= 9810 )
        nVersion = TIGER_1997;
    else if( nVersionCode >= 9812 && nVersionCode <= 9904 )
        nVersion = TIGER_1998;
    else if( nVersionCode >=    6 /*0006*/ && nVersionCode <=    8 /*0008*/ )
        nVersion = TIGER_1999;
    else if( nVersionCode >=   10 /*0010*/ && nVersionCode <=   11 /*0011*/ )
        nVersion = TIGER_2000_Redistricting;
    else if( nVersionCode >=  103 /*0103*/ && nVersionCode <=  108 /*0108*/ )
        nVersion = TIGER_2000_Census;
    else if( nVersionCode >=  203 /*0203*/ && nVersionCode <=  205 /*0205*/ )
        nVersion = TIGER_UA2000;
    else if( nVersionCode >=  206 /*0206*/ )
        nVersion = TIGER_2002;

    return nVersion;
}

/************************************************************************/
/*                         TigerVersionString()                         */
/************************************************************************/

char * TigerVersionString( TigerVersion nVersion )
{

  if (nVersion == TIGER_1990_Precensus) { return "TIGER_1990_Precensus"; }
  if (nVersion == TIGER_1990) { return "TIGER_1990"; }
  if (nVersion == TIGER_1992) { return "TIGER_1992"; }
  if (nVersion == TIGER_1994) { return "TIGER_1994"; }
  if (nVersion == TIGER_1995) { return "TIGER_1995"; }
  if (nVersion == TIGER_1997) { return "TIGER_1997"; }
  if (nVersion == TIGER_1998) { return "TIGER_1998"; }
  if (nVersion == TIGER_1999) { return "TIGER_1999"; }
  if (nVersion == TIGER_2000_Redistricting) { return "TIGER_2000_Redistricting"; }
  if (nVersion == TIGER_UA2000) { return "TIGER_UA2000"; }
  if (nVersion == TIGER_2002) { return "TIGER_2002"; }
  if (nVersion == TIGER_Unknown) { return "TIGER_Unknown"; }
  return "???";
}

/************************************************************************/
/*                         OGRTigerDataSource()                         */
/************************************************************************/

OGRTigerDataSource::OGRTigerDataSource()

{
    bWriteMode = FALSE;

    nLayers = 0;
    papoLayers = NULL;

    nModules = 0;
    papszModules = NULL;

    pszName = NULL;
    pszPath = NULL;

    papszOptions = NULL;

    poSpatialRef = new OGRSpatialReference( "GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\",SPHEROID[\"GRS 1980\",6378137,298.257222101]],PRIMEM[\"Greenwich\",0],UNIT[\"degree\",0.0174532925199433]]" );
}

/************************************************************************/
/*                        ~OGRTigerDataSource()                         */
/************************************************************************/

OGRTigerDataSource::~OGRTigerDataSource()

{
    int         i;

    for( i = 0; i < nLayers; i++ )
        delete papoLayers[i];
    
    CPLFree( papoLayers );

    CPLFree( pszName );
    CPLFree( pszPath );

    CSLDestroy( papszOptions );

    delete poSpatialRef;
}

/************************************************************************/
/*                              AddLayer()                              */
/************************************************************************/

void OGRTigerDataSource::AddLayer( OGRTigerLayer * poNewLayer )

{
    papoLayers = (OGRTigerLayer **)
        CPLRealloc( papoLayers, sizeof(void*) * ++nLayers );
    
    papoLayers[nLayers-1] = poNewLayer;
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/

OGRLayer *OGRTigerDataSource::GetLayer( int iLayer )

{
    if( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];
}

/************************************************************************/
/*                              GetLayer()                              */
/************************************************************************/

OGRLayer *OGRTigerDataSource::GetLayer( const char *pszLayerName )

{
    for( int iLayer = 0; iLayer < nLayers; iLayer++ )
    {
        if( EQUAL(papoLayers[iLayer]->GetLayerDefn()->GetName(),pszLayerName) )
            return papoLayers[iLayer];
    }

    return NULL;
}

/************************************************************************/
/*                           GetLayerCount()                            */
/************************************************************************/

int OGRTigerDataSource::GetLayerCount()

{
    return nLayers;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

int OGRTigerDataSource::Open( const char * pszFilename, int bTestOpen,
                              char ** papszLimitedFileList )

{
    VSIStatBuf      stat;
    char            **papszFileList = NULL;
    int             i;

    pszName = CPLStrdup( pszFilename );

/* -------------------------------------------------------------------- */
/*      Is the given path a directory or a regular file?                */
/* -------------------------------------------------------------------- */
    if( CPLStat( pszFilename, &stat ) != 0 
        || (!VSI_ISDIR(stat.st_mode) && !VSI_ISREG(stat.st_mode)) )
    {
        if( !bTestOpen )
            CPLError( CE_Failure, CPLE_AppDefined,
                   "%s is neither a file or directory, Tiger access failed.\n",
                      pszFilename );

        return FALSE;
    }
    
/* -------------------------------------------------------------------- */
/*      Build a list of filenames we figure are Tiger files.            */
/* -------------------------------------------------------------------- */
    if( VSI_ISREG(stat.st_mode) )
    {
        char       szModule[128];

        pszPath = CPLStrdup( CPLGetPath(pszFilename) );

        strncpy( szModule, CPLGetFilename(pszFilename), sizeof(szModule)-1 );
        szModule[strlen(szModule)-1] = '\0';

        papszFileList = CSLAddString( papszFileList, szModule );
    }
    else
    {
        char      **candidateFileList = CPLReadDir( pszFilename );
        int         i;

        pszPath = CPLStrdup( pszFilename );

        for( i = 0; 
             candidateFileList != NULL && candidateFileList[i] != NULL; 
             i++ ) 
        {
            if( papszLimitedFileList != NULL 
                && CSLFindString(papszLimitedFileList,
                                 CPLGetBasename(candidateFileList[i])) == -1 )
            {
                continue;
            }
            
            if( (EQUALN(candidateFileList[i],"TGR",3)
                 || EQUALN(candidateFileList[i],"TST",3))
                && candidateFileList[i][strlen(candidateFileList[i])-4] == '.'
                && candidateFileList[i][strlen(candidateFileList[i])-1] == '1')
            {
                char       szModule[128];

                strncpy( szModule, candidateFileList[i],
                         strlen(candidateFileList[i])-1 );

                szModule[strlen(candidateFileList[i])-1] = '\0';

                papszFileList = CSLAddString(papszFileList, szModule);
            }
        }

        if( CSLCount(papszFileList) == 0 )
        {
            if( !bTestOpen )
                CPLError( CE_Failure, CPLE_OpenFailed,
                          "No candidate Tiger files (TGR*.RT1) found in\n"
                          "directory: %s",
                          pszFilename );

            return FALSE;
        }
    }

/* -------------------------------------------------------------------- */
/*      Loop over all these files trying to open them.  In testopen     */
/*      mode we first read the first 80 characters, to verify that      */
/*      it looks like an Tiger file.  Note that we don't keep the file  */
/*      open ... we don't want to occupy alot of file handles when      */
/*      handling a whole directory.                                     */
/* -------------------------------------------------------------------- */
    papszModules = NULL;
    
    for( i = 0; papszFileList[i] != NULL; i++ )
    {
        if( bTestOpen || i == 0 )
        {
            char        szHeader[80];
            FILE        *fp;
            char        *pszFilename;

            pszFilename = BuildFilename( papszFileList[i], "1" );

            fp = VSIFOpen( pszFilename, "rb" );
            CPLFree( pszFilename );
            if( fp == NULL )
                continue;
            
            if( VSIFRead( szHeader, 80, 1, fp ) < 1 )
            {
                VSIFClose( fp );
                continue;
            }

            VSIFClose( fp );
            
            if( szHeader[0] != '1' )
                continue;

            if( !isdigit(szHeader[1]) || !isdigit(szHeader[2])
                || !isdigit(szHeader[3]) || !isdigit(szHeader[4]) )
                continue;

            nVersionCode = atoi(TigerFileBase::GetField( szHeader, 2, 5 ));
            nVersion = TigerClassifyVersion( nVersionCode );

            if(    nVersionCode !=  0
                && nVersionCode !=  2
                && nVersionCode !=  3
                && nVersionCode !=  5
                && nVersionCode != 21 
                && nVersionCode != 24
                && szHeader[3]  != '9'
                && szHeader[3]  != '0' )
                continue;

            // we could (and should) add a bunch more validation here.
        }

        papszModules = CSLAddString( papszModules, papszFileList[i] );
    }

    CSLDestroy( papszFileList );

    nModules = CSLCount( papszModules );

    if( nModules == 0 )
    {
        if( !bTestOpen )
        {
            if( VSI_ISREG(stat.st_mode) )
                CPLError( CE_Failure, CPLE_OpenFailed,
                          "No TIGER/Line files (TGR*.RT1) found in\n"
                          "directory: %s",
                          pszFilename );
            else
                CPLError( CE_Failure, CPLE_OpenFailed,
                          "File %s does not appear to be a TIGER/Line .RT1 file.",
                          pszFilename );
        }

        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Create the layers which appear to exist.                        */
/* -------------------------------------------------------------------- */
    // RT1, RT2, RT3
    AddLayer( new OGRTigerLayer( this,
                                 new TigerCompleteChain( this,
                                                         papszModules[0]) ));

    /* should we have kept track of whether we encountered an RT4 file? */
    // RT4
    AddLayer( new OGRTigerLayer( this,
                                 new TigerAltName( this,
                                                   papszModules[0]) ));

    // RT5
    AddLayer( new OGRTigerLayer( this,
                                 new TigerFeatureIds( this,
                                                      papszModules[0]) ));

    // RT6
    AddLayer( new OGRTigerLayer( this,
                                 new TigerZipCodes( this,
                                                    papszModules[0]) ));
    // RT7
    AddLayer( new OGRTigerLayer( this,
                                 new TigerLandmarks( this,
                                                     papszModules[0]) ));
    
    // RT8
    AddLayer( new OGRTigerLayer( this,
                                 new TigerAreaLandmarks( this,
                                                     papszModules[0]) ));

    // RT9
    if (nVersion < TIGER_2002) {
      AddLayer( new OGRTigerLayer( this,
                                   new TigerKeyFeatures( this,
                                                         papszModules[0]) ));
    }
    
    // RTA, RTS
    AddLayer( new OGRTigerLayer( this,
                                 new TigerPolygon( this,
                                                   papszModules[0]) ));

    // RTB
    if (nVersion >= TIGER_2002) {
      AddLayer( new OGRTigerLayer( this,
                                   new TigerPolygonCorrections( this,
                                                                papszModules[0]) ));
    }
    
    // RTC
    AddLayer( new OGRTigerLayer( this,
                                 new TigerEntityNames( this,
                                                       papszModules[0]) ));

    // RTE
    if (nVersion >= TIGER_2002) {
      AddLayer( new OGRTigerLayer( this,
                                   new TigerPolygonEconomic( this,
                                                             papszModules[0]) ));
    }

    // RTH
    AddLayer( new OGRTigerLayer( this,
                                 new TigerIDHistory( this,
                                                     papszModules[0]) ));
    
    // RTI
    AddLayer( new OGRTigerLayer( this,
                                 new TigerPolyChainLink( this,
                                                       papszModules[0]) ));
    
    // RTP
    AddLayer( new OGRTigerLayer( this,
                                 new TigerPIP( this,
                                               papszModules[0]) ));
    
    // RTR
    AddLayer( new OGRTigerLayer( this,
                                 new TigerTLIDRange( this,
                                                     papszModules[0]) ));
    
    // RTT
    if (nVersion >= TIGER_2002) {
      AddLayer( new OGRTigerLayer( this,
                                   new TigerZeroCellID( this,
                                                        papszModules[0]) ));
    }

    // RTU
    if (nVersion >= TIGER_2002) {
      AddLayer( new OGRTigerLayer( this,
                                   new TigerOverUnder( this,
                                                       papszModules[0]) ));
    }

    // RTZ
    AddLayer( new OGRTigerLayer( this,
                                 new TigerZipPlus4( this,
                                                     papszModules[0]) ));
    
    return TRUE;
}

/************************************************************************/
/*                             SetOptions()                             */
/************************************************************************/

void OGRTigerDataSource::SetOptionList( char ** papszNewOptions )

{
    CSLDestroy( papszOptions );
    papszOptions = CSLDuplicate( papszNewOptions );
}

/************************************************************************/
/*                             GetOption()                              */
/************************************************************************/

const char *OGRTigerDataSource::GetOption( const char * pszOption )

{
    return CSLFetchNameValue( papszOptions, pszOption );
}

/************************************************************************/
/*                             GetModule()                              */
/************************************************************************/

const char *OGRTigerDataSource::GetModule( int iModule )

{
    if( iModule < 0 || iModule >= nModules )
        return NULL;
    else
        return papszModules[iModule];
}

/************************************************************************/
/*                            CheckModule()                             */
/*                                                                      */
/*      This is used by the writer to check if this module has been     */
/*      written to before.                                              */
/************************************************************************/

int OGRTigerDataSource::CheckModule( const char *pszModule )

{
    int         i;

    for( i = 0; i < nModules; i++ )
    {
        if( EQUAL(pszModule,papszModules[i]) )
            return TRUE;
    }
    return FALSE;
}

/************************************************************************/
/*                             AddModule()                              */
/************************************************************************/

void OGRTigerDataSource::AddModule( const char *pszModule )

{
    if( CheckModule( pszModule ) )
        return;

    papszModules = CSLAddString( papszModules, pszModule );
    nModules++;
}

/************************************************************************/
/*                           BuildFilename()                            */
/************************************************************************/

char *OGRTigerDataSource::BuildFilename( const char *pszModuleName,
                                    const char *pszExtension )

{
    char        *pszFilename;
    char        szLCExtension[3];

/* -------------------------------------------------------------------- */
/*      Force the record type to lower case if the filename appears     */
/*      to be in lower case.                                            */
/* -------------------------------------------------------------------- */
    if( *pszExtension >= 'A' && *pszExtension <= 'Z' && *pszModuleName == 't' )
    {
        szLCExtension[0] = (*pszExtension) + 'a' - 'A';
        szLCExtension[1] = '\0';
        pszExtension = szLCExtension;
    }

/* -------------------------------------------------------------------- */
/*      Build the filename.                                             */
/* -------------------------------------------------------------------- */
    pszFilename = (char *) CPLMalloc(strlen(GetDirPath())
                                     + strlen(pszModuleName)
                                     + strlen(pszExtension) + 10);

    if( strlen(GetDirPath()) == 0 )
        sprintf( pszFilename, "%s%s",
                 pszModuleName, pszExtension );
    else
        sprintf( pszFilename, "%s/%s%s",
                 GetDirPath(), pszModuleName, pszExtension );

    return pszFilename;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRTigerDataSource::TestCapability( const char *pszCap )

{
    if( EQUAL(pszCap,ODsCCreateLayer) )
        return GetWriteMode();
    else
        return FALSE;
}

/************************************************************************/
/*                               Create()                               */
/************************************************************************/

int OGRTigerDataSource::Create( const char *pszNameIn, char **papszOptions )

{
    VSIStatBuf      stat;
    
/* -------------------------------------------------------------------- */
/*      Try to create directory if it doesn't already exist.            */
/* -------------------------------------------------------------------- */
    if( CPLStat( pszNameIn, &stat ) != 0 )
    {
        VSIMkdir( pszNameIn, 0755 );
    }

    if( CPLStat( pszNameIn, &stat ) != 0 || !VSI_ISDIR(stat.st_mode) )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "%s is not a directory, nor can be directly created as one.",
                  pszName );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Store various information.                                      */
/* -------------------------------------------------------------------- */
    pszPath = CPLStrdup( pszNameIn );
    pszName = CPLStrdup( pszNameIn );
    bWriteMode = TRUE;

    SetOptionList( papszOptions );

/* -------------------------------------------------------------------- */
/*      Work out the version.                                           */
/* -------------------------------------------------------------------- */
//    nVersionCode = 1000; /* census 2000 */

    nVersionCode = 1002; /* census 2002 */
    if( GetOption("VERSION") != NULL )
    {
        nVersionCode = atoi(GetOption("VERSION"));
        nVersionCode = MAX(0,MIN(9999,nVersionCode));
    }
    nVersion = TigerClassifyVersion(nVersionCode);

    return TRUE;
}

/************************************************************************/
/*                            CreateLayer()                             */
/************************************************************************/

OGRLayer *OGRTigerDataSource::CreateLayer( const char *pszLayerName, 
                                           OGRSpatialReference *poSpatRef, 
                                           OGRwkbGeometryType eGType, 
                                           char **papszOptions )

{
    OGRTigerLayer       *poLayer = NULL;

    if( GetLayer( pszLayerName ) != NULL )
        return GetLayer( pszLayerName );

    if( poSpatRef != NULL && 
        (!poSpatRef->IsGeographic() 
         || !EQUAL(poSpatRef->GetAttrValue("DATUM"),
                   "North_American_Datum_1983")) )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Requested coordinate system wrong for Tiger, " 
                  "forcing to GEOGCS NAD83." );
    }

    if( EQUAL(pszLayerName,"PIP") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerPIP( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"ZipPlus4") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerZipPlus4( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"TLIDRange") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerTLIDRange( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"PolyChainLink") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerPolyChainLink( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"CompleteChain") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerCompleteChain( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"AltName") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerAltName( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"FeatureIds") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerFeatureIds( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"ZipCodes") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerZipCodes( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"Landmarks") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerLandmarks( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"AreaLandmarks") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerAreaLandmarks( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"KeyFeatures") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerKeyFeatures( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"EntityNames") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerEntityNames( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"IDHistory") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerIDHistory( this, NULL ) );
    }
    else if( EQUAL(pszLayerName,"Polygon") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerPolygon( this, NULL ) );
    }

    else if( EQUAL(pszLayerName,"PolygonCorrections") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerPolygonCorrections( this, NULL ) );
    }

    else if( EQUAL(pszLayerName,"PolygonEconomic") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerPolygonEconomic( this, NULL ) );
    }

    else if( EQUAL(pszLayerName,"ZeroCellID") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerZeroCellID( this, NULL ) );
    }

    else if( EQUAL(pszLayerName,"OverUnder") )
    {
        poLayer = new OGRTigerLayer( this,
                                     new TigerOverUnder( this, NULL ) );
    }

    if( poLayer == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Unable to create layer %s, not a known TIGER/Line layer.",
                  pszLayerName );
    }
    else
        AddLayer( poLayer );

    return poLayer;
}

/************************************************************************/
/*                         DeleteModuleFiles()                          */
/************************************************************************/

void OGRTigerDataSource::DeleteModuleFiles( const char *pszModule )

{
    char        **papszDirFiles = CPLReadDir( GetDirPath() );
    int         i, nCount = CSLCount(papszDirFiles);
    
    for( i = 0; i < nCount; i++ )
    {
        if( EQUALN(pszModule,papszDirFiles[i],strlen(pszModule)) )
        {
            const char  *pszFilename;

            pszFilename = CPLFormFilename( GetDirPath(), 
                                           papszDirFiles[i], 
                                           NULL );
            if( VSIUnlink( pszFilename ) != 0 )
            {
                CPLDebug( "OGR_TIGER", "Failed to unlink %s", pszFilename );
            }
        }
    }

    CSLDestroy( papszDirFiles );
}
