/******************************************************************************
 *
 * Project:  KML Translator
 * Purpose:  Implements OGRLIBKMLDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2010, Brian Case
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
 *****************************************************************************/

#include <ogr_featurestyle.h>

#include <kml/dom.h>

using kmldom::KmlFactory;
using kmldom::DocumentPtr;
using kmldom::StylePtr;
using kmldom::IconStylePtr;
using kmldom::PolyStylePtr;
using kmldom::LineStylePtr;
using kmldom::LabelStylePtr;

#include "ogrlibkmlstyle.h"
#include "ogrlibkmllayerstyle.h"

void styletable2kml(
	OGRStyleTable *poOgrStyleTable,
	KmlFactory *poKmlFactory,
  DocumentPtr poKmlDocument)
{
	
  /***** parse the style table *****/
		
	poOgrStyleTable->ResetStyleStringReading();
	const char *pszStyleString;
	while ((pszStyleString = poOgrStyleTable->GetNextStyle())) {
			const char *pszStyleName = poOgrStyleTable->GetLastStyleName();
			
		if (*pszStyleString == '@')
			fprintf(stderr, "Warning: kml does not support url's at the layer level\n");
			
		/* we would actualy have to parse the dataset style and combine
		   the 2 styles into 1. only one linestyle etc.. per style aswell */
			
		/* but this is not practical as the style.kml can't be swaped out later */
			
		else {
				
			/***** add the style header to the kml *****/

      StylePtr poKmlStyle = poKmlFactory->CreateStyle();

      poKmlStyle->set_id(pszStyleName + 1);
        
			/***** create and init a style mamager with the style string *****/
				
			OGRStyleMgr *poOgrSM = new OGRStyleMgr;
  		poOgrSM->InitStyleString(pszStyleString);;
				
			/***** loop though the style parts *****/
				
			int i;
			for(i = 0; i < poOgrSM->GetPartCount(NULL) ; i++) {
				OGRStyleTool *poOgrST = poOgrSM->GetPart(i, NULL);
					
			  switch(poOgrST->GetType()) {
				  case OGRSTCPen:
          {
            LineStylePtr poKmlLineStyle = pen2kml(poOgrST, poKmlFactory);
            poKmlStyle->set_linestyle(poKmlLineStyle);

            break;
          }
				  case OGRSTCBrush:
          {
					  PolyStylePtr poKmlPolyStyle = brush2kml(poOgrST, poKmlFactory);
					  poKmlStyle->set_polystyle(poKmlPolyStyle);

            break;
          }
		  	  case OGRSTCSymbol:
          {
            IconStylePtr poKmlIconStyle = symbol2kml(poOgrST, poKmlFactory);
            poKmlStyle->set_iconstyle(poKmlIconStyle);

            break;
          }
	  		  case OGRSTCLabel:
          {
            LabelStylePtr poKmlLabelStyle = label2kml(poOgrST, poKmlFactory);
            poKmlStyle->set_labelstyle(poKmlLabelStyle);

            break;
          }
              
				  case OGRSTCNone:
				  default:
					  break;
			  }
	  	}
				
      poKmlDocument->set_styleselector(poKmlStyle);
			
      delete poOgrSM;

		}				
  }

  return;
}
