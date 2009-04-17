#include "pcidsk.h"

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( "Usage: pcidsk_read [-p] [-l] <src_filename> [<dst_filename>]\n" );
    exit( 1 );
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/
int main( int argc, char **argv)
{
/* -------------------------------------------------------------------- */
/*      Process options.                                                */
/* -------------------------------------------------------------------- */
    const char *src_file = NULL;
    const char *dst_file = NULL;
    std::string strategy = "-b";
    int i_arg;

    for( i_arg = 1; i_arg < argc; i_arg++ )
    {
        if( strcmp(argv[i_arg],"-p") == 0 )
            strategy = argv[i_arg];
        else if( strcmp(argv[i_arg],"-l") == 0 )
            strategy = argv[i_arg];
        else if( argv[i_arg][0] == '-' )
            Usage();
        else if( src_file == NULL )
            src_file = argv[i_arg];
        else if( dst_file == NULL )
            dst_file = argv[i_arg];
        else
            Usage();
    }

    if( src_file == NULL )
        Usage();

/* -------------------------------------------------------------------- */
/*      Open the source file.                                           */
/* -------------------------------------------------------------------- */
    PCIDSK::PCIDSKFile *file = PCIDSK::Open( src_file, "r", NULL );
    int channel_count = file->GetChannels();
    int channel_index;

/* -------------------------------------------------------------------- */
/*      If a destination raw file is requested, open it now.            */
/* -------------------------------------------------------------------- */
    FILE *fp_raw = NULL;

    if( dst_file != NULL )
        fp_raw = fopen(dst_file,"wb");

/* -------------------------------------------------------------------- */
/*      Process the imagery, channel by channel.                        */
/* -------------------------------------------------------------------- */
    if( strategy == "-b" )
    {
        for( channel_index = 1; channel_index <= channel_count; channel_index++ )
        {
            PCIDSK::PCIDSKChannel *channel = file->GetChannel( channel_index );
            int i_block;
            int x_block_count = 
                (channel->GetWidth() + channel->GetBlockWidth()-1) 
                / channel->GetBlockWidth();
            int y_block_count = 
                (channel->GetHeight() + channel->GetBlockHeight()-1) 
                / channel->GetBlockHeight();
            int block_size = PCIDSK::DataTypeSize(channel->GetType())
                * channel->GetBlockWidth() 
                * channel->GetBlockHeight();
            void *block_buffer = malloc( block_size );
            
            int block_count = x_block_count * y_block_count;
            
            printf( "Process %d blocks on channel %d...",
                    block_count, channel_index );
            fflush(stdout);
            
            for( i_block = 0; i_block < block_count; i_block++ )
            {
                channel->ReadBlock( i_block, block_buffer );
                if( fp_raw != NULL )
                    fwrite( block_buffer, 1, block_size, fp_raw );
            }
            printf( "done.\n" );
            
            free( block_buffer );
        }
    }

/* -------------------------------------------------------------------- */
/*      Process the imagery line interleaved.                           */
/* -------------------------------------------------------------------- */
    else if( strategy == "-l" )
    {
        int i_block;
        PCIDSK::PCIDSKChannel *channel = file->GetChannel(1);
        int x_block_count = 
            (channel->GetWidth() + channel->GetBlockWidth()-1) 
            / channel->GetBlockWidth();
        int y_block_count = 
            (channel->GetHeight() + channel->GetBlockHeight()-1) 
            / channel->GetBlockHeight();
        int max_block_size = channel_count * 16 
            * channel->GetBlockWidth() * channel->GetBlockHeight();
        void *block_buffer = malloc( max_block_size );
        int block_count = x_block_count * y_block_count;

        // check for consistency.
        for( channel_index=2; channel_index <= channel_count; channel_index++ )
        {
            PCIDSK::PCIDSKChannel *other_channel = 
                file->GetChannel( channel_index );
            if( other_channel->GetBlockWidth() != channel->GetBlockWidth()
                || other_channel->GetBlockHeight() != channel->GetBlockHeight() )
            {
                fprintf( stderr, 
                         "Channels are not all of matching block size,\n"
                         "interleaved access unavailable.\n" );
                exit( 1 );
            }
        }

        printf( "Process %d blocks over %d channels...",
                block_count, channel_count );
        fflush( stdout );

        // actually process imagery.
        for( i_block = 0; i_block < block_count; i_block++ )
        {
            for( channel_index = 1; 
                 channel_index <= channel_count; 
                 channel_index++ )
            {
                PCIDSK::PCIDSKChannel *channel = file->GetChannel( channel_index );
                int block_size = PCIDSK::DataTypeSize(channel->GetType())
                    * channel->GetBlockWidth() 
                    * channel->GetBlockHeight();
                
                channel->ReadBlock( i_block, block_buffer );
                if( fp_raw != NULL )
                    fwrite( block_buffer, 1, block_size, fp_raw );
            }
        }
        
        printf( "done\n" );

        free( block_buffer );
    }
        
/* -------------------------------------------------------------------- */
/*      Process imagery pixel interleaved.                              */
/* -------------------------------------------------------------------- */
    else if( strategy == "-p" )
    {
        int i_block;
        PCIDSK::PCIDSKChannel *channel = file->GetChannel(1);
        int x_block_count = 
            (channel->GetWidth() + channel->GetBlockWidth()-1) 
            / channel->GetBlockWidth();
        int y_block_count = 
            (channel->GetHeight() + channel->GetBlockHeight()-1) 
            / channel->GetBlockHeight();
        int block_size = file->GetBlockSize();
        int block_count = x_block_count * y_block_count;

        if( strcmp(file->GetInterleaving(),"PIXEL   ") != 0 )
        {
            fprintf( stderr, 
                     "Pixel Interleaved access only possible on pixel interleaved files.\n" );
            exit( 1 );
        }

        printf( "Process %d blocks over %d channels...",
                block_count, channel_count );
        fflush( stdout );

        // actually process imagery.
        for( i_block = 0; i_block < block_count; i_block++ )
        {
            void *buffer = file->ReadAndLockBlock( i_block );
            if( fp_raw != NULL )
                fwrite( buffer, 1, block_size, fp_raw );

            file->UnlockBlock( 0 );
        }
        
        printf( "done\n" );
    }
        
/* -------------------------------------------------------------------- */
/*      Close and cleanup.                                              */
/* -------------------------------------------------------------------- */
    if( fp_raw != NULL )
        fclose( fp_raw );
    delete file;
}
