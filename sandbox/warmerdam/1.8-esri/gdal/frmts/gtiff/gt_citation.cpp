/******************************************************************************
 * $Id$
 *
 * Project:  GeoTIFF Driver
 * Purpose:  Implements special parsing of Imagine citation strings, and
 *           to encode PE String info in citation fields as needed.
 * Author:   Xiuguang Zhou (ESRI)
 *
 ******************************************************************************
 * Copyright (c) 2008, Xiuguang Zhou (ESRI)
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

#include "cpl_port.h"
#include "cpl_string.h"

#include "geovalues.h"
#include "gt_citation.h"

CPL_CVSID("$Id$");

#define nCitationNameTypes 9
typedef enum 
{
  CitCsName = 0,
  CitPcsName = 1,
  CitProjectionName = 2,
  CitLUnitsName = 3,
  CitGcsName = 4,
  CitDatumName = 5,
  CitEllipsoidName = 6,
  CitPrimemName = 7,
  CitAUnitsName = 8
} CitationNameType;

static const char *apszUnitMap[] = {
    "meters", "1.0",
    "meter", "1.0",
    "m", "1.0",
    "centimeters", "0.01",
    "centimeter", "0.01",
    "cm", "0.01", 
    "millimeters", "0.001",
    "millimeter", "0.001",
    "mm", "0.001",
    "kilometers", "1000.0",
    "kilometer", "1000.0",
    "km", "1000.0",
    "us_survey_feet", "0.3048006096012192",
    "us_survey_foot", "0.3048006096012192",
    "feet", "0.3048006096012192", 
    "foot", "0.3048006096012192",
    "ft", "0.3048006096012192",
    "international_feet", "0.3048",
    "international_foot", "0.3048",
    "inches", "0.0254000508001",
    "inch", "0.0254000508001",
    "in", "0.0254000508001",
    "yards", "0.9144",
    "yard", "0.9144",
    "yd", "0.9144",
    "miles", "1304.544",
    "mile", "1304.544",
    "mi", "1304.544",
    "modified_american_feet", "0.3048122530",
    "modified_american_foot", "0.3048122530",
    "clarke_feet", "0.3047972651",
    "clarke_foot", "0.3047972651",
    "indian_feet", "0.3047995142",
    "indian_foot", "0.3047995142",
    "Yard_Indian", "0.9143985307444408", 
    "Foot_Clarke", "0.30479726540",
    "Foot_Gold_Coast", "0.3047997101815088",
    "Link_Clarke", "0.2011661951640", 
    "Yard_Sears", "0.9143984146160287", 
    "50_Kilometers", "50000.0", 
    "150_Kilometers", "150000.0", 
    NULL, NULL
};

char* ImagineCitationTranslation(char* psCitation, geokey_t keyID);
char** CitationStringParse(char* psCitation, geokey_t keyID);

/************************************************************************/
/*                     ImagineCitationTranslation()                     */
/*                                                                      */
/*      Translate ERDAS Imagine GeoTif citation                         */
/************************************************************************/
char* ImagineCitationTranslation(char* psCitation, geokey_t keyID)
{
/*
    char* ret = NULL;
    if(!psCitation)
        return ret;
    if(EQUALN(psCitation, "IMAGINE GeoTIFF Support", strlen("IMAGINE GeoTIFF Support")))
    {
        CPLString osName;

        // this is a handle IMAGING style citation
        const char* p = NULL;
        p = strchr(psCitation, '$');
        if(p)
            p = strchr(p, '\n');
        if(p)
            p++;
        const char* p1 = NULL;
        if(p)
            p1 = strchr(p, '\n');
        if(p && p1)
        {
            switch (keyID)
            {
              case PCSCitationGeoKey:
                osName = "PCS Name = ";
                break;
              case GTCitationGeoKey:
                osName = "CS Name = ";
                break;
              case GeogCitationGeoKey:
                if(!strstr(p, "Unable to"))
                    osName = "GCS Name = ";
                break;
              default:
                break;
            }
            if(strlen(osName)>0)
            {
                osName.append(p, p1-p);
                osName += "|";
            }
        }
        p = strstr(psCitation, "Projection Name = ");
        if(p)
        {
            p += strlen("Projection Name = ");
            p1 = strchr(p, '\n');
            if(!p1)
                p1 = strchr(p, '\0');
        }
        if(p && p1)
        {
            osName.append(p, p1-p);
            osName += "|";
        }
        p = strstr(psCitation, "Datum = ");
        if(p)
        {
            p += strlen("Datum = ");
            p1 = strchr(p, '\n');
            if(!p1)
                p1 = strchr(p, '\0');
        }
        if(p && p1)
        {
            osName += "Datum = ";
            osName.append(p, p1-p);
            osName += "|";
        }
        p = strstr(psCitation, "Ellipsoid = ");
        if(p)
        {
            p += strlen("Ellipsoid = ");
            p1 = strchr(p, '\n');
            if(!p1)
                p1 = strchr(p, '\0');
        }
        if(p && p1)
        {
            osName += "Ellipsoid = ";
            osName.append(p, p1-p);
            osName += "|";
        }
        p = strstr(psCitation, "Units = ");
        if(p)
        {
            p += strlen("Units = ");
            p1 = strchr(p, '\n');
            if(!p1)
                p1 = strchr(p, '\0');
        }
        if(p && p1)
        {
            osName += "LUnits = ";
            osName.append(p, p1-p);
            osName += "|";
        }
        if(strlen(osName) > 0)
        {
            ret = CPLStrdup(osName);
        }
    }
    return ret;
*/
  static const char *keyNames[] = {
    "NAD = ", "Datum = ", "Ellipsoid = ", "Units = ", NULL
  };

  char* ret = NULL;
  int adjustLen = 0, i;
  if(!psCitation)
    return ret;
  if(EQUALN(psCitation, "IMAGINE GeoTIFF Support", strlen("IMAGINE GeoTIFF Support")))
  {
    // this is a handle IMAGING style citation
    char name[256];
    name[0] = '\0';
    char* p = NULL;
    char* p1 = NULL;

    p = strchr(psCitation, '$');
    if( p && strchr(p, '\n') )
      p = strchr(p, '\n') + 1;
    if(p)
    {
      p1 = p + strlen(p);
      char *p2 = strchr(p, '\n');
      if(p2)
        p1 = MIN(p1, p2);
      p2 = strchr(p, '\0');
      if(p2)
        p1 = MIN(p1, p2);
      for(i=0; keyNames[i]!=NULL; i++)
      {
        p2 = strstr(p, keyNames[i]);
        if(p2)
          p1 = MIN(p1, p2);
      }
    }

    // PCS name, GCS name and PRJ name
    if(p && p1)
    {
      switch (keyID)
      {
        case PCSCitationGeoKey:
          if(strstr(psCitation, "Projection = "))
            strcpy(name, "PRJ Name = ");
          else
            strcpy(name, "PCS Name = ");
        break;
        case GTCitationGeoKey:
          strcpy(name, "PCS Name = ");
        break;
        case GeogCitationGeoKey:
          if(!strstr(p, "Unable to"))
            strcpy(name, "GCS Name = ");
        break;
      }
      if(strlen(name)>0)
      {
        char* p2;
        if((p2 = strstr(psCitation, "Projection Name = ")) != 0)
          p = p2 + strlen("Projection Name = ");
        if((p2 = strstr(psCitation, "Projection = ")) != 0)
          p = p2 + strlen("Projection = ");
        if(p1[0] == '\0' || p1[0] == '\n' || p1[0] == ' ')
          p1 --;
        p2 = p1 - 1;
        while( p2>0 && (p2[0] == ' ' || p2[0] == '\0' || p2[0] == '\n') )
          p2--;
        if(p2 != p1 - 1)
          p1 = p2;
        if(p1 >= p)
        {
          strncat(name, p, p1-p+1);
          strcat(name, "|");
          name[strlen(name)] = '\0';
        }
      }
    }

    // All other parameters
    for(i=0; keyNames[i]!=NULL; i++)
    {
      p = strstr(psCitation, keyNames[i]);
      if(p)
      {
        p += strlen(keyNames[i]);
        p1 = p + strlen(p);
        char *p2 = strchr(p, '\n');
        if(p2)
          p1 = MIN(p1, p2);
        p2 = strchr(p, '\0');
        if(p2)
          p1 = MIN(p1, p2);
        for(int j=0; keyNames[j]!=NULL; j++)
        {
          p2 = strstr(p, keyNames[j]);
          if(p2)
            p1 = MIN(p1, p2);
        }
      }
      if(p && p1 && p1>p)
      {
        if(EQUAL(keyNames[i], "Units = "))
          strcat(name, "LUnits = ");
        else
          strcat(name, keyNames[i]);
        if(p1[0] == '\0' || p1[0] == '\n' || p1[0] == ' ')
          p1 --;
        char* p2 = p1 - 1;
        while( p2>0 && (p2[0] == ' ' || p2[0] == '\0' || p2[0] == '\n') )
          p2--;
        if(p2 != p1 - 1)
          p1 = p2;
        if(p1 >= p)
        {
          strncat(name, p, p1-p+1);
          strcat(name, "|");
          name[strlen(name)] = '\0';
        }
      }
    }
    if(strlen(name) > 0)
      ret = CPLStrdup(name);
  }
  return ret;

}

