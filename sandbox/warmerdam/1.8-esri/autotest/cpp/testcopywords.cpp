/******************************************************************************
 * $Id$
 *
 * Project:  GDAL Core
 * Purpose:  Test GDALCopyWords().
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

#include <iostream>
#include <gdal.h>

char* pIn;
char* pOut;
int bErr = FALSE;

template <class OutType, class ConstantType>
void AssertRes(GDALDataType intype, ConstantType inval, GDALDataType outtype, ConstantType expected_outval, OutType outval, int numLine)
{
    if (fabs((double)outval - (double)expected_outval) > .1)
    {
        std::cout << "Test failed at line " << numLine <<
                     " (intype=" << GDALGetDataTypeName(intype) << 
                     ",inval=" << inval <<
                     ",outtype=" << GDALGetDataTypeName(outtype) << 
                     ",got " << outval <<
                     " expected  " << expected_outval << std::endl;
        bErr = TRUE;
    }
}

#define ASSERT(intype, inval, outtype, expected_outval, outval ) \
    AssertRes(intype, inval, outtype, expected_outval, outval, numLine)


template <class InType, class OutType, class ConstantType>
void Test(GDALDataType intype, ConstantType inval, ConstantType invali,
                 GDALDataType outtype, ConstantType outval, ConstantType outvali,
                 int numLine)
{
    memset(pIn, 0xff, 128);
    memset(pOut, 0xff, 128);

    *(InType*)(pIn) = (InType)inval;
    *(InType*)(pIn + 32) = (InType)inval;
    if (GDALDataTypeIsComplex(intype))
    {
        ((InType*)(pIn))[1] = (InType)invali;
        ((InType*)(pIn + 32))[1] = (InType)invali;
    }

    /* Test positive offsets */
    GDALCopyWords(pIn, intype, 32, pOut, outtype, 32, 2);

    /* Test negative offsets */
    GDALCopyWords(pIn + 32, intype, -32, pOut + 128 - 16, outtype, -32, 2);

    ASSERT(intype, inval, outtype, outval, *(OutType*)(pOut));
    ASSERT(intype, inval, outtype, outval, *(OutType*)(pOut + 32));
    ASSERT(intype, inval, outtype, outval, *(OutType*)(pOut + 128 - 16));
    ASSERT(intype, inval, outtype, outval, *(OutType*)(pOut + 128 - 16 - 32));

    if (GDALDataTypeIsComplex(outtype))
    {
        ASSERT(intype, invali, outtype, outvali, ((OutType*)(pOut))[1]);
        ASSERT(intype, invali, outtype, outvali, ((OutType*)(pOut + 32))[1]);

        ASSERT(intype, invali, outtype, outvali, ((OutType*)(pOut + 128 - 16))[1]);
        ASSERT(intype, invali, outtype, outvali, ((OutType*)(pOut + 128 - 16 - 32))[1]);
    }
}

template <class InType, class ConstantType> void FromR_2(GDALDataType intype, ConstantType inval, ConstantType invali, GDALDataType outtype, ConstantType outval, ConstantType outvali, int numLine)
{
    if (outtype == GDT_Byte) 
        Test<InType,GByte,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_Int16) 
        Test<InType,GInt16,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_UInt16) 
        Test<InType,GUInt16,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_Int32) 
        Test<InType,GInt32,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_UInt32) 
        Test<InType,GUInt32,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_Float32) 
        Test<InType,float,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_Float64) 
        Test<InType,double,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_CInt16) 
        Test<InType,GInt16,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_CInt32) 
        Test<InType,GInt32,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_CFloat32) 
        Test<InType,float,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (outtype == GDT_CFloat64) 
        Test<InType,double,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
}

