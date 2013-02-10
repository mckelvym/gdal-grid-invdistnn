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
 
#ifndef _HAIL_POSITIVE_H
#define _HAIL_POSITIVE_H


/*******************************************************************************
	function to parse a hail_positive packet
	
args:					buf			the buffer hail_positiveing to the start of the hail_positive packet
							p				the structure to store the hail_positives in

returns:
							hail_positiveer to the next byte in the buffer
*******************************************************************************/

char *parse_hail_positive_header(char *buf, NIDS_hail_positives *p);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in hail_positive storage

args:
						p				the structure the hail_positives are stored in

returns:
						nothing
*******************************************************************************/

void free_hail_positive_header(NIDS_hail_positives *p);

/*******************************************************************************
	function to print a hail_positive packet

args:
						p				the structure the hail_positives are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_hail_positive_header(NIDS_hail_positives *p, char *prefix);

/*******************************************************************************
	fuction to draw hail_positives in an image

args:
						im			pointer to the raster struct
						p				the structure that holds the hail_positives

returns:
						nothing
*******************************************************************************/

void hail_positives_to_raster (
	NIDS_image *im,
	NIDS_hail_positives *p);


#endif /* _HAIL_POSITIVE_H */

 
