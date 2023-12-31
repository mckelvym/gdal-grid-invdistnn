/******************************************************************************
 * $Id$
 *
 * Project:  ERMapper .ers Driver
 * Purpose:  Implementation of ERSHdrNode class for parsing/accessing .ers hdr.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2007, Frank Warmerdam <warmerdam@pobox.com>
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

#include "cpl_conv.h"
#include "cpl_string.h"
#include "ershdrnode.h"

CPL_CVSID("$Id$");


/************************************************************************/
/*                             ERSHdrNode()                             */
/************************************************************************/

ERSHdrNode::ERSHdrNode()

{
    nItemMax = 0;
    nItemCount = 0;
    papszItemName = NULL;
    papszItemValue = NULL;
    papoItemChild = NULL;
}

/************************************************************************/
/*                            ~ERSHdrNode()                             */
/************************************************************************/

ERSHdrNode::~ERSHdrNode()

{
    int i;

    for( i = 0; i < nItemCount; i++ )
    {
        if( papoItemChild[i] != NULL )
            delete papoItemChild[i];
        if( papszItemValue[i] != NULL )
            CPLFree( papszItemValue[i] );
        CPLFree( papszItemName[i] );
    }

    CPLFree( papszItemName );
    CPLFree( papszItemValue );
    CPLFree( papoItemChild );
}

/************************************************************************/
/*                             MakeSpace()                              */
/*                                                                      */
/*      Ensure we have room for at least one more entry in our item     */
/*      lists.                                                          */
/************************************************************************/

void ERSHdrNode::MakeSpace()

{
    if( nItemCount == nItemMax )
    {
        nItemMax = (int) (nItemMax * 1.3) + 10;
        papszItemName = (char **) 
            CPLRealloc(papszItemName,sizeof(char *) * nItemMax);
        papszItemValue = (char **) 
            CPLRealloc(papszItemValue,sizeof(char *) * nItemMax);
        papoItemChild = (ERSHdrNode **) 
            CPLRealloc(papoItemChild,sizeof(void *) * nItemMax);
    }
}

/************************************************************************/
/*                              ReadLine()                              */
/*                                                                      */
/*      Read one virtual line from the input source.  Multiple lines    */
/*      will be appended for objects enclosed in {}.                    */
/************************************************************************/

int ERSHdrNode::ReadLine( VSILFILE * fp, CPLString &osLine )

{
    int  nBracketLevel;

    osLine = "";

    do
    {
        const char *pszNewLine = CPLReadLineL( fp );
        
        if( pszNewLine == NULL )
            return FALSE;

        osLine += pszNewLine;

        int  bInQuote = FALSE;
        size_t  i;

        nBracketLevel = 0;

        for( i = 0; i < osLine.length(); i++ )
        {
            if( osLine[i] == '"' )
                bInQuote = !bInQuote;
            else if( osLine[i] == '{' && !bInQuote )
                nBracketLevel++;
            else if( osLine[i] == '}' && !bInQuote )
                nBracketLevel--;

            // We have to ignore escaped quotes and backslashes in strings.
            else if( osLine[i] == '\\' && osLine[i+1] == '"' && bInQuote )
                i++;
            else if( osLine[i] == '\\' && osLine[i+1] == '\\' && bInQuote )
                i++;
        }
    } while( nBracketLevel > 0 );

    return TRUE;
}

/************************************************************************/
/*                           ParseChildren()                            */
/*                                                                      */
/*      We receive the FILE * positioned after the "Object Begin"       */
/*      line for this object, and are responsible for reading all       */
/*      children.  We should return after consuming the                 */
/*      corresponding End line for this object.  Really the first       */
/*      unmatched End since we don't know what object we are.           */
/*                                                                      */
/*      This function is used recursively to read sub-objects.          */
/************************************************************************/

int ERSHdrNode::ParseChildren( VSILFILE * fp )