template<class ConstantType>
void FromR(GDALDataType intype, ConstantType inval, ConstantType invali, GDALDataType outtype, ConstantType outval, ConstantType outvali, int numLine)
{
    if (intype == GDT_Byte) 
        FromR_2<GByte,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_Int16) 
        FromR_2<GInt16,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_UInt16) 
        FromR_2<GUInt16,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_Int32) 
        FromR_2<GInt32,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_UInt32) 
        FromR_2<GUInt32,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_Float32) 
        FromR_2<float,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_Float64) 
        FromR_2<double,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_CInt16) 
        FromR_2<GInt16,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_CInt32) 
        FromR_2<GInt32,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_CFloat32) 
        FromR_2<float,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
    else if (intype == GDT_CFloat64) 
        FromR_2<double,ConstantType>(intype, inval, invali, outtype, outval, outvali, numLine); 
}


#define FROM_R(intype, inval, outtype, outval) FromR<GIntBig>(intype, inval, 0, outtype, outval, 0, __LINE__)
#define FROM_R_F(intype, inval, outtype, outval) FromR<double>(intype, inval, 0, outtype, outval, 0, __LINE__)

#define FROM_C(intype, inval, invali, outtype, outval, outvali) FromR<GIntBig>(intype, inval, invali, outtype, outval, outvali, __LINE__)
#define FROM_C_F(intype, inval, invali, outtype, outval, outvali) FromR<double>(intype, inval, invali, outtype, outval, outvali, __LINE__)

#define IS_UNSIGNED(x) (x == GDT_Byte || x == GDT_UInt16 || x == GDT_UInt32)
#define IS_FLOAT(x) (x == GDT_Float32 || x == GDT_Float64 || x == GDT_CFloat32 || x == GDT_CFloat64)

int i;
GDALDataType outtype;

#if defined(__MSVCRT__) || (defined(WIN32) && defined(_MSC_VER))
#define CST_3000000000 3000000000I64
#define CST_5000000000 5000000000I64
#else
#define CST_3000000000 3000000000LL
#define CST_5000000000 5000000000LL
#endif

void check_GDT_Byte()
{
    /* GDT_Byte */
    for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
    {
        FROM_R(GDT_Byte, 0, outtype, 0);
        FROM_R(GDT_Byte, 127, outtype, 127);
        FROM_R(GDT_Byte, 255, outtype, 255);
    }
}

void check_GDT_Int16()
{
    /* GDT_Int16 */
    FROM_R(GDT_Int16, -32000, GDT_Byte, 0); /* clamp */
    FROM_R(GDT_Int16, -32000, GDT_Int16, -32000);
    FROM_R(GDT_Int16, -32000, GDT_UInt16, 0); /* clamp */
    FROM_R(GDT_Int16, -32000, GDT_Int32, -32000);
    FROM_R(GDT_Int16, -32000, GDT_UInt32, 0); /* clamp */
    FROM_R(GDT_Int16, -32000, GDT_Float32, -32000);
    FROM_R(GDT_Int16, -32000, GDT_Float64, -32000);
    FROM_R(GDT_Int16, -32000, GDT_CInt16, -32000);
    FROM_R(GDT_Int16, -32000, GDT_CInt32, -32000);
    FROM_R(GDT_Int16, -32000, GDT_CFloat32, -32000);
    FROM_R(GDT_Int16, -32000, GDT_CFloat64, -32000);
    for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
    {
        FROM_R(GDT_Int16, 127, outtype, 127);
    }
    
    FROM_R(GDT_Int16, 32000, GDT_Byte, 255); /* clamp */
    FROM_R(GDT_Int16, 32000, GDT_Int16, 32000);
    FROM_R(GDT_Int16, 32000, GDT_UInt16, 32000);
    FROM_R(GDT_Int16, 32000, GDT_Int32, 32000);
    FROM_R(GDT_Int16, 32000, GDT_UInt32, 32000);
    FROM_R(GDT_Int16, 32000, GDT_Float32, 32000);
    FROM_R(GDT_Int16, 32000, GDT_Float64, 32000);
    FROM_R(GDT_Int16, 32000, GDT_CInt16, 32000);
    FROM_R(GDT_Int16, 32000, GDT_CInt32, 32000);
    FROM_R(GDT_Int16, 32000, GDT_CFloat32, 32000);
    FROM_R(GDT_Int16, 32000, GDT_CFloat64, 32000);
}

