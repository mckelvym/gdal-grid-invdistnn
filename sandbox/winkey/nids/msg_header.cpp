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

//#include <stdio.h>
#include <time.h>

#include "error.h"
#include "NIDS.h"
#include "get.h"
#include "msg_header.h"


/*******************************************************************************
1. Message Header Block

Halfword	Name										Description
01 				Message Code						See message code table below
02				Date of Message					Number of days since 1 Jan 1970
03				Time of Message (MSW)		Number of seconds since midnight
04				Time (LSW)
05				Length of Message (MSW) Number of bytes in message including header
06				Length (LSW) 	 
07				Source ID 	 
08				Destination ID 	 
09				Number of Blocks				1 (Header Block) + number of blocks in message
*******************************************************************************/

/*******************************************************************************
	function to parse the msg header
	
args:					buf			the buffer pointing to the start of the msg header
							h				the structure to store the msg header in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_msg_header(char *buf, NIDS_msg_header *h) {
	
	h->code = GET2(buf);
	h->time = GET2(buf + 2);
	h->time *= 24 * 60 * 60;
	h->time += GET4(buf + 4);
	h->len = GET4(buf + 8);
	h->s_id = GET2(buf + 12);
	h->d_id = GET2(buf + 14);
	h->num_blocks = GET2(buf + 16);
	
	return buf + 18;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in msg header storage

args:
						h				the structure the msg header is stored in

returns:
						nothing
*******************************************************************************/

void free_msg_header(NIDS_msg_header *h) {

}

/*******************************************************************************
	function to print the msg header

args:
						h				the structure the msg header is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_msg_header(NIDS_msg_header *h, char *prefix) {
	
	printf("%s.msg.code %i\n", prefix, h->code);
	printf("%s.msg.time %lu\n", prefix, h->time);
	printf("%s.msg.len %u\n", prefix, h->len);
	printf("%s.msg.s_id %i\n", prefix, h->s_id);
	printf("%s.msg.d_id %i\n", prefix, h->d_id);
	printf("%s.msg.num_blocks %i\n", prefix, h->num_blocks);
	
}

