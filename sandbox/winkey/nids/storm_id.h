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
 
#ifndef _STORM_ID_H
#define _STORM_ID_H

/*******************************************************************************
	function to parse a storm id packet
	
args:					buf			the buffer pointing to the start of the storm id packet
							s				the structure to store the storm ids in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_storm_id_header(char *buf, NIDS_storm_ids *s);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in storm_id storage

args:
						s				the structure the storm_ids are stored in

returns:
						nothing
*******************************************************************************/

void free_storm_id_header(NIDS_storm_ids *s);

/*******************************************************************************
	function to print a storm id packet

args:
						s				the structure the storm ids are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_storm_id_header(NIDS_storm_ids *s, char *prefix);

/*******************************************************************************
	fuction to draw a storm id in an image

args:
						t				the structure that holds the storm id
						width		pointer to return the width of the raster in
						height	pointer to return the height of the raster in

returns:
						a char pointer to the raster data
*******************************************************************************/

void storm_ids_to_raster (
	NIDS_image *im,
	NIDS_storm_ids *s);

#endif /* _STORM_ID_H */

 
