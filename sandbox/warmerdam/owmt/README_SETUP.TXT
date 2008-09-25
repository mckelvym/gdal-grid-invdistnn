		OGR Web Migration Tool  -  Setup and Installation
		=================================================


OSGeo4W
-------

OWMT is normally deployed on top of OSGeo4W, and windows distribution
including a variety of FOSS4G software packages, including Apache,
Python and GDAL/OGR.  To prepare OSGeo4W visit:

  http://osgeo4w.osgeo.org/

Download the installer, and install.  The advanced install option will
need to be selected.  Ensure that OSGeo4W is installed in the default
location - C:\OSGeo4W for minimum problems.  The following packages will
be required:
									-
  gdal-python
  gdal-oracle10g
  apache

After installing, launch the OSGeo4W command shell and enter the following
command to setup Apache on port 80 as a default service:

  apache-install.bat

OWMT
----

The OWMT package is delivered as a .zip ready for unpacking on top of
OSGeo4W.  The package should be named something like "owmt-0.9-1.zip".  Copy
this into C:\OSGeo4W and unpack it (ensuring that appropriate unzip options
are used to create subdirectories). 

This should result in:

  c:\OSGeo4W\bin\owmt_cgi.py         - the base cgi script
  C:\OSGeo4W\httpd.d\httpd_owmt.conf - OWMT related Apache configuration
  C:\OSGeo4W\apps\owmt               - the rest of OWMT - (html+.py)

As long as OSGeo4W is at C:\OSGeo4W there should be no need for OWMT 
configuration.

PostgreSQL
----------

OSGeo4W includes the postgres client libraries, but it is still required
that PostgreSQL+PostGIS be separately installed and setup on a server.  It
can be setup on the same server as OWMT or a different one. If on 
a different server, take care to start Postgres with remote connections
enabled (ie. pg_ctl start -o -i), and to update the pg_hba.conf appropriately
to connect from the machine with OWMT installed.

Oracle
------

The gdal-oracle10g package includes support for oracle access from 
GDAL/OGR, but it depends on the Oracle OCI client libraries also being
installed separately on the machine OWMT is hosted on.  

The actual oracle instance can be local or remote, but if remove it is
generally best to configure a local "TNS name" for the remote database. 


