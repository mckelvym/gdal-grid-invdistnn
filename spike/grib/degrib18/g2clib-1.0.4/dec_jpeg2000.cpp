#include "grib2.h"

#ifndef USE_JPEG2000
int dec_jpeg2000(char *injpc,g2int bufsize,g2int *outfld) {return 0;}
#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_JPEG2000_J2KSUBFILE
#include <gdal_pam.h>
#else
#include <jasper/jasper.h>
#define JAS_1_700_2
#endif /* USE_JPEG2000_J2KSUBFILE */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef USE_JPEG2000_J2KSUBFILE

#define	J2K_SIGNATURE		0x6a502020	/* Signature, string "jP  " */
#define FILE_TYPE	      0x66747970	/* File Type, string "ftyp" */
#define	J2K_CODESTREAM	0x6a703263	/* Code Stream, string "jp2c" */
#define	MAGIC_NUMBER	  0x0d0a870a  /* Magic number */
#define	MAJOR_VERSION		0x6a703220  /* Major version, string "jp2 " */
#define	MINOR_VERSION		0

void writeChar(char c, char** buf)
{
  **buf = c;
  ++(*buf);
}

void writeUInt32(g2intu i, char **buf)
{
	writeChar((i >> 24) & 0xff, buf);
	writeChar((i >> 16) & 0xff, buf);
	writeChar((i >> 8) & 0xff, buf);
	writeChar(i & 0xff, buf);
}

void writeJ2KHeaders(char* buf)
{
  // signature box
  writeUInt32(12, &buf); // size = 12 bytes: 4 size, 4 type, 4 magic
  writeUInt32(J2K_SIGNATURE, &buf);
  writeUInt32(MAGIC_NUMBER, &buf);

  // file type box
  writeUInt32(20, &buf); // size = 20 bytes: 4 size, 4 type, 4 majver, 4 minver, 4 compatibility code
  writeUInt32(FILE_TYPE, &buf);
  writeUInt32(MAJOR_VERSION, &buf);
  writeUInt32(MINOR_VERSION, &buf);
  writeUInt32(MAJOR_VERSION, &buf); // repeat majver as "compatibility code"

  // code stream box: j2k_data follows
  writeUInt32(0, &buf);  // size = 0 means rest of buffer contains j2k_data
  writeUInt32(J2K_CODESTREAM, &buf);
}

#endif /* USE_JPEG2000_J2KSUBFILE */

   int dec_jpeg2000(char *injpc,g2int bufsize,g2int *outfld)
/*$$$  SUBPROGRAM DOCUMENTATION BLOCK
*                .      .    .                                       .
* SUBPROGRAM:    dec_jpeg2000      Decodes JPEG2000 code stream
*   PRGMMR: Gilbert          ORG: W/NP11     DATE: 2002-12-02
*
* ABSTRACT: This Function decodes a JPEG2000 code stream specified in the
*   JPEG2000 Part-1 standard (i.e., ISO/IEC 15444-1) using JasPer 
*   Software version 1.500.4 (or 1.700.2) written by the University of British
*   Columbia and Image Power Inc, and others.
*   JasPer is available at http://www.ece.uvic.ca/~mdadams/jasper/.
*
* PROGRAM HISTORY LOG:
* 2002-12-02  Gilbert
*
* USAGE:     int dec_jpeg2000(char *injpc,g2int bufsize,g2int *outfld)
*
*   INPUT ARGUMENTS:
*      injpc - Input JPEG2000 code stream.
*    bufsize - Length (in bytes) of the input JPEG2000 code stream.
*
*   OUTPUT ARGUMENTS:
*     outfld - Output matrix of grayscale image values.
*
*   RETURN VALUES :
*          0 = Successful decode
*         -3 = Error decode jpeg2000 code stream.
*         -5 = decoded image had multiple color components.
*              Only grayscale is expected.
*
* REMARKS:
*
*      Requires JasPer Software version 1.500.4 or 1.700.2
*
* ATTRIBUTES:
*   LANGUAGE: C
*   MACHINE:  IBM SP
*
*$$$*/

