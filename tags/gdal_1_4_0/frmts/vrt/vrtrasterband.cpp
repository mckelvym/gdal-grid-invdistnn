/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Implementation of VRTRasterBand
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.23  2006/06/22 20:02:18  fwarmerdam
 * use GDALMultiDomainMetadata (de)serialize logic
 *
 * Revision 1.22  2006/02/08 06:12:08  fwarmerdam
 * Override SetMetadata methods so that metadata can be preserved.
 * Support saving histograms in VRT per bug 1060.
 *
 * Revision 1.21  2005/05/05 13:56:14  fwarmerdam
 * moved metadata handling to PAM
 *
 * Revision 1.20  2004/08/11 18:46:45  warmerda
 * pass pszVRTPath through serialize methods
 *
 * Revision 1.19  2004/07/28 16:55:08  warmerda
 * split alot of functionality out into VRTSourcedRasterBand
 *
 * Revision 1.18  2004/04/15 18:54:38  warmerda
 * added UnitType, Offset, Scale and CategoryNames support
 *
 * Revision 1.17  2004/03/16 18:34:35  warmerda
 * added support for relativeToVRT attribute on SourceFilename
 *
 * Revision 1.16  2003/12/10 15:44:42  warmerda
 * Fix problem with NULL pszDomain values.
 *
 * Revision 1.15  2003/07/17 20:29:15  warmerda
 * Moved all the sources implementations out.
 * Improved error checking and returning when initializing from XML.
 * Added vrt_sources (and new_vrt_sources) metadata domains to permit
 * management of sources via XML passed in/out of the metadata methods.
 *
 * Revision 1.14  2003/06/10 19:59:33  warmerda
 * added new Func based source type for passthrough to a callback
 *
 * Revision 1.13  2003/03/14 10:31:22  dron
 * Check return value of the VRTSimpleSource::XMLInit() in VRTRasterBand::XMLInit().
 *
 * Revision 1.12  2003/03/13 20:39:25  dron
 * Improved block initialization logic in IRasterIO().
 *
 * Revision 1.11  2003/01/15 21:30:48  warmerda
 * some rounding tweaks when adjusting output size in GetSrcDstWindow
 *
 * Revision 1.10  2002/12/13 15:07:07  warmerda
 * fixed bug in IReadBlock
 *
 * Revision 1.9  2002/12/04 03:12:57  warmerda
 * Fixed some bugs in last change.
 *
 * Revision 1.8  2002/12/03 15:21:19  warmerda
 * finished up window computations for odd cases
 *
 * Revision 1.7  2002/11/24 04:29:02  warmerda
 * Substantially rewrote VRTSimpleSource.  Now VRTSource is base class, and
 * sources do their own SerializeToXML(), and XMLInit().  New VRTComplexSource
 * supports scaling and nodata values.
 *
 * Revision 1.6  2002/07/15 18:51:46  warmerda
 * ensure even unshared datasets are dereferenced properly
 *
 * Revision 1.5  2002/05/29 18:13:44  warmerda
 * added nodata handling for averager
 *
 * Revision 1.4  2002/05/29 16:06:05  warmerda
 * complete detailed band metadata
 *
 * Revision 1.3  2001/12/06 19:53:58  warmerda
 * added write check to rasterio
 *
 * Revision 1.2  2001/11/16 21:38:07  warmerda
 * try to work even if bModified
 *
 * Revision 1.1  2001/11/16 21:14:31  warmerda
 * New
 *
 */

#include "vrtdataset.h"
#include "cpl_minixml.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*                          VRTRasterBand                               */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           VRTRasterBand()                            */
/************************************************************************/

VRTRasterBand::VRTRasterBand()

{
    Initialize( 0, 0 );
}

/************************************************************************/
/*                             Initialize()                             */
/************************************************************************/

void VRTRasterBand::Initialize( int nXSize, int nYSize )

