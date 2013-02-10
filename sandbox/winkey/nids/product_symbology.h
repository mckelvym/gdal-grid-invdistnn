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
 
#ifndef _PRODUCT_SYMBOLOGY_H
#define _PRODUCT_SYMBOLOGY_H

/*******************************************************************************
	function to parse the Product Symbology
	
args:					buf			the buffer pointing to the start of the Product Symbology
							s				the structure to store the Product Symbology in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_product_symbology(char *buf, NIDS_product_symbology *s);

/*******************************************************************************
	function to check if a block is a product_symbology block
	
args:
					buf 				the buffer pointing to the start of the block

returns:
					1 	if the block is a product_symbology block
					0		if it is not
*******************************************************************************/

int is_product_symbology(char *buf);

/*******************************************************************************
	function to free any dynamicly alocated memory used in product symbology storage

args:
						h				the structure the product symbology is stored in

returns:
						nothing
*******************************************************************************/

void free_product_symbology(NIDS_product_symbology *s);

/*******************************************************************************
	function to print the product symbology

args:
						s				the structure the product desc is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_product_symbology(NIDS_product_symbology *s, char *prefix);

/*******************************************************************************
	function to convert the symbology block to a raster

args:
						s				the structure the product symbology is stored in

returns:
						nothing
*******************************************************************************/

void product_symbology_to_raster(
	NIDS_image *im,
	NIDS_product_symbology *s);



#endif /* _PRODUCT_SYMBOLOGY_H */

 
