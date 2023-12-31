/**********************************************************************
 * $Id: mitab.h,v 1.73 2004/07/07 22:18:02 dmorissette Exp $
 *
 * Name:     mitab.h
 * Project:  MapInfo TAB Read/Write library
 * Language: C++
 * Purpose:  Header file containing public definitions for the library.
 * Author:   Daniel Morissette, dmorissette@dmsolutions.ca
 *
 **********************************************************************
 * Copyright (c) 1999-2004, Daniel Morissette
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log: mitab.h,v $
 * Revision 1.73  2004/07/07 22:18:02  dmorissette
 * Updated 1.3.0 release date
 *
 * Revision 1.72  2004/06/30 20:22:31  dmorissette
 * Ready for V1.3.0
 *
 * Revision 1.71  2003/08/07 03:20:46  dmorissette
 * Added mitab_c_getlibversion() to C API. (Uffe K. - bug 21)
 *
 * Revision 1.70  2003/07/24 02:47:58  daniel
 * Version 1.2.4
 *
 * Revision 1.69  2002/10/15 23:30:49  daniel
 * Version 1.2.3
 *
 * Revision 1.68  2002/07/06 15:36:57  daniel
 * Version 1.2.2
 *
 * Revision 1.67  2002/06/28 18:39:10  julien
 * Change version number (1.2.2-dev)
 *
 * Revision 1.66  2002/06/28 18:32:37  julien
 * Add SetSpatialFilter() in TABSeamless class (Bug 164, MapServer)
 * Use double for comparison in Coordsys2Int() in mitab_mapheaderblock.cpp
 *
 * Revision 1.65  2002/06/17 15:00:30  julien
 * Add IsInteriorRing() function in TABRegion to validate if a ring is internal
 *
 * Revision 1.64  2002/05/08 20:02:29  daniel
 * Version 1.2.1
 *
 * Revision 1.63  2002/05/08 15:10:48  julien
 * Implement MIFFile::SetMIFCoordSys in mitab_capi.cpp (Bug 984)
 *
 * Revision 1.62  2002/05/03 15:09:41  daniel
 * Version 1.2.0
 *
 * Revision 1.61  2002/03/26 19:27:43  daniel
 * Got rid of tabs in source
 *
 * Revision 1.60  2002/03/26 03:17:13  daniel
 * Added Get/SetCenter() to MultiPoint
 *
 * Revision 1.59  2002/03/26 01:48:40  daniel
 * Added Multipoint object type (V650)
 *
 * Revision 1.58  2001/11/17 21:54:05  daniel
 * Made several changes in order to support writing objects in 16 bits 
 * coordinate format. New TABMAPObjHdr-derived classes are used to hold 
 * object info in mem until block is full.
 *
 * Revision 1.57  2001/11/02 17:27:21  daniel
 * Version 1.1.3
 *
 * Revision 1.56  2001/09/19 21:39:15  warmerda
 * get extents efficiently
 *
 * Revision 1.55  2001/09/19 14:31:22  warmerda
 * added m_nPreloadedId to keep track of preloaded line
 *
 * Revision 1.54  2001/09/14 03:23:55  warmerda
 * Substantial upgrade to support spatial queries using spatial indexes
 *
 * Revision 1.53  2001/06/25 01:51:19  daniel
 * Version 1.1.2
 *
 * Revision 1.52  2001/05/01 18:36:10  daniel
 * Version 1.1.1
 *
 * Revision 1.51  2001/03/15 03:57:51  daniel
 * Added implementation for new OGRLayer::GetExtent(), returning data MBR.
 *
 * Revision 1.50  2001/03/09 04:16:02  daniel
 * Added TABSeamless for reading seamless TAB files
 *
 * Revision 1.49  2001/02/28 07:15:08  daniel
 * Added support for text label line end point
 *
 * Revision 1.48  2001/02/27 19:59:05  daniel
 * Enabled spatial filter in IMapInfoFile::GetNextFeature(), and avoid
 * unnecessary feature cloning in GetNextFeature() and GetFeature()
 *
 * Revision 1.47  2001/01/23 22:06:50  daniel
 * Added MITABCoordSysTableLoaded()
 *
 * Revision 1.46  2001/01/23 21:23:41  daniel
 * Added projection bounds lookup table, called from TABFile::SetProjInfo()
 *
 * Revision 1.45  2001/01/22 16:03:59  warmerda
 * expanded tabs
 *
 * Revision 1.44  2000/11/23 20:47:45  daniel
 * Use MI defaults for Pen, Brush, Font, Symbol instead of all zeros
 *
 * Revision 1.43  2000/11/22 04:04:04  daniel
 * Added TAB_WarningBoundsOverflow
 *
 * Revision 1.42  2000/11/15 04:35:35  daniel
 * MITAB_VERSION 1.0.4
 *
 * Revision 1.41  2000/10/19 20:15:41  daniel
 * Update MITAB_VERSION to 1.0.3
 *
 * Revision 1.40  2000/10/03 22:11:43  daniel
 * Added MITAB_VERSION
 *
 * Revision 1.39  2000/10/03 19:29:51  daniel
 * Include OGR StyleString stuff (implemented by Stephane)
 *
 * Revision 1.38  2000/09/19 17:23:52  daniel
 * Maintain and/or compute valid region and polyline center/label point
 *
 * Revision 1.37  2000/09/07 23:32:13  daniel
 * Added RecordDeletedFlag to TABFeature with get/set methods
 *
 * Revision 1.36  2000/07/27 02:03:57  daniel
 * Remove extra comma at end of TABCustSymbStyle enum
 *
 * Revision 1.35  2000/07/04 01:45:16  warmerda
 * avoid warning on nfieldid of IsFieldUnique
 *
 * Revision 1.34  2000/06/28 00:30:25  warmerda
 * added count of points, lines, egions and text for MIFFile
 *
 * Revision 1.33  2000/04/21 12:40:01  daniel
 * Added TABPolyline::GetNumParts()/GetPartRef()
 *
 * Revision 1.32  2000/02/28 16:41:48  daniel
 * Added support for indexed, unique, and for new V450 object types
 *
 * ...
 *
 * Revision 1.1  1999/07/12 04:18:23  daniel
 * Initial checkin
 *
 **********************************************************************/

#ifndef _MITAB_H_INCLUDED_
#define _MITAB_H_INCLUDED_

#include "mitab_priv.h"
#include "ogr_feature.h"
#include "ogrsf_frmts.h"

/*---------------------------------------------------------------------
 * Current version of the MITAB library... always useful!
 *--------------------------------------------------------------------*/
#define MITAB_VERSION      "1.3.0 (2004-07-07)"
#define MITAB_VERSION_INT  1003000  /* version x.y.z -> xxxyyyzzz */

#ifndef PI
#  define PI 3.14159265358979323846
#endif

#ifndef ROUND_INT
#  define ROUND_INT(dX) ((int)((dX) < 0.0 ? (dX)-0.5 : (dX)+0.5 ))
#endif

class TABFeature;

/*---------------------------------------------------------------------
 * Codes for the GetFileClass() in the IMapInfoFile-derived  classes
 *--------------------------------------------------------------------*/
typedef enum
{
    TABFC_IMapInfoFile = 0,
    TABFC_TABFile,
    TABFC_TABView,
    TABFC_TABSeamless,
    TABFC_MIFFile
} TABFileClass;


/*---------------------------------------------------------------------
 *                      class IMapInfoFile
 *
 * Virtual base class for the TABFile and MIFFile classes.
 *
 * This is the definition of the public interface methods that should
 * be available for any type of MapInfo dataset.
 *--------------------------------------------------------------------*/

class IMapInfoFile : public OGRLayer
{
  private:

