/******************************************************************************
 * $Id$
 *
 * Name:     hfadataset.cpp
 * Project:  Erdas Imagine Driver
 * Purpose:  Imagine Compression code.
 * Author:   Sam Gillingham <sam.gillingham at nrm.qld.gov>
 *
 ******************************************************************************
 * Copyright (c) 2005, Sam Gillingham
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
 *****************************************************************************
 *
 * $Log$
 * Revision 1.1  2005/01/10 17:40:40  fwarmerdam
 * New
 *
 */

#include "hfa_p.h"

CPL_CVSID("$Id$");

HFACompress::HFACompress( void *pData, GUInt32 nBlockSize, int nDataType )
{
  m_pData       = pData;
  m_nBlockSize  = nBlockSize;
  m_nBlockCount = nBlockSize / ( HFAGetDataTypeBits( nDataType ) / 8 );
  m_nDataType   = nDataType;

  /* Allocate some memory for the count and values - probably too big */
  /* About right for worst case scenario tho */
  m_pCounts     = (GByte*)CPLMalloc( m_nBlockSize );
  m_nSizeCounts = 0;
  
  m_pValues     = (GByte*)CPLMalloc( m_nBlockSize );
  m_nSizeValues = 0;
  
  m_nMin        = 0;
  m_nNumRuns    = 0;
  m_nNumBits    = 0;
}

HFACompress::~HFACompress()
{
  /* free the compressed data */
  CPLFree( m_pCounts );
  CPLFree( m_pValues );
}

/* returns the number of bits needed to encode a count */
GByte _FindNumBits( GUInt32 range )
{
  if( range < 0xff )
  {
    return 8; 
  } 
  else if( range < 0xffff )
  {
    return 16;
  }
  else
  {
    return 32; 
  }
}

/* Gets the value from the uncompressed block as a GUInt32 no matter the data type */
GUInt32 HFACompress::valueAsUInt32( GUInt32 index )
{
GUInt32 val = 0;

  if( HFAGetDataTypeBits( m_nDataType ) == 8 )
  {
    val = ((GByte*)m_pData)[index];
  }
  else if( HFAGetDataTypeBits( m_nDataType ) == 16 )
  {
    val = ((GUInt16*)m_pData)[index];
  }
  else if( HFAGetDataTypeBits( m_nDataType ) == 32 )
  {
    val = ((GUInt32*)m_pData)[index]; 
  }
  else
  {
    fprintf( stderr, "%d not supported\n", m_nDataType );
    CPLAssert( FALSE );
  }
  
  return val;
}

/* Finds the minimum value in a type specific fashion. This value is
  subtracted from each value in the compressed dataset. The maxmimum
  value is also found and the number of bits that the range can be stored 
  is also returned. */
/* TODO: Minimum value returned as pNumBits is now 8 - Imagine
  can handle 1, 2, and 4 bits as well */
GUInt32 HFACompress::findMin( GByte *pNumBits )
{
GUInt32 u32Val;
GUInt32 u32Min, u32Max;

   u32Min = valueAsUInt32( 0 );
   u32Max = u32Min;

  for( GUInt32 count = 1; count < m_nBlockCount; count++ )
  {
    u32Val = valueAsUInt32( count );
    if( u32Val < u32Min )
      u32Min = u32Val;
    else if( u32Val > u32Max )
      u32Max = u32Val;
  }    
  
  //fprintf( stderr, "max = %x min = %x diff = %x\n", u32Max, u32Min, u32Max - u32Min );
  *pNumBits = _FindNumBits( u32Max - u32Min );

  return u32Min;
}

/* Codes the count in the way expected by Imagine - ie the lower 2 bits specify how many bytes
   the count takes up */
void HFACompress::makeCount( GUInt32 count, GByte *pCounter, GUInt32 *pnSizeCount )
{
  /* Because Imagine stores the number of bits used in the
      lower 2 bits of the data it restricts what we can use */
  if( count < 0x40 )
  {
    pCounter[0] = count;
    *pnSizeCount = 1;
  }
  else if( count < 0x8000 )
  {
    pCounter[1] = count & 0xff;
    count /= 256;
    pCounter[0] = count | 0x40;
    *pnSizeCount = 2;
  }
  else if( count < 0x800000 )
  {
    pCounter[2] = count & 0xff;
    count /= 256;
    pCounter[1] = count & 0xff;
    count /= 256;
    pCounter[0] = count | 0x80;
    *pnSizeCount = 3;
  }
  else
  {
    pCounter[3] = count & 0xff;
    count /= 256;
    pCounter[2] = count & 0xff;
    count /= 256;
    pCounter[1] = count & 0xff;
    count /= 256;
    pCounter[0] = count | 0xc0;
    *pnSizeCount = 4;
  }
}

/* Encodes the value depending on the number of bits we are using */
void HFACompress::encodeValue( GUInt32 val, GUInt32 repeat )
{
GUInt32 nSizeCount;

  makeCount( repeat, m_pCurrCount, &nSizeCount );
  m_pCurrCount += nSizeCount;
  if( m_nNumBits == 8 )
  {
    /* Only storing 8 bits per value as the range is small */
    *(GByte*)m_pCurrValues = GByte(val - m_nMin);
    m_pCurrValues += sizeof( GByte );
  }
  else if( m_nNumBits == 16 )
  {
    /* Only storing 16 bits per value as the range is small */
    *(GUInt16*)m_pCurrValues = GUInt16(val - m_nMin);
#ifndef CPL_MSB
   CPL_SWAP16PTR( m_pCurrValues );      
#endif /* ndef CPL_MSB */
    m_pCurrValues += sizeof( GUInt16 );
  }
  else
  {
    *(GUInt32*)m_pCurrValues = GUInt32(val - m_nMin);
#ifndef CPL_MSB
   CPL_SWAP32PTR( m_pCurrValues );      
#endif /* ndef CPL_MSB */
    m_pCurrValues += sizeof( GUInt32 );
  }
}

/* This is the guts of the file - call this to compress the block */
/* returns false if the compression fails - ie compressed block bigger than input */
bool HFACompress::compressBlock()
{
GUInt32 nLastUnique = 0;

  /* reset our pointers */
  m_pCurrCount  = m_pCounts;
  m_pCurrValues = m_pValues;

  /* Get the minimum value - this can be subtracted from each value in the image */
  m_nMin = findMin( &m_nNumBits );
  
  //fprintf( stderr, "min = %x, bits = %x\n", m_nMin, (int)m_nNumBits );

  /* Go thru the block */
  GUInt32 u32Last, u32Val;
  u32Last = valueAsUInt32( 0 );
  u32Val = u32Last;
  for( GUInt32 count = 1; count < m_nBlockCount; count++ )
  {
    u32Val = valueAsUInt32( count );
    if( u32Val != u32Last )
    {
      /* The values have changed - ie a run has come to and end */
      encodeValue( u32Last, count - nLastUnique );
      
      if( ( m_pCurrValues - m_pValues ) > (int) m_nBlockSize )
      {
      	return false;
      }
      
      m_nNumRuns++;
      u32Last = u32Val;
      nLastUnique = count;
    }
  }

  /* OK we have done the block but haven't got the last run because we were only looking for a change in values */
  encodeValue( u32Last, m_nBlockCount - nLastUnique );
  m_nNumRuns++;
  
  /* set the size variables */
  m_nSizeCounts = m_pCurrCount - m_pCounts;
  m_nSizeValues = m_pCurrValues - m_pValues;
  
  // The 13 is for the header size - maybe this should live with some constants somewhere?
  return ( m_nSizeCounts +  m_nSizeValues + 13 ) < m_nBlockSize;
}

