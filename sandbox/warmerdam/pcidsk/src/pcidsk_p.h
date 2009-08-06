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

#include <map>

namespace PCIDSK {

class CPCIDSKFile;
class CTiledChannel;
class SysVirtualFile;
class MetadataSegment;

/************************************************************************/
/*                             PCIDSKBuffer                             */
/*                                                                      */
/*      Convenience class for managing ascii headers of various         */
/*      sorts.  Primarily for internal use.                             */
/************************************************************************/

class PCIDSKBuffer 
{
    friend class MetadataSegment;

private:
    std::string work_field;

public:
    PCIDSKBuffer( int size = 0 );
    PCIDSKBuffer( const char *src, int size );
    ~PCIDSKBuffer();

    char	*buffer;
    int         buffer_size;

    const char *Get( int offset, int size );
    void        Get( int offset, int size, std::string &target, int unpad=1 );

    double      GetDouble( int offset, int size );
    int         GetInt( int offset, int size );
    int64       GetInt64( int offset, int size );
    uint64      GetUInt64( int offset, int size );

    void        Put( const char *value,  int offset, int size );
    void        Put( uint64 value, int offset, int size );
    void        Put( double value, int offset, int size, const char *fmt=NULL );
    void        Put( int value, int offset, int size ) 
        { Put( (uint64) value, offset, size ); }

    void        SetSize( int size );
};

/************************************************************************/
/*                             MetadataSet                              */
/************************************************************************/

class MetadataSet 
{									
private:
    CPCIDSKFile  *file;

    bool         loaded;
    std::map<std::string,std::string> md_set;

    std::string  group;
    int          id;

    void         Load();

public:
    MetadataSet();
    ~MetadataSet();

    void        Initialize( CPCIDSKFile *file, std::string group, int id );
    std::string GetMetadataValue( std::string key );
    std::vector<std::string> GetMetadataKeys();
    
    void        SetMetadataValue( std::string key, std::string value );
};

/************************************************************************/
/*                            ProtectedFile                             */
/************************************************************************/
class ProtectedFile
{
  public:
    std::string     filename;
    void           *io_handle;
    Mutex          *io_mutex;
};
 
/************************************************************************/
/*                              PCIDSKFile                              */
/************************************************************************/
class CPCIDSKFile : public PCIDSKFile
{
    friend PCIDSKFile PCIDSK_DLL *Open( std::string filename, 
                                        std::string access,  
                                        const PCIDSKInterfaces *interfaces );

private:
    PCIDSKInterfaces interfaces;
    
    void         InitializeFromHeader();

    int          width;
    int          height;
    int          channel_count;
    std::string  interleaving;

    std::vector<PCIDSKChannel*> channels;

    int          segment_count;
    uint64       segment_pointers_offset;
    PCIDSKBuffer segment_pointers;

    std::vector<PCIDSKSegment*> segments;

    // pixel interleaved info.
    uint64       block_size; // pixel interleaved scanline size.
    int          pixel_group_size; // pixel interleaved pixel_offset value.
    uint64       first_line_offset;

    int          last_block_index;
    bool         last_block_dirty;
    int          last_block_xoff;
    int          last_block_xsize;
    void        *last_block_data;
    Mutex       *last_block_mutex;

    void        *io_handle;
    Mutex       *io_mutex;
    bool         updatable;

    uint64       file_size; // in blocks.
    
    // register of open external raw files.
    std::vector<ProtectedFile>  file_list;

    MetadataSet  metadata;
public:

    CPCIDSKFile();
    virtual ~CPCIDSKFile();

    virtual PCIDSKInterfaces *GetInterfaces() { return &interfaces; }

    PCIDSKChannel  *GetChannel( int band );
    PCIDSK::PCIDSKSegment   *GetSegment( int segment );
    std::vector<PCIDSK::PCIDSKSegment *> GetSegments();

    PCIDSK::PCIDSKSegment  *GetSegment( int type, const char *name );
    int  CreateSegment( std::string name, std::string description,
                        eSegType seg_type, int data_blocks );

    int       GetWidth() const { return width; }
    int       GetHeight() const { return height; }
    int       GetChannels() const { return channel_count; }
    std::string GetInterleaving() const { return interleaving; }
    bool      GetUpdatable() const { return updatable; } 
    
