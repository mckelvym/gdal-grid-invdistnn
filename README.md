# GDAL: gdal_grid invdistnn

This is a copy (from SVN) of the [Geospatial Data Abstraction Library (GDAL)](https://github.com/OSGeo/gdal). GDAL is an open source MIT licensed translator library for raster and vector geospatial data formats.

I had made a contribution to implement `invdistnn` for `gdal_grid`, which used inverse distance nearest neighbor for interpolation of points to a grid. This resulted in one dramatic example of one workload that took ~ 48 hours of run time to take ~ 10 minutes with this new option.

My contributions are here, in this repository:

- [gdal_grid: add invdistnn algorithm, variation on the existing inverse distance weighting algorithm with quadtree to search for points only in the neighborhood (patch by mckelvym, #6038)](https://github.com/mckelvym/gdal-grid-invdistnn/commit/f5aef856584cb177f4cbf2c016476a561cd60187)
- [document gdalgrid invdistnn algorithm (patch by mckelvym, #6038)](https://github.com/mckelvym/gdal-grid-invdistnn/commit/8040b83890f0d71ea538f378d437cdca8250ca13)

And in the OSGeo repository:

- [gdal_grid: add invdistnn algorithm, variation on the existing inverse distance weighting algorithm with quadtree to search for points only in the neighborhood (patch by mckelvym, https://trac.osgeo.org/gdal/ticket/6038)](https://github.com/OSGeo/gdal/commit/f888abb6960d991f825dc047cc0f42ec8bd80e01)
- [https://gdal.org/programs/gdal_grid.html#invdistnn](https://gdal.org/programs/gdal_grid.html#invdistnn)