void check_GDT_UInt16()
{
    /* GDT_UInt16 */
    for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
    {
        FROM_R(GDT_UInt16, 0, outtype, 0);
        FROM_R(GDT_UInt16, 127, outtype, 127);
    }
    
    FROM_R(GDT_UInt16, 65000, GDT_Byte, 255); /* clamp */
    FROM_R(GDT_UInt16, 65000, GDT_Int16, 32767); /* clamp */
    FROM_R(GDT_UInt16, 65000, GDT_UInt16, 65000);
    FROM_R(GDT_UInt16, 65000, GDT_Int32, 65000);
    FROM_R(GDT_UInt16, 65000, GDT_UInt32, 65000);
    FROM_R(GDT_UInt16, 65000, GDT_Float32, 65000);
    FROM_R(GDT_UInt16, 65000, GDT_Float64, 65000);
    FROM_R(GDT_UInt16, 65000, GDT_CInt16, 32767); /* clamp */
    FROM_R(GDT_UInt16, 65000, GDT_CInt32, 65000);
    FROM_R(GDT_UInt16, 65000, GDT_CFloat32, 65000);
    FROM_R(GDT_UInt16, 65000, GDT_CFloat64, 65000);
}

void check_GDT_Int32()
{
    /* GDT_Int32 */
    FROM_R(GDT_Int32, -33000, GDT_Byte, 0); /* clamp */
    FROM_R(GDT_Int32, -33000, GDT_Int16, -32768); /* clamp */
    FROM_R(GDT_Int32, -33000, GDT_UInt16, 0); /* clamp */
    FROM_R(GDT_Int32, -33000, GDT_Int32, -33000); /* clamp */
    FROM_R(GDT_Int32, -33000, GDT_UInt32, 0); /* clamp */
    FROM_R(GDT_Int32, -33000, GDT_Float32, -33000);
    FROM_R(GDT_Int32, -33000, GDT_Float64, -33000);
    FROM_R(GDT_Int32, -33000, GDT_CInt16, -32768); /* clamp */
    FROM_R(GDT_Int32, -33000, GDT_CInt32, -33000);
    FROM_R(GDT_Int32, -33000, GDT_CFloat32, -33000);
    FROM_R(GDT_Int32, -33000, GDT_CFloat64, -33000);
    for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
    {
        FROM_R(GDT_Int32, 127, outtype, 127);
    }
    
    FROM_R(GDT_Int32, 67000, GDT_Byte, 255); /* clamp */
    FROM_R(GDT_Int32, 67000, GDT_Int16, 32767);  /* clamp */
    FROM_R(GDT_Int32, 67000, GDT_UInt16, 65535);  /* clamp */
    FROM_R(GDT_Int32, 67000, GDT_Int32, 67000);
    FROM_R(GDT_Int32, 67000, GDT_UInt32, 67000);
    FROM_R(GDT_Int32, 67000, GDT_Float32, 67000);
    FROM_R(GDT_Int32, 67000, GDT_Float64, 67000);
    FROM_R(GDT_Int32, 67000, GDT_CInt16, 32767);  /* clamp */
    FROM_R(GDT_Int32, 67000, GDT_CInt32, 67000);
    FROM_R(GDT_Int32, 67000, GDT_CFloat32, 67000);
    FROM_R(GDT_Int32, 67000, GDT_CFloat64, 67000);
}

