#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:	GDAL2Tiles, Google Summer of Code 2007 & 2008
# Purpose:	Convert a raster into TMS tiles, create KML SuperOverlay EPSG:4326,
#			generate a simple HTML viewers based on Google Maps and OpenLayers
# Author:	Klokan Petr Pridal, klokan at klokan dot cz
# Web:		http://www.klokan.cz/projects/gdal2tiles/
#
###############################################################################
# Copyright (c) 2008 Klokan Petr Pridal. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
###############################################################################

from globalmaptiles import *
from templates import *

###################################################################

def DatasetGeoQuery(ds, ulx, uly, lrx, lry, querysize = 0):
	"""For given dataset and query in cartographic coordinates
	returns parameters for ReadRaster() in raster coordinates and
	x/y shifts (for border tiles). If the querysize is not given, the
	extent is returned in the native resolution of dataset ds."""

	geotran = ds.GetGeoTransform()
	rx= int((ulx - geotran[0]) / geotran[1] + 0.001)
	ry= int((uly - geotran[3]) / geotran[5] + 0.001)
	rxsize= int((lrx - ulx) / geotran[1] + 0.5)
	rysize= int((lry - uly) / geotran[5] + 0.5)

	if not querysize:
		wxsize, wysize = rxsize, rysize
	else:
		wxsize, wysize = querysize, querysize

	# Coordinates should not go out of the bounds of the raster
	wx = 0
	if rx < 0:
		rxshift = abs(rx)
		wx = int( wxsize * (float(rxshift) / rxsize) )
		wxsize = wxsize - wx
		rxsize = rxsize - int( rxsize * (float(rxshift) / rxsize) )
		rx = 0
	if rx+rxsize > ds.RasterXSize:
		wxsize = int( wxsize * (float(ds.RasterXSize - rx) / rxsize) )
		rxsize = ds.RasterXSize - rx

	wy = 0
	if ry < 0:
		ryshift = abs(ry)
		wy = int( wysize * (float(ryshift) / rysize) )
		wysize = wysize - wy
		rysize = rysize - int( rysize * (float(ryshift) / rysize) )
		ry = 0
	if ry+rysize > ds.RasterYSize:
		wysize = int( wysize * (float(ds.RasterYSize - ry) / rysize) )
		rysize = ds.RasterYSize - ry

	return (rx, ry, rxsize, rysize), (wx, wy, wxsize, wysize)

#---------------------

def ScaleQueryToTile(dsquery, dstile, tilefilename=''):
	"""Scales down query dataset to the tile dataset"""

	querysize = dsquery.RasterXSize
	tilesize = dstile.RasterXSize

	if resampling_method == 'average':
		# gdal.RegenerateOverview()
		for i in range(1,tilebands+1):
			res = gdal.RegenerateOverview( dsquery.GetRasterBand(i),
				dstile.GetRasterBand(i), 'average' )
			if res != 0:
			    print "RegenerateOverview() failed on %s, error %d" % (tilefilename, res)
			    sys.exit( 1 )

	elif resampling_method == 'antialias':
		# Scaling by PIL (Python Imaging Library) - Lanczos
		array = numpy.zeros((querysize, querysize, tilebands), numpy.uint8)
		for i in range(tilebands):
			array[:,:,i] = gdalarray.BandReadAsArray(dsquery.GetRasterBand(i+1), 0, 0, querysize, querysize)
		im = Image.fromarray(array, ('RGBA' if tilebands == 4 else 'RGB'))
		im1 = im.resize((tilesize,tilesize), Image.ANTIALIAS)
		im1.save(tilefilename,tileformat)
	else:
		# Other algorithms are implemented by gdal.ReprojectImage().
		dsquery.SetGeoTransform( (0.0, tilesize / float(querysize), 0.0, 0.0, 0.0, tilesize / float(querysize)) )
		dstile.SetGeoTransform( (0.0, 1.0, 0.0, 0.0, 0.0, 1.0) )

		res = gdal.ReprojectImage(dsquery, dstile, None, None, ResamplingMethod)
		if res != 0:
		    print "ReprojectImage() failed on %s, error %d" % (tilefilename, res)
		    sys.exit( 1 )

#---------------------

