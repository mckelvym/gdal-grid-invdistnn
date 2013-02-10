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
 
#ifndef _D_PRECIP_H
#define _D_PRECIP_H

/*******************************************************************************
	function to parse a precip packet
	
args:					buf			the buffer pointing to the start of the precip packet
							r				the structure to store the precip in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_d_precip_header(char *buf, NIDS_d_precip *r);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in precip storage

args:
						r				the structure the percip is stored in

returns:
						nothing
*******************************************************************************/

void free_d_precip_header(NIDS_d_precip *r);

/*******************************************************************************
	function to print a percip packet

args:
						b				the structure the percip is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_d_precip_header(NIDS_d_precip *r, char *prefix);

#endif /* _D_PRECIP_H */

 