{
    poDS = NULL;
    nBand = 0;
    eAccess = GA_ReadOnly;
    eDataType = GDT_Byte;

    nRasterXSize = nXSize;
    nRasterYSize = nYSize;
    
    nBlockXSize = MIN(128,nXSize);
    nBlockYSize = MIN(128,nYSize);

    bNoDataValueSet = FALSE;
    dfNoDataValue = -10000.0;
    poColorTable = NULL;
    eColorInterp = GCI_Undefined;

    pszUnitType = NULL;
    papszCategoryNames = NULL;
    dfOffset = 0.0;
    dfScale = 1.0;

    psSavedHistograms = NULL;
}

/************************************************************************/
/*                           ~VRTRasterBand()                           */
/************************************************************************/

VRTRasterBand::~VRTRasterBand()

{
    CPLFree( pszUnitType );

    if( poColorTable != NULL )
        delete poColorTable;

    CSLDestroy( papszCategoryNames );
}

/************************************************************************/
/*                         CopyCommonInfoFrom()                         */
/*                                                                      */
/*      Copy common metadata, pixel descriptions, and color             */
/*      interpretation from the provided source band.                   */
/************************************************************************/

CPLErr VRTRasterBand::CopyCommonInfoFrom( GDALRasterBand * poSrcBand )

{
    int bSuccess;
    double dfNoData;

    SetMetadata( poSrcBand->GetMetadata() );
    SetColorTable( poSrcBand->GetColorTable() );
    SetColorInterpretation(poSrcBand->GetColorInterpretation());
    if( strlen(poSrcBand->GetDescription()) > 0 )
        SetDescription( poSrcBand->GetDescription() );
    dfNoData = poSrcBand->GetNoDataValue( &bSuccess );
    if( bSuccess )
        SetNoDataValue( dfNoData );

    SetOffset( poSrcBand->GetOffset() );
    SetScale( poSrcBand->GetScale() );
    SetCategoryNames( poSrcBand->GetCategoryNames() );
    if( !EQUAL(poSrcBand->GetUnitType(),"") )
        SetUnitType( poSrcBand->GetUnitType() );

    return CE_None;
}

/************************************************************************/
/*                            SetMetadata()                             */
/************************************************************************/

CPLErr VRTRasterBand::SetMetadata( char **papszMetadata, 
                                   const char *pszDomain )

{
    ((VRTDataset *) poDS)->SetNeedsFlush();

    return GDALRasterBand::SetMetadata( papszMetadata, pszDomain );
}

/************************************************************************/
/*                          SetMetadataItem()                           */
/************************************************************************/

CPLErr VRTRasterBand::SetMetadataItem( const char *pszName, 
                                       const char *pszValue, 
                                       const char *pszDomain )

{
    ((VRTDataset *) poDS)->SetNeedsFlush();

    return GDALRasterBand::SetMetadataItem( pszName, pszValue, pszDomain );
}

/************************************************************************/
/*                            GetUnitType()                             */
/************************************************************************/

const char *VRTRasterBand::GetUnitType()

{
    if( pszUnitType == NULL )
        return "";
    else
        return pszUnitType;
}

/************************************************************************/
/*                            SetUnitType()                             */
/************************************************************************/

CPLErr VRTRasterBand::SetUnitType( const char *pszNewValue )

{
    CPLFree( pszUnitType );
    
    if( pszNewValue == NULL )
        pszUnitType = NULL;
    else
        pszUnitType = CPLStrdup(pszNewValue);

    return CE_None;
}

/************************************************************************/
/*                             GetOffset()                              */
/************************************************************************/

double VRTRasterBand::GetOffset( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = TRUE;

    return dfOffset;
}

/************************************************************************/
/*                             SetOffset()                              */
/************************************************************************/

CPLErr VRTRasterBand::SetOffset( double dfNewOffset )

{
    dfOffset = dfNewOffset;
    return CE_None;
}

/************************************************************************/
/*                              GetScale()                              */
/************************************************************************/

double VRTRasterBand::GetScale( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = TRUE;

    return dfScale;
}

