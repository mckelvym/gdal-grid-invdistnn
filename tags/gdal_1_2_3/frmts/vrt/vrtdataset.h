/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Declaration of virtual gdal dataset classes.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
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
 * Revision 1.18  2004/08/12 08:24:26  warmerda
 * added overview support
 *
 * Revision 1.17  2004/08/11 18:51:48  warmerda
 * added warped dataset support
 *
 * Revision 1.16  2004/07/30 21:51:00  warmerda
 * added support for VRTRawRasterBand
 *
 * Revision 1.15  2004/07/28 17:47:33  warmerda
 * Disable VRTWarpedRasterBand for now.
 *
 * Revision 1.14  2004/07/28 16:56:02  warmerda
 * added VRTSourcedRasterBand
 *
 * Revision 1.13  2004/04/15 18:54:38  warmerda
 * added UnitType, Offset, Scale and CategoryNames support
 *
 * Revision 1.12  2004/03/16 18:34:35  warmerda
 * added support for relativeToVRT attribute on SourceFilename
 *
 * Revision 1.11  2003/09/11 23:00:04  aamici
 * add class constructors and destructors where needed in order to
 * let the mingw/cygwin binutils produce sensible partially linked objet files
 * with 'ld -r'.
 *
 * Revision 1.10  2003/08/07 17:11:21  warmerda
 * added normalized flag for kernel based filters
 *
 * Revision 1.9  2003/07/17 20:30:24  warmerda
 * Added custom VRTDriver and moved all the sources class declarations in here.
 *
 * Revision 1.8  2003/06/10 19:59:33  warmerda
 * added new Func based source type for passthrough to a callback
 *
 * Revision 1.7  2003/03/13 20:38:30  dron
 * bNoDataValueSet added to VRTRasterBand class.
 *
 * Revision 1.6  2002/11/30 16:55:49  warmerda
 * added OpenXML method
 *
 * Revision 1.5  2002/11/24 04:29:02  warmerda
 * Substantially rewrote VRTSimpleSource.  Now VRTSource is base class, and
 * sources do their own SerializeToXML(), and XMLInit().  New VRTComplexSource
 * supports scaling and nodata values.
 *
 * Revision 1.4  2002/05/29 18:13:44  warmerda
 * added nodata handling for averager
 *
 * Revision 1.3  2002/05/29 16:06:05  warmerda
 * complete detailed band metadata
 *
 * Revision 1.2  2001/11/18 15:46:45  warmerda
 * added SRS and GeoTransform
 *
 * Revision 1.1  2001/11/16 21:14:31  warmerda
 * New
 *
 */

#ifndef VIRTUALDATASET_H_INCLUDED
#define VIRTUALDATASET_H_INCLUDED

#include "gdal_priv.h"
#include "cpl_minixml.h"

CPL_C_START
void	GDALRegister_VRT(void);
typedef CPLErr
(*VRTImageReadFunc)( void *hCBData,
                     int nXOff, int nYOff, int nXSize, int nYSize,
                     void *pData );
CPL_C_END

int VRTApplyMetadata( CPLXMLNode *, GDALMajorObject * );
CPLXMLNode *VRTSerializeMetadata( GDALMajorObject * );

/************************************************************************/
/*                              VRTSource                               */
/************************************************************************/

class VRTSource 
{
public:
    virtual ~VRTSource();

    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace ) = 0;

    virtual CPLErr  XMLInit( CPLXMLNode *psTree, const char * ) = 0;
    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath ) = 0;
};

typedef VRTSource *(*VRTSourceParser)(CPLXMLNode *, const char *);

VRTSource *VRTParseCoreSources( CPLXMLNode *psTree, const char * );
VRTSource *VRTParseFilterSources( CPLXMLNode *psTree, const char * );

/************************************************************************/
/*                              VRTDataset                              */
/************************************************************************/

class CPL_DLL VRTDataset : public GDALDataset
{
    char           *pszProjection;

    int            bGeoTransformSet;
    double         adfGeoTransform[6];

    int           nGCPCount;
    GDAL_GCP      *pasGCPList;
    char          *pszGCPProjection;

    int            bNeedsFlush;
    
    char          *pszVRTPath;

  public:
                 VRTDataset(int nXSize, int nYSize);
                ~VRTDataset();

    void          SetNeedsFlush() { bNeedsFlush = TRUE; }
    virtual void  FlushCache();

