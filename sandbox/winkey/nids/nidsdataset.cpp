/******************************************************************************
 *
 * Project:  NIDS (level 3 nexrad radar) translator
 * Purpose:  Implements nidsDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2012, Brian Case
 *
 * Permission is hereby granted, Free of charge, to any person obtaining a
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
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <bzlib.h>
#include "zlib.h"

#include "NIDS.h"
#include "image.h"
#include "msg_header.h"
#include "prod_desc.h"
#include "product_symbology.h"
#include "graphic_alphanumeric.h"
#include "tabular_alphanumeric.h"
#include "product_dependent_desc.h"
#include "color.h"
#include "error.h"
#include "myzlib.h"
#include "prod_info.h"

#include "gdal_pam.h"
#include "ogr_spatialref.h"

#define RASTER_X_SIZE 640
#define RASTER_Y_SIZE 640
#define RASTER_RES_MULTI 1852

class NIDSDataset : public GDALPamDataset
{
    friend class NIDSRasterBand;

  private:
    
	char                    *p;
	char                    nws[100];

	GByte                   abyHeader[1012];
    double                  padfTransform[6];
    NIDS data;
    char *pszSRSwkt;
  public:
    NIDSDataset();
    ~NIDSDataset();
    
    static GDALDataset *Open( GDALOpenInfo * );

    CPLErr      GetGeoTransform( double * padfTransform_f ) {
        memcpy(padfTransform_f, padfTransform, sizeof(double) * 6);
    }
    const char *GetProjectionRef() { return pszSRSwkt; }
};

class NIDSRasterBand : public GDALPamRasterBand
{
  public:
                NIDSRasterBand( NIDSDataset *, int );
    virtual CPLErr IReadBlock( int, int, void * );
};


NIDSRasterBand::NIDSRasterBand( NIDSDataset *poDS, int nBand )

{
    this->poDS = poDS;
    this->nBand = nBand;
    
    eDataType = GDT_Byte;

    nBlockXSize = RASTER_X_SIZE;
    nBlockYSize = RASTER_Y_SIZE;
}

/******************************************************************************
raster band ireadblock
******************************************************************************/
CPLErr NIDSRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )

{
    NIDSDataset *poGDS = (NIDSDataset *) poDS;
    char        *pszRecord;
    int         nRecordSize = nBlockXSize*5 + 9 + 2;
    int         i;

	NIDS_image im;
	
	im.raster = NULL;
	im.x_center = RASTER_X_SIZE / 2;
	im.y_center = RASTER_Y_SIZE / 2;
	im.width = RASTER_X_SIZE;
	im.height = RASTER_Y_SIZE;
	im.scale = 1;
	
	
	im.raster = (char*)pImage;
	
	product_symbology_to_raster(&im, &(poGDS->data.symb));
	graphic_alphanumeric_to_raster(&im, &(poGDS->data.graphic));


    

    return CE_None;
}

/******************************************************************************
 NIDSDataset::NIDSDataset
******************************************************************************/

NIDSDataset::NIDSDataset() {
    
	nws[100] = {0};
    data = {};
    pszSRSwkt = NULL;

    nRasterXSize = RASTER_X_SIZE;
    nRasterYSize = RASTER_Y_SIZE;
    
}

/******************************************************************************
 NIDSDataset::~NIDSDataset
******************************************************************************/

NIDSDataset::~NIDSDataset() {
	free_msg_header(&(data.msg));
    free_prod_desc(&(data.prod));
	free_product_symbology(&(data.symb));
	free_graphic_alphanumeric(&(data.graphic));
	free_tabular_alphanumeric(&(data.tab));
    if (pszSRSwkt)
        CPLFree(pszSRSwkt);
}

/******************************************************************************
    open function
******************************************************************************/

