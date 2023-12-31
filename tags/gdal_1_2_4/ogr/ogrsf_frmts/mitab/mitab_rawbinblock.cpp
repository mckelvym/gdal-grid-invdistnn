/**********************************************************************
 * $Id: mitab_rawbinblock.cpp,v 1.6 2004/06/30 20:29:04 dmorissette Exp $
 *
 * Name:     mitab_rawbinblock.cpp
 * Project:  MapInfo TAB Read/Write library
 * Language: C++
 * Purpose:  Implementation of the TABRawBinBlock class used to handle
 *           reading/writing blocks in the .MAP files
 * Author:   Daniel Morissette, dmorissette@dmsolutions.ca
 *
 **********************************************************************
 * Copyright (c) 1999, 2000, Daniel Morissette
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
 **********************************************************************
 *
 * $Log: mitab_rawbinblock.cpp,v $
 * Revision 1.6  2004/06/30 20:29:04  dmorissette
 * Fixed refs to old address danmo@videotron.ca
 *
 * Revision 1.5  2000/02/28 17:06:06  daniel
 * Added m_bModified flag
 *
 * Revision 1.4  2000/01/15 22:30:45  daniel
 * Switch to MIT/X-Consortium OpenSource license
 *
 * Revision 1.3  1999/09/26 14:59:37  daniel
 * Implemented write support
 *
 * Revision 1.2  1999/09/16 02:39:17  daniel
 * Completed read support for most feature types
 *
 * Revision 1.1  1999/07/12 04:18:25  daniel
 * Initial checkin
 *
 **********************************************************************/

#include "mitab.h"

/*=====================================================================
 *                      class TABRawBinBlock
 *====================================================================*/


/**********************************************************************
 *                   TABRawBinBlock::TABRawBinBlock()
 *
 * Constructor.
 **********************************************************************/
TABRawBinBlock::TABRawBinBlock(TABAccess eAccessMode /*= TABRead*/,
                               GBool bHardBlockSize /*= TRUE*/)
{
    m_fp = NULL;
    m_pabyBuf = NULL;
    m_nFirstBlockPtr = 0;
    m_nBlockSize = m_nSizeUsed = m_nFileOffset = m_nCurPos = 0;
    m_bHardBlockSize = bHardBlockSize;

    m_bModified = FALSE;

    m_eAccess = eAccessMode; 

}

/**********************************************************************
 *                   TABRawBinBlock::~TABRawBinBlock()
 *
 * Destructor.
 **********************************************************************/
TABRawBinBlock::~TABRawBinBlock()
{
    if (m_pabyBuf)
        CPLFree(m_pabyBuf);
}


/**********************************************************************
 *                   TABRawBinBlock::ReadFromFile()
 *
 * Load data from the specified file location and initialize the block.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABRawBinBlock::ReadFromFile(FILE *fpSrc, int nOffset, 
                                     int nSize /*= 512*/)
{
    GByte *pabyBuf;

    if (fpSrc == NULL || nSize == 0)
    {
        CPLError(CE_Failure, CPLE_AssertionFailed, 
                 "TABRawBinBlock::ReadFromFile(): Assertion Failed!");
        return -1;
    }

    m_fp = fpSrc;
    m_nFileOffset = nOffset;
    m_nCurPos = 0;
    m_bModified = FALSE;
    
    /*----------------------------------------------------------------
     * Alloc a buffer to contain the data
     *---------------------------------------------------------------*/
    pabyBuf = (GByte*)CPLMalloc(nSize*sizeof(GByte));

    /*----------------------------------------------------------------
     * Read from the file
     *---------------------------------------------------------------*/
    if (VSIFSeek(fpSrc, nOffset, SEEK_SET) != 0 ||
        (m_nSizeUsed = VSIFRead(pabyBuf, sizeof(GByte), nSize, fpSrc) ) == 0 ||
        (m_bHardBlockSize && m_nSizeUsed != nSize ) )
    {
        CPLError(CE_Failure, CPLE_FileIO,
                 "ReadFromFile() failed reading %d bytes at offset %d.",
                 nSize, nOffset);
        return -1;
    }

    /*----------------------------------------------------------------
     * Init block with the data we just read
     *---------------------------------------------------------------*/

    return InitBlockFromData(pabyBuf, nSize, FALSE, fpSrc, nOffset);
}


