# templates.py
# encoding: utf-8

# =============================================================================
def generate_tilemapresource( **args ):
    """
    Template for tilemapresource.xml. Returns filled string. Expected variables:
      title, north, south, east, west, isepsg4326, projection, publishurl,
      zoompixels, tilesize, tileformat, profile
    """

    if args['isepsg4326']:
        args['srs'] = "EPSG:4326"
    else:
        args['srs'] = args['projection']

    zoompixels = args['zoompixels']

    s = """<?xml version="1.0" encoding="utf-8"?>
<TileMap version="1.0.0" tilemapservice="http://tms.osgeo.org/1.0.0">
  <Title>%(title)s</Title>
  <Abstract></Abstract>
  <SRS>%(srs)s</SRS>
  <BoundingBox minx="%(south).20f" miny="%(west).20f" maxx="%(north).20f" maxy="%(east).20f"/>
  <Origin x="%(south).20f" y="%(west).20f"/>
  <TileFormat width="%(tilesize)d" height="%(tilesize)d" mime-type="image/%(tileformat)s" extension="%(tileformat)s"/>
  <TileSets profile="%(profile)s">
""" % args
    for z in range(len(zoompixels)):
        s += """    <TileSet href="%s%d" units-per-pixel="%.20f" order="%d"/>\n""" % (args['publishurl'], z, zoompixels[z], z)
    s += """  </TileSets>
</TileMap>
"""
    return s

# =============================================================================
def generate_rootkml( **args ):
    """
    Template for the root doc.kml. Returns filled string. Expected variables:
      title, north, south, east, west, tilesize, tileformat, publishurl
    """
    
    args['minlodpixels'] = args['tilesize'] / 2
    if not args['childkml']:
        args['childkml'] = "0/0/0.kml"

    s = """<?xml version="1.0" encoding="utf-8"?>
<kml xmlns="http://earth.google.com/kml/2.1">
  <Document>
    <name>%(title)s</name>
    <description></description>
    <Style>
      <ListStyle id="hideChildren">
        <listItemType>checkHideChildren</listItemType>
      </ListStyle>
    </Style>
    <Region>
      <LatLonAltBox>
        <north>%(north).20f</north>
        <south>%(south).20f</south>
        <east>%(east).20f</east>
        <west>%(west).20f</west>
      </LatLonAltBox>
    </Region>
    <NetworkLink>
      <open>1</open>
      <Region>
        <Lod>
          <minLodPixels>%(minlodpixels)d</minLodPixels>
          <maxLodPixels>-1</maxLodPixels>
        </Lod>
        <LatLonAltBox>
          <north>%(north).20f</north>
          <south>%(south).20f</south>
          <east>%(east).20f</east>
          <west>%(west).20f</west>
        </LatLonAltBox>
      </Region>
      <Link>
        <href>%(publishurl)s%(childkml)s</href>
        <viewRefreshMode>onRegion</viewRefreshMode>
      </Link>
    </NetworkLink>
  </Document>
</kml>
""" % args
    return s

# =============================================================================
def generate_kml( **args ):
    """
    Template for the tile kml. Returns filled string. Expected variables:
      zoom, ix, iy, rpixel, tilesize, tileformat, south, west, xsize, ysize,
      maxzoom
    """

    zoom, ix, iy, rpixel = args['zoom'], args['ix'], args['iy'], args['rpixel']
    maxzoom, tilesize = args['maxzoom'], args['tilesize']
    south, west = args['south'], args['west']
    xsize, ysize = args['xsize'], args['ysize']

    nsew = lambda ix, iy, rpixel: (south + rpixel*((iy+1)*tilesize),
                                    south + rpixel*(iy*tilesize),
                                    west + rpixel*((ix+1)*tilesize),
                                    west + rpixel*ix*tilesize)

    args['minlodpixels'] = args['tilesize'] / 2
    args['tnorth'], args['tsouth'], args['teast'], args['twest'] = nsew(ix, iy, rpixel)

    if verbose:
        print "\tKML for area NSEW: %.20f %.20f %.20f %.20f" % nsew(ix, iy, rpixel)

    xchildern = []
    ychildern = []
    if zoom < maxzoom:
        zareasize = 2.0**(maxzoom-zoom-1)*tilesize
        xchildern.append(ix*2)
        if ix*2+1 < int( ceil( xsize / zareasize)):
            xchildern.append(ix*2+1)
        ychildern.append(iy*2)
        if iy*2+1 < int( ceil( ysize / zareasize)):
            ychildern.append(iy*2+1)

    s = """<?xml version="1.0" encoding="utf-8"?>
<kml xmlns="http://earth.google.com/kml/2.1">
  <Document>
    <name>%(zoom)d/%(ix)d/%(iy)d.kml</name>
    <Region>
      <Lod>
        <minLodPixels>%(minlodpixels)d</minLodPixels>
        <maxLodPixels>-1</maxLodPixels>
      </Lod>
      <LatLonAltBox>
        <north>%(tnorth).20f</north>
        <south>%(tsouth).20f</south>
        <east>%(teast).20f</east>
        <west>%(twest).20f</west>
      </LatLonAltBox>
    </Region>
    <GroundOverlay>
      <drawOrder>%(zoom)d</drawOrder>
      <Icon>
        <href>%(iy)d.%(tileformat)s</href>
      </Icon>
      <LatLonBox>
        <north>%(tnorth).20f</north>
        <south>%(tsouth).20f</south>
        <east>%(teast).20f</east>
        <west>%(twest).20f</west>
      </LatLonBox>
    </GroundOverlay>
""" % args

    for cx in xchildern:
        for cy in ychildern:

            if verbose:
                print "\t  ", [cx, cy], "NSEW: %.20f %.20f %.20f %.20f" % nsew(cx, cy, rpixel/2)

            cnorth, csouth, ceast, cwest = nsew(cx, cy, rpixel/2)
            s += """    <NetworkLink>
      <name>%d/%d/%d.%s</name>
      <Region>
        <Lod>
          <minLodPixels>%d</minLodPixels>
          <maxLodPixels>-1</maxLodPixels>
        </Lod>
        <LatLonAltBox>
          <north>%.20f</north>
          <south>%.20f</south>
          <east>%.20f</east>
          <west>%.20f</west>
        </LatLonAltBox>
      </Region>
      <Link>
        <href>../../%d/%d/%d.kml</href>
        <viewRefreshMode>onRegion</viewRefreshMode>
        <viewFormat/>
      </Link>
    </NetworkLink>
""" % (zoom+1, cx, cy, tileformat, args['minlodpixels'], cnorth, csouth, ceast, cwest, zoom+1, cx, cy)

    s += """  </Document>
</kml>
"""
    return s