if __name__ == "__main__":

	try:
		from osgeo import gdal
		from osgeo import osr
	except ImportError:
		import gdal
		import osr
		
	import sys, os
		
	def Usage():
		print "Usage: globalmaptiles.py [-s_srs srs_def] [-r resampling_method] source_file"
		sys.exit(1)

	src_filename = None
	s_srs = None
	nogooglemaps = False
	noopenlayers = False
	generatekml = True
	tileformat = 'png' # 'jpg'
	resampling_method = 'average'
	resampling_method_list = ('average','near','bilinear','cubic','cubicspline','lanczos','antialias')
	tilenodata = 255 # by default white background for tile borders
	tminz, tmaxz = 0, 0
	
	#sys.argv=['globalmaptiles.py','../USGS-topo/O38108a6.tif']
	gdal.AllRegister()	
	
	argv = gdal.GeneralCmdLineProcessor( sys.argv )
	if argv is None:
		sys.exit( 0 )
	
	# Parse command line arguments.
	i = 1
	while i < len(argv):
		arg = argv[i]

		if arg == '-s_srs':
			i = i + 1
			s_srs = argv[i]
		
		elif arg == '-r': # resampling_method
			i = i + 1
			resampling_method = argv[i]
			if resampling_method not in resampling_method_list:
				print "Unknown resampling method. Supported are: %s " % ', '.join(resampling_method_list)
				Usage() 
		
		elif arg == '-z': # tminz, tmaxz
			pass
		
		elif arg == '-profile': # tile profile - 'mercator', 'geodetic', 'raster' (old gdal2tiles), 'zoomify'
			pass

		elif arg == '-tileformat': # tile format - 'png', 'jpg'
			pass
			
		elif arg == '-forcekml':
			pass
	
		elif src_filename is None:
			src_filename = argv[i]

		else:
			Usage()

		i = i + 1

	if not src_filename:
		Usage()
	
	memdriver = gdal.GetDriverByName( 'MEM' )
	tiledriver = gdal.GetDriverByName( tileformat )
		
	src_ds = gdal.Open(src_filename, gdal.GA_ReadOnly)
	if not src_ds:
		print "Could not open source file:", src_filename
		Usage()

	if src_ds.RasterCount != 4:
		print "Now only 4 bands (RGBA) dataset is supported as the source"
		print "From paletted file you can create such (temp.vrt) by:"
		print "gdal_translate -of vrt -expand rgba yourfile.xxx temp.vrt"
		Usage()
	
	# TODO: 'nodata' mask support:
	
	# http://trac.osgeo.org/gdal/browser/trunk/autotest/gcore/mask.py
	# get the mask band by querying GetMaskBand() on your first band.
	# print ds.GetRasterBand(1).GetNoDataValue() returns None if there's no nodata value!
	
	# We need to reproject the source file to EPSG:900913
	
	ref = osr.SpatialReference()
	ref.ImportFromEPSG(900913)
	t_srs_wkt = ref.ExportToWkt()
	
	if s_srs:
		ref.SetFromUserInput(s_srs)
		s_srs_wkt = ref.ExportToWkt()
	else:
		s_srs_wkt = src_ds.GetProjection()
		if not s_srs_wkt and src_ds.GetGCPCount() != 0:
			s_srs_wkt = src_ds.GetGCPProjection()
	
	if (src_ds.GetGeoTransform() == (0.0, 1.0, 0.0, 0.0, 0.0, 1.0) and src_ds.GetGCPCount() == 0):
		print "There is no georeference - neither affine transformation (worldfile) nor GCPs"
		print "Use a GIS software for georeference e.g. gdal_transform -gcp / -a_ullr / -a_srs"
		Usage()

	#print "s_srs_wkt: '%s'" % s_srs_wkt
	if not s_srs_wkt:
		print "The spatial reference system of the raster is not known. Specify it by -s_srs."
		Usage()

	# Generation of VRT dataset in EPSG:900913, default 'nearest neighbour' warping
	ds = gdal.AutoCreateWarpedVRT( src_ds, s_srs_wkt, t_srs_wkt )

	# Save the VRT file for usage by other GDAL tools:
	##ds.GetDriver().CreateCopy("tiles.vrt", ds)

	print "Source file:", src_filename, "( %sP x %sL - %s bands)" % (src_ds.RasterXSize, src_ds.RasterYSize, src_ds.RasterCount)
	print "Tile projected file:", "tiles.vrt", "( %sP x %sL - %s bands)" % (ds.RasterXSize, ds.RasterYSize, ds.RasterCount)
	geotran = ds.GetGeoTransform()

	originX, originY = geotran[0], geotran[3]
	pixelSize = geotran[1] # = geotran[5]
	
	# TODO: Change Title
	title = src_filename
	
	# warped to epsg:900913: pixelSize is square, no rotation on the raster 
	mmaxy = originY
	mminy = originY-ds.RasterYSize*pixelSize
	mminx = originX
	mmaxx = originX+ds.RasterXSize*pixelSize
	
	# Main object for counting the Mercator tiles extents
	mercator = GlobalMercator()
	
	print "OriginX, OriginY, PixelSize", (originX, originY), pixelSize
	print "Lat/Lon bounds:", mercator.MetersToLatLon( mminx, mminy), mercator.MetersToLatLon( mmaxx, mmaxy)

	# Generate table with min max tile coordinates for all zoomlevels
	tminmax = range(0,32)
	for tz in range(0, 32):
		tminx, tminy = mercator.MetersToTile( mminx, mminy, tz )
		tmaxx, tmaxy = mercator.MetersToTile( mmaxx, mmaxy, tz )
		tminmax[tz] = (tminx, tminy, tmaxx, tmaxy)

	# Get the minimal zoom level (map is covered by one tile)
	if not tminz:
		for tz in range(31, -1, -1):
			tminx, tminy, tmaxx, tmaxy = tminmax[tz]
			if (tminx == tmaxx and tminy == tmaxy):
				tminz = tz
				break
	print 'MinZoomLevel:', tminz

	# Get the maximal zoom level (closest possible zoom level up on the resolution of raster)
	if not tmaxz:
		tmaxz = mercator.ZoomForPixelSize( pixelSize )
		
		
	res = mercator.Resolution( tmaxz )
	print "MaxZoomLevel:", tmaxz, "(",res,")"

	tminx, tminy, tmaxx, tmaxy = tminmax[tmaxz]

	tilesize = 256
	tilebands = 4
	output_dir = '.'
	googlemapskey = 'INSERT_YOUR_GOOGLE_KEY_HERE'
	yahooappid = 'INSERT_YOUR_YAHOO_APP_ID_HERE'
	publishurl = ''
	
	########################
	## Generate text metadata
	########################
	
	# Generate tilemapresource.xml.
	# TODO
	#f = open(os.path.join(output_dir, 'tilemapresource.xml'), 'w')
	#f.write( generate_tilemapresource( ... ))
	#f.close()

	south, west = mercator.MetersToLatLon( mminx, mminy)
	north, east = mercator.MetersToLatLon( mmaxx, mmaxy)
	#sw ne =
	
	# Generate googlemaps.html
	if not nogooglemaps:
		f = open(os.path.join(output_dir, 'googlemaps.html'), 'w')
		f.write( generate_googlemaps_overlay(
		  title = title,
		  googlemapskey = googlemapskey,
		  north = north,
		  south = south,
		  east = east,
		  west = west,
		  minzoom = tminz,
		  maxzoom = tmaxz,
		  tilesize = tilesize,
		  publishurl = publishurl
		))
		f.close()

	# Generate openlayers.html
	if not noopenlayers:
		f = open(os.path.join(output_dir, 'openlayers.html'), 'w')
		f.write( generate_openlayers_overlay(
		  title = title,
		  googlemapskey = googlemapskey,
		  yahooappid = yahooappid,
		  north = north,
		  south = south,
		  east = east,
		  west = west,
		  minzoom = tminz,
		  maxzoom = tmaxz,
		  tilesize = tilesize,
		  publishurl = publishurl
		))
		f.close()
	
	x, y, x, y = tminmax[tminz]
	childkml = "%s/%s/%s.kml" % (tminz, x, y)
	# Generate Root KML
	if generatekml:
		f = open(os.path.join(output_dir, 'doc.kml'), 'w')
		f.write( generate_rootkml(
			title = title,
			north = north,
			south = south,
			east = east,
			west = west,
			tilesize = tilesize,
			tileformat = tileformat,
			publishurl = publishurl,
			childkml = childkml
		))
		f.close()
		
	########################
	## Generate all tiles in the max zoom level
	########################

	px, py = mercator.MetersToPixels( originX, originY, tmaxz)
	mx, my = mercator.PixelsToMeters( px, py, tmaxz) # OriginX, OriginY
	print "Pixel coordinates:", px, py, (mx, my)
	print
	print "Tiles generated from the max zoom level:"
	print "----------------------------------------"
	print

	# Just the center tile
	#tminx = tminx+ (tmaxx - tminx)/2
	#tminy = tminy+ (tmaxy - tminy)/2
	#tmaxx = tminx
	#tmaxy = tminy

	if resampling_method == 'average':
		ResamplingMethod = None
		querysize = tilesize * 4
	if resampling_method == 'near':
		ResamplingMethod = gdal.GRA_NearestNeighbour
		querysize = tilesize
	if resampling_method == 'bilinear':
		ResamplingMethod = gdal.GRA_Bilinear
		querysize = tilesize * 2
	elif resampling_method == 'cubic':
		ResamplingMethod = gdal.GRA_Cubic
		querysize = tilesize * 4
	elif resampling_method == 'cubicspline':
		ResamplingMethod = gdal.GRA_CubicSpline
		querysize = tilesize * 4
	elif resampling_method == 'lanczos':
		ResamplingMethod = gdal.GRA_Lanczos
		querysize = tilesize * 4
	elif resampling_method == 'antialias':
		querysize = tilesize * 4
		import osgeo.gdal_array as gdalarray
		import numpy
		from PIL import Image	
	
	s = """
	"""
	tz = tmaxz
	for ty in range(tminy, tmaxy+1):
		for tx in range(tminx, tmaxx+1):

			tilefilename = "%s/%s/%s.%s" % (tz, tx, ty, tileformat)
			print tilefilename, "( TileMapService: z / x / y )"

			# Create directories for the tile
			if not os.path.exists(os.path.dirname(tilefilename)):
				os.makedirs(os.path.dirname(tilefilename))
			
			# Tile bounds in EPSG:900913
			b = mercator.TileBounds(tx, ty, tz)
			
			print "\tgdalwarp -ts 256 256 -te %s %s %s %s %s %s_%s_%s.tif" % ( b[0], b[1], b[2], b[3], "tiles.vrt", tz, tx, ty)
					
			# Don't scale up by nearest neighbour, better change the querysize
			# to the native resolution (and return smaller query tile) for scaling

			rb, wb = DatasetGeoQuery( ds, b[0], b[3], b[2], b[1])
			nativesize = wb[0]+wb[2] # Pixel size in the raster covering query geo extent
			print "\tNative Extent (querysize",nativesize,"): ", rb, wb
			
			#if nativesize < tilesize:
			#	querysize = nativesize 
			
			# Tile bounds in raster coordinates for ReadRaster query
			rb, wb = DatasetGeoQuery( ds, b[0], b[3], b[2], b[1], querysize=querysize)
			
			rx, ry, rxsize, rysize = rb
			wx, wy, wxsize, wysize = wb
			print "\tReadRaster Extent: ", rb, wb

			# Query is in 'nearest neighbour' but can be bigger in then the tilesize
			# We scale down the query to the tilesize by supplied algorithm.
			
			# Tile dataset in memory
			dstile = memdriver.Create('', tilesize, tilesize, tilebands)
			data = ds.ReadRaster(rx, ry, rxsize, rysize, wxsize, wysize)
			
			if tilesize == querysize:
				# Use the ReadRaster result directly in tiles ('nearest neighbour' query)
				dstile.WriteRaster(wx, wy, wxsize, wysize, data, band_list=range(1,tilebands+1))
				
				# Note: For source drivers based on WaveLet compression (JPEG2000, ECW, MrSID)
				# the ReadRaster function returns high-quality raster (not ugly nearest neighbour)
			else:
				# Big ReadRaster query in memory scaled to the tilesize - all but 'near' algo
				dsquery = memdriver.Create('', querysize, querysize, tilebands)
				# TODO: fill the null value
				#for i in range(1, tilebands+1):
				#	dsquery.GetRasterBand(1).Fill(tilenodata)
				dsquery.WriteRaster(wx, wy, wxsize, wysize, data, band_list=range(1,tilebands+1))
				
				# TODO:
				ScaleQueryToTile(dsquery, dstile, tilefilename)
				del dsquery
			
			del data
		
			if resampling_method != 'antialias':
				# Write a copy of tile to png/jpg
				tiledriver.CreateCopy(tilefilename, dstile, strict=0)
			
			del dstile
			
			# Create a KML file for this tile.
			if generatekml:
				f = open( os.path.join(output_dir, '%d/%d/%d.kml' % (tz, tx, ty)), 'w')
				f.write( generate_kml_new(
					swne = mercator.TileLatLonBounds(tx, ty, tz),
					tx = tx, ty = ty, tz = tz,
					tilesize = tilesize,
					tileformat = tileformat,
					maxzoom = tmaxz,
					childern = []
				))
				f.close()
			
	########################
	## Build upper levels of the pyramid from already generated tiles of the maxzoom level...
	########################
		
	s = """
	"""
	
	# Usage of existing tiles: from 4 underlying tiles generate one as overview.
	querysize = tilesize * 2
	for tz in range(tmaxz-1, tminz-1, -1):
		tminx, tminy, tmaxx, tmaxy = tminmax[tz]
		for ty in range(tminy, tmaxy+1):
			for tx in range(tminx, tmaxx+1):
				
				tilefilename = "%s/%s/%s.%s" % (tz, tx, ty, tileformat)
				print tilefilename, "( TileMapService: z / x / y )"

				# Create directories for the tile
				if not os.path.exists(os.path.dirname(tilefilename)):
					os.makedirs(os.path.dirname(tilefilename))
				
				dsquery = memdriver.Create('', querysize, querysize, tilebands)
				# TODO: fill the null value
				#for i in range(1, tilebands+1):
				#	dsquery.GetRasterBand(1).Fill(tilenodata)
				dstile = memdriver.Create('', tilesize, tilesize, tilebands)
				
				# TODO: Implement more clever walking on the tiles with cache functionality
				# probably walk should start with reading of four tiles from top left corner
				
				kmlchildern = []
				# Read the tiles and write them to query window
				for y in range(2*ty,2*ty+2):
					for x in range(2*tx,2*tx+2):
						minx, miny, maxx, maxy = tminmax[tz+1]
						if x >= minx and x <= maxx and y >= miny and y <= maxy:
							dsquerytile = gdal.Open("%s/%s/%s.%s" % (tz+1, x, y, tileformat), gdal.GA_ReadOnly)
							dsquery.WriteRaster( x % (2*tx) * tilesize, tilesize if not (y % (2*ty)) else 0, tilesize, tilesize,
								dsquerytile.ReadRaster(0,0,tilesize,tilesize),
								band_list=range(1,tilebands+1))
							kmlchildern.append({ 'swne': mercator.TileLatLonBounds(x, y, tz+1) })
						else:
							kmlchildern.append({})

				ScaleQueryToTile(dsquery, dstile, tilefilename)
				# Write a copy of tile to png/jpg
				tiledriver.CreateCopy(tilefilename, dstile, strict=0)
				
				print "\tbuild from zoom", tz+1," tiles:", (2*tx, 2*ty), (2*tx+1, 2*ty),(2*tx, 2*ty+1), (2*tx+1, 2*ty+1)
				
				# Create a KML file for this tile.
				if generatekml:
					f = open( os.path.join(output_dir, '%d/%d/%d.kml' % (tz, tx, ty)), 'w')
					f.write( generate_kml_new(
						swne = mercator.TileLatLonBounds(tx, ty, tz),
						tx = tx, ty = ty, tz = tz,
						tilesize = tilesize,
						tileformat = tileformat,
						maxzoom = tmaxz,
						childern = kmlchildern
					))
					f.close()
	

	########################
	# Pass all the PNG tiles to 'advpng -4 -z' or use 'optipng library'/'pngcrush'?
	########################	
	