GDALDataset *NIDSDataset::Open( GDALOpenInfo * poOpenInfo )
{

    
    /***** Confirm the requested access is supported. *****/

    if( poOpenInfo->eAccess == GA_Update ) {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "The JDEM driver does not support update access to existing"
                  " datasets.\n" );
        return NULL;
    }
    
    char *temp = NULL;
    
    /***** Create a corresponding GDALDataset. *****/

    NIDSDataset *poDS;

    poDS = new NIDSDataset();

    VSILFILE *fp;
    fp = VSIFOpenL(poOpenInfo->pszFilename, "rb");
    if (fp == NULL)
    {
        delete poDS;
        return NULL;
    }

	/***** read the tacked on nws header *****/

    VSIFReadL(poDS->nws, 1, sizeof(poDS->nws) - 1, fp);


    /***** CTRL A is start of product *****/

	if (*(poDS->nws) == 1)
		if (!VSIFReadL(poDS->nws + 30, 11, 1, fp)) {
			delete poDS;
            return NULL;
	    }

	
	if (!VSIFReadL(poDS->nws, 30, 1, fp)) {
		delete poDS;
        return NULL;
	}

    buffer buf = {0};
    buffer ubuf = {0};
	/***** aloc initial buffer memory *****/
	
	if (!(buf.buf = (char*)CPLMalloc(BUFSIZE)))
		ERROR("NIDS_read");
	
	buf.alloced = BUFSIZE;
	
	do {
		size_t read;
		
		/***** CPLRealloc if we need more *****/
		
		while (buf.used + BUFSIZE > buf.alloced) {
			buf.alloced *= 2;
			
			if (!(temp = (char*) CPLRealloc(buf.buf, buf.alloced)))
				ERROR("NIDS_read");
			
			buf.buf = temp;
		}
		
		/***** read data *****/
		
		if (!(read = VSIFReadL(buf.buf + buf.used, 1, BUFSIZE, fp))) {
				ERROR("NIDS_read");
		}
		
		buf.used += read;
		
	} while (!VSIFEofL(fp) && read > 0);
	
	poDS->p = buf.buf;
	
	/***** zlibed? *****/
	
	if (is_zlib((unsigned char *)buf.buf)) {
		
		unzlib(&(buf), &(ubuf));
		
		/***** if there is any data left its uncompressed, copy it *****/
		
		if (buf.parsed < buf.used) {

			while (ubuf.alloced - ubuf.used < buf.used - buf.parsed) {
				ubuf.alloced *= 2;
			
				if (!(temp = (char*)CPLRealloc(ubuf.buf, ubuf.alloced)))
					ERROR("NIDS_read");
			
				ubuf.buf = temp;
			}
			
			memcpy(ubuf.buf + ubuf.used, buf.buf + buf.used, buf.used - buf.parsed);
			ubuf.used += buf.used - buf.parsed;
			
			}
		
	poDS->p = ubuf.buf + 54;
		

	}
	
	/***** parse the header *****/
	
    poDS->p = parse_msg_header(poDS->p, &(poDS->data.msg));
	
	/***** prod info *****/
	
	poDS->data.info = get_prod_info(poDS->data.msg.code);
 
	/***** parse prod dep desc *****/
	
	parse_product_dependent_desc(poDS->data.msg.code, poDS->p, &(poDS->data.pdd));
	
	/***** parse prod desc *****/
	
	poDS->p = parse_prod_desc(poDS->p, &(poDS->data.prod));

    
	char *bzbuf = NULL;
	int         bze;
	if (poDS->data.pdd.compression) {
		poDS->data.pdd.uncompressed_size += 1000000;
		if (!(bzbuf = (char*)CPLMalloc(poDS->data.pdd.uncompressed_size)))
			ERROR("NIDS_read");
        unsigned int tmpsize = poDS->data.pdd.uncompressed_size;
		if (0 > (bze = BZ2_bzBuffToBuffDecompress(bzbuf,
															 &(tmpsize),
															 poDS->p,
															 poDS->data.msg.len - (poDS->p - buf.buf),
															 0,
															 0))) {
			fprintf (stderr, "bze = %i\n", bze);
			ERROR("NIDS_read : bzip error");
		}											 

        poDS->data.pdd.uncompressed_size = tmpsize;
		poDS->p = bzbuf;
	}

	switch (poDS->data.msg.code) {
		
		/***** stand alone tabular_alphanumeric *****/
		
		case 62:
			poDS->p = parse_tabular_alphanumeric(buf.buf - 8, &(poDS->data.tab));
			break;
		
		default:
			if (is_product_symbology(poDS->p))
				poDS->p = parse_product_symbology(poDS->p, &(poDS->data.symb));
			if (poDS->data.prod.graph_off && is_graphic_alphanumeric(poDS->p))
				poDS->p = parse_graphic_alphanumeric(poDS->p, &(poDS->data.graphic));
			if (poDS->data.prod.tab_off && is_tabular_alphanumeric(poDS->p))
				poDS->p = parse_tabular_alphanumeric(poDS->p, &(poDS->data.tab));
			break;
	}
	
	
	if (bzbuf)
		CPLFree(bzbuf);
	if (ubuf.buf)
		CPLFree(ubuf.buf);
	CPLFree(buf.buf);

    VSIFCloseL (fp);


    /***** debug *****/

    char prefix[PREFIX_LEN];
	
	snprintf(prefix, PREFIX_LEN, "data");
	
	print_msg_header(&(poDS->data.msg), prefix);
	print_prod_desc(&(poDS->data.prod), prefix);
	print_product_symbology(&(poDS->data.symb), prefix);
	print_graphic_alphanumeric(&(poDS->data.graphic), prefix);
	print_tabular_alphanumeric(&(poDS->data.tab), prefix);
    
    /***** set the proj info *****/

    OGRSpatialReference *poSRS = new OGRSpatialReference();
    poSRS->SetWellKnownGeogCS( "WGS84");
    poSRS->SetPS(poDS->data.prod.lat, poDS->data.prod.lon, 0.0, 0.0, 0.0);
    poSRS->exportToWkt(&(poDS->pszSRSwkt));
    delete poSRS;
    
    /***** set the geotransform *****/

    poDS->padfTransform[0] = -(poDS->data.info->xres * RASTER_RES_MULTI * (RASTER_X_SIZE / 2));
    poDS->padfTransform[1] = poDS->data.info->xres * RASTER_RES_MULTI;
    poDS->padfTransform[2] = 0.0;
    poDS->padfTransform[3] = poDS->data.info->yres * RASTER_RES_MULTI * (RASTER_Y_SIZE / 2);
    poDS->padfTransform[4] = 0.0;
    poDS->padfTransform[5] = -(poDS->data.info->yres * RASTER_RES_MULTI);
    
    /***** Create band information objects. *****/

    poDS->SetBand( 1, new NIDSRasterBand( poDS, 1 ));

    /***** Initialize any PAM information. *****/
    
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML();

    /***** Initialize default overviews. *****/
    
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );
    
    return (poDS);
}








CPL_C_START
void CPL_DLL GDALRegister_NIDS(void);
CPL_C_END


void GDALRegister_NIDS()

{
    GDALDriver  *poDriver;

    if (! GDAL_CHECK_VERSION("NIDS"))
        return;

    if( GDALGetDriverByName( "NIDS" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "NIDS" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Nexrad Level 3 Radar" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#NIDS" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "nids" );

        poDriver->pfnOpen = NIDSDataset::Open;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