# =============================================================================
def generate_kml_new( **args ):
    """
    Template for the tile kml. Returns filled string. Expected data structure:
      swne, tx, ty, tz, tilesize, tileformat, childern = [{swne},{..},{},{}]
    """
    tx, ty, tz = args['tx'], args['ty'], args['tz']
    childern = args['childern']
    args['south'], args['west'], args['north'], args['east'] = args['swne']
    args['minlodpixels'] = args['tilesize'] / 2

    # TODO: How to correctly implement fading in KML SuperOverlay?

    if not args.has_key('maxlodpixels'):
        if childern:
            args['maxlodpixels'] = -1 # args['tilesize'] * 5
        else:
            args['maxlodpixels'] = -1

    if not args.has_key('maxfadeextent'):
        if childern:
            args['maxfadeextent'] = 0 # args['tilesize'] / 4
        else:
            args['maxfadeextent'] = 0

    if not args.has_key('minfadeextent'):
        args['minfadeextent'] = args['maxfadeextent']


    s = """<?xml version="1.0" encoding="utf-8"?>
<kml xmlns="http://earth.google.com/kml/2.1">
  <Document>
    <name>%(tz)d/%(tx)d/%(ty)d.kml</name>
    <Region>
      <Lod>
        <minLodPixels>%(minlodpixels)d</minLodPixels>
        <minFadeExtent>%(minfadeextent)d</minFadeExtent>
        <maxLodPixels>%(maxlodpixels)d</maxLodPixels>
        <maxFadeExtent>%(maxfadeextent)d</maxFadeExtent>
      </Lod>
      <LatLonAltBox>
        <north>%(north).20f</north>
        <south>%(south).20f</south>
        <east>%(east).20f</east>
        <west>%(west).20f</west>
      </LatLonAltBox>
    </Region>
    <GroundOverlay>
      <drawOrder>%(tz)d</drawOrder>
      <Icon>
        <href>%(ty)d.%(tileformat)s</href>
      </Icon>
      <LatLonBox>
        <north>%(north).20f</north>
        <south>%(south).20f</south>
        <east>%(east).20f</east>
        <west>%(west).20f</west>
      </LatLonBox>
    </GroundOverlay>
""" % args

    if childern:
        i = -1
        for cy in range(2*ty,2*ty+2):
            for cx in range(2*tx,2*tx+2):
                i += 1
                if childern[i] != {}:

                    csouth, cwest, cnorth, ceast = childern[i]['swne']

                    s += """    <NetworkLink>
      <name>%d/%d/%d.%s</name>
      <Region>
        <Lod>
          <minLodPixels>%d</minLodPixels>
          <maxLodPixels>%d</maxLodPixels>
        </Lod>
        <LatLonAltBox>
          <north>%.20f</north>
          <south>%.20f</south>
          <east>%.20f</east>
          <west>%.20f</west>
        </LatLonAltBox>
      </Region>
      <Link>
        <href>../../%d/%d/%d.kml</href>
        <viewRefreshMode>onRegion</viewRefreshMode>
        <viewFormat/>
      </Link>
    </NetworkLink>
""" % (tz+1, cx, cy, args['tileformat'], args['minlodpixels'], args['maxlodpixels'], cnorth, csouth, ceast, cwest, tz+1, cx, cy)

    s += """  </Document>
</kml>
"""
    return s