/**********************************************************************
 *                   TABRawBinBlock::CommitToFile()
 *
 * Commit the current state of the binary block to the file to which 
 * it has been previously attached.
 *
 * Derived classes may want to (optionally) reimplement this method if
 * they need to do special processing before committing the block to disk.
 *
 * For files created with bHardBlockSize=TRUE, a complete block of
 * the specified size is always written, otherwise only the number of
 * used bytes in the block will be written to disk.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABRawBinBlock::CommitToFile()
{
    int nStatus = 0;

    if (m_fp == NULL || m_nBlockSize <= 0 || m_pabyBuf == NULL ||
        m_nFileOffset < 0)
    {
        CPLError(CE_Failure, CPLE_AssertionFailed, 
        "TABRawBinBlock::CommitToFile(): Block has not been initialized yet!");
        return -1;
    }

    /*----------------------------------------------------------------
     * If block has not been modified, then just return... nothing to do.
     *---------------------------------------------------------------*/
    if (!m_bModified)
        return 0;

    /*----------------------------------------------------------------
     * Move the output file pointer to the right position... 
     *---------------------------------------------------------------*/
    if (VSIFSeek(m_fp, m_nFileOffset, SEEK_SET) != 0)
    {
        /*------------------------------------------------------------
         * Moving pointer failed... we may need to pad with zeros if 
         * block destination is beyond current end of file.
         *-----------------------------------------------------------*/
        int nCurPos;
        nCurPos = VSIFTell(m_fp);

        if (nCurPos < m_nFileOffset &&
            VSIFSeek(m_fp, 0L, SEEK_END) == 0 &&
            (nCurPos = VSIFTell(m_fp)) < m_nFileOffset)
        {
            GByte cZero = 0;

            while(nCurPos < m_nFileOffset && nStatus == 0)
            {
                if (VSIFWrite(&cZero, 1, 1, m_fp) != 1)
                {
                    CPLError(CE_Failure, CPLE_FileIO,
                             "Failed writing 1 byte at offset %d.", nCurPos);
                    nStatus = -1;
                    break;
                }
                nCurPos++;
            }
        }
            
        if (nCurPos != m_nFileOffset)
            nStatus = -1; // Error message will follow below

    }

    /*----------------------------------------------------------------
     * At this point we are ready to write to the file.
     *
     * If m_bHardBlockSize==FALSE, then we do not write a complete block;
     * we write only the part of the block that was used.
     *---------------------------------------------------------------*/
    int numBytesToWrite = m_bHardBlockSize?m_nBlockSize:m_nSizeUsed;

    if (nStatus != 0 ||
        VSIFWrite(m_pabyBuf,sizeof(GByte),
                    numBytesToWrite, m_fp) != (size_t)numBytesToWrite )
    {
        CPLError(CE_Failure, CPLE_FileIO,
                 "Failed writing %d bytes at offset %d.",
                 numBytesToWrite, m_nFileOffset);
        return -1;
    }

    fflush(m_fp);

    m_bModified = FALSE;

    return 0;
}


