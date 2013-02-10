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
 
#ifndef _POINT_H
#define _POINT_H

/*******************************************************************************
	function to parse a single point
	
args:
							buf			the buffer pointing to the first byte of the point
							p				the struct to store the point in;

returns:
							pointer to the next byte in the buffer 
*******************************************************************************/

char *parse_point (char *buf, NIDS_point *p);

/*******************************************************************************
	function to parse a point packet
	
args:					buf			the buffer pointing to the start of the point packet
							p				the structure to store the points in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_point_header(char *buf, NIDS_points *p);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in point storage

args:
						p				the structure the points are stored in

returns:
						nothing
*******************************************************************************/

void free_point_header(NIDS_points *p);

/*******************************************************************************
	function to print a single point

args:
						p				the structure the point is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_point(NIDS_point *p, char *prefix);

/*******************************************************************************
	function to print a point packet

args:
						p				the structure the points are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_point_header(NIDS_points *p, char *prefix);

#endif /* _POINT_H */

 
