/**********************************************************************
 *
 *  geo_free.c  -- Public routines for GEOTIFF GeoKey access.
 *
 *    Written By: Niles D. Ritter.
 *
 *  copyright (c) 1995   Niles D. Ritter
 *
 *  Permission granted to use this software, so long as this copyright
 *  notice accompanies any products derived therefrom.
 *
 **********************************************************************/

#include "geotiff.h"   /* public interface        */
#include "geo_tiffp.h" /* external TIFF interface */
#include "geo_keyp.h"  /* private interface       */


/**********************************************************************
 *
 *                        Public Routines
 *
 **********************************************************************/


void GTIFFree(GTIF* gtif)
{
	if (!gtif) return;
	
	/* Free parameter arrays */
	if (gtif->gt_ascii) _GTIFFree (gtif->gt_ascii);
	if (gtif->gt_double) _GTIFFree (gtif->gt_double);
	if (gtif->gt_short) _GTIFFree (gtif->gt_short);
	
	/* Free GeoKey arrays */
	if (gtif->gt_keys) _GTIFFree (gtif->gt_keys);
	if (gtif->gt_keyindex) _GTIFFree (gtif->gt_keyindex);
	
	_GTIFFree (gtif);
}
