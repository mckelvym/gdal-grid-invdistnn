/**********************************************************************
 * $Id$
 *
 * Project:  CPL - Common Portability Library
 * Purpose:  Declarations for MiniXML Handler.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 **********************************************************************
 * Copyright (c) 2001, Frank Warmerdam
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log$
 * Revision 1.13  2005/05/13 18:17:48  fwarmerdam
 * added CPLRemoveXMLChild
 *
 * Revision 1.12  2005/03/09 17:07:25  fwarmerdam
 * added CPLSearchXMLNode
 *
 * Revision 1.11  2004/01/29 15:29:28  warmerda
 * Added CPLCleanXMLElementName
 *
 * Revision 1.10  2003/12/04 15:46:51  warmerda
 * Added CPLAddXMLSibling()
 *
 * Revision 1.9  2003/11/05 20:14:21  warmerda
 * added lots of documentation
 *
 * Revision 1.8  2003/03/27 18:12:41  warmerda
 * Added NULL pszNameSpace support in namespace stripper (all namespaces).
 * Added XML file read/write functions.
 *
 * Revision 1.7  2003/03/24 16:46:48  warmerda
 * added CPLStripXMLNamespace
 *
 * Revision 1.6  2002/11/16 20:38:34  warmerda
 * added support for literals like DOCTYPE
 *
 * Revision 1.5  2002/05/24 04:09:10  warmerda
 * added clone and SetXMLValue functions
 *
 * Revision 1.4  2002/03/05 14:26:57  warmerda
 * expanded tabs
 *
 * Revision 1.3  2002/01/23 20:45:06  warmerda
 * handle <?...?> and comment elements
 *
 * Revision 1.2  2001/12/06 18:13:49  warmerda
 * added CPLAddXMLChild and CPLCreateElmentAndValue
 *
 * Revision 1.1  2001/11/16 15:39:48  warmerda
 * New
 *
 **********************************************************************/

#ifndef _CPL_MINIXML_H_INCLUDED
#define _CPL_MINIXML_H_INCLUDED

#include "cpl_port.h"

/**
 * \file cpl_minixml.h
 *
 * Definitions for CPL mini XML Parser/Serializer.
 */

CPL_C_START

typedef enum 
{
    /*! Node is an element */           CXT_Element = 0,
    /*! Node is a raw text value */     CXT_Text = 1,
    /*! Node is attribute */            CXT_Attribute = 2,  
    /*! Node is an XML comment. */      CXT_Comment = 3,    
    /*! Node is a special literal */    CXT_Literal = 4     
} CPLXMLNodeType;

/**
 * Document node structure. 
 *
 * This C structure is used to hold a single text fragment representing a 
 * component of the document when parsed.   It should be allocated with the
 * appropriate CPL function, and freed with CPLDestroyXMLNode().  The structure
 * contents should not normally be altered by application code, but may be
 * freely examined by application code.
 *
 * Using the psChild and psNext pointers, a heirarchical tree structure
 * for a document can be represented as a tree of CPLXMLNode structures. 
 */

typedef struct _CPLXMLNode
{
    /** 
     * Node type
     *
     * One of CXT_Element, CXT_Text, CXT_Attribute, CXT_Comment, 
     * or CXT_Literal.
     */
    CPLXMLNodeType      eType;       
    
    /** 
     * Node value
     *
     * For CXT_Element this is the name of the element, without the angle
     * brackets.  Note there is a single CXT_Element even when the document
     * contains a start and end element tag.  The node represents the pair. 
     * All text or other elements between the start and end tag will appear
     * as children nodes of this CXT_Element node. 
     *
     * For CXT_Attribute the pszValue is the attribute name.  The value of 
     * the attribute will be a CXT_Text child. 
     *
     * For CXT_Text this is the text itself (value of an attribute, or a 
     * text fragment between an element start and end tags. 
     *
     * For CXT_Literal it is all the literal text.  Currently this is just
     * used for !DOCTYPE lines, and the value would be the entire line. 
     *
     * For CXT_Comment the value is all the literal text within the comment, 
     * but not including the comment start/end indicators ("<--" and "-->"). 
     */
    char                *pszValue;   

    /** 
     * Next sibling. 
     *
     * Pointer to next sibling, that is the next node appearing after this
     * one that has the same parent as this node.  NULL if this node is the
     * last child of the parent element. 
     */
    struct _CPLXMLNode  *psNext;     

    /** 
     * Child node.
     *
     * Pointer to first child node, if any.  Only CXT_Element and CXT_Attribute
     * nodes should have children.  For CXT_Attribute it should be a single
     * CXT_Text value node, while CXT_Attribute can have any kind of child. 
     * The full list of children for a node are identified by walking the 
     * psNext's starting with the psChild node. 
     */

    struct _CPLXMLNode  *psChild;    
} CPLXMLNode;


CPLXMLNode CPL_DLL *CPLParseXMLString( const char * );
void       CPL_DLL  CPLDestroyXMLNode( CPLXMLNode * );
CPLXMLNode CPL_DLL *CPLGetXMLNode( CPLXMLNode *poRoot, 
                                   const char *pszPath );
CPLXMLNode CPL_DLL *CPLSearchXMLNode( CPLXMLNode *poRoot, 
                                      const char *pszTarget );
const char CPL_DLL *CPLGetXMLValue( CPLXMLNode *poRoot, 
                                    const char *pszPath, 
                                    const char *pszDefault );
CPLXMLNode CPL_DLL *CPLCreateXMLNode( CPLXMLNode *poParent, 
                                      CPLXMLNodeType eType,
                                      const char *pszText );
char       CPL_DLL *CPLSerializeXMLTree( CPLXMLNode *psNode );
void       CPL_DLL  CPLAddXMLChild( CPLXMLNode *psParent,
                                    CPLXMLNode *psChild );
int        CPL_DLL  CPLRemoveXMLChild( CPLXMLNode *psParent,
                                       CPLXMLNode *psChild );
void       CPL_DLL  CPLAddXMLSibling( CPLXMLNode *psOlderSibling,
                                      CPLXMLNode *psNewSibling );
CPLXMLNode CPL_DLL *CPLCreateXMLElementAndValue( CPLXMLNode *psParent,
                                                 const char *pszName,
                                                 const char *pszValue );
CPLXMLNode CPL_DLL *CPLCloneXMLTree( CPLXMLNode *psTree );
int        CPL_DLL CPLSetXMLValue( CPLXMLNode *psRoot,  const char *pszPath,
                                   const char *pszValue );
void       CPL_DLL CPLStripXMLNamespace( CPLXMLNode *psRoot, 
                                         const char *pszNameSpace, 
                                         int bRecurse );
void       CPL_DLL CPLCleanXMLElementName( char * );

CPLXMLNode CPL_DLL *CPLParseXMLFile( const char *pszFilename );
int        CPL_DLL CPLSerializeXMLTreeToFile( CPLXMLNode *psTree, 
                                              const char *pszFilename );

CPL_C_END

#endif /* _CPL_MINIXML_H_INCLUDED */
