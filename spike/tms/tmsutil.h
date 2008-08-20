
#ifndef TMSUTIL_H_INCLUDED
#define TMSUTIL_H_INCLUDED

#include "cpl_minixml.h"

#define mkarray(type,size)  (( type * ) CPLMalloc (sizeof (type) * (size)))
#define forall(i,count)     for (int i = 0; i < count; i++)

#define succes      return true
#define fail        return false
#define must(expr)  if (!(expr)) { fail; }

#define bool_to_CE_Error(expr)  ((expr) ? CE_None : CE_Failure)

#define assert(expr)    CPLAssert (expr)
#define assert_in_range(expr, bound)    assert ((expr) >= 0 && (expr) < (bound))


bool GetXMLValue (const char *desc, CPLXMLNode *xml, const char *attr, 
                  const char **result, int err_no);
                  
bool ParseDouble (const char *desc, const char *str, double *result, int err_no);
bool ParseLong   (const char *desc, const char *str, long *result, int err_no);

bool InRangeDouble (const char *desc, double val, double min, double max, int err_no);
bool InRangeLong   (const char *desc, long val, long min, long max, int err_no);

bool CreateDir (const char *dirname);
bool OpenFileWrite (const char *name, FILE **result);

#endif