/**********************************************************************
 *                   TABRawBinBlock::InitBlockFromData()
 *
 * Set the binary data buffer and initialize the block.
 *
 * Calling ReadFromFile() will automatically call InitBlockFromData() to
 * complete the initialization of the block after the data is read from the
 * file.  Derived classes should implement their own version of 
 * InitBlockFromData() if they need specific initialization... in this
 * case the derived InitBlockFromData() should call TABRawBinBlock::InitBlockFromData()
 * before doing anything else.
 *
 * By default, the buffer will be copied, but if bMakeCopy = FALSE then
 * it won't be copied, and the object will keep a reference to the
 * user's buffer... and this object will eventually free the user's buffer.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABRawBinBlock::InitBlockFromData(GByte *pabyBuf, int nSize, 
                                      GBool bMakeCopy /* = TRUE */,
                                      FILE *fpSrc /* = NULL */, 
                                      int nOffset /* = 0 */)
{
    m_fp = fpSrc;
    m_nFileOffset = nOffset;
    m_nCurPos = 0;
    m_bModified = FALSE;
    
    /*----------------------------------------------------------------
     * Alloc or realloc the buffer to contain the data if necessary
     *---------------------------------------------------------------*/
    if (!bMakeCopy)
    {
        if (m_pabyBuf != NULL)
            CPLFree(m_pabyBuf);
        m_pabyBuf = pabyBuf;
        m_nSizeUsed = m_nBlockSize = nSize;
    }
    else if (m_pabyBuf == NULL || nSize != m_nBlockSize)
    {
        m_pabyBuf = (GByte*)CPLRealloc(m_pabyBuf, nSize*sizeof(GByte));
        m_nSizeUsed = m_nBlockSize = nSize;
        memcpy(m_pabyBuf, pabyBuf, m_nBlockSize);
    }

    /*----------------------------------------------------------------
     * Extract block type... header block (first block in a file) has
     * no block type, so we assign one by default.
     *---------------------------------------------------------------*/
    if (m_nFileOffset == 0)
        m_nBlockType = TABMAP_HEADER_BLOCK;
    else
    {
        // Block type will be validated only if GetBlockType() is called
        m_nBlockType = (int)m_pabyBuf[0];
    }

    return 0;
}

/**********************************************************************
 *                   TABRawBinBlock::InitNewBlock()
 *
 * Initialize the block so that it knows to which file is is attached,
 * its block size, etc.
 *
 * This is an alternative to calling ReadFromFile() or InitBlockFromData()
 * that puts the block in a stable state without loading any initial
 * data in it.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABRawBinBlock::InitNewBlock(FILE *fpSrc, int nBlockSize, 
                                  int nFileOffset /* = 0*/)
{
    m_fp = fpSrc;
    m_nBlockSize = nBlockSize;
    m_nSizeUsed = 0;
    m_nCurPos = 0;
    m_bModified = FALSE;

    if (nFileOffset > 0)
        m_nFileOffset = nFileOffset;
    else
        m_nFileOffset = 0;

    m_nBlockType = -1;

    m_pabyBuf = (GByte*)CPLRealloc(m_pabyBuf, m_nBlockSize*sizeof(GByte));
    memset(m_pabyBuf, 0, m_nBlockSize);

    return 0;
}


/**********************************************************************
 *                   TABRawBinBlock::GetBlockType()
 *
 * Return the block type for the current object.
 *
 * Returns a block type >= 0 if succesful or -1 if an error happened, in 
 * which case  CPLError() will have been called.
 **********************************************************************/
int     TABRawBinBlock::GetBlockType()
{
    if (m_pabyBuf == NULL)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "GetBlockType(): Block has not been initialized.");
        return -1;
    }

    if (m_nBlockType > TABMAP_LAST_VALID_BLOCK_TYPE)
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                 "GetBlockType(): Unsupported block type %d.", 
                 m_nBlockType);
        return -1;
    }

    return m_nBlockType;
}

