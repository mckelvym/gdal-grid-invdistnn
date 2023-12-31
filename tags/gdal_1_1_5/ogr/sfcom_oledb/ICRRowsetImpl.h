// ICRRowsetImpl
template <class T, class RowsetInterface,
		  class RowClass = CSimpleRow,
		  class MapClass = CSimpleMap < RowClass::KeyType, RowClass* > >
class ATL_NO_VTABLE ICRRowsetImpl : public RowsetInterface
{
    public:
    typedef RowClass _HRowClass;
    ICRRowsetImpl()
    {
        m_iRowset = 0;
        m_bCanScrollBack = false;
        m_bCanFetchBack = false;
        m_bReset = true;
    }
    ~ICRRowsetImpl()
    {
        for (int i = 0; i < m_rgRowHandles.GetSize(); i++)
        delete (m_rgRowHandles.GetValueAt(i));
    }
    HRESULT RefRows(ULONG cRows, const HROW rghRows[], ULONG rgRefCounts[],
                    DBROWSTATUS rgRowStatus[], BOOL bAdd)
    {
        ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::AddRefRows\n");
        if (cRows == 0)
        return S_OK;
        if (rghRows == NULL)
        return E_INVALIDARG;
        T::ObjectLock cab((T*)this);
        BOOL bSuccess1 = FALSE;
        BOOL bFailed1 = FALSE;
        DBROWSTATUS rs;
        DWORD dwRef;
        for (ULONG iRow = 0; iRow < cRows; iRow++)
        {
            HROW hRowCur = rghRows[iRow];
            RowClass* pRow = m_rgRowHandles.Lookup((RowClass::KeyType)hRowCur);
            if (pRow == NULL)
            {
                ATLTRACE2(atlTraceDBProvider, 0, "Could not find HANDLE %x in list\n");
                rs = DBROWSTATUS_E_INVALID;
                dwRef = 0;
                bFailed1 = TRUE;
            }
            else
            {
                if (bAdd)
                dwRef = pRow->AddRefRow();
                else
                {
                    dwRef = pRow->ReleaseRow();
                    if (dwRef == 0)
                    {
                        delete pRow;
                        m_rgRowHandles.Remove((RowClass::KeyType)hRowCur);
                    }
                }
                bSuccess1 = TRUE;
                rs = DBROWSTATUS_S_OK;
            }
            if (rgRefCounts)
            rgRefCounts[iRow] = dwRef;
            if (rgRowStatus != NULL)
            rgRowStatus[iRow] = rs;
        }
        if (!bSuccess1 && !bFailed1)
        {
            ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::RefRows Unexpected state\n");
            return E_FAIL;
        }
        HRESULT hr = S_OK;
        if (bSuccess1 && bFailed1)
        hr = DB_S_ERRORSOCCURRED;
        if (!bSuccess1 && bFailed1)
        hr = DB_E_ERRORSOCCURRED;
        return hr;
    }

