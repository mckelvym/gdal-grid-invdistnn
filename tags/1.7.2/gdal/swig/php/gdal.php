<?php

/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.26
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

global $GDAL_LOADED__;
if ($GDAL_LOADED__) return;
$GDAL_LOADED__ = true;

/* if our extension has not been loaded, do what we can */
if (!extension_loaded("php_gdal")) {
  if (!dl("php_gdal.so")) return;
}



?>
