/******************************************************************************
 * $Id: cpl_vsi_mem.cpp 15219 2008-08-25 20:38:56Z mloskot $
 *
 * Project:  VSI Virtual File System
 * Purpose:  Implementation of Memory Buffer virtual IO functions.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2005, Frank Warmerdam <warmerdam@pobox.com>
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

/* Remove annoying warnings in eVC++ and VC++ 6.0 */
#if defined(WIN32CE)
#  pragma warning(disable:4786)
#endif

#include "cpl_vsi_virtual.h"
#include "cpl_string.h"
#include "cpl_multiproc.h"
#include <map>

#if defined(WIN32CE)
#  include <wce_errno.h>
#endif


CPL_CVSID("$Id: cpl_vsi_mem.cpp 15219 2008-08-25 20:38:56Z mloskot $");

/*
** Notes on Multithreading:
**
** VSIMemFilesystemHandler: This class maintains a mutex to protect
** access and update of the oFileList array which has all the "files" in 
** the memory filesystem area.  It is expected that multiple threads would
** want to create and read different files at the same time and so might
** collide access oFileList without the mutex.
**
** VSIMemFile: In theory we could allow different threads to update the
** the same memory file, but for simplicity we restrict to single writer,
** multiple reader as an expectation on the application code (not enforced
** here), which means we don't need to do any protection of this class.
**
** VSIMemHandle: This is essentially a "current location" representing
** on accessor to a file, and is inherently intended only to be used in 
** a single thread. 
**
** In General:
**
** Multiple threads accessing the memory filesystem are ok as long as
**  1) A given VSIMemHandle (ie. FILE * at app level) isn't used by multiple 
**     threads at once. 
**  2) A given memory file isn't accessed by more than one thread unless
**     all threads are just reading.
*/ 

/************************************************************************/
/* ==================================================================== */
/*                              VSIMemFile                              */
/* ==================================================================== */
/************************************************************************/

class VSIMemFile
{
public:
    CPLString     osFilename;
    int           nRefCount;

    int           bIsDirectory;

    int           bOwnData;
    GByte        *pabyData;
    vsi_l_offset  nLength;
    vsi_l_offset  nAllocLength;

                  VSIMemFile();
    virtual       ~VSIMemFile();

    bool          SetLength( vsi_l_offset nNewSize );
};

/************************************************************************/
/* ==================================================================== */
/*                             VSIMemHandle                             */
/* ==================================================================== */
/************************************************************************/

class VSIMemHandle : public VSIVirtualHandle
{ 
  public:
    VSIMemFile    *poFile;
    vsi_l_offset  nOffset;
    int           bUpdate;

    virtual int       Seek( vsi_l_offset nOffset, int nWhence );
    virtual vsi_l_offset Tell();
    virtual size_t    Read( void *pBuffer, size_t nSize, size_t nMemb );
    virtual size_t    Write( const void *pBuffer, size_t nSize, size_t nMemb );
    virtual int       Eof();
    virtual int       Close();
};

/************************************************************************/
/* ==================================================================== */
/*                       VSIMemFilesystemHandler                        */
/* ==================================================================== */
/************************************************************************/

class VSIMemFilesystemHandler : public VSIFilesystemHandler 
{
public:
    std::map<CPLString,VSIMemFile*>   oFileList;
    void             *hMutex;

                     VSIMemFilesystemHandler();
    virtual          ~VSIMemFilesystemHandler();

    virtual VSIVirtualHandle *Open( const char *pszFilename, 
                                    const char *pszAccess);
    virtual int      Stat( const char *pszFilename, VSIStatBufL *pStatBuf );
    virtual int      Unlink( const char *pszFilename );
    virtual int      Mkdir( const char *pszDirname, long nMode );
    virtual int      Rmdir( const char *pszDirname );
    virtual char   **ReadDir( const char *pszDirname );
    static  void     NormalizePath( CPLString & );
};

/************************************************************************/
/* ==================================================================== */
/*                              VSIMemFile                              */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                             VSIMemFile()                             */
/************************************************************************/

VSIMemFile::VSIMemFile()

{
    nRefCount = 0;
    bIsDirectory = FALSE;
    bOwnData = TRUE;
    pabyData = NULL;
    nLength = 0;
    nAllocLength = 0;
}

/************************************************************************/
/*                            ~VSIMemFile()                             */
/************************************************************************/

VSIMemFile::~VSIMemFile()

