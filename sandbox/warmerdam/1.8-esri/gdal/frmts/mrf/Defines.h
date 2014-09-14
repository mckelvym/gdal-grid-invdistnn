
#pragma once

typedef unsigned char Byte;

#ifndef max
#define max(a,b)      (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)      (((a) < (b)) ? (a) : (b))
#endif

#ifdef SWAPB    // define this on big endian system

#define LITTLE2LOCAL_ENDIAN_WORD(x) \
  ( ((*((uint16_t *)(&x)) & 0xff00) >>  8) \
  | ((*((uint16_t *)(&x)) & 0x00ff) <<  8) )

#define LITTLE2LOCAL_ENDIAN_DWORD(x) \
  ( ((*((uint32_t *)(&x)) & 0xff000000) >> 24) \
  | ((*((uint32_t *)(&x)) & 0x00ff0000) >>  8) \
  | ((*((uint32_t *)(&x)) & 0x0000ff00) <<  8) \
  | ((*((uint32_t *)(&x)) & 0x000000ff) << 24) )

#define LITTLE2LOCAL_ENDIAN_QWORD(x) \
  ( ((*((uint64_t *)(&x)) & 0xff00000000000000) >> 56) \
  | ((*((uint64_t *)(&x)) & 0x00ff000000000000) >> 40) \
  | ((*((uint64_t *)(&x)) & 0x0000ff0000000000) >> 24) \
  | ((*((uint64_t *)(&x)) & 0x000000ff00000000) >>  8) \
  | ((*((uint64_t *)(&x)) & 0x00000000ff000000) <<  8) \
  | ((*((uint64_t *)(&x)) & 0x0000000000ff0000) << 24) \
  | ((*((uint64_t *)(&x)) & 0x000000000000ff00) << 40) \
  | ((*((uint64_t *)(&x)) & 0x00000000000000ff) << 56) )

#define SWAP_2(x) *((uint16_t *)&x) = LITTLE2LOCAL_ENDIAN_WORD(x)
#define SWAP_4(x) *((uint32_t *)&x) = LITTLE2LOCAL_ENDIAN_DWORD(x)
#define SWAP_8(x) *((uint64_t *)&x) = LITTLE2LOCAL_ENDIAN_QWORD(x)

#else // SWAPB

#define SWAP_2(x)
#define SWAP_4(x)
#define SWAP_8(x)

#endif // SWAPB

