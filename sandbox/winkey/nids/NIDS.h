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

#ifndef _NIDS_H
#define _NIDS_H

#include "cpl_conv.h"

/*******************************************************************************
	Message Header Block
*******************************************************************************/

typedef struct {
	int code;
	time_t time;
	size_t len;
	int s_id;
	int d_id;
	int num_blocks;
} NIDS_msg_header;

typedef struct {
	float elevation_angle;
	int compression;
	size_t uncompressed_size;
	
} prod_dep_desc;

/*******************************************************************************
	Product Description
*******************************************************************************/

typedef struct {
	int code;
	int ntr;
	char name[100];
	double xres;
	double yres;
	double dres;
	int range;
	int numlevels;
	char formats[100];
} NIDS_prod_info;

/*******************************************************************************
	Product Description Block
*******************************************************************************/

typedef struct {
	float lat;
	float lon;
	int elev;
	int code;
	int mode;
	int vol_scan_pat;
	int seq_num;
	int scan_num;
	time_t scan_time;
	time_t prod_time;
	int codes[10];
	int elev_num;
	int thresholds[16];
	int num_maps;
	size_t symbol_off;
	size_t graph_off;
	size_t tab_off;
} NIDS_prod_desc;

/*******************************************************************************
	Radials
*******************************************************************************/

typedef struct {
	int num_rle;
	float start;
	float delta;
	char *run;
	char *code;
} NIDS_radial;


typedef struct {
	int index_first_bin;
	int num_bins;
	signed short x_center;
	signed short y_center;
	int scale;
	int num_radials;
	NIDS_radial *radials;
} NIDS_radials;

/*******************************************************************************
	digital radial data array
*******************************************************************************/

typedef struct {
	int num_bytes;
	int num_bins;
	float start;
	float delta;
	char *level;
} NIDS_d_radial;

typedef struct {
	int index_first_bin;
	int num_bins;
	signed short x_center;
	signed short y_center;
	int scale;
	int num_radials;
	NIDS_d_radial *radials;
} NIDS_d_radials;

/*******************************************************************************
	raster
*******************************************************************************/

typedef struct {
	short num_rle;
	char *run;
	char *code;
} NIDS_raster_row;

typedef struct {
	int op_flags1;
	int op_flags2;
	signed short x_start;
	signed short y_start;
	int x_scale_int;
	int x_scale_fract;
	int y_scale_int;
	int y_scale_fract;
	int num_rows;
	int packing;
	NIDS_raster_row *rows;
} NIDS_raster;

/*******************************************************************************
	wind barbs
*******************************************************************************/

typedef struct {
	int value;
	signed short x_start;
	signed short y_start;
	int heading;
	int speed;
} NIDS_barb;

typedef struct {
	size_t length;
	int num_barbs;
	NIDS_barb *barbs;
} NIDS_barbs;

/*******************************************************************************
	hail
*******************************************************************************/

typedef struct {
	signed short x_start;
	signed short y_start;
	int prob;
	int severe;
	int max_size;
} NIDS_hail;

typedef struct {
	size_t length;
	int num_hails;
	NIDS_hail *hails;
} NIDS_hails;

/*******************************************************************************
	vector arrows
*******************************************************************************/

typedef struct {
	signed short x_start;
	signed short y_start;
	int heading;
	int length;
	int head_length;
} NIDS_arrow;

typedef struct {
	size_t length;
	int num_arrows;
	NIDS_arrow *arrows;
} NIDS_arrows;

/*******************************************************************************
	point features
*******************************************************************************/

typedef struct {
	signed short x_start;
	signed short y_start;
	int type;
	int attr;
} NIDS_point_feature;

typedef struct {
	size_t length;
	int num_points;
	NIDS_point_feature *points;
} NIDS_point_features;

/*******************************************************************************
	point
*******************************************************************************/

typedef struct {
	signed short x_start;
	signed short y_start;
} NIDS_point;

typedef struct {
	size_t length;
	int num_points;
	NIDS_point *points;
} NIDS_points;

typedef NIDS_points NIDS_tvss;
typedef NIDS_points NIDS_etvss;
typedef NIDS_points NIDS_hail_positives;
typedef NIDS_points NIDS_hail_probables;

