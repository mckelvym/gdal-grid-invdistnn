#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  cgi to delete a predefined task from the ownt_task table.
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
import owmt_tasklist
import owmt_login

# *****************************************************************************
#		execute() 

def execute( form ):

    task_id = int(form['task_id'].value)

    error = None
    
    # Login to Postgres.
    
    owmt.pg_login( form )

    if owmt.pg_ds is None:
        return owmt_login.execute( form, owmt.pg_error )

    owmt.pg_ds.ExecuteSQL(
        """DELETE FROM owmt_tasks
        WHERE id = %d""" % task_id )

    owmt_tasklist.execute( form )
