@echo off

:: ****************************************************************************
::  $Id: $
:: 
::  Name:     makegdal_gen.bat
::  Project:  GDAL 
::  Purpose:  Generate MS Visual Studio 2003...N project files    
::  Author:   Ivan Lucena, ivan.lucena@pmldnet.com
:: 
:: ****************************************************************************
::  Copyright (c) 2007, Ivan Lucena    
:: 
::  Permission is hereby granted, free of charge, to any person obtaining a
::  copy of this software and associated documentation files (the "Software"),
::  to deal in the Software without restriction, including without limitation
::  the rights to use, copy, modify, merge, publish, distribute, sublicense,
::  and/or sell copies of the Software, and to permit persons to whom the
::  Software is furnished to do so, subject to the following conditions:
:: 
::  The above copyright notice and this permission notice shall be included
::  in all copies or substantial portions of the Software.
:: 
::  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
::  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
::  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
::  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
::  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
::  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
::  DEALINGS IN THE SOFTWARE.
:: ****************************************************************************

::  *********************
::  Usage
::  *********************

if "%1"=="" (
    echo Usage: makegdal_gen "MS Visual C++ version" ^> makegdalNN.vcproj
    echo Examples:
    echo    makegdal_gen 7.10 ^> makegdal80.vcproj
    echo    makegdal_gen 8.00 ^> makegdal80.vcproj
    goto :end
)

::  *********************
::  Get Visual C++ version
::  *********************

set _vcver_=%1

::  *********************
::  Get GDAL Version
::  *********************

for /f %%v in (VERSION) do set _gdalver_=%%v
set _gdalnum_=%_gdalver_:.=%
set _gdalnum_=%_gdalnum_:~0,2%

:: **********************************************
:: Main file generator
:: **********************************************

echo ^<?xml version="1.0" encoding="Windows-1252"?^>
echo ^<VisualStudioProject            
echo 	ProjectType="Visual C++"
echo 	Version="%_vcver_%"
echo 	Name="makegdal%_vcnum_%"
echo 	ProjectGUID="{769DD10E-E284-46BE-9172-A35184250A3A}"
echo 	Keyword="MakeFileProj"^>
echo 	^<Platforms^>
echo 		^<Platform Name="Win32"/^>
echo 	^</Platforms^>
echo 	^<Configurations^>
echo 		^<Configuration
echo 			Name="Debug|Win32"
echo 			OutputDirectory="$(ConfigurationName)"
echo 			IntermediateDirectory="$(ConfigurationName)"
echo 			ConfigurationType="0"^>
echo 			^<Tool
echo 				Name="VCNMakeTool"
echo 				BuildCommandLine=^"cd $(ProjectDir)
echo nmake -f makefile.vc^"
echo 				ReBuildCommandLine=^"cd $(ProjectDir)
echo nmake -f makefile.vc
echo nmake -f makefile.vc install^"
echo 				CleanCommandLine=^"cd $(ProjectDir)
echo nmake -f makefile.vc clean^"
echo 				Output="gdal%_gdalnum_%.dll"/^>
echo 		^</Configuration^>
echo 		^<Configuration
echo 			Name="Release|Win32"
echo 			OutputDirectory="$(ConfigurationName)"
echo 			IntermediateDirectory="$(ConfigurationName)"
echo 			ConfigurationType="0"^>
echo 			^<Tool
echo 				Name="VCNMakeTool"
echo 				BuildCommandLine=^"cd $(ProjectDir)
echo nmake -f makefile.vc^"
echo 				ReBuildCommandLine=^"cd $(ProjectDir)
echo nmake -f makefile.vc clean
echo nmake -f makefile.vc
echo nmake -f makefile.vc install^"
echo 				CleanCommandLine=^"cd $(ProjectDir)
echo nmake -f makefile.vc clean^"
echo 				Output="gdal%_gdalnum_%.dll"/^>
echo 		^</Configuration^>
echo 	^</Configurations^>
echo 	^<References^>
echo 	^</References^>
echo 	^<Files^>
call :create_filter . "*.vc;*.opt" "Make Files"    "	"
call :create_filter . "*.h"        "Include Files" "	"
call :create_filter . "*.c;*.cpp"  "Source Files"  "	"
echo 	^</Files^>
echo 	^<Globals^>
echo 	^</Globals^>
echo ^</VisualStudioProject^>

goto :end

:: **********************************************
:create_filter
:: **********************************************

    set _path_=%1
    set _mask_=%2
    set _name_=%3
    set _tabs_="	"%4
    set _next_="	"%_tabs_%
    
    ::  *********************
    ::  remove quotes 
    ::  *********************
    
    set _name_=%_name_:"=%
    set _mask_=%_mask_:"=% 
    set _tabs_=%_tabs_:"=%
    set _next_=%_next_:"=%
    
    ::  *********************
    ::  stop folders
    ::  *********************
    
    for %%d in (data debian dist_docs docs html m4 pymod swig vb6) do (
        if "%_name_%"=="%%d" (
            goto :end
        )
    )

    ::  *********************
    ::  check whole folder tree
    ::  *********************
    
    set _find_=0
    for /R %%f in (%_mask_%) do set _find_=1    
    if %_find_%==0 (
        goto :end
    )

    ::  *********************
    ::  create filter 
    ::  *********************
    
    echo %_tabs_%^<Filter Name="%_name_%" Filter="%_mask_%"^>
    
    ::  *********************
    ::  add files
    ::  *********************
    
    for %%f in (%_mask_%) do (
        echo %_next_%^<File RelativePath="%_path_%\%%f" /^>
    )
    
    ::  *********************
    ::  clib all the branches
    ::  *********************
    
    for /D %%d in (*) do (
        cd %%d
        call :create_filter %_path_%\%%d "%_mask_%" %%d "%_tabs_%"
        cd ..
    )
    
    echo %_tabs_%^</Filter^>

:: **********************************************
:end
:: **********************************************