    virtual const char *GetProjectionRef(void);
    virtual CPLErr SetProjection( const char * );
    virtual CPLErr GetGeoTransform( double * );
    virtual CPLErr SetGeoTransform( double * );

    virtual int    GetGCPCount();
    virtual const char *GetGCPProjection();
    virtual const GDAL_GCP *GetGCPs();
    virtual CPLErr SetGCPs( int nGCPCount, const GDAL_GCP *pasGCPList,
                            const char *pszGCPProjection );

    virtual CPLErr AddBand( GDALDataType eType, 
                            char **papszOptions=NULL );

    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath);
    virtual CPLErr      XMLInit( CPLXMLNode *, const char * );
 
    static GDALDataset *Open( GDALOpenInfo * );
    static GDALDataset *OpenXML( const char *, const char * );
    static GDALDataset *Create( const char * pszName,
                                int nXSize, int nYSize, int nBands,
                                GDALDataType eType, char ** papszOptions );
};

/************************************************************************/
/*                           VRTWarpedDataset                           */
/************************************************************************/

class GDALWarpOperation;
class VRTWarpedRasterBand;

class CPL_DLL VRTWarpedDataset : public VRTDataset
{
    int               nBlockXSize;
    int               nBlockYSize;
    GDALWarpOperation *poWarper;

public:
    int               nOverviewCount;
    VRTWarpedDataset  **papoOverviews;

public:
                      VRTWarpedDataset( int nXSize, int nYSize );
                     ~VRTWarpedDataset();

    CPLErr            Initialize( /* GDALWarpOptions */ void * );

    virtual CPLErr IBuildOverviews( const char *, int, int *,
                                    int, int *, GDALProgressFunc, void * );
    
    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath );
    virtual CPLErr    XMLInit( CPLXMLNode *, const char * );

    virtual CPLErr AddBand( GDALDataType eType, 
                            char **papszOptions=NULL );
    
    CPLErr            ProcessBlock( int iBlockX, int iBlockY );

    void              GetBlockSize( int *, int * );
};

/************************************************************************/
/*                            VRTRasterBand                             */
/*                                                                      */
/*      Provides support for all the various kinds of metadata but      */
/*      no raster access.  That is handled by derived classes.          */
/************************************************************************/

class CPL_DLL VRTRasterBand : public GDALRasterBand
{
  protected:
    int            bNoDataValueSet;
    double         dfNoDataValue;

    GDALColorTable *poColorTable;

    GDALColorInterp eColorInterp;

    char           *pszUnitType;
    char           **papszCategoryNames;
    
    double         dfOffset;
    double         dfScale;

    void           Initialize( int nXSize, int nYSize );

  public:

    		   VRTRasterBand();
    virtual        ~VRTRasterBand();

    virtual CPLErr         XMLInit( CPLXMLNode *, const char * );
    virtual CPLXMLNode *   SerializeToXML( const char *pszVRTPath );

#define VRT_NODATA_UNSET -1234.56

    virtual CPLErr SetNoDataValue( double );
    virtual double GetNoDataValue( int *pbSuccess = NULL );

    virtual CPLErr SetColorTable( GDALColorTable * ); 
    virtual GDALColorTable *GetColorTable();

    virtual CPLErr SetColorInterpretation( GDALColorInterp );
    virtual GDALColorInterp GetColorInterpretation();

    virtual const char *GetUnitType();
    CPLErr SetUnitType( const char * ); 

    virtual char **GetCategoryNames();
    virtual CPLErr SetCategoryNames( char ** );

    virtual double GetOffset( int *pbSuccess = NULL );
    CPLErr SetOffset( double );
    virtual double GetScale( int *pbSuccess = NULL );
    CPLErr SetScale( double );
    
    CPLErr         CopyCommonInfoFrom( GDALRasterBand * );
};

/************************************************************************/
/*                         VRTSourcedRasterBand                         */
/************************************************************************/

class CPL_DLL VRTSourcedRasterBand : public VRTRasterBand
{
    int		   nSources;
    VRTSource    **papoSources;

    int            bEqualAreas;

