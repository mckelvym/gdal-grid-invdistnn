/******************************************************************************
 *
 * Project:  NIDS (level 3 nexrad radar) translator
 * Purpose:  Implements nidsDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2012, Brian Case
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
 *****************************************************************************/
 
#ifndef _TVS_H
#define _TVS_H


/*******************************************************************************
	function to parse a tvs packet
	
args:					buf			the buffer tvsing to the start of the tvs packet
							p				the structure to store the tvss in

returns:
							tvser to the next byte in the buffer
*******************************************************************************/

char *parse_tvs_header(char *buf, NIDS_tvss *p);

/*******************************************************************************
	function to free any dynamicly alocated memory used in tvs storage

args:
						p				the structure the tvss are stored in

returns:
						nothing
*******************************************************************************/

void free_tvs_header(NIDS_tvss *p);

/*******************************************************************************
	function to print a tvs packet

args:
						p				the structure the tvss are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_tvs_header(NIDS_tvss *p, char *prefix);

/*******************************************************************************
	fuction to draw tvss in an image

args:
						im			pointer to the raster struct
						p				the structure that holds the tvss

returns:
						nothing
*******************************************************************************/

void tvss_to_raster (
	NIDS_image *im,
	NIDS_tvss *p);

#endif /* _TVS_H */

 
