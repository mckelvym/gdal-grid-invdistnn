		OGR Web Migration Tool  -  User Guide
		=====================================

Refer to README_SETUP.TXT for information on installing OWMT and
related software.

Connect
-------

To login to OWMT, visit the login page, typically:

  http://localhost/cgi-bin/owmt_cgi.py?op=login

If no login page is shown, review the system setup.


Login
-----

On the login screen provide the connect information for the 
PostgreSQL database to which data will be migrated.  This database
will also be used to host most of the OWMT dynamic data (ie. 
migration task definitions).  

If the Postgres database is local, the host can be "localhost", 
otherwise it should be the ip# or DNS name of the machine with the
postgres instance. 

The instance name is required.  It is also required that the database
instance have been precreated, and that this instance includes PostGIS
support.  

The userid and password may be required depending on how authentication
has been setup for the postgresql instance.


Predefined Task List
--------------------

If postgresql login is successful, the user will be shown a "Predefined
Task List" screen.  This will show all the migration tasks that have been
defined for this postgres database - initally none.

To create a migration ask hit the "Create Task" button. 

Once there are tasks, one or more can be selected (using the radio buttons)
and run using "Run Tasks". 

Running migration tasks can be viewed using the "View Queue" button.


Create Task
-----------

By default the create task screen shows a list of spatial tables from
the default schema of the Oracle database.  This occurs if the Oracle 
Connect field reads "(default)".  

However, on first entering this screen the oracle connect information
will not be known.  Select the "Update Oracle Connect Default" to 
enter the oracle connect info. 

Once properly supplied the screen will show available tables.  It
is also possible to replace the (default) text field value with any
OGR supported datasource name (oracle or otherwise) in order to 
load other spatial datasources.  For instance, this could be used
to load shapefiles into Postgres.

The known spatial tables/layers from the source will be shown 
with radio buttons.  One or more should be selected for the migration
task.  Also, a name should be entered for the migration task.  Then
hit "Create Task".  

Once this is done, the user will be returned to the predefined task list
with the new task added to the list.


Run Tasks
---------

On the predefined task view screen the user can select one or more tasks
to run, and hit "Run Tasks".  This adds these tasks to the migration task
queue and if necessary starts a worker process to processing the migration(s).

The user is then shown the current tasks (pending, active and completed).
They can hit "Refresh Queue" to see up to date information on task processing
or "Return to Predefined Tasks" to return to the main dialog. 

Tasks will only be shown in the "Completed Task(s)" list once with the
completion status (normally "OK").  


Error Recover
-------------

Should something happen and the migration tool gets "stuck", such that
tasks are no longer being processed properly it may be necessary to clear
the task queue.  This is an emergency process, and can be accomplished
by connecting to the PostgreSQL database with the psql commandline
program and issuing the command:

  delete from owmt_task_queue;






