/**********************************************************************
 * $Id: mitab_maptoolblock.cpp,v 1.2 1999/09/26 14:59:37 daniel Exp $
 *
 * Name:     mitab_maptoollock.cpp
 * Project:  MapInfo TAB Read/Write library
 * Language: C++
 * Purpose:  Implementation of the TABMAPToolBlock class used to handle
 *           reading/writing of the .MAP files' drawing tool blocks
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 1999, Daniel Morissette
 *
 * All rights reserved.  This software may be copied or reproduced, in
 * all or in part, without the prior written consent of its author,
 * Daniel Morissette (danmo@videotron.ca).  However, any material copied
 * or reproduced must bear the original copyright notice (above), this 
 * original paragraph, and the original disclaimer (below).
 * 
 * The entire risk as to the results and performance of the software,
 * supporting text and other information contained in this file
 * (collectively called the "Software") is with the user.  Although 
 * considerable efforts have been used in preparing the Software, the 
 * author does not warrant the accuracy or completeness of the Software.
 * In no event will the author be liable for damages, including loss of
 * profits or consequential damages, arising out of the use of the 
 * Software.
 * 
 **********************************************************************
 *
 * $Log: mitab_maptoolblock.cpp,v $
 * Revision 1.2  1999/09/26 14:59:37  daniel
 * Implemented write support
 *
 * Revision 1.1  1999/09/16 02:39:17  daniel
 * Completed read support for most feature types
 *
 **********************************************************************/

#include "mitab.h"

/*=====================================================================
 *                      class TABMAPToolBlock
 *====================================================================*/

#define MAP_TOOL_HEADER_SIZE   8

/**********************************************************************
 *                   TABMAPToolBlock::TABMAPToolBlock()
 *
 * Constructor.
 **********************************************************************/
TABMAPToolBlock::TABMAPToolBlock(TABAccess eAccessMode /*= TABRead*/):
    TABRawBinBlock(eAccessMode, TRUE)
{
    m_nNextToolBlock = m_numDataBytes = 0;

    m_numBlocksInChain = 1;  // Current block counts as 1
 
    m_poBlockManagerRef = NULL;
}

/**********************************************************************
 *                   TABMAPToolBlock::~TABMAPToolBlock()
 *
 * Destructor.
 **********************************************************************/
TABMAPToolBlock::~TABMAPToolBlock()
{
   
}


/**********************************************************************
 *                   TABMAPToolBlock::EndOfChain()
 *
 * Return TRUE if we reached the end of the last block in the chain
 * TABMAPToolBlocks, or FALSE if there is still data to be read from 
 * this chain.
 **********************************************************************/
GBool TABMAPToolBlock::EndOfChain()
{
   if (m_pabyBuf && 
       (m_nCurPos < (m_numDataBytes+MAP_TOOL_HEADER_SIZE) || 
        m_nNextToolBlock > 0 ) )
   {
       return FALSE;  // There is still data to be read.
   }

   return TRUE;
}

/**********************************************************************
 *                   TABMAPToolBlock::InitBlockFromData()
 *
 * Perform some initialization on the block after its binary data has
 * been set or changed (or loaded from a file).
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPToolBlock::InitBlockFromData(GByte *pabyBuf, int nSize, 
                                         GBool bMakeCopy /* = TRUE */,
                                         FILE *fpSrc /* = NULL */, 
                                         int nOffset /* = 0 */)
{
    int nStatus;

    /*-----------------------------------------------------------------
     * First of all, we must call the base class' InitBlockFromData()
     *----------------------------------------------------------------*/
    nStatus = TABRawBinBlock::InitBlockFromData(pabyBuf, nSize, bMakeCopy,
                                            fpSrc, nOffset);
    if (nStatus != 0)   
        return nStatus;

    /*-----------------------------------------------------------------
     * Validate block type
     *----------------------------------------------------------------*/
    if (m_nBlockType != TABMAP_TOOL_BLOCK)
    {
        CPLError(CE_Failure, CPLE_FileIO,
                 "InitBlockFromData(): Invalid Block Type: got %d expected %d",
                 m_nBlockType, TABMAP_TOOL_BLOCK);
        CPLFree(m_pabyBuf);
        m_pabyBuf = NULL;
        return -1;
    }

    /*-----------------------------------------------------------------
     * Init member variables
     *----------------------------------------------------------------*/
    GotoByteInBlock(0x002);
    m_numDataBytes = ReadInt16();       /* Excluding 8 bytes header */

    m_nNextToolBlock = ReadInt32();

    /*-----------------------------------------------------------------
     * The read ptr is now located at the beginning of the data part.
     *----------------------------------------------------------------*/
    GotoByteInBlock(MAP_TOOL_HEADER_SIZE);

    return 0;
}

