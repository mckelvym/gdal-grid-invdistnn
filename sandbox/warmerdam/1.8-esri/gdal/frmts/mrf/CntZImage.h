
/*
COPYRIGHT 1995-2011 ESRI

TRADE SECRETS: ESRI PROPRIETARY AND CONFIDENTIAL
Unpublished material - all rights reserved under the
Copyright Laws of the United States.

For additional information, contact:
Environmental Systems Research Institute, Inc.
Attn: Contracts Dept
380 New York Street
Redlands, California, USA 92373

email: contracts@esri.com
*/

#pragma once

// ---- includes ------------------------------------------------------------ ;

#include "TImage.hpp"
#include "BitStuffer.h"

// -------------------------------------------------------------------------- ;

// ---- related classes ----------------------------------------------------- ;

//class String;

// -------------------------------------------------------------------------- ;

/**	count / z image
 *
 *	count can also be a weight, therefore float;
 *	z can be elevation or intensity;
 */

class CntZImage : public TImage< CntZ >
{
public:
  CntZImage();
  virtual ~CntZImage();
  std::string getTypeString() const    { return "CntZImage "; }

  bool resizeFill0(long width, long height);
  bool hasValidPixel() const;
  void normalize();

  /// binary file IO with optional compression
  /// (maxZError = 0  means no lossy compression for Z; the Cnt part is compressed lossless or not at all)
  /// read succeeds only if maxZError on file <= maxZError requested (!)

  unsigned long computeNumBytesNeededToWrite(double maxZError, bool onlyZPart = false)
    { return computeNumBytesNeededToWrite(maxZError, onlyZPart, m_infoFromComputeNumBytes); }

  static unsigned long numExtraBytesToAllocate()    { return BitStuffer::numExtraBytesToAllocate(); }
  static unsigned long computeNumBytesNeededToWriteVoidImage();

  /// these 2 do not allocate memory. Byte ptr is moved like a file pointer.
  bool write(Byte** ppByte,
             double maxZError = 0,
             bool useInfoFromPrevComputeNumBytes = false,
             bool onlyZPart = false) const;

  bool read (Byte** ppByte,
             double maxZError,
             bool onlyHeader = false,
             bool onlyZPart = false);

//  bool write(FILE* fp, double maxZError = 0) const;
//  bool read (FILE* fp, double maxZError, bool onlyHeader = false);

  //bool write(const String& fn, double maxZError = 0) const;
  //bool read (const String& fn, double maxZError, bool onlyHeader = false);


  template<class T> bool ConvertToMemBlock(T* arr, T noDataValue) const
  {
    if (!arr)
      return false;

    const CntZ* srcPtr = getData();
    T* dstPtr = arr;

    for (long i = 0; i < height_; i++)
      for (long j = 0; j < width_; j++)
      {
        if (srcPtr->cnt > 0)
          *dstPtr++ = (T)srcPtr->z;
        else
          *dstPtr++ = noDataValue;
        srcPtr++;
      }

    return true;
  };


protected:

  struct InfoFromComputeNumBytes
  {
    double maxZError;
    bool cntsNoInt;
    long numTilesVertCnt;
    long numTilesHoriCnt;
    long numBytesCnt;
    float maxCntInImg;
    long numTilesVertZ;
    long numTilesHoriZ;
    long numBytesZ;
    float maxZInImg;
  };

  unsigned long computeNumBytesNeededToWrite(double maxZError, bool onlyZPart,
    InfoFromComputeNumBytes& info) const;


  bool write(void* fp,
             Byte** ppByte,
             double maxZError,
             bool useInfoFromPrevComputeNumBytes,
             bool onlyZPart) const;

  bool read( void* fp,
             Byte** ppByte,
             double maxZError,
             bool onlyHeader,
             bool onlyZPart);

  bool findTiling(bool zPart, double maxZError, bool cntsNoInt, 
    long& numTilesVert, long& numTilesHori, long& numBytesOpt, float& maxValInImg) const;

  bool writeTiles(bool zPart, double maxZError, bool cntsNoInt,
    long numTilesVert, long numTilesHori, Byte* bArr, long& numBytes, float& maxValInImg) const;

  bool readTiles( bool zPart, double maxZErrorInFile,
    long numTilesVert, long numTilesHori, float maxValInImg, Byte* bArr);

  bool cntsNoInt() const;
  bool computeCntStats(long i0, long i1, long j0, long j1, float& cntMin, float& cntMax) const;
  bool computeZStats(  long i0, long i1, long j0, long j1, float& zMin, float& zMax, long& numValidPixel) const;

  long numBytesCntTile(long numPixel, float cntMin, float cntMax, bool cntsNoInt) const;
  long numBytesZTile(long numValidPixel, float zMin, float zMax, double maxZError) const;

  bool writeCntTile(Byte** ppByte, long& numBytes, long i0, long i1, long j0, long j1,
    float cntMin, float cntMax, bool cntsNoInt) const;

  bool writeZTile(Byte** ppByte, long& numBytes, long i0, long i1, long j0, long j1,
    long numValidPixel, float zMin, float zMax, double maxZError) const;

  bool readCntTile(Byte** ppByte, long i0, long i1, long j0, long j1);
  bool readZTile(  Byte** ppByte, long i0, long i1, long j0, long j1, double maxZErrorInFile, float maxZInImg);

  int numBytesFlt(float z) const;    // returns 1, 2, or 4
  bool writeFlt(Byte** ppByte, float z, int numBytes) const;
  bool readFlt( Byte** ppByte, float& z, int numBytes) const;

protected:

  InfoFromComputeNumBytes m_infoFromComputeNumBytes;

  std::vector<unsigned long> m_tmpDataVec;    // used in read fcts
};

// -------------------------------------------------------------------------- ;

