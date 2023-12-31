/******************************************************************************
 * $Id$
 *
 * Project:  Interlis 2 Reader
 * Purpose:  Implementation of ILI2Handler class.
 * Author:   Markus Schnider, Sourcepole AG
 *
 ******************************************************************************
 * Copyright (c) 2004, Pirmin Kalberer, Sourcepole AG
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
 ****************************************************************************/

#include "ogr_ili2.h"
#include "cpl_conv.h"
#include "cpl_string.h"

#include "ili2readerp.h"
#include <xercesc/sax2/Attributes.hpp>

CPL_CVSID("$Id$");

// 
// constants
// 
static const char* ILI2_DATASECTION = "DATASECTION";

// 
// ILI2Handler
// 
ILI2Handler::ILI2Handler( ILI2Reader *poReader ) {
  m_poReader = poReader;
  
  // initialize once
  static int ili2DomTreeInitialized = FALSE;
  
  if (!ili2DomTreeInitialized) {
    XMLCh *tmpCh = XMLString::transcode("CORE");
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(tmpCh);
    delete [] tmpCh;
    
    // the root element
    tmpCh = XMLString::transcode("ROOT");
    dom_doc = impl->createDocument(0,tmpCh,0);
    delete [] tmpCh;
  
    // the first element is root
    dom_elem = dom_doc->getDocumentElement();
    
    ili2DomTreeInitialized = TRUE;
  }
}

ILI2Handler::~ILI2Handler() {
  
  // remove all elements
  DOMNode *tmpNode = dom_doc->getFirstChild();
  while (tmpNode != NULL) {
    tmpNode = dom_doc->removeChild(tmpNode);
    tmpNode = dom_doc->getFirstChild();
  }
  
  // release the dom tree
  dom_doc->release();
    
}

    
void ILI2Handler::startDocument() {
  // the level counter starts with DATASECTION
  level = -1;
}

void ILI2Handler::endDocument() {
  // nothing to do
}
    
void ILI2Handler::startElement(
        const   XMLCh* const    uri,
        const   XMLCh* const    localname,
        const   XMLCh* const    qname,
        const   Attributes& attrs
    ) {
  
  // start to add the layers, features with the DATASECTION  
  if ((level >= 0) || (cmpStr(ILI2_DATASECTION, XMLString::transcode(qname)) == 0)) {
    level++;
    
    if (level >= 2) { 
      
      // create the dom tree
      DOMElement *elem = (DOMElement*)dom_doc->createElement(qname);
      
      // add all attributes
      unsigned int len = attrs.getLength();
      for (unsigned int index = 0; index < len; index++)
        elem->setAttribute(attrs.getQName(index), attrs.getValue(index));
      dom_elem->appendChild(elem);
      dom_elem = elem;
    }
  }
}

void ILI2Handler::endElement(
        const   XMLCh* const    uri,
        const   XMLCh* const    localname,
        const   XMLCh* const    qname
    ) {
    
  if (level >= 0) {
    if (level == 2) {
    
      // go to the parent element and parse the child element
      DOMElement* childElem = dom_elem;
      dom_elem = (DOMElement*)dom_elem->getParentNode();
       
      m_poReader->AddFeature(childElem);
      
      // remove the child element
      childElem = (DOMElement*)dom_elem->removeChild(childElem);
    } else if (level >= 3) {
    
      // go to the parent element
      dom_elem = (DOMElement*)dom_elem->getParentNode();
    }
    level--;
  }
}

void ILI2Handler::characters( const XMLCh *const chars,
                     const unsigned int length ) {
  
  // add the text element
  if (level >= 3) {
    std::string tmpCh = XMLString::transcode(chars);
    
    // only add the text if it is not empty
    if (trim(tmpCh) != "") 
      dom_elem->appendChild(dom_doc->createTextNode(chars));
  }
}

void ILI2Handler::fatalError(const SAXParseException&) {
  // FIXME Error handling
}
