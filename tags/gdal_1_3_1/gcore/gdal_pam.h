/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Declaration for Peristable Auxilary Metadata classes.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2005, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.6  2005/09/24 19:02:15  fwarmerdam
 * added RasterAttributeTable support
 *
 * Revision 1.5  2005/09/11 18:03:34  fwarmerdam
 * added Clear() method on multidomainmetadata
 *
 * Revision 1.4  2005/05/22 08:14:06  fwarmerdam
 * added multidomain metadata support
 *
 * Revision 1.3  2005/05/13 18:19:04  fwarmerdam
 * Added SetDefaultHistogram
 *
 * Revision 1.2  2005/05/11 14:04:21  fwarmerdam
 * added getdefaulthistogram
 *
 * Revision 1.1  2005/04/27 16:27:44  fwarmerdam
 * New
 *
 */

#ifndef GDAL_PAM_H_INCLUDED
#define GDAL_PAM_H_INCLUDED

#include "gdal_priv.h"
#include "cpl_minixml.h"

class GDALPamRasterBand;

/* Clone Info Flags */

#define GCIF_GEOTRANSFORM       0x01
#define GCIF_PROJECTION         0x02
#define GCIF_METADATA           0x04
#define GCIF_GCPS               0x08

#define GCIF_NODATA             0x001000
#define GCIF_CATEGORYNAMES      0x002000
#define GCIF_MINMAX             0x004000
#define GCIF_SCALEOFFSET        0x008000
#define GCIF_UNITTYPE           0x010000
#define GCIF_COLORTABLE         0x020000
#define GCIF_COLORINTERP        0x020000
#define GCIF_BAND_METADATA      0x040000
#define GCIF_RAT                0x080000

#define GCIF_ONLY_IF_MISSING    0x10000000
#define GCIF_PROCESS_BANDS      0x20000000

#define GCIF_PAM_DEFAULT        (GCIF_GEOTRANSFORM | GCIF_PROJECTION |     \
                                 GCIF_METADATA | GCIF_GCPS |               \
                                 GCIF_NODATA | GCIF_CATEGORYNAMES |        \
                                 GCIF_MINMAX | GCIF_SCALEOFFSET |          \
                                 GCIF_UNITTYPE | GCIF_COLORTABLE |         \
                                 GCIF_COLORINTERP | GCIF_BAND_METADATA |   \
                                 GCIF_RAT |                                \
                                 GCIF_ONLY_IF_MISSING | GCIF_PROCESS_BANDS )

/* GDAL PAM Flags */
#define GPF_DIRTY		0x01  // .pam file needs to be written on close
#define GPF_TRIED_READ_FAILED   0x02  // no need to keep trying to read .pam.
#define GPF_DISABLED            0x04  // do not try any PAM stuff. 
#define GPF_AUXMODE             0x08  // store info in .aux (HFA) file.

/************************************************************************/
/*                       GDALMultiDomainMetadata                        */
/************************************************************************/

class CPL_DLL GDALMultiDomainMetadata
{
private:
    char **papszDomainList;
    char ***papapszMetadataLists;

public:
    GDALMultiDomainMetadata();
    ~GDALMultiDomainMetadata();

    int         XMLInit( CPLXMLNode *psMetadata );
    CPLXMLNode  *Serialize();

    char      **GetDomainList() { return papszDomainList; }

    char      **GetMetadata( const char * pszDomain = "" );
    CPLErr      SetMetadata( char ** papszMetadata,
                             const char * pszDomain = "" );
    const char *GetMetadataItem( const char * pszName,
                                 const char * pszDomain = "" );
    CPLErr      SetMetadataItem( const char * pszName,
                                 const char * pszValue,
                                 const char * pszDomain = "" );

    void        Clear();
};

/* ==================================================================== */
/*      GDALDatasetPamInfo                                              */
/*                                                                      */
/*      We make these things a seperate structure of information        */
/*      primarily so we can modify it without altering the size of      */
/*      the GDALPamDataset.  It is an effort to reduce ABI churn for    */
/*      driver plugins.                                                 */
/* ==================================================================== */
typedef struct {
    char       *pszPamFilename;

    char	*pszProjection;

    int         bHaveGeoTransform;
    double      adfGeoTransform[6];

    int         nGCPCount;
    GDAL_GCP   *pasGCPList;
    char       *pszGCPProjection;

    CPLXMLNode *psHistograms;

    GDALMultiDomainMetadata  oMDMD;

} GDALDatasetPamInfo;

/* ******************************************************************** */
/*                           GDALPamDataset                             */
/* ******************************************************************** */

class CPL_DLL GDALPamDataset : public GDALDataset
{
    friend class GDALPamRasterBand;

  protected:
                GDALPamDataset(void);

    int         nPamFlags;
    GDALDatasetPamInfo *psPam;

    virtual CPLXMLNode *SerializeToXML( const char *);
    virtual CPLErr      XMLInit( CPLXMLNode *, const char * );
    
