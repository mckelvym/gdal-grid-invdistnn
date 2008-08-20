/******************************************************************************
 *
 * Project:  TMS Driver
 * Purpose:  Declare TMS related utility functions.
 * Author:   Vaclav Klusak (keo at keo.cz)
 *
 ******************************************************************************
 * Copyright (c) 2008, Vaclav Klusak
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

#ifndef TMSUTIL_H_INCLUDED
#define TMSUTIL_H_INCLUDED

#include "cpl_minixml.h"

#define mkarray(type,size)  (( type * ) CPLMalloc (sizeof (type) * (size)))
#define forall(i,count)     for (int i = 0; i < count; i++)

#define succes      return true
#define fail        return false
#define must(expr)  if (!(expr)) { fail; }

#define bool_to_CE_Error(expr)  ((expr) ? CE_None : CE_Failure)

#define assert(expr)    CPLAssert (expr)
#define assert_in_range(expr, bound)    assert ((expr) >= 0 && (expr) < (bound))


bool GetXMLValue (const char *desc, CPLXMLNode *xml, const char *attr, 
                  const char **result, int err_no);
                  
bool ParseDouble (const char *desc, const char *str, double *result, int err_no);
bool ParseLong   (const char *desc, const char *str, long *result, int err_no);

bool InRangeDouble (const char *desc, double val, double min, double max, int err_no);
bool InRangeLong   (const char *desc, long val, long min, long max, int err_no);

bool CreateDir (const char *dirname);
bool OpenFileWrite (const char *name, FILE **result);

#endif

