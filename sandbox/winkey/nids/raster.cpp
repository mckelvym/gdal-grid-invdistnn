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
#include <errno.h>
#include <time.h>

#include "NIDS.h"
#include "get.h"
#include "image.h"
#include "raster.h"
#include "error.h"

/*******************************************************************************

Individual Rows

Each row contains Run Length Encoded (RLE) values.
01 			number of RLE halfwords			RLE data always padded to even halfword boundary
02 			Run0 	Code0 	Run1 	Code1 	Run is 4 bit value for number of bins for this value
03 			Run2 	Code2 	Run3 	Code3 	Code is 4 bit value (0-15) for the value within the run
04-## 	Run Codes										Repeated for entire row
##			RunN 	CodeN									00 	00

*******************************************************************************/

/*******************************************************************************
	function to parse a single row
	
args:
							buf			the buffer pointing to the first byte of the row
							r				the struct to store the row in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_raster_row(char *buf, NIDS_raster_row *r) {
	char *p;
	int i;
	
	r->num_rle = GET2(buf);
		
	if (!(r->run = (char*)CPLMalloc(r->num_rle * 2)))
		ERROR("parse_raster_row");
	
	if (!(r->code = (char*)CPLMalloc(r->num_rle * 2)))
		ERROR("parse_raster_row");
	
	p = buf + 2;
	
	for (i = 0 ; i < r->num_rle ; i++) {
		r->run[i] = (p[i] >> 4) & 0x0f;
		r->code[i] = p[i] & 0x0f;
	}
	
	return p + i;
}	


 
/*******************************************************************************
3.2. Raster Data Packet

Raster data is pixelized data defined by rows and columns.
01 	Packet Code 	Hex "BA0F" or "BA07"
02 	Op Flags 	Hex "8000"
03 	Op Flags 	Hex "00C0"
04 	I Start Coordinate 	Start coordinate on -2048 to 2047 coordinate system
05 	J Start Coordinate 	 
06 	X Scale (INT) 	Reserved
07 	X Scale (FRACT) 	 
08 	Y Scale (INT) 	Reserved
09 	Y Scale (FRACT) 	 
10 	Number of Rows 	 
11 	Packing Descriptor 	Always 2
*******************************************************************************/

/*******************************************************************************
	function to parse a raster packet
	
args:					buf			the buffer pointing to the start of the raster packet
							r				the structure to store the raster in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_raster_header(char *buf, NIDS_raster *r) {
	int i;
	char *p;
	
	r->op_flags1 = GET2(buf);
	r->op_flags2 = GET2(buf + 2);
	r->x_start = GET2(buf + 4);
	r->y_start = GET2(buf + 6);
	r->x_scale_int = GET2(buf + 8);
	r->x_scale_fract = GET2(buf + 10);
	r->y_scale_int = GET2(buf + 12);
	r->y_scale_fract = GET2(buf + 14);
	r->num_rows = GET2(buf + 16);
	r->packing = GET2(buf + 18);
	
	if (!(r->rows = (NIDS_raster_row*)CPLMalloc(r->num_rows * sizeof(NIDS_raster_row))))
		ERROR("parse_radial_header");
	
	p = buf + 20;
	
	for (i = 0 ; i < r->num_rows ; i++) {
		p = parse_raster_row(p, r->rows + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree a raster row

args:
						r			the row to CPLFree

returns:
						nothing
*******************************************************************************/

void free_row(NIDS_raster_row *r) {
	CPLFree(r->run);
	CPLFree(r->code);
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in raster storage

args:
						r				the structure the raster is stored in

returns:
						nothing
*******************************************************************************/

void free_raster_header(NIDS_raster *r) {
	int i;
	
	for (i = 0 ; i < r->num_rows ; i++)
		free_row(r->rows + i);
	
	CPLFree(r->rows);
	
}

/*******************************************************************************
	function to print a single raster row

args:
						r				the structure the row is stored in
						prefix	the start of the line
						rn			the row number

returns:
						nothing
*******************************************************************************/

void print_row(NIDS_raster_row *r, char *prefix, int rn) {
	
	printf("%s.rast.rows[%i].num_rle %i\n", prefix, rn, r->num_rle);
	
}

/*******************************************************************************
	function to print a raster packet

args:
						r				the structure the raster is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_raster_header(NIDS_raster *r, char *prefix) {
	int i;
	
	printf("%s.rast.x_start %i\n", prefix, r->x_start);
	printf("%s.rast.y_start %i\n", prefix, r->y_start);
	printf("%s.rast.num_rows %i\n", prefix, r->num_rows);
	
	for (i = 0 ; i < r->num_rows ; i++)
		print_row(r->rows + i, prefix, i);

}

/*******************************************************************************
	function to convert a single row to a raster
*******************************************************************************/

void row_to_raster (NIDS_raster_row *r, NIDS_image *im, int row) {
	int x = 0, y = row;
	int i, j;

	
	for (i = 0 ; i < r->num_rle ; i++) {
		for (j = 0 ; j < r->run[i] ; j++) {
			plot(im, x, y, r->code[i]);
		x++;
		}
	}
	
}

/*******************************************************************************
	function to convert a raster to a raster

args:
						r				the structure that holds the radials
						width		pointer to return the width of the raster in
						height	pointer to return the height of the raster in

returns:
						a char pointer to the raster data

*******************************************************************************/

void rasters_to_raster (
	NIDS_image *im,
	NIDS_raster *r)
{
	int i;
	
	im->x_center = im->width / 2 - r->num_rows / 2;
	im->y_center = im->height / 2 - r->num_rows / 2;
	
	for (i = 0 ; i < r->num_rows ; i++)
		row_to_raster(r->rows + i, im, i);
	
}