/************************************************************************/
/*                              SetScale()                              */
/************************************************************************/

CPLErr VRTRasterBand::SetScale( double dfNewScale )

{
    dfScale = dfNewScale;
    return CE_None;
}

/************************************************************************/
/*                          GetCategoryNames()                          */
/************************************************************************/

char **VRTRasterBand::GetCategoryNames()

{
    return papszCategoryNames;
}

/************************************************************************/
/*                          SetCategoryNames()                          */
/************************************************************************/

CPLErr VRTRasterBand::SetCategoryNames( char ** papszNewNames )

{
    CSLDestroy( papszCategoryNames );
    papszCategoryNames = CSLDuplicate( papszNewNames );

    return CE_None;
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTRasterBand::XMLInit( CPLXMLNode * psTree, 
                               const char *pszVRTPath )

{
/* -------------------------------------------------------------------- */
/*      Validate a bit.                                                 */
/* -------------------------------------------------------------------- */
    if( psTree == NULL || psTree->eType != CXT_Element
        || !EQUAL(psTree->pszValue,"VRTRasterBand") )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Invalid node passed to VRTRasterBand::XMLInit()." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Set the band if provided as an attribute.                       */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLValue( psTree, "band", NULL) != NULL )
        nBand = atoi(CPLGetXMLValue( psTree, "band", "0"));

/* -------------------------------------------------------------------- */
/*      Set the band if provided as an attribute.                       */
/* -------------------------------------------------------------------- */
    const char *pszDataType = CPLGetXMLValue( psTree, "dataType", NULL);
    if( pszDataType != NULL )
    {
        for( int iType = 0; iType < GDT_TypeCount; iType++ )
        {
            const char *pszThisName = GDALGetDataTypeName((GDALDataType)iType);

            if( pszThisName != NULL && EQUAL(pszDataType,pszThisName) )
            {
                eDataType = (GDALDataType) iType;
                break;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Apply any band level metadata.                                  */
/* -------------------------------------------------------------------- */
    oMDMD.XMLInit( psTree, TRUE );

/* -------------------------------------------------------------------- */
/*      Collect various other items of metadata.                        */
/* -------------------------------------------------------------------- */
    SetDescription( CPLGetXMLValue( psTree, "Description", "" ) );
    
    if( CPLGetXMLValue( psTree, "NoDataValue", NULL ) != NULL )
        SetNoDataValue( atof(CPLGetXMLValue( psTree, "NoDataValue", "0" )) );

    SetUnitType( CPLGetXMLValue( psTree, "UnitType", NULL ) );

    SetOffset( atof(CPLGetXMLValue( psTree, "Offset", "0.0" )) );
    SetScale( atof(CPLGetXMLValue( psTree, "Scale", "1.0" )) );

    if( CPLGetXMLValue( psTree, "ColorInterp", NULL ) != NULL )
    {
        const char *pszInterp = CPLGetXMLValue( psTree, "ColorInterp", NULL );
        int        iInterp;
        
        for( iInterp = 0; iInterp < 13; iInterp++ )
        {
            const char *pszCandidate 
                = GDALGetColorInterpretationName( (GDALColorInterp) iInterp );

            if( pszCandidate != NULL && EQUAL(pszCandidate,pszInterp) )
            {
                SetColorInterpretation( (GDALColorInterp) iInterp );
                break;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Category names.                                                 */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLNode( psTree, "CategoryNames" ) != NULL )
    {
        CPLXMLNode *psEntry;

        CSLDestroy( papszCategoryNames );
        papszCategoryNames = NULL;

        for( psEntry = CPLGetXMLNode( psTree, "CategoryNames" )->psChild;
             psEntry != NULL; psEntry = psEntry->psNext )
        {
            if( psEntry->eType != CXT_Element 
                || !EQUAL(psEntry->pszValue,"Category") 
                || psEntry->psChild == NULL 
                || psEntry->psChild->eType != CXT_Text )
                continue;
            
            papszCategoryNames = CSLAddString( papszCategoryNames, 
                                               psEntry->psChild->pszValue );
        }
    }

/* -------------------------------------------------------------------- */
/*      Collect a color table.                                          */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLNode( psTree, "ColorTable" ) != NULL )
    {
        CPLXMLNode *psEntry;
        GDALColorTable oTable;
        int        iEntry = 0;

        for( psEntry = CPLGetXMLNode( psTree, "ColorTable" )->psChild;
             psEntry != NULL; psEntry = psEntry->psNext )
        {
            GDALColorEntry sCEntry;

            sCEntry.c1 = (short) atoi(CPLGetXMLValue( psEntry, "c1", "0" ));
            sCEntry.c2 = (short) atoi(CPLGetXMLValue( psEntry, "c2", "0" ));
            sCEntry.c3 = (short) atoi(CPLGetXMLValue( psEntry, "c3", "0" ));
            sCEntry.c4 = (short) atoi(CPLGetXMLValue( psEntry, "c4", "255" ));

            oTable.SetColorEntry( iEntry++, &sCEntry );
        }
        
        SetColorTable( &oTable );
    }

/* -------------------------------------------------------------------- */
/*      Histograms                                                      */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psHist = CPLGetXMLNode( psTree, "Histograms" );
    if( psHist != NULL )
    {
        CPLXMLNode *psNext = psHist->psNext;
        psHist->psNext = NULL;

        psSavedHistograms = CPLCloneXMLTree( psHist );
        psHist->psNext = psNext;
    }

    return CE_None;
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTRasterBand::SerializeToXML( const char *pszVRTPath )

{
    CPLXMLNode *psTree;

    psTree = CPLCreateXMLNode( NULL, CXT_Element, "VRTRasterBand" );

/* -------------------------------------------------------------------- */
/*      Various kinds of metadata.                                      */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psMD;

    CPLSetXMLValue( psTree, "#dataType", 
                    GDALGetDataTypeName( GetRasterDataType() ) );

    if( nBand > 0 )
        CPLSetXMLValue( psTree, "#band", CPLSPrintf( "%d", GetBand() ) );

    psMD = oMDMD.Serialize();
    if( psMD != NULL )
        CPLAddXMLChild( psTree, psMD );

    if( strlen(GetDescription()) > 0 )
        CPLSetXMLValue( psTree, "Description", GetDescription() );

    if( bNoDataValueSet )
        CPLSetXMLValue( psTree, "NoDataValue", 
                        CPLSPrintf( "%.14E", dfNoDataValue ) );

    if( pszUnitType != NULL )
        CPLSetXMLValue( psTree, "UnitType", pszUnitType );

    if( dfOffset != 0.0 )
        CPLSetXMLValue( psTree, "Offset", 
                        CPLSPrintf( "%.16g", dfOffset ) );

    if( dfScale != 1.0 )
        CPLSetXMLValue( psTree, "Scale", 
                        CPLSPrintf( "%.16g", dfScale ) );

    if( eColorInterp != GCI_Undefined )
        CPLSetXMLValue( psTree, "ColorInterp", 
                        GDALGetColorInterpretationName( eColorInterp ) );

/* -------------------------------------------------------------------- */
/*      Category names.                                                 */
/* -------------------------------------------------------------------- */
    if( papszCategoryNames != NULL )
    {
        CPLXMLNode *psCT_XML = CPLCreateXMLNode( psTree, CXT_Element, 
                                                 "CategoryNames" );

        for( int iEntry=0; papszCategoryNames[iEntry] != NULL; iEntry++ )
        {
            CPLCreateXMLElementAndValue( psCT_XML, "Category", 
                                         papszCategoryNames[iEntry] );
        }
    }

/* -------------------------------------------------------------------- */
/*      Histograms.                                                     */
/* -------------------------------------------------------------------- */
    if( psSavedHistograms != NULL )
        CPLAddXMLChild( psTree, CPLCloneXMLTree( psSavedHistograms ) );

/* -------------------------------------------------------------------- */
/*      Color Table.                                                    */
/* -------------------------------------------------------------------- */
    if( poColorTable != NULL )
    {
        CPLXMLNode *psCT_XML = CPLCreateXMLNode( psTree, CXT_Element, 
                                                 "ColorTable" );

        for( int iEntry=0; iEntry < poColorTable->GetColorEntryCount(); 
             iEntry++ )
        {
            GDALColorEntry sEntry;
            CPLXMLNode *psEntry_XML = CPLCreateXMLNode( psCT_XML, CXT_Element, 
                                                        "Entry" );

            poColorTable->GetColorEntryAsRGB( iEntry, &sEntry );
            
            CPLSetXMLValue( psEntry_XML, "#c1", CPLSPrintf("%d",sEntry.c1) );
            CPLSetXMLValue( psEntry_XML, "#c2", CPLSPrintf("%d",sEntry.c2) );
            CPLSetXMLValue( psEntry_XML, "#c3", CPLSPrintf("%d",sEntry.c3) );
            CPLSetXMLValue( psEntry_XML, "#c4", CPLSPrintf("%d",sEntry.c4) );
        }
    }

    return psTree;
}

/************************************************************************/
/*                           SetNoDataValue()                           */
/************************************************************************/

CPLErr VRTRasterBand::SetNoDataValue( double dfNewValue )

{
    bNoDataValueSet = TRUE;
    dfNoDataValue = dfNewValue;
    
    ((VRTDataset *)poDS)->SetNeedsFlush();

    return CE_None;
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

double VRTRasterBand::GetNoDataValue( int *pbSuccess )

{
    if( pbSuccess )
        *pbSuccess = bNoDataValueSet;

    return dfNoDataValue;
}

/************************************************************************/
/*                           SetColorTable()                            */
/************************************************************************/

CPLErr VRTRasterBand::SetColorTable( GDALColorTable *poTableIn )

{
    if( poColorTable != NULL )
    {
        delete poColorTable;
        poColorTable = NULL;
    }

    if( poTableIn )
    {
        poColorTable = poTableIn->Clone();
        eColorInterp = GCI_PaletteIndex;
    }

    ((VRTDataset *)poDS)->SetNeedsFlush();

    return CE_None;
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

GDALColorTable *VRTRasterBand::GetColorTable()

{
    return poColorTable;
}

/************************************************************************/
/*                       SetColorInterpretation()                       */
/************************************************************************/

CPLErr VRTRasterBand::SetColorInterpretation( GDALColorInterp eInterpIn )

{
    ((VRTDataset *)poDS)->SetNeedsFlush();

    eColorInterp = eInterpIn;

    return CE_None;
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp VRTRasterBand::GetColorInterpretation()

{
    return eColorInterp;
}

/************************************************************************/
/*                            GetHistogram()                            */
/************************************************************************/

CPLErr VRTRasterBand::GetHistogram( double dfMin, double dfMax,
                                    int nBuckets, int * panHistogram,
                                    int bIncludeOutOfRange, int bApproxOK,
                                    GDALProgressFunc pfnProgress, 
                                    void *pProgressData )

{
/* -------------------------------------------------------------------- */
/*      Check if we have a matching histogram.                          */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psHistItem;

    psHistItem = PamFindMatchingHistogram( psSavedHistograms, 
                                           dfMin, dfMax, nBuckets, 
                                           bIncludeOutOfRange, bApproxOK );
    if( psHistItem != NULL )
    {
        int *panTempHist = NULL;

        if( PamParseHistogram( psHistItem, &dfMin, &dfMax, &nBuckets, 
                               &panTempHist,
                               &bIncludeOutOfRange, &bApproxOK ) )
        {
            memcpy( panHistogram, panTempHist, sizeof(int) * nBuckets );
            CPLFree( panTempHist );
            return CE_None;
        }
    }

/* -------------------------------------------------------------------- */
/*      We don't have an existing histogram matching the request, so    */
/*      generate one manually.                                          */
/* -------------------------------------------------------------------- */
    CPLErr eErr;

    eErr = GDALRasterBand::GetHistogram( dfMin, dfMax, 
                                         nBuckets, panHistogram, 
                                         bIncludeOutOfRange, bApproxOK,
                                         pfnProgress, pProgressData );

/* -------------------------------------------------------------------- */
/*      Save an XML description of this histogram.                      */
/* -------------------------------------------------------------------- */
    if( eErr == CE_None )
    {
        CPLXMLNode *psXMLHist;

        psXMLHist = PamHistogramToXMLTree( dfMin, dfMax, nBuckets, 
                                           panHistogram, 
                                           bIncludeOutOfRange, bApproxOK );
        if( psXMLHist != NULL )
        {
            ((VRTDataset *) poDS)->SetNeedsFlush();

            if( psSavedHistograms == NULL )
                psSavedHistograms = CPLCreateXMLNode( NULL, CXT_Element,
                                                      "Histograms" );
            
            CPLAddXMLChild( psSavedHistograms, psXMLHist );
        }
    }

    return eErr;
}

/************************************************************************/
/*                        SetDefaultHistogram()                         */
/************************************************************************/

CPLErr VRTRasterBand::SetDefaultHistogram( double dfMin, double dfMax, 
                                           int nBuckets, int *panHistogram)

{
    CPLXMLNode *psNode;

/* -------------------------------------------------------------------- */
/*      Do we have a matching histogram we should replace?              */
/* -------------------------------------------------------------------- */
    psNode = PamFindMatchingHistogram( psSavedHistograms, 
                                       dfMin, dfMax, nBuckets,
                                       TRUE, TRUE );
    if( psNode != NULL )
    {
        /* blow this one away */
        CPLRemoveXMLChild( psSavedHistograms, psNode );
        CPLDestroyXMLNode( psNode );
    }

/* -------------------------------------------------------------------- */
/*      Translate into a histogram XML tree.                            */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psHistItem;

    psHistItem = PamHistogramToXMLTree( dfMin, dfMax, nBuckets, 
                                        panHistogram, TRUE, FALSE );

/* -------------------------------------------------------------------- */
/*      Insert our new default histogram at the front of the            */
/*      histogram list so that it will be the default histogram.        */
/* -------------------------------------------------------------------- */
    ((VRTDataset *) poDS)->SetNeedsFlush();

    if( psSavedHistograms == NULL )
        psSavedHistograms = CPLCreateXMLNode( NULL, CXT_Element,
                                              "Histograms" );
            
    psHistItem->psNext = psSavedHistograms->psChild;
    psSavedHistograms->psChild = psHistItem;
    
    return CE_None;
}

/************************************************************************/
/*                        GetDefaultHistogram()                         */
/************************************************************************/

CPLErr 
VRTRasterBand::GetDefaultHistogram( double *pdfMin, double *pdfMax, 
                                    int *pnBuckets, int **ppanHistogram, 
                                    int bForce,
                                    GDALProgressFunc pfnProgress, 
                                    void *pProgressData )
    
{
    if( psSavedHistograms != NULL )
    {
        CPLXMLNode *psXMLHist;

        for( psXMLHist = psSavedHistograms->psChild;
             psXMLHist != NULL; psXMLHist = psXMLHist->psNext )
        {
            int bApprox, bIncludeOutOfRange;

            if( psXMLHist->eType != CXT_Element
                || !EQUAL(psXMLHist->pszValue,"HistItem") )
                continue;

            if( PamParseHistogram( psXMLHist, pdfMin, pdfMax, pnBuckets, 
                                   ppanHistogram, &bIncludeOutOfRange,
                                   &bApprox ) )
                return CE_None;
            else
                return CE_Failure;
        }
    }

    return GDALRasterBand::GetDefaultHistogram( pdfMin, pdfMax, pnBuckets, 
                                                ppanHistogram, bForce, 
                                                pfnProgress,pProgressData);
}
