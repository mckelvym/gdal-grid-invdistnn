/*****************************************************************************
 * myassert.h
 *
 * DESCRIPTION
 *    This file contains the code to handle assert statements.  There is no
 * actual code unless DEBUG is defined.
 *
 * HISTORY
 *   12/2003 Arthur Taylor (MDL / RSIS): Created.
 *
 * NOTES
 *****************************************************************************
 */
#ifndef MYASSERT_H
#define MYASSERT_H

#ifndef CPL_C_START
#ifdef __cplusplus
#  define CPL_C_START           extern "C" {
#  define CPL_C_END             }
#else
#  define CPL_C_START
#  define CPL_C_END
#endif
#endif

#ifdef DEBUG
CPL_C_START
   void _myAssert (const char *file, int lineNum);
CPL_C_END

   #define myAssert(f) \
      if (f)          \
         {}           \
      else            \
         _myAssert (__FILE__, __LINE__)
#else
   #define myAssert(f)
#endif

#endif