  protected: 
    int                 m_nCurFeatureId;
    TABFeature         *m_poCurFeature;
    GBool               m_bBoundsSet;

  public:
    IMapInfoFile() ;
    virtual ~IMapInfoFile();

    virtual TABFileClass GetFileClass() {return TABFC_IMapInfoFile;}

    virtual int Open(const char *pszFname, const char *pszAccess,
                     GBool bTestOpenNoError = FALSE ) = 0;
    virtual int Close() = 0;

    virtual const char *GetTableName() = 0;

    ///////////////
    // Static method to detect file type, create an object to read that
    // file and open it.
    static IMapInfoFile *SmartOpen(const char *pszFname,
                                   GBool bTestOpenNoError = FALSE);

    ///////////////
    //  OGR methods for read support
    virtual void        ResetReading() = 0;
    virtual int         GetFeatureCount (int bForce) = 0;
    virtual OGRFeature *GetNextFeature();
    virtual OGRFeature *GetFeature(long nFeatureId);
    virtual OGRErr      CreateFeature(OGRFeature *poFeature);
    virtual int         TestCapability( const char * pszCap ) =0;
    virtual int         GetExtent(OGREnvelope *psExtent, int bForce) =0;

    ///////////////
    // Read access specific stuff
    //
    virtual int GetNextFeatureId(int nPrevId) = 0;
    virtual TABFeature *GetFeatureRef(int nFeatureId) = 0;
    virtual OGRFeatureDefn *GetLayerDefn() = 0;

    virtual TABFieldType GetNativeFieldType(int nFieldId) = 0;

    virtual int GetBounds(double &dXMin, double &dYMin, 
                          double &dXMax, double &dYMax,
                          GBool bForce = TRUE ) = 0;
    
    virtual OGRSpatialReference *GetSpatialRef() = 0;

    virtual int GetFeatureCountByType(int &numPoints, int &numLines,
                                      int &numRegions, int &numTexts,
                                      GBool bForce = TRUE ) = 0;

    virtual GBool IsFieldIndexed(int nFieldId) = 0;
    virtual GBool IsFieldUnique(int nFieldId) = 0;

    ///////////////
    // Write access specific stuff
    //
    GBool       IsBoundsSet()            {return m_bBoundsSet;}
    virtual int SetBounds(double dXMin, double dYMin, 
                          double dXMax, double dYMax) = 0;
    virtual int SetFeatureDefn(OGRFeatureDefn *poFeatureDefn,
                            TABFieldType *paeMapInfoNativeFieldTypes = NULL)=0;
    virtual int AddFieldNative(const char *pszName, TABFieldType eMapInfoType,
                               int nWidth=0, int nPrecision=0,
                               GBool bIndexed=FALSE, GBool bUnique=FALSE) = 0;
    virtual OGRErr CreateField( OGRFieldDefn *poField, int bApproxOK = TRUE );
    
    virtual int SetSpatialRef(OGRSpatialReference *poSpatialRef) = 0;

    virtual int SetFeature(TABFeature *poFeature, int nFeatureId = -1) = 0;

    virtual int SetFieldIndexed(int nFieldId) = 0;

    ///////////////
    // semi-private.
    virtual int  GetProjInfo(TABProjInfo *poPI) = 0;
    virtual int  SetProjInfo(TABProjInfo *poPI) = 0;
    virtual int  SetMIFCoordSys(const char *pszMIFCoordSys) = 0;

#ifdef DEBUG
    virtual void Dump(FILE *fpOut = NULL) = 0;
#endif
};

/*---------------------------------------------------------------------
 *                      class TABFile
 *
 * The main class for TAB datasets.  External programs should use this
 * class to open a TAB dataset and read/write features from/to it.
 *
 *--------------------------------------------------------------------*/
class TABFile: public IMapInfoFile
{
  private:
    char        *m_pszFname;
    TABAccess   m_eAccessMode;
    char        **m_papszTABFile;
    int         m_nVersion;
    char        *m_pszCharset;
    int         *m_panIndexNo;
    TABTableType m_eTableType;  // NATIVE (.DAT) or DBF

    TABDATFile  *m_poDATFile;   // Attributes file
    TABMAPFile  *m_poMAPFile;   // Object Geometry file
    TABINDFile  *m_poINDFile;   // Attributes index file

    OGRFeatureDefn *m_poDefn;
    OGRSpatialReference *m_poSpatialRef;
    int         bUseSpatialTraversal;

    int         m_nLastFeatureId;


    ///////////////
    // Private Read access specific stuff
    //
    int         ParseTABFileFirstPass(GBool bTestOpenNoError);
    int         ParseTABFileFields();

     ///////////////
    // Private Write access specific stuff
    //
    int         WriteTABFile();

  public:
    TABFile();
    virtual ~TABFile();

    virtual TABFileClass GetFileClass() {return TABFC_TABFile;}

    virtual int Open(const char *pszFname, const char *pszAccess,
                     GBool bTestOpenNoError = FALSE );
    virtual int Close();

    virtual const char *GetTableName()
                            {return m_poDefn?m_poDefn->GetName():"";};

    virtual void        ResetReading();
    virtual int         TestCapability( const char * pszCap );
    virtual int         GetFeatureCount (int bForce);
    virtual int         GetExtent(OGREnvelope *psExtent, int bForce);

    ///////////////
    // Read access specific stuff
    //

    int         GetNextFeatureId_Spatial( int nPrevId );

    virtual int GetNextFeatureId(int nPrevId);
    virtual TABFeature *GetFeatureRef(int nFeatureId);
    virtual OGRFeatureDefn *GetLayerDefn();

    virtual TABFieldType GetNativeFieldType(int nFieldId);

    virtual int GetBounds(double &dXMin, double &dYMin, 
                          double &dXMax, double &dYMax,
                          GBool bForce = TRUE );
    
    virtual OGRSpatialReference *GetSpatialRef();

    virtual int GetFeatureCountByType(int &numPoints, int &numLines,
                                      int &numRegions, int &numTexts,
                                      GBool bForce = TRUE);

    virtual GBool IsFieldIndexed(int nFieldId);
    virtual GBool IsFieldUnique(int /*nFieldId*/)   {return FALSE;};

    ///////////////
    // Write access specific stuff
    //
    virtual int SetBounds(double dXMin, double dYMin, 
                          double dXMax, double dYMax);
    virtual int SetFeatureDefn(OGRFeatureDefn *poFeatureDefn,
                            TABFieldType *paeMapInfoNativeFieldTypes = NULL);
    virtual int AddFieldNative(const char *pszName, TABFieldType eMapInfoType,
                               int nWidth=0, int nPrecision=0,
                               GBool bIndexed=FALSE, GBool bUnique=FALSE);
    virtual int SetSpatialRef(OGRSpatialReference *poSpatialRef);

    virtual int SetFeature(TABFeature *poFeature, int nFeatureId = -1);

    virtual int SetFieldIndexed(int nFieldId);

    ///////////////
    // semi-private.
    virtual int  GetProjInfo(TABProjInfo *poPI)
            { return m_poMAPFile->GetHeaderBlock()->GetProjInfo( poPI ); }
    virtual int  SetProjInfo(TABProjInfo *poPI);
    virtual int  SetMIFCoordSys(const char *pszMIFCoordSys);

    int         GetFieldIndexNumber(int nFieldId);
    TABINDFile  *GetINDFileRef();

    TABMAPFile  *GetMAPFileRef() { return m_poMAPFile; }

#ifdef DEBUG
    virtual void Dump(FILE *fpOut = NULL);
#endif
};


/*---------------------------------------------------------------------
 *                      class TABView
 *
 * TABView is used to handle special type of .TAB files that are
 * composed of a number of .TAB datasets linked through some indexed 
 * fields.
 *
 * NOTE: The current implementation supports only TABViews composed
 *       of 2 TABFiles linked through an indexed field of integer type.
 *       It is unclear if any other type of views could exist anyways.
 *--------------------------------------------------------------------*/
