/******************************************************************************
 *
 * Project:  NIDS (level 3 nexrad radar) translator
 * Purpose:  Implements nidsDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2012, Brian Case
 *
 * Permission is hereby granted, CPLFree of charge, to any person obtaining a
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
#include "precip.h"
#include "error.h"

/*******************************************************************************
                        MSB               HALFWORD                 LSB
                                          PACKET CODE (=18)
                                                SPARE
                                                SPARE
                                   NUMBER OF LFM BOXES IN ROW
                                          NUMBER OF ROWS
REPEAT FOR                            NUMBER OF BYTES IN ROW
EACH ROW                  RUN (0)      LEVEL (0)      RUN (1)       LEVEL (1)
                          RUN (2)      LEVEL (2)      RUN (3)       LEVEL (3)
                                                 •••
                                                 •••
                          RUN (N)      LEVEL (N)         0000         0000
    Figure 3-11b. Precipitation Rate Data Array Packet - Packet Code 18 (Sheet 1)
*******************************************************************************/

/*******************************************************************************
	function to parse a single row
	
args:
							buf			the buffer pointing to the first byte of the row
							r				the struct to store the row in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_precip_row(char *buf, NIDS_precip_row *r) {
	char *p;
	int i;
	
	r->num_rle = GET2(buf);
		
	if (!(r->run = (char*)CPLMalloc(r->num_rle)))
		ERROR("parse_precip_row");
	
	if (!(r->code = (char*)CPLMalloc(r->num_rle)))
		ERROR("parse_precip_row");
	
	p = buf + 2;
	
	for (i = 0 ; i < r->num_rle ; i++) {
		r->run[i] = (p[i] >> 4) & 0x0f;
		r->code[i] = p[i] & 0x0f;
	}
	
	return p + i;
}


/*******************************************************************************
	function to parse a precip packet
	
args:					buf			the buffer pointing to the start of the precip packet
							r				the structure to store the precip in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_precip_header(char *buf, NIDS_precip *r) {
	int i;
	char *p;
	
	r->op_flags1 = GET2(buf);
	r->op_flags2 = GET2(buf + 2);
	r->lfm_per_row = GET2(buf + 4);
	r->num_rows = GET2(buf + 6);
	
	if (!(r->rows = (NIDS_precip_row*)CPLMalloc(r->num_rows * sizeof(NIDS_precip))))
		ERROR("parse_precip_header");
	
	p = buf + 8;
	
	for (i = 0 ; i < r->num_rows ; i++) {
		p = parse_precip_row(p, r->rows + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree a precip row

args:
						r			the precip row to CPLFree

returns:
						nothing
*******************************************************************************/

void free_precip_row(NIDS_precip_row *r) {
	CPLFree(r->run);
	CPLFree(r->code);
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in precip storage

args:
						r				the structure the percip is stored in

returns:
						nothing
*******************************************************************************/

void free_precip_header(NIDS_precip *r) {
	int i;
	
	for (i = 0 ; i < r->num_rows ; i++)
		free_precip_row(r->rows + i);
	
	CPLFree(r->rows);
	
}

/*******************************************************************************
	function to print a single precip row

args:
						r				the structure the precip row is stored in
						prefix	the start of the line
						rn			the barb number

returns:
						nothing
*******************************************************************************/

void print_precip_row(NIDS_precip_row *r, char *prefix, int rn) {
	
	printf("%s.precip.rows[%i].num_rle %i\n", prefix, rn, r->num_rle);
	
}
	
/*******************************************************************************
	function to print a percip packet

args:
						b				the structure the percip is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_precip_header(NIDS_precip *r, char *prefix) {
	int i;
	
	printf("%s.precip.op_flags1 %i\n", prefix, r->op_flags1);
	printf("%s.precip.op_flags2 %i\n", prefix, r->op_flags2);
	printf("%s.precip.lfm_per_row %i\n", prefix, r->lfm_per_row);
	printf("%s.precip.num_rows %i\n", prefix, r->num_rows);
	
	for (i = 0 ; i < r->num_rows ; i++)
		print_precip_row(r->rows + i, prefix, i);
	
}

/*******************************************************************************
	function to convert a single row to a raster
*******************************************************************************/

void precip_to_raster (NIDS_precip_row *r, NIDS_image *im, int row) {
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

void precips_to_raster (
	NIDS_image *im,
	NIDS_precip *r)
{
	int i;
	
	im->x_center = im->width / 2 - r->num_rows / 2;
	im->y_center = im->height / 2 - r->num_rows / 2;
	
	for (i = 0 ; i < r->num_rows ; i++)
		precip_to_raster(r->rows + i, im, i);
	
}

