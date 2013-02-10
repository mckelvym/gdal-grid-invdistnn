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
 
#ifndef _HAIL_PROBABLE_H
#define _HAIL_PROBABLE_H

/*******************************************************************************
	function to parse a hail_probable packet
	
args:					buf			the buffer hail_probableing to the start of the hail_probable packet
							p				the structure to store the hail_probables in

returns:
							hail_probableer to the next byte in the buffer
*******************************************************************************/

char *parse_hail_probable_header(char *buf, NIDS_hail_probables *p);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in hail_probable storage

args:
						p				the structure the hail_probables are stored in

returns:
						nothing
*******************************************************************************/

void free_hail_probable_header(NIDS_hail_probables *p);

/*******************************************************************************
	function to print a hail_probable packet

args:
						p				the structure the hail_probables are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_hail_probable_header(NIDS_hail_probables *p, char *prefix);

/*******************************************************************************
	fuction to draw hail_probables in an image

args:
						im			pointer to the raster struct
						p				the structure that holds the hail_probables

returns:
						nothing
*******************************************************************************/

void hail_probables_to_raster (
	NIDS_image *im,
	NIDS_hail_probables *p);

#endif /* _HAIL_PROBABLE_H */

 
