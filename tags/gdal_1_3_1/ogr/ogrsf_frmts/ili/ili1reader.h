/******************************************************************************
 * $Id$
 *
 * Project:  Interlis 1 Reader
 * Purpose:  Private Declarations for Reader code.
 * Author:   Pirmin Kalberer, Sourcepole AG
 *
 ******************************************************************************
 * Copyright (c) 2004, Pirmin Kalberer, Sourcepole AG
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.2  2005/08/06 22:21:53  pka
 * Area polygonizer added
 *
 * Revision 1.1  2005/07/08 22:10:57  pka
 * Initial import of OGR Interlis driver
 *
 */

#ifndef _CPL_ILI1READER_H_INCLUDED
#define _CPL_ILI1READER_H_INCLUDED


class CPL_DLL IILI1Reader
{
public:
    virtual     ~IILI1Reader();

    virtual int  OpenFile( const char *pszFilename ) = 0;
    
    virtual int  ReadModel( const char *pszModelFilename ) = 0;
    virtual int  ReadFeatures() = 0;       
    
    virtual OGRLayer *GetLayer( int ) = NULL;
    virtual int GetLayerCount() = 0;
};

IILI1Reader *CreateILI1Reader();

#endif
