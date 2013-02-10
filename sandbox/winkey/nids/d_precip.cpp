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
#include "d_precip.h"
#include "error.h"

/*******************************************************************************
Individual Rows

Each row contains Run Length Encoded (RLE) values.
01 	Number of Bytes in Row 	RLE data always padded to even halfword boundary
02 	Run0 	Code0 	Run is 8 bit value for number of bins for this value
03 	Run1 	Code1 	Code is 8 bit value (0-255) for the value within the run
04-## 	Run Codes 	Repeated for entire row
## 	RunN 	CodeN 	  
*******************************************************************************/

/*******************************************************************************
	function to parse a single row
	
args:
							buf			the buffer pointing to the first byte of the row
							r				the struct to store the row in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_d_precip_row(char *buf, NIDS_d_precip_row *r) {
	char *p;
	int i;
	
	r->num_rle = GET2(buf);
	
	if (!(r->run = (char *)CPLMalloc(r->num_rle)))
		ERROR("parse_d_precip_row");
	
	if (!(r->code = (char *)CPLMalloc(r->num_rle)))
		ERROR("parse_d_precip_row");
	
	p = buf + 2;
	
	for (i = 0 ; i < r->num_rle ; i++, p += 2) {
		r->run[i] = *p;
		r->code[i] = *(p + 1);
	}
	
	if (!(i%2))
		p += 2;
	
	return p;
}	

/*******************************************************************************
3.5. Digital Precipitation Array Packet
01 	Packet Code = 17 	Hex "11"
02 	Op Flags 	Reserved
03 	Op Flags 	Reserved
04 	Number of LFM Boxes in Row 	 
05 	Number of Rows 	 
*******************************************************************************/

/*******************************************************************************
	function to parse a digital precip packet
	
args:					buf			the buffer pointing to the start of the precip packet
							r				the structure to store the precip in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_d_precip_header(char *buf, NIDS_d_precip *r) {
	int i;
	char *p;
	
	r->op_flags1 = GET2(buf);
	r->op_flags2 = GET2(buf + 2);
	r->lfm_per_row = GET2(buf + 4);
	r->num_rows = GET2(buf + 6);
	
	if (!(r->rows = (NIDS_d_precip_row*)CPLMalloc(r->num_rows * sizeof(NIDS_d_precip))))
		ERROR("parse_d_precip_header");
	
	p = buf + 8;
	
	for (i = 0 ; i < r->num_rows ; i++) {
		p = parse_d_precip_row(p, r->rows + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree a digital precip row

args:
						r			the precip row to CPLFree

returns:
						nothing
*******************************************************************************/

void free_d_precip_row(NIDS_d_precip_row *r) {
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

void free_d_precip_header(NIDS_d_precip *r) {
	int i;
	
	for (i = 0 ; i < r->num_rows ; i++)
		free_d_precip_row(r->rows + i);
	
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

void print_d_precip_row(NIDS_d_precip_row *r, char *prefix, int rn) {
	
	printf("%s.d_precip.rows[%i].num_rle %i\n", prefix, rn, r->num_rle);
	
}
	
/*******************************************************************************
	function to print a percip packet

args:
						b				the structure the percip is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_d_precip_header(NIDS_d_precip *r, char *prefix) {
	int i;
	
	printf("%s.d_precip.op_flags1 %i\n", prefix, r->op_flags1);
	printf("%s.d_precip.op_flags2 %i\n", prefix, r->op_flags2);
	printf("%s.d_precip.lfm_per_row %i\n", prefix, r->lfm_per_row);
	printf("%s.d_precip.num_rows %i\n", prefix, r->num_rows);
	
	for (i = 0 ; i < r->num_rows ; i++)
		print_d_precip_row(r->rows + i, prefix, i);
	
}
