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
 
#ifndef _D_RADIAL_H
#define _D_RADIAL_H

/*******************************************************************************
	function to parse a Digital Radial Data Array packet
	
args:					buf			the buffer pointing to the start of the radial packet
							r				the structure to store the radials in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_d_radial_header(char *buf, NIDS_d_radials *r);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in Digital Radial Data
	Array storage

args:
						r				the structure the radials is stored in

returns:
						nothing
*******************************************************************************/

void free_d_radial_header(NIDS_d_radials *r);

/*******************************************************************************
	function to print a Digital Radial Data Array packet

args:
						r				the structure the radials are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_d_radial_header(NIDS_d_radials *r, char *prefix);



#endif /* _D_RADIAL_H */

 
