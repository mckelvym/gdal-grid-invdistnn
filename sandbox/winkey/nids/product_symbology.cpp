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
#include <errno.h>
#include <time.h>

#include "NIDS.h"
#include "get.h"
#include "image.h"
#include "radial.h"
#include "raster.h"
#include "arrow.h"
#include "tvs.h"
#include "etvs.h"
#include "hail_positive.h"
#include "hail_probable.h"
#include "point.h"
#include "point_feature.h"
#include "barb.h"
#include "hail.h"
#include "d_precip.h"
#include "precip.h"
#include "vector.h"
#include "v_vector.h"
#include "d_radial.h"
#include "circle.h"
#include "mesocyclone.h"
#include "text.h"
#include "v_text.h"
#include "storm_id.h"
#include "linked_vector.h"
#include "v_linked_vector.h"
#include "forecast.h"
#include "product_symbology.h"
#include "error.h"


char *parse_symbology_packet (char *buf, NIDS_symbology_packet *l) {
	char *p;
	
	l->data_type = GET2(buf);
		
	p = buf + 2;
	
	switch (l->data_type) {
		case RADIAL:
			p = parse_radial_header(p, &(l->rad));
			break;
		
		case D_RADIAL:
			p = parse_d_radial_header(p, &(l->d_radial));
			break;
		
		case RASTER1:
		case RASTER2:
			p = parse_raster_header(p, &(l->rast));
			break;
		
		case ARROW:
			p = parse_arrow_header(p, &(l->arrow));
			break;
		
		case POINT_FEATURE:
			p = parse_point_feature_header(p, &(l->point_feature));
			break;
		
		case TVS:
			p = parse_tvs_header(p, &(l->tvs));
			break;
		
		case ETVS:
			p = parse_etvs_header(p, &(l->etvs));
			break;
		
		case HAIL_POSITIVE:
			p = parse_hail_positive_header(p, &(l->hail_positive));
			break;
		
		case HAIL_PROBABLE:
			p = parse_hail_probable_header(p, &(l->hail_probable));
			break;
		
		case BARB:
			p = parse_barb_header(p, &(l->barb));
			break;
		
		case HAIL:
			p = parse_hail_header(p, &(l->hail));
			break;
		
		case D_PRECIP:
			p = parse_d_precip_header(p, &(l->d_precip));
			break;
	
		case PRECIP:
			p = parse_precip_header(p, &(l->precip));
			break;
	
		case VECTOR:
			p = parse_vector_header(p, &(l->vector));
			break;
		
		case V_VECTOR:
			p = parse_v_vector_header(p, &(l->v_vector));
			break;
		
		case MESOCYCLONE1:
		case MESOCYCLONE2:
			p = parse_mesocyclone_header(p, &(l->mesocyclone));
			break;
		
		case CIRCLE3:
			p = parse_circle_header(p, &(l->circle));
			break;
		
		case TEXT1:
		case TEXT2:
			p = parse_text_header(p, &(l->text));
			break;
		
		case V_TEXT:
			p = parse_v_text_header(p, &(l->v_text));
			break;
		
		case STORM_ID:
			p = parse_storm_id_header(p, &(l->storm_id));
			break;
		
		case LINKED_VECTOR:
			p = parse_linked_vector_header(p, &(l->linked_vector));
			break;
		
		case V_LINKED_VECTOR:
			p = parse_v_linked_vector_header(p, &(l->v_linked_vector));
			break;
		
		case FORECAST1:
		case FORECAST2:
			p = parse_forecast_header(p, &(l->forecast));
			break;
		
		default:
			printf("unknown symbology layer 0x%04x\n", l->data_type);
						 
	}
	
	return p;
}

/*******************************************************************************
Symbology Layer
Within the layer, there are a number of data packets which describe the type of 
information to be plotted.

01 	Divider 	 
02 	Length of Block (MSW) 	Length in bytes
03 	Length (LSW) 	 
04-## 	Display Data Packets 	 
*******************************************************************************/


