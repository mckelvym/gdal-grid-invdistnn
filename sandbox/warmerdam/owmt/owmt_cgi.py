#******************************************************************************
# 
#  Project:  OGR Web Migration Tool
#  Purpose:  Generic cgi interpreter - calls particular operations.
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
import string
import os
import cgi

import cgitb
cgitb.enable()

sys.path.append( os.environ['OWMT_HOME'] )

import owmt
from osgeo import ogr, gdal

#******************************************************************************
#		ServiceMain()

print 'Content-type: text/html'
print

form = cgi.FieldStorage()

op = form['op'].value

if op == 'login':
    import owmt_login
    owmt_login.execute( form )

elif op == 'tasklist':
    import owmt_tasklist
    owmt_tasklist.execute( form )

elif op == 'createtask':
    import owmt_createtask
    owmt_createtask.execute( form )

elif op == 'submittask':
    import owmt_submittask
    owmt_submittask.execute( form )

elif op == 'runtasks':
    import owmt_runtasks
    owmt_runtasks.execute( form )

elif op == 'showqueue':
    import owmt_showqueue
    owmt_showqueue.execute( form )

elif op == 'delete_queued_task':
    import owmt_delete_queued_task
    owmt_delete_queued_task.execute( form )

elif op == 'delete_task':
    import owmt_delete_task
    owmt_delete_task.execute( form )

else:
    print '<h1>Error</h1>'
    print
    print 'op=%s not yet supported.' % op

