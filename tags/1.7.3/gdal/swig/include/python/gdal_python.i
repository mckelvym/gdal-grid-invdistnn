/*
 * $Id$
 *
 * python specific code for gdal bindings.
 */


%feature("autodoc");

%init %{
  /* gdal_python.i %init code */
  if ( GDALGetDriverCount() == 0 ) {
    GDALAllRegister();
  }
%}

%pythoncode %{

  
  have_warned = 0
  def deprecation_warn( module ):
    global have_warned

    if have_warned == 1:
        return

    have_warned = 1

    from warnings import warn
    warn('%s.py was placed in a namespace, it is now available as osgeo.%s' % (module,module),
         DeprecationWarning)
         
         
  from gdalconst import *
  import gdalconst


  import sys
  byteorders = {"little": "<",
                "big": ">"}
  array_modes = { gdalconst.GDT_Int16:    ("%si2" % byteorders[sys.byteorder]),
                  gdalconst.GDT_UInt16:   ("%su2" % byteorders[sys.byteorder]),
                  gdalconst.GDT_Int32:    ("%si4" % byteorders[sys.byteorder]),
                  gdalconst.GDT_UInt32:   ("%su4" % byteorders[sys.byteorder]),
                  gdalconst.GDT_Float32:  ("%sf4" % byteorders[sys.byteorder]),
                  gdalconst.GDT_Float64:  ("%sf8" % byteorders[sys.byteorder]),
                  gdalconst.GDT_CFloat32: ("%sf4" % byteorders[sys.byteorder]),
                  gdalconst.GDT_CFloat64: ("%sf8" % byteorders[sys.byteorder]),
                  gdalconst.GDT_Byte:     ("%st8" % byteorders[sys.byteorder]),
  }

  def RGBFile2PCTFile( src_filename, dst_filename ):
    src_ds = Open(src_filename)
    if src_ds is None or src_ds == 'NULL':
        return 1

    ct = ColorTable()
    err = ComputeMedianCutPCT( src_ds.GetRasterBand(1),
                               src_ds.GetRasterBand(2),
                               src_ds.GetRasterBand(3),
                               256, ct )
    if err != 0:
        return err

    gtiff_driver = GetDriverByName('GTiff')
    if gtiff_driver is None:
        return 1

    dst_ds = gtiff_driver.Create( dst_filename,
                                  src_ds.RasterXSize, src_ds.RasterYSize )
    dst_ds.GetRasterBand(1).SetRasterColorTable( ct )

    err = DitherRGB2PCT( src_ds.GetRasterBand(1),
                         src_ds.GetRasterBand(2),
                         src_ds.GetRasterBand(3),
                         dst_ds.GetRasterBand(1),
                         ct )
    dst_ds = None
    src_ds = None

    return 0
%}



%include "python_exceptions.i"
%include "python_strings.i"

%extend GDAL_GCP {
%pythoncode {
  def __str__(self):
    str = '%s (%.2fP,%.2fL) -> (%.7fE,%.7fN,%.2f) %s '\
          % (self.Id, self.GCPPixel, self.GCPLine,
             self.GCPX, self.GCPY, self.GCPZ, self.Info )
    return str

  def serialize(self,with_Z=0):
    base = [CXT_Element,'GCP']
    base.append([CXT_Attribute,'Id',[CXT_Text,self.Id]])
    pixval = '%0.15E' % self.GCPPixel       
    lineval = '%0.15E' % self.GCPLine
    xval = '%0.15E' % self.GCPX
    yval = '%0.15E' % self.GCPY
    zval = '%0.15E' % self.GCPZ
    base.append([CXT_Attribute,'Pixel',[CXT_Text,pixval]])
    base.append([CXT_Attribute,'Line',[CXT_Text,lineval]])
    base.append([CXT_Attribute,'X',[CXT_Text,xval]])
    base.append([CXT_Attribute,'Y',[CXT_Text,yval]])
    if with_Z:
        base.append([CXT_Attribute,'Z',[CXT_Text,zval]])        
    return base
} /* pythoncode */
}

%extend GDALRasterBandShadow {
%pythoncode {
  def ReadAsArray(self, xoff=0, yoff=0, win_xsize=None, win_ysize=None,
                  buf_xsize=None, buf_ysize=None, buf_obj=None):
      import gdalnumeric

      return gdalnumeric.BandReadAsArray( self, xoff, yoff,
                                          win_xsize, win_ysize,
                                          buf_xsize, buf_ysize, buf_obj )
    
  def WriteArray(self, array, xoff=0, yoff=0):
      import gdalnumeric

      return gdalnumeric.BandWriteArray( self, array, xoff, yoff )

  def __get_array_interface__(self):
      shape = [1, self.XSize, self.YSize]
      
}
}