/*******************************************************************************
	function to parse a symbology layer
	
args:
							buf			the buffer pointing to the first byte of the layer
							l				the struct to store the layer in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_symbology_layer (char *buf, NIDS_symbology_layer *l) {
	char *p;
	int i;
	NIDS_symbology_packet *temp;
	
	l->length = GET4(buf + 2);
	p = buf + 6;
	
	l->packets = NULL;
	
	l->num_packets = 0;
	
	for (i = 0; p - buf + 6 < l->length; i++) {
		
		l->num_packets++;
		
		if (!(temp = (NIDS_symbology_packet*)CPLRealloc(l->packets, l->num_packets * sizeof(NIDS_symbology_packet))))
			ERROR("parse_symbology_layer");
		l->packets = temp;
		
		p = parse_symbology_packet(p, l->packets + i);
	}
	
	return p;
}

/*******************************************************************************
3. Product Symbology Block

The Symbology Block has a header plus a number of data layers. 
These layers can be graphical such as radial and rastor scan data or could be
textual or symbolic such as TVS locations.

01 	Divider 	 
02 	Block ID 	Integer value 1
03 	Length of Block (MSW) 	Length in bytes
04 	Length (LSW) 	 
05 	Number of Layers 	 
*******************************************************************************/

/*******************************************************************************
	function to parse the Product Symbology
	
args:					buf			the buffer pointing to the start of the Product Symbology
							s				the structure to store the Product Symbology in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_product_symbology(char *buf, NIDS_product_symbology *s) {
	int i;
	char *p;
	
	s->id = GET2(buf + 2);
	s->length = GET4(buf + 4);
	s->num_layers = GET2(buf + 8);
	
	if (!(s->layers = (NIDS_symbology_layer*)CPLMalloc(s->num_layers * sizeof(NIDS_symbology_layer))))
		ERROR("parse_product_symbology");
	
	p = buf + 10;
	
	for (i = 0 ; i < s->num_layers ; i++) {
		
		p = parse_symbology_layer(p, s->layers + i);
		//p += s->layers[i].length;
	}
			
	return p;
}

/*******************************************************************************
	function to check if a block is a product_symbology block
	
args:
					buf 				the buffer pointing to the start of the block

returns:
					1 	if the block is a product_symbology block
					0		if it is not
*******************************************************************************/

int is_product_symbology(char *buf) {
	int result = 0;
	
	if (GET2(buf + 2) == 1)
		result = 1;
	
	return result;
}
	
/*******************************************************************************
	function to free all the memory used in a symbology packet

args:
						l				the structure the packet is stored in

returns:
						nothing
*******************************************************************************/

void free_symbology_packet (NIDS_symbology_packet *l) {
		
	switch (l->data_type) {
		case RADIAL:
			free_radial_header(&(l->rad));
			break;
		
		case D_RADIAL:
			free_d_radial_header(&(l->d_radial));
			break;
		
		case RASTER1:
		case RASTER2:
			free_raster_header(&(l->rast));
			break;
		
		case ARROW:
			free_arrow_header(&(l->arrow));
			break;
		
		case POINT_FEATURE:
			free_point_feature_header(&(l->point_feature));
			break;
		
		case TVS:
			free_tvs_header(&(l->tvs));
			break;
		
		case ETVS:
			free_etvs_header(&(l->etvs));
			break;
		
		case HAIL_POSITIVE:
			free_hail_positive_header(&(l->hail_positive));
			break;
		
		case HAIL_PROBABLE:
			free_hail_probable_header(&(l->hail_probable));
			break;
		
		case BARB:
			free_barb_header(&(l->barb));
			break;
		
		case HAIL:
			free_hail_header(&(l->hail));
			break;
		
		case D_PRECIP:
			free_d_precip_header(&(l->d_precip));
			break;
		
		case PRECIP:
			free_precip_header(&(l->precip));
			break;
		
		case VECTOR:
			free_vector_header(&(l->vector));
			break;
		
		case V_VECTOR:
			free_v_vector_header(&(l->v_vector));
			break;
		
		case MESOCYCLONE1:
		case MESOCYCLONE2:
			free_mesocyclone_header(&(l->mesocyclone));
			break;
		
		case CIRCLE3:
			free_circle_header(&(l->circle));
			break;
		
		case TEXT1:
		case TEXT2:
			free_text_header(&(l->text));
			break;
		
		case V_TEXT:
			free_v_text_header(&(l->v_text));
			break;
		
		case STORM_ID:
			free_storm_id_header(&(l->storm_id));
			break;
		
		case LINKED_VECTOR:
			free_linked_vector_header(&(l->linked_vector));
			break;
		
		case V_LINKED_VECTOR:
			free_v_linked_vector_header(&(l->v_linked_vector));
			break;
		
		case FORECAST1:
		case FORECAST2:
			free_forecast_header(&(l->forecast));
			break;
		
		default:
			fprintf(stderr, "unknown symbology layer %04x\n", l->data_type);
						 
	}
}