    // the following are only for pixel interleaved IO
    int       GetPixelGroupSize() const { return pixel_group_size; }
    void     *ReadAndLockBlock( int block_index, int xoff=-1, int xsize=-1 );
    void      UnlockBlock( bool mark_dirty = false );
    void      WriteBlock( int block_index, void *buffer );
    void      FlushBlock();

    void      WriteToFile( const void *buffer, uint64 offset, uint64 size );
    void      ReadFromFile( void *buffer, uint64 offset, uint64 size );

    void      GetIODetails( void ***io_handle_pp, Mutex ***io_mutex_pp,
                            std::string filename = "" );

    std::string GetMetadataValue( std::string key ) 
		{ return metadata.GetMetadataValue(key); }
    std::vector<std::string> GetMetadataKeys() 
        	{ return metadata.GetMetadataKeys(); }

    // not exposed to applications.
    void      ExtendFile( uint64 blocks_requested, bool prezero = false );
    uint64    GetFileSize() const { return file_size; }
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
    CPCIDSKFile *file;
    MetadataSet  metadata;

    int       channel_number;
    eChanType pixel_type;
    char      byte_order; // 'S': littleendian, 'N': bigendian
    int       needs_swap;

    // width/height, and block size.
    int       width;
    int       height;
    int       block_width;
    int       block_height;

    // info about overviews;
    void      EstablishOverviewInfo();

    bool                         overviews_initialized;
    std::vector<std::string>     overview_infos;
    std::vector<CTiledChannel *> overview_bands;
    
public:
    CPCIDSKChannel( PCIDSKBuffer &image_header, 
                    CPCIDSKFile *file, eChanType pixel_type,
                    int channel_number );
    virtual   ~CPCIDSKChannel();

    virtual int GetBlockWidth() { return block_width; }
    virtual int GetBlockHeight() { return block_height; }
    virtual int GetBlockCount();

    virtual int GetWidth() { return width; }
    virtual int GetHeight() { return height; }
    virtual eChanType GetType() { return pixel_type; }

    int       GetOverviewCount();
    PCIDSKChannel  *GetOverview( int i );

    int         GetChannelNumber() { return channel_number; }

    std::string GetMetadataValue( std::string key ) 
		{ return metadata.GetMetadataValue(key); }
    std::vector<std::string> GetMetadataKeys() 
        	{ return metadata.GetMetadataKeys(); }
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

    virtual int ReadBlock( int block_index, void *buffer,
                           int xoff=-1, int yoff=-1,
                           int xsize=-1, int ysize=-1 );

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

    virtual int ReadBlock( int block_index, void *buffer,
                           int xoff=-1, int yoff=-1,
                           int xsize=-1, int ysize=-1 );
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
    virtual int ReadBlock( int block_index, void *buffer,
                           int xoff=-1, int yoff=-1,
                           int xsize=-1, int ysize=-1 );
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
    int                      image;

    SysVirtualFile          *vfile;

    std::string              compression;

    std::vector<uint64>      tile_offsets;
    std::vector<int>         tile_sizes;

    void                     EstablishAccess();
    void                     RLEDecompressBlock( PCIDSKBuffer &oCompressed,
                                                 PCIDSKBuffer &oDecompressed );

public:
    CTiledChannel( PCIDSKBuffer &image_header, 
                   PCIDSKBuffer &file_header, 
                   int channelnum,
                   CPCIDSKFile *file,
                   eChanType pixel_type );
    virtual ~CTiledChannel();

    virtual int GetBlockWidth();
    virtual int GetBlockHeight();
    virtual int GetWidth();
    virtual int GetHeight();
    virtual eChanType GetType();

    virtual int ReadBlock( int block_index, void *buffer,
                           int xoff=-1, int yoff=-1,
                           int xsize=-1, int ysize=-1 );
    virtual int WriteBlock( int block_index, void *buffer );
};

/************************************************************************/
/*                            CPCIDSKSegment                            */
/*                                                                      */
/*      Base class for accessing all segments.  Provides core           */
/*      PCIDSKObject implementation for segments with raw segment io    */
/*      options.                                                        */
/************************************************************************/

class CPCIDSKSegment : public PCIDSKSegment
{
protected:
    CPCIDSKFile *file;