{
    if( nRefCount != 0 )
        CPLDebug( "VSIMemFile", "Memory file %s deleted with %d references.",
                  osFilename.c_str(), nRefCount );

    if( bOwnData && pabyData )
        CPLFree( pabyData );
}

/************************************************************************/
/*                             SetLength()                              */
/************************************************************************/

bool VSIMemFile::SetLength( vsi_l_offset nNewLength )

{
/* -------------------------------------------------------------------- */
/*      Grow underlying array if needed.                                */
/* -------------------------------------------------------------------- */
    if( nNewLength > nAllocLength )
    {
        GByte *pabyNewData;
        vsi_l_offset nNewAlloc = (nNewLength + nNewLength / 10) + 5000;

        pabyNewData = (GByte *) CPLRealloc(pabyData, (size_t)nNewAlloc);
        if( pabyNewData == NULL )
            return false;

        pabyData = pabyNewData;
        nAllocLength = nNewAlloc;
    }

    nLength = nNewLength;

    return true;
}

/************************************************************************/
/* ==================================================================== */
/*                             VSIMemHandle                             */
/* ==================================================================== */
/************************************************************************/


/************************************************************************/
/*                               Close()                                */
/************************************************************************/

int VSIMemHandle::Close()

{
    if( --(poFile->nRefCount) == 0 )
        delete poFile;

    poFile = NULL;

    return 0;
}

/************************************************************************/
/*                                Seek()                                */
/************************************************************************/

int VSIMemHandle::Seek( vsi_l_offset nOffset, int nWhence )

{
    if( nWhence == SEEK_CUR )
        this->nOffset += nOffset;
    else if( nWhence == SEEK_SET )
        this->nOffset = nOffset;
    else if( nWhence == SEEK_END )
        this->nOffset = poFile->nLength + nOffset;
    else
    {
        errno = EINVAL;
        return -1;
    }

    if( this->nOffset < 0 )
    {
        this->nOffset = 0;
        return -1;
    }
    
    if( this->nOffset > poFile->nLength )
    {
        if( !bUpdate ) // Read-only files cannot be extended by seek.
        {
            CPLDebug( "VSIMemHandle", 
                      "Attempt to extend read-only file '%s' to length %d from %d, .", 
                      poFile->osFilename.c_str(), 
                      (int) this->nOffset, (int) poFile->nLength );

            this->nOffset = poFile->nLength;
            errno = EACCES;
            return -1;
        }
        else // Writeable files are zero-extended by seek past end.
        {
            if( !poFile->SetLength( this->nOffset ) )
                return -1;
        }
    }

    return 0;
}

/************************************************************************/
/*                                Tell()                                */
/************************************************************************/

vsi_l_offset VSIMemHandle::Tell()

{
    return nOffset;
}

/************************************************************************/
/*                                Read()                                */
/************************************************************************/

size_t VSIMemHandle::Read( void * pBuffer, size_t nSize, size_t nCount )

{
    // FIXME: Integer overflow check should be placed here:
    size_t nBytesToRead = nSize * nCount; 

    if( nBytesToRead + nOffset > poFile->nLength )
    {
        nBytesToRead = poFile->nLength - nOffset;
        nCount = nBytesToRead / nSize;
    }

    memcpy( pBuffer, poFile->pabyData + nOffset, (size_t)nBytesToRead );
    nOffset += nBytesToRead;

    return nCount;
}

/************************************************************************/
/*                               Write()                                */
/************************************************************************/

size_t VSIMemHandle::Write( const void * pBuffer, size_t nSize, size_t nCount )

{
    if( !bUpdate )
    {
        errno = EACCES;
        return 0;
    }

    // FIXME: Integer overflow check should be placed here:
    size_t nBytesToWrite = nSize * nCount; 

    if( nBytesToWrite + nOffset > poFile->nLength )
    {
        if( !poFile->SetLength( nBytesToWrite + nOffset ) )
            return 0;
    }

    memcpy( poFile->pabyData + nOffset, pBuffer, nBytesToWrite );
    nOffset += nBytesToWrite;

    return nCount;
}

/************************************************************************/
/*                                Eof()                                 */
/************************************************************************/

int VSIMemHandle::Eof()

{
    return nOffset == poFile->nLength;
}

/************************************************************************/
/* ==================================================================== */
/*                       VSIMemFilesystemHandler                        */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                      VSIMemFilesystemHandler()                       */
/************************************************************************/

