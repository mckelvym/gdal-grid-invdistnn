/******************************************************************************
 * $Id$
 *
 * Project:  FITS Driver
 * Purpose:  Implement FITS raster read/write support
 * Author:   Simon Perkins, s.perkins@lanl.gov
 *
 ******************************************************************************
 * Copyright (c) 2001, Simon Perkins
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
 * Revision 1.10  2002/01/22 02:12:17  sperkins
 * Minor formatting change.
 *
 * Revision 1.9  2002/01/22 01:36:54  sperkins
 * Silently truncate when writing values to FITS files outside valid range.
 *
 * Revision 1.8  2001/12/06 19:25:12  warmerda
 * updated as per submission by Diana Esch-Mosher
 *
 * Revision 1.7  2001/11/11 23:50:59  warmerda
 * added required class keyword to friend declarations
 *
 * Revision 1.6  2001/09/14 17:05:39  warmerda
 * Used strrchr() instead of rindex().
 *
 * Revision 1.5  2001/07/18 04:51:56  warmerda
 * added CPL_CVSID
 *
 * Revision 1.4  2001/03/11 22:31:03  sperkins
 * Added FITS support for "byte scaled" files.
 *
 * Revision 1.3  2001/03/09 01:57:48  sperkins
 * Added support for reading and writing metadata to FITS files.
 *
 * Revision 1.2  2001/03/06 23:06:01  sperkins
 * Fixed bug in situation where we attempt to read from a newly created band.
 *
 * Revision 1.1  2001/03/06 03:53:44  sperkins
 * Added FITS format support.
 *
 */



#include "gdal_priv.h"
#include "cpl_string.h"
#include <string.h>

CPL_CVSID("$Id$");

static GDALDriver* poFITSDriver = NULL;

CPL_C_START
#include <fitsio.h>
void	GDALRegister_FITS(void);
CPL_C_END

/************************************************************************/
/* ==================================================================== */
/*				FITSDataset				*/
/* ==================================================================== */
/************************************************************************/

class FITSRasterBand;

class FITSDataset : public GDALDataset {

  friend class FITSRasterBand;
  
  fitsfile* hFITS;
  GDALDataType gdalDataType;   // GDAL code for the image type
  int fitsDataType;   // FITS code for the image type
  bool isExistingFile;
  long highestOffsetWritten;  // How much of image has been written
  bool isBScaled;         // Is the file format floats stored as bytes
  float bZero, bScale;   // Parameters for float<->byte conversion
  FITSDataset();     // Others shouldn't call this constructor explicitly

  CPLErr Init(fitsfile* hFITS_, bool isExistingFile_);

public:
  ~FITSDataset();

  static GDALDataset* Open(GDALOpenInfo* );
  static GDALDataset* Create(const char* pszFilename,
			     int nXSize, int nYSize, int nBands,
			     GDALDataType eType,
			     char** papszParmList);
  
};

/************************************************************************/
/* ==================================================================== */
/*                            FITSRasterBand                           */
/* ==================================================================== */
/************************************************************************/

class FITSRasterBand : public GDALRasterBand {

  friend class	FITSDataset;
  
public:

  FITSRasterBand(FITSDataset*, int);
  ~FITSRasterBand();

  virtual CPLErr IReadBlock( int, int, void * );
  virtual CPLErr IWriteBlock( int, int, void * ); 
};


/************************************************************************/
/*                          FITSRasterBand()                           */
/************************************************************************/

FITSRasterBand::FITSRasterBand(FITSDataset *poDS, int nBand) {

  this->poDS = poDS;
  this->nBand = nBand;
  eDataType = poDS->gdalDataType;
  nBlockXSize = poDS->nRasterXSize;;
  nBlockYSize = 1;
}

/************************************************************************/
/*                          ~FITSRasterBand()                           */
/************************************************************************/

FITSRasterBand::~FITSRasterBand() {
  // Nothing here yet
}