    int         segment;

    eSegType    segment_type;
    char        segment_flag;
    std::string segment_name;

    uint64	data_offset;     // includes 1024 byte segment header.
    uint64      data_size;
    
    PCIDSKBuffer header;

    MetadataSet  metadata;

public:
                CPCIDSKSegment( CPCIDSKFile *file, int segment,
                                const char *segment_pointer );
    virtual	~CPCIDSKSegment();

    void        LoadSegmentPointer( const char *segment_pointer );
    void        LoadSegmentHeader();

    PCIDSKBuffer &GetHeader();

    void      WriteToFile( const void *buffer, uint64 offset, uint64 size );
    void      ReadFromFile( void *buffer, uint64 offset, uint64 size );

    eSegType    GetSegmentType() { return segment_type; }
    std::string GetName() { return segment_name; }
    std::string GetDescription();
    int         GetSegmentNumber() { return segment; }

    std::string GetMetadataValue( std::string key ) 
		{ return metadata.GetMetadataValue(key); }
    std::vector<std::string> GetMetadataKeys() 
        	{ return metadata.GetMetadataKeys(); }
};

/************************************************************************/
/*                            CPCIDSKGeoref                             */
/************************************************************************/

class CPCIDSKGeoref : public CPCIDSKSegment, public PCIDSKGeoref
{
private:
    bool         loaded;

    std::string  geosys;
    double       a1, a2, xrot, b1, yrot, b3;

    void         Load();

    PCIDSKBuffer seg_data;

public:
    CPCIDSKGeoref( CPCIDSKFile *file, int segment,const char *segment_pointer );

    virtual     ~CPCIDSKGeoref();

    void        GetTransform( double &a1, double &a2, double &xrot, 
                              double &b1, double &yrot, double &b3 );
    std::string GetGeosys();

    void        WriteSimple( std::string geosys, 
                             double a1, double a2, double xrot, 
                             double b1, double yrot, double b3 );
};

/************************************************************************/
/*                             SysBlockMap                              */
/************************************************************************/

class SysBlockMap : public CPCIDSKSegment
{
private:
    bool         loaded;

    void         Load();

    PCIDSKBuffer seg_data;

    int          block_count;
    int          first_free_block;

    int          block_map_offset;
    int          layer_list_offset;

    std::vector<SysVirtualFile*> virtual_files;

public:
    SysBlockMap( CPCIDSKFile *file, int segment,const char *segment_pointer );

    virtual     ~SysBlockMap();

    SysVirtualFile *GetImageSysFile( int image );
};

/************************************************************************/
/*                           MetadataSegment                            */
/************************************************************************/

class MetadataSegment : public CPCIDSKSegment
{
private:
    bool         loaded;

    void         Load();

    PCIDSKBuffer seg_data;

public:
    MetadataSegment( CPCIDSKFile *file, int segment,
                     const char *segment_pointer );
    virtual     ~MetadataSegment();

    void         FetchMetadata( const char *group, int id, 
                                std::map<std::string,std::string> &md_set );
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
/*                            SysVirtualFile                            */
/************************************************************************/

class SysVirtualFile
{
private:
    static const int       block_size = 8192;

    CPCIDSKFile           *file;

    uint64                 file_length;

    std::vector<int>       block_segment;
    std::vector<int>       block_index;

    int                    loaded_block;
    uint8                  block_data[block_size];

    void                   LoadBlock( int requested_block );

public:
    SysVirtualFile( CPCIDSKFile *file, int start_block, uint64 image_length,
                    PCIDSKBuffer &block_map_data );
    ~SysVirtualFile();

    void      WriteToFile( const void *buffer, uint64 offset, uint64 size );
    void      ReadFromFile( void *buffer, uint64 offset, uint64 size );
};

/************************************************************************/
/*                          Utility functions.                          */
/************************************************************************/

uint64 atouint64( const char *);
int64  atoint64( const char *);
void   SwapData( void *data, int value_size, int value_count );
void   GetCurrentDateTime( char *out_datetime );

}; // end of PCIDSK namespace

#endif // PCIDSK_H_INCLUDED