/*******************************************************************************
	function to CPLFree all the memory used in a symbology layer

args:
						l				the structure the layer is stored in

returns:
						nothing
*******************************************************************************/

void free_symbology_layer (NIDS_symbology_layer *l) {
	int i;
	
	for (i = 0 ; i < l->num_packets ; i++)
		free_symbology_packet(l->packets + i);
	
	CPLFree(l->packets);
	
}
/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in product symbology storage

args:
						h				the structure the product symbology is stored in

returns:
						nothing
*******************************************************************************/

void free_product_symbology(NIDS_product_symbology *s) {
	int i;
	
	for (i = 0 ; i < s->num_layers ; i++)
		free_symbology_layer(s->layers + i);

	CPLFree(s->layers);
}

/*******************************************************************************
	function to print a symbology packet

args:
						l				the structure the packet is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_symbology_packet (NIDS_symbology_packet *l, char *prefix) {
	
	printf("%s.data_type %04x\n", prefix, l->data_type);
	
	switch (l->data_type) {
		case RADIAL:
			print_radial_header(&(l->rad), prefix);
			break;
		
		case D_RADIAL:
			print_d_radial_header(&(l->d_radial), prefix);
			break;
		
		case RASTER1:
		case RASTER2:
			print_raster_header(&(l->rast), prefix);
			break;
		
		case ARROW:
			print_arrow_header(&(l->arrow), prefix);
			break;
		
		case POINT_FEATURE:
			print_point_feature_header(&(l->point_feature), prefix);
			break;
		
		case TVS:
			print_tvs_header(&(l->tvs), prefix);
			break;
		
		case ETVS:
			print_etvs_header(&(l->etvs), prefix);
			break;
		
		case HAIL_POSITIVE:
			print_hail_positive_header(&(l->hail_positive), prefix);
			break;
		
		case HAIL_PROBABLE:
			print_hail_probable_header(&(l->hail_probable), prefix);
			break;
		
		case BARB:
			print_barb_header(&(l->barb), prefix);
			break;
		
		case HAIL:
			print_hail_header(&(l->hail), prefix);
			break;
		
		case D_PRECIP:
			print_d_precip_header(&(l->d_precip), prefix);
			break;
		
		case PRECIP:
			print_precip_header(&(l->precip), prefix);
			break;
		
		case VECTOR:
			print_vector_header(&(l->vector), prefix);
			break;
		
		case V_VECTOR:
			print_v_vector_header(&(l->v_vector), prefix);
			break;
		
		case MESOCYCLONE1:
		case MESOCYCLONE2:
			print_mesocyclone_header(&(l->mesocyclone), prefix);
			break;
					
		case CIRCLE3:
			print_circle_header(&(l->circle), prefix);
			break;
		
		case TEXT1:
		case TEXT2:
			print_text_header(&(l->text), prefix);
			break;
		
		case V_TEXT:
			print_v_text_header(&(l->v_text), prefix);
			break;
		
		case STORM_ID:
			print_storm_id_header(&(l->storm_id), prefix);
			break;
		
		case LINKED_VECTOR:
			print_linked_vector_header(&(l->linked_vector), prefix);
			break;
		
		case V_LINKED_VECTOR:
			print_v_linked_vector_header(&(l->v_linked_vector), prefix);
			break;
		
		case FORECAST1:
		case FORECAST2:
			print_forecast_header(&(l->forecast), prefix);
			break;
		
		default:
			fprintf(stderr, "unknown symbology layer 0x%04x\n", l->data_type);
							 
	}
	
}

/*******************************************************************************
	function to print a symbology layer

args:
						l				the structure the layer is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_symbology_layer (NIDS_symbology_layer *l, char *prefix) {
	char myprefix[PREFIX_LEN];
	int i;
	
	printf ("%s.length %i\n", prefix, l->length);
	for (i = 0 ; i < l->num_packets ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.packets[%i]", prefix, i);
		print_symbology_packet(l->packets + i, myprefix);
	}
	
}

/*******************************************************************************
	function to print the product symbology

args:
						s				the structure the product desc is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_product_symbology(NIDS_product_symbology *s, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.symb.id %i\n", prefix, s->id);
	printf("%s.symb.length %u\n", prefix, s->length);
	printf("%s.symb.num_layers %i\n", prefix, s->num_layers);
	
	for (i = 0 ; i < s->num_layers ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.symb.layers[%i]", prefix, i);
		print_symbology_layer(s->layers + i, myprefix);
	}
}

/*******************************************************************************
	function to convert a symbology packet to a raster
*******************************************************************************/

