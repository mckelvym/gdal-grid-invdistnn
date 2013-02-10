
#include <stdio.h>
#include <time.h>
#include "NIDS.h"
#include "prod_info.h"

NIDS_prod_info prod_info[] = {
	{16, 1, "Base Reflectivity",
	 .54, .54, 1.0, 124, 8, "Radial Image"},
	{17, 1, "Base Reflectivity",
	 1.1, 1.1, 1.0, 248, 8, "Radial Image"},
	{18, 1, "Base Reflectivity",
	 2.2, 2.2, 1.0, 248, 8, "Radial Image"},
	{19, 1, "Base Reflectivity",
	 .54, .54, 1.0, 124, 16, "Radial Image"},
	{20, 1, "Base Reflectivity",
	 1.1, 1.1, 1.0, 248, 16, "Radial Image"},
	{21, 1, "Base Reflectivity",
	 2.2, 2.2, 2.0, 248, 16, "Radial Image"},
	{22, 2, "Base Velocity",
	 .13, .13, 1.0, 32, 8, "Radial Image"},
	{23, 2, "Base Velocity",
	 .27, .27, 1.0, 62, 8, "Radial Image"},
	{24, 2, "Base Velocity",
	 .54, .54, 1.0, 124, 8, "Radial Image"},
	{25, 2, "Base Velocity",
	 .13, .13, 1.0, 32, 16, "Radial Image"},
	{26, 2, "Base Velocity",
	 .27, .27, 1.0, 62, 16, "Radial Image"},
	{27, 2, "Base Velocity",
	 .54, .54, 1.0, 124, 16, "Radial Image"},
	{28, 3, "Base Spectrum Width",
	 .13, .13, 1.0, 32, 8, "Radial Image"},
	{29, 3, "Base Spectrum Width",
	 .27, .27, 1.0, 62, 8, "Radial Image"},
	{30, 3, "Base Spectrum Width",
	 .54, .54, 1.0, 124, 8, "Radial Image"},
	{31, 32, "User Selectable Storm Total",
	 1.1, 1.1, 1.0, 124, 16, "Radial Image/Geographic Alpha"},
	{41, 8, "Echo Tops  ",
		2.2, 2.2, 1.0, 124, 16, "Raster Image"},
	{55, 16, "Storm Relative Mean Radial Velocity",
	 .27, .27, 1.0, 27, 16, "Radial Image (Region)"},
	{56, 16, "Storm Relative Mean Radial Velocity",
	 .54, .54, 1.0, 124, 16, "Radial Image (Map)"},
	{-1, -1, "", -1, -1, -1, -1, -1, ""},
};
/*
 
                                         Table III. Message Codes for Products
CODE NTR PRODUCT NAME                      RESOLUTION                         RANGE DATA       MESSAGE FORMAT



                                                                                    LEVEL

 31  32  User Selectable Storm Total       1.1 x 1   Nmi x Deg                124   16         Radial
         Precipitation                                                                         Image/Geographic Alpha
32   33  Digital Hybrid Scan Reflectivity  .54 x 1   Nmi x Deg                124   256        Radial Image
33   33  Hybrid Scan Reflectivity          .54 x 1   Nmi x Deg                124   16         Radial Image
34   34  Clutter Filter Control            1 x 1.4 Km x Deg (Ver. 0)          124   8 (Ver. 0) Radial Image
                                           1 x 1.0 Km x Deg (Ver. 1)                4 (Ver. 1)
35   6   Composite Reflectivity            .54 x .54  Nmi x Nmi               124   8          Raster Image/Non-
                                                                                               geographic Alpha
36   6   Composite Reflectivity            2.2 x 2.2  Nmi x Nmi               248   8          Raster Image/Non-
                                                                                               geographic Alpha
37   6   Composite Reflectivity            .54 x .54  Nmi x Nmi               124   16         Raster Image/Non-
                                                                                               geographic Alpha
38   6   Composite Reflectivity            2.2 x 2.2  Nmi x Nmi               248   16         Raster Image/Non-
                                                                                               geographic Alpha
39
40
CODE NTR          PRODUCT NAME                                     RESOLUTION                                             RANGE                DATA          MESSAGE FORMAT
                                                                                                                                               LEVEL
41   8            Echo Tops                                        2.2 x 2.2 Nmi x Nmi                                     124                        16     Raster Image
42                Spare
43   10           Severe Weather (Reflectivity)                    .54 x 1 Nmi x Deg                                      27                   16 Ref        Radial Image
                                                                                                                                                             (Reflectivity)
44   10           Severe Weather (Velocity)                        .13 x 1 Nmi x Deg                                      27                   16 Vel        Radial Image (Velocity)
45   10           Severe Weather (Spectrum                         .13 x 1 Nmi x Deg                                      27                   8             Radial Image
                  Width)                                                                                                                                     (Spectrum Width)
46   10           Severe Weather (Shear)                           .27 x 1 Nmi x Deg                                      27                   16 Sh         Radial Image (Shear)
47   11           Severe Weather Probability                       2.2 x 2.2 Nmi x Nmi                                    124                  N/A           Geographic
                                                                                                                                                             Alphanumeric
48   12           VAD Wind Profile                                 5 Knots                                                N/A                  5             Non-geographic
                                                                                                                                                             Alphanumeric
49                Spare                                                                                                                        16            Raster Image/Non-
                                                                                                                                                             geographic
                                                                                                                                                             Alphanumeric
50   14           Cross Section (Reflectivity)                     .54 Horizontal x .27 Vert Nmi x Nmi                    124                  16            Raster Image
                                                                                                                                                             (Reflectivity)
51   14           Cross Section (Velocity)                         .54 Horizontal x .27 Vert Nmi x Nmi                    124                  16            Raster Image (Velocity)
52                Spare
53                Spare
54   --------------------------------------------------------------------Reserved---------------------------------------------------------------------------
55   16           Storm Relative Mean Radial                       .27 x 1 Nmi x Deg                                      27                   16            Radial Image (Region)
                  Velocity
56   16           Storm Relative Mean Radial                       .54 x 1 Nmi x Deg                                      124                  16            Radial Image (Map)
                  Velocity
57   17           Vertically Integrated Liquid                     2.2 x 2.2 Nmi x Nmi                                    124                  16            Raster Image
58   18           Storm Tracking Information                       N/A                                                    248                  N/A           Geographic and Non-
                                                                                                                                                             geographic Alpha
59   19           Hail Index                                       N/A                                                    124                  N/A           Geographic and Non-
                                                                                                                                                             geographic Alpha
60   20           Mesocyclone                                      N/A                                                    124                  N/A
61 21           Tornado Vortex Signature                         N/A                                           124                   N/A                Geographic and Non-
                                                                                                                                                        geographic
                                                                                                                                                        Alphanumeric
62 22           Storm Structure                                  N/A                                            248                  N/A                Alphanumeric
63 23           Layer Composite Reflectivity                     2.2 x 2.2 Nmi x Nmi                           124                   8 Avg              Raster Image (Layer 1
                                                                                                                                                        Average)
64 23           Layer Composite Reflectivity                     2.2 x 2.2 Nmi x Nmi                           124                   8 Avg              Raster Image (Layer 2
                                                                                                                                                        Average)
65 23           Layer Composite Reflectivity                     2.2 x 2.2 Nmi x Nmi                           124                   8 Max              Raster Image (Layer 1
                                                                                                                                                        Maximum)
66 23           Layer Composite Reflectivity                     2.2 x 2.2 Nmi x Nmi                           124                   8 Max              Raster Image (Layer 2
                                                                                                                                                        Maximum)
67 23           Layer Composite Reflectivity -                   2.2 x 2.2 Nmi x Nmi                           124                   8 Max              Raster Image
                AP Removed
68              Spare
69              Spare
70              Spare
71              Spare
72              Spare
73 25           User Alert Message                               N/A                                           N/A                   N/A                Alphanumeric
74 26           Radar Coded Message                              1/16 LFM                                        248                        9           Alphanumeric
75 27           CPLFree Text Message                                N/A                                             N/A                 N/A                Alphanumeric
76 -------------------------------------------------------------Reserved for internal PUP use ----------------------------------------------------------------------------------
78 28           Surface Rainfall Accum. (1 hr)                   1.1 x 1 Nmi x Deg                             124                   16                 Radial Image
79 28           Surface Rainfall Accum. (3 hr)                   1.1 x 1 Nmi x Deg                             124                   16                 Radial Image
80 29           Storm Total Rainfall                             1.1 x 1 Nmi x Deg                             124                   16                 Radial Image
                Accumulation
81 30           Hourly Digital Precipitation                     1/40 LFM                                      124                   256/8              Raster Image /
                Array                                                                                                                                   Alphanumeric
82 31           Supplemental Precipitation                       N/A                                           N/A                   N/A                Alphanumeric
                Data
83              Spare                                                                                                                9
84 12           Velocity Azimuth Display                         5 Knots                                       N/A                   8                  Non-geographic
                                                                                                                                                        Alphanumeric
                                                                                3
CODE  NTR PRODUCT NAME                  RESOLUTION           RANGE        DATA  MESSAGE FORMAT
                                                                          LEVEL
85    14  Cross Section Reflectivity    .54 Horizontal       124          8     Raster Image
                                        x .27 Vert Nmi x Nmi                    (Reflectivity)
86    14  Cross Section Velocity        .54 Horizontal       124          8     Raster Image (Velocity)
                                        x .27 Vert Nmi x Nmi
87    4   Combined Shear                Adaptable Nmi x Nmi  62           16    Raster Image
88        Spare
89    23  Layer Composite Reflectivity  2.2 x 2.2 Nmi x Nmi  124          8 Avg raster Image - Layer 3
                                                                                Average
90    23  Layer Composite Reflectivity  2.2 x 2.2 Nmi x Nmi  124          8 Max Raster Image - Layer 3
                                                                                Maximum
91-92     Reserved for internal PUP and
          RPG Use
93    35  ITWS Digital Base Velocity    .54 x 1 Nmi x Deg    Lesser of 62 256   Radial Image
                                                             Nmi or
                                                             18Kft AGL
94    1   Base Reflectivity Data Array  .54 x 1 Nmi x Deg    248          256   Radial Image
95    6   Composite Reflectivity Edited .54x.54 Nmi x Nmi    124          8     Raster Image/Non-
          for AP                                                                geographic Alpha
96    6   Composite Reflectivity Edited 2.2 x 2.2 Nmi x Nmi  248          8     Raster Image/Non-
          for AP                                                                geographic alpha
97    6   Composite Reflectivity Edited .54 x .54 Nmi x Nmi  124          16    Raster Image/Non-
          for AP                                                                geographic Alpha
98    6   Composite Reflectivity Edited 2.2 x 2.2 Nmi x Nmi  248          16    Raster Image/Non-
          for AP                                                                geographic Alpha
99    2   Base Velocity Data Array      .13 x 1 Nmi x Deg    124          256   Radial Image
100       Site Adaptable parameters for
          VAD Wind Profile (Product 48)
101       Storm Track Alphanumeric
          Block
102       Hail Index Alphanumeric Block
103       Mesocyclone Alphanumeric
          Block
104       TVS Alphanumeric Block
105       Site Adaptable Parameters for
CODE    NTR PRODUCT NAME                    RESOLUTION                        RANGE      DATA  MESSAGE FORMAT
                                                                                         LEVEL
            Combined Shear
106         Spare
107         Surface Rainfall (1 hr)
            Alphanumeric Block
108         Surface Rainfall (3 hr)
            Alphanumeric Block
109         Storm Total Accumulation
            Alphanumeric Block
110         Clutter Likelihood Reflectivity
            Alphanumeric Block
111         Clutter Likelihood Doppler
            Alphanumeric Block
112-131     Reserved for Future Products
132     36  Clutter Likelihood Reflectivity .54 x 1 Nmi. x Deg                124        11    Radial Image
133     37  Clutter Likelihood Doppler      .54 x 1 Nmi. x Deg                124        12    Radial Image
134     39  High Resolution VIL             .54 x 1 Nmi x Deg                 248        256   Radial Image
135     41  Enhanced Echo Tops              .54 x 1 Nmi x Deg                 186        199   Radial Image
136     38  SuperOb                         Adaptable, default = 5 km x 6 deg Adaptable, N/A   Latitude, Longitude
                                                                              default =        (ICD packet code 27)
                                                                              100 km
137     40  User                            0.54 Nmi x1Deg                    124 nmi    16    Radial
            Selectable                                                                         image
            Layer
            Composite
            Reflectivity
138     29  Digital Storm Total             1.1Nmi x 1Deg                     124        256   Radial Image
            Precipitation
139     20  Mesocyclone Rapid Update        N/A                               124        N/A   Geographic and Non-
                                                                                               geographic Alpha
140     46  Gust Front MIGFA                N/A                               38         N/A   Generic Data Format
141     20  Mesocyclone Detection           N/A                               124        N/A   Geographic and Non-
                                                                                               geographic Alpha
143     21  Tornado Vortex Signature Rapid  N/A                               124        N/A   Geographic and Non-
            Update                                                                             geographic
CODE      NTR       PRODUCT NAME                       RESOLUTION                               RANGE           DATA  MESSAGE FORMAT
                                                                                                                LEVEL
                                                                                                                      Alphanumeric
144       42        One-hour Snow Water                0.54 x 1 Nmi x Deg                       124                   Radial Image
                    Equivalent
145       42        One-hour Snow Depth                0.54 x 1 Nmi x Deg                       124
146       43        Storm Total Snow Water             0.54 x 1 Nmi x Deg                       124
                    Equivalent
147       43        Storm Total Snow Depth             0.54 x 1 Nmi x Deg                       124
149       20        Digital Mesocyclone Detection      N/A                                      124
150       44        User Selectable Snow Water         0.54 x 1 Nmi x Deg                       124
                    Equivalent
151       44        User Selectable Snow Depth         0.54 x 1 Nmi x Deg                       124
152                 Archive III Status Product
153-160             Reserved for Future Products
161-170             Reserved for Future Products
171-180             Reserved for Future Products
181-190             Reserved for Future Products
191-200             Reserved for Future Products
201-210             Reserved for Future Products
211-220             Reserved for Future Products
221-230             Reserved for Future Products
231-240             Reserved for Future Products
241-250             Reserved for Future Products
251-260             Reserved for Future Products
261-270             Reserved for Future Products
271-280             Reserved for Future Products
281-290             Reserved for Future Products
291-296             Reserved for Internal RPG Use.
297-299             Reserved for Internal RPG use
Note: For all message codes for products: Units is N/A, Range is 0 to value shown and Accuracy/Precision is 1.1
*/

NIDS_prod_info *get_prod_info(int code) {
	NIDS_prod_info *info;
	
	for (info = prod_info ; code != info->code && info->code > 0 ; info++) {
	}
	
	return info;
}