/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr FITSRasterBand::IReadBlock(int nBlockXOff, int nBlockYOff,
                                  void* pImage ) {
 
  // A FITS block is one row (we assume BSQ formatted data)
  FITSDataset* dataset = (FITSDataset*) poDS;
  fitsfile* hFITS = dataset->hFITS;
  int status = 0;

  // Since a FITS block is a whole row, nBlockXOff must be zero
  // and the row number equals the y block offset. Also, nBlockYOff
  // cannot be greater than the number of rows
  CPLAssert(nBlockXOff == 0);
  CPLAssert(nBlockYOff < nRasterYSize);

  // Calculate offsets and read in the data. Note that FITS array offsets
  // start at 1...
  long offset = (nBand - 1) * nRasterXSize * nRasterYSize +
    nBlockYOff * nRasterXSize + 1;
  long nElements = nRasterXSize;

  // If we haven't written this block to the file yet, then attempting
  // to read causes an error, so in this case, just return zeros.
  if (!dataset->isExistingFile && offset > dataset->highestOffsetWritten) {
    memset(pImage, 0, nBlockXSize * nBlockYSize
	   * GDALGetDataTypeSize(eDataType) / 8);
    return CE_None;
  }

  // Otherwise read in the image data
  fits_read_img(hFITS, dataset->fitsDataType, offset, nElements,
		0, pImage, 0, &status);
  if (status) {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "Couldn't read image data from FITS file (%d).", status);
    return CE_Failure;
  }
  
  return CE_None;
}

/************************************************************************/
/*                            IWriteBlock()                             */
/*                                                                      */
/************************************************************************/

CPLErr FITSRasterBand::IWriteBlock(int nBlockXOff, int nBlockYOff,
				   void* pImage) {

  FITSDataset* dataset = (FITSDataset*) poDS;
  fitsfile* hFITS = dataset->hFITS;
  int status = 0;

  // Since a FITS block is a whole row, nBlockXOff must be zero
  // and the row number equals the y block offset. Also, nBlockYOff
  // cannot be greater than the number of rows

  // Calculate offsets and read in the data. Note that FITS array offsets
  // start at 1...
  long offset = (nBand - 1) * nRasterXSize * nRasterYSize +
    nBlockYOff * nRasterXSize + 1;
  long nElements = nRasterXSize;
  fits_write_img(hFITS, dataset->fitsDataType, offset, nElements,
		 pImage, &status);

  // Capture special case of non-zero status due to data range
  // overflow Standard GDAL policy is to silently truncate, which is
  // what CFITSIO does (in addition to returning 412 as the status).
  if (status == 412)
    status = 0;

  // Check for other errors
  if (status) {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "Error writing image data to FITS file (%d).", status);
    return CE_Failure;
  }

  // When we write a block, update the offset counter that we've written
  if (offset > dataset->highestOffsetWritten)
    dataset->highestOffsetWritten = offset;

  return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                             FITSDataset                             */
/* ==================================================================== */
/************************************************************************/

// Some useful utility functions
static int guessFITSHeaderDataType(fitsfile* hFITS, const char* key,
				   const char* value) {
  // For now we just look at the value and decide its type based on that
  // in theory we could also look at the FITS file and see what type the
  // existing key has if present.
  char* endPtr;
  // Test for int
  strtol(value, &endPtr, 10);
  if (endPtr - value == int(strlen(value)))
    return TLONG;
  // Test for double
  strtod(value, &endPtr);
  if (endPtr - value == int(strlen(value)))
    return TDOUBLE;
  // Test for logical
  if (!strcmp(value, "T") || !strcmp(value, "F"))
    return TLOGICAL;
  // Otherwise assume string
  return TSTRING;
}

// Simple static function to determine if FITS header keyword should
// be saved in meta data.
static const int ignorableHeaderCount = 17;
static char* ignorableFITSHeaders[ignorableHeaderCount] = {
  "SIMPLE", "BITPIX", "NAXIS", "NAXIS1", "NAXIS2", "NAXIS3", "END",
  "XTENSION", "PCOUNT", "GCOUNT", "EXTEND", "BSCALE", "BZERO", "CONTINUE",
  "COMMENT", "", "LONGSTRN"
};
static bool isIgnorableFITSHeader(const char* name) {
  for (int i = 0; i < ignorableHeaderCount; ++i) {
    if (strcmp(name, ignorableFITSHeaders[i]) == 0)
      return true;
  }
  return false;
}


/************************************************************************/
/*                            FITSDataset()                            */
/************************************************************************/

FITSDataset::FITSDataset() {
  hFITS = 0;
}

/************************************************************************/
/*                           ~FITSDataset()                            */
/************************************************************************/

