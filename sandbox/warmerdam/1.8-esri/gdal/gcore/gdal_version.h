
/* -------------------------------------------------------------------- */
/*      GDAL Version Information.                                       */
/* -------------------------------------------------------------------- */

#ifndef GDAL_VERSION_MAJOR
#  define GDAL_VERSION_MAJOR    1
#  define GDAL_VERSION_MINOR    8
#  define GDAL_VERSION_REV      0
#  define GDAL_VERSION_BUILD    10300
#endif

/* GDAL_COMPUTE_VERSION macro introduced in GDAL 1.10 */
/* Must be used ONLY to compare with version numbers for GDAL >= 1.10 */
#ifndef GDAL_COMPUTE_VERSION
#define GDAL_COMPUTE_VERSION(maj,min,rev) ((maj)*1000000+(min)*10000+(rev)*100)
#endif

/* Note: the formula to compute GDAL_VERSION_NUM has changed in GDAL 1.10 */
#ifndef GDAL_VERSION_NUM
#  define GDAL_VERSION_NUM      (GDAL_VERSION_MAJOR*1000+GDAL_VERSION_MINOR*100+GDAL_VERSION_REV*10+GDAL_VERSION_BUILD)
#endif

#ifndef GDAL_RELEASE_DATE
#  define GDAL_RELEASE_DATE     20110112
#endif
#ifndef GDAL_RELEASE_NAME
#  define GDAL_RELEASE_NAME     "1.8+e"
#endif

