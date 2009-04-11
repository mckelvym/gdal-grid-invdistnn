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
    virtual ~Mutex() {}

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
    virtual void   *Open( const char *filename, const char *access ) const = 0;
    virtual uint64  Seek( void *io_handle, uint64 offset, int whence ) const = 0;
    virtual uint64  Tell( void *io_handle ) const = 0;
    virtual uint64  Read( void *buffer, uint64 size, uint64 nmemb, void *io_handle ) const = 0;
    virtual uint64  Write( const void *buffer, uint64 size, uint64 nmemb, void *io_handle ) const = 0;
    virtual int     Eof( void *io_handle ) const = 0;
    virtual int     Flush( void *io_handle ) const = 0;
    virtual int     Close( void *io_handle ) const = 0;
};

const IOInterfaces PCIDSK_DLL *GetDefaultIOInterfaces();

/************************************************************************/
/*                           PCIDSKInterfaces                           */
/************************************************************************/
class PCIDSK_DLL PCIDSKInterfaces 
{
  public:
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
class PCIDSK_DLL PCIDSKObject 
{
protected:
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
class PCIDSK_DLL PCIDSKChannel : public PCIDSKObject
{
    friend class PCIDSKFile;

protected:
    virtual ~PCIDSKChannel() {};

public:
    virtual int GetBlockWidth() = 0;
    virtual int GetBlockHeight() = 0;
    virtual int GetWidth() = 0;
    virtual int GetHeight() = 0;
    virtual eChanType GetType() = 0;
    virtual int ReadBlock( int block_index, void *buffer ) = 0;
    virtual int WriteBlock( int block_index, void *buffer ) = 0;
    virtual int GetOverviewCount() = 0;
    virtual PCIDSKChannel *GetOverview( int i ) = 0;
};

/************************************************************************/
/*                              PCIDSKFile                              */
/************************************************************************/
class PCIDSK_DLL PCIDSKFile : PCIDSKObject
{
public:
    virtual ~PCIDSKFile() {};

    virtual PCIDSKInterfaces *GetInterfaces() = 0;

    virtual PCIDSKChannel  *GetChannel( int band ) = 0;
    virtual PCIDSKObject   *GetSegment( int segment ) = 0;
    virtual std::vector<PCIDSKObject *> GetObjects() = 0;

    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual int GetChannels() const = 0;
    
    virtual void WriteToFile( const void *buffer, uint64 offset, uint64 size)=0;
    virtual void ReadFromFile( void *buffer, uint64 offset, uint64 size ) = 0;
};

PCIDSKFile PCIDSK_DLL *Open( const char *filename, const char *access,  
                             const PCIDSKInterfaces *interfaces );

int PCIDSK_DLL DataTypeSize( eChanType );

}; // end of PCIDSK namespace

#endif // PCIDSK_H_INCLUDED