    STDMETHOD(AddRefRows)(ULONG cRows,
                          const HROW rghRows[],
                          ULONG rgRefCounts[],
                          DBROWSTATUS rgRowStatus[])
    {
        ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::AddRefRows\n");
        if (cRows == 0)
        return S_OK;
        return RefRows(cRows, rghRows, rgRefCounts, rgRowStatus, TRUE);
    }
    virtual DBSTATUS GetDBStatus(RowClass* poRC, ATLCOLUMNINFO*poColInfo)
    {
        T* pT = (T*) this;

        return pT->GetRCDBStatus(poRC, poColInfo);
        
        //return DBSTATUS_S_OK;
    }
    OUT_OF_LINE HRESULT GetDataHelper(HACCESSOR hAccessor,
                                      ATLCOLUMNINFO*& rpInfo,
                                      void** ppBinding,
                                      void*& rpSrcData,
                                      ULONG& rcCols,
                                      CComPtr<IDataConvert>& rspConvert,
                                      RowClass* pRow)
    {
        ATLASSERT(ppBinding != NULL);
        T* pT = (T*) this;
        *ppBinding = (void*)pT->m_rgBindings.Lookup((int)hAccessor);
        if (*ppBinding == NULL)
        return DB_E_BADACCESSORHANDLE;
        rpSrcData = (void*)&(pT->m_rgRowData[pRow->m_iRowset]);
        rpInfo = T::GetColumnInfo((T*)this, &rcCols);
        rspConvert = pT->m_spConvert;
        return S_OK;

    }
    STDMETHOD(GetData)(HROW hRow,
                       HACCESSOR hAccessor,
                       void *pDstData)
    {
        ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::GetData\n");
        if (pDstData == NULL)
        return E_INVALIDARG;
        HRESULT hr = S_OK;
        RowClass* pRow = (RowClass*)hRow;
        if (hRow == NULL || (pRow = m_rgRowHandles.Lookup((RowClass::KeyType)hRow)) == NULL)
        return DB_E_BADROWHANDLE;
        T::_BindType* pBinding;
        void* pSrcData;
        ULONG cCols;
        ATLCOLUMNINFO* pColInfo;
        CComPtr<IDataConvert> spConvert;
        hr = GetDataHelper(hAccessor, pColInfo, (void**)&pBinding, pSrcData, cCols, spConvert, pRow);
        if (FAILED(hr))
        return hr;
        for (ULONG iBind =0; iBind < pBinding->cBindings; iBind++)
        {
            DBBINDING* pBindCur = &(pBinding->pBindings[iBind]);
            for (ULONG iColInfo = 0;
                 iColInfo < cCols && pBindCur->iOrdinal != pColInfo[iColInfo].iOrdinal;
                 iColInfo++);
            if (iColInfo == cCols)
            return DB_E_BADORDINAL;
            ATLCOLUMNINFO* pColCur = &(pColInfo[iColInfo]);
            // Ordinal found at iColInfo
            BOOL bProvOwn = pBindCur->dwMemOwner == DBMEMOWNER_PROVIDEROWNED;
            bProvOwn;
            DBSTATUS dbStat = GetDBStatus(pRow, pColCur);

            // If the provider's field is NULL, we can optimize this situation,
            // set the fields to 0 and continue.
            if (dbStat == DBSTATUS_S_ISNULL)
            {
                if (pBindCur->dwPart & DBPART_STATUS)
                *((DBSTATUS*)((BYTE*)(pDstData) + pBindCur->obStatus)) = dbStat;

                if (pBindCur->dwPart & DBPART_LENGTH)
                *((ULONG*)((BYTE*)(pDstData) + pBindCur->obLength)) = 0;

                if (pBindCur->dwPart & DBPART_VALUE)
                *((BYTE*)(pDstData) + pBindCur->obValue) = NULL;
                continue;
            }
            ULONG cbDst = pBindCur->cbMaxLen;
            ULONG cbCol;
            BYTE* pSrcTemp;

            if (bProvOwn && pColCur->wType == pBindCur->wType)
            {
                pSrcTemp = ((BYTE*)(pSrcData) + pColCur->cbOffset);
            }
            else
            {
                BYTE* pDstTemp = (BYTE*)pDstData + pBindCur->obValue;
                switch (pColCur->wType)
                {
                    case DBTYPE_STR:
                    cbCol = lstrlenA((LPSTR)(((BYTE*)pSrcData) + pColCur->cbOffset));
                    break;
                    case DBTYPE_WSTR:
                    case DBTYPE_BSTR:
                    cbCol = lstrlenW((LPWSTR)(((BYTE*)pSrcData) + pColCur->cbOffset)) * sizeof(WCHAR);
                    break;
                    default:
                    cbCol = pColCur->ulColumnSize;
                    break;
                }
                if (pBindCur->dwPart & DBPART_VALUE)
                {
                    hr = spConvert->DataConvert(pColCur->wType, pBindCur->wType,
                                                cbCol, &cbDst, (BYTE*)(pSrcData) + pColCur->cbOffset,
                                                pDstTemp, pBindCur->cbMaxLen, dbStat, &dbStat,
                                                pBindCur->bPrecision, pBindCur->bScale,0);
                }
            }
            if (pBindCur->dwPart & DBPART_LENGTH)
            *((ULONG*)((BYTE*)(pDstData) + pBindCur->obLength)) = cbDst;
            if (pBindCur->dwPart & DBPART_STATUS)
            *((DBSTATUS*)((BYTE*)(pDstData) + pBindCur->obStatus)) = dbStat;
            if (FAILED(hr))
            return hr;
        }
        return hr;
    }