/*******************************************************************************
	Unlinked Vector
*******************************************************************************/

typedef struct {
	signed short x_start;
	signed short y_start;
	signed short x_end;
	signed short y_end;
} NIDS_vector;

typedef struct {
	size_t length;
	int num_vectors;
	NIDS_vector *vectors;
} NIDS_vectors;

/*******************************************************************************
	Valued Unlinked Vector Packet
*******************************************************************************/

typedef struct {
	int value;
	signed short x_start;
	signed short y_start;
	signed short x_end;
	signed short y_end;
} NIDS_v_vector;

typedef struct {
	size_t length;
	int num_vectors;
	NIDS_v_vector *vectors;
} NIDS_v_vectors;

/*******************************************************************************
	circle
*******************************************************************************/

typedef struct {
	signed short x_start;
	signed short y_start;
	int radius;
} NIDS_circle;

typedef struct {
	size_t length;
	int num_circles;
	NIDS_circle *circles;
} NIDS_circles;


typedef NIDS_circle NIDS_mesocyclone;

typedef struct {
	size_t length;
	int num_mesocyclones;
	NIDS_mesocyclone *mesocyclones;
} NIDS_mesocyclones;

/*******************************************************************************
	Precipitation Array
*******************************************************************************/

typedef struct {
	short num_rle;
	char *run;
	char *code;
} NIDS_precip_row;

typedef struct {
	int op_flags1;
	int op_flags2;
	int lfm_per_row;
	int num_rows;
	NIDS_precip_row *rows;
} NIDS_precip;

/*******************************************************************************
	Digital Precipitation Array
*******************************************************************************/

typedef struct {
	short num_rle;
	char *run;
	char *code;
} NIDS_d_precip_row;

typedef struct {
	int op_flags1;
	int op_flags2;
	int lfm_per_row;
	int num_rows;
	NIDS_d_precip_row *rows;
} NIDS_d_precip;

/*******************************************************************************
	text packet
*******************************************************************************/

typedef struct {
	size_t length;
	int num_chars;
	signed short x_start;
	signed short y_start;
	char *chars;
} NIDS_text;

/*******************************************************************************
	valued text packet
*******************************************************************************/

typedef struct {
	size_t length;
	int num_chars;
	int value;
	signed short x_start;
	signed short y_start;
	char *chars;
} NIDS_v_text;

/*******************************************************************************
	Storm Id Packet
*******************************************************************************/

typedef struct {
	signed short x_pos;
	signed short y_pos;
	char id[3];
} NIDS_storm_id;

typedef struct {
	int length;
	int num_ids;
	NIDS_storm_id *ids;
} NIDS_storm_ids;

/*******************************************************************************
	linked vector
*******************************************************************************/

typedef NIDS_vectors NIDS_linked_vectors;

/*******************************************************************************
	value linked vector
*******************************************************************************/

typedef NIDS_v_vectors NIDS_v_linked_vectors;

/*******************************************************************************
	past or forecast packet
*******************************************************************************/

typedef struct {
	int data_type;
	union {
		NIDS_text text;
		NIDS_linked_vectors linked_vector;
		NIDS_circles circle;
	};
} NIDS_forecast;


typedef struct {
	size_t length;
	int num_forecasts;
	NIDS_forecast *forecasts;
} NIDS_forecasts;

/*******************************************************************************
	Symbology data type codes
*******************************************************************************/

#define TEXT1							0x0001
#define TEXT2							0x0002
#define MESOCYCLONE1 			0x0003
#define BARB							0x0004
#define ARROW							0x0005
#define LINKED_VECTOR			0x0006
#define VECTOR						0x0007
#define V_TEXT						0x0008
#define V_LINKED_VECTOR		0x0009
#define V_VECTOR					0x000A
#define MESOCYCLONE2			0x000B
#define TVS								0x000C
#define HAIL_POSITIVE			0x000D
#define HAIL_PROBABLE			0x000E
#define STORM_ID					0x000F
#define D_RADIAL					0x0010
#define D_PRECIP					0x0011
#define PRECIP						0x0012
#define HAIL							0x0013
#define POINT_FEATURE			0x0014

#define FORECAST1					0x0017
#define FORECAST2					0x0018
#define CIRCLE3						0x0019
#define ETVS							0x001A