/************************************************************************/
/*                        CitationStringParse()                         */
/*                                                                      */
/*      Parse a Citation string                                         */
/************************************************************************/
/*
char** CitationStringParse(const char* psCitation)
{
    char ** ret = NULL;
    if(!psCitation)
        return ret;

    ret = (char **) CPLCalloc(sizeof(char*), nCitationNameTypes); 
    const char* pDelimit = NULL;
    const char* pStr = psCitation;
    CPLString osName;
    int nCitationLen = strlen(psCitation);
    OGRBoolean nameFound = FALSE;
    while((pStr-psCitation+1)< nCitationLen)
    {
        if( (pDelimit = strstr(pStr, "|")) != NULL )
        {
            osName = "";
            osName.append(pStr, pDelimit-pStr);
            pStr = pDelimit+1;
        }
        else
        {
            osName = pStr;
            pStr += strlen(pStr);
        }
        const char* name = osName.c_str();
        if( strstr(name, "PCS Name = ") )
        {
            if (!ret[CitPcsName])
                ret[CitPcsName] = CPLStrdup(name+strlen("PCS Name = "));
            nameFound = TRUE;
        }
        if(strstr(name, "Projection Name = "))
        {
            if (!ret[CitProjectionName])
                ret[CitProjectionName] = CPLStrdup(name+strlen("Projection Name = "));
            nameFound = TRUE;
        }
        if(strstr(name, "LUnits = "))
        {
            if (!ret[CitLUnitsName])
                ret[CitLUnitsName] = CPLStrdup(name+strlen("LUnits = "));
            nameFound = TRUE;
        }
        if(strstr(name, "GCS Name = "))
        {
            if (!ret[CitGcsName])
                ret[CitGcsName] = CPLStrdup(name+strlen("GCS Name = "));
            nameFound = TRUE;
        }
        if(strstr(name, "Datum = "))
        {
            if (!ret[CitDatumName])
                ret[CitDatumName] = CPLStrdup(name+strlen("Datum = "));
            nameFound = TRUE;
        }
        if(strstr(name, "Ellipsoid = "))
        {
            if (!ret[CitEllipsoidName])
                ret[CitEllipsoidName] = CPLStrdup(name+strlen("Ellipsoid = "));
            nameFound = TRUE;
        }
        if(strstr(name, "Primem = "))
        {
            if (!ret[CitPrimemName])
                ret[CitPrimemName] = CPLStrdup(name+strlen("Primem = "));
            nameFound = TRUE;
        }
        if(strstr(name, "AUnits = "))
        {
            if (!ret[CitAUnitsName])
                ret[CitAUnitsName] = CPLStrdup(name+strlen("AUnits = "));
            nameFound = TRUE;
        }
    }
    if(!nameFound)
    {
        CPLFree( ret );
        ret = (char**)NULL;
    }
    return ret;
}
*/
char** CitationStringParse(char* psCitation, geokey_t keyID)
{
  char ** ret = NULL;
  if(!psCitation)
    return ret;

  ret = (char **) CPLCalloc(sizeof(char*), nCitationNameTypes); 
  char* pDelimit = NULL;
  char* pStr = psCitation;
  char name[512];
  int nameLen = strlen(psCitation);
  OGRBoolean nameFound = FALSE;
  while((pStr-psCitation+1)< nameLen)
  {
    if( pDelimit = strstr(pStr, "|"))
    {
      strncpy( name, pStr, pDelimit-pStr );
      name[pDelimit-pStr] = '\0';
      pStr = pDelimit+1;
    }
    else
    {
      strcpy (name, pStr);
      pStr += strlen(pStr);
    }
    if( strstr(name, "PCS Name = ") )
    {
      ret[CitPcsName] = CPLStrdup(name+strlen("PCS Name = "));
      nameFound = TRUE;
    }
    if(strstr(name, "PRJ Name = "))
    {
      ret[CitProjectionName] = CPLStrdup(name+strlen("PRJ Name = "));
      nameFound = TRUE;
    }
    if(strstr(name, "LUnits = "))
    {
      ret[CitLUnitsName] = CPLStrdup(name+strlen("LUnits = "));
      nameFound = TRUE;
    }
    if(strstr(name, "GCS Name = "))
    {
      ret[CitGcsName] = CPLStrdup(name+strlen("GCS Name = "));
      nameFound = TRUE;
    }
    if(strstr(name, "Datum = "))
    {
      ret[CitDatumName] = CPLStrdup(name+strlen("Datum = "));
      nameFound = TRUE;
    }
    if(strstr(name, "Ellipsoid = "))
    {
      ret[CitEllipsoidName] = CPLStrdup(name+strlen("Ellipsoid = "));
      nameFound = TRUE;
    }
    if(strstr(name, "Primem = "))
    {
      ret[CitPrimemName] = CPLStrdup(name+strlen("Primem = "));    
      nameFound = TRUE;
    }
    if(strstr(name, "AUnits = "))
    {
      ret[CitAUnitsName] = CPLStrdup(name+strlen("AUnits = "));
      nameFound = TRUE;
    }
  }
  if( !nameFound && keyID == GeogCitationGeoKey )
  {
    ret[CitGcsName] = CPLStrdup(name);
    nameFound = TRUE;
  }
  if(!nameFound)
  {
    CPLFree( ret );
    ret = (char**)NULL;
  }
  return ret;
}


