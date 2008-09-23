#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Implementation of the login cgi form action.
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
from osgeo import gdal, ogr

# *****************************************************************************
#		execute() 

def execute( form, error = None ):

    sub_dict = {}

    if error is not None:
        sub_dict['ERROR'] = error + '<hr>'
    else:
        sub_dict['ERROR'] = ''
        
    owmt.PG_Database = ''   # avoid putting out extra hidden variables in emit

    template = owmt.get_template( 'owmt_login.html' )

    print owmt.sub_template( template, sub_dict )
    


    
