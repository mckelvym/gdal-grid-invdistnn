/******************************************************************************
 * $Id$
 *
 * Project:  OGR ODBC Driver
 * Purpose:  Declarations for ODBC Access Cover API.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam
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
 * Revision 1.19  2006/06/30 18:00:20  dron
 * Do not define MISSING_SQLULEN on MSVC/x64 target.
 *
 * Revision 1.18  2006/06/01 12:15:39  mloskot
 * Added CPLODBCDriverInstaller utility class to CPL.
 *
 * Revision 1.17  2006/05/12 15:22:23  fwarmerdam
 * on MSVC 6 we sometimes dont have SQLULEN
 *
 * Revision 1.16  2006/04/03 23:08:58  fwarmerdam
 * Use SQLULEN by default now.
 *
 * Revision 1.15  2006/02/19 21:54:34  mloskot
 * [WINCE] Changes related to Windows CE port of CPL. Most changes are #ifdef wrappers.
 *
 * Revision 1.14  2005/09/05 20:18:43  fwarmerdam
 * added binary column support
 *
 * Revision 1.13  2005/08/31 03:32:41  fwarmerdam
 * GetTypeName now returns CPLString
 *
 * Revision 1.12  2005/06/29 01:01:01  ssoule
 * Changed return type of CPLODBCStatement::GetTypeName from const char * to
 * std::string.
 *
 * Revision 1.11  2005/01/13 03:24:54  fwarmerdam
 * changed type of m_panColSize, per ODBC 3.52 requirements
 *
 * Revision 1.10  2004/06/23 16:11:30  warmerda
 * just testing cvs commits
 *
 * Revision 1.9  2004/06/01 20:40:02  warmerda
 * expanded tabs
 *
 * Revision 1.8  2003/11/24 20:45:00  warmerda
 * make CollectResultsInfo() public
 *
 * Revision 1.7  2003/10/29 17:56:57  warmerda
 * Added PrimaryKeys() support
 *
 * Revision 1.6  2003/10/06 20:04:08  warmerda
 * added escaping support
 *
 * Revision 1.5  2003/10/06 17:16:18  warmerda
 * added windows.h for windows, and fixed m_panColSize type
 *
 * Revision 1.4  2003/09/26 20:02:41  warmerda
 * update GetColData()
 *
 * Revision 1.3  2003/09/26 13:51:02  warmerda
 * Add documentation
 *
 * Revision 1.2  2003/09/25 17:09:49  warmerda
 * added some more methods
 *
 * Revision 1.1  2003/09/24 15:39:14  warmerda
 * New
 *
 */

#ifndef CPL_ODBC_H_INCLUDED
#define CPL_ODBC_H_INCLUDED

#include "cpl_port.h"

#ifndef WIN32CE /* ODBC is not supported on Windows CE. */

#ifdef WIN32
#  include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <odbcinst.h>
#include "cpl_string.h"

#ifdef PATH_MAX
#  define ODBC_FILENAME_MAX PATH_MAX
#else
#  define ODBC_FILENAME_MAX (255 + 1) /* Max path length */
#endif

 
/**
 * \file cpl_odbc.h
 *
 * ODBC Abstraction Layer (C++).
 */

/**
 * A class providing functions to install or remove ODBC driver.
 */
class CPL_DLL CPLODBCDriverInstaller
{
    char m_szPathOut[ODBC_FILENAME_MAX];
    char m_szError[SQL_MAX_MESSAGE_LENGTH];
    DWORD m_nErrorCode;
    DWORD m_nUsageCount;

  public:
    
    // Default constructor.
    CPLODBCDriverInstaller();


    /**
     * Installs ODBC driver or updates definition of already installed driver.
     * Interanally, it calls ODBC's SQLInstallDriverEx function.
     * 
     * @param pszDriver - The driver definition as a list of keyword-value
     * pairs describing the driver (See ODBC API Reference).
     *
     * @param pszPathIn - Full path of the target directory of the installation,
     * or a null pointer (for unixODBC, NULL is passed).
     *
     * @param fRequest - The fRequest argument must contain one of
     * the following values:
     * ODBC_INSTALL_COMPLETE - (default) complete the installation request
     * ODBC_INSTALL_INQUIRY - inquire about where a driver can be installed
     *
     * @return TRUE indicates success, FALSE if it fails.
     */
    int InstallDriver( const char* pszDriver, const char* pszPathIn,
            WORD fRequest = ODBC_INSTALL_COMPLETE );

    /**
     * Removes or changes information about the driver from
     * the Odbcinst.ini entry in the system information.
     *
     * @param pszDriverName - The name of the driver as registered in
     * the Odbcinst.ini key of the system information.
     * 
     * @param fRemoveDSN - TRUE: Remove DSNs associated with the driver 
     * specified in lpszDriver. FALSE: Do not remove DSNs associated
     * with the driver specified in lpszDriver. 
     *
     * @return The function returns TRUE if it is successful,
     * FALSE if it fails. If no entry exists in the system information
     * when this function is called, the function returns FALSE.
     * In order to obtain usage count value, call GetUsageCount().
     */
    int RemoveDriver( const char* pszDriverName, int fRemoveDSN = FALSE );


