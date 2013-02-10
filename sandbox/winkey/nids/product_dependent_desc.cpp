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
#include "error.h"


void parse_product_dependent_desc(int msgcode, char *buf, prod_dep_desc *pdd) {
	
	switch (msgcode) {
		case 16:
		case 17:
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 43:
		case 44:
		case 45:
		case 46:
		case 55:
		case 56:
		case 87:
		case 93:
		case 132:
		case 133:
		case 139:
		case 143:
			pdd->elevation_angle = GET2(buf + 40);
			pdd->elevation_angle /= 10;
			break;
		
		case 32:
		case 134:
		case 135:
		case 136:
			pdd->compression = GET2(buf + 82);
			pdd->uncompressed_size = GET4(buf + 84);
			break;

		case 84:
			pdd->elevation_angle = GET2(buf + 78);
			pdd->elevation_angle /= 10;
			break;
		
		case 94:
		case 99:
		case 149:
			pdd->elevation_angle = GET2(buf + 40);
			pdd->elevation_angle /= 10;
			pdd->compression = GET2(buf + 82);
			pdd->uncompressed_size = GET4(buf + 84);
			break;

		case 152:
			pdd->compression = GET2(buf + 82);
			pdd->uncompressed_size = GET2(buf + 84);
			break;
	}
	
	
}