class TABView: public IMapInfoFile
{
  private:
    char        *m_pszFname;
    TABAccess   m_eAccessMode;
    char        **m_papszTABFile;
    char        *m_pszVersion;
    char        *m_pszCharset;
    
    char        **m_papszTABFnames;
    TABFile     **m_papoTABFiles;
    int         m_numTABFiles;
    int         m_nMainTableIndex; // The main table is the one that also 
                                   // contains the geometries
    char        **m_papszFieldNames;
    char        **m_papszWhereClause;

    TABRelation *m_poRelation;
    GBool       m_bRelFieldsCreated;

    ///////////////
    // Private Read access specific stuff
    //
    int         ParseTABFile(const char *pszDatasetPath, 
                             GBool bTestOpenNoError = FALSE);

    int         OpenForRead(const char *pszFname, 
                            GBool bTestOpenNoError = FALSE );

    ///////////////
    // Private Write access specific stuff
    //
    int         OpenForWrite(const char *pszFname );
    int         WriteTABFile();


  public:
    TABView();
    virtual ~TABView();

    virtual TABFileClass GetFileClass() {return TABFC_TABView;}

    virtual int Open(const char *pszFname, const char *pszAccess,
                     GBool bTestOpenNoError = FALSE );
    virtual int Close();

    virtual const char *GetTableName()
           {return m_poRelation?m_poRelation->GetFeatureDefn()->GetName():"";};

    virtual void        ResetReading();
    virtual int         TestCapability( const char * pszCap );
    virtual int         GetFeatureCount (int bForce);
    virtual int         GetExtent(OGREnvelope *psExtent, int bForce);
    
    ///////////////
    // Read access specific stuff
    //

    virtual int GetNextFeatureId(int nPrevId);
    virtual TABFeature *GetFeatureRef(int nFeatureId);
    virtual OGRFeatureDefn *GetLayerDefn();

    virtual TABFieldType GetNativeFieldType(int nFieldId);

    virtual int GetBounds(double &dXMin, double &dYMin, 
                          double &dXMax, double &dYMax,
                          GBool bForce = TRUE );
    
    virtual OGRSpatialReference *GetSpatialRef();

    virtual int GetFeatureCountByType(int &numPoints, int &numLines,
                                      int &numRegions, int &numTexts,
                                      GBool bForce = TRUE);

    virtual GBool IsFieldIndexed(int nFieldId);
    virtual GBool IsFieldUnique(int nFieldId);

    ///////////////
    // Write access specific stuff
    //
    virtual int SetBounds(double dXMin, double dYMin, 
                          double dXMax, double dYMax);
    virtual int SetFeatureDefn(OGRFeatureDefn *poFeatureDefn,
                           TABFieldType *paeMapInfoNativeFieldTypes=NULL);
    virtual int AddFieldNative(const char *pszName,
                               TABFieldType eMapInfoType,
                               int nWidth=0, int nPrecision=0,
                               GBool bIndexed=FALSE, GBool bUnique=FALSE);
    virtual int SetSpatialRef(OGRSpatialReference *poSpatialRef);

    virtual int SetFeature(TABFeature *poFeature, int nFeatureId = -1);

    virtual int SetFieldIndexed(int nFieldId);

    ///////////////
    // semi-private.
    virtual int  GetProjInfo(TABProjInfo *poPI)
            { return m_nMainTableIndex!=-1?
                     m_papoTABFiles[m_nMainTableIndex]->GetProjInfo(poPI):-1; }
    virtual int  SetProjInfo(TABProjInfo *poPI)
            { return m_nMainTableIndex!=-1?
                     m_papoTABFiles[m_nMainTableIndex]->SetProjInfo(poPI):-1; }
    virtual int  SetMIFCoordSys(const char * /*pszMIFCoordSys*/) {return -1;};

#ifdef DEBUG
    virtual void Dump(FILE *fpOut = NULL);
#endif
};


/*---------------------------------------------------------------------
 *                      class TABSeamless
 *
 * TABSeamless is used to handle seamless .TAB files that are
 * composed of a main .TAB file in which each feature is the MBR of
 * a base table.
 *
 * TABSeamless are supported for read access only.
 *--------------------------------------------------------------------*/
class TABSeamless: public IMapInfoFile
{
  private:
    char        *m_pszFname;
    char        *m_pszPath;
    TABAccess   m_eAccessMode;
    OGRFeatureDefn *m_poFeatureDefnRef;

    TABFile     *m_poIndexTable;
    int         m_nTableNameField;
    int         m_nCurBaseTableId;
    TABFile     *m_poCurBaseTable;
    GBool       m_bEOF;

    ///////////////
    // Private Read access specific stuff
    //
    int         OpenForRead(const char *pszFname, 
                            GBool bTestOpenNoError = FALSE );
    int         OpenBaseTable(TABFeature *poIndexFeature,
                              GBool bTestOpenNoError = FALSE);
    int         OpenBaseTable(int nTableId, GBool bTestOpenNoError = FALSE);
    int         OpenNextBaseTable(GBool bTestOpenNoError =FALSE);
    int         EncodeFeatureId(int nTableId, int nBaseFeatureId);
    int         ExtractBaseTableId(int nEncodedFeatureId);
    int         ExtractBaseFeatureId(int nEncodedFeatureId);

  public:
    TABSeamless();
    virtual ~TABSeamless();

    virtual TABFileClass GetFileClass() {return TABFC_TABSeamless;}

    virtual int Open(const char *pszFname, const char *pszAccess,
                     GBool bTestOpenNoError = FALSE );
    virtual int Close();

    virtual const char *GetTableName()
           {return m_poFeatureDefnRef?m_poFeatureDefnRef->GetName():"";};

    virtual void        SetSpatialFilter( OGRGeometry * );

    virtual void        ResetReading();
    virtual int         TestCapability( const char * pszCap );
    virtual int         GetFeatureCount (int bForce);
    virtual int         GetExtent(OGREnvelope *psExtent, int bForce);
    
    ///////////////
    // Read access specific stuff
    //

    virtual int GetNextFeatureId(int nPrevId);
    virtual TABFeature *GetFeatureRef(int nFeatureId);
    virtual OGRFeatureDefn *GetLayerDefn();

    virtual TABFieldType GetNativeFieldType(int nFieldId);

    virtual int GetBounds(double &dXMin, double &dYMin, 
                          double &dXMax, double &dYMax,
                          GBool bForce = TRUE );
    
    virtual OGRSpatialReference *GetSpatialRef();

    virtual int GetFeatureCountByType(int &numPoints, int &numLines,
                                      int &numRegions, int &numTexts,
                                      GBool bForce = TRUE);

    virtual GBool IsFieldIndexed(int nFieldId);
    virtual GBool IsFieldUnique(int nFieldId);

    ///////////////
    // Write access specific stuff
    //
    virtual int SetBounds(double dXMin, double dYMin, 
                          double dXMax, double dYMax)   {return -1;}
    virtual int SetFeatureDefn(OGRFeatureDefn *poFeatureDefn,
                               TABFieldType *paeMapInfoNativeFieldTypes=NULL)
                                                        {return -1;}
    virtual int AddFieldNative(const char *pszName,
                               TABFieldType eMapInfoType,
                               int nWidth=0, int nPrecision=0,
                               GBool bIndexed=FALSE, 
                               GBool bUnique=FALSE)     {return -1;}

    virtual int SetSpatialRef(OGRSpatialReference *poSpatialRef) {return -1;}

    virtual int SetFeature(TABFeature *poFeature, 
                           int nFeatureId = -1) {return -1;}