{
    while( TRUE )
    { 
        size_t iOff;
        CPLString osLine;

/* -------------------------------------------------------------------- */
/*      Read the next line (or multi-line for bracketed value).         */
/* -------------------------------------------------------------------- */
        if( !ReadLine( fp, osLine ) )
            return FALSE;

/* -------------------------------------------------------------------- */
/*      Got a Name=Value.                                               */
/* -------------------------------------------------------------------- */
        if( (iOff = osLine.find_first_of( '=' )) != std::string::npos )
        {
            CPLString osName = osLine.substr(0,iOff-1);
            osName.Trim();

            CPLString osValue = osLine.c_str() + iOff + 1;
            osValue.Trim();

            MakeSpace();
            papszItemName[nItemCount] = CPLStrdup(osName);
            papszItemValue[nItemCount] = CPLStrdup(osValue);
            papoItemChild[nItemCount] = NULL;

            nItemCount++;
        }

/* -------------------------------------------------------------------- */
/*      Got a Begin for an object.                                      */
/* -------------------------------------------------------------------- */
        else if( (iOff = osLine.find( " Begin" )) != std::string::npos )
        {
            CPLString osName = osLine.substr(0,iOff);
            osName.Trim();
            
            MakeSpace();
            papszItemName[nItemCount] = CPLStrdup(osName);
            papszItemValue[nItemCount] = NULL;
            papoItemChild[nItemCount] = new ERSHdrNode();

            nItemCount++;
            
            if( !papoItemChild[nItemCount-1]->ParseChildren( fp ) )
                return FALSE;
        }

/* -------------------------------------------------------------------- */
/*      Got an End for our object.  Well, at least we *assume* it       */
/*      must be for our object.                                         */
/* -------------------------------------------------------------------- */
        else if( osLine.find( " End" ) != std::string::npos )
        {
            return TRUE;
        }

/* -------------------------------------------------------------------- */
/*      Error?                                                          */
/* -------------------------------------------------------------------- */
        else if( osLine.Trim().length() > 0 )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Unexpected line parsing .ecw:\n%s", 
                      osLine.c_str() );
            return FALSE;
        }
    }
}

/************************************************************************/
/*                             WriteSelf()                              */
/*                                                                      */
/*      Recursively write self and children to file.                    */
/************************************************************************/

int ERSHdrNode::WriteSelf( VSILFILE * fp, int nIndent )

{
    CPLString oIndent;
    int i;

    oIndent.assign( nIndent, '\t' );

    for( i = 0; i < nItemCount; i++ )
    {
        if( papszItemValue[i] != NULL )
        {
            if( VSIFPrintfL( fp, "%s%s\t= %s\n", 
                             oIndent.c_str(), 
                             papszItemName[i], 
                             papszItemValue[i] ) < 1 )
                return FALSE;
        }
        else
        {
            VSIFPrintfL( fp, "%s%s Begin\n", 
                         oIndent.c_str(), papszItemName[i] );
            if( !papoItemChild[i]->WriteSelf( fp, nIndent+1 ) )
                return FALSE;
            if( VSIFPrintfL( fp, "%s%s End\n", 
                             oIndent.c_str(), papszItemName[i] ) < 1 )
                return FALSE;
        }
    }

    return TRUE;
}

/************************************************************************/
/*                                Find()                                */
/*                                                                      */
/*      Find the desired entry value.  The input is a path with         */
/*      components seperated by dots, relative to the current node.     */
/************************************************************************/

const char *ERSHdrNode::Find( const char *pszPath, const char *pszDefault )

{
    int i;

/* -------------------------------------------------------------------- */
/*      If this is the final component of the path, search for a        */
/*      matching child and return the value.                            */
/* -------------------------------------------------------------------- */
    if( strchr(pszPath,'.') == NULL )
    {
        for( i = 0; i < nItemCount; i++ )
        {
            if( EQUAL(pszPath,papszItemName[i]) )
            {
                if( papszItemValue[i] != NULL )
                {
                    if( papszItemValue[i][0] == '"' )
                    {
                        // strip off quotes. 
                        osTempReturn = papszItemValue[i];
                        osTempReturn = 
                            osTempReturn.substr( 1, osTempReturn.length()-2 );
                        return osTempReturn;
                    }
                    else
                        return papszItemValue[i];
                }
                else
                    return pszDefault;
            }
        }
        return pszDefault;
    }

/* -------------------------------------------------------------------- */
/*      This is a dot path - extract the first element, find a match    */
/*      and recurse.                                                    */
/* -------------------------------------------------------------------- */
    CPLString osPathFirst, osPathRest, osPath = pszPath;
    int iDot;
    
    iDot = osPath.find_first_of('.');
    osPathFirst = osPath.substr(0,iDot);
    osPathRest = osPath.substr(iDot+1);

    for( i = 0; i < nItemCount; i++ )
    {
        if( EQUAL(osPathFirst,papszItemName[i]) )
        {
            if( papoItemChild[i] != NULL )
                return papoItemChild[i]->Find( osPathRest, pszDefault );
            else
                return pszDefault;
        }
    }

    return pszDefault;
}

