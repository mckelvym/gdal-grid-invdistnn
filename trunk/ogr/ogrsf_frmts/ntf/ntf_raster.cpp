/******************************************************************************
 * $Id$
 *
 * Project:  NTF Translator
 * Purpose:  Handle UK Ordnance Survey Raster DTM products.  Includes some
 *           raster related methods from NTFFileReader and the implementation
 *           of OGRNTFRasterLayer.
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
 * Revision 1.2  1999/10/04 11:36:53  warmerda
 * Added HEIGHT attribute.
 *
 * Revision 1.1  1999/10/04 03:07:49  warmerda
 * New
 *
 */

#include "ntf.h"

/************************************************************************/
/* ==================================================================== */
/*		       NTFFileReader Raster Methods                     */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                          IsRasterProduct()                           */
/************************************************************************/

int NTFFileReader::IsRasterProduct()

{
    return GetProductId() == NPC_LANDRANGER_DTM
        || GetProductId() == NPC_LANDFORM_PROFILE_DTM;
}

/************************************************************************/
/*                       EstablishRasterAccess()                        */
/************************************************************************/

void NTFFileReader::EstablishRasterAccess()

{
/* -------------------------------------------------------------------- */
/*      Read the type 50 record.                                        */
/* -------------------------------------------------------------------- */
    NTFRecord	*poRecord;

    while( (poRecord = ReadRecord()) != NULL
           && poRecord->GetType() != NRT_GRIDHREC
           && poRecord->GetType() != NRT_VTR )
    {
        delete poRecord;
    }

    if( poRecord->GetType() != NRT_GRIDHREC )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Unable to find GRIDHREC (type 50) record in what appears\n"
                  "to be an NTF Raster DTM product." );
        return;
    }

/* -------------------------------------------------------------------- */
/*      Parse if LANDRANGER_DTM                                         */
/* -------------------------------------------------------------------- */
    if( GetProductId() == NPC_LANDRANGER_DTM )
    {
        nRasterXSize = atoi(poRecord->GetField(13,16));
        nRasterYSize = atoi(poRecord->GetField(17,20));

        adfGeoTransform[0] = atoi(poRecord->GetField(25,34));
        adfGeoTransform[1] = 50;
        adfGeoTransform[2] = 0;
        adfGeoTransform[3] = atoi(poRecord->GetField(87,96));
        adfGeoTransform[4] = 0;
        adfGeoTransform[5] = -50;
        
        nRasterDataType = 3; /* GDT_Int16 */
    }

/* -------------------------------------------------------------------- */
/*      Parse if LANDFORM_PROFILE_DTM                                   */
/* -------------------------------------------------------------------- */
    else if( GetProductId() == NPC_LANDFORM_PROFILE_DTM )
    {
        nRasterXSize = atoi(poRecord->GetField(23,30));
        nRasterYSize = atoi(poRecord->GetField(31,38));

        adfGeoTransform[0] = atoi(poRecord->GetField(13,17))
		           + GetXOrigin();
        adfGeoTransform[1] = atoi(poRecord->GetField(39,42));
        adfGeoTransform[2] = 0;
        adfGeoTransform[3] = atoi(poRecord->GetField(18,22))
		           + GetYOrigin();
        adfGeoTransform[4] = 0;
        adfGeoTransform[5] = -1 * atoi(poRecord->GetField(43,46));
        
        nRasterDataType = 3; /* GDT_Int16 */
    }

/* -------------------------------------------------------------------- */
/*      Initialize column offsets table.                                */
/* -------------------------------------------------------------------- */
    panColumnOffset = (long *) CPLCalloc(sizeof(long),nRasterXSize);

    GetFPPos( panColumnOffset+0, NULL );

/* -------------------------------------------------------------------- */
/*      Create an OGRSFLayer for this file readers raster points.       */
/* -------------------------------------------------------------------- */
    if( poDS != NULL )
    {
        poRasterLayer = new OGRNTFRasterLayer( poDS, this );
        poDS->AddLayer( poRasterLayer );
    }
}

/************************************************************************/
/*                          ReadRasterColumn()                          */
/************************************************************************/

CPLErr NTFFileReader::ReadRasterColumn( int iColumn, float *pafElev )