    virtual int SetFieldIndexed(int nFieldId)   {return -1;}

    ///////////////
    // semi-private.
    virtual int  GetProjInfo(TABProjInfo *poPI)
            { return m_poIndexTable?m_poIndexTable->GetProjInfo(poPI):-1; }
    virtual int  SetProjInfo(TABProjInfo *poPI)         { return -1; }
    virtual int  SetMIFCoordSys(const char * /*pszMIFCoordSys*/) {return -1;};

#ifdef DEBUG
    virtual void Dump(FILE *fpOut = NULL);
#endif
};


/*---------------------------------------------------------------------
 *                      class MIFFile
 *
 * The main class for (MID/MIF) datasets.  External programs should use this
 * class to open a (MID/MIF) dataset and read/write features from/to it.
 *
 *--------------------------------------------------------------------*/
class MIFFile: public IMapInfoFile
{
  private:
    char        *m_pszFname;
    TABAccess    m_eAccessMode;
    char        *m_pszVersion;
    char        *m_pszCharset;
    char        *m_pszDelimiter;
    char        *m_pszUnique;
    char        *m_pszIndex;
    char        *m_pszCoordSys;

    TABFieldType *m_paeFieldType;
    GBool       *m_pabFieldIndexed;
    GBool       *m_pabFieldUnique;
    
    double       m_dfXMultiplier;
    double       m_dfYMultiplier;
    double       m_dfXDisplacement;
    double       m_dfYDisplacement;

    /* these are the projection bounds, possibly much broader than extents */
    double      m_dXMin;
    double      m_dYMin;
    double      m_dXMax;
    double      m_dYMax;

    /* extents, as cached by MIFFile::PreParseFile() */
    int         m_bExtentsSet;
    OGREnvelope m_sExtents;

    int         m_nPoints;
    int         m_nLines;
    int         m_nRegions;
    int         m_nTexts;

    int         m_nPreloadedId;  // preloaded mif line is for this feature id
    MIDDATAFile  *m_poMIDFile;   // Mid file
    MIDDATAFile  *m_poMIFFile;   // Mif File

    OGRFeatureDefn *m_poDefn;
    OGRSpatialReference *m_poSpatialRef;

    int         m_nFeatureCount;
    int         m_nWriteFeatureId;
    int         m_nAttribut;

    ///////////////
    // Private Read access specific stuff
    //
    int         ReadFeatureDefn();
    int         ParseMIFHeader();
    void        PreParseFile();
    int         AddFields(const char *pszLine);
    int         GotoFeature(int nFeatureId);
    int         NextFeature();

    ///////////////
    // Private Write access specific stuff
    //
    GBool       m_bPreParsed;
    GBool       m_bHeaderWrote;
    
    int         WriteMIFHeader();
    void UpdateExtents(double dfX,double dfY);

  public:
    MIFFile();
    virtual ~MIFFile();

    virtual TABFileClass GetFileClass() {return TABFC_MIFFile;}

    virtual int Open(const char *pszFname, const char *pszAccess,
                     GBool bTestOpenNoError = FALSE );
    virtual int Close();

    virtual const char *GetTableName()
                           {return m_poDefn?m_poDefn->GetName():"";};

    virtual int         TestCapability( const char * pszCap ) ;
    virtual int         GetFeatureCount (int bForce);
    virtual void        ResetReading();
    virtual int         GetExtent(OGREnvelope *psExtent, int bForce);

    ///////////////
    // Read access specific stuff
    //
    
    virtual int GetNextFeatureId(int nPrevId);
    virtual TABFeature *GetFeatureRef(int nFeatureId);
    virtual OGRFeatureDefn *GetLayerDefn();

    virtual TABFieldType GetNativeFieldType(int nFieldId);

    virtual int GetBounds(double &dXMin, double &dYMin, 
                          double &dXMax, double &dYMax,
                          GBool bForce = TRUE );
    
    virtual OGRSpatialReference *GetSpatialRef();

    virtual int GetFeatureCountByType(int &numPoints, int &numLines,
                                      int &numRegions, int &numTexts,
                                      GBool bForce = TRUE);

    virtual GBool IsFieldIndexed(int nFieldId);
    virtual GBool IsFieldUnique(int nFieldId);

    ///////////////
    // Write access specific stuff
    //
    virtual int SetBounds(double dXMin, double dYMin, 
                          double dXMax, double dYMax);
    virtual int SetFeatureDefn(OGRFeatureDefn *poFeatureDefn,
                            TABFieldType *paeMapInfoNativeFieldTypes = NULL);
    virtual int AddFieldNative(const char *pszName, TABFieldType eMapInfoType,
                               int nWidth=0, int nPrecision=0,
                               GBool bIndexed=FALSE, GBool bUnique=FALSE);
    /* TODO */
    virtual int SetSpatialRef(OGRSpatialReference *poSpatialRef);

    virtual int SetFeature(TABFeature *poFeature, int nFeatureId = -1);

    virtual int SetFieldIndexed(int nFieldId);

    ///////////////
    // semi-private.
    virtual int  GetProjInfo(TABProjInfo * /*poPI*/){return -1;}
    /*  { return m_poMAPFile->GetHeaderBlock()->GetProjInfo( poPI ); }*/
    virtual int  SetProjInfo(TABProjInfo * /*poPI*/){return -1;}
    /*  { return m_poMAPFile->GetHeaderBlock()->SetProjInfo( poPI ); }*/
    virtual int  SetMIFCoordSys(const char * pszMIFCoordSys);

#ifdef DEBUG
    virtual void Dump(FILE * /*fpOut*/ = NULL) {};
#endif
};

/*---------------------------------------------------------------------
 * Define some error codes specific to this lib.
 *--------------------------------------------------------------------*/
#define TAB_WarningFeatureTypeNotSupported     501
#define TAB_WarningInvalidFieldName            502
#define TAB_WarningBoundsOverflow              503

/*---------------------------------------------------------------------
 * Codes for the known MapInfo Geometry types
 *--------------------------------------------------------------------*/
#define TAB_GEOM_NONE           0
#define TAB_GEOM_SYMBOL_C       0x01
#define TAB_GEOM_SYMBOL         0x02
#define TAB_GEOM_LINE_C         0x04
#define TAB_GEOM_LINE           0x05
#define TAB_GEOM_PLINE_C        0x07
#define TAB_GEOM_PLINE          0x08
#define TAB_GEOM_ARC_C          0x0a
#define TAB_GEOM_ARC            0x0b
#define TAB_GEOM_REGION_C       0x0d
#define TAB_GEOM_REGION         0x0e
#define TAB_GEOM_TEXT_C         0x10
#define TAB_GEOM_TEXT           0x11
#define TAB_GEOM_RECT_C         0x13
#define TAB_GEOM_RECT           0x14
#define TAB_GEOM_ROUNDRECT_C    0x16
#define TAB_GEOM_ROUNDRECT      0x17
#define TAB_GEOM_ELLIPSE_C      0x19
#define TAB_GEOM_ELLIPSE        0x1a
#define TAB_GEOM_MULTIPLINE_C   0x25
#define TAB_GEOM_MULTIPLINE     0x26
#define TAB_GEOM_FONTSYMBOL_C   0x28 
#define TAB_GEOM_FONTSYMBOL     0x29
#define TAB_GEOM_CUSTOMSYMBOL_C 0x2b
#define TAB_GEOM_CUSTOMSYMBOL   0x2c
/* Version 450 object types: */
#define TAB_GEOM_V450_REGION_C  0x2e
#define TAB_GEOM_V450_REGION    0x2f
#define TAB_GEOM_V450_MULTIPLINE_C 0x31
#define TAB_GEOM_V450_MULTIPLINE   0x32
/* Version 650 object types: */
#define TAB_GEOM_MULTIPOINT_C   0x34
#define TAB_GEOM_MULTIPOINT     0x35
#define TAB_GEOM_COLLECTION_C   0x37
#define TAB_GEOM_COLLECTION     0x38


