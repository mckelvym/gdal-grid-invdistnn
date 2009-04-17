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

import pygtk; pygtk.require('2.0')
import gtk

import sys
import JPIPImageReader

title = "JPIP Viewer - 0.1"
border = 5
winWidth, winHeight = 1000, 900

class Viewer(object):
    def __init__(self):
        self.state = 0
        
        # create a new window
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.connect("delete_event", self.delete_event)
        self.window.connect("destroy", self.destroy)
        self.window.connect("window_state_event", self.window_event)
        
        self.window.set_title(title)
        # set the border width of the window
        self.window.set_border_width(border)
        self.window.set_position(gtk.WIN_POS_CENTER)
        self.window.set_size_request(winWidth, winHeight)
        
        self.vbox = gtk.VBox()
        self.window.add(self.vbox)
        
        # create a new scrolled window
        self.scrolled_window = gtk.ScrolledWindow()
        self.scrolled_window.set_border_width(border)
        self.scrolled_window.set_shadow_type(gtk.SHADOW_ETCHED_OUT)
        
        self.scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        
        self.vbox.pack_start(self.scrolled_window, expand=True, fill=True)
  
        self.hbox = gtk.HBox()
        self.vbox.pack_start(self.hbox, expand=False, fill=False)

        # adjustment object, corresponds to jp2 zoom levels
        self.adj = gtk.Adjustment(0, 0, 5, 1, 1, 1)
        self.zoomBar = gtk.ProgressBar(adjustment=self.adj)   

        imgZoomOut = gtk.Image()
        imgZoomOut.set_from_stock(gtk.STOCK_ZOOM_OUT, gtk.ICON_SIZE_SMALL_TOOLBAR)
        btnZoomOut = gtk.Button()
        btnZoomOut.set_image(imgZoomOut)
        btnZoomOut.connect("clicked", self.zoom, "out")
        
        imgZoomIn = gtk.Image()
        imgZoomIn.set_from_stock(gtk.STOCK_ZOOM_IN, gtk.ICON_SIZE_SMALL_TOOLBAR)
        btnZoomIn = gtk.Button()
        btnZoomIn.set_image(imgZoomIn)
        btnZoomIn.connect("clicked", self.zoom, "in")
        
        self.hbox.pack_start(btnZoomOut, expand=False, fill=False)
        self.hbox.pack_start(self.zoomBar, expand=True, fill=True)
        self.hbox.pack_start(btnZoomIn, expand=False, fill=False)
        
        # add a image widget
        self.image = gtk.Image()
        self.image.set_alignment(0, 0)
        self.image.set_padding(0, 0)
        self.viewport = gtk.Viewport()
        self.viewport.add(self.image)
        self.scrolled_window.add(self.viewport)
        
        self.scrolled_window.get_hscrollbar().connect("value-changed", self.viewportChanged)
        self.scrolled_window.get_vscrollbar().connect("value-changed", self.viewportChanged)
        
        self.reader = JPIPImageReader.JPIPImageReader()
        self.reader.connect("update", self.update)
        self.reader.connect("completed", self.completed)
        
        self.window.show_all()    
    
    def delete_event(self, widget, event, data=None):
        return False

    # Another callback
    def destroy(self, widget, data=None):
        self.reader.close()
        gtk.main_quit()  
        
    def window_event(self, widget, event, data=None):
        state = event.new_window_state
        x, y = int(self.viewport.get_hadjustment().get_value()), int(self.scrolled_window.get_vadjustment().get_value())
        windowW = 0
        windowH = 0
        
        if (state == 0):
            windowW, windowH = self.window.get_size_request()
        elif (state == gtk.gdk.WINDOW_STATE_MAXIMIZED):
            screen = self.window.get_screen()
            windowW, windowH = screen.get_width(), screen.get_height()
            
        level, w, h = self.reader.getLevel(windowW, windowH)
        self.adj.value = level

        self.image.set_size_request(w, h)
        self.image.set_from_pixbuf(gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB,
           has_alpha=False, bits_per_sample=8,
           width=w, height=h))

        # requests are in 1:1 resolution
        p = self.getRequestParameters(level, x, y, windowW, windowH)
        self.reader.request(p[0], p[1], p[2], p[3])

        self.state = state
        if (self.reader.isCancelled()):
            self.reader.start()
        
    def update(self, thread, pixBuf):
        dest = self.image.get_pixbuf()
        x, y = int(self.viewport.get_hadjustment().get_value()), int(self.scrolled_window.get_vadjustment().get_value())
        pixBuf.copy_area(0, 0, pixBuf.get_width(), pixBuf.get_height(), dest, x, y)
        self.image.queue_draw()
        
    def completed(self, thread):
        pass
       
    def zoom(self, widget, data=None):
        level = int(self.adj.get_value());
        
        if data == "in":
            if (level < self.adj.upper):
                level += 1
                self.adj.set_value(level)
        elif data =="out":
            if (level > self.adj.lower):
                level -= 1
                self.adj.set_value(level)

        w, h = self.reader.setLevel(level)
        
        self.image.set_size_request(w, h)
        self.image.set_from_pixbuf(gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB,
           has_alpha=False, bits_per_sample=8,
           width=w, height=h))
 
        if (self.state == 0):
            w,h = self.window.get_size_request()
            p = self.getRequestParameters(level, 0, 0, w, h)
            self.reader.request(p[0], p[1], p[2], p[3])

        elif (self.state == gtk.gdk.WINDOW_STATE_MAXIMIZED):
            screen = self.window.get_screen()
            w, h = screen.get_width(), screen.get_height()
            p = self.getRequestParameters(level, 0, 0, w, h)
            self.reader.request(p[0], p[1], p[2], p[3])
            

    def viewportChanged(self, range):
        x, y = int(self.viewport.get_hadjustment().get_value()), int(self.scrolled_window.get_vadjustment().get_value())
        level = int(self.adj.value)

        if (self.state == 0):
            w,h = self.window.get_size_request()
            p = self.getRequestParameters(level, x, y, w, h)
            self.reader.request(p[0], p[1], p[2], p[3])
        elif (self.state == gtk.gdk.WINDOW_STATE_MAXIMIZED):
            screen = self.window.get_screen()
            w, h = screen.get_width(), screen.get_height()
            p = self.getRequestParameters(level, x, y, w, h)
            self.reader.request(p[0], p[1], p[2], p[3])
        
    def getRequestParameters(self, level, x, y, w, h):
        # compute parameters at 1:1
        level = self.reader.cLevels - level - 1
        return [x * 2**level, y * 2**level, w * 2**level, h * 2**level]        
        
    def main(self, url=None):
        self.reader.open(url)
        # fix the adjustment range with the new data from the server
        self.adj.upper = self.reader.cLevels -1
        
        gtk.main()
        
if __name__ == "__main__":
    # debug import problems
    #print sys.path
    viewer = Viewer()
    viewer.main(sys.argv[1]);
        