    void           Initialize( int nXSize, int nYSize );

    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
                              void *, int, int, GDALDataType,
                              int, int );
  public:

    		   VRTSourcedRasterBand( GDALDataset *poDS, int nBand );
                   VRTSourcedRasterBand( GDALDataType eType, 
                                         int nXSize, int nYSize );
                   VRTSourcedRasterBand( GDALDataset *poDS, int nBand, 
                                         GDALDataType eType, 
                                         int nXSize, int nYSize );
    virtual        ~VRTSourcedRasterBand();

    virtual char      **GetMetadata( const char * pszDomain = "" );
    virtual CPLErr      SetMetadata( char ** papszMetadata,
                                     const char * pszDomain = "" );

    virtual CPLErr         XMLInit( CPLXMLNode *, const char * );
    virtual CPLXMLNode *   SerializeToXML( const char *pszVRTPath );

    CPLErr         AddSource( VRTSource * );
    CPLErr         AddSimpleSource( GDALRasterBand *poSrcBand, 
                                    int nSrcXOff=-1, int nSrcYOff=-1, 
                                    int nSrcXSize=-1, int nSrcYSize=-1, 
                                    int nDstXOff=-1, int nDstYOff=-1, 
                                    int nDstXSize=-1, int nDstYSize=-1,
                                    const char *pszResampling = "near",
                                    double dfNoDataValue = VRT_NODATA_UNSET);
    CPLErr         AddComplexSource( GDALRasterBand *poSrcBand, 
                                     int nSrcXOff=-1, int nSrcYOff=-1, 
                                     int nSrcXSize=-1, int nSrcYSize=-1, 
                                     int nDstXOff=-1, int nDstYOff=-1, 
                                     int nDstXSize=-1, int nDstYSize=-1,
                                     double dfScaleOff=0.0, 
                                     double dfScaleRatio=1.0,
                                     double dfNoDataValue = VRT_NODATA_UNSET);

    CPLErr         AddFuncSource( VRTImageReadFunc pfnReadFunc, void *hCBData,
                                  double dfNoDataValue = VRT_NODATA_UNSET );


    virtual CPLErr IReadBlock( int, int, void * );
};

/************************************************************************/
/*                         VRTWarpedRasterBand                          */
/************************************************************************/

class CPL_DLL VRTWarpedRasterBand : public VRTRasterBand
{
  public:
                   VRTWarpedRasterBand( GDALDataset *poDS, int nBand,
                                     GDALDataType eType = GDT_Unknown );
    virtual        ~VRTWarpedRasterBand();

    virtual CPLErr         XMLInit( CPLXMLNode *, const char * );
    virtual CPLXMLNode *   SerializeToXML( const char *pszVRTPath );

    virtual CPLErr IReadBlock( int, int, void * );

    virtual int GetOverviewCount();
    virtual GDALRasterBand *GetOverview(int);
};

/************************************************************************/
/*                           VRTRawRasterBand                           */
/************************************************************************/

class RawRasterBand;

class CPL_DLL VRTRawRasterBand : public VRTRasterBand
{
    RawRasterBand  *poRawRaster;

    char           *pszSourceFilename;
    int            bRelativeToVRT;

  public:
                   VRTRawRasterBand( GDALDataset *poDS, int nBand,
                                     GDALDataType eType = GDT_Unknown );
    virtual        ~VRTRawRasterBand();

    virtual CPLErr         XMLInit( CPLXMLNode *, const char * );
    virtual CPLXMLNode *   SerializeToXML( const char *pszVRTPath );

    virtual CPLErr IRasterIO( GDALRWFlag, int, int, int, int,
                              void *, int, int, GDALDataType,
                              int, int );

    virtual CPLErr IReadBlock( int, int, void * );
    virtual CPLErr IWriteBlock( int, int, void * );

    CPLErr         SetRawLink( const char *pszFilename, 
                               const char *pszVRTPath,
                               int bRelativeToVRT, 
                               vsi_l_offset nImageOffset, 
                               int nPixelOffset, int nLineOffset, 
                               const char *pszByteOrder );

    void           ClearRawLink();

};

/************************************************************************/
/*                              VRTDriver                               */
/************************************************************************/

class VRTDriver : public GDALDriver
{
  public:
                 VRTDriver();
                 ~VRTDriver();

    char         **papszSourceParsers;

    virtual char      **GetMetadata( const char * pszDomain = "" );
    virtual CPLErr      SetMetadata( char ** papszMetadata,
                                     const char * pszDomain = "" );

    VRTSource   *ParseSource( CPLXMLNode *psSrc, const char *pszVRTPath );
    void         AddSourceParser( const char *pszElementName, 
                                  VRTSourceParser pfnParser );
};

/************************************************************************/
/*                           VRTSimpleSource                            */
/************************************************************************/

class VRTSimpleSource : public VRTSource
{
protected:
    GDALRasterBand      *poRasterBand;