/**********************************************************************
 *                   TABRawBinBlock::GotoByteInBlock()
 *
 * Move the block pointer to the specified position relative to the 
 * beginning of the block.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABRawBinBlock::GotoByteInBlock(int nOffset)
{
    if ( (m_eAccess == TABRead && nOffset > m_nSizeUsed) ||
         (m_eAccess != TABRead && nOffset > m_nBlockSize) )
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "GotoByteInBlock(): Attempt to go past end of data block.");
        return -1;
    }

    if (nOffset < 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
               "GotoByteInBlock(): Attempt to go before start of data block.");
        return -1;
    }

    m_nCurPos = nOffset;
    
    m_nSizeUsed = MAX(m_nSizeUsed, m_nCurPos);

    return 0;
}

/**********************************************************************
 *                   TABRawBinBlock::GotoByteRel()
 *
 * Move the block pointer by the specified number of bytes relative
 * to its current position.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABRawBinBlock::GotoByteRel(int nOffset)
{
    return GotoByteInBlock(m_nCurPos + nOffset);
}

/**********************************************************************
 *                   TABRawBinBlock::GotoByteInFile()
 *
 * Move the block pointer to the specified position relative to the 
 * beginning of the file.  
 * 
 * In read access, the current block may be reloaded to contain a right
 * block of binary data if necessary.
 *
 * In write mode, the current block may automagically be committed to 
 * disk and a new block initialized if necessary.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int     TABRawBinBlock::GotoByteInFile(int nOffset)
{
    int nNewBlockPtr;

    if (nOffset < 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
               "GotoByteInFile(): Attempt to go before start of file.");
        return -1;
    }

    nNewBlockPtr = ( (nOffset-m_nFirstBlockPtr)/m_nBlockSize)*m_nBlockSize +
                     m_nFirstBlockPtr;

    if (m_eAccess == TABRead)
    {
        if ( (nOffset<m_nFileOffset || nOffset>=m_nFileOffset+m_nSizeUsed) &&
             ReadFromFile(m_fp, nNewBlockPtr, m_nBlockSize) != 0)
        {
            // Failed reading new block... error has already been reported.
            return -1;
        }
    }
    else if (m_eAccess == TABWrite)
    {
        if ( (nOffset<m_nFileOffset || nOffset>=m_nFileOffset+m_nBlockSize) &&
             (CommitToFile() != 0 ||
              InitNewBlock(m_fp, m_nBlockSize, nNewBlockPtr) != 0)  )
        {
            // Failed reading new block... error has already been reported.
            return -1;
        }
    }
    else
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                 "Access mode not supported yet!");
        return -1;
    }

    m_nCurPos = nOffset-m_nFileOffset;

    m_nSizeUsed = MAX(m_nSizeUsed, m_nCurPos);

    return 0;
}


/**********************************************************************
 *                   TABRawBinBlock::SetFirstBlockPtr()
 *
 * Set the position in the file at which the first block starts.
 * This value will usually be the header size and needs to be specified 
 * only if the header size is different from the other blocks size. 
 *
 * This value will be used by GotoByteInFile() to properly align the data
 * blocks that it loads automatically when a requested position is outside
 * of the block currently in memory.
 **********************************************************************/
void  TABRawBinBlock::SetFirstBlockPtr(int nOffset)
{
    m_nFirstBlockPtr = nOffset;
}


/**********************************************************************
 *                   TABRawBinBlock::GetNumUnusedBytes()
 *
 * Return the number of unused bytes in this block.
 **********************************************************************/
int     TABRawBinBlock::GetNumUnusedBytes()
{
    return (m_nBlockSize - m_nSizeUsed);
}

/**********************************************************************
 *                   TABRawBinBlock::GetFirstUnusedByteOffset()
 *
 * Return the position of the first unused byte in this block relative
 * to the beginning of the file, or -1 if the block is full.
 **********************************************************************/
int     TABRawBinBlock::GetFirstUnusedByteOffset()
{
    if (m_nSizeUsed < m_nBlockSize)
        return m_nFileOffset + m_nSizeUsed;
    else
        return -1;
}

/**********************************************************************
 *                   TABRawBinBlock::GetCurAddress()
 *
 * Return the current pointer position, relative to beginning of file.
 **********************************************************************/
int     TABRawBinBlock::GetCurAddress()
{
    return (m_nFileOffset + m_nCurPos);
}

