%rename (ColorEntry) GDALColorEntry;
typedef struct
{
    /*! gray, red, cyan or hue */
    short      c1;      
    /*! green, magenta, or lightness */    
    short      c2;      
    /*! blue, yellow, or saturation */
    short      c3;      
    /*! alpha or blackband */
    short      c4;      
} GDALColorEntry;
