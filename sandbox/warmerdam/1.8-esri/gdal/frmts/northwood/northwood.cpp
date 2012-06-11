/******************************************************************************
 * $Id$
 *
 * Project:  GRC/GRD Reader
 * Purpose:  Northwood Format basic implementation
 * Author:   Perry Casson
 *
 ******************************************************************************
 * Copyright (c) 2007, Waypoint Information Technology
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


//#ifndef MSVC
#include "gdal_pam.h"
//#endif

#include "northwood.h"


int nwt_ParseHeader( NWT_GRID * pGrd, char *nwtHeader )
{
    int i;
    unsigned short usTmp;
    double dfTmp;
    unsigned char cTmp[256];

    if( nwtHeader[4] == '1' )
        pGrd->cFormat = 0x00;        // grd - surface type
    else if( nwtHeader[4] == '8' )
        pGrd->cFormat = 0x80;        //  grc classified type

    pGrd->stClassDict = NULL;

    memcpy( (void *) &pGrd->fVersion, (void *) &nwtHeader[5],
              sizeof(pGrd->fVersion) );
    CPL_LSBPTR32(&pGrd->fVersion);

    memcpy( (void *) &usTmp, (void *) &nwtHeader[9], 2 );
    CPL_LSBPTR16(&usTmp);
    pGrd->nXSide = (unsigned int) usTmp;
    if( pGrd->nXSide == 0 )
    {
        memcpy( (void *) &pGrd->nXSide, (void *) &nwtHeader[128],
                sizeof(pGrd->nXSide) );
        CPL_LSBPTR32(&pGrd->nXSide);
    }

    memcpy( (void *) &usTmp, (void *) &nwtHeader[11], 2 );
    CPL_LSBPTR16(&usTmp);
    pGrd->nYSide = (unsigned int) usTmp;
    if( pGrd->nYSide == 0 )
    {
        memcpy( (void *) &pGrd->nYSide, (void *) &nwtHeader[132],
                sizeof(pGrd->nYSide) );
        CPL_LSBPTR32(&pGrd->nYSide);
    }

    memcpy( (void *) &pGrd->dfMinX, (void *) &nwtHeader[13],
            sizeof(pGrd->dfMinX) );
    CPL_LSBPTR64(&pGrd->dfMinX);
    memcpy( (void *) &pGrd->dfMaxX, (void *) &nwtHeader[21],
            sizeof(pGrd->dfMaxX) );
    CPL_LSBPTR64(&pGrd->dfMaxX);
    memcpy( (void *) &pGrd->dfMinY, (void *) &nwtHeader[29],
            sizeof(pGrd->dfMinY) );
    CPL_LSBPTR64(&pGrd->dfMinY);
    memcpy( (void *) &pGrd->dfMaxY, (void *) &nwtHeader[37],
            sizeof(pGrd->dfMaxY) );
    CPL_LSBPTR64(&pGrd->dfMaxY);

    pGrd->dfStepSize = (pGrd->dfMaxX - pGrd->dfMinX) / (pGrd->nXSide - 1);
    dfTmp = (pGrd->dfMaxY - pGrd->dfMinY) / (pGrd->nYSide - 1);

    memcpy( (void *) &pGrd->fZMin, (void *) &nwtHeader[45],
            sizeof(pGrd->fZMin) );
    CPL_LSBPTR32(&pGrd->fZMin);
    memcpy( (void *) &pGrd->fZMax, (void *) &nwtHeader[49],
            sizeof(pGrd->fZMax) );
    CPL_LSBPTR32(&pGrd->fZMax);
    memcpy( (void *) &pGrd->fZMinScale, (void *) &nwtHeader[53],
            sizeof(pGrd->fZMinScale) );
    CPL_LSBPTR32(&pGrd->fZMinScale);
    memcpy( (void *) &pGrd->fZMaxScale, (void *) &nwtHeader[57],
            sizeof(pGrd->fZMaxScale) );
    CPL_LSBPTR32(&pGrd->fZMaxScale);

    memcpy( (void *) &pGrd->cDescription, (void *) &nwtHeader[61],
            sizeof(pGrd->cDescription) );
    memcpy( (void *) &pGrd->cZUnits, (void *) &nwtHeader[93],
            sizeof(pGrd->cZUnits) );

    memcpy( (void *) &i, (void *) &nwtHeader[136], 4 );
    CPL_LSBPTR32(&i);

    if( i == 1129336130 )
    {                            //BMPC
        if( nwtHeader[140] & 0x01 )
        {
            pGrd->cHillShadeBrightness = nwtHeader[144];
            pGrd->cHillShadeContrast = nwtHeader[145];
        }
    }

    memcpy( (void *) &pGrd->cMICoordSys, (void *) &nwtHeader[256],
            sizeof(pGrd->cMICoordSys) );
    pGrd->cMICoordSys[sizeof(pGrd->cMICoordSys)-1] = '\0';

    pGrd->iZUnits = nwtHeader[512];

    if( nwtHeader[513] & 0x80 )
        pGrd->bShowGradient = true;

    if( nwtHeader[513] & 0x40 )
        pGrd->bShowHillShade = true;

    if( nwtHeader[513] & 0x20 )
        pGrd->bHillShadeExists = true;

    memcpy( (void *) &pGrd->iNumColorInflections, (void *) &nwtHeader[516],
            2 );
    CPL_LSBPTR16(&pGrd->iNumColorInflections);

    if (pGrd->iNumColorInflections > 32)
    {
        CPLError(CE_Failure, CPLE_AppDefined, "Corrupt header");
        pGrd->iNumColorInflections = i;
        return FALSE;
    }
    
    for( i = 0; i < pGrd->iNumColorInflections; i++ )
    {
        
        memcpy( (void *) &pGrd->stInflection[i].zVal,
                (void *) &nwtHeader[518 + (7 * i)], 4 );
        CPL_LSBPTR32(&pGrd->stInflection[i].zVal);
        memcpy( (void *) &pGrd->stInflection[i].r,
                (void *) &nwtHeader[522 + (7 * i)], 1 );
        memcpy( (void *) &pGrd->stInflection[i].g,
                (void *) &nwtHeader[523 + (7 * i)], 1 );
        memcpy( (void *) &pGrd->stInflection[i].b,
                (void *) &nwtHeader[524 + (7 * i)], 1 );
    }

    memcpy( (void *) &pGrd->fHillShadeAzimuth, (void *) &nwtHeader[966],
            sizeof(pGrd->fHillShadeAzimuth) );
    CPL_LSBPTR32(&pGrd->fHillShadeAzimuth);
    memcpy( (void *) &pGrd->fHillShadeAngle, (void *) &nwtHeader[970],
            sizeof(pGrd->fHillShadeAngle) );
    CPL_LSBPTR32(&pGrd->fHillShadeAngle);

    pGrd->cFormat += nwtHeader[1023];    // the msb for grd/grc was already set


    // there are more types than this - need to build other types for testing
    if( pGrd->cFormat & 0x80 )
    {
        if( nwtHeader[1023] == 0 )
            pGrd->nBitsPerPixel = 16;
        else
            pGrd->nBitsPerPixel = nwtHeader[1023] * 4;
    }
    else
        pGrd->nBitsPerPixel = nwtHeader[1023] * 8;


    if( pGrd->cFormat & 0x80 )        // if is GRC load the Dictionary
    {
        VSIFSeekL( pGrd->fp,
               1024 + (pGrd->nXSide * pGrd->nYSide) * pGrd->nBitsPerPixel / 8,
               SEEK_SET );

        if( !VSIFReadL( &usTmp, 2, 1, pGrd->fp) )
            return FALSE;
        CPL_LSBPTR16(&usTmp);
        pGrd->stClassDict =
            (NWT_CLASSIFIED_DICT *) calloc( sizeof(NWT_CLASSIFIED_DICT), 1 );

        pGrd->stClassDict->nNumClassifiedItems = usTmp;

        pGrd->stClassDict->stClassifedItem =
            (NWT_CLASSIFIED_ITEM **) calloc( sizeof(NWT_CLASSIFIED_ITEM *),
                                             pGrd->
                                             stClassDict->nNumClassifiedItems +
                                             1 );

        //load the dictionary
        for( usTmp=0; usTmp < pGrd->stClassDict->nNumClassifiedItems; usTmp++ )
        {
            pGrd->stClassDict->stClassifedItem[usTmp] =
              (NWT_CLASSIFIED_ITEM *) calloc( sizeof(NWT_CLASSIFIED_ITEM), 1 );
            if( !VSIFReadL( &cTmp, 9, 1, pGrd->fp ) )
                return FALSE;
            memcpy( (void *) &pGrd->stClassDict->
                    stClassifedItem[usTmp]->usPixVal, (void *) &cTmp[0], 2 );
            CPL_LSBPTR16(&pGrd->stClassDict->stClassifedItem[usTmp]->usPixVal);
            memcpy( (void *) &pGrd->stClassDict->stClassifedItem[usTmp]->res1,
                    (void *) &cTmp[2], 1 );
            memcpy( (void *) &pGrd->stClassDict->stClassifedItem[usTmp]->r,
                    (void *) &cTmp[3], 1 );
            memcpy( (void *) &pGrd->stClassDict->stClassifedItem[usTmp]->g,
                    (void *) &cTmp[4], 1 );
            memcpy( (void *) &pGrd->stClassDict->stClassifedItem[usTmp]->b,
                    (void *) &cTmp[5], 1 );
            memcpy( (void *) &pGrd->stClassDict->stClassifedItem[usTmp]->res2,
                    (void *) &cTmp[6], 1 );
            memcpy( (void *) &pGrd->stClassDict->stClassifedItem[usTmp]->usLen,
                    (void *) &cTmp[7], 2 );
            CPL_LSBPTR16(&pGrd->stClassDict->stClassifedItem[usTmp]->usLen);
                    
            if ( pGrd->stClassDict->stClassifedItem[usTmp]->usLen > 256)
                return FALSE;

            if( !VSIFReadL( &pGrd->stClassDict->stClassifedItem[usTmp]->szClassName,
                        pGrd->stClassDict->stClassifedItem[usTmp]->usLen,
                        1, pGrd->fp ) )
                return FALSE;
                
            pGrd->stClassDict->stClassifedItem[usTmp]->szClassName[255] = '\0';
        }
    }
    
    return TRUE;
}


// Create a color gradient ranging from ZMin to Zmax using the color
// inflections defined in grid
int nwt_LoadColors( NWT_RGB * pMap, int mapSize, NWT_GRID * pGrd )
{
    int i;
    NWT_RGB sColor;
    int nWarkerMark = 0;

    createIP( 0, 255, 255, 255, pMap, &nWarkerMark );
    // If Zmin is less than the 1st inflection use the 1st inflections color to
    // the start of the ramp
    if( pGrd->fZMin <= pGrd->stInflection[0].zVal )
    {
        createIP( 1, pGrd->stInflection[0].r,
                     pGrd->stInflection[0].g,
                     pGrd->stInflection[0].b, pMap, &nWarkerMark );
    }
    // find what inflections zmin is between
    for( i = 0; i < pGrd->iNumColorInflections; i++ )
    {
        if( pGrd->fZMin < pGrd->stInflection[i].zVal )
        {
            // then we must be between i and i-1
            linearColor( &sColor, &pGrd->stInflection[i - 1],
                                  &pGrd->stInflection[i],
                                  pGrd->fZMin );
            createIP( 1, sColor.r, sColor.g, sColor.b, pMap, &nWarkerMark );
            break;
        }
    }
    // the interesting case of zmin beig higher than the max inflection value
    if( i >= pGrd->iNumColorInflections )
    {
        createIP( 1,
                  pGrd->stInflection[pGrd->iNumColorInflections - 1].r,
                  pGrd->stInflection[pGrd->iNumColorInflections - 1].g,
                  pGrd->stInflection[pGrd->iNumColorInflections - 1].b,
                  pMap, &nWarkerMark );
        createIP( mapSize - 1,
                  pGrd->stInflection[pGrd->iNumColorInflections - 1].r,
                  pGrd->stInflection[pGrd->iNumColorInflections - 1].g,
                  pGrd->stInflection[pGrd->iNumColorInflections - 1].b,
                  pMap, &nWarkerMark );
    }
    else
    {
        int index = 0;
        for( ; i < pGrd->iNumColorInflections; i++ )
        {
            if( pGrd->fZMax < pGrd->stInflection[i].zVal )
            {
                // then we must be between i and i-1
                linearColor( &sColor, &pGrd->stInflection[i - 1],
                                      &pGrd->stInflection[i], pGrd->fZMin );
                index = mapSize - 1;
                createIP( index, sColor.r, sColor.g, sColor.b, pMap,
                           &nWarkerMark );
                break;
            }
            // save the inflections between zmin and zmax
            index = (int)( ( (pGrd->stInflection[i].zVal - pGrd->fZMin) /
                                              (pGrd->fZMax - pGrd->fZMin) )
                           * mapSize);
                           
            if ( index >= mapSize )
                index = mapSize - 1;
            createIP( index,
                      pGrd->stInflection[i].r,
                      pGrd->stInflection[i].g,
                      pGrd->stInflection[i].b,
                      pMap, &nWarkerMark );
        }
        if( index < mapSize - 1 )
            createIP( mapSize - 1,
                      pGrd->stInflection[pGrd->iNumColorInflections - 1].r,
                      pGrd->stInflection[pGrd->iNumColorInflections - 1].g,
                      pGrd->stInflection[pGrd->iNumColorInflections - 1].b,
                      pMap, &nWarkerMark );
    }
    return 0;
}

//solve for a color between pIPLow and pIPHigh
void linearColor( NWT_RGB * pRGB, NWT_INFLECTION * pIPLow, NWT_INFLECTION * pIPHigh,
                      float fMid )
{
    if( fMid < pIPLow->zVal )
    {
        pRGB->r = pIPLow->r;
        pRGB->g = pIPLow->g;
        pRGB->b = pIPLow->b;
    }
    else if( fMid > pIPHigh->zVal )
    {
        pRGB->r = pIPHigh->r;
        pRGB->g = pIPHigh->g;
        pRGB->b = pIPHigh->b;
    }
    else
    {
        float scale = (fMid - pIPLow->zVal) / (pIPHigh->zVal - pIPLow->zVal);
        pRGB->r = (unsigned char)
                (scale * (pIPHigh->r - pIPLow->r) + pIPLow->r + 0.5);
        pRGB->g = (unsigned char)
                (scale * (pIPHigh->g - pIPLow->g) + pIPLow->g + 0.5);
        pRGB->b = (unsigned char)
                (scale * (pIPHigh->b - pIPLow->b) + pIPLow->b + 0.5);
    }
}

// insert IP's into the map filling as we go
void createIP( int index, unsigned char r, unsigned char g, unsigned char b,
               NWT_RGB * map, int *pnWarkerMark )
{
    int i;

    if( index == 0 )
    {
        map[0].r = r;
        map[0].g = g;
        map[0].b = b;
        *pnWarkerMark = 0;
        return;
    }

    if( index <= *pnWarkerMark )
        return;

    int wm = *pnWarkerMark;

    float rslope = (float)(r - map[wm].r) / (float)(index - wm);
    float gslope = (float)(g - map[wm].g) / (float)(index - wm);
    float bslope = (float)(b - map[wm].b) / (float)(index - wm);
    for( i = wm + 1; i < index; i++)
    {
        map[i].r = map[wm].r + (unsigned char)(((i - wm) * rslope) + 0.5);
        map[i].g = map[wm].g + (unsigned char)(((i - wm) * gslope) + 0.5);
        map[i].b = map[wm].b + (unsigned char)(((i - wm) * bslope) + 0.5);
    }
    map[index].r = r;
    map[index].g = g;
    map[index].b = b;
    *pnWarkerMark = index;
    return;
}

void nwt_HillShade( unsigned char *r, unsigned char *g, unsigned char *b,
                    char *h )
{
    HLS hls;
    NWT_RGB rgb;
    rgb.r = *r;
    rgb.g = *g;
    rgb.b = *b;
    hls = RGBtoHLS( rgb );
    hls.l += ((short) *h) * HLSMAX / 256;
    rgb = HLStoRGB( hls );

    *r = rgb.r;
    *g = rgb.g;
    *b = rgb.b;
    return;
}


NWT_GRID *nwtOpenGrid( char *filename )
{
    NWT_GRID *pGrd;
    char nwtHeader[1024];
    VSILFILE *fp;

    if( (fp = VSIFOpenL( filename, "rb" )) == NULL )
    {
        fprintf( stderr, "\nCan't open %s\n", filename );
        return NULL;
    }

    if( !VSIFReadL( nwtHeader, 1024, 1, fp ) )
        return NULL;

    if( nwtHeader[0] != 'H' ||
        nwtHeader[1] != 'G' ||
        nwtHeader[2] != 'P' ||
        nwtHeader[3] != 'C' )
          return NULL;

    pGrd = (NWT_GRID *) calloc( sizeof(NWT_GRID), 1 );

    if( nwtHeader[4] == '1' )
        pGrd->cFormat = 0x00;        // grd - surface type
    else if( nwtHeader[4] == '8' )
        pGrd->cFormat = 0x80;        //  grc classified type
    else
    {
        fprintf( stderr, "\nUnhandled Northwood format type = %0xd\n",
                 nwtHeader[4] );
        if( pGrd )
            free( pGrd );
        return NULL;
    }

    strcpy( pGrd->szFileName, filename );
    pGrd->fp = fp;
    nwt_ParseHeader( pGrd, nwtHeader );

    return pGrd;
}

//close the file and free the mem
void nwtCloseGrid( NWT_GRID * pGrd )
{
    unsigned short usTmp;

    if( (pGrd->cFormat & 0x80) && pGrd->stClassDict )        // if is GRC - free the Dictionary
    {
        for( usTmp = 0; usTmp < pGrd->stClassDict->nNumClassifiedItems; usTmp++ )
        {
            free( pGrd->stClassDict->stClassifedItem[usTmp] );
        }
        free( pGrd->stClassDict->stClassifedItem );
        free( pGrd->stClassDict );
    }
    if( pGrd->fp )
        VSIFCloseL( pGrd->fp );
    free( pGrd );
        return;
}

void nwtGetRow( NWT_GRID * pGrd )
{

}

void nwtPrintGridHeader( NWT_GRID * pGrd )
{
    int i;

    if( pGrd->cFormat & 0x80 )
    {
        printf( "\n%s\n\nGrid type is Classified ", pGrd->szFileName );
        if( pGrd->cFormat == 0x81 )
            printf( "4 bit (Less than 16 Classes)" );
        else if( pGrd->cFormat == 0x82 )
            printf( "8 bit (Less than 256 Classes)" );
        else if( pGrd->cFormat == 0x84 )
            printf( "16 bit (Less than 65536 Classes)" );
        else
        {
            printf( "GRC - Unhandled Format or Type %d", pGrd->cFormat );
            return;
        }
    }
    else
    {
        printf( "\n%s\n\nGrid type is Numeric ", pGrd->szFileName );
        if( pGrd->cFormat == 0x00 )
            printf( "16 bit (Standard Percision)" );
        else if( pGrd->cFormat == 0x01 )
            printf( "32 bit (High Percision)" );
        else
        {
            printf( "GRD - Unhandled Format or Type %d", pGrd->cFormat );
            return;
        }
    }
    printf( "\nDim (x,y) = (%d,%d)", pGrd->nXSide, pGrd->nYSide );
    printf( "\nStep Size = %f", pGrd->dfStepSize );
    printf( "\nBounds = (%f,%f) (%f,%f)", pGrd->dfMinX, pGrd->dfMinY,
            pGrd->dfMaxX, pGrd->dfMaxY );
    printf( "\nCoordinate System = %s", pGrd->cMICoordSys );

    if( !(pGrd->cFormat & 0x80) )    // print the numeric specific stuff
    {
        printf( "\nMin Z = %f Max Z = %f Z Units = %d \"%s\"", pGrd->fZMin,
                pGrd->fZMax, pGrd->iZUnits, pGrd->cZUnits );

        printf( "\n\nDisplay Mode =" );
        if( pGrd->bShowGradient )
            printf( " Color Gradient" );

        if( pGrd->bShowGradient && pGrd->bShowHillShade )
            printf( " and" );

        if( pGrd->bShowHillShade )
            printf( " Hill Shading" );

        for( i = 0; i < pGrd->iNumColorInflections; i++ )
        {
            printf( "\nColor Inflection %d - %f (%d,%d,%d)", i + 1,
                    pGrd->stInflection[i].zVal, pGrd->stInflection[i].r,
                    pGrd->stInflection[i].g, pGrd->stInflection[i].b );
        }

        if( pGrd->bHillShadeExists )
        {
            printf("\n\nHill Shade Azumith = %.1f Inclination = %.1f "
                   "Brightness = %d Contrast = %d",
                   pGrd->fHillShadeAzimuth, pGrd->fHillShadeAngle,
                   pGrd->cHillShadeBrightness, pGrd->cHillShadeContrast );
        }
        else
            printf( "\n\nNo Hill Shade Data" );
    }
    else                            // print the classified specific stuff
    {
        printf( "\nNumber of Classes defined = %d",
                pGrd->stClassDict->nNumClassifiedItems );
        for( i = 0; i < (int) pGrd->stClassDict->nNumClassifiedItems; i++ )
        {
            printf( "\n%s - (%d,%d,%d)  Raw = %d  %d %d",
                    pGrd->stClassDict->stClassifedItem[i]->szClassName,
                    pGrd->stClassDict->stClassifedItem[i]->r,
                    pGrd->stClassDict->stClassifedItem[i]->g,
                    pGrd->stClassDict->stClassifedItem[i]->b,
                    pGrd->stClassDict->stClassifedItem[i]->usPixVal,
                    pGrd->stClassDict->stClassifedItem[i]->res1,
                    pGrd->stClassDict->stClassifedItem[i]->res2 );
        }
    }
}

HLS RGBtoHLS( NWT_RGB rgb )
{
    short R, G, B;                /* input RGB values */
    HLS hls;
    unsigned char cMax, cMin;        /* max and min RGB values */
    short Rdelta, Gdelta, Bdelta;    /* intermediate value: % of spread from max */
    /* get R, G, and B out of DWORD */
    R = rgb.r;
    G = rgb.g;
    B = rgb.b;

    /* calculate lightness */
    cMax = MAX( MAX(R,G), B );
    cMin = MIN( MIN(R,G), B );
    hls.l = (((cMax + cMin) * HLSMAX) + RGBMAX) / (2 * RGBMAX);

    if( cMax == cMin )
    {                            /* r=g=b --> achromatic case */
        hls.s = 0;                /* saturation */
        hls.h = UNDEFINED;        /* hue */
    }
    else
    {                            /* chromatic case */
        /* saturation */
        if( hls.l <= (HLSMAX / 2) )
            hls.s =
              (((cMax - cMin) * HLSMAX) + ((cMax + cMin) / 2)) / (cMax + cMin);
        else
            hls.s= (((cMax - cMin) * HLSMAX) + ((2 * RGBMAX - cMax - cMin) / 2))
              / (2 * RGBMAX - cMax - cMin);

        /* hue */
        Rdelta =
            (((cMax - R) * (HLSMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin);
        Gdelta =
            (((cMax - G) * (HLSMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin);
        Bdelta =
            (((cMax - B) * (HLSMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin);

        if( R == cMax )
            hls.h = Bdelta - Gdelta;
        else if( G == cMax )
            hls.h = (HLSMAX / 3) + Rdelta - Bdelta;
        else                        /* B == cMax */
            hls.h = ((2 * HLSMAX) / 3) + Gdelta - Rdelta;

        if( hls.h < 0 )
            hls.h += HLSMAX;
        if( hls.h > HLSMAX )
            hls.h -= HLSMAX;
    }
    return hls;
}


/* utility routine for HLStoRGB */
short HueToRGB( short n1, short n2, short hue )
{
    /* range check: note values passed add/subtract thirds of range */
    if( hue < 0 )
        hue += HLSMAX;

    if( hue > HLSMAX )
        hue -= HLSMAX;

    /* return r,g, or b value from this tridrant */
    if( hue < (HLSMAX / 6) )
        return (n1 + (((n2 - n1) * hue + (HLSMAX / 12)) / (HLSMAX / 6)));
    if( hue < (HLSMAX / 2) )
        return (n2);
    if( hue < ((HLSMAX * 2) / 3) )
        return (n1 +
                (((n2 - n1) * (((HLSMAX * 2) / 3) - hue) +
                (HLSMAX / 12)) / (HLSMAX / 6)));
    else
        return (n1);
}

NWT_RGB HLStoRGB( HLS hls )
{
    NWT_RGB rgb;
    short Magic1, Magic2;            /* calculated magic numbers (really!) */

    if( hls.s == 0 )
    {                            /* achromatic case */
        rgb.r = rgb.g = rgb.b = (hls.l * RGBMAX) / HLSMAX;
        if( hls.h != UNDEFINED )
        {
            /* ERROR */
        }
    }
    else
    {                            /* chromatic case */
        /* set up magic numbers */
        if( hls.l <= (HLSMAX / 2) )
            Magic2 = (hls.l * (HLSMAX + hls.s) + (HLSMAX / 2)) / HLSMAX;
        else
            Magic2 = hls.l + hls.s - ((hls.l * hls.s) + (HLSMAX / 2)) / HLSMAX;
        Magic1 = 2 * hls.l - Magic2;

        /* get RGB, change units from HLSMAX to RGBMAX */
        rgb.r = (HueToRGB (Magic1, Magic2, hls.h + (HLSMAX / 3)) * RGBMAX +
                 (HLSMAX / 2)) / HLSMAX;
        rgb.g = (HueToRGB (Magic1, Magic2, hls.h) * RGBMAX + (HLSMAX / 2)) /
                  HLSMAX;
        rgb.b = (HueToRGB (Magic1, Magic2, hls.h - (HLSMAX / 3)) * RGBMAX +
                 (HLSMAX / 2)) / HLSMAX;
    }

    return rgb;
}

#if defined( ESRI_BUILD )

//
// ESRI specific
//
// These types, tables and functions are copied from 
// ogr/ogrsf_frmts/mitab/mitab_coordsys.cpp and ogr/ogrsf_frmts/mitab/mitab_spatialref.cpp 
// to allow spatial referencing to work without requiring OGR.
//

typedef struct {
    int         nMapInfoDatumID;
    const char  *pszOGCDatumName;
    int         nEllipsoid;
    double      dfShiftX;
    double      dfShiftY;
    double      dfShiftZ;
    double      dfDatumParm0; /* RotX */
    double      dfDatumParm1; /* RotY */
    double      dfDatumParm2; /* RotZ */
    double      dfDatumParm3; /* Scale Factor */
    double      dfDatumParm4; /* Prime Meridian */
} MapInfoDatumInfo;

typedef struct
{
    int         nMapInfoId;
    const char *pszMapinfoName;
    double      dfA; /* semi major axis in meters */
    double      dfInvFlattening; /* Inverse flattening */
} MapInfoSpheroidInfo;

/* -------------------------------------------------------------------- */
/*      This table was automatically generated by doing translations    */
/*      between mif and tab for each datum, and extracting the          */
/*      parameters from the tab file.  The OGC names were added         */
/*      afterwards and may be incomplete or inaccurate.                 */
/* -------------------------------------------------------------------- */
MapInfoDatumInfo asDatumInfoList[] =
{

{104, "WGS_1984",                   28,0, 0, 0, 0, 0, 0, 0, 0},
{74,  "North_American_Datum_1983",  0, 0, 0, 0, 0, 0, 0, 0, 0},

{0,  "",                            29, 0,   0,    0,   0, 0, 0, 0, 0}, // Datum ignore

{1,  "Adindan",                     6, -162, -12,  206, 0, 0, 0, 0, 0},
{2,  "Afgooye",                     3, -43,  -163, 45,  0, 0, 0, 0, 0},
{3,  "Ain_el_Abd_1970",             4, -150, -251, -2,  0, 0, 0, 0, 0},
{4,  "Anna_1_Astro_1965",           2, -491, -22,  435, 0, 0, 0, 0, 0},
{5,  "Arc_1950",                    15,-143, -90,  -294,0, 0, 0, 0, 0},
{6,  "Arc_1960",                    6, -160, -8,   -300,0, 0, 0, 0, 0},
{7,  "Ascension_Islands",           4, -207, 107,  52,  0, 0, 0, 0, 0},
{8,  "Astro_Beacon_E",              4, 145,  75,   -272,0, 0, 0, 0, 0},
{9,  "Astro_B4_Sorol_Atoll",        4, 114,  -116, -333,0, 0, 0, 0, 0},
{10, "Astro_Dos_71_4",              4, -320, 550,  -494,0, 0, 0, 0, 0},
{11, "Astronomic_Station_1952",     4, 124,  -234, -25, 0, 0, 0, 0, 0},
{12, "Australian_Geodetic_Datum_66",2, -133, -48,  148, 0, 0, 0, 0, 0},
{13, "Australian_Geodetic_Datum_84",2, -134, -48,  149, 0, 0, 0, 0, 0},
{14, "Bellevue_Ign",                4, -127, -769, 472, 0, 0, 0, 0, 0},
{15, "Bermuda_1957",                7, -73,  213,  296, 0, 0, 0, 0, 0},
{16, "Bogota",                      4, 307,  304,  -318,0, 0, 0, 0, 0},
{17, "Campo_Inchauspe",             4, -148, 136,  90,  0, 0, 0, 0, 0},
{18, "Canton_Astro_1966",           4, 298,  -304, -375,0, 0, 0, 0, 0},
{19, "Cape",                        6, -136, -108, -292,0, 0, 0, 0, 0},
{20, "Cape_Canaveral",              7, -2,   150,  181, 0, 0, 0, 0, 0},
{21, "Carthage",                    6, -263, 6,    431, 0, 0, 0, 0, 0},
{22, "Chatham_1971",                4, 175,  -38,  113, 0, 0, 0, 0, 0},
{23, "Chua",                        4, -134, 229,  -29, 0, 0, 0, 0, 0},
{24, "Corrego_Alegre",              4, -206, 172,  -6,  0, 0, 0, 0, 0},
{25, "Batavia",                     10,-377,681,   -50, 0, 0, 0, 0, 0},
{26, "Dos_1968",                    4, 230,  -199, -752,0, 0, 0, 0, 0},
{27, "Easter_Island_1967",          4, 211,  147,  111, 0, 0, 0, 0, 0},
{28, "European_Datum_1950",         4, -87,  -98,  -121,0, 0, 0, 0, 0},
{29, "European_Datum_1979",         4, -86,  -98,  -119,0, 0, 0, 0, 0},
{30, "Gandajika_1970",              4, -133, -321, 50,  0, 0, 0, 0, 0},
{31, "New_Zealand_GD49",            4, 84,   -22,  209, 0, 0, 0, 0, 0},
{31, "New_Zealand_Geodetic_Datum_1949",4,84, -22,  209, 0, 0, 0, 0, 0},
{32, "GRS_67",                      21,0,    0,    0,   0, 0, 0, 0, 0},
{33, "GRS_80",                      0, 0,    0,    0,   0, 0, 0, 0, 0},
{34, "Guam_1963",                   7, -100, -248, 259, 0, 0, 0, 0, 0},
{35, "Gux_1_Astro",                 4, 252,  -209, -751,0, 0, 0, 0, 0},
{36, "Hito_XVIII_1963",             4, 16,   196,  93,  0, 0, 0, 0, 0},
{37, "Hjorsey_1955",                4, -73,  46,   -86, 0, 0, 0, 0, 0},
{38, "Hong_Kong_1963",              4, -156, -271, -189,0, 0, 0, 0, 0},
{39, "Hu_Tzu_Shan",                 4, -634, -549, -201,0, 0, 0, 0, 0},
{40, "Indian_Thailand_Vietnam",     11,214,  836,  303, 0, 0, 0, 0, 0},
{41, "Indian_Bangladesh",           11,289,  734,  257, 0, 0, 0, 0, 0},
{42, "Ireland_1965",                13,506,  -122, 611, 0, 0, 0, 0, 0},
{43, "ISTS_073_Astro_1969",         4, 208,  -435, -229,0, 0, 0, 0, 0},
{44, "Johnston_Island_1961",        4, 191,  -77,  -204,0, 0, 0, 0, 0},
{45, "Kandawala",                   11,-97,  787,  86,  0, 0, 0, 0, 0},
{46, "Kerguyelen_Island",           4, 145,  -187, 103, 0, 0, 0, 0, 0},
{47, "Kertau",                      17,-11,  851,  5,   0, 0, 0, 0, 0},
{48, "L_C_5_Astro",                 7, 42,   124,  147, 0, 0, 0, 0, 0},
{49, "Liberia_1964",                6, -90,  40,   88,  0, 0, 0, 0, 0},
{50, "Luzon_Phillippines",          7, -133, -77,  -51, 0, 0, 0, 0, 0},
{51, "Luzon_Mindanao_Island",       7, -133, -79,  -72, 0, 0, 0, 0, 0},
{52, "Mahe_1971",                   6, 41,   -220, -134,0, 0, 0, 0, 0},
{53, "Marco_Astro",                 4, -289, -124, 60,  0, 0, 0, 0, 0},
{54, "Massawa",                     10,639,  405,  60,  0, 0, 0, 0, 0},
{55, "Merchich",                    16,31,   146,  47,  0, 0, 0, 0, 0},
{56, "Midway_Astro_1961",           4, 912,  -58,  1227,0, 0, 0, 0, 0},
{57, "Minna",                       6, -92,  -93,  122, 0, 0, 0, 0, 0},
{58, "Nahrwan_Masirah_Island",      6, -247, -148, 369, 0, 0, 0, 0, 0},
{59, "Nahrwan_Un_Arab_Emirates",    6, -249, -156, 381, 0, 0, 0, 0, 0},
{60, "Nahrwan_Saudi_Arabia",        6, -231, -196, 482, 0, 0, 0, 0, 0},
{61, "Naparima_1972",               4, -2,   374,  172, 0, 0, 0, 0, 0},
{62, "NAD_1927",                    7, -8,   160,  176, 0, 0, 0, 0, 0},
{62, "North_American_Datum_1927",   7, -8,   160,  176, 0, 0, 0, 0, 0},
{63, "NAD_27_Alaska",               7, -5,   135,  172, 0, 0, 0, 0, 0},
{64, "NAD_27_Bahamas",              7, -4,   154,  178, 0, 0, 0, 0, 0},
{65, "NAD_27_San_Salvador",         7, 1,    140,  165, 0, 0, 0, 0, 0},
{66, "NAD_27_Canada",               7, -10,  158,  187, 0, 0, 0, 0, 0},
{67, "NAD_27_Canal_Zone",           7, 0,    125,  201, 0, 0, 0, 0, 0},
{68, "NAD_27_Caribbean",            7, -7,   152,  178, 0, 0, 0, 0, 0},
{69, "NAD_27_Central_America",      7, 0,    125,  194, 0, 0, 0, 0, 0},
{70, "NAD_27_Cuba",                 7, -9,   152,  178, 0, 0, 0, 0, 0},
{71, "NAD_27_Greenland",            7, 11,   114,  195, 0, 0, 0, 0, 0},
{72, "NAD_27_Mexico",               7, -12,  130,  190, 0, 0, 0, 0, 0},
{73, "NAD_27_Michigan",             8, -8,   160,  176, 0, 0, 0, 0, 0},
{75, "Observatorio_1966",           4, -425, -169, 81,  0, 0, 0, 0, 0},
{76, "Old_Egyptian",                22,-130, 110, -13,  0, 0, 0, 0, 0},
{77, "Old_Hawaiian",                7, 61,   -285, -181,0, 0, 0, 0, 0},
{78, "Oman",                        6, -346, -1,   224, 0, 0, 0, 0, 0},
{79, "OSGB_1936",                   9, 375,  -111, 431, 0, 0, 0, 0, 0},
{80, "Pico_De_Las_Nieves",          4, -307, -92,  127, 0, 0, 0, 0, 0},
{81, "Pitcairn_Astro_1967",         4, 185,  165,  42,  0, 0, 0, 0, 0},
{82, "Provisional_South_American",  4, -288, 175,  -376,0, 0, 0, 0, 0},
{83, "Puerto_Rico",                 7, 11,   72,   -101,0, 0, 0, 0, 0},
{84, "Qatar_National",              4, -128, -283, 22,  0, 0, 0, 0, 0},
{85, "Qornoq",                      4, 164,  138, -189, 0, 0, 0, 0, 0},
{86, "Reunion",                     4, 94,   -948,-1262,0, 0, 0, 0, 0},
{87, "Monte_Mario",                 4, -225, -65, 9,    0, 0, 0, 0, 0},
{88, "Santo_Dos",                   4, 170,  42,  84,   0, 0, 0, 0, 0},
{89, "Sao_Braz",                    4, -203, 141, 53,   0, 0, 0, 0, 0},
{90, "Sapper_Hill_1943",            4, -355, 16,  74,   0, 0, 0, 0, 0},
{91, "Schwarzeck",                  14,616,  97,  -251, 0, 0, 0, 0, 0},
{92, "South_American_Datum_1969",   24,-57,  1,   -41,  0, 0, 0, 0, 0},
{93, "South_Asia",                  19,7,    -10, -26,  0, 0, 0, 0, 0},
{94, "Southeast_Base",              4, -499, -249,314,  0, 0, 0, 0, 0},
{95, "Southwest_Base",              4, -104, 167, -38,  0, 0, 0, 0, 0},
{96, "Timbalai_1948",               11,-689, 691, -46,  0, 0, 0, 0, 0},
{97, "Tokyo",                       10,-128, 481, 664,  0, 0, 0, 0, 0},
{98, "Tristan_Astro_1968",          4, -632, 438, -609, 0, 0, 0, 0, 0},
{99, "Viti_Levu_1916",              6, 51,   391, -36,  0, 0, 0, 0, 0},
{100, "Wake_Entiwetok_1960",        23,101,  52,  -39,  0, 0, 0, 0, 0},
{101, "WGS_60",                     26,0,    0,   0,    0, 0, 0, 0, 0},
{102, "WGS_66",                     27,0,    0,   0,    0, 0, 0, 0, 0},
{103, "WGS_1972",                   1, 0,    8,   10,   0, 0, 0, 0, 0},
{104, "WGS_1984",                   28,0,    0,   0,    0, 0, 0, 0, 0},
{105, "Yacare",                     4, -155, 171, 37,   0, 0, 0, 0, 0},
{106, "Zanderij",                   4, -265, 120, -358, 0, 0, 0, 0, 0},
{107, "NTF",                        30,-168, -60, 320,  0, 0, 0, 0, 0},
{108, "European_Datum_1987",        4, -83,  -96, -113, 0, 0, 0, 0, 0},
{109, "Netherlands_Bessel",         10,593,  26,  478,  0, 0, 0, 0, 0},
{110, "Belgium_Hayford",            4, 81,   120, 129,  0, 0, 0, 0, 0},
{111, "NWGL_10",                    1, -1,   15,  1,    0, 0, 0, 0, 0},
{112, "Rikets_koordinatsystem_1990",10,498,  -36, 568,  0, 0, 0, 0, 0},
{113, "Lisboa_DLX",                 4, -303, -62, 105,  0, 0, 0, 0, 0},
{114, "Melrica_1973_D73",           4, -223, 110, 37,   0, 0, 0, 0, 0},
{115, "Euref_98",                   0, 0,    0,   0,    0, 0, 0, 0, 0},
{116, "GDA94",                      0, 0,    0,   0,    0, 0, 0, 0, 0},
{117, "NZGD2000",                   0, 0,    0,   0,    0, 0, 0, 0, 0},
{117, "New_Zealand_Geodetic_Datum_2000",0,0, 0,   0,    0, 0, 0, 0, 0},
{118, "America_Samoa",              7, -115, 118, 426,  0, 0, 0, 0, 0},
{119, "Antigua_Astro_1965",         6, -270, 13,  62,   0, 0, 0, 0, 0},
{120, "Ayabelle_Lighthouse",        6, -79, -129, 145,  0, 0, 0, 0, 0},
{121, "Bukit_Rimpah",               10,-384, 664, -48,  0, 0, 0, 0, 0},
{122, "Estonia_1937",               10,374, 150,  588,  0, 0, 0, 0, 0},
{123, "Dabola",                     6, -83, 37,   124,  0, 0, 0, 0, 0},
{124, "Deception_Island",           6, 260, 12,   -147, 0, 0, 0, 0, 0},
{125, "Fort_Thomas_1955",           6, -7, 215,   225,  0, 0, 0, 0, 0},
{126, "Graciosa_base_1948",         4, -104, 167, -38,  0, 0, 0, 0, 0},
{127, "Herat_North",                4, -333, -222,114,  0, 0, 0, 0, 0},
{128, "Hermanns_Kogel",             10,682, -203, 480,  0, 0, 0, 0, 0},
{129, "Indian",                     50,283, 682,  231,  0, 0, 0, 0, 0},
{130, "Indian_1954",                11,217, 823,  299,  0, 0, 0, 0, 0},
{131, "Indian_1960",                11,198, 881,  317,  0, 0, 0, 0, 0},
{132, "Indian_1975",                11,210, 814,  289,  0, 0, 0, 0, 0},
{133, "Indonesian_Datum_1974",      4, -24, -15,  5,    0, 0, 0, 0, 0},
{134, "ISTS061_Astro_1968",         4, -794, 119, -298, 0, 0, 0, 0, 0},
{135, "Kusaie_Astro_1951",          4, 647, 1777, -1124,0, 0, 0, 0, 0},
{136, "Leigon",                     6, -130, 29,  364,  0, 0, 0, 0, 0},
{137, "Montserrat_Astro_1958",      6, 174, 359,  365,  0, 0, 0, 0, 0},
{138, "Mporaloko",                  6, -74, -130, 42,   0, 0, 0, 0, 0},
{139, "North_Sahara_1959",          6, -186, -93, 310,  0, 0, 0, 0, 0},
{140, "Observatorio_Met_1939",      4, -425, -169,81,   0, 0, 0, 0, 0},
{141, "Point_58",                   6, -106, -129,165,  0, 0, 0, 0, 0},
{142, "Pointe_Noire",               6, -148, 51,  -291, 0, 0, 0, 0, 0},
{143, "Porto_Santo_1936",           4, -499, -249,314,  0, 0, 0, 0, 0},
{144, "Selvagem_Grande_1938",       4, -289, -124,60,   0, 0, 0, 0, 0},
{145, "Sierra_Leone_1960",          6, -88,  4,   101,  0, 0, 0, 0, 0},
{146, "S_JTSK_Ferro",               10, 589, 76,  480,  0, 0, 0, 0, 0},
{147, "Tananarive_1925",            4, -189, -242,-91,  0, 0, 0, 0, 0},
{148, "Voirol_1874",                6, -73,  -247,227,  0, 0, 0, 0, 0},
{149, "Virol_1960",                 6, -123, -206,219,  0, 0, 0, 0, 0},
{150, "Hartebeesthoek94",           0, 0,    0,   0,    0, 0, 0, 0, 0},
{151, "ATS77",                      51, 0, 0, 0, 0, 0, 0, 0, 0},
{152, "JGD2000",                    0, 0, 0, 0, 0, 0, 0, 0, 0},
{1000,"DHDN_Potsdam_Rauenberg",     10,582,  105, 414, -1.04, -0.35, 3.08, 8.3, 0},
{1001,"Pulkovo_1942",               3, 24,   -123, -94, -0.02, 0.25, 0.13, 1.1, 0},
{1002,"NTF_Paris_Meridian",         30,-168, -60, 320, 0, 0, 0, 0, 2.337229166667},
{1003,"Switzerland_CH_1903",        10,660.077,13.551, 369.344, 0.804816, 0.577692, 0.952236, 5.66,0},
{1004,"Hungarian_Datum_1972",       21,-56,  75.77, 15.31, -0.37, -0.2, -0.21, -1.01, 0},
{1005,"Cape_7_Parameter",           28,-134.73,-110.92, -292.66, 0, 0, 0, 1, 0},
{1006,"AGD84_7_Param_Aust",         2, -117.763,-51.51, 139.061, -0.292, -0.443, -0.277, -0.191, 0},
{1007,"AGD66_7_Param_ACT",          2, -129.193,-41.212, 130.73, -0.246, -0.374, -0.329, -2.955, 0},
{1008,"AGD66_7_Param_TAS",          2, -120.271,-64.543, 161.632, -0.2175, 0.0672, 0.1291, 2.4985, 0},
{1009,"AGD66_7_Param_VIC_NSW",      2, -119.353,-48.301, 139.484, -0.415, -0.26, -0.437, -0.613, 0},
{1010,"NZGD_7_Param_49",            4, 59.47, -5.04, 187.44, -0.47, 0.1, -1.024, -4.5993, 0},
{1011,"Rikets_Tri_7_Param_1990",    10,419.3836, 99.3335, 591.3451, -0.850389, -1.817277, 7.862238, -0.99496, 0},
{1012,"Russia_PZ90",                52, -1.08,-0.27,-0.9,0, 0, -0.16,-0.12, 0},
{1013,"Russia_SK42",                52, 23.92,-141.27,-80.9, 0, -0.35,-0.82, -0.12, 0},
{1014,"Russia_SK95",                52, 24.82,-131.21,-82.66,0,0,-0.16,-0.12, 0},
{1015,"Tokyo",                      10, -146.414, 507.337, 680.507,0,0,0,0,0},
{1016,"Finnish_KKJ",                4, -96.062, -82.428, -121.754, -4.801, -0.345, 1.376, 1.496, 0},
{1017,"Xian 1980",					53, 24, -123, -94, -0.02, -0.25, 0.13, 1.1, 0},
{1018,"Lithuanian Pulkovo 1942",	4, -40.59527, -18.54979, -69.33956, -2.508, -1.8319, 2.6114, -4.2991, 0},
{1019,"Belgian 1972 7 Parameter",   4, -99.059, 53.322, -112.486, -0.419, 0.83, -1.885, 0.999999, 0},
{1020,"S-JTSK with Ferro prime meridian", 10, 589, 76, 480, 0, 0, 0, 0, -17.666666666667}, 

{-1, NULL,                          0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/* -------------------------------------------------------------------- */
/*      This table was hand entered from Appendix I of the mapinfo 6    */
/*      manuals.                                                        */
/* -------------------------------------------------------------------- */

MapInfoSpheroidInfo asSpheroidInfoList[] =
{
{ 9,"Airy 1930",                                6377563.396,    299.3249646},
{13,"Airy 1930 (modified for Ireland 1965",     6377340.189,    299.3249646},
{51,"ATS77 (Average Terrestrial System 1977)",  6378135,        298.257},
{ 2,"Australian",                               6378160.0,      298.25},
{10,"Bessel 1841",                              6377397.155,    299.1528128},
{35,"Bessel 1841 (modified for NGO 1948)",      6377492.0176,   299.15281},
{14,"Bessel 1841 (modified for Schwarzeck)",    6377483.865,    299.1528128},
{36,"Clarke 1858",                              6378293.639,    294.26068},
{ 7,"Clarke 1866",                              6378206.4,      294.9786982},
{ 8,"Clarke 1866 (modified for Michigan)",      6378450.047484481,294.9786982},
{ 6,"Clarke 1880",                              6378249.145,    293.465},
{15,"Clarke 1880 (modified for Arc 1950)",      6378249.145326, 293.4663076},
{30,"Clarke 1880 (modified for IGN)",           6378249.2,      293.4660213},
{37,"Clarke 1880 (modified for Jamaica)",       6378249.136,    293.46631},
{16,"Clarke 1880 (modified for Merchich)",      6378249.2,      293.46598},
{38,"Clarke 1880 (modified for Palestine)",     6378300.79,     293.46623},
{39,"Everest (Brunei and East Malaysia)",       6377298.556,    300.8017},
{11,"Everest (India 1830)",                     6377276.345,    300.8017},
{40,"Everest (India 1956)",                     6377301.243,    300.80174},
{50,"Everest (Pakistan)",                       6377309.613,    300.8017},
{17,"Everest (W. Malaysia and Singapore 1948)", 6377304.063,    300.8017},
{48,"Everest (West Malaysia 1969)",             6377304.063,    300.8017},
{18,"Fischer 1960",                             6378166.0,      298.3},
{19,"Fischer 1960 (modified for South Asia)",   6378155.0,      298.3},
{20,"Fischer 1968",                             6378150.0,      298.3},
{21,"GRS 67",                                   6378160.0,      298.247167427},
{ 0,"GRS 80",                                   6378137.0,      298.257222101},
{ 5,"Hayford",                                  6378388.0,      297.0},
{22,"Helmert 1906",                             6378200.0,      298.3},
{23,"Hough",                                    6378270.0,      297.0},
{31,"IAG 75",                                   6378140.0,      298.257222},
{41,"Indonesian",                               6378160.0,      298.247},
{ 4,"International 1924",                       6378388.0,      297.0},
{49,"Irish (WOFO)",                             6377542.178,    299.325},
{ 3,"Krassovsky",                               6378245.0,      298.3},
{32,"MERIT 83",                                 6378137.0,      298.257},
{33,"New International 1967",                   6378157.5,      298.25},
{42,"NWL 9D",                                   6378145.0,      298.25},
{43,"NWL 10D",                                  6378135.0,      298.26},
{44,"OSU86F",                                   6378136.2,      298.25722},
{45,"OSU91A",                                   6378136.3,      298.25722},
{46,"Plessis 1817",                             6376523.0,      308.64},
{52,"PZ90",                                     6378136.0,      298.257839303},
{24,"South American",                           6378160.0,      298.25},
{12,"Sphere",                                   6370997.0,      0.0},
{47,"Struve 1860",                              6378297.0,      294.73},
{34,"Walbeck",                                  6376896.0,      302.78},
{25,"War Office",                               6378300.583,    296.0},
{26,"WGS 60",                                   6378165.0,      298.3},
{27,"WGS 66",                                   6378145.0,      298.25},
{ 1,"WGS 72",                                   6378135.0,      298.26},
{28,"WGS 84",                                   6378137.0,      298.257223563},
{29,"WGS 84 (MAPINFO Datum 0)",                 6378137.01,     298.257223563},
{-1,NULL,                                       0.0,            0.0}
};

/************************************************************************/
/*                             GetMIFParm()                             */
/************************************************************************/

static double GetMIFParm( char ** papszFields, int iField, double dfDefault )

{
    if( iField >= CSLCount(papszFields) )
        return dfDefault;
    else
        return atof(papszFields[iField]);
}

/************************************************************************/
/*                      MITABCoordSys2SpatialRef()                      */
/*                                                                      */
/*      Convert a MIF COORDSYS string into a new OGRSpatialReference    */
/*      object.                                                         */
/************************************************************************/

OGRSpatialReference *MITABCoordSys2SpatialRef( const char * pszCoordSys )

{
    char        **papszFields;
    OGRSpatialReference *poSR;

    if( pszCoordSys == NULL )
        return NULL;
    
/* -------------------------------------------------------------------- */
/*      Parse the passed string into words.                             */
/* -------------------------------------------------------------------- */
    while(*pszCoordSys == ' ') pszCoordSys++;  // Eat leading spaces
    if( EQUALN(pszCoordSys,"CoordSys",8) )
        pszCoordSys += 9;
    
    papszFields = CSLTokenizeStringComplex( pszCoordSys, " ,", TRUE, FALSE );

/* -------------------------------------------------------------------- */
/*      Clip off Bounds information.                                    */
/* -------------------------------------------------------------------- */
    int         iBounds = CSLFindString( papszFields, "Bounds" );

    while( iBounds != -1 && papszFields[iBounds] != NULL )
    {
        CPLFree( papszFields[iBounds] );
        papszFields[iBounds] = NULL;
        iBounds++;
    }

/* -------------------------------------------------------------------- */
/*      Create a spatialreference object to operate on.                 */
/* -------------------------------------------------------------------- */
    poSR = new OGRSpatialReference;

/* -------------------------------------------------------------------- */
/*      Fetch the projection.                                           */
/* -------------------------------------------------------------------- */
    char        **papszNextField;
    int nProjection = 0;

    if( CSLCount( papszFields ) >= 3
        && EQUAL(papszFields[0],"Earth")
        && EQUAL(papszFields[1],"Projection") )
    {
        nProjection = atoi(papszFields[2]);
        papszNextField = papszFields + 3;
    }
    else if (CSLCount( papszFields ) >= 2
             && EQUAL(papszFields[0],"NonEarth") )
    {
        // NonEarth Units "..." Bounds (x, y) (x, y)
        nProjection = 0;
        papszNextField = papszFields + 2;

        if( papszNextField[0] != NULL && EQUAL(papszNextField[0],"Units") )
            papszNextField++;
    }
    else
    {
        // Invalid projection string ???
        if (CSLCount(papszFields) > 0)
            CPLError(CE_Warning, CPLE_IllegalArg,
                     "Failed parsing CoordSys: '%s'", pszCoordSys);
        CSLDestroy(papszFields);
        delete poSR;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Fetch the datum information.                                    */
/* -------------------------------------------------------------------- */
    int         nDatum = 0;
    double      adfDatumParm[8] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
    int         nEllipsoid=0;

    if( nProjection != 0 && CSLCount(papszNextField) > 0 )
    {
        nDatum = atoi(papszNextField[0]);
        papszNextField++;
    }

    if( (nDatum == 999 || nDatum == 9999)
        && CSLCount(papszNextField) >= 4 )
    {
        nEllipsoid = atoi(papszNextField[0]);
        adfDatumParm[0] = atof(papszNextField[1]);
        adfDatumParm[1] = atof(papszNextField[2]);
        adfDatumParm[2] = atof(papszNextField[3]);
        papszNextField += 4;
    }

    if( nDatum == 9999
        && CSLCount(papszNextField) >= 5 )
    {
        adfDatumParm[3] = atof(papszNextField[0]);
        adfDatumParm[4] = atof(papszNextField[1]);
        adfDatumParm[5] = atof(papszNextField[2]);
        adfDatumParm[6] = atof(papszNextField[3]);
        adfDatumParm[7] = atof(papszNextField[4]);
        papszNextField += 5;
    }

/* -------------------------------------------------------------------- */
/*      Fetch the units string.                                         */
/* -------------------------------------------------------------------- */
    const char  *pszMIFUnits = NULL;
    const char  *pszUnitsName = NULL;
    double dfUnitsConv = 1.0;
    
    if( CSLCount(papszNextField) > 0 )
    {
        pszMIFUnits = papszNextField[0];
        papszNextField++;
    }

    if( nProjection == 1 || pszMIFUnits == NULL )
        /* do nothing */;
    else if( EQUAL(pszMIFUnits,"km") )
    {
        pszUnitsName = "Kilometer"; 
        dfUnitsConv = 1000.0;
    }
    else if( EQUAL(pszMIFUnits, "in" ) )
    {
        pszUnitsName = "IINCH"; 
        dfUnitsConv = 0.0254; 
    }
    else if( EQUAL(pszMIFUnits, "ft" ) )
    {
        pszUnitsName = SRS_UL_FOOT;
        dfUnitsConv = CPLAtof(SRS_UL_FOOT_CONV);
    }
    else if( EQUAL(pszMIFUnits, "yd" ) )
    {
        pszUnitsName = "IYARD";
        dfUnitsConv = 0.9144;
    }
    else if( EQUAL(pszMIFUnits, "mm" ) )
    {
        pszUnitsName = "Millimeter";
        dfUnitsConv = 0.001;
    }
    else if( EQUAL(pszMIFUnits, "cm" ) )
    {
        pszUnitsName = "Centimeter";
        dfUnitsConv = 0.01;
    }
    else if( EQUAL(pszMIFUnits, "m" ) )
    {
        pszUnitsName = SRS_UL_METER;
        dfUnitsConv = 1.0;
    }   
    else if( EQUAL(pszMIFUnits, "survey foot" )
             || EQUAL(pszMIFUnits, "survey ft" ) )
    {
        pszUnitsName = SRS_UL_US_FOOT;
        dfUnitsConv = CPLAtof(SRS_UL_US_FOOT_CONV);
    }   
    else if( EQUAL(pszMIFUnits, "nmi" ) )
    {
        pszUnitsName = SRS_UL_NAUTICAL_MILE;
        dfUnitsConv = CPLAtof(SRS_UL_NAUTICAL_MILE_CONV);
    }   
    else if( EQUAL(pszMIFUnits, "li" ) )
    {
        pszUnitsName = SRS_UL_LINK;
        dfUnitsConv = CPLAtof(SRS_UL_LINK_CONV);
    }
    else if( EQUAL(pszMIFUnits, "ch" ) )
    {
        pszUnitsName = SRS_UL_CHAIN;
        dfUnitsConv = CPLAtof(SRS_UL_CHAIN_CONV);
    }   
    else if( EQUAL(pszMIFUnits, "rd" ) )
    {
        pszUnitsName = SRS_UL_ROD;
        dfUnitsConv = CPLAtof(SRS_UL_ROD);
    }   
    else if( EQUAL(pszMIFUnits, "mi" ) )
    {
        pszUnitsName = "Mile";
        dfUnitsConv = 1609.344;
    }

/* -------------------------------------------------------------------- */
/*      Handle the PROJCS style projections, but add the datum          */
/*      later.                                                          */
/*                                                                      */
/*      Note that per GDAL bug 1113 the false easting and north are     */
/*      in local units, not necessarily meters.                         */
/* -------------------------------------------------------------------- */
    int nBaseProjection = nProjection;
    if (nBaseProjection>=3000) nBaseProjection -=3000;
    else if (nBaseProjection>=2000) nBaseProjection -=2000;
    else if (nBaseProjection>=1000) nBaseProjection -=1000;
    switch( nBaseProjection )
    {
        /*--------------------------------------------------------------
         * NonEarth ... we return with an empty SpatialRef.  Eventually
         * we might want to include the units, but not for now.
         *
         * __TODO__ Changed to return NULL because returning an empty
         * SpatialRef caused confusion between Latlon and NonEarth since
         * empty SpatialRefs do have a GEOGCS set and makes them look like
         * Lat/Lon SpatialRefs.
         *
         * Ideally we would like to return a SpatialRef whith no GEGOCS
         *-------------------------------------------------------------*/
      case 0:
        poSR->SetLocalCS( "Nonearth" );
        break;

        /*--------------------------------------------------------------
         * lat/long .. just add the GEOGCS later.
         *-------------------------------------------------------------*/
      case 1:
        break;

        /*--------------------------------------------------------------
         * Cylindrical Equal Area
         *-------------------------------------------------------------*/
      case 2:
        poSR->SetCEA( GetMIFParm( papszNextField, 1, 0.0 ),
                      GetMIFParm( papszNextField, 0, 0.0 ),
                      GetMIFParm( papszNextField, 2, 0.0 ),
                      GetMIFParm( papszNextField, 3, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Lambert Conic Conformal
         *-------------------------------------------------------------*/
      case 3:
        poSR->SetLCC( GetMIFParm( papszNextField, 2, 0.0 ),
                      GetMIFParm( papszNextField, 3, 0.0 ),
                      GetMIFParm( papszNextField, 1, 0.0 ),
                      GetMIFParm( papszNextField, 0, 0.0 ),
                      GetMIFParm( papszNextField, 4, 0.0 ),
                      GetMIFParm( papszNextField, 5, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Lambert Azimuthal Equal Area
         *-------------------------------------------------------------*/
      case 4: 
      case 29:
        poSR->SetLAEA( GetMIFParm( papszNextField, 1, 0.0 ),
                       GetMIFParm( papszNextField, 0, 0.0 ),
                       0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Azimuthal Equidistant 
         *-------------------------------------------------------------*/
      case 5:  /* polar aspect only */
      case 28: /* all aspects */
        poSR->SetAE( GetMIFParm( papszNextField, 1, 0.0 ),
                     GetMIFParm( papszNextField, 0, 0.0 ),
                     0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Equidistant Conic
         *-------------------------------------------------------------*/
      case 6:
        poSR->SetEC( GetMIFParm( papszNextField, 2, 0.0 ),
                     GetMIFParm( papszNextField, 3, 0.0 ),
                     GetMIFParm( papszNextField, 1, 0.0 ),
                     GetMIFParm( papszNextField, 0, 0.0 ),
                     GetMIFParm( papszNextField, 4, 0.0 ),
                     GetMIFParm( papszNextField, 5, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Hotine Oblique Mercator
         *-------------------------------------------------------------*/
      case 7:
        poSR->SetHOM( GetMIFParm( papszNextField, 1, 0.0 ),
                      GetMIFParm( papszNextField, 0, 0.0 ),
                      GetMIFParm( papszNextField, 2, 0.0 ),
                      90.0,
                      GetMIFParm( papszNextField, 3, 1.0 ),
                      GetMIFParm( papszNextField, 4, 0.0 ),
                      GetMIFParm( papszNextField, 5, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Transverse Mercator
         *-------------------------------------------------------------*/
      case 8:
        poSR->SetTM( GetMIFParm( papszNextField, 1, 0.0 ),
                     GetMIFParm( papszNextField, 0, 0.0 ),
                     GetMIFParm( papszNextField, 2, 1.0 ),
                     GetMIFParm( papszNextField, 3, 0.0 ),
                     GetMIFParm( papszNextField, 4, 0.0 ) );
        break;

        /*----------------------------------------------------------------
         * Transverse Mercator,(modified for Danish System 34 Jylland-Fyn)
         *---------------------------------------------------------------*/
      case 21:
        poSR->SetTMVariant( SRS_PT_TRANSVERSE_MERCATOR_MI_21,
                            GetMIFParm( papszNextField, 1, 0.0 ),
                            GetMIFParm( papszNextField, 0, 0.0 ),
                            GetMIFParm( papszNextField, 2, 1.0 ),
                            GetMIFParm( papszNextField, 3, 0.0 ),
                            GetMIFParm( papszNextField, 4, 0.0 ));
        break;

        /*--------------------------------------------------------------
         * Transverse Mercator,(modified for Danish System 34 Sjaelland)
         *-------------------------------------------------------------*/
      case 22:
        poSR->SetTMVariant( SRS_PT_TRANSVERSE_MERCATOR_MI_22,
                            GetMIFParm( papszNextField, 1, 0.0 ),
                            GetMIFParm( papszNextField, 0, 0.0 ),
                            GetMIFParm( papszNextField, 2, 1.0 ),
                            GetMIFParm( papszNextField, 3, 0.0 ),
                            GetMIFParm( papszNextField, 4, 0.0 ));
        break;

        /*----------------------------------------------------------------
         * Transverse Mercator,(modified for Danish System 34/45 Bornholm)
         *---------------------------------------------------------------*/
      case 23:
        poSR->SetTMVariant( SRS_PT_TRANSVERSE_MERCATOR_MI_23,
                            GetMIFParm( papszNextField, 1, 0.0 ),
                            GetMIFParm( papszNextField, 0, 0.0 ),
                            GetMIFParm( papszNextField, 2, 1.0 ),
                            GetMIFParm( papszNextField, 3, 0.0 ),
                            GetMIFParm( papszNextField, 4, 0.0 ));
        break;

        /*--------------------------------------------------------------
         * Transverse Mercator,(modified for Finnish KKJ)
         *-------------------------------------------------------------*/
      case 24:
        poSR->SetTMVariant( SRS_PT_TRANSVERSE_MERCATOR_MI_24,
                            GetMIFParm( papszNextField, 1, 0.0 ),
                            GetMIFParm( papszNextField, 0, 0.0 ),
                            GetMIFParm( papszNextField, 2, 1.0 ),
                            GetMIFParm( papszNextField, 3, 0.0 ),
                            GetMIFParm( papszNextField, 4, 0.0 ));
        break;

        /*--------------------------------------------------------------
         * Albers Conic Equal Area
         *-------------------------------------------------------------*/
      case 9:
        poSR->SetACEA( GetMIFParm( papszNextField, 2, 0.0 ),
                       GetMIFParm( papszNextField, 3, 0.0 ),
                       GetMIFParm( papszNextField, 1, 0.0 ),
                       GetMIFParm( papszNextField, 0, 0.0 ),
                       GetMIFParm( papszNextField, 4, 0.0 ),
                       GetMIFParm( papszNextField, 5, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Mercator
         *-------------------------------------------------------------*/
      case 10:
        poSR->SetMercator( 0.0, GetMIFParm( papszNextField, 0, 0.0 ),
                           1.0, 0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Miller Cylindrical
         *-------------------------------------------------------------*/
      case 11:
        poSR->SetMC( 0.0, GetMIFParm( papszNextField, 0, 0.0 ),
                     0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Robinson
         *-------------------------------------------------------------*/
      case 12:
        poSR->SetRobinson( GetMIFParm( papszNextField, 0, 0.0 ),
                           0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Mollweide
         *-------------------------------------------------------------*/
      case 13:
        poSR->SetMollweide( GetMIFParm( papszNextField, 0, 0.0 ),
                            0.0, 0.0 );

        /*--------------------------------------------------------------
         * Eckert IV
         *-------------------------------------------------------------*/
      case 14:
        poSR->SetEckertIV( GetMIFParm( papszNextField, 0, 0.0 ),
                           0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Eckert VI
         *-------------------------------------------------------------*/
      case 15:
        poSR->SetEckertVI( GetMIFParm( papszNextField, 0, 0.0 ),
                           0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Sinusoidal
         *-------------------------------------------------------------*/
      case 16:
        poSR->SetSinusoidal( GetMIFParm( papszNextField, 0, 0.0 ),
                             0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Gall
         *-------------------------------------------------------------*/
      case 17:
        poSR->SetGS( GetMIFParm( papszNextField, 0, 0.0 ),
                     0.0, 0.0 );
        break;
        
        /*--------------------------------------------------------------
         * New Zealand Map Grid
         *-------------------------------------------------------------*/
      case 18:
        poSR->SetNZMG( GetMIFParm( papszNextField, 1, 0.0 ),
                       GetMIFParm( papszNextField, 0, 0.0 ),
                       GetMIFParm( papszNextField, 2, 0.0 ),
                       GetMIFParm( papszNextField, 3, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Lambert Conic Conformal (Belgium)
         *-------------------------------------------------------------*/
      case 19:
        poSR->SetLCCB( GetMIFParm( papszNextField, 2, 0.0 ),
                       GetMIFParm( papszNextField, 3, 0.0 ),
                       GetMIFParm( papszNextField, 1, 0.0 ),
                       GetMIFParm( papszNextField, 0, 0.0 ),
                       GetMIFParm( papszNextField, 4, 0.0 ),
                       GetMIFParm( papszNextField, 5, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Stereographic
         *-------------------------------------------------------------*/
      case 20:
      case 31: /* double stereographic */
        poSR->SetStereographic( 
            GetMIFParm( papszNextField, 1, 0.0 ),
            GetMIFParm( papszNextField, 0, 0.0 ),
            GetMIFParm( papszNextField, 2, 1.0 ),
            GetMIFParm( papszNextField, 3, 0.0 ),
            GetMIFParm( papszNextField, 4, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Swiss Oblique Mercator / Cylindrical
         *-------------------------------------------------------------*/
      case 25:
        poSR->SetSOC( GetMIFParm( papszNextField, 1, 0.0 ),
                      GetMIFParm( papszNextField, 0, 0.0 ),
                      GetMIFParm( papszNextField, 2, 0.0 ),
                      GetMIFParm( papszNextField, 3, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Regional Mercator
         *-------------------------------------------------------------*/
      case 26:
        poSR->SetMercator( GetMIFParm( papszNextField, 1, 0.0 ), 
                           GetMIFParm( papszNextField, 0, 0.0 ),
                           1.0, 0.0, 0.0 );
        break;

        /*--------------------------------------------------------------
         * Polygonic
         *-------------------------------------------------------------*/
      case 27:
        poSR->SetPolyconic( GetMIFParm( papszNextField, 1, 0.0 ), 
                            GetMIFParm( papszNextField, 0, 0.0 ),
                          GetMIFParm( papszNextField, 2, 0.0 ),
                          GetMIFParm( papszNextField, 3, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * CassiniSoldner
         *-------------------------------------------------------------*/
      case 30:
        poSR->SetCS( 
            GetMIFParm( papszNextField, 1, 0.0 ),
            GetMIFParm( papszNextField, 0, 0.0 ),
            GetMIFParm( papszNextField, 2, 0.0 ),
            GetMIFParm( papszNextField, 3, 0.0 ) );
        break;

        /*--------------------------------------------------------------
         * Krovak
         *-------------------------------------------------------------*/
       case 32:
         poSR->SetKrovak( GetMIFParm( papszNextField, 1, 0.0 ),  // dfCenterLat
                          GetMIFParm( papszNextField, 0, 0.0 ),  // dfCenterLong
                          GetMIFParm( papszNextField, 3, 1.0 ),  // dfAzimuth
                          GetMIFParm( papszNextField, 2, 0.0 ),  // dfPseudoStdParallelLat
                          1.0,									  // dfScale
                          GetMIFParm( papszNextField, 4, 0.0 ),  // dfFalseEasting
                          GetMIFParm( papszNextField, 5, 0.0 )); // dfFalseNorthing
         break;

      default:
        break;
    }

/* -------------------------------------------------------------------- */
/*      Set linear units.                                               */
/* -------------------------------------------------------------------- */
    if( pszUnitsName != NULL )
        poSR->SetLinearUnits( pszUnitsName, dfUnitsConv );

/* -------------------------------------------------------------------- */
/*      For Non-Earth projection, we're done at this point.             */
/* -------------------------------------------------------------------- */
    if (nProjection == 0)
    {
        CSLDestroy(papszFields);
        return poSR;
    }

/* ==================================================================== */
/*      Establish the GeogCS                                            */
/* ==================================================================== */
    const char *pszGeogName = "unnamed";
    const char *pszSpheroidName = "GRS_1980";
    double      dfSemiMajor = 6378137.0;
    double      dfInvFlattening = 298.257222101;
    const char *pszPrimeM = "Greenwich";
    double      dfPMLongToGreenwich = 0.0;

/* -------------------------------------------------------------------- */
/*      Find the datum, and collect it's parameters if possible.        */
/* -------------------------------------------------------------------- */
    int         iDatum;
    MapInfoDatumInfo *psDatumInfo = NULL;
    
    for( iDatum = 0; asDatumInfoList[iDatum].nMapInfoDatumID != -1; iDatum++ )
    {
        if( asDatumInfoList[iDatum].nMapInfoDatumID == nDatum )
        {
            psDatumInfo = asDatumInfoList + iDatum;
            break;
        }
    }

    if( asDatumInfoList[iDatum].nMapInfoDatumID == -1
        && nDatum != 999 && nDatum != 9999 )
    {
        /* use WGS84 */
        psDatumInfo = asDatumInfoList + 0;
    }

    if( psDatumInfo != NULL )
    {
        nEllipsoid = psDatumInfo->nEllipsoid;
        adfDatumParm[0] =  psDatumInfo->dfShiftX;
        adfDatumParm[1] = psDatumInfo->dfShiftY;
        adfDatumParm[2] = psDatumInfo->dfShiftZ;
        adfDatumParm[3] = psDatumInfo->dfDatumParm0;
        adfDatumParm[4] = psDatumInfo->dfDatumParm1;
        adfDatumParm[5] = psDatumInfo->dfDatumParm2;
        adfDatumParm[6] = psDatumInfo->dfDatumParm3;
        adfDatumParm[7] = psDatumInfo->dfDatumParm4;
    }
    
/* -------------------------------------------------------------------- */
/*      Set the spheroid if it is known from the table.                 */
/* -------------------------------------------------------------------- */
    for( int i = 0; asSpheroidInfoList[i].nMapInfoId != -1; i++ )
    {
        if( asSpheroidInfoList[i].nMapInfoId == nEllipsoid )
        {
            dfSemiMajor = asSpheroidInfoList[i].dfA;
            dfInvFlattening = asSpheroidInfoList[i].dfInvFlattening;
            pszSpheroidName = asSpheroidInfoList[i].pszMapinfoName;
            break;
        }
    }

/* -------------------------------------------------------------------- */
/*      apply datum parameters.                                         */
/* -------------------------------------------------------------------- */
    char        szDatumName[128];

    if( nDatum == 999 )
    {
        sprintf( szDatumName,
                 "MIF 9999,%d,%.15g,%.15g,%.15g",
                 nEllipsoid,
                 adfDatumParm[0],
                 adfDatumParm[1],
                 adfDatumParm[2] );
    }
    else if( nDatum == 9999 )
    {
        sprintf( szDatumName,
                 "MIF 9999,%d,%.15g,%.15g,%.15g,%.15g,%.15g,%.15g,%.15g,%.15g",
                 nEllipsoid,
                 adfDatumParm[0],
                 adfDatumParm[1],
                 adfDatumParm[2],
                 adfDatumParm[3],
                 adfDatumParm[4],
                 adfDatumParm[5],
                 adfDatumParm[6],
                 adfDatumParm[7] );
    }
    else if( psDatumInfo->pszOGCDatumName != NULL
             && strlen(psDatumInfo->pszOGCDatumName) > 0 )
    {
        strncpy( szDatumName, psDatumInfo->pszOGCDatumName,
                 sizeof(szDatumName) );
    }
    else
    {
        sprintf( szDatumName, "MIF %d", nDatum );
    }

/* -------------------------------------------------------------------- */
/*      Set prime meridian for 9999 datums.                             */
/* -------------------------------------------------------------------- */
    if( nDatum == 9999 || adfDatumParm[7] != 0.0 )
    {
        pszPrimeM = "non-Greenwich";
        dfPMLongToGreenwich = adfDatumParm[7];
    }

/* -------------------------------------------------------------------- */
/*      Set the GeogCS.                                                 */
/* -------------------------------------------------------------------- */
    poSR->SetGeogCS( pszGeogName, szDatumName, pszSpheroidName,
                     dfSemiMajor, dfInvFlattening,
                     pszPrimeM, dfPMLongToGreenwich,
                     SRS_UA_DEGREE,
                     CPLAtof(SRS_UA_DEGREE_CONV) );

    poSR->SetTOWGS84( adfDatumParm[0], adfDatumParm[1], adfDatumParm[2],
                      -adfDatumParm[3], -adfDatumParm[4], -adfDatumParm[5], 
                      adfDatumParm[6] );

/* -------------------------------------------------------------------- */
/*      Report on translation.                                          */
/* -------------------------------------------------------------------- */
    char        *pszWKT;

    poSR->exportToWkt( &pszWKT );
    if( pszWKT != NULL )
    {
        CPLDebug( "MITAB",
                  "This CoordSys value:\n%s\nwas translated to:\n%s\n",
                  pszCoordSys, pszWKT );
        CPLFree( pszWKT );
    }

    CSLDestroy(papszFields);

    return poSR;
}
#endif