void check_GDT_UInt32()
{
    /* GDT_UInt32 */
    for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
    {
        FROM_R(GDT_UInt32, 0, outtype, 0);
        FROM_R(GDT_UInt32, 127, outtype, 127);
    }
    
    FROM_R(GDT_UInt32, 3000000000U, GDT_Byte, 255); /* clamp */
    FROM_R(GDT_UInt32, 3000000000U, GDT_Int16, 32767);  /* clamp */
    FROM_R(GDT_UInt32, 3000000000U, GDT_UInt16, 65535);  /* clamp */
    FROM_R(GDT_UInt32, 3000000000U, GDT_Int32, 2147483647);  /* clamp */
    FROM_R(GDT_UInt32, 3000000000U, GDT_UInt32, 3000000000U);
    FROM_R(GDT_UInt32, 3000000000U, GDT_Float32, 3000000000U);
    FROM_R(GDT_UInt32, 3000000000U, GDT_Float64, 3000000000U);
    FROM_R(GDT_UInt32, 3000000000U, GDT_CInt16, 32767);  /* clamp */
    FROM_R(GDT_UInt32, 3000000000U, GDT_CInt32, 2147483647);  /* clamp */
    FROM_R(GDT_UInt32, 3000000000U, GDT_CFloat32, 3000000000U);
    FROM_R(GDT_UInt32, 3000000000U, GDT_CFloat64, 3000000000U);
}

void check_GDT_Float32and64()
{
    /* GDT_Float32 and GDT_Float64 */
    for(i=0;i<2;i++)
    {
        GDALDataType intype = (i == 0) ? GDT_Float32 : GDT_Float64;
        for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
        {
            if (IS_FLOAT(outtype))
            {
                FROM_R_F(intype, 127.1, outtype, 127.1);
                FROM_R_F(intype, -127.1, outtype, -127.1);
            }
            else
            {
                FROM_R_F(intype, 127.1, outtype, 127);
                FROM_R_F(intype, 127.9, outtype, 128);
                if (!IS_UNSIGNED(outtype))
                {
                    FROM_R_F(intype, -125.9, outtype, -126);
                    FROM_R_F(intype, -127.1, outtype, -127);
                }
            }
        }
        FROM_R(intype, -1, GDT_Byte, 0);
        FROM_R(intype, 256, GDT_Byte, 255);
        FROM_R(intype, -33000, GDT_Int16, -32768);
        FROM_R(intype, 33000, GDT_Int16, 32767);
        FROM_R(intype, -1, GDT_UInt16, 0);
        FROM_R(intype, 66000, GDT_UInt16, 65535);
        FROM_R(intype, -CST_3000000000, GDT_Int32, INT_MIN);
        FROM_R(intype, CST_3000000000, GDT_Int32, 2147483647);
        FROM_R(intype, -1, GDT_UInt32, 0);
        FROM_R(intype, CST_5000000000, GDT_UInt32, 4294967295UL);
        FROM_R(intype, CST_5000000000, GDT_Float32, CST_5000000000);
        FROM_R(intype, -CST_5000000000, GDT_Float32, -CST_5000000000);
        FROM_R(intype, CST_5000000000, GDT_Float64, CST_5000000000);
        FROM_R(intype, -CST_5000000000, GDT_Float64, -CST_5000000000);
        FROM_R(intype, -33000, GDT_CInt16, -32768);
        FROM_R(intype, 33000, GDT_CInt16, 32767);
        FROM_R(intype, -CST_3000000000, GDT_CInt32, INT_MIN);
        FROM_R(intype, CST_3000000000, GDT_CInt32, 2147483647);
        FROM_R(intype, CST_5000000000, GDT_CFloat32, CST_5000000000);
        FROM_R(intype, -CST_5000000000, GDT_CFloat32, -CST_5000000000);
        FROM_R(intype, CST_5000000000, GDT_CFloat64, CST_5000000000);
        FROM_R(intype, -CST_5000000000, GDT_CFloat64, -CST_5000000000);
    }
}