    int                 nSrcXOff;
    int                 nSrcYOff;
    int                 nSrcXSize;
    int                 nSrcYSize;

    int                 nDstXOff;
    int                 nDstYOff;
    int                 nDstXSize;
    int                 nDstYSize;

    int                 bNoDataSet;
    double              dfNoDataValue;

public:
            VRTSimpleSource();
    virtual ~VRTSimpleSource();

    virtual CPLErr  XMLInit( CPLXMLNode *psTree, const char * );
    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath );

    void           SetSrcBand( GDALRasterBand * );
    void           SetSrcWindow( int, int, int, int );
    void           SetDstWindow( int, int, int, int );
    void           SetNoDataValue( double dfNoDataValue );

    int            GetSrcDstWindow( int, int, int, int, int, int, 
                                    int *, int *, int *, int *,
                                    int *, int *, int *, int * );

    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace );

    void            DstToSrc( double dfX, double dfY,
                              double &dfXOut, double &dfYOut );
    void            SrcToDst( double dfX, double dfY,
                              double &dfXOut, double &dfYOut );

};

/************************************************************************/
/*                          VRTAveragedSource                           */
/************************************************************************/

class VRTAveragedSource : public VRTSimpleSource
{
public:
                    VRTAveragedSource();
    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace );
    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath );
};

/************************************************************************/
/*                           VRTComplexSource                           */
/************************************************************************/

class VRTComplexSource : public VRTSimpleSource
{
public:
                   VRTComplexSource();
    virtual        ~VRTComplexSource();

    virtual CPLErr RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                             void *pData, int nBufXSize, int nBufYSize, 
                             GDALDataType eBufType, 
                             int nPixelSpace, int nLineSpace );
    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath );
    virtual CPLErr XMLInit( CPLXMLNode *, const char * );

    int		   bDoScaling;
    double	   dfScaleOff;
    double         dfScaleRatio;

};

/************************************************************************/
/*                           VRTFilteredSource                          */
/************************************************************************/

class VRTFilteredSource : public VRTComplexSource
{
private:
    int          IsTypeSupported( GDALDataType eType );

protected:
    int          nSupportedTypesCount;
    GDALDataType aeSupportedTypes[20];

    int          nExtraEdgePixels;

public:
            VRTFilteredSource();
    virtual ~VRTFilteredSource();

    void    SetExtraEdgePixels( int );
    void    SetFilteringDataTypesSupported( int, GDALDataType * );

    virtual CPLErr  FilterData( int nXSize, int nYSize, GDALDataType eType, 
                                GByte *pabySrcData, GByte *pabyDstData ) = 0;

    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace );
};

/************************************************************************/
/*                       VRTKernelFilteredSource                        */
/************************************************************************/

class VRTKernelFilteredSource : public VRTFilteredSource
{
protected:
    int     nKernelSize;

    double  *padfKernelCoefs;

    int     bNormalized;

public:
            VRTKernelFilteredSource();
    virtual ~VRTKernelFilteredSource();

    virtual CPLErr  XMLInit( CPLXMLNode *psTree, const char * );
    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath );

    virtual CPLErr  FilterData( int nXSize, int nYSize, GDALDataType eType, 
                                GByte *pabySrcData, GByte *pabyDstData );

    CPLErr          SetKernel( int nKernelSize, double *padfCoefs );
    void            SetNormalized( int );
};

/************************************************************************/
/*                       VRTAverageFilteredSource                       */
/************************************************************************/

class VRTAverageFilteredSource : public VRTKernelFilteredSource
{
public:
            VRTAverageFilteredSource( int nKernelSize );
    virtual ~VRTAverageFilteredSource();

    virtual CPLErr  XMLInit( CPLXMLNode *psTree, const char * );
    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath );
};

/************************************************************************/
/*                            VRTFuncSource                             */
/************************************************************************/
class VRTFuncSource : public VRTSource
{
public:
            VRTFuncSource();
    virtual ~VRTFuncSource();

    virtual CPLErr  XMLInit( CPLXMLNode *, const char *) { return CE_Failure; }
    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath );

    virtual CPLErr  RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                              void *pData, int nBufXSize, int nBufYSize, 
                              GDALDataType eBufType, 
                              int nPixelSpace, int nLineSpace );

    VRTImageReadFunc    pfnReadFunc;
    void               *pCBData;
    GDALDataType        eType;
    
    float               fNoDataValue;
};

#endif /* ndef VIRTUALDATASET_H_INCLUDED */
