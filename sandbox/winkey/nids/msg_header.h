/*
 * libNIDS
 * Copyright (C) Brian Case 2008 <rush@gisweather.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
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

