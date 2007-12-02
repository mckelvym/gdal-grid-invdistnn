#ifdef PERL_CPAN_NAMESPACE
%module "Geo::OGR"
#else
%module ogr
#endif

%init %{

  UseExceptions();
  if ( OGRGetDriverCount() == 0 ) {
    OGRRegisterAll();
  }
  
%}
