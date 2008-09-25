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
import owmt_login

# *****************************************************************************
#		owmt_tasklist.execute() 

def execute( form, error = None ):

    owmt.pg_login( form )

    if owmt.pg_ds is None:
        return owmt_login.execute( form, owmt.pg_error )

    sub_dict = {}

    if error is not None:
        sub_dict['ERROR'] = '%s<hr>' % error
    else:
        sub_dict['ERROR'] = ''

    rs = owmt.pg_ds.ExecuteSQL( 'select * from owmt_tasks' )

    if rs is None:
        rs = []

    task_list = ''
    
    for row in rs:

        delete_url = owmt.url_prepare( 'delete_task' )
        delete_url += '&task_id=%d' % row.id
        
        task_list += """
          <tr>
            <td><input type="checkbox" name="task_%d"></td>
            <td><a href="%s">Delete</a></td>
            <td>%s</td>
            <td>%s</td>
          </tr>""" % (row.id, delete_url, row.name, row.src_table)

    sub_dict['TASK_LIST'] = task_list
    
    template = owmt.get_template('owmt_tasklist.html')

    print owmt.sub_template( template, sub_dict )
        
        

    
    

    
