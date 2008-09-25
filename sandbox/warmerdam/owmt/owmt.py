#!/usr/bin/env python
#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Support Python Library for OWMT cgi's. 
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

import os
import sys
import string

sys.path.append( '/home/warmerda/bld/lib/python2.5/site-packages' )

import urllib
from osgeo import ogr, gdal

pg_ds = None
pg_connection = ''

PG_Userid = ''
PG_Password = ''
PG_Database = ''
PG_Host = ''
pg_error = None

OCI_Connect = ''
oci_ds = None

# *****************************************************************************

def pg_login( form ):

    """Login to Postgres, and ensure the owmt schema is established."""

    global pg_ds, pg_connection, pg_error
    global PG_Userid, PG_Password, PG_Database, PG_Host

    if form.has_key('PG_Userid'):
        PG_Userid = form['PG_Userid'].value
    else:
        PG_Userid = ''

    if form.has_key('PG_Password'):
        PG_Password = form['PG_Password'].value
    else:
        PG_Password = ''

    if form.has_key('PG_Database'):
        PG_Database = form['PG_Database'].value
    else:
        PG_Database = ''

    if form.has_key('PG_Host'):
        PG_Host = form['PG_Host'].value
    else:
        PG_Host = ''

    connection = 'PG:dbname=%s' % PG_Database

    if PG_Host != '':
        connection += ' host=%s' % PG_Host

    if PG_Userid != '':
        connection += ' user=%s' % PG_Userid
        
    if PG_Password != '':
        connection += ' password=%s' % PG_Password

    pg_connection = connection
    
    pg_ds = ogr.Open( connection, update=1 )

    if pg_ds is None:
        pg_error = """<b>Login to Postgres Database failed!</b><p>
                   Connection String: %s<p>
                   Error:<br>%s<p>""" % (connection,
                                         gdal.GetLastErrorMsg())
        return 0

    lyr = pg_ds.ExecuteSQL( 'select * from owmt_tasks' )
    if lyr is None:
        pg_setup_skeleton()
    else:
        pg_ds.ReleaseResultSet( lyr )
  
        
# *****************************************************************************

def pg_setup_skeleton():

    """Create OWMT required tables."""

    global pg_ds

    gdal.PushErrorHandler('CPLQuietErrorHandler')

    pg_ds.ExecuteSQL( """

    CREATE TABLE owmt_tasks (
      id               SERIAL PRIMARY KEY,
      name             VARCHAR,
      oci_connection   VARCHAR,
      src_table        VARCHAR,
      dst_table        VARCHAR,
      options          VARCHAR
      )""" )

    pg_ds.ExecuteSQL( """

    CREATE TABLE owmt_task_queue (
      id               SERIAL PRIMARY KEY,
      task_id          INTEGER,
      owner_pid        INTEGER,
      status           INTEGER, 
      report           VARCHAR
      )""" )

    gdal.PopErrorHandler()

# *****************************************************************************

def oci_login( form ):

    """Login to Oracle (or any source datasource)"""

    global OCI_Connect, oci_ds

    if form.has_key('OCI_Connect'):
        OCI_Connect = form['OCI_Connect'].value
    else:
        OCI_Connect = '(default)'

    wrk_connect = OCI_Connect
    if wrk_connect == '(default)':
        oci_info = get_oci_connect_info()
        wrk_connect = oci_info[4]

    try:
        oci_ds = ogr.Open( wrk_connect )
    except:
        return 0

    if oci_ds is None:
        return 0
    else:
        return 1
  
        
# *****************************************************************************

def get_template( filename ):

    """Fetch desired template file."""
    
    full_path = os.environ['OWMT_HOME'] + '/' + filename
    try:
        return open( full_path ).read()
    except:
        return ''

# *****************************************************************************

def form_emit( operation ):

    """Write out the form preamble."""

    print form_prepare( operation )

def form_prepare( operation ):
    
    form = """
<form action="/cgi-bin/owmt_cgi.py" method="post">
<input name="op" type="hidden" value="%s">
""" % operation

    global PG_Userid, PG_Password, PG_Database, PG_Host
    if PG_Database != '':
        form += """
<input name="PG_Userid" type="hidden" value="%s">
<input name="PG_Password" type="hidden" value="%s">
<input name="PG_Database" type="hidden" value="%s">
<input name="PG_Host" type="hidden" value="%s">
""" % (PG_Userid,PG_Password,PG_Database,PG_Host)

    return form
        
# *****************************************************************************

def url_prepare( operation ):

    """Prepare an url suitable for a link href with the PG connect info
    endcoded as parameters."""

    url = '/cgi-bin/owmt_cgi.py?op=%s' % operation

    if PG_Database != '':

        url += '&PG_Userid='
        url += urllib.quote_plus( PG_Userid )
        
        url += '&PG_Password='
        url += urllib.quote_plus( PG_Password )
        
        url += '&PG_Database='
        url += urllib.quote_plus( PG_Database )
        
        url += '&PG_Host='
        url += urllib.quote_plus( PG_Host )

    return url

# *****************************************************************************

def sub_template( template, segment_dict ):

    """Perform template substitution of forms, urls and other provided
    dictionary items.  Return updated document."""

    template_chunks = template.split( '@@' )

    chunk_count = len(template_chunks)

    for iChunk in range(len(template_chunks)):

        chunk = template_chunks[iChunk]
        
        if chunk[:4] == 'URL:':
            template_chunks[iChunk] = url_prepare( chunk[4:] )

        elif chunk[:5] == 'FORM:':
            template_chunks[iChunk] = form_prepare( chunk[5:] )

        elif segment_dict.has_key(chunk):
            template_chunks[iChunk] = segment_dict[chunk]

    return ''.join(template_chunks)

# *****************************************************************************

def get_oci_connect_info():

#    full_path = os.environ['OWMT_HOME'] + '/oci_connect.txt'
    full_path = '/osgeo4w/tmp/oci_connect.txt'

    try:
	lines = open(full_path).readlines()

        return [string.strip(lines[0]),
                string.strip(lines[1]),
                string.strip(lines[2]),
                string.strip(lines[3]),
                string.strip(lines[4])]
    except:
        return ('','','', '', '')

# *****************************************************************************

def set_oci_connect_info( userid, password, dbinstance, table_list ):

#    full_path = os.environ['OWMT_HOME'] + '/oci_connect.txt'
    full_path = '/osgeo4w/tmp/oci_connect.txt'

    connect = 'OCI:'
    if userid != '':
        connect += userid

        if password != '':
            connect += '/' + password

        connect += '@'

    connect += dbinstance
    if table_list != '':
        connect += ':' + table_list

    text = '%s\n%s\n%s\n%s\n%s\n' \
           % (userid,password,dbinstance,table_list,connect)

    open(full_path,'w').write(text)

    return
    


