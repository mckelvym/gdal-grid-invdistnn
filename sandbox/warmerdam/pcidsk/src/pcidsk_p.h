/******************************************************************************
 *
 * Purpose:  Private declarations for use within the PCIDSK SDK, but not by 
 *           clients.
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

#ifndef PCIDSKP_H_INCLUDED
#define PCIDSKP_H_INCLUDED

#include "pcidsk.h"

namespace PCIDSK {

class CPCIDSKFile;

/************************************************************************/
/*                             PCIDSKBuffer                             */
/*                                                                      */
/*      Convenience class for managing ascii headers of various         */
/*      sorts.  Primarily for internal use.                             */
/************************************************************************/

class PCIDSKBuffer 
{
private:
    std::string work_field;

public:
    PCIDSKBuffer( int size = 0 );
    ~PCIDSKBuffer();

    char	*buffer;
    int         buffer_size;

    const char *Get( int offset, int size );
    void        Get( int offset, int size, std::string &target, int unpad=1 );

    int64       GetInt64( int offset, int size );
    uint64      GetUInt64( int offset, int size );

    void        Put( const char *value,  int offset, int size );

    void        SetSize( int size );
};

/************************************************************************/
/*                              PCIDSKFile                              */
/************************************************************************/
class CPCIDSKFile : public PCIDSKFile
{
    friend PCIDSKFile PCIDSK_DLL *Open( const char *filename, 
                                        const char *access,  
                                        const PCIDSKInterfaces *interfaces );

private:
    PCIDSKInterfaces interfaces;
    
    void         InitializeFromHeader();

    void        *io_handle;
    Mutex       *io_mutex;

    int          width;
    int          height;
    int          channel_count;
    std::string  interleaving;

    std::vector<PCIDSKChannel*> channels;

    int          segment_count;
    PCIDSKBuffer segment_pointers;

    // pixel interleaved info.
    uint64       block_size; // pixel interleaved scanline size.
    int          pixel_group_size; // pixel interleaved pixel_offset value.
    uint64       first_line_offset;

    int          last_block_index;
    int          last_block_dirty;
    void        *last_block_data;
    Mutex       *last_block_mutex;

    // register of open external raw files.
    std::vector<std::string> 

public:

    CPCIDSKFile();
    virtual ~CPCIDSKFile();

    virtual PCIDSKInterfaces *GetInterfaces() { return &interfaces; }

    PCIDSKChannel  *GetChannel( int band );
    PCIDSK::PCIDSKObject   *GetSegment( int segment );
    std::vector<PCIDSK::PCIDSKObject *> GetObjects();

    int       GetWidth() const { return width; }
    int       GetHeight() const { return height; }
    int       GetChannels() const { return channel_count; }
    const char *GetInterleaving() const { return interleaving.c_str(); }
    
    // the following are only for pixel interleaved IO
    uint64    GetBlockSize() const { return block_size; }
    int       GetPixelGroupSize() const { return pixel_group_size; }
    void     *ReadAndLockBlock( int block_index );
    void      UnlockBlock( int mark_dirty );
    void      WriteBlock( int block_index, void *buffer );
    void      FlushBlock();

    void      WriteToFile( const void *buffer, uint64 offset, uint64 size );
    void      ReadFromFile( void *buffer, uint64 offset, uint64 size );

    void      GetIODetails( void ***io_handle_pp, Mutex ***io_mutex_pp )
        { *io_handle_pp = &io_handle; *io_mutex_pp = &io_mutex; }
    void      GetIODetails( void ***io_handle_pp, Mutex ***io_mutex_pp,
                            std::string &filename );
};

/************************************************************************/
/*                            CPCIDSKChannel                            */
/*                                                                      */
/*      This is not really a concrete class - it is necessary to        */
/*      instantiate a subclass the actually implements imagery io.      */
/************************************************************************/
class CPCIDSKChannel : public PCIDSKChannel
{
    friend class PCIDSKFile;

protected:
    eChanType pixel_type;
    char      byte_order; // 'S': littleendian, 'N': bigendian
    int       needs_swap;