/**********************************************************************
 *                   TABMAPToolBlock::CommitToFile()
 *
 * Commit the current state of the binary block to the file to which 
 * it has been previously attached.
 *
 * This method makes sure all values are properly set in the map object
 * block header and then calls TABRawBinBlock::CommitToFile() to do
 * the actual writing to disk.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPToolBlock::CommitToFile()
{
    int nStatus = 0;

    if ( m_pabyBuf == NULL )
    {
        CPLError(CE_Failure, CPLE_AssertionFailed, 
                 "CommitToFile(): Block has not been initialized yet!");
        return -1;
    }

    /*-----------------------------------------------------------------
     * Make sure 8 bytes block header is up to date.
     *----------------------------------------------------------------*/
    GotoByteInBlock(0x000);

    WriteInt16(TABMAP_TOOL_BLOCK);    // Block type code
    WriteInt16(m_nSizeUsed - MAP_TOOL_HEADER_SIZE); // num. bytes used
    WriteInt32(m_nNextToolBlock);

    nStatus = CPLGetLastErrorNo();

    /*-----------------------------------------------------------------
     * OK, call the base class to write the block to disk.
     *----------------------------------------------------------------*/
    if (nStatus == 0)
        nStatus = TABRawBinBlock::CommitToFile();

    return nStatus;
}

/**********************************************************************
 *                   TABMAPToolBlock::InitNewBlock()
 *
 * Initialize a newly created block so that it knows to which file it
 * is attached, its block size, etc . and then perform any specific 
 * initialization for this block type, including writing a default 
 * block header, etc. and leave the block ready to receive data.
 *
 * This is an alternative to calling ReadFromFile() or InitBlockFromData()
 * that puts the block in a stable state without loading any initial
 * data in it.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPToolBlock::InitNewBlock(FILE *fpSrc, int nBlockSize, 
                                        int nFileOffset /* = 0*/)
{
    /*-----------------------------------------------------------------
     * Start with the default initialisation
     *----------------------------------------------------------------*/
    if ( TABRawBinBlock::InitNewBlock(fpSrc, nBlockSize, nFileOffset) != 0)
        return -1;

    /*-----------------------------------------------------------------
     * And then set default values for the block header.
     *----------------------------------------------------------------*/
    m_nNextToolBlock = 0;
 
    m_numDataBytes = 0;

    GotoByteInBlock(0x000);

    if (m_eAccess != TABRead)
    {
        WriteInt16(TABMAP_TOOL_BLOCK); // Block type code
        WriteInt16(0);                 // num. bytes used, excluding header
        WriteInt32(0);                 // Pointer to next tool block
    }

    if (CPLGetLastErrorNo() != 0)
        return -1;

    return 0;
}

/**********************************************************************
 *                   TABMAPToolBlock::SetNextToolBlock()
 *
 * Set the address (offset from beginning of file) of the drawing tool block
 * that follows the current one.
 **********************************************************************/
void     TABMAPToolBlock::SetNextToolBlock(GInt32 nNextToolBlockAddress)
{
    m_nNextToolBlock = nNextToolBlockAddress;
}