/**********************************************************************
 *                   TABRawBinBlock::ReadBytes()
 *
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
int     TABRawBinBlock::ReadBytes(int numBytes, GByte *pabyDstBuf)
{
    /*----------------------------------------------------------------
     * Make sure block is initialized with Read access and that the
     * operation won't go beyond the buffer's size.
     *---------------------------------------------------------------*/
    if (m_pabyBuf == NULL)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "ReadBytes(): Block has not been initialized.");
        return -1;
    }

    if (m_eAccess != TABRead && m_eAccess != TABReadWrite )
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "ReadBytes(): Block does not support read operations.");
        return -1;
    }

    if (m_nCurPos + numBytes > m_nSizeUsed)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "ReadBytes(): Attempt to read past end of data block.");
        return -1;
    }

    if (pabyDstBuf)
    {
        memcpy(pabyDstBuf, m_pabyBuf + m_nCurPos, numBytes);
    }

    m_nCurPos += numBytes;

    return 0;
}

/**********************************************************************
 *                   TABRawBinBlock::Read<datatype>()
 *
 * MapInfo files are binary files with LSB first (Intel) byte 
 * ordering.  The following functions will read from the input file
 * and return a value with the bytes ordered properly for the current 
 * platform.
 **********************************************************************/
GByte  TABRawBinBlock::ReadByte()
{
    GByte byValue;

    ReadBytes(1, (GByte*)(&byValue));

    return byValue;
}

GInt16  TABRawBinBlock::ReadInt16()
{
    GInt16 n16Value;

    ReadBytes(2, (GByte*)(&n16Value));

#ifdef CPL_MSB
    return (GInt16)CPL_SWAP16(n16Value);
#else
    return n16Value;
#endif
}

GInt32  TABRawBinBlock::ReadInt32()
{
    GInt32 n32Value;

    ReadBytes(4, (GByte*)(&n32Value));

#ifdef CPL_MSB
    return (GInt32)CPL_SWAP32(n32Value);
#else
    return n32Value;
#endif
}

float   TABRawBinBlock::ReadFloat()
{
    float fValue;

    ReadBytes(4, (GByte*)(&fValue));

#ifdef CPL_MSB
    *(GUInt32*)(&fValue) = CPL_SWAP32(*(GUInt32*)(&fValue));
#endif
    return fValue;
}

double  TABRawBinBlock::ReadDouble()
{
    double dValue;

    ReadBytes(8, (GByte*)(&dValue));

#ifdef CPL_MSB
    CPL_SWAPDOUBLE(&dValue);
#endif

    return dValue;
}



/**********************************************************************
 *                   TABRawBinBlock::WriteBytes()
 *
 * Copy the number of bytes from the user's buffer pointed by pabySrcBuf
 * to the data block's internal buffer.
 * Note that this call only writes to the memory buffer... nothing is
 * written to the file until WriteToFile() is called.
 *
 * Passing pabySrcBuf = NULL will only move the write pointer by the
 * specified number of bytes as if the copy had happened... but it 
 * won't crash.
 *
 * Returns 0 if succesful or -1 if an error happened, in which case 
 * CPLError() will have been called.
 **********************************************************************/
int  TABRawBinBlock::WriteBytes(int nBytesToWrite, GByte *pabySrcBuf)
{
    /*----------------------------------------------------------------
     * Make sure block is initialized with Write access and that the
     * operation won't go beyond the buffer's size.
     *---------------------------------------------------------------*/
    if (m_pabyBuf == NULL)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "WriteBytes(): Block has not been initialized.");
        return -1;
    }

    if (m_eAccess != TABWrite && m_eAccess != TABReadWrite )
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "WriteBytes(): Block does not support write operations.");
        return -1;
    }

    if (m_nCurPos + nBytesToWrite > m_nBlockSize)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "WriteBytes(): Attempt to write past end of data block.");
        return -1;
    }

    /*----------------------------------------------------------------
     * Everything is OK... copy the data
     *---------------------------------------------------------------*/
    if (pabySrcBuf)
    {
        memcpy(m_pabyBuf + m_nCurPos, pabySrcBuf, nBytesToWrite);
    }

    m_nCurPos += nBytesToWrite;

    m_nSizeUsed = MAX(m_nSizeUsed, m_nCurPos);

    m_bModified = TRUE;

    return 0;
}


