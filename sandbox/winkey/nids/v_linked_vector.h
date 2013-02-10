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
 
#ifndef _V_LINKED_VECTOR_H
#define _V_LINKED_VECTOR_H

/*******************************************************************************
	function to parse an valued linked vector packet
	
args:					buf			the buffer pointing to the start of the vector packet
							v				the structure to store the vectors in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_v_linked_vector_header(char *buf, NIDS_v_linked_vectors *v);

/*******************************************************************************
	function to free any dynamicly alocated memory used in valued linked vector storage

args:
						v				the structure the vector is stored in

returns:
						nothing
*******************************************************************************/

void free_v_linked_vector_header(NIDS_v_linked_vectors *v);

/*******************************************************************************
	function to print a valued linked vector packet

args:
						v				the structure the vectors is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_v_linked_vector_header(NIDS_v_linked_vectors *v, char *prefix);

/*******************************************************************************
	fuction to draw a vector in an image

args:
						raster	pointer to the raster
						v				the structure that holds the vectors
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void v_linked_vectors_to_raster (
	NIDS_image *im,
	NIDS_v_linked_vectors *v);

#endif /* _V_LINKED_VECTOR_H */

 