/*---------------------------------------------------------------------
 * Codes for the feature classes
 *--------------------------------------------------------------------*/
typedef enum
{
    TABFCNoGeomFeature = 0,
    TABFCPoint = 1,
    TABFCFontPoint = 2,
    TABFCCustomPoint = 3,
    TABFCText = 4,
    TABFCPolyline = 5,
    TABFCArc = 6,
    TABFCRegion = 7,
    TABFCRectangle = 8,
    TABFCEllipse = 9,
    TABFCMultiPoint = 10,
    TABFCCollection = 11,
    TABFCDebugFeature
} TABFeatureClass;

/*---------------------------------------------------------------------
 * Definitions for text attributes
 *--------------------------------------------------------------------*/
typedef enum TABTextJust_t
{
    TABTJLeft = 0,      // Default: Left Justification
    TABTJCenter,
    TABTJRight
} TABTextJust;

typedef enum TABTextSpacing_t
{
    TABTSSingle = 0,    // Default: Single spacing
    TABTS1_5,           // 1.5
    TABTSDouble
} TABTextSpacing;

typedef enum TABTextLineType_t
{
    TABTLNoLine = 0,    // Default: No line
    TABTLSimple,
    TABTLArrow
} TABTextLineType;

typedef enum TABFontStyle_t     // Can be OR'ed
{                               // except box and halo are mutually exclusive
    TABFSNone       = 0,
    TABFSBold       = 0x0001,
    TABFSItalic     = 0x0002,
    TABFSUnderline  = 0x0004,
    TABFSStrikeout  = 0x0008,
    TABFSOutline    = 0x0010,
    TABFSShadow     = 0x0020,
    TABFSInverse    = 0x0040,
    TABFSBlink      = 0x0080,
    TABFSBox        = 0x0100,   // See note about box vs halo below.
    TABFSHalo       = 0x0200,   // MIF uses 256, see MIF docs, App.A
    TABFSAllCaps    = 0x0400,   // MIF uses 512
    TABFSExpanded   = 0x0800    // MIF uses 1024
} TABFontStyle;

/* TABFontStyle enum notes:
 *
 * The enumeration values above correspond to the values found in a .MAP
 * file. However, they differ a little from what is found in a MIF file:
 * Values 0x01 to 0x80 are the same in .MIF and .MAP files.
 * Values 0x200 to 0x800 in .MAP are 0x100 to 0x400 in .MIF
 *
 * What about TABFSBox (0x100) ?
 * TABFSBox is stored just like the other styles in .MAP files but it is not 
 * explicitly stored in a MIF file.
 * If a .MIF FONT() clause contains the optional BG color, then this implies
 * that either Halo or Box was set.  Thus if TABFSHalo (value 256 in MIF) 
 * is not set in the style, then this implies that TABFSBox should be set.
 */

typedef enum TABCustSymbStyle_t // Can be OR'ed
{ 
    TABCSNone       = 0,        // Transparent BG, use default colors
    TABCSBGOpaque   = 0x01,     // White pixels are opaque
    TABCSApplyColor = 0x02      // non-white pixels drawn using symbol color
} TABCustSymbStyle;

/*=====================================================================
  Base classes to be used to add supported drawing tools to each feature type
 =====================================================================*/

class ITABFeaturePen
{
  protected:
    int         m_nPenDefIndex;
    TABPenDef   m_sPenDef;
  public:
    ITABFeaturePen();
    ~ITABFeaturePen() {};
    int         GetPenDefIndex() {return m_nPenDefIndex;};
    TABPenDef  *GetPenDefRef() {return &m_sPenDef;};

    GByte       GetPenWidthPixel();
    double      GetPenWidthPoint();
    int         GetPenWidthMIF();
    GByte       GetPenPattern() {return m_sPenDef.nLinePattern;};
    GInt32      GetPenColor()   {return m_sPenDef.rgbColor;};

    void        SetPenWidthPixel(GByte val);
    void        SetPenWidthPoint(double val);
    void        SetPenWidthMIF(int val);

    void        SetPenPattern(GByte val) {m_sPenDef.nLinePattern=val;};
    void        SetPenColor(GInt32 clr)  {m_sPenDef.rgbColor = clr;};

    const char *GetPenStyleString();

    void        DumpPenDef(FILE *fpOut = NULL);
};

class ITABFeatureBrush
{
  protected:
    int         m_nBrushDefIndex;
    TABBrushDef m_sBrushDef;
  public:
    ITABFeatureBrush();
    ~ITABFeatureBrush() {};
    int         GetBrushDefIndex() {return m_nBrushDefIndex;};
    TABBrushDef *GetBrushDefRef() {return &m_sBrushDef;};

    GInt32      GetBrushFGColor()     {return m_sBrushDef.rgbFGColor;};
    GInt32      GetBrushBGColor()     {return m_sBrushDef.rgbBGColor;};
    GByte       GetBrushPattern()     {return m_sBrushDef.nFillPattern;};
    GByte       GetBrushTransparent() {return m_sBrushDef.bTransparentFill;};

    void        SetBrushFGColor(GInt32 clr)  { m_sBrushDef.rgbFGColor = clr;};
    void        SetBrushBGColor(GInt32 clr)  { m_sBrushDef.rgbBGColor = clr;};
    void        SetBrushPattern(GByte val)   { m_sBrushDef.nFillPattern=val;};
    void        SetBrushTransparent(GByte val)
                                          {m_sBrushDef.bTransparentFill=val;};

    const char *GetBrushStyleString();

    void        DumpBrushDef(FILE *fpOut = NULL);
};

class ITABFeatureFont
{
  protected:
    int         m_nFontDefIndex;
    TABFontDef  m_sFontDef;
  public:
    ITABFeatureFont();
    ~ITABFeatureFont() {};
    int         GetFontDefIndex() {return m_nFontDefIndex;};
    TABFontDef *GetFontDefRef() {return &m_sFontDef;};

    const char *GetFontNameRef() {return m_sFontDef.szFontName;};

    void        SetFontName(const char *pszName)
                              { strncpy( m_sFontDef.szFontName, pszName, 32);
                                m_sFontDef.szFontName[32] = '\0';  };

    void        DumpFontDef(FILE *fpOut = NULL);
};

class ITABFeatureSymbol
{
  protected:
    int         m_nSymbolDefIndex;
    TABSymbolDef m_sSymbolDef;
  public:
    ITABFeatureSymbol();
    ~ITABFeatureSymbol() {};
    int         GetSymbolDefIndex() {return m_nSymbolDefIndex;};
    TABSymbolDef *GetSymbolDefRef() {return &m_sSymbolDef;};

    GInt16      GetSymbolNo()    {return m_sSymbolDef.nSymbolNo;};
    GInt16      GetSymbolSize()  {return m_sSymbolDef.nPointSize;};
    GInt32      GetSymbolColor() {return m_sSymbolDef.rgbColor;};

    void        SetSymbolNo(GInt16 val)     { m_sSymbolDef.nSymbolNo = val;};
    void        SetSymbolSize(GInt16 val)   { m_sSymbolDef.nPointSize = val;};
    void        SetSymbolColor(GInt32 clr)  { m_sSymbolDef.rgbColor = clr;};

    const char *GetSymbolStyleString(double dfAngle = 0.0);

    void        DumpSymbolDef(FILE *fpOut = NULL);
};


/*=====================================================================
                        Feature Classes
 =====================================================================*/

