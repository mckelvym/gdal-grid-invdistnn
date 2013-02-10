
OBJ	=   arrow.o\
        color.o\
        d_radial.o\
        graphic_alphanumeric.o\
        hail_probable.o\
        nidsdataset.o\
        myzlib.o\
        point_feature.o\
        prod_info.o\
        raster.o\
        text.o\
        v_text.o\
        barb.o\
        compression.o\
        etvs.o\
        hail.o\
        image.o\
        mesocyclone.o\
        nidsdataset.o\
        precip.o\
        product_dependent_desc.o\
        radial.o\
        storm_id.o\
        tvs.o\
        v_vector.o\
        circle.o\
        d_precip.o\
        forecast.o\
        hail_positive.o\
        linked_vector.o\
        msg_header.o\
        point.o\
        prod_desc.o\
        product_symbology.o\
        radial2raster.o\
        tabular_alphanumeric.o\
        v_linked_vector.o\
        vector.o
        

include ../../GDALmake.opt

CPPFLAGS	:=	$(GDAL_INCLUDE) $(CPPFLAGS)

default:	$(OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

install-obj:	$(O_OBJ:.o=.$(OBJ_EXT))
