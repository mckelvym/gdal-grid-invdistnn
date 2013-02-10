/******************************************************************************
 *
 * Project:  NIDS (level 3 nexrad radar) translator
 * Purpose:  Implements nidsDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2012, Brian Case
 *
 * Permission is hereby granted, CPLFree of charge, to any person obtaining a
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
#include "error.h"
#include "msg_header.h"
#include "prod_desc.h"
#include "tabular_alphanumeric.h"



/*******************************************************************************
TABULAR ALPHANUMERIC BLOCK (see Note 3)
                                                                      PRECISION/
FIELDNAME                          TYPE         UNITS   RANGE         ACCURACY      REMARKS
Block Divider                      INT*2        N/A     -1            N/A            Integer value of -1
                                                                                    used to delineate the
                                                                                    Tabular Alphanumeric
                                                                                    Block
Block ID                           INT*2        N/A     3             N/A            Constant value of 3
                                                                                    which identifies this
                                                                                    block
Length of Block                    INT*4        Bytes   1 to 65535    1             Length of block in
                                                                                    bytes from the divider
                                                                                    to the end of message
------------------------------------------ SECOND MESSAGE HEADER BLOCK ---------------------------------------------
--------------------------------------------- SECOND PRODUCT DESCRIPTION BLOCK ----------------------------------
Block Divider               INT*2        N/A     -1            N/A            Integer value of -1
                                                                             used to delineate the
                                                                             data from the Product
                                                                             Description Block
Number of Pages             INT*2        N/A     1 to 48       1             Total number of pages
Number of Characters INT*2               N/A     0 to 80       1             Number of characters
                                                                             in a line
Character Data              CHAR         8 Bit   ASCII         N/A           Characters are ASCII
                                         ASCII   Character Set               when the MSB is set
                                                                                    to zero. When the
                                                                                    MSB is set to one, the
                                                                                    remaining 7 bits
                                                                                    define the special
                                                                                    symbol
End of Page Flag                   INT*2        N/A     -1            N/A           Integer value of -1 to
                                                                                    delineate the end of
                                                                                    page
Note 3. Tabular Alphanumeric Block must be the last block in a product message. Maximum lines
per page = 17. Alphanumeric Products containing RPG Site Adaptable Parameters must have the
Site Adaptable Parameters formatted as the last page(s) of the Product.
                                 Figure 3-6. Graphic Product Message (Sheet 10)

                         MSB      HALFWORD          LSB
       TABULAR
                                 BLOCK DIVIDER (-1)
                                    BLOCK ID (3)
       ALPHANUMERIC
                            LENGTH OF BLOCK (MSW)
       BLOCK
                             LENGTH OF BLOCK (LSW)
                                                          SECOND
                            MESSAGE HEADER BLOCK          HEADER
                                   (see Figure 3-3)
                                                          AND
                                                          PRODUCT
                          PRODUCT DESCRIPTION BLOCK
                                     (see sheet 2)        DESCRIPTION
                                                          BLOCK
                                                          DATA
                                BLOCK DIVIDER (-1)        FORMATTED
                                                          AS
                                NUMBER OF PAGES           ALPHANUMERIC
REPEAT REPEAT                                             PRODUCT
                            NUMBER OF CHARACTERS          MESSAGE
FOR    FOR
EACH   EACH
                                CHARACTER DATA
PAGE   LINE
                              END OF PAGE FLAG (-1)
            Figure 3-6. Graphic Product Message (Sheet 5)

*******************************************************************************/

char *parse_tabular_alphanumeric_line (char *buf, NIDS_tabular_alphanumeric_line *t) {
	char *p = buf;
	
	t->line_len = GET2(buf);
	p += 2;
	
	memset(t->line, 0, sizeof(t->line));
	
	strncpy(t->line, p, t->line_len);
	
	p += t->line_len;
	
	return p;
}

