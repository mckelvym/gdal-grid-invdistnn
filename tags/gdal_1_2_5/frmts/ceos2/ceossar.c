/******************************************************************************
 * $Id$
 *
 * Project:  ASI CEOS Translator
 * Purpose:  Functions related to CeosSARVolume_t.
 * Author:   Paul Lahaie, pjlahaie@atlsci.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Atlantis Scientific Inc
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
 * Revision 1.4  2001/07/18 04:51:56  warmerda
 * added CPL_CVSID
 *
 * Revision 1.3  2000/09/25 21:17:04  warmerda
 * Avoid warnings.
 *
 * Revision 1.2  2000/03/31 13:34:43  warmerda
 * ported to GDAL, replace headers
 *
 */

#include "ceos.h"

CPL_CVSID("$Id$");

extern Link_t *RecipeFunctions;

void InitCeosSARVolume(CeosSARVolume_t *volume, int32 file_name_convention)
{
    volume->Flavour = \
	volume->Sensor = \
	volume->ProductType = 0;

    volume->FileNamingConvention = file_name_convention ;

    volume->VolumeDirectoryFile =
	volume->SARLeaderFile =
        volume->SARTrailerFile =
	volume->NullVolumeDirectoryFile =
	volume->ImageDesc.ImageDescValid = FALSE;

    volume->RecordList = NULL;
}


void CalcCeosSARImageFilePosition(CeosSARVolume_t *volume, int channel, int line, int *record, int *file_offset)
{
    struct CeosSARImageDesc *ImageDesc;
    int TotalRecords=0, TotalBytes=0;

    if(record)
	*record = 0;
    if(file_offset)
	*file_offset = 0;

    if( volume )
    {
	if( volume->ImageDesc.ImageDescValid )
	{
	    ImageDesc = &( volume->ImageDesc );

	    switch( ImageDesc->ChannelInterleaving )
	    {
	    case __CEOS_IL_PIXEL:
		TotalRecords = (line - 1) * ImageDesc->RecordsPerLine;
		TotalBytes = (TotalRecords) * ( ImageDesc->BytesPerRecord );
		break;
	    case __CEOS_IL_LINE:
		TotalRecords = (ImageDesc->NumChannels * (line - 1) + 
				(channel - 1)) * ImageDesc->RecordsPerLine;
		TotalBytes = (TotalRecords) * ( ImageDesc->BytesPerRecord ) ;
		break;
	    case __CEOS_IL_BAND:
		TotalRecords = (((channel - 1) * ImageDesc->Lines) * 
				ImageDesc->RecordsPerLine) +
				(line - 1) * ImageDesc->RecordsPerLine;

		TotalBytes = (TotalRecords) * ( ImageDesc->BytesPerRecord );
		break;
	    }
	    if(file_offset)
		*file_offset = ImageDesc->FileDescriptorLength + TotalBytes;
	    if(record)
		*record = TotalRecords + 1;
	}
    }
}

int32 GetCeosSARImageData(CeosSARVolume_t *volume, CeosRecord_t *processed_data_record, int channel, int xoff, int xsize, int bufsize, uchar *buffer)
{
    return 0;
}

void DetermineCeosSARPixelOrder( CeosSARVolume_t *volume, CeosRecord_t *record )
{

}

void GetCeosSAREmbeddedInfo(CeosSARVolume_t *volume, CeosRecord_t *processed_data_record, CeosSAREmbeddedInfo_t *info)
{
}

void DeleteCeosSARVolume(CeosSARVolume_t *volume)
{
    Link_t *Links;
    
    if( volume )
    {
	if( volume->RecordList )
	{
	    for(Links = volume->RecordList; Links != NULL; Links = Links->next)
	    {
		if(Links->object)
		{
		    DeleteCeosRecord( Links->object );
		    Links->object = NULL;
		}
	    }
	    DestroyList( volume->RecordList );
	}
	HFree( volume );
    }
}