/*---------------------------------------------------------------------
 *                      class TABFeature
 *
 * Extend the OGRFeature to support MapInfo specific extensions related
 * to geometry types, representation strings, etc.
 *
 * TABFeature will be used as a base class for all the feature classes.
 *
 * This class will also be used to instanciate objects with no Geometry
 * (i.e. type TAB_GEOM_NONE) which is a valid case in MapInfo.
 *
 * The logic to read/write the object from/to the .DAT and .MAP files is also
 * implemented as part of this class and derived classes.
 *--------------------------------------------------------------------*/
class TABFeature: public OGRFeature
{
  protected:
    int         m_nMapInfoType;

    double      m_dXMin;
    double      m_dYMin;
    double      m_dXMax;
    double      m_dYMax;

    GBool       m_bDeletedFlag;

    void        CopyTABFeatureBase(TABFeature *poDestFeature);

    // Compr. Origin is set for TAB files by ValidateCoordType()
    GInt32      m_nXMin;
    GInt32      m_nYMin;
    GInt32      m_nXMax;
    GInt32      m_nYMax;
    GInt32      m_nComprOrgX;
    GInt32      m_nComprOrgY;

  public:
             TABFeature(OGRFeatureDefn *poDefnIn );
    virtual ~TABFeature();

    virtual TABFeature     *CloneTABFeature(OGRFeatureDefn *pNewDefn = NULL);
    virtual TABFeatureClass GetFeatureClass() { return TABFCNoGeomFeature; };
    virtual int             GetMapInfoType()  { return m_nMapInfoType; };
    virtual int            ValidateMapInfoType(TABMAPFile *poMapFile = NULL)
                                                {m_nMapInfoType=TAB_GEOM_NONE;
                                                 return m_nMapInfoType;};

    GBool       IsRecordDeleted() { return m_bDeletedFlag; };
    void        SetRecordDeleted(GBool bDeleted) { m_bDeletedFlag=bDeleted; };

    /*-----------------------------------------------------------------
     * TAB Support
     *----------------------------------------------------------------*/

    virtual int ReadRecordFromDATFile(TABDATFile *poDATFile);
    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int WriteRecordToDATFile(TABDATFile *poDATFile,
                                     TABINDFile *poINDFile, int *panIndexNo);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    GBool       ValidateCoordType(TABMAPFile * poMapFile);

    /*-----------------------------------------------------------------
     * Mid/Mif Support
     *----------------------------------------------------------------*/

    virtual int ReadRecordFromMIDFile(MIDDATAFile *fp);
    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);

    virtual int WriteRecordToMIDFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    void ReadMIFParameters(MIDDATAFile *fp);
    void WriteMIFParameters(MIDDATAFile *fp);

    /*-----------------------------------------------------------------
     *----------------------------------------------------------------*/

    void        SetMBR(double dXMin, double dYMin, 
                       double dXMax, double dYMax);
    void        GetMBR(double &dXMin, double &dYMin, 
                       double &dXMax, double &dYMax);

    virtual void DumpMID(FILE *fpOut = NULL);
    virtual void DumpMIF(FILE *fpOut = NULL);

};


/*---------------------------------------------------------------------
 *                      class TABPoint
 *
 * Feature class to handle old style MapInfo point symbols:
 *
 *     TAB_GEOM_SYMBOL_C        0x01
 *     TAB_GEOM_SYMBOL          0x02
 *
 * Feature geometry will be a OGRPoint
 *
 * The symbol number is in the range [31..67], with 31=None and corresponds
 * to one of the 35 predefined "Old MapInfo Symbols"
 *
 * NOTE: This class is also used as a base class for the other point
 * symbol types TABFontPoint and TABCustomPoint.
 *--------------------------------------------------------------------*/
class TABPoint: public TABFeature, 
                public ITABFeatureSymbol
{
  public:
             TABPoint(OGRFeatureDefn *poDefnIn);
    virtual ~TABPoint();

    virtual TABFeatureClass GetFeatureClass() { return TABFCPoint; };
    virtual int             ValidateMapInfoType(TABMAPFile *poMapFile = NULL);

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    double      GetX();
    double      GetY();

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    virtual void DumpMIF(FILE *fpOut = NULL);
};


/*---------------------------------------------------------------------
 *                      class TABFontPoint
 *
 * Feature class to handle MapInfo Font Point Symbol types:
 *
 *     TAB_GEOM_FONTSYMBOL_C    0x28 
 *     TAB_GEOM_FONTSYMBOL      0x29
 *
 * Feature geometry will be a OGRPoint
 *
 * The symbol number refers to a character code in the specified Windows
 * Font (e.g. "Windings").
 *--------------------------------------------------------------------*/
class TABFontPoint: public TABPoint, 
                    public ITABFeatureFont
{
  protected:
    double      m_dAngle;
    GInt16      m_nFontStyle;           // Bold/shadow/halo/etc.

  public:
             TABFontPoint(OGRFeatureDefn *poDefnIn);
    virtual ~TABFontPoint();

    virtual TABFeatureClass GetFeatureClass() { return TABFCFontPoint; };

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    GBool       QueryFontStyle(TABFontStyle eStyleToQuery);
    void        ToggleFontStyle(TABFontStyle eStyleToToggle, GBool bStatus);

    int         GetFontStyleMIFValue();
    void        SetFontStyleMIFValue(int nStyle);
    int         GetFontStyleTABValue()           {return m_nFontStyle;};
    void        SetFontStyleTABValue(int nStyle){m_nFontStyle=(GInt16)nStyle;};

    // GetSymbolAngle(): Return angle in degrees counterclockwise
    double      GetSymbolAngle()        {return m_dAngle;};
    void        SetSymbolAngle(double dAngle);
};


/*---------------------------------------------------------------------
 *                      class TABCustomPoint
 *
 * Feature class to handle MapInfo Custom Point Symbol (Bitmap) types:
 *
 *     TAB_GEOM_CUSTOMSYMBOL_C  0x2b
 *     TAB_GEOM_CUSTOMSYMBOL    0x2c
 *
 * Feature geometry will be a OGRPoint
 *
 * The symbol name is the name of a BMP file stored in the "CustSymb"
 * directory (e.g. "arrow.BMP").  The symbol number has no meaning for 
 * this symbol type.
 *--------------------------------------------------------------------*/
class TABCustomPoint: public TABPoint, 
                      public ITABFeatureFont
{
  protected:
    GByte       m_nCustomStyle;         // Show BG/Apply Color                 

  public:
    GByte       m_nUnknown_;

  public:
             TABCustomPoint(OGRFeatureDefn *poDefnIn);
    virtual ~TABCustomPoint();

    virtual TABFeatureClass GetFeatureClass() { return TABFCCustomPoint; };

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    const char *GetSymbolNameRef()      { return GetFontNameRef(); };
    void        SetSymbolName(const char *pszName) {SetFontName(pszName);};
    
    GByte       GetCustomSymbolStyle()              {return m_nCustomStyle;}
    void        SetCustomSymbolStyle(GByte nStyle)  {m_nCustomStyle = nStyle;}
};


/*---------------------------------------------------------------------
 *                      class TABPolyline
 *
 * Feature class to handle the various MapInfo line types:
 *
 *     TAB_GEOM_LINE_C         0x04
 *     TAB_GEOM_LINE           0x05
 *     TAB_GEOM_PLINE_C        0x07
 *     TAB_GEOM_PLINE          0x08
 *     TAB_GEOM_MULTIPLINE_C   0x25
 *     TAB_GEOM_MULTIPLINE     0x26
 *     TAB_GEOM_V450_MULTIPLINE_C 0x31
 *     TAB_GEOM_V450_MULTIPLINE   0x32
 *
 * Feature geometry can be either a OGRLineString or a OGRMultiLineString
 *--------------------------------------------------------------------*/
