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
#include <time.h>

#include "NIDS.h"
#include "get.h"
#include "prod_desc.h"
#include "error.h"

/*******************************************************************************
2. Product Description Block

Halfword	Name											Description
10				Divider										Integer value -1
11				Latitude of Radar (MSW)		Latitude*1000, + is north
12				Latitude (LSW) 	 
13				Longitude of Radar (MSW)	Longitude*1000, + is east
14				Longitude (LSW) 	 
15				Height of Radar						Feet above sea level
16				Product Code							See message code table below
17				Operational Mode					0=Maintenance, 1=Clear air,
																		2=Precipitation/Severe weather
18				Volume Scan Pattern				RDA volume coverage pattern
19				Sequence Number						Sequence number of the request that
																		generated the product
20				Volume Scan Number				Counter, cycles 1 to 80
21				Volume Scan Date 	 
22				Volume Scan Time (MSW) 	 
23				Time (LSW) 	 
24				Product Generation Date 	 
25				Product Generation Time (MSW) 	 
26				Time (LSW) 	 
27				P1												Product specific codes, see product codes
																		below
28				P2 	 
29				Elevation Number					Elevation scan within volume scan
30				P3 	 
31				Data 1 Threshold					Data threshold values, see threshold
																		discussion below
32				Data 2 Threshold 	 
33				Data 3 Threshold 	 
34				Data 4 Threshold 	 
35				Data 5 Threshold 	 
36				Data 6 Threshold 	 
37				Data 7 Threshold 	 
38				Data 8 Threshold 	 
39				Data 9 Threshold 	 
40				Data 10 Threshold 	 
41				Data 11 Threshold 	 
42				Data 12 Threshold 	 
43				Data 13 Threshold 	 
44				Data 14 Threshold 	 
45				Data 15 Threshold
46				Data 16 Threshold 	 
47				P4 	 
48				P5 	 
49				P6 	 
50				P7 	 
51				P8 	 
52				P9 	 
53				P10 	 
54				Number of Maps 	 
55				Offset to Symbology Block (MSW) 	Number of halfwords (2 bytes) from
																						start of product to block
56				Offset (LSW) 	 
57				Offset to Graphic Block (MSW) 	 
58				Offset (LSW) 	 
59				Offset to Tabular Block (MSW) 	 
60b				Offset (LSW) 	 
*******************************************************************************/

/*******************************************************************************
	function to parse the product desc
	
args:					buf			the buffer pointing to the start of the product desc
							d				the structure to store the product desc in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_prod_desc(char *buf, NIDS_prod_desc *d) {

	d->lat = GET4(buf + 2);
	d->lat /= 1000;
	d->lon = GET4(buf + 6);
	d->lon /= 1000;
	d->elev = GET2(buf + 10);
	d->code = GET2(buf + 12);
	d->mode = GET2(buf + 14);
	d->vol_scan_pat = GET2(buf + 16);
	d->seq_num = GET2(buf + 18);
	d->scan_num = GET2(buf + 20);
	d->scan_time = GET2(buf + 22);
	d->scan_time *= 24 * 60 * 60;
	d->scan_time += GET4(buf + 24);
	d->prod_time = GET2(buf + 28);
	d->prod_time *= 24 * 60 * 60;
	d->prod_time += GET4(buf + 30);
	d->codes[0] = GET2(buf + 34);
	d->codes[1] = GET2(buf + 36);
	d->elev_num = GET2(buf + 38);
	d->codes[2] = GET2(buf + 40);
	d->thresholds[0] = GET2(buf + 42);
	d->thresholds[1] = GET2(buf + 44);
	d->thresholds[2] = GET2(buf + 46);
	d->thresholds[3] = GET2(buf + 48);
	d->thresholds[4] = GET2(buf + 50);
	d->thresholds[5] = GET2(buf + 52);
	d->thresholds[6] = GET2(buf + 54);
	d->thresholds[7] = GET2(buf + 56);
	d->thresholds[8] = GET2(buf + 58);
	d->thresholds[9] = GET2(buf + 60);
	d->thresholds[10] = GET2(buf + 62);
	d->thresholds[11] = GET2(buf + 64);
	d->thresholds[12] = GET2(buf + 66);
	d->thresholds[13] = GET2(buf + 68);
	d->thresholds[14] = GET2(buf + 70);
	d->thresholds[15] = GET2(buf + 72);
	d->codes[3] = GET2(buf + 74);
	d->codes[4] = GET2(buf + 76);
	d->codes[5] = GET2(buf + 78);
	d->codes[6] = GET2(buf + 80);
	d->codes[7] = GET2(buf + 82);
	d->codes[8] = GET2(buf + 84);
	d->codes[9] = GET2(buf + 86);
	d->num_maps = GET2(buf + 88);
	d->symbol_off = GET4(buf + 90);
	d->graph_off = GET4(buf + 94);
	d->tab_off = GET4(buf + 98);
	
	return buf + 102;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in product desc storage

args:
						d				the structure the product desc is stored in

returns:
						nothing
*******************************************************************************/

void free_prod_desc(NIDS_prod_desc *d) {

}

/*******************************************************************************
	function to print the product desc

args:
						d				the structure the product desc is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_prod_desc(NIDS_prod_desc *d, char *prefix) {
	int i;

	printf("%s.prod.lat %f\n", prefix, d->lat);
	printf("%s.prod.lon %f\n", prefix, d->lon);
	printf("%s.prod.elev %i\n", prefix, d->elev);
	printf("%s.prod.codes %i\n", prefix, d->code);
	printf("%s.prod.mode %i\n", prefix, d->mode);
	printf("%s.prod.vol_scan_pat %i\n", prefix, d->vol_scan_pat);
	printf("%s.prod.seq_num %i\n", prefix, d->seq_num);
	printf("%s.prod.scan_num %i\n", prefix, d->scan_num);
	printf("%s.prod.scan_time %lu\n", prefix, d->scan_time);
	printf("%s.prod.prod_time %lu\n", prefix, d->prod_time);
	
	for (i = 0 ; i < 10 ; i++)
		printf("%s.prod.codes[%i] %i\n", prefix, i, d->codes[i]);
	
	printf("%s.prod.elev_num %i\n", prefix, d->elev_num);
				 
	for (i = 0 ; i < 16 ; i++)
		printf("%s.prod.thresholds[%i] %i\n", prefix, i, d->thresholds[i]);
	
	printf("%s.prod.num_maps %i\n", prefix, d->num_maps);
	printf("%s.prod.sybol_off %u\n", prefix, d->symbol_off);
	printf("%s.prod.graph_off %u\n", prefix, d->graph_off);
	printf("%s.prod.tab_off %u\n", prefix, d->tab_off);
}