# =============================================================================
def generate_googlemaps_overlay( **args ):
    """
    Template for googlemaps.html implementing Overlay of tiles.
    It returns filled string. Expected variables:
      title, googlemapskey, north, south, east, west, minzoom, maxzoom, tilesize, tileformat, publishurl
    """

    s = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
	<html xmlns="http://www.w3.org/1999/xhtml" xmlns:v="urn:schemas-microsoft-com:vml"> 
	  <head>
	    <title>%(title)s</title>
	    <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
	    <meta http-equiv='imagetoolbar' content='no'/>
	    <style type="text/css"> v\:* {behavior:url(#default#VML);}
	        html, body { overflow: hidden; padding: 0; height: 100%%; width: 100%%; font-family: 'Lucida Grande',Geneva,Arial,Verdana,sans-serif; }
	        body { margin: 10px; background: #fff; }
	        h1 { margin: 0; padding: 6px; border:0; font-size: 20pt; }
	        #header { height: 43px; padding: 0; background-color: #eee; border: 1px solid #888; }
	        #subheader { height: 12px; text-align: right; font-size: 10px; color: #555;}
	        #map { height: 95%%; border: 1px solid #888; }
	    </style>
	    <script src='http://maps.google.com/maps?file=api&amp;v=2&amp;key=%(googlemapskey)s' type='text/javascript'></script>
	    <script type="text/javascript">
	    //<![CDATA[

	    /*
	     * Constants for given map
	     * TODO: read it from tilemapresource.xml
	     */

	    var mapBounds = new GLatLngBounds(new GLatLng(%(south)s, %(west)s), new GLatLng(%(north)s, %(east)s));
	    var mapMinZoom = %(minzoom)s;
	    var mapMaxZoom = %(maxzoom)s;

	    var opacity = 0.75;
	    var map;
	    var hybridOverlay;

	    /*
	     * Create a Custom Transparency GControl
	     * (based on XSlider of Mike Williams, bitmaps comes from GeoGarrage viewer)
	     */

	    var CTransparencyLENGTH = 58; 
	    // maximum width that the knob can move (slide width minus knob width)

	    function CTransparencyControl( overlay ) {
	        this.overlay = overlay;
	        this.opacity = overlay.getTileLayer().getOpacity();
	    }
	    CTransparencyControl.prototype = new GControl();

	    // This function positions the slider to match the specified opacity
	    CTransparencyControl.prototype.setSlider = function(pos) {
	        var left = Math.round((CTransparencyLENGTH*pos));
	        this.slide.left = left;
	        this.knob.style.left = left+"px";
	    }

	    // This function reads the slider and sets the overlay opacity level
	    CTransparencyControl.prototype.setOpacity = function() {
		    // set the global variable
	        opacity = this.slide.left/CTransparencyLENGTH;
	        this.map.clearOverlays();
	        this.map.addOverlay(this.overlay, { zPriority: 0 });
	        if (this.map.getCurrentMapType() == G_HYBRID_MAP) {
	            this.map.addOverlay(hybridOverlay);
	        }
	    }

	    // This gets called by the API when addControl(new CTransparencyControl())
	    CTransparencyControl.prototype.initialize = function(map) {
	        var that=this;
	        this.map = map;

	        // Is this MSIE, if so we need to use AlphaImageLoader
	        var agent = navigator.userAgent.toLowerCase();
	        if ((agent.indexOf("msie") > -1) && (agent.indexOf("opera") < 1)){this.ie = true} else {this.ie = false}

	        // create the background graphic as a <div> containing an image
	        var container = document.createElement("div");
	        container.style.width="70px";
	        container.style.height="21px";

	        // Handle transparent PNG files in MSIE
	        if (this.ie) {
	          var loader = "filter:progid:DXImageTransform.Microsoft.AlphaImageLoader(src='http://www.maptiler.org/img/opacity-slider.png', sizingMethod='crop');";
	          container.innerHTML = '<div style="height:21px; width:70px; ' +loader+ '" ></div>';
	        } else {
	          container.innerHTML = '<div style="height:21px; width:70px; background-image: url(http://www.maptiler.org/img/opacity-slider.png)" ></div>';
	        }

	        // create the knob as a GDraggableObject
	        // Handle transparent PNG files in MSIE
	        if (this.ie) {
	          var loader = "progid:DXImageTransform.Microsoft.AlphaImageLoader(src='http://www.maptiler.org/img/opacity-slider.png', sizingMethod='crop');";
	          this.knob = document.createElement("div"); 
	          this.knob.style.height="21px";
	          this.knob.style.width="13px";
		  this.knob.style.overflow="hidden";
	          this.knob_img = document.createElement("div"); 
	          this.knob_img.style.height="21px";
	          this.knob_img.style.width="83px";
	          this.knob_img.style.filter=loader;
		  this.knob_img.style.position="relative";
		  this.knob_img.style.left="-70px";
	          this.knob.appendChild(this.knob_img);
	        } else {
	          this.knob = document.createElement("div"); 
	          this.knob.style.height="21px";
	          this.knob.style.width="13px";
	          this.knob.style.backgroundImage="url(http://www.maptiler.org/img/opacity-slider.png)";
	          this.knob.style.backgroundPosition="-70px 0px";
	        }
	        container.appendChild(this.knob);
	        this.slide=new GDraggableObject(this.knob, {container:container});
	        this.slide.setDraggableCursor('pointer');
	        this.slide.setDraggingCursor('pointer');
	        this.container = container;

	        // attach the control to the map
	        map.getContainer().appendChild(container);

	        // init slider
	        this.setSlider(this.opacity);

	        // Listen for the slider being moved and set the opacity
	        GEvent.addListener(this.slide, "dragend", function() {that.setOpacity()});
	        //GEvent.addListener(this.container, "click", function( x, y ) { alert(x, y) });

	        return container;
	      }

	      // Set the default position for the control
	      CTransparencyControl.prototype.getDefaultPosition = function() {
	        return new GControlPosition(G_ANCHOR_TOP_RIGHT, new GSize(7, 47));
	      }

	    /*
	     * Full-screen Window Resize
	     */

	    function getWindowHeight() {
	        if (self.innerHeight) return self.innerHeight;
	        if (document.documentElement && document.documentElement.clientHeight)
	            return document.documentElement.clientHeight;
	        if (document.body) return document.body.clientHeight;
	        return 0;
	    }

	    function getWindowWidth() {
	        if (self.innerWidth) return self.innerWidth;
	        if (document.documentElement && document.documentElement.clientWidth)
	            return document.documentElement.clientWidth;
	        if (document.body) return document.body.clientWidth;
	        return 0;
	    }

	    function resize() {  
	        var map = document.getElementById("map");  
	        var header = document.getElementById("header");  
	        var subheader = document.getElementById("subheader");  
	        map.style.height = (getWindowHeight()-80) + "px";
	        map.style.width = (getWindowWidth()-20) + "px";
	        header.style.width = (getWindowWidth()-20) + "px";
	        subheader.style.width = (getWindowWidth()-20) + "px";
	        // map.checkResize();
	    } 


	    /*
	     * Main load function:
	     */

	    function load() {

	       if (GBrowserIsCompatible()) {

	          // Bug in the Google Maps: Copyright for Overlay is not correctly displayed
	          var gcr = GMapType.prototype.getCopyrights;
	          GMapType.prototype.getCopyrights = function(bounds,zoom) {
	              return ['Overlay © USGS '].concat(gcr.call(this,bounds,zoom));
	          }

	          map = new GMap2( document.getElementById("map"), { backgroundColor: '#fff' } );

	          map.addMapType(G_PHYSICAL_MAP);
	          map.setMapType(G_PHYSICAL_MAP);

	          //map.setCenter(new GLatLng(0, 0));
	          map.setCenter( mapBounds.getCenter(), map.getBoundsZoomLevel( mapBounds ));

	          hybridOverlay = new GTileLayerOverlay( G_HYBRID_MAP.getTileLayers()[1] );
	          GEvent.addListener(map, "maptypechanged", function() {
	            if (map.getCurrentMapType() == G_HYBRID_MAP) {
	                map.addOverlay(hybridOverlay);
	            } else {
	               map.removeOverlay(hybridOverlay);
	            }
	          } );

	          var tilelayer = new GTileLayer(GCopyrightCollection(''), mapMinZoom, mapMaxZoom);
	          var mercator = new GMercatorProjection(mapMaxZoom+1);
	          tilelayer.getTileUrl = function(tile,zoom) {
	              if ((zoom < mapMinZoom) || (zoom > mapMaxZoom)) {
	                  return "none.png";
	              } 
	              var ymax = 1 << zoom;
	              var y = ymax - tile.y -1;
	              var tileBounds = new GLatLngBounds(
	                  mercator.fromPixelToLatLng( new GPoint( (tile.x)*256, (tile.y+1)*256 ) , zoom ),
	                  mercator.fromPixelToLatLng( new GPoint( (tile.x+1)*256, (tile.y)*256 ) , zoom )
	              );
	              if (mapBounds.intersects(tileBounds)) {
	                  return zoom+"/"+tile.x+"/"+y+".png";
	              } else {
	                  return "none.png";
	              }
	          }
	          // IE 7-: support for PNG alpha channel
	          // Unfortunately, the opacity for whole overlay is then not changeable, either or...
	          tilelayer.isPng = function() { return true;};
	          tilelayer.getOpacity = function() { return opacity; }

	          overlay = new GTileLayerOverlay( tilelayer );
	          map.addOverlay(overlay);

	          map.addControl(new GLargeMapControl());
	          map.addControl(new GHierarchicalMapTypeControl());
	          map.addControl(new CTransparencyControl( overlay ));

	          map.addMapType(G_SATELLITE_3D_MAP);
	          map.getEarthInstance(getEarthInstanceCB);

	          map.enableContinuousZoom();
	          map.enableScrollWheelZoom();

	          map.setMapType(G_HYBRID_MAP);
	       }
	       resize();
	    }

	    function getEarthInstanceCB(object) {
	       var ge = object;

	       if (ge) {
	           var link = ge.createLink("");
	           link.setHref("%(publishurl)s/doc.kml");
	           var networkLink = ge.createNetworkLink("");
	           networkLink.setName("TMS Map Overlay");
	           networkLink.setFlyToView(true);  
	           networkLink.setLink(link);
	           ge.getFeatures().appendChild(networkLink);
	       } else {
	           // alert("You should open a KML in Google Earth");
	           // add div with the link to generated KML... - maybe JavaScript redirect to the URL of KML?
	       }
	    }

	    onresize=function(){ resize(); };

	    //]]>
	    </script>
	  </head>
	  <body onload="load()">
	      <div id="header"><h1>%(title)s</h1></div>
	      <div id="subheader">Generated by <a href="http://www.maptiler.org/">MapTiler</a>/<a href="http://www.klokan.cz/projects/gdal2tiles/">GDAL2Tiles</a>, Copyright © 2008 <a href="http://www.klokan.cz/">Klokan Petr Pridal</a>,  <a href="http://www.gdal.org/">GDAL</a> &amp; <a href="http://www.osgeo.org/">OSGeo</a> <a href="http://code.google.com/soc/">GSoC</a>
	<!-- PLEASE, LET THIS NOTE ABOUT AUTHOR AND PROJECT SOMEWHERE ON YOUR WEBSITE, OR AT LEAST IN THE COMMENT IN HTML. THANK YOU -->
	      </div>
	       <div id="map"></div>
	  </body>
	</html>
""" % args

    return s


# =============================================================================
def generate_openlayers_overlay( **args ):
    """
    Template for openlayers.html implementing overlay of available Spherical Mercator layers.

    TODO: Replace /dev/ version of OpenLayers with new stable version and link should contain the version number (../api/2.7/OpenLayers.js).

    It returns filled string. Expected variables:
      title, googlemapskey, yahooappid, north, south, east, west, minzoom, maxzoom, tilesize, tileformat, publishurl
    """
    s = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
	<html xmlns="http://www.w3.org/1999/xhtml"
	  <head>
	    <title>%(title)s</title>
	    <style type="text/css">
	        html, body { width: 100%%; height: 100%%; border: 0; padding:0; margin:0; }
	        #map { width: 100%%; height: 100%%; }
	    </style>

	    <script src='http://dev.virtualearth.net/mapcontrol/mapcontrol.ashx?v=6.1'></script>
	    <script src='http://maps.google.com/maps?file=api&amp;v=2&amp;key=%(googlemapskey)s' type='text/javascript'></script>
	    <script src="http://api.maps.yahoo.com/ajaxymap?v=3.0&amp;appid=%(yahooappid)s"></script>
	    <script src="http://www.openlayers.org/dev/OpenLayers.js" type="text/javascript"></script>
	    <script type="text/javascript">

	        // make map available for easy debugging
	        var map;

		    var mapBounds = new OpenLayers.Bounds( %(west)s, %(south)s, %(east)s, %(north)s);
		    var mapMinZoom = %(minzoom)s;
		    var mapMaxZoom = %(maxzoom)s;

	        // avoid pink tiles
	        OpenLayers.IMAGE_RELOAD_ATTEMPTS = 3;
	        OpenLayers.Util.onImageLoadErrorColor = "transparent";

	        function init(){
	            var options = {
	                controls: [],
	                projection: new OpenLayers.Projection("EPSG:900913"),
	                displayProjection: new OpenLayers.Projection("EPSG:4326"),
	                units: "m",
	                maxResolution: 156543.0339,
	                maxExtent: new OpenLayers.Bounds(-20037508, -20037508,
	                                                 20037508, 20037508.34)
	            };
	            map = new OpenLayers.Map('map', options);

	            // create Google Mercator layers
	            var gmap = new OpenLayers.Layer.Google(
	                "Google Streets",
	                {'sphericalMercator': true}
	            );
	            var gsat = new OpenLayers.Layer.Google(
	                "Google Satellite",
	                {type: G_SATELLITE_MAP, 'sphericalMercator': true, numZoomLevels: 22}
	            );
	            var ghyb = new OpenLayers.Layer.Google(
	                "Google Hybrid",
	                {type: G_HYBRID_MAP, 'sphericalMercator': true}
	            );
	            var gter = new OpenLayers.Layer.Google(
	                "Google Terrain",
	                {type: G_PHYSICAL_MAP, 'sphericalMercator': true}
	            );

	            // create Virtual Earth layers
	            var veroad = new OpenLayers.Layer.VirtualEarth(
	                "Virtual Earth Roads",
	                {'type': VEMapStyle.Road, 'sphericalMercator': true}
	            );
	            var veaer = new OpenLayers.Layer.VirtualEarth(
	                "Virtual Earth Aerial",
	                {'type': VEMapStyle.Aerial, 'sphericalMercator': true}
	            );
	            var vehyb = new OpenLayers.Layer.VirtualEarth(
	                "Virtual Earth Hybrid",
	                {'type': VEMapStyle.Hybrid, 'sphericalMercator': true}
	            );

	            // create Yahoo layer
	            var yahoo = new OpenLayers.Layer.Yahoo(
	                "Yahoo Street",
	                {'sphericalMercator': true}
	            );
	            var yahoosat = new OpenLayers.Layer.Yahoo(
	                "Yahoo Satellite",
	                {'type': YAHOO_MAP_SAT, 'sphericalMercator': true}
	            );
	            var yahoohyb = new OpenLayers.Layer.Yahoo(
	                "Yahoo Hybrid",
	                {'type': YAHOO_MAP_HYB, 'sphericalMercator': true}
	            );

	            // create OSM layer
	            var osm = new OpenLayers.Layer.TMS(
	                "OpenStreetMap",
	                "http://tile.openstreetmap.org/",
	                {
	                    type: 'png', getURL: osm_getTileURL,
	                    displayOutsideMaxExtent: true,
	                    attribution: '<a href="http://www.openstreetmap.org/">OpenStreetMap</a>'
	                }
	            );
	            // create OSM layer
	            var oam = new OpenLayers.Layer.TMS(
	                "OpenAerialMap",
	                "http://tile.openaerialmap.org/tiles/1.0.0/openaerialmap-900913/",
	                {
	                    type: 'png', getURL: osm_getTileURL
	                }
	            );


	            // create TMS Overlay layer
	            var tmsoverlay = new OpenLayers.Layer.TMS(
	                "TMS Overlay",
	                "",
	                {
	                    // url: '', serviceVersion: '.', layername: '.',
						type: 'png', getURL: overlay_getTileURL,
						isBaseLayer: false, opacity: 0.7
	                }
	            );

		        function overlay_getTileURL(bounds) {
		            var res = this.map.getResolution();
		            var x = Math.round((bounds.left - this.maxExtent.left) / (res * this.tileSize.w));
		            var y = Math.round((bounds.bottom - this.tileOrigin.lat) / (res * this.tileSize.h));
		            var z = this.map.getZoom();
		            if (this.map.baseLayer.name == 'Virtual Earth Roads' || this.map.baseLayer.name == 'Virtual Earth Aerial' || this.map.baseLayer.name == 'Virtual Earth Hybrid') {
		               z = z + 1;
		            }
			    if (mapBounds.intersectsBounds( bounds )) {
		               //console.log( this.url + z + "/" + x + "/" + y + "." + this.type);
		               return this.url + z + "/" + x + "/" + y + "." + this.type;
	                    } else {
	                       return "none.png";
	                    }
		        }

	            map.addLayers([gmap, gsat, ghyb, gter, veroad, veaer, vehyb,
	                           yahoo, yahoosat, yahoohyb, osm, oam,
	                           tmsoverlay]);

	            var switcherControl = new OpenLayers.Control.LayerSwitcher();
	            map.addControl(switcherControl);
	            switcherControl.maximizeControl();

	            map.addControl(new OpenLayers.Control.PanZoomBar());
	            map.addControl(new OpenLayers.Control.Permalink());
	            map.addControl(new OpenLayers.Control.MousePosition());
	            map.addControl(new OpenLayers.Control.MouseDefaults());
	            map.addControl(new OpenLayers.Control.KeyboardDefaults());

	            map.zoomToExtent( mapBounds.transform(map.displayProjection, map.projection ) );
	        }

	        function osm_getTileURL(bounds) {
	            var res = this.map.getResolution();
	            var x = Math.round((bounds.left - this.maxExtent.left) / (res * this.tileSize.w));
	            var y = Math.round((this.maxExtent.top - bounds.top) / (res * this.tileSize.h));
	            var z = this.map.getZoom();
	            var limit = Math.pow(2, z);

	            if (y < 0 || y >= limit) {
	                return OpenLayers.Util.getImagesLocation() + "404.png";
	            } else {
	                x = ((x %% limit) + limit) %% limit;
	                return this.url + z + "/" + x + "/" + y + "." + this.type;
	            }
	        }

	        onresize=function(){ map.updateSize(); };

	    </script>
	  </head>
	  <body onload="init()" onresize="map.updateSize()">
	    <div id="map"></div>
	  </body>
	</html>
""" % args

    return s

# =============================================================================
def generate_googlemaps( **args ):
    """
    Template for googlemaps.html. Returns filled string. Expected variables:
      title, googlemapskey, xsize, ysize, maxzoom, tilesize, 
    """

    args['zoom'] = min( 3, args['maxzoom'])
    args['tileformat'] = tileformat

    s = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:v="urn:schemas-microsoft-com:vml"> 
  <head>
    <title>%(title)s</title>
    <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
    <meta http-equiv='imagetoolbar' content='no'/>
    <style type="text/css"> v\:* {behavior:url(#default#VML);}
        html, body { overflow: hidden; padding: 0; height: 100%%; width: 100%%; font-family: 'Lucida Grande',Geneva,Arial,Verdana,sans-serif; }
        body { margin: 10px; background: #fff; }
        h1 { margin: 0; padding: 6px; border:0; font-size: 20pt; }
        #header { height: 43px; padding: 0; background-color: #eee; border: 1px solid #888; }
        #subheader { height: 12px; text-align: right; font-size: 10px; color: #555;}
        #map { height: 95%%; border: 1px solid #888; }
    </style>
    <script src='http://maps.google.com/maps?file=api&amp;v=2.x&amp;key=%(googlemapskey)s' type='text/javascript'></script>
    <script type="text/javascript">
    //<![CDATA[

    function getWindowHeight() {
	    if (self.innerHeight) return self.innerHeight;
	    if (document.documentElement && document.documentElement.clientHeight)
	        return document.documentElement.clientHeight;
	    if (document.body) return document.body.clientHeight;
	    return 0;
    }

    function getWindowWidth() {
	    if (self.innerWidth) return self.innerWidth;
    	if (document.documentElement && document.documentElement.clientWidth)
	        return document.documentElement.clientWidth;
	    if (document.body) return document.body.clientWidth;
	    return 0;
    }
    
    function resize() {  
	    var map = document.getElementById("map");  
	    var header = document.getElementById("header");  
	    var subheader = document.getElementById("subheader");  
	    map.style.height = (getWindowHeight()-80) + "px";
	    map.style.width = (getWindowWidth()-20) + "px";
	    header.style.width = (getWindowWidth()-20) + "px";
	    subheader.style.width = (getWindowWidth()-20) + "px";
	    // map.checkResize();
    } 


	// See http://www.google.com/apis/maps/documentation/reference.html#GProjection 
	// This code comes from FlatProjection.js, done by Smurf in project gwmap
	/**
	 * Creates a custom GProjection for flat maps.
	 *
	 * @classDescription	Creates a custom GProjection for flat maps.
	 * @param {Number} width The width in pixels of the map at the specified zoom level.
	 * @param {Number} height The height in pixels of the map at the specified zoom level.
	 * @param {Number} pixelsPerLon The number of pixels per degree of longitude at the specified zoom level.
	 * @param {Number} zoom The zoom level width, height, and pixelsPerLon are set for.
	 * @param {Number} maxZoom The maximum zoom level the map will go.
	 * @constructor	
	 */
	function FlatProjection(width,height,pixelsPerLon,zoom,maxZoom)
	{
		this.pixelsPerLonDegree = new Array(maxZoom);
		this.tileBounds = new Array(maxZoom);

		width /= Math.pow(2,zoom);
		height /= Math.pow(2,zoom);
		pixelsPerLon /= Math.pow(2,zoom);
		
		for(var i=maxZoom; i>=0; i--)
		{
			this.pixelsPerLonDegree[i] = pixelsPerLon*Math.pow(2,i);
			this.tileBounds[i] = new GPoint(Math.ceil(width*Math.pow(2,i)/256), Math.ceil(height*Math.pow(2,i)/256));
		}
	}

	FlatProjection.prototype = new GProjection();

	FlatProjection.prototype.fromLatLngToPixel = function(point,zoom)
	{
		var x = Math.round(point.lng() * this.pixelsPerLonDegree[zoom]);
		var y = Math.round(point.lat() * this.pixelsPerLonDegree[zoom]);
		return new GPoint(x,y);
	}

	FlatProjection.prototype.fromPixelToLatLng = function(pixel,zoom,unbounded)	
	{
		var lng = pixel.x/this.pixelsPerLonDegree[zoom];
		var lat = pixel.y/this.pixelsPerLonDegree[zoom];
		return new GLatLng(lat,lng,true);
	}

	FlatProjection.prototype.tileCheckRange = function(tile, zoom, tilesize)
	{
		if( tile.y<0 || tile.x<0 || tile.y>=this.tileBounds[zoom].y || tile.x>=this.tileBounds[zoom].x )
		{
			return false;
		}
		return true;
	}
	FlatProjection.prototype.getWrapWidth = function(zoom)
	{
		return Number.MAX_VALUE;
	}

    /*
     * Main load function:
     */

    function load() {
        var MapWidth = %(xsize)d;
        var MapHeight = %(ysize)d;
        var MapMaxZoom = %(maxzoom)d;
        var MapPxPerLon = %(tilesize)d;

        if (GBrowserIsCompatible()) {
            var map = new GMap2( document.getElementById("map") );
            var tileLayer = [ new GTileLayer( new GCopyrightCollection(null), 0, MapMaxZoom ) ];
            tileLayer[0].getTileUrl = function(a,b) {
                var y = Math.floor(MapHeight / (MapPxPerLon * Math.pow(2, (MapMaxZoom-b)))) - a.y;
                // Google Maps indexed tiles from top left, we from bottom left, it causes problems during zooming, solution?
                return b+"/"+a.x+"/"+y+".%(tileformat)s";
            }
            var mapType = new GMapType(
                tileLayer,
                new FlatProjection( MapWidth, MapHeight, MapPxPerLon, MapMaxZoom-2, MapMaxZoom),
                'Default',
                { maxResolution: MapMaxZoom, minResolution: 0, tileSize: MapPxPerLon }
            );
            map.addMapType(mapType);

            map.removeMapType(G_NORMAL_MAP);
            map.removeMapType(G_SATELLITE_MAP);
            map.removeMapType(G_HYBRID_MAP);

            map.setCenter(new GLatLng((MapHeight/MapPxPerLon/4)/2, (MapWidth/MapPxPerLon/4)/2), %(zoom)d, mapType);

            //alert((MapHeight/MapPxPerLon/4)/2 + " x " + (MapWidth/MapPxPerLon/4)/2);
            //map.addOverlay(new GMarker( new GLatLng((MapHeight/MapPxPerLon/4)/2,(MapWidth/MapPxPerLon/4)/2) ));

            map.getContainer().style.backgroundColor='#fff';

            map.addControl(new GLargeMapControl());
            // Overview Map Control is not running correctly...
            // map.addControl(new GOverviewMapControl());
        }
        resize();
    }

    onresize=function(){ resize(); };

    //]]>
    </script>
  </head>
  <body onload="load()" onunload="GUnload()">
      <div id="header"><h1>%(title)s</h1></div>
      <div id="subheader">Generated by <a href="http://www.maptiler.org/">MapTiler</a>/<a href="http://www.klokan.cz/projects/gdal2tiles/">GDAL2Tiles</a>, Copyright © 2008 <a href="http://www.klokan.cz/">Klokan Petr Pridal</a>,  <a href="http://www.gdal.org/">GDAL</a> &amp; <a href="http://www.osgeo.org/">OSGeo</a> <a href="http://code.google.com/soc/">SoC 2007</a>
      </div>
       <div id="map"></div>
  </body>
</html>
""" % args

    return s

# =============================================================================
def generate_openlayers( **args ):
    """
    Template for openlayers.html. Returns filled string. Expected variables:
        title, xsize, ysize, maxzoom, tileformat
    """

    args['maxresolution'] = 2**(args['maxzoom'])
    args['zoom'] = min( 3, args['maxzoom'])
    args['maxzoom'] += 1

    s = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:v="urn:schemas-microsoft-com:vml"> 
  <head>
    <title>%(title)s</title>
    <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
    <meta http-equiv='imagetoolbar' content='no'/>
    <style type="text/css"> v\:* {behavior:url(#default#VML);}
        html, body { overflow: hidden; padding: 0; height: 100%%; width: 100%%; font-family: 'Lucida Grande',Geneva,Arial,Verdana,sans-serif; }
        body { margin: 10px; background: #fff; }
        h1 { margin: 0; padding: 6px; border:0; font-size: 20pt; }
        #header { height: 43px; padding: 0; background-color: #eee; border: 1px solid #888; }
        #subheader { height: 12px; text-align: right; font-size: 10px; color: #555;}
        #map { height: 100%%; border: 1px solid #888; background-color: #fff; }
    </style>
    <script src="http://www.openlayers.org/api/2.5/OpenLayers.js" type="text/javascript"></script>
    <script type="text/javascript">
    //<![CDATA[
    var map, layer;

    function load(){
        // I realized sometimes OpenLayers has some troubles with Opera and Safari.. :-( Advices or patch are welcome...
        // Correct TMS Driver should read TileMapResource by OpenLayers.loadURL and parseXMLStringOpenLayers.parseXMLString
        // For correct projection the patch for OpenLayers TMS is needed, index to tiles (both x an y) has to be rounded into integers
        /* Then definition is like this:
        var options = {
            controls: [],
            // maxExtent: new OpenLayers.Bounds(13.7169981668, 49.608691789, 13.9325582389, 49.7489724456),
            maxExtent: new OpenLayers.Bounds(13.71699816677651995178, 49.46841113248020604942, 13.93255823887490052471, 49.60869178902321863234),
            maxResolution: 0.00150183372679037197,
            numZoomLevels: 6,
            units: 'degrees',
            projection: "EPSG:4326"
        };
        map = new OpenLayers.Map("map", options);
        */

        /* Just pixel based view now */
        var options = {
            controls: [],
            maxExtent: new OpenLayers.Bounds(0, 0, %(xsize)d, %(ysize)d),
            maxResolution: %(maxresolution)d,
            numZoomLevels: %(maxzoom)d,
            units: 'pixels',
            projection: ""
        };
        map = new OpenLayers.Map("map", options);

        // map.addControl(new OpenLayers.Control.MousePosition());
        map.addControl(new OpenLayers.Control.PanZoomBar());
        map.addControl(new OpenLayers.Control.MouseDefaults());
        map.addControl(new OpenLayers.Control.KeyboardDefaults());

        // OpenLayers 2.5 TMS driver
        layer = new OpenLayers.Layer.TMS( "TMS", 
                "", {layername: '.', serviceVersion: '.', type:'%(tileformat)s'} );
        map.addLayer(layer);
        map.zoomTo(%(zoom)d);

        resize();
    }

    function getWindowHeight() {
        if (self.innerHeight) return self.innerHeight;
        if (document.documentElement && document.documentElement.clientHeight)
            return document.documentElement.clientHeight;
        if (document.body) return document.body.clientHeight;
	        return 0;
    }

    function getWindowWidth() {
	    if (self.innerWidth) return self.innerWidth;
	    if (document.documentElement && document.documentElement.clientWidth)
	        return document.documentElement.clientWidth;
	    if (document.body) return document.body.clientWidth;
	        return 0;
    }
    
    function resize() {  
	    var map = document.getElementById("map");  
	    var header = document.getElementById("header");  
	    var subheader = document.getElementById("subheader");  
	    map.style.height = (getWindowHeight()-80) + "px";
	    map.style.width = (getWindowWidth()-20) + "px";
	    header.style.width = (getWindowWidth()-20) + "px";
	    subheader.style.width = (getWindowWidth()-20) + "px";
    } 

    onresize=function(){ resize(); };

    //]]>
    </script>
  </head>
  <body onload="load()">
      <div id="header"><h1>%(title)s</h1></div>
      <div id="subheader">Generated by <a href="http://www.maptiler.org/">MapTiler</a>/<a href="http://www.klokan.cz/projects/gdal2tiles/">GDAL2Tiles</a>, Copyright © 2008 <a href="http://www.klokan.cz/">Klokan Petr Pridal</a>,  <a href="http://www.gdal.org/">GDAL</a> &amp; <a href="http://www.osgeo.org/">OSGeo</a> <a href="http://code.google.com/soc/">SoC 2007</a>
      </div>
       <div id="map"></div>
  </body>
</html>
""" % args

    return s
