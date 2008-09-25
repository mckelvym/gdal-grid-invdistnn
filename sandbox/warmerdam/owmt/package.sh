#!/bin/bash
cd /c/osgeo4w/
VER="0.9-1"
tar cjvf ~/owmt-${VER}.tar.bz2 bin/owmt*.py httpd.d/httpd_owmt.conf \
                    apps/owmt/*.*
zip -r ~/owmt-${VER}.zip bin/owmt*.py httpd.d/httpd_owmt.conf \
                    apps/owmt/*.*
