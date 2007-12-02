%import destroy.i

ALTERED_DESTROY(OGRDataSourceShadow, OGRc, delete_DataSource)
ALTERED_DESTROY(OGRFeatureShadow, OGRc, delete_Feature)
ALTERED_DESTROY(OGRFeatureDefnShadow, OGRc, delete_FeatureDefn)
ALTERED_DESTROY(OGRFieldDefnShadow, OGRc, delete_FieldDefn)
ALTERED_DESTROY(OGRGeometryShadow, OGRc, delete_Geometry)

%rename (GetDriverCount) OGRGetDriverCount;
%rename (GetOpenDSCount) OGRGetOpenDSCount;
%rename (SetGenerate_DB2_V72_BYTE_ORDER) OGRSetGenerate_DB2_V72_BYTE_ORDER;
%rename (RegisterAll) OGRRegisterAll();

%extend OGRGeometryShadow {

    %rename (AddPoint_3D) AddPoint;
    %rename (SetPoint_3D) SetPoint;
    %rename (GetPoint_3D) GetPoint;

}

%rename (_GetLayerByIndex) GetLayerByIndex;
%rename (_GetLayerByName) GetLayerByName;
%rename (_CreateLayer) CreateLayer;
%rename (_DeleteLayer) DeleteLayer;
%rename (_GetFieldType) GetFieldType;
%rename (_SetGeometryDirectly) SetGeometryDirectly;
%rename (_ExportToWkb) ExportToWkb;
%rename (_GetDriver) GetDriver;

