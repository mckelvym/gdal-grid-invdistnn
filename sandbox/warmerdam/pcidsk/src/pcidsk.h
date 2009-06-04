/******************************************************************************
 *
 * Purpose:  Primary public include file for PCIDSK SDK.
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

/**
 * \file pcidsk.h
 *
 * Public PCIDSK library classes and functions.
 */

#ifndef PCIDSK_H_INCLUDED
#define PCIDSK_H_INCLUDED

#include "pcidsk_config.h"
#include <string>
#include <vector>
#include <exception>
#include <stdarg.h>

//! Namespace for all PCIDSK Library classes and functions.

namespace PCIDSK {

class PCIDSKSegment;
class PCIDSKChannel;

//! Channel pixel data types.
typedef enum {
    CHN_8U=0,     /*!< 8 bit unsigned byte */
    CHN_16S=1,    /*!< 16 bit signed integer */
    CHN_16U=2,    /*!< 16 bit unsigned integer */
    CHN_32R=3,    /*!< 32 bit ieee floating point */
    CHN_UNKNOWN=99 /*!< unknown channel type */
} eChanType;

//! Segment types.
typedef enum {
    SEG_UNKNOWN = -1, 

    SEG_BIT = 101,
    SEG_VEC = 116, 
    SEG_SIG = 121,
    SEG_TEX = 140,
    SEG_GEO = 150,
    SEG_ORB = 160,
    SEG_LUT = 170,
    SEG_PCT = 171,
    SEG_BLUT = 172,
    SEG_BPCT = 173,
    SEG_BIN = 180,
    SEG_ARR = 181,
    SEG_SYS = 182,
    SEG_GCPOLD = 214,
    SEG_GCP2 = 215,
} eSegType;

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

void PCIDSK_DLL ThrowPCIDSKException( const char *fmt, ... ); 

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

//! IO Interface class.

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

//! Collection of PCIDSK hookable interfaces.

class PCIDSK_DLL PCIDSKInterfaces 
{
  public:
    PCIDSKInterfaces();

    const IOInterfaces 	*io;
    Mutex               *(*CreateMutex)(void);
//    DBInterfaces 	db_interfaces;
};

/************************************************************************/
/*                              PCIDSKFile                              */
/************************************************************************/

//! Top interface to PCIDSK (.pix) files.

class PCIDSK_DLL PCIDSKFile
{
public:
    virtual ~PCIDSKFile() {};

    virtual PCIDSKInterfaces *GetInterfaces() = 0;

    virtual PCIDSKChannel  *GetChannel( int band ) = 0;
    virtual PCIDSKSegment  *GetSegment( int segment ) = 0;
    virtual std::vector<PCIDSKSegment *> GetSegments() = 0;

    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual int GetChannels() const = 0;
    virtual const char *GetInterleaving() const = 0;
    virtual bool GetUpdatable() const = 0;

    virtual int  CreateSegment( const char *name, const char *description,
                                eSegType seg_type, int data_blocks ) = 0;

    // the following are only for pixel interleaved IO
    virtual int    GetPixelGroupSize() const = 0;
    virtual void *ReadAndLockBlock( int block_index, int xoff=-1, int xsize=-1) = 0;
    virtual void  UnlockBlock( bool mark_dirty = false ) = 0;

    // low level io, primarily internal.
    virtual void WriteToFile( const void *buffer, uint64 offset, uint64 size)=0;
    virtual void ReadFromFile( void *buffer, uint64 offset, uint64 size ) = 0;

    virtual void GetIODetails( void ***io_handle_pp, Mutex ***io_mutex_pp,
                            const char *filename = "" ) = 0;

    virtual const char *GetMetadataValue( const char *key ) = 0;
    virtual std::vector<std::string> GetMetadataKeys() = 0;
};

/************************************************************************/
/*                            PCIDSKChannel                             */
/************************************************************************/

//! Interface to one PCIDSK channel (band).

class PCIDSK_DLL PCIDSKChannel 
{
public:
    virtual ~PCIDSKChannel() {};
    virtual int GetBlockWidth() = 0;
    virtual int GetBlockHeight() = 0;
    virtual int GetBlockCount() = 0;
    virtual int GetWidth() = 0;
    virtual int GetHeight() = 0;
    virtual eChanType GetType() = 0;
    virtual int ReadBlock( int block_index, void *buffer,
                           int win_xoff=-1, int win_yoff=-1,
                           int win_xsize=-1, int win_ysize=-1 ) = 0;
    virtual int WriteBlock( int block_index, void *buffer ) = 0;
    virtual int GetOverviewCount() = 0;
    virtual PCIDSKChannel *GetOverview( int i ) = 0;

    virtual const char *GetMetadataValue( const char *key ) = 0;
    virtual std::vector<std::string> GetMetadataKeys() = 0;
};

/************************************************************************/
/*                            PCIDSKSegment                             */
/************************************************************************/

//! Interface to one PCIDSK auxilary segment.

class PCIDSKSegment 
{
public:
    virtual	~PCIDSKSegment() {}

    virtual void WriteToFile( const void *buffer, uint64 offset, uint64 size)=0;
    virtual void ReadFromFile( void *buffer, uint64 offset, uint64 size ) = 0;

    virtual eSegType    GetSegmentType() = 0;
    virtual const char *GetName() = 0;
    virtual const char *GetDescription() = 0;
    virtual int         GetSegmentNumber() = 0;

    virtual const char *GetMetadataValue( const char *key ) = 0;
    virtual std::vector<std::string> GetMetadataKeys() = 0;
};

/************************************************************************/
/*                             PCIDSKGeoref                             */
/************************************************************************/

//! Interface to PCIDSK georeferencing segment.

class PCIDSK_DLL PCIDSKGeoref
{
public:
    virtual	~PCIDSKGeoref() {}

    virtual void GetTransform( double &a1, double &a2, double &xrot, 
                               double &b1, double &yrot, double &b3 ) = 0;
    virtual const char *GetGeosys() = 0;
    
    virtual void WriteSimple( const char *geosys, 
                              double a1, double a2, double xrot, 
                              double b1, double yrot, double b3 ) = 0;
};

/************************************************************************/
/*                        Supporting functions.                         */
/************************************************************************/

PCIDSKFile PCIDSK_DLL *Open( const char *filename, const char *access,  
                             const PCIDSKInterfaces *interfaces );
PCIDSKFile PCIDSK_DLL *Create( const char *filename, int pixels, int lines,
                               int channel_count, eChanType *channel_types, 
                               const char *options,
                               const PCIDSKInterfaces *interfaces );

int PCIDSK_DLL DataTypeSize( eChanType );
const char PCIDSK_DLL *DataTypeName( eChanType );
const char PCIDSK_DLL *SegmentTypeName( eSegType );

}; // end of PCIDSK namespace

#endif // PCIDSK_H_INCLUDED
