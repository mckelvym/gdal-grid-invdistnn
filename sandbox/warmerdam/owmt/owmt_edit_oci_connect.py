#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Implementation of dialog to enter Oracle connect info.
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
#		owmt_edit_oci_connect.execute() 

def execute( form ):

    # We only do this to ensure the PG parms will get passed on.
    # We don't really need access to postgres.
    
    owmt.pg_login( form )

    # Are we processing an OCI info update?  If so, apply it.
    if form.has_key('OCI_Instance') and form.has_key('OCI_Userid'):

        try:
            password = form['OCI_Password'].value
        except:
            password = ''
        try:
            table_list = form['OCI_Tables'].value
        except:
            table_list = ''
            
        owmt.set_oci_connect_info( form['OCI_Userid'].value,
                                   password,
                                   form['OCI_Instance'].value,
                                   table_list )

    # Now fetch the current values and show them - ready for editing.
    
    template = owmt.get_template('owmt_edit_oci_connect.html')

    sub_dict = {}

    userid,password,dbinstance,table_list,connect = owmt.get_oci_connect_info()

    sub_dict['OCI_INSTANCE'] = dbinstance
    sub_dict['OCI_USERID'] = userid
    sub_dict['OCI_PASSWORD'] = password
    sub_dict['OCI_TABLES'] = table_list
    
    print owmt.sub_template( template, sub_dict )

    
