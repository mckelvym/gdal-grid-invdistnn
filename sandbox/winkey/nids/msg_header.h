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
 
#ifndef _MSG_HEADER_H
#define _MSG_HEADER_H

/*******************************************************************************
	function to parse the msg header
	
args:					buf			the buffer pointing to the start of the msg header
							h				the structure to store the msg header in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_msg_header(char *buf, NIDS_msg_header *h);

/*******************************************************************************
	function to free any dynamicly alocated memory used in msg header storage

args:
						h				the structure the msg header is stored in

returns:
						nothing
*******************************************************************************/

void free_msg_header(NIDS_msg_header *h);

/*******************************************************************************
	function to print a msg header packet

args:
						h				the structure the msg header is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_msg_header(NIDS_msg_header *h, char *prefix);

#endif /* _MSG_HEADER_H */