class TABPolyline: public TABFeature, 
                   public ITABFeaturePen
{
  private:
    GBool       m_bCenterIsSet;
    double      m_dCenterX, m_dCenterY;

  public:
             TABPolyline(OGRFeatureDefn *poDefnIn);
    virtual ~TABPolyline();

    virtual TABFeatureClass GetFeatureClass() { return TABFCPolyline; };
    virtual int             ValidateMapInfoType(TABMAPFile *poMapFile = NULL);

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    /* 2 methods to simplify access to rings in a multiple polyline
     */
    int                 GetNumParts();
    OGRLineString      *GetPartRef(int nPartIndex);

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    virtual void DumpMIF(FILE *fpOut = NULL);

    int         GetCenter(double &dX, double &dY);
    void        SetCenter(double dX, double dY);

    // MapInfo-specific attributes... made available through public vars
    // for now.
    GBool       m_bSmooth;

};

/*---------------------------------------------------------------------
 *                      class TABRegion
 *
 * Feature class to handle the MapInfo region types:
 *
 *     TAB_GEOM_REGION_C         0x0d
 *     TAB_GEOM_REGION           0x0e
 *     TAB_GEOM_V450_REGION_C    0x2e
 *     TAB_GEOM_V450_REGION      0x2f
 *
 * Feature geometry will be returned as OGRPolygon (with a single ring)
 * or OGRMultiPolygon (for multiple rings).
 *
 * REGIONs with multiple rings are returned as OGRMultiPolygon instead of
 * as OGRPolygons since OGRPolygons require that the first ring be the
 * outer ring, and the other all be inner rings, but this is not guaranteed
 * inside MapInfo files.  However, when writing features, OGRPolygons with
 * multiple rings will be accepted without problem.
 *--------------------------------------------------------------------*/
class TABRegion: public TABFeature, 
                 public ITABFeaturePen, 
                 public ITABFeatureBrush
{
    GBool       m_bSmooth;
  private:
    GBool       m_bCenterIsSet;
    double      m_dCenterX, m_dCenterY;

    int     ComputeNumRings(TABMAPCoordSecHdr **ppasSecHdrs, 
                            TABMAPFile *poMAPFile);
    int     AppendSecHdrs(OGRPolygon *poPolygon,
                          TABMAPCoordSecHdr * &pasSecHdrs,
                          TABMAPFile *poMAPFile,
                          int &iLastRing);

  public:
             TABRegion(OGRFeatureDefn *poDefnIn);
    virtual ~TABRegion();

    virtual TABFeatureClass GetFeatureClass() { return TABFCRegion; };
    virtual int             ValidateMapInfoType(TABMAPFile *poMapFile = NULL);

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    /* 2 methods to make the REGION's geometry look like a single collection
     * of OGRLinearRings 
     */
    int                 GetNumRings();
    OGRLinearRing      *GetRingRef(int nRequestedRingIndex);
    GBool               IsInteriorRing(int nRequestedRingIndex);

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    virtual void DumpMIF(FILE *fpOut = NULL);

    int         GetCenter(double &dX, double &dY);
    void        SetCenter(double dX, double dY);
};


/*---------------------------------------------------------------------
 *                      class TABRectangle
 *
 * Feature class to handle the MapInfo rectangle types:
 *
 *     TAB_GEOM_RECT_C         0x13
 *     TAB_GEOM_RECT           0x14
 *     TAB_GEOM_ROUNDRECT_C    0x16
 *     TAB_GEOM_ROUNDRECT      0x17
 *
 * A rectangle is defined by the coords of its 2 opposite corners (the MBR)
 * Its corners can optionaly be rounded, in which case a X and Y rounding
 * radius will be defined.
 *
 * Feature geometry will be OGRPolygon
 *--------------------------------------------------------------------*/
class TABRectangle: public TABFeature, 
                    public ITABFeaturePen, 
                    public ITABFeatureBrush
{
  public:
             TABRectangle(OGRFeatureDefn *poDefnIn);
    virtual ~TABRectangle();

    virtual TABFeatureClass GetFeatureClass() { return TABFCRectangle; };
    virtual int             ValidateMapInfoType(TABMAPFile *poMapFile = NULL);

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    virtual void DumpMIF(FILE *fpOut = NULL);

    // MapInfo-specific attributes... made available through public vars
    // for now.
    GBool       m_bRoundCorners;
    double      m_dRoundXRadius;
    double      m_dRoundYRadius;

};


/*---------------------------------------------------------------------
 *                      class TABEllipse
 *
 * Feature class to handle the MapInfo ellipse types:
 *
 *     TAB_GEOM_ELLIPSE_C      0x19
 *     TAB_GEOM_ELLIPSE        0x1a
 *
 * An ellipse is defined by the coords of its 2 opposite corners (the MBR)
 *
 * Feature geometry can be either an OGRPoint defining the center of the
 * ellipse, or an OGRPolygon defining the ellipse itself.
 *
 * When an ellipse is read, the returned geometry is a OGRPolygon representing
 * the ellipse with 2 degrees line segments.
 *
 * In the case of the OGRPoint, then the X/Y Radius MUST be set, but.  
 * However with an OGRPolygon, if the X/Y radius are not set (== 0) then
 * the MBR of the polygon will be used to define the ellipse parameters 
 * and the center of the MBR is used as the center of the ellipse... 
 * (i.e. the polygon vertices themselves will be ignored).
 *--------------------------------------------------------------------*/
class TABEllipse: public TABFeature, 
                  public ITABFeaturePen, 
                  public ITABFeatureBrush
{

  public:
             TABEllipse(OGRFeatureDefn *poDefnIn);
    virtual ~TABEllipse();

    virtual TABFeatureClass GetFeatureClass() { return TABFCEllipse; };
    virtual int             ValidateMapInfoType(TABMAPFile *poMapFile = NULL);

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    virtual void DumpMIF(FILE *fpOut = NULL);

    // MapInfo-specific attributes... made available through public vars
    // for now.
    double      m_dCenterX;
    double      m_dCenterY;
    double      m_dXRadius;
    double      m_dYRadius;

};


/*---------------------------------------------------------------------
 *                      class TABArc
 *
 * Feature class to handle the MapInfo arc types:
 *
 *     TAB_GEOM_ARC_C      0x0a
 *     TAB_GEOM_ARC        0x0b
 *
 * In MapInfo, an arc is defined by the coords of the MBR corners of its 
 * defining ellipse, which in this case is different from the arc's MBR,
 * and a start and end angle in degrees.
 *
 * Feature geometry can be either an OGRLineString or an OGRPoint.
 *
 * In any case, X/Y radius X/Y center, and start/end angle (in degrees 
 * counterclockwise) MUST be set.
 *
 * When an arc is read, the returned geometry is an OGRLineString 
 * representing the arc with 2 degrees line segments.
 *--------------------------------------------------------------------*/
class TABArc: public TABFeature, 
              public ITABFeaturePen
{
  private:
    double      m_dStartAngle;  // In degrees, counterclockwise, 
    double      m_dEndAngle;    // starting at 3 o'clock

  public:
             TABArc(OGRFeatureDefn *poDefnIn);
    virtual ~TABArc();

    virtual TABFeatureClass GetFeatureClass() { return TABFCArc; };
    virtual int             ValidateMapInfoType(TABMAPFile *poMapFile = NULL);

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    virtual void DumpMIF(FILE *fpOut = NULL);

    double      GetStartAngle() { return m_dStartAngle; };
    double      GetEndAngle()   { return m_dEndAngle; };
    void        SetStartAngle(double dAngle);
    void        SetEndAngle(double dAngle);

    // MapInfo-specific attributes... made available through public vars
    // for now.
    double      m_dCenterX;
    double      m_dCenterY;
    double      m_dXRadius;
    double      m_dYRadius;
};