    // The usage count of the driver after this function has been called
    int GetUsageCount() const {  return m_nUsageCount; }


    // Path of the target directory where the driver should be installed.
    // For details, see ODBC API Reference and lpszPathOut
    // parameter of SQLInstallDriverEx 
    const char* GetPathOut() const { return m_szPathOut; }


    // If InstallDriver returns FALSE, then GetLastError then
    // error message can be obtained by calling this function.
    // Internally, it calls ODBC's SQLInstallerError function.
    const char* GetLastError() const { return m_szError; }
   

    // If InstallDriver returns FALSE, then GetLastErrorCode then
    // error code can be obtained by calling this function.
    // Internally, it calls ODBC's SQLInstallerError function.
    // See ODBC API Reference for possible error flags.
    DWORD GetLastErrorCode() const { return m_nErrorCode; }
};

class CPLODBCStatement;

/* On MSVC SQLULEN is missing in some cases (ie. VC6)
** but it is always a #define so test this way.   On Unix
** it is a typedef so we can't always do this.
*/
#if defined(_MSC_VER) && !defined(SQLULEN) && !defined(_WIN64)
#  define MISSING_SQLULEN
#endif

#if !defined(MISSING_SQLULEN)
/* ODBC types to support 64 bit compilation */
#  define _SQLULEN SQLULEN
#  define _SQLLEN  SQLLEN
#else
#  define _SQLULEN SQLUINTEGER
#  define _SQLLEN  SQLINTEGER
#endif	/* ifdef SQLULEN */


/**
 * A class representing an ODBC database session. 
 *
 * Includes error collection services.
 */

class CPL_DLL CPLODBCSession {
    char      m_szLastError[SQL_MAX_MESSAGE_LENGTH + 1];
    HENV      m_hEnv;
    HDBC      m_hDBC;

  public:
    CPLODBCSession();
    ~CPLODBCSession();

    int         EstablishSession( const char *pszDSN, 
                                  const char *pszUserid, 
                                  const char *pszPassword );
    const char  *GetLastError();

    // Essentially internal. 

    int         CloseSession();

    int         Failed( int, HSTMT = NULL );
    HDBC        GetConnection() { return m_hDBC; }
    HENV        GetEnvironment()  { return m_hEnv; }
};

/**
 * Abstraction for statement, and resultset.
 *
 * Includes methods for executing an SQL statement, and for accessing the
 * resultset from that statement.  Also provides for executing other ODBC
 * requests that produce results sets such as SQLColumns() and SQLTables()
 * requests.
 */

class CPL_DLL CPLODBCStatement {

    CPLODBCSession     *m_poSession;
    HSTMT               m_hStmt;

    short          m_nColCount;
    char         **m_papszColNames;
    short         *m_panColType;
    _SQLULEN       *m_panColSize;
    short         *m_panColPrecision;
    short         *m_panColNullable;

    char         **m_papszColValues;
    int           *m_panColValueLengths;
    
    int            Failed( int );

    char          *m_pszStatement;
    int            m_nStatementMax;
    int            m_nStatementLen;

  public:
    CPLODBCStatement( CPLODBCSession * );
    ~CPLODBCStatement();

    HSTMT          GetStatement() { return m_hStmt; }

    // Command buffer related.
    void           Clear();
    void           AppendEscaped( const char * );
    void           Append( const char * );
    void           Append( int );
    void           Append( double );
    int            Appendf( const char *, ... );
    const char    *GetCommand() { return m_pszStatement; }

    int            ExecuteSQL( const char * = NULL );

    // Results fetching
    int            Fetch( int nOrientation = SQL_FETCH_NEXT, 
                          int nOffset = 0 );
    void           ClearColumnData();

    int            GetColCount();
    const char    *GetColName(int iCol);
    short          GetColType(int iCol);
    short          GetColSize(int iCol);
    short          GetColPrecision(int iCol);
    short          GetColNullable(int iCol);

    int            GetColId( const char * );
    const char    *GetColData( int, const char * = NULL );
    const char    *GetColData( const char *, const char * = NULL );
    int            GetColDataLength( int );

    // Fetch special metadata.
    int            GetColumns( const char *pszTable, 
                               const char *pszCatalog = NULL,
                               const char *pszSchema = NULL );
    int            GetPrimaryKeys( const char *pszTable, 
                                   const char *pszCatalog = NULL,
                                   const char *pszSchema = NULL );

    int            GetTables( const char *pszCatalog = NULL,
                              const char *pszSchema = NULL );

    void           DumpResult( FILE *fp, int bShowSchema = FALSE );

    static CPLString GetTypeName( int );

    int            CollectResultsInfo();
};

#endif /* #ifndef WIN32CE */

#endif


