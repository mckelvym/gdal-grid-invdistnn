#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Script to show the current migration queue.
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

from osgeo import ogr, gdal

import owmt
import owmt_login
import owmt_tasklist

# *****************************************************************************
#		owmt_showqueue.execute() 

def execute( form, error = None ):

    owmt.pg_login( form )

    if owmt.pg_ds is None:
        return owmt_login.execute( form, owmt.pg_error )

    pid = os.getpid()

    # Take "ownership" of finished tasks. 
    owmt.pg_ds.ExecuteSQL(
        """UPDATE owmt_task_queue
           SET owner_pid = %d
           WHERE owner_pid = -1 and status = 101
           """ % pid )

    rs = owmt.pg_ds.ExecuteSQL(
        """SELECT owmt_task_queue.id,
                  owmt_task_queue.status, owmt_task_queue.report,
                  owmt_tasks.name, owmt_tasks.src_table
           FROM owmt_task_queue
           INNER JOIN owmt_tasks
           ON owmt_task_queue.task_id = owmt_tasks.id""" )

    if rs is None:
        rs = []

    ###########################################################################

    sub_dict = {}

    if error is not None:
        sub_dict['ERROR'] = error + '<hr>'
    else:
        sub_dict['ERROR'] = ''
        
    ###########################################################################
        
    pending_tasks = ''
    
    for row in rs:
        if row.status == -1:

            delete_url = owmt.url_prepare( 'delete_queued_task' )
            delete_url += '&queue_id=%d' % row.id

            pending_tasks += """
              <tr>
                <td>%s</td>
                <td><a href="%s">Delete</a></td>
                <td>%s</td>
              </tr>""" % (row.name, delete_url, row.src_table)

    sub_dict['PENDING_TASKS'] = pending_tasks

    ###########################################################################

    active_tasks = ''
    if len(rs) > 0:
        rs.ResetReading()
    for row in rs:
        if row.status >= 0 and row.status <= 100:
            
            kill_url = owmt.url_prepare( 'delete_queued_task' )
            kill_url += '&queue_id=%d' % row.id

            active_tasks += """
              <tr>
                <td>%s</td>
                <td><a href="%s">Kill</a></td>
                <td>%s</td>
                <td>%d%%</td>
                <td><pre>%s</pre></td>
              </tr>""" % (row.name, kill_url, row.src_table, row.status,
                          row.report)

    sub_dict['ACTIVE_TASKS'] = active_tasks

    ###########################################################################
    
    completed_tasks = ''
    
    if len(rs) > 0:
        rs.ResetReading()
    for row in rs:
        if row.status > 100:
            completed_tasks += """
              <tr>
                <td>%s</td>
                <td>%s</td>
                <td><pre>%s</pre></td>
              </tr>""" % (row.name, row.src_table, row.report)

    sub_dict['COMPLETED_TASKS'] = completed_tasks

    if len(rs) > 0:
        owmt.pg_ds.ReleaseResultSet( rs )

    ###########################################################################
    
    template = owmt.get_template('owmt_showqueue.html')

    print owmt.sub_template( template, sub_dict )
    
    ###########################################################################
    # Wipe reported tasks.
    
    owmt.pg_ds.ExecuteSQL(
        """DELETE FROM owmt_task_queue
           WHERE owner_pid = %d
           """ % pid )

    
    

    