/*---------------------------------------------------------------------
 *                      class TABText
 *
 * Feature class to handle the MapInfo text types:
 *
 *     TAB_GEOM_TEXT_C         0x10
 *     TAB_GEOM_TEXT           0x11
 *
 * Feature geometry is an OGRPoint corresponding to the lower-left 
 * corner of the text MBR BEFORE ROTATION.
 * 
 * Text string, and box height/width (box before rotation is applied)
 * are required in a valid text feature and MUST be set.  
 * Text angle and other styles are optional.
 *--------------------------------------------------------------------*/
class TABText: public TABFeature, 
               public ITABFeatureFont,
               public ITABFeaturePen
{
  protected:
    char        *m_pszString;

    double      m_dAngle;
    double      m_dHeight;
    double      m_dWidth;
    double      m_dfLineEndX;
    double      m_dfLineEndY;
    GBool       m_bLineEndSet;
    void        UpdateTextMBR();

    GInt32      m_rgbForeground;
    GInt32      m_rgbBackground;

    GInt16      m_nTextAlignment;       // Justification/Vert.Spacing/arrow
    GInt16      m_nFontStyle;           // Bold/italic/underlined/shadow/...

    const char *GetLabelStyleString();

  public:
             TABText(OGRFeatureDefn *poDefnIn);
    virtual ~TABText();

    virtual TABFeatureClass GetFeatureClass() { return TABFCText; };
    virtual int             ValidateMapInfoType(TABMAPFile *poMapFile = NULL);

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    virtual void DumpMIF(FILE *fpOut = NULL);

    const char *GetTextString();
    double      GetTextAngle();
    double      GetTextBoxHeight();
    double      GetTextBoxWidth();
    GInt32      GetFontFGColor();
    GInt32      GetFontBGColor();
    void        GetTextLineEndPoint(double &dX, double &dY);

    TABTextJust GetTextJustification();
    TABTextSpacing  GetTextSpacing();
    TABTextLineType GetTextLineType();
    GBool       QueryFontStyle(TABFontStyle eStyleToQuery);

    void        SetTextString(const char *pszStr);
    void        SetTextAngle(double dAngle);
    void        SetTextBoxHeight(double dHeight);
    void        SetTextBoxWidth(double dWidth);
    void        SetFontFGColor(GInt32 rgbColor);
    void        SetFontBGColor(GInt32 rgbColor);
    void        SetTextLineEndPoint(double dX, double dY);

    void        SetTextJustification(TABTextJust eJust);
    void        SetTextSpacing(TABTextSpacing eSpacing);
    void        SetTextLineType(TABTextLineType eLineType);
    void        ToggleFontStyle(TABFontStyle eStyleToToggle, GBool bStatus);

    int         GetFontStyleMIFValue();
    void        SetFontStyleMIFValue(int nStyle, GBool bBGColorSet=FALSE);
    GBool       IsFontBGColorUsed();
    int         GetFontStyleTABValue()           {return m_nFontStyle;};
    void        SetFontStyleTABValue(int nStyle){m_nFontStyle=(GInt16)nStyle;};

};


/*---------------------------------------------------------------------
 *                      class TABMultiPoint
 *
 * Feature class to handle MapInfo Multipoint features:
 *
 *     TAB_GEOM_MULTIPOINT_C        0x34
 *     TAB_GEOM_MULTIPOINT          0x35
 *
 * Feature geometry will be a OGRMultiPoint
 *
 * The symbol number is in the range [31..67], with 31=None and corresponds
 * to one of the 35 predefined "Old MapInfo Symbols"
 *--------------------------------------------------------------------*/
class TABMultiPoint: public TABFeature, 
                     public ITABFeatureSymbol
{
  private:
    // We call it center, but it's more like a label point
    // Its value default to be the location of the first point
    GBool       m_bCenterIsSet;
    double      m_dCenterX, m_dCenterY;

  public:
             TABMultiPoint(OGRFeatureDefn *poDefnIn);
    virtual ~TABMultiPoint();

    virtual TABFeatureClass GetFeatureClass() { return TABFCMultiPoint; };
    virtual int             ValidateMapInfoType(TABMAPFile *poMapFile = NULL);

    virtual TABFeature *CloneTABFeature(OGRFeatureDefn *poNewDefn = NULL );

    int         GetXY(int i, double &dX, double &dY);
    int         GetNumPoints();

    int         GetCenter(double &dX, double &dY);
    void        SetCenter(double dX, double dY);

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual const char *GetStyleString();

    virtual void DumpMIF(FILE *fpOut = NULL);
};



/*---------------------------------------------------------------------
 *                      class TABDebugFeature
 *
 * Feature class to use for testing purposes... this one does not 
 * correspond to any MapInfo type... it's just used to dump info about
 * feature types that are not implemented yet.
 *--------------------------------------------------------------------*/
class TABDebugFeature: public TABFeature
{
  private:
    GByte       m_abyBuf[512];
    int         m_nSize;
    int         m_nCoordDataPtr;  // -1 if none
    int         m_nCoordDataSize;

  public:
             TABDebugFeature(OGRFeatureDefn *poDefnIn);
    virtual ~TABDebugFeature();

    virtual TABFeatureClass GetFeatureClass() { return TABFCDebugFeature; };

    virtual int ReadGeometryFromMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);
    virtual int WriteGeometryToMAPFile(TABMAPFile *poMapFile, TABMAPObjHdr *);

    virtual int ReadGeometryFromMIFFile(MIDDATAFile *fp);
    virtual int WriteGeometryToMIFFile(MIDDATAFile *fp);

    virtual void DumpMIF(FILE *fpOut = NULL);
};

/* -------------------------------------------------------------------- */
/*      Some stuff related to spatial reference system handling.        */
/* -------------------------------------------------------------------- */

char *MITABSpatialRef2CoordSys( OGRSpatialReference * );
OGRSpatialReference * MITABCoordSys2SpatialRef( const char * );
GBool MITABExtractCoordSysBounds( const char * pszCoordSys,
                                  double &dXMin, double &dYMin,
                                  double &dXMax, double &dYMax );
int MITABCoordSys2TABProjInfo(const char * pszCoordSys, TABProjInfo *psProj);

typedef struct {
    int         nMapInfoDatumID;
    const char  *pszOGCDatumName;
    int         nEllipsoid;
    double      dfShiftX;
    double      dfShiftY;
    double      dfShiftZ;
    double      dfDatumParm0; /* RotX */
    double      dfDatumParm1; /* RotY */
    double      dfDatumParm2; /* RotZ */
    double      dfDatumParm3; /* Scale Factor */
    double      dfDatumParm4; /* Prime Meridian */
} MapInfoDatumInfo;

typedef struct
{
    int         nMapInfoId;
    const char *pszMapinfoName;
    double      dfA; /* semi major axis in meters */
    double      dfInvFlattening; /* Inverse flattening */
} MapInfoSpheroidInfo;


/*---------------------------------------------------------------------
 * The following are used for coordsys bounds lookup
 *--------------------------------------------------------------------*/
typedef struct
{
    TABProjInfo sProj;          /* Projection/datum definition */
    double      dXMin;          /* Default bounds for that coordsys */
    double      dYMin;
    double      dXMax;
    double      dYMax;
} MapInfoBoundsInfo;

GBool   MITABLookupCoordSysBounds(TABProjInfo *psCS,
                                  double &dXMin, double &dYMin,
                                  double &dXMax, double &dYMax);
int     MITABLoadCoordSysTable(const char *pszFname);
void    MITABFreeCoordSysTable();
GBool   MITABCoordSysTableLoaded();

#endif /* _MITAB_H_INCLUDED_ */