FITSDataset::~FITSDataset() {

  int status;
  if (hFITS) {
    if(eAccess == GA_Update) {   // Only do this if we've successfully opened the file and  update capability
      // Write any meta data to the file that's compatible with FITS
      status = 0;
      fits_movabs_hdu(hFITS, 1, 0, &status);
      fits_write_key_longwarn(hFITS, &status);
      if (status) {
        CPLError(CE_Warning, CPLE_AppDefined,
	         "Couldn't move to first HDU in FITS file %s (%d).\n", 
	         GetDescription(), status);
      }
      char** metaData = GetMetadata();
      int count = CSLCount(metaData);
      for (int i = 0; i < count; ++i) {
        const char* field = CSLGetField(metaData, i);
        if (strlen(field) == 0)
	  continue;
        else {
	  char* key;
	  const char* value = CPLParseNameValue(field, &key);
	  // FITS keys must be less than 8 chars
	  if (strlen(key) <= 8 && !isIgnorableFITSHeader(key)) {
	    int dataType = guessFITSHeaderDataType(hFITS, key, value);
	    if (dataType == TDOUBLE) {
	      double dblValue = atof(value);
	      fits_update_key(hFITS, dataType, key, &dblValue,  0, &status);
	    }
	    else if (dataType == TLONG) {
	      long longValue = atoi(value);
	      fits_update_key(hFITS, dataType, key, &longValue,  0, &status);
	    }
	    else if (dataType == TLOGICAL) {
	      int logicValue = !strcmp(value, "T") ? 1 : 0;
	      fits_update_key(hFITS, dataType, key, &logicValue,  0, &status);
	    }
	    else {  // Assume it's a string
	      // Avoid warning by copying const string to non const one...
	      char* valueCpy = strdup(value);
		  fits_update_key_longstr(hFITS, key, valueCpy, 0, &status);

	      free(valueCpy);
	    }
	    // Check for errors
	    if (status) {
	      CPLError(CE_Warning, CPLE_AppDefined,
		       "Couldn't update key %s in FITS file %s (%d).", 
		       key, GetDescription(), status);
	      return;
	    }
	  }
	  // Must free up key
	  CPLFree(key);
        }
      }

      // Make sure we flush the raster cache before we close the file!
      FlushCache();
    }

    // Close the FITS handle - ignore the error status
    fits_close_file(hFITS, &status);

  }
}


/************************************************************************/
/*                           Init()                                     */
/************************************************************************/

