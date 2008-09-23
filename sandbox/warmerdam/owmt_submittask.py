#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Implementation of the Task List cgi form action which shows
#            the list of available tasks to execute.
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
import owmt_createtask
import owmt_login
import owmt_tasklist

# *****************************************************************************
#		owmt_submittask.execute() 

def execute( form ):

    # Did we get a task name?
    if not form.has_key('task'):
        owmt_createtask.execute( form, error='Require Task name is missing.' )
        return

    # Check at least some layer(s) have been selected.
    layer_ids = []
    for key in form.keys():
        if key[:6] == 'layer_':
            layer_ids.append( int(key[6:]) )

    if len(layer_ids) == 0:
        owmt_createtask.execute( form, error='No tables selected to transfer.' )
        return
        
    # Login to Postgres.
    
    owmt.pg_login( form )

    if owmt.pg_ds is None:
        return owmt_login.execute( form, owmt.pg_error )

    # Verify our task name is not already in use.
    # TODO

    # Login to Oracle
    
    if not owmt.oci_login( form ):
        print 'Login to Oracle Database failed!<p>'
        print 'Connection String: %s<p>' % owmt.OCI_Connect
        
        print 'Error:<br>%s<p>' % gdal.GetLastErrorMsg()
        return

    # Confirm we have all the layers indicated, and turn the layer
    # numbers into layer/table names.
    layer_names = ''
    for iLayer in layer_ids:
        lyr = owmt.oci_ds.GetLayer(iLayer)
        if layer_names != '':
            layer_names += ','
        layer_names += lyr.GetLayerDefn().GetName()

    # add this task as a record in the table.
    owmt.pg_ds.ExecuteSQL(
        """INSERT INTO owmt_tasks
           (name,oci_connection,src_table,dst_table)
           VALUES
           ('%s','%s','%s','%s')""" \
           % (form['task'].value,owmt.OCI_Connect,layer_names,layer_names) )

    owmt_tasklist.execute( form )
