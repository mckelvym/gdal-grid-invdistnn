/*
 * $Id$
 *
 * python specific code for ogr bindings.
 */

/*
 * $Log$
 * Revision 1.2  2005/09/02 21:42:42  kruland
 * The compactdefaultargs feature should be turned on for all bindings not just
 * python.
 *
 * Revision 1.1  2005/09/02 16:19:23  kruland
 * Major reorganization to accomodate multiple language bindings.
 * Each language binding can define renames and supplemental code without
 * having to have a lot of conditionals in the main interface definition files.
 *
 */

%feature("autodoc");

%{
static PyObject *
py_OPTGetProjectionMethods(PyObject *self, PyObject *args) {

    PyObject *py_MList;
    char     **papszMethods;
    int      iMethod;
    
    self = self;
    args = args;

    papszMethods = OPTGetProjectionMethods();
    py_MList = PyList_New(CSLCount(papszMethods));

    for( iMethod = 0; papszMethods[iMethod] != NULL; iMethod++ )
    {
	char    *pszUserMethodName;
	char    **papszParameters;
	PyObject *py_PList;
	int       iParam;

	papszParameters = OPTGetParameterList( papszMethods[iMethod], 
					       &pszUserMethodName );
        if( papszParameters == NULL )
            return NULL;

	py_PList = PyList_New(CSLCount(papszParameters));
	for( iParam = 0; papszParameters[iParam] != NULL; iParam++ )
       	{
	    char    *pszType;
	    char    *pszUserParamName;
            double  dfDefault;

	    OPTGetParameterInfo( papszMethods[iMethod], 
				 papszParameters[iParam], 
				 &pszUserParamName, 
				 &pszType, &dfDefault );
	    PyList_SetItem(py_PList, iParam, 
			   Py_BuildValue("(sssd)", 
					 papszParameters[iParam], 
					 pszUserParamName, 
                                         pszType, dfDefault ));
	}
	
	CSLDestroy( papszParameters );

	PyList_SetItem(py_MList, iMethod, 
		       Py_BuildValue("(ssO)", 
		                     papszMethods[iMethod], 
				     pszUserMethodName, 
		                     py_PList));
    }

    CSLDestroy( papszMethods );

    return py_MList;
}
%}
%native(GetProjectionMethods) py_OPTGetProjectionMethods;

%include typemaps_python.i