/************************************************************************/
/*                       SetLinearUnitCitation()                        */
/*                                                                      */
/*      Set linear unit Citation string                                 */
/************************************************************************/
void SetLinearUnitCitation(GTIF* psGTIF, char* pszLinearUOMName)
{
    char szName[512];
    CPLString osCitation;
    int n = 0;
    if( GTIFKeyGet( psGTIF, PCSCitationGeoKey, szName, 0, sizeof(szName) ) )
        n = strlen(szName);
    if(n>0)
    {
        osCitation = szName;
        if(osCitation[n-1] != '|')
            osCitation += "|";
        osCitation += "LUnits = ";
        osCitation += pszLinearUOMName;
        osCitation += "|";
    }
    else
    {
        osCitation = "LUnits = ";
        osCitation += pszLinearUOMName;
    }
    GTIFKeySet( psGTIF, PCSCitationGeoKey, TYPE_ASCII, 0, osCitation.c_str() );
    return;
}

/************************************************************************/
/*                         SetGeogCSCitation()                          */
/*                                                                      */
/*      Set geogcs Citation string                                      */
/************************************************************************/
void SetGeogCSCitation(GTIF * psGTIF, OGRSpatialReference *poSRS, char* angUnitName, int nDatum, short nSpheroid)
{
    int bRewriteGeogCitation = FALSE;
    char szName[256];
    CPLString osCitation;
    size_t n = 0;
    if( GTIFKeyGet( psGTIF, GeogCitationGeoKey, szName, 0, sizeof(szName) ) )
        n = strlen(szName);
    if (n == 0)
        return;

    if(!EQUALN(szName, "GCS Name = ", strlen("GCS Name = ")))
    {
        osCitation = "GCS Name = ";
        osCitation += szName;
    }
    else
    {
        osCitation = szName;
    }

    if(nDatum == KvUserDefined )
    {
        const char* datumName = poSRS->GetAttrValue( "DATUM" );
        if(datumName && strlen(datumName) > 0)
        {
            osCitation += "|Datum = ";
            osCitation += datumName;
            bRewriteGeogCitation = TRUE;
        }
    }
    if(nSpheroid == KvUserDefined )
    {
        const char* spheroidName = poSRS->GetAttrValue( "SPHEROID" );
        if(spheroidName && strlen(spheroidName) > 0)
        {
            osCitation += "|Ellipsoid = ";
            osCitation += spheroidName;
            bRewriteGeogCitation = TRUE;
        }
    }

    const char* primemName = poSRS->GetAttrValue( "PRIMEM" );
    if(primemName && strlen(primemName) > 0)
    {
        osCitation += "|Primem = ";
        osCitation += primemName;
        bRewriteGeogCitation = TRUE;

        double primemValue = poSRS->GetPrimeMeridian(NULL);
        if(angUnitName && !EQUAL(angUnitName, "Degree"))
        {
            double aUnit = poSRS->GetAngularUnits(NULL);
            primemValue *= aUnit;
        }
        GTIFKeySet( psGTIF, GeogPrimeMeridianLongGeoKey, TYPE_DOUBLE, 1, 
                    primemValue );
    } 
    if(angUnitName && strlen(angUnitName) > 0 && !EQUAL(angUnitName, "Degree"))
    {
        osCitation += "|AUnits = ";
        osCitation += angUnitName;
        bRewriteGeogCitation = TRUE;
    }

    if (osCitation[strlen(osCitation) - 1] != '|')
        osCitation += "|";

    if (bRewriteGeogCitation)
        GTIFKeySet( psGTIF, GeogCitationGeoKey, TYPE_ASCII, 0, osCitation.c_str() );

    return;
}

