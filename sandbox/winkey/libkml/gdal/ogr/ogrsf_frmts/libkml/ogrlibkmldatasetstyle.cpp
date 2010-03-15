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
using kmldom::ObjectPtr;
using kmldom::StyleSelectorPtr;
using kmldom::IconStylePtr;
using kmldom::PolyStylePtr;
using kmldom::LineStylePtr;
using kmldom::LabelStylePtr;

#include "ogrlibkmlstyle.h"
#include "ogrlibkmldatasetstyle.h"

void datasetstyletable2kml (
    OGRStyleTable * poOgrStyleTable,
    KmlFactory * poKmlFactory,
    ContainerPtr poKmlContainer  )
{

    /***** just return if the styletable is null *****/
    
    if (!poOgrStyleTable)
        return;
    
    /***** parse the style table *****/

    poOgrStyleTable->ResetStyleStringReading (  );
    const char *pszStyleString;

    while ( ( pszStyleString = poOgrStyleTable->GetNextStyle (  ) ) ) {
        const char *pszStyleName = poOgrStyleTable->GetLastStyleName (  );

        /***** add the style header to the kml *****/

        StylePtr poKmlStyle = poKmlFactory->CreateStyle (  );

        poKmlStyle->set_id ( pszStyleName + 1 );

        /***** parse the style string *****/

        addstylestring2kml ( pszStyleString, poKmlStyle, poKmlFactory );

        /***** add the style to the container *****/

        ObjectPtr pokmlObject = boost::static_pointer_cast <kmldom::Object> (poKmlStyle) ;
        poKmlContainer->add_feature ( boost::static_pointer_cast <kmldom::Feature>(pokmlObject));
        

    }

    return;
}



void kml2datasetstyletable (
    OGRStyleTable * poOgrStyleTable,
    StylePtr poKmlStyle )
{

    
    /***** no reason to add it if it don't have an id *****/
    
    if ( poKmlStyle->has_id (  ) ) {  

        OGRStyleMgr *poOgrSM = new OGRStyleMgr ( poOgrStyleTable );

        poOgrSM->InitStyleString ( NULL );

        /***** read the style *****/

        kml2stylestring ( poKmlStyle, poOgrSM );

        /***** add the style to the style table *****/

        const std::string oName = poKmlStyle->get_id (  );


        poOgrSM->AddStyle ( CPLString (  ). Printf ( "@%s",
                                                    oName.c_str (  ) ),
                            NULL );
        
        /***** cleanup the style manager *****/

        delete poOgrSM;
    }

    else {
        CPLError ( CE_Failure, CPLE_AppDefined,
                   "ERROR Parseing kml Style: No id");
    }

    return;
}