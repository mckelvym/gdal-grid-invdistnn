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
#ifndef _POINT_FEATURE_H
#define _POINT_FEATURE_H

/*******************************************************************************
	function to parse a point feature packet
	
args:					buf			the buffer pointing to the start of the point feature packet
							p				the structure to store the points in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_point_feature_header(char *buf, NIDS_point_features *p);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in point feature storage

args:
						p				the structure the points are stored in

returns:
						nothing
*******************************************************************************/

void free_point_feature_header(NIDS_point_features *p);

/*******************************************************************************
	function to print a point feature packet

args:
						p				the structure the points are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_point_feature_header(NIDS_point_features *p, char *prefix);

/*******************************************************************************
	fuction to draw point feature in an image

args:
						im			pointer to the raster struct
						p				the structure that holds the point features

returns:
						nothing
*******************************************************************************/

void point_features_to_raster (
	NIDS_image *im,
	NIDS_point_features *p);

#endif /* _POINT_FEATURE_H */

 