{

#ifdef USE_JPEG2000_J2KSUBFILE
     
    // J2K_SUBFILE method
    
    // Specifically ECW needs a few JP2 headers (the so-called "boxes") .. prepend them to the memory buffer
    // Does Kakadu need them as well?
    const int headersize = 40;  // 40 bytes of headers will be prepended
    char* jpcbuf = injpc; // remember old buffer
    injpc = (char*)malloc(bufsize + headersize); // old buffer is too small, thus allocate new buffer
    writeJ2KHeaders(injpc); // write out the 40 header bytes
    memcpy(injpc + headersize, jpcbuf, bufsize); // append the jpeg2000 data

    // create "memory file" from buffer
    int fileNumber = 0;
    VSIStatBufL   sStatBuf;
    char *pszFileName = CPLStrdup( "/vsimem/work.jp2" );

    // ensure we don't overwrite an existing file accidentally
    while ( VSIStatL( pszFileName, &sStatBuf ) == 0 ) {
          CPLFree( pszFileName );
          pszFileName = CPLStrdup( CPLSPrintf( "/vsimem/work%d.jp2", ++fileNumber ) );
    }

    VSIFCloseL( VSIFileFromMemBuffer( pszFileName, (unsigned char*)injpc, bufsize + headersize, TRUE ) ); // TRUE to let vsi delete the buffer when done

    // Open it with a JPEG2000 driver supporting J2K_SUBFILE
    char *pszDSName = CPLStrdup( CPLSPrintf( "J2K_SUBFILE:%d,%d,%s", 0, bufsize, pszFileName ) );

    // Open memory buffer for reading 
    GDALDataset* poJ2KDataset = (GDALDataset *)GDALOpen( pszDSName, GA_ReadOnly ); // This goes to CNCSJP2FileView
    //GDALDataset* poJ2KDataset = (GDALDataset *)GDALOpen( "/vsimem/work.jp2", GA_ReadOnly ); // This goes to CNCSJP2File, and fails
 
    if( poJ2KDataset == NULL )
    {
        printf("dec_jpeg2000: Unable to open JPEG2000 image within GRIB file.\n"
                  "Is the JPEG2000 driver available?" );
        return -3;
    }

    if( poJ2KDataset->GetRasterCount() != 1 )
    {
       printf("dec_jpeg2000: Found color image.  Grayscale expected.\n");
       return (-5);
    }

    GDALRasterBand *poBand = poJ2KDataset->GetRasterBand(1);

    // Fulfill administration: initialize parameters required for RasterIO
    int nXSize = poJ2KDataset->GetRasterXSize();
    int nYSize = poJ2KDataset->GetRasterYSize();
    int nXOff = 0;
    int nYOff = 0;
    int nBufXSize = nXSize;
    int nBufYSize = nYSize;
    GDALDataType eBufType = GDT_Int32; // map to type of "outfld" buffer: g2int*
    int nBandCount = 1;
    int* panBandMap = NULL;
    int nPixelSpace = 0;
    int nLineSpace = 0;
    int nBandSpace = 0;

    //    Decompress the JPEG2000 into the output integer array.
    poJ2KDataset->RasterIO( GF_Read, nXOff, nYOff, nXSize, nYSize,
                                       outfld, nBufXSize, nBufYSize, eBufType,
                                       nBandCount, panBandMap, 
                                       nPixelSpace, nLineSpace, nBandSpace );

    // close source file, and "unlink" it.
    GDALClose( poJ2KDataset );
    VSIUnlink( pszFileName );

    CPLFree( pszDSName );
    CPLFree( pszFileName );

    return 0;

#else /* USE_JPEG2000_J2KSUBFILE */

    // JasPer method
    
    int ier;
    g2int i,j,k;
    jas_image_t *image=0;
    jas_stream_t *jpcstream;
    jas_image_cmpt_t *pcmpt;
    char *opts=0;
    jas_matrix_t *data;

//    jas_init();

    ier=0;
//   
//     Create jas_stream_t containing input JPEG200 codestream in memory.
//       

    jpcstream=jas_stream_memopen(injpc,bufsize);

//   
//     Decode JPEG200 codestream into jas_image_t structure.
//       

    image=jpc_decode(jpcstream,opts);
    if ( image == 0 ) {
       printf(" jpc_decode return = %d \n",ier);
       return -3;
    }
    
    pcmpt=image->cmpts_[0];
/*
    printf(" SAGOUT DECODE:\n");
    printf(" tlx %d \n",image->tlx_);
    printf(" tly %d \n",image->tly_);
    printf(" brx %d \n",image->brx_);
    printf(" bry %d \n",image->bry_);
    printf(" numcmpts %d \n",image->numcmpts_);
    printf(" maxcmpts %d \n",image->maxcmpts_);
#ifdef JAS_1_500_4
    printf(" colormodel %d \n",image->colormodel_);
#endif
#ifdef JAS_1_700_2
    printf(" colorspace %d \n",image->clrspc_);
#endif
    printf(" inmem %d \n",image->inmem_);
    printf(" COMPONENT:\n");
    printf(" tlx %d \n",pcmpt->tlx_);
    printf(" tly %d \n",pcmpt->tly_);
    printf(" hstep %d \n",pcmpt->hstep_);
    printf(" vstep %d \n",pcmpt->vstep_);
    printf(" width %d \n",pcmpt->width_);
    printf(" height %d \n",pcmpt->height_);
    printf(" prec %d \n",pcmpt->prec_);
    printf(" sgnd %d \n",pcmpt->sgnd_);
    printf(" cps %d \n",pcmpt->cps_);
#ifdef JAS_1_700_2
    printf(" type %d \n",pcmpt->type_);
#endif
*/

//   Expecting jpeg2000 image to be grayscale only.
//   No color components.
//
    if (image->numcmpts_ != 1 ) {
       printf("dec_jpeg2000: Found color image.  Grayscale expected.\n");
       return (-5);
    }

// 
//    Create a data matrix of grayscale image values decoded from
//    the jpeg2000 codestream.
//
    data=jas_matrix_create(jas_image_height(image), jas_image_width(image));
    jas_image_readcmpt(image,0,0,0,jas_image_width(image),
                       jas_image_height(image),data);
//
//    Copy data matrix to output integer array.
//
    k=0;
    for (i=0;i<pcmpt->height_;i++) 
      for (j=0;j<pcmpt->width_;j++) 
        outfld[k++]=data->rows_[i][j];
//
//     Clean up JasPer work structures.
//
    jas_matrix_destroy(data);
    ier=jas_stream_close(jpcstream);
    jas_image_destroy(image);

    return 0;
#endif /* USE_JPEG2000_J2KSUBFILE */
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* USE_JPEG2000 */
