#ifndef ESRI_EXPORTS_H_INCLUDED
#define ESRI_EXPORTS_H_INCLUDED

#define ESRI_DMD_MD "MULTIDIMENSION" /* Metadata domain used by Esri scientific implementation (HDF, NetCDf, GRIB drivers)  */

/* ==================================================================== */
/*      GRIBTAB Lookup                                                  */
/* ==================================================================== */

int CPL_DLL CPL_STDCALL GDALAutoLoadGRIBTAB();
int CPL_DLL CPL_STDCALL GDALLoadGRIBTAB(const char *pszFilename);
int CPL_DLL CPL_STDCALL GDALClearGRIBTAB();
int CPL_DLL CPL_STDCALL GDALLookupGRIBTAB(int nCode, int nCenter, int nSubCenter, int nTable,
                                          const char **ppszShortName, const char **ppszLongName, const char **ppszUnit);

#endif
