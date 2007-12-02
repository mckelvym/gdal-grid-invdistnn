/******************************************************************************
 * $Id: cpl.i 11420 2007-05-07 02:26:00Z warmerdam $
 *
 * Name:     cpl.i
 * Project:  GDAL Python Interface
 * Purpose:  GDAL Core SWIG Interface declarations.
 * Author:   Kevin Ruland, kruland@ku.edu
 *
 ******************************************************************************
 * Copyright (c) 2005, Kevin Ruland
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
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

%include cpl_renames.i

void CPLPushErrorHandler( CPLErrorHandler );

void CPLPopErrorHandler();

void CPLErrorReset();

int CPLGetLastErrorNo();

CPLErr CPLGetLastErrorType();

char const *CPLGetLastErrorMsg();

void CPLPushFinderLocation( const char * );

void CPLPopFinderLocation();

void CPLFinderClean();

const char * CPLFindFile( const char *, const char * );

%apply (char **options) {char **};
char **VSIReadDir( const char * );
%clear char **;

void CPLSetConfigOption( const char *, const char * );

const char * CPLGetConfigOption( const char *, const char * );

/* Provide hooks to hex encoding methods */
char *CPLBinaryToHex( int nBytes, const GByte *pabyData );
GByte *CPLHexToBinary( const char *pszHex, int *pnBytes );
