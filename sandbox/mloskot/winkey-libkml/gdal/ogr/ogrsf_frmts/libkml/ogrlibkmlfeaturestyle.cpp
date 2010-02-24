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

#include <ogrsf_frmts.h>
#include <ogr_featurestyle.h>

#include <kml/dom.h>

using kmldom::KmlFactory;;
using kmldom::PlacemarkPtr;
using kmldom::IconStylePtr;
using kmldom::PolyStylePtr;
using kmldom::LineStylePtr;
using kmldom::LabelStylePtr;
using kmldom::StylePtr;
using kmldom::StyleMapPtr;

#include "ogrlibkmlstyle.h"

void featurestyle2kml(
	OGRLayer *poOgrLayer,
	OGRFeature *poOgrFeat,
  KmlFactory *poKmlFactory,
  PlacemarkPtr poKmlPlacemark)
{
	
	/***** get the style table *****/
	
	OGRStyleTable *poOgrSTBL;
	
	const char *stylestring = poOgrFeat->GetStyleString();
	
	/***** does the feature have style? *****/
	
	if (stylestring) {

    /***** does it ref a style table? *****/
    
		if (*stylestring == '@') {
			
			/***** is the name in the layer style table *****/
			
			OGRStyleTable *hSTBLLayer;
			const char *testy = NULL;
			
			if ((hSTBLLayer = poOgrLayer->GetStyleTable()))
				testy = hSTBLLayer->Find(stylestring);
			
			if (testy)
        poKmlPlacemark->set_styleurl("#stylemap");//(char *)stylestring + 1);
				
			
			/***** assume its a dataset style, mayby the user will add it later *****/
			
			else
				poKmlPlacemark->set_styleurl("style/style.kml#stylemap");//(char *)stylestring + 1);
		}
		
    /***** no style table ref *****/
    
    else {

      StylePtr poKmlStyle = poKmlFactory->CreateStyle();
      
			/***** create and init a style mamager with the style string *****/
			
			OGRStyleMgr *poOgrSM = new OGRStyleMgr;
			poOgrSM->InitStyleString(stylestring);

      
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
#warning do the label case
					case OGRSTCNone:
					default:
						break;
				}
			}
      
			poKmlPlacemark->set_styleselector(poKmlStyle);
			
      delete poOgrSM;
      
		}
	} /* end style sting */
	
	/***** get the style table *****/
	
	else if ((poOgrSTBL = poOgrFeat->GetStyleTable())) {
		

    StylePtr poKmlStyle = poKmlFactory->CreateStyle();
      
		/***** parse the style table *****/
		
		poOgrSTBL->ResetStyleStringReading();
		const char *stylestring;
		while ((stylestring = poOgrSTBL->GetNextStyle())) {

			if (*stylestring == '@') {
				
				/***** is the name in the layer style table *****/
				
				OGRStyleTable *poOgrSTBLLayer;
				const char *testy = NULL;
				
				if ((poOgrSTBLLayer = poOgrLayer->GetStyleTable()))
					poOgrSTBLLayer->Find(stylestring);

#warning fixme
				if (testy)
					poKmlPlacemark->set_styleurl("#stylemap");//(char *)stylestring + 1);
				
				/***** assume its a dataset style, mayby the user will add it later *****/
#warning fixme
				
				else
					poKmlPlacemark->set_styleurl("style/style.kml#stylemap");//(char *)stylestring + 1);
			}
			
			else {
				
				/***** create and init a style mamager with the style string *****/
				
				OGRStyleMgr *poOgrSM = new OGRStyleMgr;
  			poOgrSM->InitStyleString(stylestring);;
				
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
				
        poKmlPlacemark->set_styleselector(poKmlStyle);
			
        delete poOgrSM;
			}
		}
	}
}
