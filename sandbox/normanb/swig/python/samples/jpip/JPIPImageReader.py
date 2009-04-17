#
# ITT Visual Information Systems grants you use of this code, under the following license:
# 
# Copyright (c) 2000-2007, ITT Visual Information Solutions 
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions: 
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


import gobject
import threading
import gtk
import math
import Image
gobject.threads_init()

from osgeo import gdal, gdalconst


JPIP_NQUALITYLAYERS = "JPIP_NQUALITYLAYERS"
JPIP_NRESOLUTIONLEVELS = "JPIP_NRESOLUTIONLEVELS"
JPIP_NCOMPS = "JPIP_NCOMPS"
JPIP_SPRECISION = "JPIP_SPRECISION"

class _IdleObject(gobject.GObject):
    """
    Override gobject.GObject to always emit signals in the main thread
    by emitting on an idle handler
    """
    def __init__(self):
        gobject.GObject.__init__(self)
    
    
    def emit(self, *args):
        gobject.idle_add(gobject.GObject.emit, self, *args)  

class JPIPImageReader(threading.Thread, _IdleObject):
    __gsignals__ = {
            "completed": (
                gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, []),
            "update": (
                gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, [gtk.gdk.Pixbuf]) # updating, bytes received
        }
        
        
    def __init__(self):
        threading.Thread.__init__(self)
        _IdleObject.__init__(self)
        self.__cancelled = True
        self.finished = True
        self.cLevels = 0
        self.qLevels = 0
        self.nComps = 0
        self.nPrec = 0
        self.dataset = None
        self.url = None

        self.currentLevel = 0
        self.currentWindow = None
        self.levels = []
        
    def setLevel(self, level):
        lock = threading.Lock()
        try:
            self.currentLevel = self.cLevels - level - 1
            self.finished = False
        finally:
            try:
                lock.release()
            except:
                pass

        level = self.levels[self.currentLevel]
        return level[0], level[1]  
        
    def getLevel(self, windowW, windowH):
        # calculate a 'good' level to request matching the current display
        result = 0  
        for level in self.levels:
            if (level[0] < windowW) or (level[1] < windowH):
                break 
            result = result + 1
        
        self.currentLevel = result
        level = self.levels[self.currentLevel]

        return self.cLevels - self.currentLevel - 1, level[0], level[1]  
        
    def request(self, x, y, w, h):
        lock = threading.Lock()
        try:
            self.finished = False
            bands = None
            if self.nComps > 1:
                bands = [0, 1, 2]
            else:
                bands = [0]
                
            self.currentWindow = [x, y, w, h, bands]
        finally:
            try:
                lock.release()
            except:
                pass

    def cancel(self):
        """ 
        Threads in python are not cancellable, so we implement our own
        cancellation logic
        """
        self.__cancelled = True
  
    def open(self, url):
        gdal.SetConfigOption("CPL_DEBUG", "ON");

        self.url = url
        # use GDAL to open jpip(s) url
        if not self.dataset is None:
            self.dataset = None
            
        self.dataset = gdal.Open(self.url)
        if self.dataset != None:
            # as an example print image info
            print "Opened JPIP dataset with %s" % self.dataset.GetDriver().ShortName
            print 'Size is ', self.dataset.RasterXSize,'x', self.dataset.RasterYSize, \
            'x', self.dataset.RasterCount # raster band interface is not implemented, hence raster count is zero
            print 'Projection is ', self.dataset.GetProjection()
            geotransform =  self.dataset.GetGeoTransform()
            if not geotransform is None:
                print 'Origin = (', geotransform[0], ',', geotransform[3], ')'
                print 'Pixel Size = (', geotransform[1], ',', geotransform[5], ')'
            
            # get JPIP info
            meta = self.dataset.GetMetadata("JPIP")
            
            self.qLevels = int(meta[JPIP_NQUALITYLAYERS])
            print "Quality Layers %i" % self.qLevels
            
            self.cLevels = int(meta[JPIP_NRESOLUTIONLEVELS])
            print "Resolution Levels %i" %  self.cLevels
            
            self.nComps = int(meta[JPIP_NCOMPS])
            print "Number Components %i" % self.nComps
            
            self.nPrec = int(meta[JPIP_SPRECISION])
            print "Data precision %i" %self.nPrec
            
            # calculate levels
            for i in range(0, self.cLevels + 1):
                # round up is the default
                scale = 2 ** i
                w = math.ceil(1.0 * self.dataset.RasterXSize / scale)
                h = math.ceil(1.0 * self.dataset.RasterYSize / scale)
                self.levels.append([int(w), int(h)])
        else:
            print "Cannot open %s" % self.url
            self.__cancelled = True
            self.finished = True
 
    def isCancelled(self):
        return self.__cancelled
    
    def close(self):
        self.__cancelled = True
        
    def run(self):
        window = None
        asyncRasterIO = None
        buffer = None
        self.__cancelled=False

        while 1:
            if self.__cancelled:
                break
            
            if (self.currentWindow is not None) and (window != self.currentWindow):
                self.finished = False
                window = self.currentWindow      
            else:
                self.finished = True                

            if not self.finished:
                # make requests to GDAL
                bands = []
                if not asyncRasterIO is None:
                    self.dataset.EndAsyncRasterIO(asyncRasterIO)
                    asyncRasterIO = None
                    
                options = ["LEVEL=%i" % self.currentLevel]
                
                # note we can specify the buffer type (defaults to 8 bit), bands
                bands = window[4]
                asyncRasterIO = self.dataset.BeginAsyncRasterIO(window[0], window[1], window[2], window[3], band_list=bands, options=options)
                
                xOff = 0
                yOff = 0
                xSize = 0
                ySize = 0

                newWindow = asyncRasterIO.GetNextUpdatedRegion(True, 2000)
                while (newWindow[0] == gdalconst.GARIO_UPDATE):
                    # GARIO result in in window[0]
                    # check whether current window has changed
                    if window != self.currentWindow:
                        break
                    
                    # process response in asyncRasterIO.Buffer
                    img = Image.fromstring("RGBA", (asyncRasterIO.XSize, asyncRasterIO.YSize), asyncRasterIO.Buffer)
                    b,g,r,a = img.split()
                    img = Image.merge("RGB", (r, g, b))
                    
                    
                    img.save("c:/tmp/test.jpg", "JPEG")

                    pixelBuffer = gtk.gdk.pixbuf_new_from_data(img.tostring(), gtk.gdk.COLORSPACE_RGB, False, 8, \
                        asyncRasterIO.XSize, asyncRasterIO.YSize, asyncRasterIO.XSize * 3)
                    
                    self.emit("update", pixelBuffer)
        
                    print "READ %i Kbs from the server " % (asyncRasterIO.NDataRead / 1024)                    
                    
                    newWindow = asyncRasterIO.GetNextUpdatedRegion(True, 2000)

                self.finished = True