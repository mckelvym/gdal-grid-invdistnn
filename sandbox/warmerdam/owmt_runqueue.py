#!/usr/bin/env python
#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Script to execute the tasks in the pending queue.  This is
#            run as a distinct process. 
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

import owmt
from osgeo import ogr
from osgeo import gdal

###############################################################################

def post_progress( queue_id, progress_ratio ):

    percent = int(progress_ratio * 100)
    if percent > 100:
        percent = 100
    if percent < 0:
        percent = 0

    owmt.pg_ds.ExecuteSQL(
        """UPDATE owmt_task_queue
        SET status = %d
        WHERE id = %d
        """ % (percent, queue_id) )

###############################################################################

def post_report( queue_id, message ):

    # Launder message.
    
    for i in range(len(message)):
        if ord(message[i]) > 127:
            message = message[:i] + '*' + message[i+1:]
        if message[i] == "'":
            message = message[:i] + '"' + message[i+1:]

    owmt.pg_ds.ExecuteSQL(
        """UPDATE owmt_task_queue
        SET report = '%s'
        WHERE id = %d
        """ % (message, queue_id) )

###############################################################################

def execute_table_copy( queue_id, src_table, dst_table, options,
                        progress_base, progress_scale ):

    post_progress( queue_id, progress_base )
    post_report( queue_id, 'Processing: %s' % src_table )
    
    # get the source table, and count features.
    src_layer = owmt.oci_ds.GetLayerByName( src_table )
    if src_layer is None:
        post_report( 'failed to find table %s on source database.' % src_table)
        return 0

    feat_count = src_layer.GetFeatureCount()
    report_freq = feat_count / 20
    if report_freq < 100:
        report_freq = 100
    if report_freq > 5000:
        report_freq = feat_count / 100

    # Does the destination table already exist?  If so, delete it.
    for iLayer in range(owmt.pg_ds.GetLayerCount()):
        name = owmt.pg_ds.GetLayerByIndex( iLayer ).GetLayerDefn().GetName()
        if name.lower() == dst_table.lower():
            owmt.pg_ds.DeleteLayer( iLayer )
            break

    # Create the destination table.
    dst_layer = owmt.pg_ds.CreateLayer( dst_table, None, ogr.wkbUnknown )

    # Create fields.
    src_defn = src_layer.GetLayerDefn()
    for iField in range(src_defn.GetFieldCount()):
        src_field = src_defn.GetFieldDefn( iField )
        dst_layer.CreateField( src_field, 1 )

    # Transfer features.
    src_layer.ResetReading()
    src_feat = src_layer.GetNextFeature()

    last_report = 0
    feat_moved = 0

    while src_feat is not None:

        dst_feat = ogr.Feature( dst_layer.GetLayerDefn() )
        dst_feat.SetFrom( src_feat )
        
        result = dst_layer.CreateFeature( dst_feat )

        if result != 0:
            err_msg = gdal.GetLastErrorMsg()
            post_report( queue_id, 'feature transfer failed:\n%s' % err_msg)
            return 0

        feat_moved += 1

        if feat_moved >= last_report + report_freq:
            post_progress( queue_id,
                           progress_base + (progress_scale*feat_moved) / feat_count )
            last_report = feat_moved
            
        dst_feat = None
        src_feat = src_layer.GetNextFeature()

    post_progress( queue_id, progress_base + progress_scale )

    return 1

###############################################################################

def execute_queue_task( queue_id, task_id ):

    owmt.queue_id = queue_id
    
    # Find out details about this task.
    rs = owmt.pg_ds.ExecuteSQL(
        """SELECT * FROM owmt_tasks
           WHERE id = %d""" % task_id )
    task_row = rs.GetNextFeature()

    # Connect to datasource.
    owmt.OCI_Connect = task_row.oci_connection
    owmt.oci_ds = ogr.Open( owmt.OCI_Connect, update=0 )

    if owmt.oci_ds is None:
        post_report( 'Failed to open Oracle database: %s' % owmt.OCI_Connect )
        return 0

    # split the table lists so we can operate on pairs.

    src_tables = task_row.src_table.split(',')
    dst_tables = task_row.dst_table.split(',')

    result = 1
    
    if len(src_tables) != len(dst_tables):
        post_report( queue_id, 'mismatched count of source and destination tables' )
        result = 0
        
    # Process a single table copy.
    if result:
        for iTable in range(len(src_tables)):
            try:
                result = execute_table_copy( queue_id,
                                             src_tables[iTable],
                                             dst_tables[iTable],
                                             task_row.options,
                                             0.0, 1.0 )
            except:
                import traceback
                post_report( queue_id, traceback.format_exc() )
                result = 0
                break
            

    # set status to completed, whether successful or not.
    owmt.pg_ds.ExecuteSQL(
        """UPDATE owmt_task_queue
        SET status = 101, owner_pid = -1
        WHERE id = %d
        """ % queue_id )

    if result:
        post_report( queue_id, 'OK' )
        
    return result
    
###############################################################################
#                          main()
#
# Usage:
# owmt_runqueue.py <postgres connection string>

connection = sys.argv[1]

###############################################################################
# Connect to postgres database (owmt_slave instance)

owmt.pg_ds = ogr.Open( connection, update=1 )

if owmt.pg_ds is None:
    print """<b>Login to Postgres Database failed!</b><p>
    Connection String: %s<p>
    Error:<br>%s<p>""" % (connection,gdal.GetLastErrorMsg())
    sys.exit( 1 )

###############################################################################
# Loop until we either fail to find a task, or till doing a task fails.

pid = os.getpid()

while 1:

    # identify a queued item we want to take ownership of.
    rs = owmt.pg_ds.ExecuteSQL(
        """SELECT id, task_id
           FROM owmt_task_queue
           WHERE owner_pid = -1 and status = -1
           LIMIT 1""" )

    row = rs.GetNextFeature()
    if row is not None:
        queue_id = row.id
        task_id = row.task_id
        row = None
        owmt.pg_ds.ReleaseResultSet( rs )
    else:
        queue_id = -1

    # no pending task?
    if queue_id == -1:
        sys.exit( 0 )
        
    # Take "ownership" of finished tasks. 
    owmt.pg_ds.ExecuteSQL(
        """UPDATE owmt_task_queue
        SET owner_pid = %d, status = 0
        WHERE owner_pid = -1 and status = -1 and id = %d
        """ % (pid,queue_id) )

    execute_queue_task( queue_id, task_id )
    

    