%extend GDALDatasetShadow {
%pythoncode {
    def ReadAsArray(self, xoff=0, yoff=0, xsize=None, ysize=None, buf_obj=None ):
        import gdalnumeric
        return gdalnumeric.DatasetReadAsArray( self, xoff, yoff, xsize, ysize, buf_obj )
    def WriteRaster(self, xoff, yoff, xsize, ysize,
                    buf_string,
                    buf_xsize = None, buf_ysize = None, buf_type = None,
                    band_list = None,
                    buf_pixel_space = None, buf_line_space = None, buf_band_space = None ):

        if buf_xsize is None:
            buf_xsize = xsize;
        if buf_ysize is None:
            buf_ysize = ysize;
        if band_list is None:
            band_list = range(1,self.RasterCount+1)
        if buf_type is None:
            buf_type = self.GetRasterBand(1).DataType

        return _gdal.Dataset_WriteRaster(self,
                 xoff, yoff, xsize, ysize,
                buf_string, buf_xsize, buf_ysize, buf_type, band_list,
                buf_pixel_space, buf_line_space, buf_band_space )

    def ReadRaster(self, xoff, yoff, xsize, ysize,
                   buf_xsize = None, buf_ysize = None, buf_type = None,
                   band_list = None,
                   buf_pixel_space = None, buf_line_space = None, buf_band_space = None ):

        if band_list is None:
            band_list = range(1,self.RasterCount+1)
        if buf_xsize is None:
            buf_xsize = xsize;
        if buf_ysize is None:
            buf_ysize = ysize;

        if buf_type is None:
            buf_type = self.GetRasterBand(1).DataType;
        return _gdal.Dataset_ReadRaster(self, xoff, yoff, xsize, ysize,
                                           buf_xsize, buf_ysize, buf_type,
                                           band_list, buf_pixel_space, buf_line_space, buf_band_space )

    def GetSubDatasets(self):
        sd_list = []
        
        sd = self.GetMetadata('SUBDATASETS')
        if sd is None:
            return sd_list

        i = 1
        while sd.has_key('SUBDATASET_'+str(i)+'_NAME'):
            sd_list.append( ( sd['SUBDATASET_'+str(i)+'_NAME'],
                              sd['SUBDATASET_'+str(i)+'_DESC'] ) )
            i = i + 1
        return sd_list
}
}

%extend GDALMajorObjectShadow {
%pythoncode {
  def GetMetadata( self, domain = '' ):
    if domain[:4] == 'xml:':
      return self.GetMetadata_List( domain )
    return self.GetMetadata_Dict( domain )
}
}

/* ==================================================================== */
/*	Support function for progress callbacks to python.              */
/* ==================================================================== */

%{

typedef struct {
    PyObject *psPyCallback;
    PyObject *psPyCallbackData;
    int nLastReported;
} PyProgressData;

/************************************************************************/
/*                          PyProgressProxy()                           */
/************************************************************************/

int CPL_STDCALL
PyProgressProxy( double dfComplete, const char *pszMessage, void *pData )

{
    PyProgressData *psInfo = (PyProgressData *) pData;
    PyObject *psArgs, *psResult;
    int      bContinue = TRUE;

    if( psInfo->nLastReported == (int) (100.0 * dfComplete) )
        return TRUE;

    if( psInfo->psPyCallback == NULL || psInfo->psPyCallback == Py_None )
        return TRUE;

    psInfo->nLastReported = (int) (100.0 * dfComplete);
    
    if( pszMessage == NULL )
        pszMessage = "";

    if( psInfo->psPyCallbackData == NULL )
        psArgs = Py_BuildValue("(dsO)", dfComplete, pszMessage, Py_None );
    else
        psArgs = Py_BuildValue("(dsO)", dfComplete, pszMessage, 
	                       psInfo->psPyCallbackData );

    psResult = PyEval_CallObject( psInfo->psPyCallback, psArgs);
    Py_XDECREF(psArgs);

    if( psResult == NULL )
    {
        return TRUE;
    }

    if( psResult == Py_None )
    {
	Py_XDECREF(Py_None);
        return TRUE;
    }

    if( !PyArg_Parse( psResult, "i", &bContinue ) )
    {
        PyErr_SetString(PyExc_ValueError, "bad progress return value");
	return FALSE;
    }

    Py_XDECREF(psResult);

    return bContinue;    
}
%}
%import typemaps_python.i