    // width/height, and block size.
    int       width;
    int       height;
    int       block_width;
    int       block_height;

public:
    CPCIDSKChannel( PCIDSKBuffer &image_header, 
                    CPCIDSKFile *file, eChanType pixel_type );
    virtual   ~CPCIDSKChannel();

    int       GetBlockWidth() { return block_width; }
    int       GetBlockHeight() { return block_height; }
    int       GetWidth() { return width; }
    int       GetHeight() { return height; }
    eChanType GetType() { return pixel_type; }

    int       GetOverviewCount();
    PCIDSKChannel  *GetOverview( int i );
};

/************************************************************************/
/*                       CPixelInterleavedChannel                       */
/************************************************************************/

class CPixelInterleavedChannel : public CPCIDSKChannel
{
private:
    int      image_offset;

public:
    CPixelInterleavedChannel( PCIDSKBuffer &image_header, 
                              PCIDSKBuffer &file_header, 
                              int channelnum,
                              CPCIDSKFile *file,
                              int image_offset,
                              eChanType pixel_type );
    virtual ~CPixelInterleavedChannel();

    virtual int ReadBlock( int block_index, void *buffer );
    virtual int WriteBlock( int block_index, void *buffer );
};

/************************************************************************/
/*                       CBandInterleavedChannel                        */
/*                                                                      */
/*      Also used for FILE interleaved raw files.                       */
/************************************************************************/

class CBandInterleavedChannel : public CPCIDSKChannel
{
private:
    // raw file layout - internal or external
    uint64    start_byte;
    uint64    pixel_offset;
    uint64    line_offset;

    std::string filename;

    void     **io_handle_p;
    Mutex    **io_mutex_p;

public:
    CBandInterleavedChannel( PCIDSKBuffer &image_header, 
                             PCIDSKBuffer &file_header, 
                             int channelnum,
                             CPCIDSKFile *file,
                             uint64 image_offset,
                             eChanType pixel_type );
    virtual ~CBandInterleavedChannel();

    virtual int ReadBlock( int block_index, void *buffer );
    virtual int WriteBlock( int block_index, void *buffer );
};

/************************************************************************/
/*                             CGDBChannel                              */
/*                                                                      */
/*      External formatted files accessed through GDB or similar        */
/*      external format libraries.                                      */
/************************************************************************/

class CGDBChannel : public CPCIDSKChannel
{
private:
    // For external non-raw files - GDB linked.
    int       src_channel;
    int       src_xoff;
    int       src_yoff;
    int       src_xsize;
    int       src_ysize;

public:
    virtual int ReadBlock( int block_index, void *buffer );
    virtual int WriteBlock( int block_index, void *buffer );
};


/************************************************************************/
/*                            CTiledChannel                             */
/*                                                                      */
/*      Internal tiled data stored in special tiled imagery             */
/*      segments.  Imagery may be compressed.                           */
/************************************************************************/

class CTiledChannel : public CPCIDSKChannel
{
private:

public:
    virtual int ReadBlock( int block_index, void *buffer );
    virtual int WriteBlock( int block_index, void *buffer );
};


/************************************************************************/
/*                             MutexHolder                              */
/************************************************************************/
class PCIDSK_DLL MutexHolder
{
  private:
    Mutex     *mutex;

  public:
    MutexHolder( Mutex *mutex ) 
    { 
        this->mutex = mutex; 
        if( mutex != NULL )
            mutex->Acquire(); 
    }
    ~MutexHolder() 
    { 
        if( mutex )
            mutex->Release(); 
    }
};

/************************************************************************/
/*                          Utility functions.                          */
/************************************************************************/

uint64 atouint64( const char *);
int64  atoint64( const char *);
void   SwapData( void *data, int value_size, int value_count );

}; // end of PCIDSK namespace

#endif // PCIDSK_H_INCLUDED
