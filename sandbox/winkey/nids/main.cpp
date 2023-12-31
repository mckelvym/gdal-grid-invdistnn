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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <bzlib.h>
#include <zlib.h>

#include "NIDS.h"
#include "image.h"
#include "msg_header.h"
#include "prod_desc.h"
#include "product_symbology.h"
#include "graphic_alphanumeric.h"
#include "tabular_alphanumeric.h"
#include "product_dependent_desc.h"
#include "color.h"
#include "error.h"
#include "myzlib.h"
#include "prod_info.h"



FILE *NIDS_open(char *filename) {
	FILE *result = NULL;
	
	if (!(result = fopen(filename, "r")))
		ERROR("NIDS_open");
	
	return result;
}

void NIDS_close(FILE *fp) {
	
	fclose(fp);
	
}

void NIDS_read (FILE *fp, NIDS *data) {
  buffer buf = {};
	buffer ubuf = {};
	

	char *p;
	char nws[100] = {};
	char *bzbuf = NULL;
	int bze;
	char *temp = NULL;

	
	/***** read the tacked on nws header *****/
	
	if (!fread(nws, 30, 1, fp))
		ERROR("NIDS_read");
			
	if (*nws == 1)
		if (!fread(nws + 30, 11, 1, fp))
			ERROR("NIDS_read");
	
	printf("%s\n", nws);
	
	/***** aloc initial buffer memory *****/
	
	if (!(buf.buf = CPLMalloc(BUFSIZE)))
		ERROR("NIDS_read");
	
	buf.alloced = BUFSIZE;
	
	do {
		size_t read;
		
		/***** CPLRealloc if we need more *****/
		
		while (buf.used + BUFSIZE > buf.alloced) {
			buf.alloced *= 2;
			
			if (!(temp = CPLRealloc(buf.buf, buf.alloced)))
				ERROR("NIDS_read");
			
			buf.buf = temp;
		}
		
		/***** read data *****/
		
		if (!(read = fread(buf.buf + buf.used, 1, BUFSIZE, fp))) {
			if (ferror(fp))
				ERROR("NIDS_read");
		}
		
		buf.used += read;
		
	} while (!feof(fp) && read > 0);
	
	p = buf.buf;
	
	/***** zlibed? *****/
	
	if (is_zlib((unsigned char *)buf.buf)) {
		
		unzlib(&buf, &ubuf);
		
		/***** if there is any data left its uncompressed, copy it *****/
		
		if (buf.parsed < buf.used) {

			while (ubuf.alloced - ubuf.used < buf.used - buf.parsed) {
				ubuf.alloced *= 2;
			
				if (!(temp = CPLRealloc(ubuf.buf, ubuf.alloced)))
					ERROR("NIDS_read");
			
				ubuf.buf = temp;
			}
			
			memcpy(ubuf.buf + ubuf.used, buf.buf + buf.used, buf.used - buf.parsed);
			ubuf.used += buf.used - buf.parsed;
			
			}
		
	p = ubuf.buf + 54;
		

	}
	
	/***** parse the header *****/
	
  p = parse_msg_header(p, &(data->msg));
	
	/***** prod info *****/
	
	data->info = get_prod_info(data->msg.code);
 
	/***** parse prod dep desc *****/
	
	parse_product_dependent_desc(data->msg.code, p, &(data->pdd));
	
	/***** parse prod desc *****/
	
	p = parse_prod_desc(p, &(data->prod));
	
	if (data->pdd.compression) {
		data->pdd.uncompressed_size += 1000000;
		if (!(bzbuf = CPLMalloc(data->pdd.uncompressed_size)))
			ERROR("NIDS_read");

		if (0 > (bze = BZ2_bzBuffToBuffDecompress(bzbuf,
															 &(data->pdd.uncompressed_size),
															 p,
															 data->msg.len - (p - buf.buf),
															 0,
															 0))) {
			fprintf (stderr, "bze = %i\n", bze);
			ERROR("NIDS_read : bzip error");
		}											 
		
		p = bzbuf;
	}

	switch (data->msg.code) {
		
		/***** stand alone tabular_alphanumeric *****/
		
		case 62:
			p = parse_tabular_alphanumeric(buf.buf - 8, &(data->tab));
			break;
		
		default:
			if (is_product_symbology(p))
				p = parse_product_symbology(p, &(data->symb));
			if (data->prod.graph_off && is_graphic_alphanumeric(p))
				p = parse_graphic_alphanumeric(p, &(data->graphic));
			if (data->prod.tab_off && is_tabular_alphanumeric(p))
				p = parse_tabular_alphanumeric(p, &(data->tab));
			break;
	}
	
	
	if (bzbuf)
		CPLFree(bzbuf);
	if (ubuf.buf)
		CPLFree(ubuf.buf);
	CPLFree(buf.buf);
  
}

void NIDS_CPLFree(NIDS *data) {
	CPLFree_msg_header(&(data->msg));
	CPLFree_prod_desc(&(data->prod));
	CPLFree_product_symbology(&(data->symb));
	CPLFree_graphic_alphanumeric(&(data->graphic));
	CPLFree_tabular_alphanumeric(&(data->tab));
}

void NIDS_print(NIDS *data) {
	char prefix[PREFIX_LEN];
	
	snprintf(prefix, PREFIX_LEN, "data");
	
	print_msg_header(&(data->msg), prefix);
	print_prod_desc(&(data->prod), prefix);
	print_product_symbology(&(data->symb), prefix);
	print_graphic_alphanumeric(&(data->graphic), prefix);
	print_tabular_alphanumeric(&(data->tab), prefix);
}

#define RASTER_X_SIZE 640
#define RASTER_Y_SIZE 640

char *NIDS_to_raster(
	NIDS *data,
	int *width,
	int *height)
{
	NIDS_image im;
	
	im.raster = NULL;
	im.x_center = RASTER_X_SIZE / 2;
	im.y_center = RASTER_Y_SIZE / 2;
	*width = im.width = RASTER_X_SIZE;
	*height = im.height = RASTER_Y_SIZE;
	im.scale = 1;
	
	
	if (!(im.raster = calloc(RASTER_X_SIZE, RASTER_Y_SIZE)))
		ERROR("NIDS_to_raster");

	product_symbology_to_raster(&im, &(data->symb));
	graphic_alphanumeric_to_raster(&im, &(data->graphic));
	
	return im.raster;
}




void NIDS_get_color(
	NIDS *data,
	NIDS_color **colors)
{
	
	get_product_dependent_color(data->msg.code, colors);
	
	return;
}
	
