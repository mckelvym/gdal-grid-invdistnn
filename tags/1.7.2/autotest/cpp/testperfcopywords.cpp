/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Test performance of GDALCopyWords().
 * Author:   Even Rouault, <even dot rouault at mines dash paris dot org>
 *
 ******************************************************************************
 * Copyright (c) 2009, Even Rouault
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
 ****************************************************************************/
 
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "gdal.h"

int main(int argc, char* argv[])
{
    void* in = calloc(1, 256 * 256 * 16);
    void* out = malloc(256 * 256 * 16);
    
    int i;
    int intype, outtype;

    struct timeval tv1;
    struct timeval tv2;
    
    for(intype=GDT_Byte;intype<=GDT_CFloat64;intype++)
    {
        for(outtype=GDT_Byte;outtype<=GDT_CFloat64;outtype++)
        {
            gettimeofday(&tv1, NULL);
            
            for(i=0;i<1000;i++)
                GDALCopyWords(in, (GDALDataType)intype, 16, out, (GDALDataType)outtype, 16, 256 * 256);
                
            gettimeofday(&tv2, NULL);
            
            double diff = tv2.tv_sec - tv1.tv_sec + 1e-6 * (tv2.tv_usec - tv1.tv_usec);
            printf("%s -> %s : %.2f s\n",
                   GDALGetDataTypeName((GDALDataType)intype),
                   GDALGetDataTypeName((GDALDataType)outtype),
                   diff);
        }
    }

    return 0;
}

