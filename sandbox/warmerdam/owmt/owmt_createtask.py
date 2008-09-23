#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Implementation of dialog to enter information for new tasks.
#  Author:   Frank Warmerdam, warmerdam@pobox.com
# 
#******************************************************************************
#  Copyright (c) 2008, Frank Warmerdam <warmerdam@pobox.com>
# 
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#******************************************************************************

import sys
import os

sys.path.append( os.environ['OWMT_HOME'] )

from osgeo import gdal, ogr
import owmt

# *****************************************************************************
#		owmt_createtask.execute() 

def execute( form ):

    owmt.pg_login( form )

    if owmt.pg_ds is None:
        print 'Login to Postgres Database failed!<p>'
        print 'Connection String: %s<p>' % owmt.pg_connection
        
        print 'Error:<br>%s<p>' % gdal.GetLastErrorMsg()
        return

    print owmt.get_template('owmt_createtask_header.html')
    
    if form.has_key('OCI_Connect'):
        OCI_Connect = form['OCI_Connect'].value
    else:
        OCI_Connect = ''

    # The first form just hold OCI connect info, and a submit results
    # in regenerating the list of tables in the database.
    owmt.form_emit( 'createtask' )
    print """
        Oracle Connect:
        <input type="text" size="45" name="OCI_Connect" value="%s"><p>
        <input type="submit" value="Fetch Tables">
        </form>""" % OCI_Connect

    # Try to connect to the indicated database.

    if not owmt.oci_login( form ) and OCI_Connect != '':
        print 'Login to Oracle Database failed!<p>'
        print 'Connection String: %s<p>' % owmt.OCI_Connect
        
        print 'Error:<br>%s<p>' % gdal.GetLastErrorMsg()
        return
        
    # The second form lets us select tables to migrate.

    if owmt.oci_ds is not None:

        owmt.form_emit( 'submittask' )
        print """<table>"""
        for i in range(owmt.oci_ds.GetLayerCount()):
            name = owmt.oci_ds.GetLayer(i).GetLayerDefn().GetName()
            
            print '<tr>'
            print '  <td><input type="checkbox" name="layer_%d"></td>' % i
            print '  <td>%s</td>' % name
            print '</tr>'

        print """</table><p>

        Task Name: <input name="task" type="text" value="" size="45"><p>
        <input type="submit" value="Submit">
        </form>"""
    
    print owmt.get_template('owmt_createtask_footer.html')
    
