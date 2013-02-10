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
 
#ifndef _GRAPHIC_ALPHANUMERIC_H
#define _GRAPHIC_ALPHANUMERIC_H

/*******************************************************************************
	function to parse the graphic alphanumeric block
	
args:					buf			the buffer pointing to the start of the block
							g				the structure to store the block in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_graphic_alphanumeric(char *buf, NIDS_graphic_alphanumeric *g);

/*******************************************************************************
	function to check if a block is a graphic alphanumeric block
	
args:
					buf 				the buffer pointing to the start of the block

returns:
					1 	if the block is a graphic_alphanumeric block
					0		if it is not
*******************************************************************************/

int is_graphic_alphanumeric(char *buf);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in graphic alphanumeric storage

args:
						h				the structure the product symbology is stored in

returns:
						nothing
*******************************************************************************/

void free_graphic_alphanumeric(NIDS_graphic_alphanumeric *g);

/*******************************************************************************
	function to print the graphic_alphanumeric block

args:
						g				the structure the block is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_graphic_alphanumeric(NIDS_graphic_alphanumeric *g, char *prefix);

/*******************************************************************************
	function to convert the graphic_alphanumeric block to a raster

args:
						g				the structure the graphic_alphanumeric is stored in

returns:
						nothing
*******************************************************************************/

void graphic_alphanumeric_to_raster(
	NIDS_image *im,
	NIDS_graphic_alphanumeric *g);

#endif /* _GRAPHIC_ALPHANUMERIC_H */

 