char *parse_tabular_alphanumeric_page (char *buf, NIDS_tabular_alphanumeric_page *t) {
	char *p;
	int i;
	NIDS_tabular_alphanumeric_line *temp;
	
	p = buf;
	
	t->lines = NULL;
	t->num_lines = 0;
	
	for (i = 0; GET2(p) != 65535 ; i++) {
		t->num_lines++;
		
		if (!(temp = (NIDS_tabular_alphanumeric_line*)CPLRealloc(t->lines, t->num_lines * sizeof(NIDS_tabular_alphanumeric_line))))
			ERROR("parse_tabular_alphanumeric_page");
		t->lines = temp;
		
		p = parse_tabular_alphanumeric_line(p, t->lines + i);
	}
	
	p += 2;
	
	return p;
}

/*******************************************************************************
	function to parse the tabular alphanumeric block

args:					buf			the buffer pointing to the start of the block
							t				the structure to store the block in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_tabular_alphanumeric(char *buf, NIDS_tabular_alphanumeric *t) {
	char *p;
	int i;
	
	t->id = GET2(buf + 2);
	t->length = GET4(buf + 4);
	
	p = buf + 8;
	
	p = parse_msg_header(p, &(t->msg));
	p = parse_prod_desc(p, &(t->prod));
	
	p+= 2;
	
	t->num_pages = GET2(p);
	printf("num_pages %i\n", t->num_pages);
	if (!(t->pages = (NIDS_tabular_alphanumeric_page*)CPLMalloc(t->num_pages * sizeof(NIDS_tabular_alphanumeric_page))))
		ERROR("parse_tabular_alphanumeric");
	
	p += 2;
	
	for (i = 0 ; i < t->num_pages ; i++) {
		p = parse_tabular_alphanumeric_page(p, t->pages + i);
	}
	
	return p;
}
	
/*******************************************************************************
	function to check if a block is a tabular alphanumeric block
	
args:
					buf 				the buffer pointing to the start of the block

returns:
					1 	if the block is a graphic_alphanumeric block
					0		if it is not
*******************************************************************************/

int is_tabular_alphanumeric(char *buf) {
	int result = 0;
	
	if (GET2(buf + 2) == 3)
		result = 1;
	
	return result;
}


/*******************************************************************************
	function to CPLFree a tabular alphanumeric page

							t				pointer to a page

returns:
							nothing
*******************************************************************************/

void free_tabular_alphanumeric_page(NIDS_tabular_alphanumeric_page *t) {
	
	CPLFree(t->lines);
	
}

/*******************************************************************************
	function to CPLFree the tabular alphanumeric block

							t				pointer to the block

returns:
							nothing
*******************************************************************************/

void free_tabular_alphanumeric(NIDS_tabular_alphanumeric *t) {
	int i;
	
	for (i = 0 ; i < t->num_pages ; i++)
		free_tabular_alphanumeric_page(t->pages + i);
	
	CPLFree(t->pages);
	
}

/*******************************************************************************
	function to print a tabular_alphanumeric page

args:
						t				the structure the page is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_tabular_alphanumeric_line(NIDS_tabular_alphanumeric_line *t, char *prefix) {
	
	printf("%s.line_len %i\n", prefix, t->line_len);
	printf("%s.line %s\n", prefix, t->line);
	
}

/*******************************************************************************
	function to print a tabular_alphanumeric page

args:
						t				the structure the page is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_tabular_alphanumeric_page(NIDS_tabular_alphanumeric_page *t, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.num_lines %i\n", prefix, t->num_lines);
	
	for (i = 0 ; i < t->num_lines ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.lines[%i]", prefix, i);
		print_tabular_alphanumeric_line(t->lines + i, myprefix);
	}
}

/*******************************************************************************
	function to print the tabular_alphanumeric block

args:
						t				the structure the block is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_tabular_alphanumeric(NIDS_tabular_alphanumeric *t, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.tab.id %i\n", prefix, t->id);
	printf("%s.tab.length %u\n", prefix, t->length);
	
	snprintf(myprefix, PREFIX_LEN, "%s.tab", prefix);
	print_msg_header(&(t->msg), myprefix);
	print_prod_desc(&(t->prod), myprefix);
	
	printf("%s.tab.num_pages %i\n", prefix, t->num_pages);
	
	for (i = 0 ; i < t->num_pages ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.tab.pages[%i]", prefix, i);
		print_tabular_alphanumeric_page(t->pages + i, myprefix);
	}
}
	
