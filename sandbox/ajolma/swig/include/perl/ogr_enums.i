typedef int OGRwkbByteOrder;
typedef int OGRwkbGeometryType;
typedef int OGRFieldType;
typedef int OGRJustification;

%constant wkb25Bit = wkb25DBit;
%constant wkbUnknown = 0;

%constant wkbPoint = 1;
%constant wkbLineString = 2;
%constant wkbPolygon = 3;
%constant wkbMultiPoint = 4;
%constant wkbMultiLineString = 5;
%constant wkbMultiPolygon = 6;
%constant wkbGeometryCollection = 7;
%constant wkbNone = 100;
%constant wkbLinearRing = 101;
%constant wkbPoint25D =              wkbPoint              + wkb25DBit;
%constant wkbLineString25D =         wkbLineString         + wkb25DBit;
%constant wkbPolygon25D =            wkbPolygon            + wkb25DBit;
%constant wkbMultiPoint25D =         wkbMultiPoint         + wkb25DBit;
%constant wkbMultiLineString25D =    wkbMultiLineString    + wkb25DBit;
%constant wkbMultiPolygon25D =       wkbMultiPolygon       + wkb25DBit;
%constant wkbGeometryCollection25D = wkbGeometryCollection + wkb25DBit;

%constant OFTInteger = 0;
%constant OFTIntegerList= 1;
%constant OFTReal = 2;
%constant OFTRealList = 3;
%constant OFTString = 4;
%constant OFTStringList = 5;
%constant OFTWideString = 6;
%constant OFTWideStringList = 7;
%constant OFTBinary = 8;
%constant OFTDate = 9;
%constant OFTTime = 10;
%constant OFTDateTime = 11;

%constant OJUndefined = 0;
%constant OJLeft = 1;
%constant OJRight = 2;

%constant wkbXDR = 0;
%constant wkbNDR = 1;

%constant char *OLCRandomRead          = "RandomRead";
%constant char *OLCSequentialWrite     = "SequentialWrite";
%constant char *OLCRandomWrite         = "RandomWrite";
%constant char *OLCFastSpatialFilter   = "FastSpatialFilter";
%constant char *OLCFastFeatureCount    = "FastFeatureCount";
%constant char *OLCFastGetExtent       = "FastGetExtent";
%constant char *OLCCreateField         = "CreateField";
%constant char *OLCTransactions        = "Transactions";
%constant char *OLCDeleteFeature       = "DeleteFeature";
%constant char *OLCFastSetNextByIndex  = "FastSetNextByIndex";

%constant char *ODsCCreateLayer        = "CreateLayer";
%constant char *ODsCDeleteLayer        = "DeleteLayer";

%constant char *ODrCCreateDataSource   = "CreateDataSource";
%constant char *ODrCDeleteDataSource   = "DeleteDataSource";

%inline %{
    void OGRGeometryShadow_Move(OGRGeometryShadow *g, double dx, double dy, double dz) {
	int n = OGR_G_GetGeometryCount(g);
	if (n > 0) {
	    int i;
	    for (i = 0; i < n; i++) {
		OGRGeometryShadow *g = (OGRGeometryShadow*)OGR_G_GetGeometryRef(g, i);
		OGRGeometryShadow_Move(g, dx, dy, dz);
	    }
	} else {
	    int i;
	    for (i = 0; i < OGR_G_GetPointCount(g); i++) {
		double x = OGR_G_GetX(g, i);
		double y = OGR_G_GetY(g, i);
		double z = OGR_G_GetZ(g, i);
		OGR_G_SetPoint(g, i, x+dx, y+dy, z+dz);
	    }
	}
    }
%}
