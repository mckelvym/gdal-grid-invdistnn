%rename (GCP) GDAL_GCP;
%rename (GCPsToGeoTransform) GDALGCPsToGeoTransform;
%rename (VersionInfo) GDALVersionInfo;
%rename (AllRegister) GDALAllRegister;
%rename (GetCacheMax) GDALGetCacheMax;
%rename (SetCacheMax) GDALSetCacheMax;
%rename (GetCacheUsed) GDALGetCacheUsed;
%rename (GetDataTypeSize) GDALGetDataTypeSize;
%rename (DataTypeIsComplex) GDALDataTypeIsComplex;
%rename (GCPsToGeoTransform) GDALGCPsToGeoTransform;
%rename (GetDataTypeName) GDALGetDataTypeName;
%rename (GetDataTypeByName) GDALGetDataTypeByName;
%rename (GetColorInterpretationName) GDALGetColorInterpretationName;
%rename (GetPaletteInterpretationName) GDALGetPaletteInterpretationName;
%rename (DecToDMS) GDALDecToDMS;
%rename (PackedDMSToDec) GDALPackedDMSToDec;
%rename (DecToPackedDMS) GDALDecToPackedDMS;
%rename (ParseXMLString) CPLParseXMLString;
%rename (SerializeXMLTree) CPLSerializeXMLTree;

%rename (GetMetadata) GetMetadata_Dict;
%ignore GetMetadata_List;

%rename (_GetDataTypeSize) GetDataTypeSize;
%rename (_DataTypeIsComplex) DataTypeIsComplex;
%rename (_GetDriver) GetDriver;
%rename (_Open) Open;
%newobject _Open;
%rename (_OpenShared) OpenShared;
%newobject _OpenShared;
%rename (_ComputeMedianCutPCT) ComputeMedianCutPCT;
%rename (_DitherRGB2PCT) DitherRGB2PCT;
%rename (_ReprojectImage) ReprojectImage;
%rename (_AutoCreateWarpedVRT) AutoCreateWarpedVRT;
%newobject _AutoCreateWarpedVRT;
%rename (_Create) Create;
%newobject _Create;
%rename (_GetRasterBand) GetRasterBand;
%rename (_AddBand) AddBand;
%rename (_GetPaletteInterpretation) GetPaletteInterpretation;
%rename (_SetColorEntry) SetColorEntry;
%rename (_GetUsageOfCol) GetUsageOfCol;
%rename (_GetColOfUsage) GetColOfUsage;
%rename (_GetTypeOfCol) GetTypeOfCol;
%rename (_CreateColumn) CreateColumn;

%import destroy.i

/*ALTERED_DESTROY(GDALRasterBandShadow, GDALc, delete_Band) !does not work! */
ALTERED_DESTROY(GDALColorTableShadow, GDALc, delete_ColorTable)
ALTERED_DESTROY(GDALConstShadow, GDALc, delete_Const)
ALTERED_DESTROY(GDALDatasetShadow, GDALc, delete_Dataset)
ALTERED_DESTROY(GDALDriverShadow, GDALc, delete_Driver)
ALTERED_DESTROY(GDAL_GCP, GDALc, delete_GCP)
ALTERED_DESTROY(GDALMajorObjectShadow, GDALc, delete_MajorObject)
ALTERED_DESTROY(GDALRasterAttributeTableShadow, GDALc, delete_RasterAttributeTable)
