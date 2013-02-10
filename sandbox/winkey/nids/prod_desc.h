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
 
#ifndef _PROD_DESC_H
#define _PROD_DESC_H

/*******************************************************************************
	function to parse the product desc
	
args:					buf			the buffer pointing to the start of the product desc
							d				the structure to store the product desc in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_prod_desc(char *buf, NIDS_prod_desc *d);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in product desc storage

args:
						d				the structure the product desc is stored in

returns:
						nothing
*******************************************************************************/

void free_prod_desc(NIDS_prod_desc *d);

/*******************************************************************************
	function to print the product desc

args:
						d				the structure the product desc is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_prod_desc(NIDS_prod_desc *d, char *prefix);

#endif /* _PROD_DESC_H */

 
