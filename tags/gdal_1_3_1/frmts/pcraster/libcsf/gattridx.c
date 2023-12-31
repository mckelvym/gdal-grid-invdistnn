/*
 * gattridx.c
$Log$
Revision 1.2  2005/10/03 07:22:12  kdejong
Lots of small edits for x86-64 support, removed rcs id string.

Revision 1.1.1.1  2000/01/04 21:04:37  cees
Initial import Cees

Revision 2.0  1996/05/23 13:16:26  cees
csf2clean

Revision 1.1  1996/05/23 13:11:49  cees
Initial revision

Revision 1.3  1995/11/01 17:23:03  cees
.

 * Revision 1.2  1994/09/06  13:27:31  cees
 * const'fied
 * added c2man docs
 *
 * Revision 1.1  1994/08/26  13:33:23  cees
 * Initial revision
 *
 */
#include "csf.h"
#include "csfimpl.h"

/* search attribute index in block (LIBRARY_INTERNAL)
 * returns index in block where id is found, NR_ATTR_IN_BLOCK if not found 
 */
int CsfGetAttrIndex(
	CSF_ATTR_ID id,  /* id to be found */
	const ATTR_CNTRL_BLOCK *b) /* block to inspect */
{
	int i = 0;

	while(i < NR_ATTR_IN_BLOCK)
	{
		if (b->attrs[i].attrId == id) 
			break;
		i++;
	}
	return(i);
}