void check_GDT_CInt16()
{
    /* GDT_CInt16 */
    FROM_C(GDT_CInt16, -32000, -32500, GDT_Byte, 0, 0); /* clamp */
    FROM_C(GDT_CInt16, -32000, -32500, GDT_Int16, -32000, 0);
    FROM_C(GDT_CInt16, -32000, -32500, GDT_UInt16, 0, 0); /* clamp */
    FROM_C(GDT_CInt16, -32000, -32500, GDT_Int32, -32000, 0);
    FROM_C(GDT_CInt16, -32000, -32500, GDT_UInt32, 0,0); /* clamp */
    FROM_C(GDT_CInt16, -32000, -32500, GDT_Float32, -32000, 0);
    FROM_C(GDT_CInt16, -32000, -32500, GDT_Float64, -32000, 0);
    FROM_C(GDT_CInt16, -32000, -32500, GDT_CInt16, -32000, -32500);
    FROM_C(GDT_CInt16, -32000, -32500, GDT_CInt32, -32000, -32500);
    FROM_C(GDT_CInt16, -32000, -32500, GDT_CFloat32, -32000, -32500);
    FROM_C(GDT_CInt16, -32000, -32500, GDT_CFloat64, -32000, -32500);
    for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
    {
        FROM_C(GDT_CInt16, 127, 128, outtype, 127, 128);
    }
    
    FROM_C(GDT_CInt16, 32000, 32500, GDT_Byte, 255, 0); /* clamp */
    FROM_C(GDT_CInt16, 32000, 32500, GDT_Int16, 32000, 0);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_UInt16, 32000, 0);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_Int32, 32000, 0);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_UInt32, 32000, 0);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_Float32, 32000, 0);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_Float64, 32000, 0);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_CInt16, 32000, 32500);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_CInt32, 32000, 32500);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_CFloat32, 32000, 32500);
    FROM_C(GDT_CInt16, 32000, 32500, GDT_CFloat64, 32000, 32500);
}

void check_GDT_CInt32()
{
    /* GDT_CInt32 */
    FROM_C(GDT_CInt32, -33000, -33500, GDT_Byte, 0, 0); /* clamp */
    FROM_C(GDT_CInt32, -33000, -33500, GDT_Int16, -32768, 0); /* clamp */
    FROM_C(GDT_CInt32, -33000, -33500, GDT_UInt16, 0, 0); /* clamp */
    FROM_C(GDT_CInt32, -33000, -33500, GDT_Int32, -33000, 0);
    FROM_C(GDT_CInt32, -33000, -33500, GDT_UInt32, 0,0); /* clamp */
    FROM_C(GDT_CInt32, -33000, -33500, GDT_Float32, -33000, 0);
    FROM_C(GDT_CInt32, -33000, -33500, GDT_Float64, -33000, 0);
    FROM_C(GDT_CInt32, -33000, -33500, GDT_CInt16, -32768, -32768); /* clamp */
    FROM_C(GDT_CInt32, -33000, -33500, GDT_CInt32, -33000, -33500);
    FROM_C(GDT_CInt32, -33000, -33500, GDT_CFloat32, -33000, -33500);
    FROM_C(GDT_CInt32, -33000, -33500, GDT_CFloat64, -33000, -33500);
    for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
    {
        FROM_C(GDT_CInt32, 127, 128, outtype, 127, 128);
    }
    
    FROM_C(GDT_CInt32, 67000, 67500, GDT_Byte, 255, 0); /* clamp */
    FROM_C(GDT_CInt32, 67000, 67500, GDT_Int16, 32767, 0); /* clamp */
    FROM_C(GDT_CInt32, 67000, 67500, GDT_UInt16, 65535, 0); /* clamp */
    FROM_C(GDT_CInt32, 67000, 67500, GDT_Int32, 67000, 0);
    FROM_C(GDT_CInt32, 67000, 67500, GDT_UInt32, 67000, 0);
    FROM_C(GDT_CInt32, 67000, 67500, GDT_Float32, 67000, 0);
    FROM_C(GDT_CInt32, 67000, 67500, GDT_Float64, 67000, 0);
    FROM_C(GDT_CInt32, 67000, 67500, GDT_CInt16, 32767, 32767); /* clamp */
    FROM_C(GDT_CInt32, 67000, 67500, GDT_CInt32, 67000, 67500);
    FROM_C(GDT_CInt32, 67000, 67500, GDT_CFloat32, 67000, 67500);
    FROM_C(GDT_CInt32, 67000, 67500, GDT_CFloat64, 67000, 67500);
}