s = '''	

try:
	from osgeo import gdal
	from osgeo.gdalconst import GA_ReadOnly
	from osgeo.osr import SpatialReference
except ImportError:
	import gdal
	from gdalconst import GA_ReadOnly
	from osr import SpatialReference

import sys, os, tempfile
from math import ceil, log10
import operator

from templates import *
from globalmaptiles import GlobalMercator, GlobalGeodetic

#TODO
#verbose = False
verbose = True

tilesize = 256
tileformat = 'png'

tempdriver = gdal.GetDriverByName( 'MEM' )
tiledriver = gdal.GetDriverByName( tileformat )

# =============================================================================
def writetile( filename, data, dxsize, dysize, bands):
	"""
	Write raster 'data' (of the size 'dataxsize' x 'dataysize') read from
	'dataset' into the tile 'filename' with size 'tilesize' pixels.
	Later this should be replaced by new <TMS Tile Raster Driver> from GDAL.
	"""

	# Create needed directories for output
	dirs, file = os.path.split(filename)
	if not os.path.isdir(dirs):
		os.makedirs(dirs)

	# GRR, PNG DRIVER DOESN'T HAVE CREATE() !!!
	# so we have to create temporary file in memmory...

	#TODO: Add transparency to files with one band only too (grayscale).
	if bands == 3 and tileformat == 'png':
		tmp = tempdriver.Create('', tilesize, tilesize, bands=4)
		alpha = tmp.GetRasterBand(4)
		alphaarray = (zeros((dysize, dxsize)) + 255).astype('l')
		alpha.WriteArray( alphaarray, 0, tilesize-dysize )
	else:
		tmp = tempdriver.Create('', tilesize, tilesize, bands=bands)

	# (write data from the bottom left corner)
	tmp.WriteRaster( 0, tilesize-dysize, dxsize, dysize, data, band_list=range(1, bands+1))
 
	# ... and then copy it into the final tile with given filename
	tiledriver.CreateCopy(filename, tmp, strict=0)

	return 0



# =============================================================================
def Usage():
	print 'Usage: gdal2tiles.py [-title "Title"] [-publishurl http://yourserver/dir/]'
	print '						[-nogooglemaps] [-noopenlayers] [-nokml]'
	print '						[-googlemapskey KEY] [-forcekml] [-v]'
	print '						input_file [output_dir]'
	print

# =============================================================================
#
# Program mainline.
#
# =============================================================================

if __name__ == '__main__':

	profile = 'local' # later there should be support for TMS global profiles
	title = ''
	publishurl = ''
	googlemapskey = 'INSERT_YOUR_KEY_HERE' # when not supplied as parameter
	nogooglemaps = False
	noopenlayers = False
	nokml = False
	forcekml = False

	input_file = ''
	output_dir = ''

	isepsg4326 = False
	generatekml = False

	gdal.AllRegister()
	argv = gdal.GeneralCmdLineProcessor( sys.argv )
	if argv is None:
		sys.exit( 0 )

	# Parse command line arguments.
	i = 1
	while i < len(argv):
		arg = argv[i]

		if arg == '-v':
			verbose = True

		elif arg == '-nogooglemaps':
			nogooglemaps = True

		elif arg == '-noopenlayers':
			noopenlayers = True

		elif arg == '-nokml':
			nokml = True

		elif arg == '-forcekml':
			forcekml = True

		elif arg == '-title':
			i += 1
			title = argv[i]

		elif arg == '-publishurl':
			i += 1
			publishurl = argv[i]

		elif arg == '-googlemapskey':
			i += 1
			googlemapskey = argv[i]

		elif arg[:1] == '-':
			print >>sys.stderr, 'Unrecognised command option: ', arg
			Usage()
			sys.exit( 1 )

		elif not input_file:
			input_file = argv[i]

		elif not output_dir:
			output_dir = argv[i]

		else:
			print >>sys.stderr, 'Too many parameters already: ', arg
			Usage()
			sys.exit( 1 )
			
		i = i + 1

	if not input_file:
		input_file = 'IIvojenske/google_GW_II_11.tif'
		#TODO
		#print >>sys.stderr, 'No input_file was given.'
		#Usage()
		#sys.exit( 1 )

	# Set correct default values.
	if not title:
		title = os.path.basename( input_file )
	if not output_dir:
		output_dir = os.path.splitext(os.path.basename( input_file ))[0]
	if publishurl and not publishurl.endswith('/'):
		publishurl += '/'
	if publishurl:
		publishurl += os.path.basename(output_dir) + '/'

	# Open input_file and get all necessary information.
	dataset = gdal.Open( input_file, GA_ReadOnly )
	if dataset is None:
		Usage()
		sys.exit( 1 )
		
	bands = dataset.RasterCount
	if bands == 3 and tileformat == 'png':
		try:
			from numpy import zeros
		except ImportError:
			from Numeric import zeros
	xsize = dataset.RasterXSize
	ysize = dataset.RasterYSize

	geotransform = dataset.GetGeoTransform()
	projection = dataset.GetProjection()

	north = geotransform[3]
	south = geotransform[3]+geotransform[5]*ysize
	east = geotransform[0]+geotransform[1]*xsize
	west = geotransform[0]

	mercator = GlobalMercator( tileSize = 256 )

	if verbose:
		print "Input (%s):" % input_file
		print "="*80
		print "	 Driver:", dataset.GetDriver().ShortName,'/', dataset.GetDriver().LongName
		print "	 Size:", xsize, 'x', ysize, 'x', bands
		print "	 Projection:", projection
		print "	 NSEW: ", (north, south, east, west) 
		print "---"
		print "Global Mercator:"
		z = mercator.ZoomForPixelSize( geotransform[1] ) - 1
		print z
		print "Corner tiles:"
		tminx, tminy = mercator.MetersToTile( west, south, z )
		tmaxx, tmaxy = mercator.MetersToTile( east, north, z )
		print tminx, tminy, tmaxx, tmaxy
		tx, ty = tminx + ((tmaxx-tminx)/2), tminy + ((tmaxy-tminy)/2)
		print tx, ty, mercator.TileBounds(tx, ty, z)
		print mercator.GoogleTile(tx, ty, z)

	if projection:
		# CHECK: Is there better way how to test that given file is in EPSG:4326?
		#spatialreference = SpatialReference(wkt=projection)
		#if spatialreference.???() == 4326:
		if projection.endswith('AUTHORITY["EPSG","4326"]]'):
			isepsg4326 = True
			if verbose:
				print "Projection detected as EPSG:4326"

	if (isepsg4326 or forcekml) and (north, south, east, west) != (0, xsize, ysize, 0):
		generatekml = True
	if verbose:
		if generatekml:
			print "Generating of KML is possible"
		else:
			print "It is not possible to generate KML (projection is not EPSG:4326 or there are no coordinates)!"
	
	if nokml:
		generatekml = False

	if forcekml and (north, south, east, west) == (0, xsize, ysize, 0):
		print >> sys.stderr, "Geographic coordinates not available for given file '%s'" % input_file
		print >> sys.stderr, "so KML file can not be generated."
		sys.exit( 1 )


	# Python 2.2 compatibility.
	log2 = lambda x: log10(x) / log10(2) # log2 (base 2 logarithm)
	sum = lambda seq, start=0: reduce( operator.add, seq, start)

	# Zoom levels of the pyramid.
	maxzoom = int(max( ceil(log2(xsize/float(tilesize))), ceil(log2(ysize/float(tilesize)))))
	zoompixels = [geotransform[1] * 2.0**(maxzoom-zoom) for zoom in range(0, maxzoom+1)]
	tilecount = sum( [
		int( ceil( xsize / (2.0**(maxzoom-zoom)*tilesize))) * \
		int( ceil( ysize / (2.0**(maxzoom-zoom)*tilesize))) \
		for zoom in range(maxzoom+1)
	] )

	# Create output directory, if it doesn't exist
	if not os.path.isdir(output_dir):
		os.makedirs(output_dir)

	if verbose:
		print "Output (%s):" % output_dir
		print "="*80
		print "	 Format of tiles:", tiledriver.ShortName, '/', tiledriver.LongName
		print "	 Size of a tile:", tilesize, 'x', tilesize, 'pixels'
		print "	 Count of tiles:", tilecount
		print "	 Zoom levels of the pyramid:", maxzoom
		print "	 Pixel resolution by zoomlevels:", zoompixels

	# Generate tilemapresource.xml.
	f = open(os.path.join(output_dir, 'tilemapresource.xml'), 'w')
	f.write( generate_tilemapresource( 
		title = title,
		north = north,
		south = south,
		east = east,
		west = west,
		isepsg4326 = isepsg4326,
		projection = projection,
		publishurl = publishurl,
		zoompixels = zoompixels,
		tilesize = tilesize,
		tileformat = tileformat,
		profile = profile
	))
	f.close()

	# Generate googlemaps.html
	if not nogooglemaps:
		f = open(os.path.join(output_dir, 'googlemaps.html'), 'w')
		f.write( generate_googlemaps(
		  title = title,
		  googlemapskey = googlemapskey,
		  xsize = xsize,
		  ysize = ysize,
		  maxzoom = maxzoom,
		  tilesize = tilesize
		))
		f.close()

	# Generate openlayers.html
	if not noopenlayers:
		f = open(os.path.join(output_dir, 'openlayers.html'), 'w')
		f.write( generate_openlayers(
		  title = title,
		  xsize = xsize,
		  ysize = ysize,
		  maxzoom = maxzoom,
		  tileformat = tileformat
		))
		f.close()
		
	# Generate Root KML
	if generatekml:
		f = open(os.path.join(output_dir, 'doc.kml'), 'w')
		f.write( generate_rootkml(
			title = title,
			north = north,
			south = south,
			east = east,
			west = west,
			tilesize = tilesize,
			tileformat = tileformat,
			publishurl = ""
		))
		f.close()
		
	# Generate Root KML with publishurl
	if generatekml and publishurl:
		f = open(os.path.join(output_dir, os.path.basename(output_dir)+'.kml'), 'w')
		f.write( generate_rootkml(
			title = title,
			north = north,
			south = south,
			east = east,
			west = west,
			tilesize = tilesize,
			tileformat = tileformat,
			publishurl = publishurl
		))
		f.close()

	#
	# Main cycle for TILE and KML generating.
	#
	tileno = 0
	progress = 0
	for zoom in range(maxzoom, -1, -1):

		# Maximal size of read window in pixels.
		rmaxsize = 2.0**(maxzoom-zoom)*tilesize

		if verbose:
			print "-"*80
			print "Zoom %s - pixel %.20f" % (zoom, zoompixels[zoom]), int(2.0**zoom*tilesize)
			print "-"*80

		for iy in range(0, int( ceil( ysize / rmaxsize))):

			# Read window ysize in pixels.
			if iy+1 == int( ceil( ysize / rmaxsize)) and ysize % rmaxsize != 0:
				rysize = int(ysize % rmaxsize)
			else:
				rysize = int(rmaxsize)

			# Read window top coordinate in pixels.
			ry = int(ysize - (iy * rmaxsize)) - rysize
			
			for ix in range(0, int( ceil( xsize / rmaxsize))):

				# Read window xsize in pixels.
				if ix+1 == int( ceil( xsize / rmaxsize)) and xsize % rmaxsize != 0:
					rxsize = int(xsize % rmaxsize)
				else:
					rxsize = int(rmaxsize)
				
				# Read window left coordinate in pixels.
				rx = int(ix * rmaxsize)


				dxsize = int(rxsize/rmaxsize * tilesize)
				dysize = int(rysize/rmaxsize * tilesize)
				filename = os.path.join(output_dir, '%d/%d/%d.%s' % (zoom, ix, iy, tileformat))

				if verbose:
					# Print info about tile and read area.
					print "%d/%d" % (tileno+1,tilecount), filename, [ix, iy], [rx, ry], [rxsize, rysize], [dxsize, dysize]
				else:
					# Show the progress bar.
					percent = int(ceil((tileno) / float(tilecount-1) * 100))
					while progress <= percent:
						if progress % 10 == 0:
							sys.stdout.write( "%d" % progress )
							sys.stdout.flush()
						else:
							sys.stdout.write( '.' )
							sys.stdout.flush()
						progress += 2.5
			   
				# Load raster from read window.
				data = dataset.ReadRaster(rx, ry, rxsize, rysize, dxsize, dysize)
				# Write that raster to the tile.
				writetile( filename, data, dxsize, dysize, bands)
			   
				# Create a KML file for this tile.
				if generatekml:
					f = open( os.path.join(output_dir, '%d/%d/%d.kml' % (zoom, ix, iy)), 'w')
					f.write( generate_kml(
						zoom = zoom,
						ix = ix,
						iy = iy,
						rpixel = zoompixels[zoom],
						tilesize = tilesize,
						tileformat = tileformat,
						south = south,
						west = west,
						xsize = xsize,
						ysize = ysize,
						maxzoom = maxzoom
					))
					f.close()

				tileno += 1

	# Last \n for the progress bar
	print "\nDone"
'''