void product_symbology_packet_to_raster (
	NIDS_image *im,
	NIDS_symbology_packet *l)
{
	
	switch (l->data_type) {
		case RADIAL:
			radials_to_raster(im, &(l->rad));
			break;
		
		case RASTER1:
		case RASTER2:
			rasters_to_raster(im, &(l->rast));
			break;
		
		case ARROW:
			//arrows_to_raster(im, &(l->arrow));
			break;
		
		case POINT_FEATURE:
			point_features_to_raster(im, &(l->point_feature));
			break;
		
		case TVS:
			tvss_to_raster(im, &(l->tvs));
			break;
		
		case ETVS:
			etvss_to_raster(im, &(l->etvs));
			break;
		
		case HAIL_POSITIVE:
			hail_positives_to_raster(im, &(l->hail_positive));
			break;
		
		case HAIL_PROBABLE:
			hail_probables_to_raster(im, &(l->hail_probable));
			break;
		
		case PRECIP:
			precips_to_raster(im, &(l->precip));
			break;
		
		case VECTOR:
			vectors_to_raster(im, &(l->vector));
			break;
		
		case V_VECTOR:
			v_vectors_to_raster(im, &(l->v_vector));
			break;
		
		case MESOCYCLONE1:
		case MESOCYCLONE2:
			mesocyclones_to_raster(im, &(l->mesocyclone));
			break;
		
			
		case CIRCLE3:
			circles_to_raster(im, &(l->circle));
			break;
		
		case TEXT1:
		case TEXT2:
			texts_to_raster(im, &(l->text));
			break;
		
		case V_TEXT:
			v_texts_to_raster(im, &(l->v_text));
			break;
		
		case STORM_ID:
			storm_ids_to_raster(im, &(l->storm_id));
			break;
		
		case BARB:
			barbs_to_raster(im, &(l->barb));
			break;
		
		case HAIL:
			hails_to_raster(im, &(l->hail));
			break;
		
		case LINKED_VECTOR:
			linked_vectors_to_raster(im, &(l->linked_vector));
			break;
		
		case V_LINKED_VECTOR:
			v_linked_vectors_to_raster(im, &(l->v_linked_vector));
			break;
		
		case FORECAST1:
		case FORECAST2:
			forecasts_to_raster(im, &(l->forecast));
			break;
		
		default:
			fprintf(stderr, "unknown symbology layer %04x\n", l->data_type);
							 
	}
	
}

/*******************************************************************************
	function to convert a symbology layer to a raster
*******************************************************************************/

void product_symbology_layer_to_raster (
	NIDS_image *im,
	NIDS_symbology_layer *l)
{
	int i;
	
	for (i = 0; i < l->num_packets ; i++)
		product_symbology_packet_to_raster(im, l->packets + i);
	
}

/*******************************************************************************
	function to convert the symbology block to a raster

args:
						s				the structure the product symbology is stored in

returns:
						nothing
*******************************************************************************/

void product_symbology_to_raster(
	NIDS_image *im,
	NIDS_product_symbology *s)
{
	int i;
	
	for (i = 0 ; i < s->num_layers ; i++)
		product_symbology_layer_to_raster(im, s->layers + i);
	
}