/**********************************************************************
 *                    TABRawBinBlock::Write<datatype>()
 *
 * Arc/Info files are binary files with MSB first (Motorola) byte 
 * ordering.  The following functions will reorder the byte for the
 * value properly and write that to the output file.
 *
 * If a problem happens, then CPLError() will be called and 
 * CPLGetLastErrNo() can be used to test if a write operation was 
 * succesful.
 **********************************************************************/
int  TABRawBinBlock::WriteByte(GByte byValue)
{
    return WriteBytes(1, (GByte*)&byValue);
}

int  TABRawBinBlock::WriteInt16(GInt16 n16Value)
{
#ifdef CPL_MSB
    n16Value = (GInt16)CPL_SWAP16(n16Value);
#endif

    return WriteBytes(2, (GByte*)&n16Value);
}

int  TABRawBinBlock::WriteInt32(GInt32 n32Value)
{
#ifdef CPL_MSB
    n32Value = (GInt32)CPL_SWAP32(n32Value);
#endif

    return WriteBytes(4, (GByte*)&n32Value);
}

int  TABRawBinBlock::WriteFloat(float fValue)
{
#ifdef CPL_MSB
    *(GUInt32*)(&fValue) = CPL_SWAP32(*(GUInt32*)(&fValue));
#endif

    return WriteBytes(4, (GByte*)&fValue);
}

int  TABRawBinBlock::WriteDouble(double dValue)
{
#ifdef CPL_MSB
    CPL_SWAPDOUBLE(&dValue);
#endif

    return WriteBytes(8, (GByte*)&dValue);
}


/**********************************************************************
 *                    TABRawBinBlock::WriteZeros()
 *
 * Write a number of zeros (sepcified in bytes) at the current position 
 * in the file.
 *
 * If a problem happens, then CPLError() will be called and 
 * CPLGetLastErrNo() can be used to test if a write operation was 
 * succesful.
 **********************************************************************/
