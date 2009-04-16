#include "pcidsk.h"

int main( int argc, char **argv)
{
    if( argc < 2 )
    {
        printf( "Usage: pcidsk_read <filename>\n" );
        exit( 1 );
    }

    const char *src_file = argv[1];

    PCIDSK::PCIDSKFile *file = PCIDSK::Open( src_file, "r", NULL );
    int channel_count = file->GetChannels();
    int channel_index;

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

        void *block_buffer = 
            malloc( PCIDSK::DataTypeSize(channel->GetType())
                    * channel->GetBlockWidth() 
                    * channel->GetBlockHeight() );

        int block_count = x_block_count * y_block_count;

        printf( "Process %d blocks on channel %d...",
                block_count, channel_index );
        fflush(stdout);

        for( i_block = 0; i_block < block_count; i_block++ )
        {
            channel->ReadBlock( i_block, block_buffer );
        }
        printf( "done.\n" );

        free( block_buffer );
    }
}