/************************************************************************/
/*                          SetCitationToSRS()                          */
/*                                                                      */
/*      Parse and set Citation string to SRS                            */
/************************************************************************/
OGRBoolean SetCitationToSRS(GTIF* hGTIF, char* szCTString, int nCTStringLen,
                            geokey_t geoKey,  OGRSpatialReference*	poSRS, OGRBoolean* linearUnitIsSet)
{
/*
    OGRBoolean ret = FALSE;
    *linearUnitIsSet = FALSE;
    char* imgCTName = ImagineCitationTranslation(szCTString, geoKey);
    if(imgCTName)
    {
        strncpy(szCTString, imgCTName, nCTStringLen);
        szCTString[nCTStringLen-1] = '\0';
        CPLFree( imgCTName );
    }
    char** ctNames = CitationStringParse(szCTString);
    if(ctNames)
    {
        if( poSRS->GetRoot() == NULL)
            poSRS->SetNode( "PROJCS", "unnamed" );
        if(ctNames[CitPcsName])
        {
            poSRS->SetNode( "PROJCS", ctNames[CitPcsName] );
            ret = TRUE;
        }
        else if(geoKey != GTCitationGeoKey) 
        {
            char    szPCSName[128];
            if( GTIFKeyGet( hGTIF, GTCitationGeoKey, szPCSName, 0, sizeof(szPCSName) ) )
            {
                poSRS->SetNode( "PROJCS", szPCSName );
                ret = TRUE;
            }
        }
    
        if(ctNames[CitProjectionName])
            poSRS->SetProjection( ctNames[CitProjectionName] );

        if(ctNames[CitLUnitsName])
        {
            double unitSize;
            if (GTIFKeyGet(hGTIF, ProjLinearUnitSizeGeoKey, &unitSize, 0,
                           sizeof(unitSize) ))
            {
                poSRS->SetLinearUnits( ctNames[CitLUnitsName], unitSize);
                *linearUnitIsSet = TRUE;
            }
        }
        for(int i= 0; i<nCitationNameTypes; i++) 
            CPLFree( ctNames[i] );
        CPLFree( ctNames );
    }
    return ret;
*/
  OGRBoolean ret = FALSE;
  char* lUnitName = NULL;
  double	dfLinearUOM = poSRS->GetLinearUnits( &lUnitName );
  if(!lUnitName || strlen(lUnitName) == 0  || EQUAL(lUnitName, "unknown"))
    *linearUnitIsSet = FALSE;
  else
    *linearUnitIsSet = TRUE;

  char* imgCTName = ImagineCitationTranslation(szCTString, geoKey);
  if(imgCTName)
  {
    strncpy(szCTString, imgCTName, nCTStringLen);
    szCTString[nCTStringLen-1] = '\0';
    CPLFree( imgCTName );
  }
  char** ctNames = CitationStringParse(szCTString, geoKey);
  if(ctNames)
  {
    if( poSRS->GetRoot() == NULL)
      poSRS->SetNode( "PROJCS", "unnamed" );
    if(ctNames[CitPcsName])
    {
      poSRS->SetNode( "PROJCS", ctNames[CitPcsName] );
      ret = TRUE;
    }
    if(ctNames[CitProjectionName])
      poSRS->SetProjection( ctNames[CitProjectionName] );

    if(ctNames[CitLUnitsName])
    {
      double unitSize = 0.0;
      int size = strlen(ctNames[CitLUnitsName]);
      if(strchr(ctNames[CitLUnitsName], '\0'))
        size -= 1;
      for( int i = 0; apszUnitMap[i] != NULL; i += 2 )
      {
            if( EQUALN(apszUnitMap[i], ctNames[CitLUnitsName], size) )
            {
                unitSize = atof(apszUnitMap[i+1]);
                break;
            }
      }
      if( unitSize == 0.0 )
        GTIFKeyGet(hGTIF, ProjLinearUnitSizeGeoKey, &unitSize, 0,
                         sizeof(unitSize) );
      poSRS->SetLinearUnits( ctNames[CitLUnitsName], unitSize);
      *linearUnitIsSet = TRUE;
    }
    for(int i= 0; i<nCitationNameTypes; i++) 
      CPLFree( ctNames[i] );
    CPLFree( ctNames );
  }

  /* if no "PCS Name = " (from Erdas) in GTCitationGeoKey */
  if(geoKey == GTCitationGeoKey)
  {
    if(strlen(szCTString) > 0 && !strstr(szCTString, "PCS Name = "))
    {
      const char* pszProjCS = poSRS->GetAttrValue( "PROJCS" );
      if(!(pszProjCS && strlen(pszProjCS) > 0) && !strstr(szCTString, "Projected Coordinates")
         || strstr(pszProjCS, "unnamed"))
        poSRS->SetNode( "PROJCS", szCTString );
      ret = TRUE;
    }
  }

  return ret;

}

