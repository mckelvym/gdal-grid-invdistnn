/******************************************************************************
 *
 * Purpose:  Primary include file for PCIDSK SDK.
 * 
 ******************************************************************************
 * Copyright (c) 2009
 * PCI Geomatics, 50 West Wilmot Street, Richmond Hill, Ont, Canada
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

#ifndef PCIDSK_H_INCLUDED
#define PCIDSK_H_INCLUDED

#include "pcidsk_config.h"
#include <string>
#include <vector>
#include <exception>
#include <stdarg.h>

namespace PCIDSK {

/* -------------------------------------------------------------------- */
/*      Channel types.                                                  */
/* -------------------------------------------------------------------- */
typedef enum {
    CHN_8U,
    CHN_16U,
    CHN_16S,
    CHN_32R,
    CHN_UNKNOWN
} eChanType;

/************************************************************************/
/*                              Exception                               */
/************************************************************************/

class PCIDSKException : public std::exception
{
private:
    std::string   message;
public:
    PCIDSKException(const char *fmt, ... );
    virtual ~PCIDSKException() throw();

    void vPrintf( const char *fmt, va_list list );
    virtual const char *what() const throw() { return message.c_str(); }
};

/************************************************************************/
/*                                Mutex                                 */
/************************************************************************/
class Mutex
{
public:
    virtual ~Mutex() = 0;

    virtual int  Acquire() = 0;
    virtual int  Release() = 0;
};

Mutex PCIDSK_DLL *DefaultCreateMutex(void);

/************************************************************************/
/*                             IOInterfaces                             */
/************************************************************************/

class IOInterfaces
{
public:
    virtual ~IOInterfaces() {}
    virtual void   *Open( const char *filename, const char *access ) = 0;
    virtual uint64  Seek( void *io_handle, uint64 offset, int whence ) = 0;
    virtual uint64  Tell( void *io_handle ) = 0;
    virtual uint64  Read( void *buffer, uint64 size, uint64 nmemb, void *io_handle ) = 0;
    virtual uint64  Write( void *buffer, uint64 size, uint64 nmemb, void *io_handle ) = 0;
    virtual int     Eof( void *io_handle ) = 0;
    virtual int     Flush( void *io_handle ) = 0;
    virtual int     Close( void *io_handle ) = 0;
};

const IOInterfaces PCIDSK_DLL *GetDefaultIOInterfaces();

/************************************************************************/
/*                           PCIDSKInterfaces                           */
/************************************************************************/
class PCIDSK_DLL PCIDSKInterfaces 
{
    PCIDSKInterfaces();

    const IOInterfaces 	*io;
    Mutex               *(*CreateMutex)(void);

//    DBInterfaces 	db_interfaces;
//    ErrorInterfaces     error_interfaces;
};

class PCIDSKFile;

/************************************************************************/
/*                             PCIDSKObject                             */
/************************************************************************/
class PCIDSKObject 
{
private:
    PCIDSKFile  *file; // owner

public:
    PCIDSKObject();
    virtual ~PCIDSKObject();
  // metadata read/write
  // name
  // band/segment #
  // class (band, pct, etc)
};

/************************************************************************/
/*                            PCIDSKChannel                             */
/************************************************************************/
class PCIDSKChannel : PCIDSKObject
{
public:
    int       GetBlockWidth();
    int       GetBlockHeight();
    int       GetWidth();
    int       GetHeight();
    eChanType GetType();
    int       ReadBlock( int block_index, void *buffer );
    int       GetOverviewCount();
    PCIDSKChannel  *GetOverview( int i );
};

/************************************************************************/
/*                              PCIDSKFile                              */
/************************************************************************/
class PCIDSKFile : PCIDSKObject
{
private:
    PCIDSKInterfaces interfaces;
    
    void         *io_handle;

public:

    PCIDSKChannel  *GetBand( int band );
    PCIDSKObject   *GetSegment( int iSegment );
    std::vector<PCIDSKObject *> GetObjects();

    int       GetWidth();
    int       GetHeight();
    int       GetChannels();
    
    void     *GetIOHandle();

    static PCIDSKFile *PCIDSKOpen( const char *filename, const char *access,  
                                   const PCIDSKInterfaces *interfaces );
};

}; // end of PCIDSK namespace

#endif // PCIDSK_H_INCLUDED
