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
#include <kml/base/color32.h>

using kmldom::KmlFactory;;
using kmldom::LineStylePtr;
using kmldom::PolyStylePtr;
using kmldom::IconStylePtr;
using kmldom::LabelStylePtr;
using kmldom::HotSpotPtr;
using kmlbase::Color32;

#include "ogrlibkmlstyle.h"

#warning all of this neends more work. units, scale, hotspots, and more

/******************************************************************************
 pen
******************************************************************************/

LineStylePtr pen2kml(
	OGRStyleTool *poOgrST,
  KmlFactory *poKmlFactory)
{
	GBool nullcheck;
	
	LineStylePtr poKmlLineStyle = poKmlFactory->CreateLineStyle();

  OGRStylePen *poStylePen = (OGRStylePen *)poOgrST;

  /***** pen color *****/
	
	int r, g, b, a;
  
  const char *color = poStylePen->Color(nullcheck);
	if (!nullcheck && poStylePen->GetRGBFromString(color, r, g, b, a)) {
    poKmlLineStyle->set_color(Color32(a, b, g, r));
	}

	double width = poStylePen->Width(nullcheck);
	if (nullcheck)
		width = 1.0;
  
  poKmlLineStyle->set_width(width);
  
  return poKmlLineStyle;
}

/******************************************************************************
 brush
******************************************************************************/

PolyStylePtr brush2kml(
	OGRStyleTool *poOgrST,
  KmlFactory *poKmlFactory)
{
	GBool nullcheck;

  PolyStylePtr poKmlPolyStyle = poKmlFactory->CreatePolyStyle();

  OGRStyleBrush *poStyleBrush = (OGRStyleBrush *)poOgrST;

  /***** brush color *****/
	
	int r, g, b, a;


	const char *color = poStyleBrush->ForeColor(nullcheck);
	if (!nullcheck && poStyleBrush->GetRGBFromString(color, r, g, b, a)) {
		poKmlPolyStyle->set_color(Color32(a, b, g, r));
	}

  
	  
  return poKmlPolyStyle;
}

/******************************************************************************
 symbol
******************************************************************************/

IconStylePtr symbol2kml(
	OGRStyleTool *poOgrST,
  KmlFactory *poKmlFactory)
{
	GBool nullcheck;
	GBool nullcheck2;

  IconStylePtr poKmlIconStyle = poKmlFactory->CreateIconStyle();
  
  OGRStyleSymbol *poStyleSymbol = (OGRStyleSymbol *)poOgrST;

  /***** id (kml icon) *****/
	const char *id = poStyleSymbol->Id(nullcheck);
	if (!nullcheck)
  {}
#warning set the icon		id = NULL;

  /***** heading *****/
  
	double heading = poStyleSymbol->Angle(nullcheck);
	if (!nullcheck)
		poKmlIconStyle->set_heading(heading);

  /***** scale *****/
  
	double scale = poStyleSymbol->Size(nullcheck);
	if (!nullcheck)
		poKmlIconStyle->set_scale(scale);

  /***** color *****/
	int r, g, b, a;
	const char *color = poStyleSymbol->Color(nullcheck);
	if (!nullcheck && poOgrST->GetRGBFromString(color, r,g, b, a)) {
		poKmlIconStyle->set_color(Color32(a, b, g, r));
	}

  /***** hotspot *****/
  
	double dx = poStyleSymbol->SpacingX(nullcheck);
	double dy = poStyleSymbol->SpacingY(nullcheck2);
	if (!nullcheck && !nullcheck2) {
    HotSpotPtr poKmlHotSpot = poKmlFactory->CreateHotSpot();
    poKmlHotSpot->set_x(dx);
		poKmlHotSpot->set_y(dy);
#warning set units
    poKmlIconStyle->set_hotspot(poKmlHotSpot);
	}
	
	return poKmlIconStyle;
}

/******************************************************************************
 label
******************************************************************************/

LabelStylePtr label2kml(
	OGRStyleTool *poOgrST,
  KmlFactory *poKmlFactory)
{

	LabelStylePtr poKmlLabelStyle = poKmlFactory->CreateLabelStyle();

#warning do the labelstyle *****/
  
  return poKmlLabelStyle;
}

