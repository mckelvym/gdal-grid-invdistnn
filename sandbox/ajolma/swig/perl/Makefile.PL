use ExtUtils::MakeMaker;

$gdal = 'c:/dev/gdal';

$libdir = $ENV{'PREFIX'} ? "-L$ENV{'PREFIX'}/lib" : '';

$ext = $^O eq 'MSWin32' ? '.dll' : ''; # works with MinGW but probably not with ActivePerl

# for old-style, use gdal, gdalconst,ogr, and osr as keys

%object = ( 'Geo::GDAL' => 'gdal_wrap.o',
	    'Geo::GDAL::Const' => 'gdalconst_wrap.o',
	    'Geo::OGR' => 'ogr_wrap.o',
	    'Geo::OSR' => 'osr_wrap.o' );

for my $module (keys %object) {
    my $add = $module;
    $add =~ s/:/_/g;
    WriteMakefile( NAME => $module,
		   VERSION_FROM => 'lib/Geo/GDAL.pm',
		   MAKEFILE => 'Makefile_'.$add,
		   LIBS => "$libdir -L/msys/1.0/local/lib -L$gdal/.libs -L$gdal -lgdal$ext",
		   INC => "-I$gdal/gcore -I$gdal/alg -I$gdal/ogr -I$gdal/port",
		   OBJECT => $object{$module}
		   );
}