    virtual CPLErr TryLoadXML();
    virtual CPLErr TrySaveXML();

    CPLErr  TryLoadAux();
    CPLErr  TrySaveAux();

    virtual const char *BuildPamFilename();

    void   PamInitialize();
    void   PamClear();

  public:
    virtual     ~GDALPamDataset();

    virtual void FlushCache(void);

    virtual const char *GetProjectionRef(void);
    virtual CPLErr SetProjection( const char * );

    virtual CPLErr GetGeoTransform( double * );
    virtual CPLErr SetGeoTransform( double * );

    virtual int    GetGCPCount();
    virtual const char *GetGCPProjection();
    virtual const GDAL_GCP *GetGCPs();
    virtual CPLErr SetGCPs( int nGCPCount, const GDAL_GCP *pasGCPList,
                            const char *pszGCPProjection );

    virtual char      **GetMetadata( const char * pszDomain = "" );
    virtual CPLErr      SetMetadata( char ** papszMetadata,
                                     const char * pszDomain = "" );
    virtual const char *GetMetadataItem( const char * pszName,
                                 const char * pszDomain = "" );
    virtual CPLErr      SetMetadataItem( const char * pszName,
                                         const char * pszValue,
                                         const char * pszDomain = "" );

    virtual CPLErr CloneInfo( GDALDataset *poSrcDS, int nCloneInfoFlags );


    // "semi private" methods.
    void   MarkPamDirty() { nPamFlags |= GPF_DIRTY; }
    GDALDatasetPamInfo *GetPamInfo() { return psPam; }
};

/* ==================================================================== */
/*      GDALDatasetPamInfo                                              */
/*                                                                      */
/*      We make these things a seperate structure of information        */
/*      primarily so we can modify it without altering the size of      */
/*      the GDALPamDataset.  It is an effort to reduce ABI churn for    */
/*      driver plugins.                                                 */
/* ==================================================================== */
typedef struct {
    GDALPamDataset *poParentDS;

    int            bNoDataValueSet;
    double         dfNoDataValue;

    GDALColorTable *poColorTable;

    GDALColorInterp eColorInterp;

    char           *pszUnitType;
    char           **papszCategoryNames;
    
    double         dfOffset;
    double         dfScale;

    int            bHaveMinMax;
    double         dfMin;
    double         dfMax;

    int            bHaveStats;
    double         dfMean;
    double         dfStdDev;

    CPLXMLNode     *psSavedHistograms;

    GDALMultiDomainMetadata  oMDMD;

    GDALRasterAttributeTable *poDefaultRAT;

} GDALRasterBandPamInfo;

/* ******************************************************************** */
/*                          GDALPamRasterBand                           */
/* ******************************************************************** */
class CPL_DLL GDALPamRasterBand : public GDALRasterBand
{
    friend class GDALPamDataset;

  protected:

    virtual CPLXMLNode *SerializeToXML( const char *pszVRTPath );
    virtual CPLErr      XMLInit( CPLXMLNode *, const char * );
    
    void   PamInitialize();
    void   PamClear();

    GDALRasterBandPamInfo *psPam;

  public:
                GDALPamRasterBand();
    virtual     ~GDALPamRasterBand();

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

    virtual CPLErr  GetHistogram( double dfMin, double dfMax,
                          int nBuckets, int * panHistogram,
                          int bIncludeOutOfRange, int bApproxOK,
                          GDALProgressFunc, void *pProgressData );

    virtual CPLErr GetDefaultHistogram( double *pdfMin, double *pdfMax,
                                        int *pnBuckets, int ** ppanHistogram,
                                        int bForce,
                                        GDALProgressFunc, void *pProgressData);

    virtual CPLErr SetDefaultHistogram( double dfMin, double dfMax,
                                        int nBuckets, int *panHistogram );

    virtual char      **GetMetadata( const char * pszDomain = "" );
    virtual CPLErr      SetMetadata( char ** papszMetadata,
                                     const char * pszDomain = "" );
    virtual const char *GetMetadataItem( const char * pszName,
                                 const char * pszDomain = "" );
    virtual CPLErr      SetMetadataItem( const char * pszName,
                                         const char * pszValue,
                                         const char * pszDomain = "" );

    virtual const GDALRasterAttributeTable *GetDefaultRAT();
    virtual CPLErr SetDefaultRAT( const GDALRasterAttributeTable * );

    // new in GDALPamRasterBand. 
    virtual CPLErr CloneInfo( GDALRasterBand *poSrcBand, int nCloneInfoFlags );

    // "semi private" methods.
    GDALRasterBandPamInfo *GetPamInfo() { return psPam; }
};

// These are mainly helper functions for internal use.
int CPL_DLL PamApplyMetadata( CPLXMLNode *psTree, GDALMajorObject *poMO );
CPLXMLNode CPL_DLL *PamSerializeMetadata( GDALMajorObject *poMO );

#endif /* ndef GDAL_PAM_H_INCLUDED */