/**********************************************************************
 *                   TABMAPToolBlock::SetMAPBlockManagerRef()
 *
 * Pass a reference to the block manager object for the file this 
 * block belongs to.  The block manager will be used by this object
 * when it needs to automatically allocate a new block.
 **********************************************************************/
void TABMAPToolBlock::SetMAPBlockManagerRef(TABMAPBlockManager *poBlockMgr)
{
    m_poBlockManagerRef = poBlockMgr;
};


/**********************************************************************
 *                   TABMAPToolBlock::ReadBytes()
 *
 * Cover function for TABRawBinBlock::ReadBytes() that will automagically
 * load the next coordinate block in the chain before reading the 
 * requested bytes if we are at the end of the current block and if
 * m_nNextToolBlock is a valid block.
 *
 * Then the control is passed to TABRawBinBlock::ReadBytes() to finish the
 * work:
 * Copy the number of bytes from the data block's internal buffer to
 * the user's buffer pointed by pabyDstBuf.
 *
 * Passing pabyDstBuf = NULL will only move the read pointer by the
 * specified number of bytes as if the copy had happened... but it 
 * won't crash.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABMAPToolBlock::ReadBytes(int numBytes, GByte *pabyDstBuf)
{
    int nStatus;

    if (m_pabyBuf && 
        m_nCurPos >= (m_numDataBytes+MAP_TOOL_HEADER_SIZE) && 
        m_nNextToolBlock > 0)
    {
        if ( (nStatus=GotoByteInFile(m_nNextToolBlock)) != 0)
        {
            // Failed.... an error has already been reported.
            return nStatus;
        }

        GotoByteInBlock(MAP_TOOL_HEADER_SIZE);  // Move pointer past header
        m_numBlocksInChain++;
    }

    return TABRawBinBlock::ReadBytes(numBytes, pabyDstBuf);
}

/**********************************************************************
 *                   TABMAPToolBlock::WriteBytes()
 *
 * Cover function for TABRawBinBlock::WriteBytes() that will automagically
 * CommitToFile() the current block and create a new one if we are at 
 * the end of the current block.
 *
 * Then the control is passed to TABRawBinBlock::WriteBytes() to finish the
 * work.
 *
 * Passing pabySrcBuf = NULL will only move the write pointer by the
 * specified number of bytes as if the copy had happened... but it 
 * won't crash.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int  TABMAPToolBlock::WriteBytes(int nBytesToWrite, GByte *pabySrcBuf)
{
    if (m_eAccess == TABWrite && m_poBlockManagerRef &&
        GetNumUnusedBytes() < nBytesToWrite)
    {
        int nNewBlockOffset = m_poBlockManagerRef->AllocNewBlock();
        SetNextToolBlock(nNewBlockOffset);

        if (CommitToFile() != 0 ||
            InitNewBlock(m_fp, 512, nNewBlockOffset) != 0)
        {
            // An error message should have already been reported.
            return -1;
        }

        m_numBlocksInChain ++;
    }

    return TABRawBinBlock::WriteBytes(nBytesToWrite, pabySrcBuf);
}




/**********************************************************************
 *                   TABMAPToolBlock::Dump()
 *
 * Dump block contents... available only in DEBUG mode.
 **********************************************************************/
#ifdef DEBUG

void TABMAPToolBlock::Dump(FILE *fpOut /*=NULL*/)
{
    if (fpOut == NULL)
        fpOut = stdout;

    fprintf(fpOut, "----- TABMAPToolBlock::Dump() -----\n");
    if (m_pabyBuf == NULL)
    {
        fprintf(fpOut, "Block has not been initialized yet.");
    }
    else
    {
        fprintf(fpOut,"Coordinate Block (type %d) at offset %d.\n", 
                                                 m_nBlockType, m_nFileOffset);
        fprintf(fpOut,"  m_numDataBytes        = %d\n", m_numDataBytes);
        fprintf(fpOut,"  m_nNextToolBlock     = %d\n", m_nNextToolBlock);
    }

    fflush(fpOut);
}

#endif // DEBUG



