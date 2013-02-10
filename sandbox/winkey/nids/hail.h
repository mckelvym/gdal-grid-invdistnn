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
 
#ifndef _HAIL_H
#define _HAIL_H

/*******************************************************************************
	function to parse a hail packet
	
args:					buf			the buffer pointing to the start of the hail packet
							b				the structure to store the hails in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_hail_header(char *buf, NIDS_hails *h);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in hail storage

args:
						h				the structure the hails are stored in

returns:
						nothing
*******************************************************************************/

void free_hail_header(NIDS_hails *h);

/*******************************************************************************
	function to print a hail packet

args:
						h				the structure the hails are stored in
						prefix	the start of the line
returns:
						nothing
*******************************************************************************/

void print_hail_header(NIDS_hails *h, char *prefix);

/*******************************************************************************
	fuction to draw hails in an image

args:
						im			pointer to the raster struct
						h				the structure that holds the hails

returns:
						nothing
*******************************************************************************/

void hails_to_raster (
	NIDS_image *im,
	NIDS_hails *h);

#endif /* _HAIL_H */

 