#define RADIAL						0xAF1F
#define RASTER1						0xBA0F
#define RASTER2						0xBA07




/*******************************************************************************
	Symbology Layer
*******************************************************************************/

#define PREFIX_LEN 100

typedef struct {
	int data_type;
	union {
		NIDS_radials rad;
		NIDS_d_radials d_radial;
		NIDS_raster rast;
		NIDS_arrows arrow;
		NIDS_point_features point_feature;
		NIDS_points point;
		NIDS_tvss tvs;
		NIDS_etvss etvs;
		NIDS_hail_positives hail_positive;
		NIDS_hail_probables hail_probable;
		NIDS_barbs barb;
		NIDS_hails hail;
		NIDS_d_precip d_precip;
		NIDS_precip precip;
		NIDS_vectors vector;
		NIDS_v_vectors v_vector;
		NIDS_circles circle;
		NIDS_mesocyclones mesocyclone;
		NIDS_text text;
		NIDS_v_text v_text;
		NIDS_storm_ids storm_id;
		NIDS_linked_vectors linked_vector;
		NIDS_v_linked_vectors v_linked_vector;
		NIDS_forecasts forecast;
	};
} NIDS_symbology_packet;
	
typedef struct {
	size_t length;
	int num_packets;
	NIDS_symbology_packet *packets;
} NIDS_symbology_layer;


/*******************************************************************************
	Product Symbology
*******************************************************************************/

typedef struct {
	int id;
	size_t length;
	int num_layers;
	NIDS_symbology_layer *layers;
} NIDS_product_symbology;

/*******************************************************************************
	graphic alphanumeric block
*******************************************************************************/

typedef struct {
	int data_type;
	union {
		NIDS_v_vectors v_vector;
		NIDS_v_text v_text;
	};
} NIDS_graphic_alphanumeric_text;

typedef struct {
	int page;
	size_t length;
	int num_texts;
	NIDS_graphic_alphanumeric_text *texts;
} NIDS_graphic_alphanumeric_page;

typedef struct {
	int id;
	size_t length;
	int num_pages;
	NIDS_graphic_alphanumeric_page *pages;
} NIDS_graphic_alphanumeric;


/*******************************************************************************
	tabular alphanumeric block
*******************************************************************************/

typedef struct {
	size_t line_len;
	char line[81];
} NIDS_tabular_alphanumeric_line;

typedef struct {
	int num_lines;
	NIDS_tabular_alphanumeric_line *lines;
} NIDS_tabular_alphanumeric_page;

typedef struct {
	int id;
	size_t length;
	NIDS_msg_header msg;
	NIDS_prod_desc prod;
	int num_pages;
	NIDS_tabular_alphanumeric_page *pages;
} NIDS_tabular_alphanumeric;

/*******************************************************************************
	master struct to holdwhole the whole thing
********************************************************************************/

typedef struct {
	NIDS_msg_header msg;
	NIDS_prod_info *info; /**** we need to change this, its different *****/
	NIDS_prod_desc prod;
	prod_dep_desc pdd;
	NIDS_product_symbology symb;
	NIDS_graphic_alphanumeric graphic;
	NIDS_tabular_alphanumeric tab;
} NIDS;


FILE *NIDS_open(char *filename);

void NIDS_close(FILE *fp);

void NIDS_read(FILE *fp, NIDS *data);

void NIDS_CPLFree(NIDS *data);

void NIDS_print(NIDS *data);


char *NIDS_to_raster(
	NIDS *data,
	int *width,
	int *height);

typedef struct {
  float value;
  char color[7];
} NIDS_color;

void NIDS_get_color(
	NIDS *data,
	NIDS_color **colors);

/*******************************************************************************
	function to get the scale

	args:
							name				the name of the scale

	returns:
							the scale
	
	NOTE: you must CPLFree() the result when your done with it
*******************************************************************************/

NIDS_color *color_getscale(
	char *name);

/*******************************************************************************
	function to get a color from a scale

	args:
							scales			the scale to check
							value				the value to check for in the scale

	returns:
							the color for the value
*******************************************************************************/
 
char *color_checkscale(
	NIDS_color *scales,
	float value);



#endif /* _NIDS_H */