VSIMemFilesystemHandler::VSIMemFilesystemHandler()

{
    hMutex = NULL;
}

/************************************************************************/
/*                      ~VSIMemFilesystemHandler()                      */
/************************************************************************/

VSIMemFilesystemHandler::~VSIMemFilesystemHandler()

{
    std::map<CPLString,VSIMemFile*>::const_iterator iter;

    for( iter = oFileList.begin(); iter != oFileList.end(); iter++ )
    {
        iter->second->nRefCount--;
        delete iter->second;
    }

    if( hMutex != NULL )
        CPLDestroyMutex( hMutex );
    hMutex = NULL;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

VSIVirtualHandle *
VSIMemFilesystemHandler::Open( const char *pszFilename, 
                               const char *pszAccess )

{
    CPLMutexHolder oHolder( &hMutex );
    VSIMemFile *poFile;
    CPLString osFilename = pszFilename;
    NormalizePath( osFilename );

/* -------------------------------------------------------------------- */
/*      Get the filename we are opening, create if needed.              */
/* -------------------------------------------------------------------- */
    if( oFileList.find(osFilename) == oFileList.end() )
        poFile = NULL;
    else
        poFile = oFileList[osFilename];

    if( strstr(pszAccess,"w") == NULL && poFile == NULL )
    {
        errno = ENOENT;
        return NULL;
    }

    if( strstr(pszAccess,"w") )
    {
        if( poFile )
            poFile->SetLength( 0 );
        else
        {
            poFile = new VSIMemFile;
            poFile->osFilename = osFilename;
            oFileList[poFile->osFilename] = poFile;
            poFile->nRefCount++; // for file list
        }
    }

    if( poFile->bIsDirectory )
    {
        errno = EISDIR;
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Setup the file handle on this file.                             */
/* -------------------------------------------------------------------- */
    VSIMemHandle *poHandle = new VSIMemHandle;

    poHandle->poFile = poFile;
    poHandle->nOffset = 0;
    if( strstr(pszAccess,"w") || strstr(pszAccess,"+") 
        || strstr(pszAccess,"a") )
        poHandle->bUpdate = TRUE;
    else
        poHandle->bUpdate = FALSE;

    poFile->nRefCount++;

    if( strstr(pszAccess,"a") )
        poHandle->nOffset = poFile->nLength;

    return poHandle;
}

/************************************************************************/
/*                                Stat()                                */
/************************************************************************/

int VSIMemFilesystemHandler::Stat( const char * pszFilename, 
                                   VSIStatBufL * pStatBuf )
    
{
    CPLMutexHolder oHolder( &hMutex );

    CPLString osFilename = pszFilename;
    NormalizePath( osFilename );

    if( oFileList.find(osFilename) == oFileList.end() )
    {
        errno = ENOENT;
        return -1;
    }

    VSIMemFile *poFile = oFileList[osFilename];

    memset( pStatBuf, 0, sizeof(VSIStatBufL) );

    if( poFile->bIsDirectory )
    {
        pStatBuf->st_size = 0;
        pStatBuf->st_mode = S_IFDIR;
    }
    else
    {
        pStatBuf->st_size = poFile->nLength;
        pStatBuf->st_mode = S_IFREG;
    }

    return 0;
}

/************************************************************************/
/*                               Unlink()                               */
/************************************************************************/

int VSIMemFilesystemHandler::Unlink( const char * pszFilename )

{
    CPLMutexHolder oHolder( &hMutex );

    CPLString osFilename = pszFilename;
    NormalizePath( osFilename );

    VSIMemFile *poFile;

    if( oFileList.find(osFilename) == oFileList.end() )
    {
        errno = ENOENT;
        return -1;
    }
    else
    {
        poFile = oFileList[osFilename];

        if( --(poFile->nRefCount) == 0 )
            delete poFile;

        oFileList.erase( oFileList.find(osFilename) );

        return 0;
    }
}

/************************************************************************/
/*                               Mkdir()                                */
/************************************************************************/

int VSIMemFilesystemHandler::Mkdir( const char * pszPathname,
                                    long nMode )

{
    CPLMutexHolder oHolder( &hMutex );

    CPLString osPathname = pszPathname;

    NormalizePath( osPathname );

    if( oFileList.find(osPathname) != oFileList.end() )
    {
        errno = EEXIST;
        return -1;
    }

    VSIMemFile *poFile = new VSIMemFile;

    poFile->osFilename = osPathname;
    poFile->bIsDirectory = TRUE;
    oFileList[osPathname] = poFile;
    poFile->nRefCount++; /* referenced by file list */

    return 0;
}

/************************************************************************/
/*                               Rmdir()                                */
/************************************************************************/

int VSIMemFilesystemHandler::Rmdir( const char * pszPathname )

{
    CPLMutexHolder oHolder( &hMutex );

    return Unlink( pszPathname );
}

/************************************************************************/
/*                              ReadDir()                               */
/************************************************************************/

char **VSIMemFilesystemHandler::ReadDir( const char *pszPath )

{
    CPLMutexHolder oHolder( &hMutex );

    CPLString osPath = pszPath;

    NormalizePath( osPath );

    std::map<CPLString,VSIMemFile*>::const_iterator iter;
    char **papszDir = NULL;
    int nPathLen = strlen(osPath);

    if( osPath[nPathLen-1] == '/' )
        nPathLen--;

    /* In case of really big number of files in the directory, CSLAddString */
    /* can be slow (see #2158). We then directly build the list. */
    int nItems=0;
    int nAllocatedItems=0;

    for( iter = oFileList.begin(); iter != oFileList.end(); iter++ )
    {
        const char *pszFilePath = iter->second->osFilename.c_str();
        if( EQUALN(osPath,pszFilePath,nPathLen)
            && pszFilePath[nPathLen] == '/' 
            && strstr(pszFilePath+nPathLen+1,"/") == NULL )
        {
            if (nItems == 0)
            {
                papszDir = (char**) CPLCalloc(2,sizeof(char*));
                nAllocatedItems = 1;
            }
            else if (nItems >= nAllocatedItems)
            {
                nAllocatedItems = nAllocatedItems * 2;
                papszDir = (char**)CPLRealloc(papszDir, 
                                              (nAllocatedItems+2)*sizeof(char*));
            }

            papszDir[nItems] = CPLStrdup(pszFilePath+nPathLen+1);
            papszDir[nItems+1] = NULL;

            nItems++;
        }
    }

    return papszDir;
}

/************************************************************************/
/*			NormalizePath() 				*/
/************************************************************************/

void VSIMemFilesystemHandler::NormalizePath( CPLString &oPath )

{
    int  i, nSize = oPath.size();

    for( i = 0; i < nSize; i++ )
    {
        if( oPath[i] == '\\' )
            oPath[i] = '/';
    }

}

/************************************************************************/
/*                     VSIInstallLargeFileHandler()                     */
/************************************************************************/

/**
 * \brief Install "memory" file system handler. 
 *
 * A special file handler is installed that allows block of memory to be
 * treated as files.   All portions of the file system underneath the base
 * path "/vsimem/" will be handled by this driver.  
 *
 * Normal VSI*L functions can be used freely to create and destroy memory
 * arrays treating them as if they were real file system objects.  Some
 * additional methods exist to efficient create memory file system objects
 * without duplicating original copies of the data or to "steal" the block
 * of memory associated with a memory file. 
 *
 * At this time the memory handler does not properly handle directory
 * semantics for the memory portion of the filesystem.  The VSIReadDir()
 * function is not supported though this will be corrected in the future. 
 *
 * Calling this function repeatedly should do no harm, though it is not
 * necessary.  It is already called the first time a virtualizable 
 * file access function (ie. VSIFOpenL(), VSIMkDir(), etc) is called. 
 *
 * This code example demonstrates using GDAL to translate from one memory
 * buffer to another.
 *
 * \code
 * GByte *ConvertBufferFormat( GByte *pabyInData, vsi_l_offset nInDataLength, 
 *                             vsi_l_offset *pnOutDataLength )
 * {
 *     // create memory file system object from buffer.
 *     VSIFCloseL( VSIFileFromMemBuffer( "/vsimem/work.dat", pabyInData,
 *                                       nInDataLength, FALSE ) );
 *
 *     // Open memory buffer for read.
 *     GDALDatasetH hDS = GDALOpen( "/vsimem/work.dat", GA_ReadOnly );
 * 
 *     // Get output format driver. 
 *     GDALDriverH hDriver = GDALGetDriverByName( "GTiff" );
 *     GDALDatasetH hOutDS;
 *
 *     hOutDS = GDALCreateCopy( hDriver, "/vsimem/out.tif", hDS, TRUE, NULL, 
 *                              NULL, NULL );
 * 
 *     // close source file, and "unlink" it.  
 *     GDALClose( hDS );
 *     VSIUnlink( "/vsimem/work.dat" );
 *
 *     // seize the buffer associated with the output file.
 *
 *     return VSIGetMemFileBuffer( "/vsimem/out.tif", pnOutDataLength, TRUE );
 * }
 * \endcode
 */

void VSIInstallMemFileHandler()
{
    VSIFileManager::InstallHandler( "/vsimem/", new VSIMemFilesystemHandler );
}

/************************************************************************/
/*                        VSIFileFromMemBuffer()                        */
/************************************************************************/

/**
 * \brief Create memory "file" from a buffer.
 *
 * A virtual memory file is created from the passed buffer with the indicated
 * filename.  Under normal conditions the filename would need to be absolute
 * and within the /vsimem/ portion of the filesystem.  
 *
 * If bTakeOwnership is TRUE, then the memory file system handler will take
 * ownership of the buffer, freeing it when the file is deleted.  Otherwise
 * it remains the responsibility of the caller, but should not be freed as
 * long as it might be accessed as a file.  In no circumstances does this
 * function take a copy of the pabyData contents. 
 *
 * @param pszFilename the filename to be created.
 * @param pabyData the data buffer for the file. 
 * @param nDataLength the length of buffer in bytes.
 * @param bTakeOwnership TRUE to transfer "ownership" of buffer or FALSE. 
 *
 * @return open file handle on created file (see VSIFOpenL()). 
 */

FILE *VSIFileFromMemBuffer( const char *pszFilename, 
                          GByte *pabyData, 
                          vsi_l_offset nDataLength,
                          int bTakeOwnership )

{
    if( VSIFileManager::GetHandler("") 
        == VSIFileManager::GetHandler("/vsimem/") )
        VSIInstallMemFileHandler();

    VSIMemFilesystemHandler *poHandler = (VSIMemFilesystemHandler *) 
        VSIFileManager::GetHandler("/vsimem/");

    VSIMemFile *poFile = new VSIMemFile;

    poFile->osFilename = pszFilename;
    poFile->bOwnData = bTakeOwnership;
    poFile->pabyData = pabyData;
    poFile->nLength = nDataLength;
    poFile->nAllocLength = nDataLength;

    {
        CPLMutexHolder oHolder( &poHandler->hMutex );
        poHandler->oFileList[poFile->osFilename] = poFile;
        poFile->nRefCount++;
    }

    return (FILE *) poHandler->Open( pszFilename, "r+" );
}

/************************************************************************/
/*                        VSIGetMemFileBuffer()                         */
/************************************************************************/

/**
 * \brief Fetch buffer underlying memory file. 
 *
 * This function returns a pointer to the memory buffer underlying a 
 * virtual "in memory" file.  If bUnlinkAndSeize is TRUE the filesystem
 * object will be deleted, and ownership of the buffer will pass to the 
 * caller otherwise the underlying file will remain in existance. 
 *
 * @param pszFilename the name of the file to grab the buffer of.
 * @param pnDataLength (file) length returned in this variable.
 * @param bUnlinkAndSeize TRUE to remove the file, or FALSE to leave unaltered.
 *
 * @return pointer to memory buffer or NULL on failure.
 */

GByte *VSIGetMemFileBuffer( const char *pszFilename, 
                            vsi_l_offset *pnDataLength,
                            int bUnlinkAndSeize )

{
    VSIMemFilesystemHandler *poHandler = (VSIMemFilesystemHandler *) 
        VSIFileManager::GetHandler("/vsimem/");

    CPLMutexHolder oHolder( &poHandler->hMutex );

    if( poHandler->oFileList.find(pszFilename) == poHandler->oFileList.end() )
        return NULL;

    VSIMemFile *poFile = poHandler->oFileList[pszFilename];
    GByte *pabyData;

    pabyData = poFile->pabyData;
    if( pnDataLength != NULL )
        *pnDataLength = poFile->nLength;
    
    if( bUnlinkAndSeize )
    {
        if( !poFile->bOwnData )
            CPLDebug( "VSIMemFile", 
                      "File doesn't own data in VSIGetMemFileBuffer!" );
        else
            poFile->bOwnData = FALSE;

        poHandler->oFileList.erase( poHandler->oFileList.find(pszFilename) );
        --(poFile->nRefCount);
        delete poFile;
    }

    return pabyData;
}
                            