void check_GDT_CFloat32and64()
{
    /* GDT_CFloat32 and GDT_CFloat64 */
    for(i=0;i<2;i++)
    {
        GDALDataType intype = (i == 0) ? GDT_CFloat32 : GDT_CFloat64;
        for(outtype=GDT_Byte; outtype<=GDT_CFloat64;outtype = (GDALDataType)(outtype + 1))
        {
            if (IS_FLOAT(outtype))
            {
                FROM_C_F(intype, 127.1, 127.9, outtype, 127.1, 127.9);
                FROM_C_F(intype, -127.1, -127.9, outtype, -127.1, -127.9);
            }
            else
            {
                FROM_C_F(intype, 127.1, 150.9, outtype, 127, 151);
                FROM_C_F(intype, 127.9, 150.1, outtype, 128, 150);
                if (!IS_UNSIGNED(outtype))
                {
                    FROM_C_F(intype, -125.9, -127.1, outtype, -126, -127);
                }
            }
        }
        FROM_C(intype, -1, 256, GDT_Byte, 0, 0);
        FROM_C(intype, 256, -1, GDT_Byte, 255, 0);
        FROM_C(intype, -33000, 33000, GDT_Int16, -32768, 0);
        FROM_C(intype, 33000, -33000, GDT_Int16, 32767, 0);
        FROM_C(intype, -1, 66000, GDT_UInt16, 0, 0);
        FROM_C(intype, 66000, -1, GDT_UInt16, 65535, 0);
        FROM_C(intype, -CST_3000000000, -CST_3000000000, GDT_Int32, INT_MIN, 0);
        FROM_C(intype, CST_3000000000, CST_3000000000, GDT_Int32, 2147483647, 0);
        FROM_C(intype, -1, CST_5000000000, GDT_UInt32, 0, 0);
        FROM_C(intype, CST_5000000000, -1, GDT_UInt32, 4294967295UL, 0);
        FROM_C(intype, CST_5000000000, -1, GDT_Float32, CST_5000000000, 0);
        FROM_C(intype, CST_5000000000, -1, GDT_Float64, CST_5000000000, 0);
        FROM_C(intype, -CST_5000000000, -1, GDT_Float32, -CST_5000000000, 0);
        FROM_C(intype, -CST_5000000000, -1, GDT_Float64, -CST_5000000000, 0);
        FROM_C(intype, -33000, 33000, GDT_CInt16, -32768, 32767);
        FROM_C(intype, 33000, -33000, GDT_CInt16, 32767, -32768);
        FROM_C(intype, -CST_3000000000, -CST_3000000000, GDT_CInt32, INT_MIN, INT_MIN);
        FROM_C(intype, CST_3000000000, CST_3000000000, GDT_CInt32, 2147483647, 2147483647);
        FROM_C(intype, CST_5000000000, -CST_5000000000, GDT_CFloat32, CST_5000000000, -CST_5000000000);
        FROM_C(intype, CST_5000000000, -CST_5000000000, GDT_CFloat64, CST_5000000000, -CST_5000000000);
    }
}

int main(int argc, char* argv[])
{
    pIn = (char*)malloc(128);
    pOut = (char*)malloc(128);
    
    check_GDT_Byte();
    check_GDT_Int16();
    check_GDT_UInt16();
    check_GDT_Int32();
    check_GDT_UInt32();
    check_GDT_Float32and64();
    check_GDT_CInt16();
    check_GDT_CInt32();
    check_GDT_CFloat32and64();
    
    free(pIn);
    free(pOut);
    
    if (bErr == FALSE)
        printf("success !\n");
    else
        printf("fail !\n");
    
    return (bErr == FALSE) ? 0 : -1;
}
