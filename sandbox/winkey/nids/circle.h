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
 
#ifndef _CIRCLE_H
#define _CIRCLE_H

/*******************************************************************************
	function to parse a circle packet
	
args:					buf			the buffer pointing to the start of the circle packet
							c				the structure to store the circles in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_circle_header(char *buf, NIDS_circles *c);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in circle storage

args:
						c			the structure the circles are stored in

returns:
						nothing
*******************************************************************************/

void free_circle_header(NIDS_circles *c);

/*******************************************************************************
	function to print a circle packet

args:
						c				the structure the circles are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_circle_header(NIDS_circles *c, char *prefix);

/*******************************************************************************
	fuction to draw a circle in an image

args:
						raster	pointer to the raster
						v				the structure that holds the circles
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void circles_to_raster (
	NIDS_image *im,
	NIDS_circles *c);

#endif /* _CIRCLE_H */

 
