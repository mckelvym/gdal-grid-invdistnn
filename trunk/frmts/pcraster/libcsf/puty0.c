/*
 * puty0.c
$Log$
Revision 1.1  2005/09/28 20:54:53  kdejong
Initial version of internal csf library code.

Revision 1.1.1.1  2000/01/04 21:04:57  cees
Initial import Cees

Revision 2.0  1996/05/23 13:16:26  cees
csf2clean

Revision 1.1  1996/05/23 13:11:49  cees
Initial revision

Revision 1.3  1995/11/01 17:23:03  cees
.

 * Revision 1.2  1994/09/08  17:16:23  cees
 * added c2man docs + small code changes
 *
 * Revision 1.1  1994/08/26  13:33:23  cees
 * Initial revision
 *
 */
#ifndef lint
 static const char *rcs_id = 
 "$Header$";
#endif


#include "csf.h"
#include "csfimpl.h"

/* change the y value of upper left co-ordinate
 * RputYUL changes the y value of upper left co-ordinate.
 * Whether this is the largest or smallest y value depends on the
 * projection (See MgetProjection()).
 * returns the new y value of upper left co-ordinate or 0
 * case of an error.
 *
 * Merrno
 * NOACCESS
 */
REAL8 RputYUL(
	MAP *map, /* map handle */
	REAL8 yUL) /* new y value of upper left co-ordinate */
{
	CHECKHANDLE_GOTO(map, error);
	if(! WRITE_ENABLE(map))
	{
		M_ERROR(NOACCESS);
		goto error;
	}
	map->raster.yUL = yUL;
	return(yUL);
error:  return(0);
}