/************************************************************************/
/*                       GetGeogCSFromCitation()                        */
/*                                                                      */
/*      Parse and get geogcs names from a Citation string               */
/************************************************************************/
void GetGeogCSFromCitation(char* szGCSName, int nGCSName,
                           geokey_t geoKey, 
                           char	**ppszGeogName,
                           char	**ppszDatumName,
                           char	**ppszPMName,
                           char	**ppszSpheroidName,
                           char	**ppszAngularUnits)
{
    *ppszGeogName = *ppszDatumName = *ppszPMName = 
        *ppszSpheroidName = *ppszAngularUnits = NULL;

    char* imgCTName = ImagineCitationTranslation(szGCSName, geoKey);
    if(imgCTName)
    {
        strncpy(szGCSName, imgCTName, nGCSName);
        szGCSName[nGCSName-1] = '\0';
        CPLFree( imgCTName );
    }
//    char** ctNames = CitationStringParse(szGCSName);
    char** ctNames = CitationStringParse(szGCSName, geoKey);
    if(ctNames)
    {
        if(ctNames[CitGcsName])
            *ppszGeogName = CPLStrdup( ctNames[CitGcsName] );

        if(ctNames[CitDatumName])
            *ppszDatumName = CPLStrdup( ctNames[CitDatumName] );

        if(ctNames[CitEllipsoidName])
            *ppszSpheroidName = CPLStrdup( ctNames[CitEllipsoidName] );

        if(ctNames[CitPrimemName])
            *ppszPMName = CPLStrdup( ctNames[CitPrimemName] );

        if(ctNames[CitAUnitsName])
            *ppszAngularUnits = CPLStrdup( ctNames[CitAUnitsName] );

        for(int i= 0; i<nCitationNameTypes; i++)
            CPLFree( ctNames[i] );
        CPLFree( ctNames );
    }
    return;
}