int  TABRawBinBlock::WriteZeros(int nBytesToWrite)
{
    char acZeros[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int i;
    int nStatus = 0;

    /* Write by 8 bytes chunks.  The last chunk may be less than 8 bytes 
     */
    for(i=0; nStatus == 0 && i< nBytesToWrite; i+=8)
    {
        nStatus = WriteBytes(MIN(8,(nBytesToWrite-i)), (GByte*)acZeros);
    }

    return nStatus;
}

/**********************************************************************
 *                   TABRawBinBlock::WritePaddedString()
 *
 * Write a string and pad the end of the field (up to nFieldSize) with
 * spaces number of spaces at the current position in the file.
 *
 * If a problem happens, then CPLError() will be called and 
 * CPLGetLastErrNo() can be used to test if a write operation was 
 * succesful.
 **********************************************************************/
int  TABRawBinBlock::WritePaddedString(int nFieldSize, const char *pszString)
{
    char acSpaces[8] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    int i, nLen, numSpaces;
    int nStatus = 0;

    nLen = strlen(pszString);
    nLen = MIN(nLen, nFieldSize);
    numSpaces = nFieldSize - nLen;

    if (nLen > 0)
        nStatus = WriteBytes(nLen, (GByte*)pszString);

    /* Write spaces by 8 bytes chunks.  The last chunk may be less than 8 bytes
     */
    for(i=0; nStatus == 0 && i< numSpaces; i+=8)
    {
        nStatus = WriteBytes(MIN(8,(numSpaces-i)), (GByte*)acSpaces);
    }

    return nStatus;
}

/**********************************************************************
 *                   TABRawBinBlock::Dump()
 *
 * Dump block contents... available only in DEBUG mode.
 **********************************************************************/
#ifdef DEBUG

void TABRawBinBlock::Dump(FILE *fpOut /*=NULL*/)
{
    if (fpOut == NULL)
        fpOut = stdout;

    fprintf(fpOut, "----- TABRawBinBlock::Dump() -----\n");
    if (m_pabyBuf == NULL)
    {
        fprintf(fpOut, "Block has not been initialized yet.");
    }
    else
    {
        fprintf(fpOut, "Block (type %d) size=%d bytes at offset %d in file.\n",
                m_nBlockType, m_nBlockSize, m_nFileOffset);
        fprintf(fpOut, "Current pointer at byte %d\n", m_nCurPos);
    }

    fflush(fpOut);
}

#endif // DEBUG



/**********************************************************************
 *                   TABCreateMAPBlockFromFile()
 *
 * Load data from the specified file location and create and initialize 
 * a TABMAP*Block of the right type to handle it.
 *
 * Returns the new object if succesful or NULL if an error happened, in 
 * which case CPLError() will have been called.
 **********************************************************************/
TABRawBinBlock *TABCreateMAPBlockFromFile(FILE *fpSrc, int nOffset, 
                                          int nSize /*= 512*/, 
                                          GBool bHardBlockSize /*= TRUE */,
                                          TABAccess eAccessMode /*= TABRead*/)
{
    TABRawBinBlock *poBlock = NULL;
    GByte *pabyBuf;

    if (fpSrc == NULL || nSize == 0)
    {
        CPLError(CE_Failure, CPLE_AssertionFailed, 
                 "TABCreateMAPBlockFromFile(): Assertion Failed!");
        return NULL;
    }

    /*----------------------------------------------------------------
     * Alloc a buffer to contain the data
     *---------------------------------------------------------------*/
    pabyBuf = (GByte*)CPLMalloc(nSize*sizeof(GByte));

    /*----------------------------------------------------------------
     * Read from the file
     *---------------------------------------------------------------*/
    if (VSIFSeek(fpSrc, nOffset, SEEK_SET) != 0 ||
        VSIFRead(pabyBuf, sizeof(GByte), nSize, fpSrc)!=(unsigned int)nSize )
    {
        CPLError(CE_Failure, CPLE_FileIO,
         "TABCreateMAPBlockFromFile() failed reading %d bytes at offset %d.",
                 nSize, nOffset);
        return NULL;
    }

    /*----------------------------------------------------------------
     * Create an object of the right type
     * Header block is different: it does not start with the object 
     * type byte but it is always the first block in a file
     *---------------------------------------------------------------*/
    if (nOffset == 0)
    {
        poBlock = new TABMAPHeaderBlock;
    }
    else
    {
        switch(pabyBuf[0])
        {
          case TABMAP_INDEX_BLOCK:
            poBlock = new TABMAPIndexBlock(eAccessMode);
            break;
          case TABMAP_OBJECT_BLOCK:
            poBlock = new TABMAPObjectBlock(eAccessMode);
            break;
          case TABMAP_COORD_BLOCK:
            poBlock = new TABMAPCoordBlock(eAccessMode);
            break;
          case TABMAP_TOOL_BLOCK:
            poBlock = new TABMAPToolBlock(eAccessMode);
            break;
          case TABMAP_GARB_BLOCK:
          default:
            poBlock = new TABRawBinBlock(eAccessMode, bHardBlockSize);
            break;
        }
    }

    /*----------------------------------------------------------------
     * Init new object with the data we just read
     *---------------------------------------------------------------*/
    if (poBlock->InitBlockFromData(pabyBuf, nSize, FALSE, fpSrc, nOffset) != 0)
    {
        // Some error happened... and CPLError() has been called
        delete poBlock;
        poBlock = NULL;
    }

    return poBlock;
}