    HRESULT CreateRow(LONG lRowsOffset, ULONG& cRowsObtained, HROW* rgRows)
    {
        RowClass* pRow = NULL;
        ATLASSERT(lRowsOffset >= 0);
        RowClass::KeyType key = lRowsOffset+1;
        ATLASSERT(key > 0);
        pRow = m_rgRowHandles.Lookup(key);
        if (pRow == NULL)
        {
            ATLTRY(pRow = new RowClass(lRowsOffset))
            if (pRow == NULL)
            return E_OUTOFMEMORY;
            if (!m_rgRowHandles.Add(key, pRow))
            return E_OUTOFMEMORY;
        }
        pRow->AddRefRow();
        m_bReset = false;
        rgRows[cRowsObtained++] = (HROW)key;
        return S_OK;
    }

    STDMETHOD(GetNextRows)(HCHAPTER /*hReserved*/,
                           LONG lRowsOffset,
                           LONG cRows,
                           ULONG *pcRowsObtained,
                           HROW **prghRows)
    {
        LONG lTmpRows = lRowsOffset;
        ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::GetNextRows\n");
        if (pcRowsObtained != NULL)
        *pcRowsObtained = 0;
        if (prghRows == NULL || pcRowsObtained == NULL)
        return E_INVALIDARG;
        if (cRows == 0)
        return S_OK;
        HRESULT hr = S_OK;
        T* pT = (T*) this;
        T::ObjectLock cab(pT);
        if (lRowsOffset < 0 && !m_bCanScrollBack)
        return DB_E_CANTSCROLLBACKWARDS;
        if (cRows < 0  && !m_bCanFetchBack)
        return DB_E_CANTFETCHBACKWARDS;

        // Calculate # of rows in set and the base fetch position.  If the rowset
        // is at its head position, then lRowOffset < 0 means moving from the BACK
        // of the rowset and not the front.
        LONG cRowsInSet = pT->m_rgRowData.GetSize();
        if (((lRowsOffset == LONG_MIN) && (cRowsInSet != LONG_MIN))
            || (abs(lRowsOffset)) > cRowsInSet ||
            (abs(lRowsOffset) == cRowsInSet && lRowsOffset < 0 && cRows < 0) ||
            (abs(lRowsOffset) == cRowsInSet && lRowsOffset > 0 && cRows > 0))
        return DB_S_ENDOFROWSET;

        // In the case where the user is moving backwards after moving forwards,
        // we do not wrap around to the end of the rowset.
        if ((m_iRowset == 0 && !m_bReset && cRows < 0) ||
            (((LONG)m_iRowset + lRowsOffset) > cRowsInSet) ||
            (m_iRowset == (DWORD)cRowsInSet && lRowsOffset >= 0 && cRows > 0))
        return DB_S_ENDOFROWSET;

        // Note, if m_bReset, m_iRowset must be 0
        if (lRowsOffset < 0 && m_bReset)
        {
            ATLASSERT(m_iRowset == 0);
            m_iRowset = cRowsInSet;
        }

        int iStepSize = cRows >= 0 ? 1 : -1;

        // If cRows == LONG_MIN, we can't use ABS on it.  Therefore, we reset it
        // to a value just greater than cRowsInSet
        if (cRows == LONG_MIN && cRowsInSet != LONG_MIN)
        cRows = cRowsInSet + 2; // set the value to something we can deal with
        else
        cRows = abs(cRows);

        if (iStepSize < 0 && m_iRowset == 0 && m_bReset && lRowsOffset <= 0)
        m_iRowset = cRowsInSet;

        lRowsOffset += m_iRowset;

        *pcRowsObtained = 0;
        CAutoMemRelease<HROW, CComFree< HROW > > amr;
        if (*prghRows == NULL)
        {
            int cHandlesToAlloc = (cRows > cRowsInSet) ? cRowsInSet : cRows;
            if (iStepSize == 1 && (cRowsInSet - lRowsOffset) < cHandlesToAlloc)
            cHandlesToAlloc = cRowsInSet - lRowsOffset;
            if (iStepSize == -1 && lRowsOffset < cHandlesToAlloc)
            cHandlesToAlloc = lRowsOffset;
            *prghRows = (HROW*)CoTaskMemAlloc((cHandlesToAlloc) * sizeof(HROW*));
            amr.Attach(*prghRows);
        }
        if (*prghRows == NULL)
        return E_OUTOFMEMORY;
        while ((lRowsOffset >= 0 && cRows != 0) &&
               ((lRowsOffset < cRowsInSet) || (lRowsOffset <= cRowsInSet && iStepSize < 0)))
        {
            // cRows > cRowsInSet && iStepSize < 0
            if (lRowsOffset == 0 && cRows > 0 && iStepSize < 0)
            break;

            // in the case where we have iStepSize < 0, move the row back
            // further because we want the previous row
            LONG lRow = lRowsOffset;
            if ((lRowsOffset == 0) && (lTmpRows == 0) && (iStepSize < 0))
            lRow = cRowsInSet;

            if (iStepSize < 0)
            lRow += iStepSize;

            hr = pT->CreateRow(lRow, *pcRowsObtained, *prghRows);
            if (FAILED(hr))
            {
                RefRows(*pcRowsObtained, *prghRows, NULL, NULL, FALSE);
                for (ULONG iRowDel = 0; iRowDel < *pcRowsObtained; iRowDel++)
                *prghRows[iRowDel] = NULL;
                *pcRowsObtained = 0;
                return hr;
            }
            cRows--;
            lRowsOffset += iStepSize;
        }

        if ((lRowsOffset >= cRowsInSet && cRows) || (lRowsOffset < 0 && cRows)  ||
            (lRowsOffset == 0 && cRows > 0 && iStepSize < 0))
        hr = DB_S_ENDOFROWSET;
        m_iRowset = lRowsOffset;
        if (SUCCEEDED(hr))
        amr.Detach();
        return hr;
    }

    STDMETHOD(ReleaseRows)(ULONG cRows,
                           const HROW rghRows[],
                           DBROWOPTIONS rgRowOptions[],
                           ULONG rgRefCounts[],
                           DBROWSTATUS rgRowStatus[])
    {
        ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::ReleaseRows\n");
        if (cRows == 0)
        return S_OK;
        rgRowOptions;
        return RefRows(cRows, rghRows, rgRefCounts, rgRowStatus, FALSE);
    }

    STDMETHOD(RestartPosition)(HCHAPTER /*hReserved*/)
    {
        ATLTRACE2(atlTraceDBProvider, 0, "IRowsetImpl::RestartPosition\n");
        m_iRowset = 0;
        m_bReset = true;
        return S_OK;
    }

    MapClass  m_rgRowHandles;
    DWORD     m_iRowset; // cursor
    unsigned  m_bCanScrollBack:1;
    unsigned  m_bCanFetchBack:1;
    unsigned  m_bReset:1;
};

