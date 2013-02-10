/*
 * This program is CPLFree software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the CPLFree Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the CPLFree Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "error.h"
#include "myzlib.h"
#include "cpl_conv.h"

char *zerr(int err)
{
	char *result = NULL;
	
	switch (err) {
		case Z_ERRNO:
			result = "Z_ERRNO;";
			break;
		
		case Z_STREAM_ERROR:
			result = "invalid compression level";
			break;
		
		case Z_DATA_ERROR:
			result = "invalid or incomplete deflate data";
			break;
		
		case Z_MEM_ERROR:
			result = "out of memory";
			break;
		
		case Z_VERSION_ERROR:
			result = "zlib version mismatch!";
			break;
		
		default:
			result = "default";
			break;
		
		
	}
	
	return result;
}

int is_zlib(unsigned char *buf) {
	int result = 0;
	
	if ( (*buf & 0xf) == Z_DEFLATED )
		if ( (*buf >> 4) + 8 <= 15 )
			if ( !(((*buf << 8) + *(buf+1)) % 31) )
				result = 1;
	
	return result;
}

int unzlib(buffer *in, buffer *out) {
	int err = 0;
	z_stream strm = {};
	char *temp = NULL;

	
	/***** zlibed? *****/
	
	if (is_zlib((unsigned char *)in->buf)) {
		
		/***** aloc initial out buffer memory *****/
	
		if (!(out->buf = (char*)CPLMalloc(BUFSIZE)))
			ERROR("NIDS_read");
		
		out->alloced = BUFSIZE;
		
		/***** init *****/
		
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
				
		while (is_zlib((unsigned char *)in->buf + in->parsed)) {
			
			do {
				strm.next_in = (unsigned char *)in->buf + in->parsed;
				strm.avail_in = in->used - in->parsed;
				strm.next_out = (unsigned char *)out->buf + out->used;
				strm.avail_out = out->alloced - out->used;
				
				/***** init *****/
			
				if (0 > (err = inflateInit(&strm)))
					ERROR(zerr(err));
				
				/***** inflate *****/
			
				if (0 > (err = inflate(&strm, Z_NO_FLUSH)))
					ERROR(zerr(err));
				
				if (err > 1) {
					if (0 > (err = inflateEnd(&strm)))
						ERROR(zerr(err));
					return err;
				}
				
				/***** was the output buffer big enough *****/
				
				if (!strm.avail_out) {
					out->alloced *= 2;
		
					if (!(temp = (char*)CPLRealloc(out->buf, out->alloced)))
						ERROR("NIDS_read");
		
					out->buf = temp;
					
					if (0 > (err = inflateEnd(&strm)))
						ERROR(zerr(err));
				}
				
				else {
					in->parsed += strm.total_in;
					out->used += strm.total_out;
				}
			
			} while (err == Z_OK);
			
			/***** cleanup *****/
			
			if (0 > (err = inflateEnd(&strm)))
				  ERROR(zerr(err));
		
		}
	}
	
	return err;
}