CPLErr FITSDataset::Init(fitsfile* hFITS_, bool isExistingFile_) {

  hFITS = hFITS_;
  isExistingFile = isExistingFile_;
  highestOffsetWritten = 0;
  int status = 0;

  // Move to the primary HDU
  fits_movabs_hdu(hFITS, 1, 0, &status);
  if (status) {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "Couldn't move to first HDU in FITS file %s (%d).\n", 
	     GetDescription(), status);
    return CE_Failure;
  }

  // Get the image info for this dataset (note that all bands in a FITS dataset
  // have the same type)
  int bitpix;
  int naxis;
  const int maxdim = 3;
  long naxes[maxdim];
  fits_get_img_param(hFITS, maxdim, &bitpix, &naxis, naxes, &status);
  if (status) {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "Couldn't determine image parameters of FITS file %s (%d).", 
	     GetDescription(), status);
    return CE_Failure;
  }

  // Determine if image is byte scaled
  isBScaled = false;
  fits_read_key_flt(hFITS, "BZERO", &bZero, 0, &status);
  fits_read_key_flt(hFITS, "BSCALE", &bScale, 0, &status);
  if (status)
    // BZERO and BSCALE headers not present - reset status
    status = 0;
  else
    isBScaled = true;
  
  // Determine data type
  if (isBScaled && bitpix == 8) {  // If we have a b-scaled image
    if (isExistingFile) {          // treat it as float or double
      gdalDataType = GDT_Float32;
      fitsDataType = TFLOAT;
    }
    else {       // If the file was created, it could be either double or float
      if (gdalDataType == GDT_Float64)
	fitsDataType = TDOUBLE;
      else
	fitsDataType = TFLOAT;
    }
  }
  else if (bitpix == BYTE_IMG) {
     gdalDataType = GDT_Byte;
     fitsDataType = TBYTE;
  }
  else if (bitpix == SHORT_IMG) {
    gdalDataType = GDT_Int16;
    fitsDataType = TSHORT;
  }
  else if (bitpix == LONG_IMG) {
    gdalDataType = GDT_Int32;
    fitsDataType = TLONG;
  }
  else if (bitpix == FLOAT_IMG) {
    gdalDataType = GDT_Float32;
    fitsDataType = TFLOAT;
  }
  else if (bitpix == DOUBLE_IMG) {
    gdalDataType = GDT_Float64;
    fitsDataType = TDOUBLE;
  }
  else {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "FITS file %s has unknown data type: %d.", GetDescription(), 
	     bitpix);
    return CE_Failure;
  }

  // Determine image dimensions - we assume BSQ ordering
  if (naxis == 2) {
    nRasterXSize = naxes[0];
    nRasterYSize = naxes[1];
    nBands = 1;
  }
  else if (naxis == 3) {
    nRasterXSize = naxes[0];
    nRasterYSize = naxes[1];
    nBands = naxes[2];
  }
  else {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "FITS file %s does not have 2 or 3 dimensions.", 
	     GetDescription());
    return CE_Failure;
  }

  // Read header information from file and use it to set metadata
  // This process understands the CONTINUE standard for long strings.
  // We don't bother to capture header names that duplicate information
  // already captured elsewhere (e.g. image dimensions and type)
  int keyNum = 1;
  char key[100];
  char value[100];
  bool endReached = false;
  do {
    fits_read_keyn(hFITS, keyNum, key, value, 0, &status);
    if (status) {
      CPLError(CE_Failure, CPLE_AppDefined,
	       "Error while reading key %d from FITS file %s (%d)", 
	       keyNum, GetDescription(), status);
      return CE_Failure;
    }
    // What we do next depends upon the key and the value
    if (strcmp(key, "END") == 0) {
      endReached = true;
    }
    else if (isIgnorableFITSHeader(key)) {
      // Ignore it!
    }
    else {   // Going to store something, but check for long strings etc
      // Strip off leading and trailing quote if present
      char* newValue = value;
      if (value[0] == '\'' && value[strlen(value) - 1] == '\'') {
	newValue = value + 1;
	value[strlen(value) - 1] = '\0';
      }
      // Check for long string
      if (strrchr(newValue, '&') == newValue + strlen(newValue) - 1) {
	// Value string ends in "&", so use long string conventions
	char* longString = 0;
	fits_read_key_longstr(hFITS, key, &longString, 0, &status);
	// Note that read_key_longst already strips quotes
	if (status) {
	  CPLError(CE_Failure, CPLE_AppDefined,
		   "Error while reading long string for key %s from "
		   "FITS file %s (%d)", key, GetDescription(), status);
	  return CE_Failure;
	}
	SetMetadataItem(key, longString);
	free(longString);
      }
      else {  // Normal keyword
	SetMetadataItem(key, newValue);
      }
    }
    ++keyNum;
  } while (!endReached);
  
  // Create the bands
  for (int i = 0; i < nBands; ++i)
    SetBand(i+1, new FITSRasterBand(this, i+1));
  
  return CE_None;
}



/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset* FITSDataset::Open(GDALOpenInfo* poOpenInfo) {

  const char* fitsID = "SIMPLE  =                    T";  // Spaces important!
  size_t fitsIDLen = strlen(fitsID);  // Should be 30 chars long

  if ((size_t)poOpenInfo->nHeaderBytes < fitsIDLen)
    return NULL;
  if (memcmp(poOpenInfo->pabyHeader, fitsID, fitsIDLen))
    return NULL;
  
  // Get access mode and attempt to open the file
  int status = 0;
  fitsfile* hFITS = 0;
  if (poOpenInfo->eAccess == GA_ReadOnly) 
    fits_open_file(&hFITS, poOpenInfo->pszFilename, READONLY, &status);
  else
    fits_open_file(&hFITS, poOpenInfo->pszFilename, READWRITE, &status);
  if (status) {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "Error while opening FITS file %s (%d).\n", 
	     poOpenInfo->pszFilename, status);
    fits_close_file(hFITS, &status);
    return NULL;
  }

  // Create a FITSDataset object and initialize it from the FITS handle
  FITSDataset* dataset = new FITSDataset();
  dataset->eAccess = poOpenInfo->eAccess;
  dataset->poDriver = poFITSDriver;

  dataset->SetDescription(poOpenInfo->pszFilename);
  if (dataset->Init(hFITS, true) != CE_None) {
    delete dataset;
    return NULL;
  }
  else
    return dataset;
}


/************************************************************************/
/*                               Create()                               */
/*                                                                      */
/*      Create a new FITS file.                                         */
/************************************************************************/

