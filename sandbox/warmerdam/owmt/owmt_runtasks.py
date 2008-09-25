#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Add one or more tasks to the task queue.
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

import time

from osgeo import ogr, gdal

import owmt
import owmt_createtask
import owmt_login
import owmt_tasklist
import owmt_showqueue

# *****************************************************************************
#		owmt_runtasks.execute() 

def execute( form ):

    # Get the list of tasks to run.
    tasks_to_run = []
    for key in form.keys():
        if key[:5] == 'task_':
            tasks_to_run.append( int(key[5:]) )

    # if no tasks are selected, just report this and redisplay the task list.

    if len(tasks_to_run) == 0:
        owmt_tasklist.execute( form,
                               error='<b>Please select one or more tasks...</b>')
        return

    # Login to Postgres.
    
    owmt.pg_login( form )

    if owmt.pg_ds is None:
        return owmt_login.execute( form, owmt.pg_error )

    # Insert these new records in the table.

    for task in tasks_to_run:
        owmt.pg_ds.ExecuteSQL(
            """INSERT INTO owmt_task_queue
               (task_id, owner_pid, status)
               VALUES
               (%d, -1, -1 )""" % (task) )

    # Do we need to fire off the worker?

    q_lyr = owmt.pg_ds.ExecuteSQL(
        """SELECT task_id from owmt_task_queue
           WHERE owner_pid != -1 AND status < 101""" )

    if q_lyr is None or q_lyr.GetFeatureCount() == 0:
        worker_active = 0
	owmt.pg_ds.ReleaseResultSet( q_lyr )
    else:
        worker_active = 1

    if not worker_active:
        launch_worker( form )

        time.sleep( 0.333 ) # just a bit of time for the worker to accept a task
        
    owmt_showqueue.execute( form )
    return
    
# *****************************************************************************

def launch_worker( form ):

#    python = '/usr/bin/python'
    python = '/osgeo4w/bin/python.exe'

    runqueue = os.environ['OWMT_HOME'] + '/owmt_runqueue.py'

    os.spawnl( os.P_NOWAIT, python, python, runqueue, '"'+owmt.pg_connection+'"' )

