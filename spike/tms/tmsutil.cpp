
#include "tmsutil.h"

#include "cpl_string.h"
#include "cpl_port.h"

#include <cstdlib>

bool GetXMLValue (const char *desc, CPLXMLNode *xml, const char *attr, 
                  const char **result, int err_no)
{
    const char *ret = CPLGetXMLValue (xml, attr, NULL);
    if (!ret) {
        CPLError (CE_Failure, err_no, "%s was not provided.", desc);
        fail;
    } 

    *result = ret;
    succes;
}

bool ParseDouble (const char *desc, const char *str, double *result, int err_no)
{
    char *end;
    double v;
    
    errno = 0;
    v = strtod (str, &end);
    if (*end != '\0') {
        CPLError (CE_Failure, err_no,
            "%s[%s] is not a valid real number.", desc, str);
        fail;
    }
    if ((v == HUGE_VAL || v == -HUGE_VAL) && errno != 0) {
        CPLError (CE_Failure, err_no,
            "%s[%s] value would cause overflow.", desc, str);
        fail;
    }
    
    *result = v;
    succes;
}

bool ParseLong (const char *desc, const char *str, long *result, int err_no)
{
    char *end;
    long v;
    
    errno = 0;
    v = strtol (str, &end, 10);
    if (*end != '\0') {
        CPLError (CE_Failure, err_no,
            "%s[%s] is not a valid integer number.", desc, str);
        fail;
    }
    if ((v == LONG_MIN || v == LONG_MAX) && errno != 0) {
        CPLError (CE_Failure, CPLE_OpenFailed,
            "%s[%s] value would cause overflow.", desc, str);
        fail;
    }
    
    *result = v;
    succes;
}

bool InRangeDouble (const char *desc, double val, double min, double max, int err_no)
{
    if (val < min || val > max) {
        CPLError (CE_Failure, err_no,
            "%s[%f] is out of range [%f ; %f].", 
            desc, val, min, max);
        fail;
    } else
        succes;
}

bool InRangeLong (const char *desc, long val, long min, long max, int err_no)
{
    if (val < min || val > max) {
        CPLError (CE_Failure, err_no,
            "%s[%ld] is out of range [%ld ; %ld].", 
            desc, val, min, max);
        fail;
    } else
        succes;
}

bool CreateDir (const char *dirname)
{
    if (VSIMkdir (dirname, 0777) != 0) {
        CPLError (CE_Failure, CPLE_FileIO,
            "Can not create directory '%s'.", dirname);
        fail;
    } else
        succes;
}

bool OpenFileWrite (const char *name, FILE **result)
{
    FILE *fd;
    
    fd = VSIFOpen (name, "w");
    if (!fd) {
        CPLError (CE_Failure, CPLE_FileIO,
            "Can not open output file '%s'.", name);
        fail;
    }
    
    *result = fd;
    succes;
}
