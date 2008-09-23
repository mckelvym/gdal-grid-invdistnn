#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  cgi to delete a task from the task queue (either a pending or
#            or an active task).
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
import owmt_showqueue
import owmt_login

# *****************************************************************************
#		execute() 

def execute( form ):

    queue_id = int(form['queue_id'].value)

    error = None
    
    # Login to Postgres.
    
    owmt.pg_login( form )

    if owmt.pg_ds is None:
        return owmt_login.execute( form, owmt.pg_error )

    # Find the task in question.
    rs = owmt.pg_ds.ExecuteSQL(
        """SELECT owner_pid, status
           FROM owmt_task_queue
           WHERE id = %d""" % queue_id )

    try:
        feat = rs.GetNextFeature()
        if feat.owner_pid != -1:
            error = 'Not yet able to kill active tasks.'

        else:
            owmt.pg_ds.ExecuteSQL(
                """DELETE FROM owmt_task_queue
                   WHERE id = %d""" % queue_id )
    except:
        error = 'Could not find task in queue!\n' + gdal.GetLastErrorMsg()

    owmt_showqueue.execute( form, error )
