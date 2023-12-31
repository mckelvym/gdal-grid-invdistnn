/*
 * pvalscal.c
$Log$
Revision 1.2  2005/10/03 07:22:12  kdejong
Lots of small edits for x86-64 support, removed rcs id string.

Revision 1.1.1.1  2000/01/04 21:04:57  cees
Initial import Cees

Revision 2.1  1996/12/29 19:35:21  cees
src tree clean up

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
#include "csf.h"
#include "csfimpl.h"

/* put new value scale
 * RputValueScale changes the value scale
 * of the map.
 *
 * returns
 *  new value scale or VS_UNDEFINED in case of an error.
 * 
 * NOTE
 * Note that there is no check if the cell representation
 * is complaint.
 *
 * Merrno
 * NOACCESS
 */
CSF_VS RputValueScale(
	MAP *map,         /* map handle */
	CSF_VS valueScale) /* new value scale */
{
	CHECKHANDLE_GOTO(map, error);
	if(! WRITE_ENABLE(map))
	{
		 M_ERROR(NOACCESS);
		 goto error;
	}
	map->raster.valueScale = valueScale;
	return(valueScale);
error:	return(VS_UNDEFINED);
}
