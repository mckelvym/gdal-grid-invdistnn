/******************************************************************************
 * $Id$
 *
 * Name:     gdal_csharp_extensions.i
 * Project:  GDAL SWIG Interface
 * Purpose:  C# specific GDAL extensions 
 * Author:   Tamas Szekeres (szekerest@gmail.com)
 *
 */
 
/******************************************************************************
 * GDAL raster R/W support                                                    *
 *****************************************************************************/

%extend GDALRasterBandShadow 
{
	%apply (void *buffer_ptr) {void *buffer};
	CPLErr ReadRaster(int xOff, int yOff, int xSize, int ySize, void* buffer,
                          int buf_xSize, int buf_ySize, GDALDataType buf_type, 
                          int pixelSpace, int lineSpace) {
       return GDALRasterIO( self, GF_Read, xOff, yOff, xSize, ySize, 
		        buffer, buf_xSize, buf_ySize, buf_type, pixelSpace, lineSpace );
    }
    CPLErr WriteRaster(int xOff, int yOff, int xSize, int ySize, void* buffer,
                          int buf_xSize, int buf_ySize, GDALDataType buf_type, 
                          int pixelSpace, int lineSpace) {
       return GDALRasterIO( self, GF_Write, xOff, yOff, xSize, ySize, 
		        buffer, buf_xSize, buf_ySize, buf_type, pixelSpace, lineSpace );
    }
    %clear void *buffer;
}

%extend GDALDatasetShadow 
{
	%apply (void *buffer_ptr) {void *buffer};
	CPLErr ReadRaster(int xOff, int yOff, int xSize, int ySize, void* buffer,
                          int buf_xSize, int buf_ySize, GDALDataType buf_type, 
                          int bandCount, int pixelSpace, int lineSpace, int bandSpace) {
       return GDALDatasetRasterIO( self, GF_Read, xOff, yOff, xSize, ySize, 
		        buffer, buf_xSize, buf_ySize, buf_type, bandCount, 
		        NULL, pixelSpace, lineSpace, bandSpace);
    }
    CPLErr WriteRaster(int xOff, int yOff, int xSize, int ySize, void* buffer,
                          int buf_xSize, int buf_ySize, GDALDataType buf_type, 
                          int bandCount, int pixelSpace, int lineSpace, int bandSpace) {
       return GDALDatasetRasterIO( self, GF_Write, xOff, yOff, xSize, ySize, 
		        buffer, buf_xSize, buf_ySize, buf_type, bandCount, 
		        NULL, pixelSpace, lineSpace, bandSpace);
    }
    %clear void *buffer;
    
    %apply (void *buffer_ptr) {const GDAL_GCP* __GetGCPs};
    const GDAL_GCP* __GetGCPs( ) {
      return GDALGetGCPs( self );
    }
    %clear const GDAL_GCP* __GetGCPs;
    
    CPLErr __SetGCPs( int nGCPs, GDAL_GCP const *pGCPs, const char *pszGCPProjection ) {
        return GDALSetGCPs( self, nGCPs, pGCPs, pszGCPProjection );
    }
    IMPLEMENT_ARRAY_MARSHALER(GDAL_GCP)
}

IMPLEMENT_ARRAY_MARSHALER_STATIC(GDAL_GCP)