{
/* -------------------------------------------------------------------- */
/*      If we don't already have the scanline offset of the previous    */
/*      line, force reading of previous records to establish it.        */
/* -------------------------------------------------------------------- */
    if( panColumnOffset[iColumn] == 0 )
    {
        int	iPrev;
        
        for( iPrev = 0; iPrev < iColumn; iPrev++ )
        {
            if( panColumnOffset[iPrev] == 0 )
            {
                CPLErr	eErr;
                
                eErr = ReadRasterColumn( iPrev, pafElev );
                if( eErr != CE_None )
                    return eErr;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Read requested record.                                          */
/* -------------------------------------------------------------------- */
    NTFRecord	*poRecord;
    
    SetFPPos( panColumnOffset[iColumn], 0 );
    poRecord = ReadRecord();

    if( iColumn < nRasterXSize-1 )
    {
        GetFPPos( panColumnOffset+iColumn+1, NULL );
    }
    
/* -------------------------------------------------------------------- */
/*      Handle LANDRANGER DTM columns.                                  */
/* -------------------------------------------------------------------- */
    if( GetProductId() == NPC_LANDRANGER_DTM )
    {
        double	dfVScale, dfVOffset;

        dfVOffset = atoi(poRecord->GetField(56,65));
        dfVScale = atoi(poRecord->GetField(66,75)) * 0.001;

        for( int iPixel = 0; iPixel < nRasterXSize; iPixel++ )
        {
            pafElev[iPixel] = dfVOffset + dfVScale *
                atoi(poRecord->GetField(84+iPixel*4,87+iPixel*4));
        }
    }
            
/* -------------------------------------------------------------------- */
/*      Handle PROFILE                                                  */
/* -------------------------------------------------------------------- */
    else if( GetProductId() == NPC_LANDFORM_PROFILE_DTM )
    {
        for( int iPixel = 0; iPixel < nRasterXSize; iPixel++ )
        {
            pafElev[iPixel] = 
                atoi(poRecord->GetField(19+iPixel*5,23+iPixel*5));
        }
    }
    
    delete poRecord;

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*		          OGRNTFRasterLayer                             */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                          OGRNTFRasterLayer                           */
/************************************************************************/

OGRNTFRasterLayer::OGRNTFRasterLayer( OGRNTFDataSource *poDSIn,
                                      NTFFileReader * poReaderIn )

{
    poReader = poReaderIn;
    poDS = poDSIn;
    poFilterGeom = NULL;

    poFeatureDefn = new OGRFeatureDefn( poReaderIn->GetTileName() );
    poFeatureDefn->SetGeomType( wkbPoint25D );

    OGRFieldDefn      oHeight( "HEIGHT", OFTInteger );
    oHeight.SetWidth(5);
    poFeatureDefn->AddFieldDefn( &oHeight );

    pafColumn = (float *) CPLCalloc(sizeof(float),
                                    poReader->GetRasterYSize());
    iColumnOffset = -1;
    iCurrentFC = 0;
}

/************************************************************************/
/*                         ~OGRNTFRasterLayer()                         */
/************************************************************************/

OGRNTFRasterLayer::~OGRNTFRasterLayer()

{
    delete poFeatureDefn;

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRNTFRasterLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        poFilterGeom = poGeomIn->clone();
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRNTFRasterLayer::ResetReading()

{
    iCurrentFC = 0;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRNTFRasterLayer::GetNextFeature()

{
    return GetFeature( (long) ++iCurrentFC );
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRNTFRasterLayer::GetFeature( long nFeatureId )

{
    int		iReqColumn, iReqRow;
    
/* -------------------------------------------------------------------- */
/*      Is this in the range of legal feature ids (pixels)?             */
/* -------------------------------------------------------------------- */
    if( nFeatureId < 1
        || nFeatureId > poReader->GetRasterXSize()*poReader->GetRasterYSize() )
    {
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Do we need to load a different column.                          */
/* -------------------------------------------------------------------- */
    iReqColumn = (nFeatureId - 1) / poReader->GetRasterYSize();
    iReqRow = nFeatureId - iReqColumn * poReader->GetRasterXSize() - 1;
    
    if( iReqColumn != iColumnOffset )
    {
        iColumnOffset = iReqColumn;
        if( poReader->ReadRasterColumn( iReqColumn, pafColumn ) != CE_None )
            return NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Create a corresponding feature.                                 */
/* -------------------------------------------------------------------- */
    OGRFeature	*poFeature = new OGRFeature( poFeatureDefn );
    double	*padfGeoTransform = poReader->GetGeoTransform();

    poFeature->SetFID( nFeatureId );
    poFeature->SetGeometryDirectly(
        new OGRPoint( padfGeoTransform[0] + padfGeoTransform[1] * iReqColumn,
                      padfGeoTransform[3] + padfGeoTransform[5] * iReqRow,
                      pafColumn[iReqRow] ) );
    poFeature->SetField( 0, (int) pafColumn[iReqRow] );
    
    return poFeature;
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/*                                                                      */
/*      If a spatial filter is in effect, we turn control over to       */
/*      the generic counter.  Otherwise we return the total count.      */
/*      Eventually we should consider implementing a more efficient     */
/*      way of counting features matching a spatial query.              */
/************************************************************************/

int OGRNTFRasterLayer::GetFeatureCount( int bForce )

{
    return poReader->GetRasterXSize() * poReader->GetRasterYSize();
}

/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference *OGRNTFRasterLayer::GetSpatialRef()

{
    return poDS->GetSpatialRef();
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRNTFRasterLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return TRUE;

    else if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return TRUE;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else 
        return FALSE;
}