/************************************************************************/
/*                              FindElem()                              */
/*                                                                      */
/*      Find a particular element from an array valued item.            */
/************************************************************************/

const char *ERSHdrNode::FindElem( const char *pszPath, int iElem, 
                                  const char *pszDefault )

{
    const char *pszArray = Find( pszPath, NULL );
    char **papszTokens;
    int  bDefault = TRUE;

    if( pszArray == NULL )
        return pszDefault;

    papszTokens = CSLTokenizeStringComplex( pszArray, "{ \t}", TRUE, FALSE );
    if( iElem >= 0 && iElem < CSLCount(papszTokens) )
    {
        osTempReturn = papszTokens[iElem];
        bDefault = FALSE;
    }
    
    CSLDestroy( papszTokens );

    if( bDefault )
        return pszDefault;
    else
        return osTempReturn;
}

/************************************************************************/
/*                              FindNode()                              */
/*                                                                      */
/*      Find the desired node.                                          */
/************************************************************************/

ERSHdrNode *ERSHdrNode::FindNode( const char *pszPath )

{
    int i;
    CPLString osPathFirst, osPathRest, osPath = pszPath;
    int iDot;
    
    iDot = osPath.find_first_of('.');
    if( iDot == -1 )
    {
        osPathFirst = osPath;
    }
    else
    {
        osPathFirst = osPath.substr(0,iDot);
        osPathRest = osPath.substr(iDot+1);
    }

    for( i = 0; i < nItemCount; i++ )
    {
        if( EQUAL(osPathFirst,papszItemName[i]) )
        {
            if( papoItemChild[i] != NULL )
            {
                if( osPathRest.length() > 0 )
                    return papoItemChild[i]->FindNode( osPathRest );
                else
                    return papoItemChild[i];
            }
            else
                return NULL;
        }
    }

    return NULL;
}

/************************************************************************/
/*                                Set()                                 */
/*                                                                      */
/*      Set a value item.                                               */
/************************************************************************/

void ERSHdrNode::Set( const char *pszPath, const char *pszValue )

{
    CPLString  osPath = pszPath;
    int iDot;
    
    iDot = osPath.find_first_of('.');

/* -------------------------------------------------------------------- */
/*      We have an intermediate node, find or create it and             */
/*      recurse.                                                        */
/* -------------------------------------------------------------------- */
    if( iDot != -1 )
    {
        CPLString osPathFirst = osPath.substr(0,iDot);
        CPLString osPathRest = osPath.substr(iDot+1);
        ERSHdrNode *poFirst = FindNode( osPathFirst );
        
        if( poFirst == NULL )
        {
            poFirst = new ERSHdrNode();
            
            MakeSpace();
            papszItemName[nItemCount] = CPLStrdup(osPathFirst);
            papszItemValue[nItemCount] = NULL;
            papoItemChild[nItemCount] = poFirst;
            nItemCount++;
        }

        poFirst->Set( osPathRest, pszValue );
        return;
    }

/* -------------------------------------------------------------------- */
/*      This is the final item name.  Find or create it.                */
/* -------------------------------------------------------------------- */
    int i;

    for( i = 0; i < nItemCount; i++ )
    {
        if( EQUAL(osPath,papszItemName[i]) 
            && papszItemValue[i] != NULL )
        {
            CPLFree( papszItemValue[i] );
            papszItemValue[i] = CPLStrdup( pszValue );
            return;
        }
    }

    MakeSpace();
    papszItemName[nItemCount] = CPLStrdup(osPath);
    papszItemValue[nItemCount] = CPLStrdup(pszValue);
    papoItemChild[nItemCount] = NULL;
    nItemCount++;
}
