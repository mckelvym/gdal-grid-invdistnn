/******************************************************************************
 * $Id$
 *
 * Project:  ASI CEOS Translator
 * Purpose:  Link list function replacements.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Atlantis Scientific Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.2.2.1  2003/03/10 18:34:39  gwalter
 * Bring branch up to date.
 *
 * Revision 1.3  2003/02/28 18:45:20  gpotts
 * Prefixed CreateLink with ceos2 since there are multiple symbol conflict on static build under mac. Garrett Potts (gpotts@imagelinks.com)
 *
 * Revision 1.2  2001/07/18 04:51:56  warmerda
 * added CPL_CVSID
 *
 * Revision 1.1  2000/03/31 13:35:03  warmerda
 * New
 *
 */

#include "ceos.h"

CPL_CVSID("$Id$");


/************************************************************************/
/*                             ceos2CreateLink()                             */
/************************************************************************/

Link_t *ceos2CreateLink( void *pObject )

{
    Link_t	*psLink = (Link_t *) CPLCalloc(sizeof(Link_t),1);

    psLink->object = pObject;

    return psLink;
}

/************************************************************************/
/*                            DestroyList()                             */
/************************************************************************/

void DestroyList( Link_t * psList )

{
    while( psList != NULL )
    {
        Link_t	*psNext = psList->next;

        CPLFree( psList );
        psList = psNext;
    }
}

/************************************************************************/
/*                             InsertLink()                             */
/************************************************************************/

Link_t *InsertLink( Link_t *psList, Link_t *psLink )

{
    psLink->next = psList;

    return psLink;
}

/************************************************************************/
/*                              AddLink()                               */
/************************************************************************/

Link_t *AddLink( Link_t *psList, Link_t *psLink )

{
    Link_t	*psNode;

    if( psList == NULL )
        return psLink;

    for( psNode = psList; psNode->next != NULL; psNode = psNode->next ) {}

    psNode->next = psLink;

    return psList;
}