GDALDataset *FITSDataset::Create(const char* pszFilename,
				 int nXSize, int nYSize, 
				 int nBands, GDALDataType eType,
				 char** papszParmList) {


  FITSDataset* dataset;
  fitsfile* hFITS;
  int status = 0;

  // Parse options - setting BSCALE and BZERO (both or neither must be set)
  // causes a float format image to be stored in byte scaled form. As a result
  // these options only make sense if eType is float or double.
  bool haveBZero = false, haveBScale = false;
  float bZero, bScale;
  if (CSLFetchNameValue(papszParmList,"BZERO") != NULL) {
    haveBZero = true;
    bZero = atof(CSLFetchNameValue(papszParmList,"BZERO"));

  }
  if (CSLFetchNameValue(papszParmList,"BSCALE") != NULL) {
    haveBScale = true;
    bScale = atof(CSLFetchNameValue(papszParmList,"BSCALE"));
  }
  if (haveBZero != haveBScale) {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "Either neither or both BSCALE and BZERO options must be "
	     "specified when creating a FITS file");
    return NULL;
  }
  if (haveBScale && !(eType == GDT_Float32 || eType == GDT_Float64)) {
    CPLError(CE_Warning, CPLE_AppDefined,
	     "BSCALE and BZERO may only be used with float or double "
	     "FITS images\nCreation options ignored");
    haveBZero = haveBScale = false;
  }

  // Create the file - to force creation, we prepend the name with '!'
  char* extFilename = new char[strlen(pszFilename) + 10];  // 10 for margin!
  sprintf(extFilename, "!%s", pszFilename);
  fits_create_file(&hFITS, extFilename, &status);
  delete extFilename;
  if (status) {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "Couldn't create FITS file %s (%d).\n", pszFilename, status);
    return NULL;
  }

  // Determine FITS type of image
  int bitpix;
  if (eType == GDT_Byte)
    bitpix = BYTE_IMG;
  else if (eType == GDT_Int16)
    bitpix = SHORT_IMG;
  else if (eType == GDT_Int32)
    bitpix = LONG_IMG;
  else if (eType == GDT_Float32)
    bitpix = FLOAT_IMG;
  else if (eType == GDT_Float64)
    bitpix = DOUBLE_IMG;
  else {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "GDALDataType (%d) unsupported for FITS", eType);
    fits_close_file(hFITS, &status);
    return NULL;
  }

  // Now create an image of appropriate size and type
  long naxes[3] = {nXSize, nYSize, nBands};
  int naxis = (nBands == 1) ? 2 : 3;
  fits_create_img(hFITS, bitpix, naxis, naxes, &status);
  if (status) {
    CPLError(CE_Failure, CPLE_AppDefined,
	     "Couldn't create image within FITS file %s (%d).", 
	     pszFilename, status);
    fits_close_file(hFITS, &status);
    return NULL;
  }

  // If bscaling is requested, add appropriate BSCALE and BZERO headers
  // We also need to set BITPIX to 8
  if (haveBScale) {
    bitpix = 8;
    fits_movabs_hdu(hFITS, 1, 0, &status);
    fits_update_key(hFITS, TINT, "BITPIX", &bitpix, 0, &status);
    fits_update_key(hFITS, TFLOAT, "BSCALE", &bScale, 0, &status);
    fits_update_key(hFITS, TFLOAT, "BZERO", &bZero, 0, &status);
    if (status) {
      CPLError(CE_Failure, CPLE_AppDefined,
	       "Couldn't set BSCALE/BZERO headers in FITS file %s (%d)",
	       pszFilename, status);
      fits_close_file(hFITS, &status);
      return NULL;
    }
  }
  
  dataset = new FITSDataset();
  dataset->poDriver = poFITSDriver;;
  dataset->nRasterXSize = nXSize;
  dataset->nRasterYSize = nYSize;
  dataset->eAccess = GA_Update;
  dataset->SetDescription(pszFilename);
  dataset->gdalDataType = eType;  // For b-scale we need to know intended type

  // Init recalculates a lot of stuff we already know, but...
  if (dataset->Init(hFITS, false) != CE_None) { 
    delete dataset;
    return NULL;
  }
  else {
    return dataset;
  }
}


/************************************************************************/
/*                          GDALRegister_FITS()                        */
/************************************************************************/

void GDALRegister_FITS() {

  GDALDriver* poDriver;

  if(poFITSDriver == NULL) {
    poFITSDriver = poDriver = new GDALDriver();
        
    poDriver->pszShortName = "FITS";
    poDriver->pszLongName = "Flexible Image Transport System";
    poDriver->pszHelpTopic = "frmt_various.html#FITS";
        
    poDriver->pfnOpen = FITSDataset::Open;
    poDriver->pfnCreate = FITSDataset::Create;
    poDriver->pfnCreateCopy = NULL;

    GetGDALDriverManager()->RegisterDriver(poDriver);
  }
}














