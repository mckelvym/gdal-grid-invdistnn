#ifdef PERL_CPAN_NAMESPACE
%module "Geo::GDAL"
#else
%module gdal
#endif

%init %{
  /* gdal_init.i %init code */
  UseExceptions();
  if ( GDALGetDriverCount() == 0 ) {
    GDALAllRegister();
  }
%}

%inline %{
    typedef struct
    {
	SV *fct;
	SV *data;
    } SavedEnv;
    int callback_d_cp_vp(double d, const char *cp, void *vp)
    {
	int count, ret;
	SavedEnv *env_ptr = (SavedEnv *)vp;
	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSVnv(d)));
	XPUSHs(sv_2mortal(newSVpv(cp, 0)));
	if (env_ptr->data)
	    XPUSHs(env_ptr->data);
	PUTBACK;
	count = call_sv(env_ptr->fct, G_SCALAR);
	SPAGAIN;
	if (count != 1)
	    croak("Big trouble\n");
	ret = POPi;
	PUTBACK;
	FREETMPS;
	LEAVE;
	return ret;
    }
%}

%{
typedef void OGRLayerShadow;
%}
