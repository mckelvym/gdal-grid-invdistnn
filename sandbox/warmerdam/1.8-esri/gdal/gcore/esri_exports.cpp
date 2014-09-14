#include "gdal.h"

#include "../frmts/grib/degrib18/degrib/customgribtab.h"

/* Exported function for used outside GDAL */

int CPL_STDCALL GDALAutoLoadGRIBTAB()
{
    const char *pszGRIBTABPath = CPLGetConfigOption("GDAL_GRIBTAB_PATH", "NONE");
    if (pszGRIBTABPath)
    {
        if (EQUAL(pszGRIBTABPath, "NONE"))
        {
            GRIBTABUnloadTables();
            CE_None;
        }
        else
        {
            if (!GRIBTABLoadUserTable(pszGRIBTABPath))
            {
                CPLError( CE_Failure, CPLE_AppDefined,
                    "Unable to load user tables from specified path: %s\n", pszGRIBTABPath);
                return CE_Failure;
            }
        }
    }
    return CE_None;
}

int CPL_STDCALL GDALLoadGRIBTAB(const char *pszFilename)
{
    if (GRIBTABLoadUserTable(pszFilename))
        return CE_None;
    return CE_Failure;
}

int CPL_STDCALL GDALClearGRIBTAB()
{
    GRIBTABUnloadTables();
    return CE_None;
}

int CPL_STDCALL GDALLookupGRIBTAB(int nCode, int nCenter, int nSubCenter, int nTable,
                      const char **ppszShortName, const char **ppszLongName, const char **ppszUnit)
{
    GRIBTABEntry result = GRIBTABLookUp(nCode, nCenter, nSubCenter, nTable,
                                        ppszShortName, ppszLongName, ppszUnit);
    if (!result.empty())
        return CE_None;
    return CE_Failure;
}
