/******************************************************************************
 *
 * Project:  WMS Client Driver
 * Purpose:  Implementation of Dataset and RasterBand classes for WMS
 *           and other similar services.
 * Author:   Adam Nowacki, nowak@xpam.de
 *
 ******************************************************************************
 * Copyright (c) 2007, Adam Nowacki
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

#include "stdinc.h"

GDALWMSRasterBand::GDALWMSRasterBand(GDALWMSDataset *parent_dataset, int band, double scale) {
    //	printf("[%p] GDALWMSRasterBand::GDALWMSRasterBand(%p, %d, %f)\n", this, parent_dataset, band, scale);
    m_parent_dataset = parent_dataset;
    m_scale = scale;
    m_overview = -1;

    poDS = parent_dataset;
    nRasterXSize = static_cast<int>(m_parent_dataset->m_data_window.m_sx * scale + 0.5);
    nRasterYSize = static_cast<int>(m_parent_dataset->m_data_window.m_sy * scale + 0.5);
    nBand = band + 1;
    eDataType = m_parent_dataset->m_data_type;
    nBlockXSize = m_parent_dataset->m_block_size_x;
    nBlockYSize = m_parent_dataset->m_block_size_y;
}

GDALWMSRasterBand::~GDALWMSRasterBand() {
    for (std::vector<GDALWMSRasterBand *>::iterator it = m_overviews.begin(); it != m_overviews.end(); ++it) {
        GDALWMSRasterBand *p = *it;
        delete p;
    }
}

CPLErr GDALWMSRasterBand::IReadBlock(int x, int y, void *buffer) {
    CPLErr ret = CE_None;

    //	printf("[%p] GDALWMSRasterBand::IReadBlock(%d, %d, %p), band: %d, overview: %d\n", this, x, y, buffer, nBand, m_overview);
    int bx0 = x;
    int by0 = y;
    int bx1 = x;
    int by1 = y;
    int i;

    if ((m_parent_dataset->m_hint.m_valid) && (m_parent_dataset->m_hint.m_overview == m_overview)) {
        int tbx0 = m_parent_dataset->m_hint.m_x0 / nBlockXSize;
        int tby0 = m_parent_dataset->m_hint.m_y0 / nBlockYSize;
        int tbx1 = (m_parent_dataset->m_hint.m_x0 + m_parent_dataset->m_hint.m_sx - 1) / nBlockXSize;
        int tby1 = (m_parent_dataset->m_hint.m_y0 + m_parent_dataset->m_hint.m_sy - 1) / nBlockYSize;
        if ((tbx0 <= bx0) && (tby0 <= by0) && (tbx1 >= bx1) && (tby1 >= by1)) {
            bx0 = tbx0;
            by0 = tby0;
            bx1 = tbx1;
            by1 = tby1;
        }
    }
    //	printf("Loading blocks %d %d %d %d\n", bx0, by0, bx1, by1);

    int max_request_count = (bx1 - bx0 + 1) * (by1 - by0 + 1);
    int request_count = 0;
    CPLHTTPRequest *download_requests = new CPLHTTPRequest[max_request_count];
    GDALWMSCache *cache = m_parent_dataset->m_cache;
    struct BlockXY {
        int x, y;
    } *download_blocks = new BlockXY[max_request_count];

    for (int iy = by0; iy <= by1; ++iy) {
        for (int ix = bx0; ix <= bx1; ++ix) {
            bool need_this_block = false;
            for (int ib = 1; ib <= m_parent_dataset->nBands; ++ib) {
                if ((ix == x) && (iy == y) && (ib == nBand)) {
                    need_this_block = true;
                } else {
                    GDALWMSRasterBand *band = static_cast<GDALWMSRasterBand *>(m_parent_dataset->GetRasterBand(ib));
                    if (m_overview >= 0) band = static_cast<GDALWMSRasterBand *>(band->GetOverview(m_overview));
                    if (!band->IsBlockInCache(ix, iy)) need_this_block = true;
                }
            }
            CPLString url;
            if (need_this_block) {
                CPLString file_name;
                AskMiniDriverForBlock(&url, ix, iy);
                if ((cache != NULL) && (cache->Read(url.c_str(), &file_name) == CE_None)) {
                    void *p = 0;
                    if ((ix == x) && (iy == y)) p = buffer;
                    if (ReadBlockFromFile(ix, iy, file_name.c_str(), nBand, p) == CE_None) need_this_block = false;
                }
            }
            if (need_this_block) {
                CPLHTTPInitializeRequest(&download_requests[request_count], url.c_str());
                download_blocks[request_count].x = ix;
                download_blocks[request_count].y = iy;
                ++request_count;
            }
        }
    }

    if (request_count > 0) {
        if (CPLHTTPFetchMulti(download_requests, request_count) != CE_None) {
            CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: CPLHTTPFetchMulti failed.");
            ret = CE_Failure;
        }
    }

    for (i = 0; i < request_count; ++i) {
        if (ret == CE_None) {
            if ((download_requests[i].nStatus == 200) && (download_requests[i].pabyData != NULL) && (download_requests[i].nDataLen > 0)) {
                CPLString file_name(BufferToVSIFile(download_requests[i].pabyData, download_requests[i].nDataLen));
                if (file_name.size() > 0) {
                    /* check for error xml */
                    if (download_requests[i].nDataLen >= 20) {
                        const char *download_data = reinterpret_cast<char *>(download_requests[i].pabyData);
                        if (EQUALN(download_data, "<?xml ", 6) 
                        || EQUALN(download_data, "<!DOCTYPE ", 10)
                        || EQUALN(download_data, "<ServiceException", 17)) {
                            if (ReportWMSException(file_name.c_str()) != CE_None) {
                                CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: The server returned unknown exception.");
                            }
                            ret = CE_Failure;
                        }
                    }
                    if (ret == CE_None) {
                        void *p = 0;
                        if ((download_blocks[i].x == x) && (download_blocks[i].y == y)) p = buffer;
                        if (ReadBlockFromFile(download_blocks[i].x, download_blocks[i].y, file_name.c_str(), nBand, p) == CE_None) {
                            if (cache != NULL) {
                                cache->Write(download_requests[i].pszURL, file_name);
                            }
                        } else {
                            CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: ReadBlockFromFile failed.");
                            ret = CE_Failure;
                        }
                    }
                    VSIUnlink(file_name.c_str());
                }
            } else if (download_requests[i].nStatus == 204) {
                void *p = 0;
                if ((download_blocks[i].x == x) && (download_blocks[i].y == y)) p = buffer;
                if (ZeroBlock(download_blocks[i].x, download_blocks[i].y, nBand, p) != CE_None) {
                    CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: ZeroBlock failed.");
                    ret = CE_Failure;
                }
            } else {
                CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: Unable to download block %d, %d (%s). HTTP status code: %d",
                    download_blocks[i].x, download_blocks[i].y, download_requests[i].pszURL, download_requests[i].nStatus);
                ret = CE_Failure;
            }
        }
        CPLHTTPCleanupRequest(&download_requests[i]);
    }
    delete[] download_blocks;
    delete[] download_requests;

    return ret;
}

CPLErr GDALWMSRasterBand::IRasterIO(GDALRWFlag rw, int x0, int y0, int sx, int sy, void *buffer, int bsx, int bsy, GDALDataType bdt, int pixel_space, int line_space) {
    CPLErr ret;

    if (rw != GF_Read) return CE_Failure;
    if (buffer == NULL) return CE_Failure;
    if ((sx == 0) || (sy == 0) || (bsx == 0) || (bsy == 0)) return CE_None;

    m_parent_dataset->m_hint.m_x0 = x0;
    m_parent_dataset->m_hint.m_y0 = y0;
    m_parent_dataset->m_hint.m_sx = sx;
    m_parent_dataset->m_hint.m_sy = sy;
    m_parent_dataset->m_hint.m_overview = m_overview;
    m_parent_dataset->m_hint.m_valid = true;
    ret = GDALRasterBand::IRasterIO(rw, x0, y0, sx, sy, buffer, bsx, bsy, bdt, pixel_space, line_space);
    m_parent_dataset->m_hint.m_valid = false;

    return ret;
}

int GDALWMSRasterBand::HasArbitraryOverviews() {
//    return m_parent_dataset->m_mini_driver_caps.m_has_arb_overviews;
    return 0; // not implemented yet
}

int GDALWMSRasterBand::GetOverviewCount() {
    return m_overviews.size();
}

GDALRasterBand *GDALWMSRasterBand::GetOverview(int n) {
    if ((m_overviews.size() > 0) && (static_cast<size_t>(n) < m_overviews.size())) return m_overviews[n];
    else return NULL;
}

void GDALWMSRasterBand::AddOverview(double scale) {
    int i;
    GDALWMSRasterBand *overview = new GDALWMSRasterBand(m_parent_dataset, nBand - 1, scale);
    std::vector<GDALWMSRasterBand *>::iterator it = m_overviews.begin();
    for (; it != m_overviews.end(); ++it) {
        GDALWMSRasterBand *p = *it;
        if (p->m_scale < scale) break;
    }
    m_overviews.insert(it, overview);
    it = m_overviews.begin();
    for (i = 0; it != m_overviews.end(); ++it, ++i) {
        GDALWMSRasterBand *p = *it;
        p->m_overview = i;
    }
}

bool GDALWMSRasterBand::IsBlockInCache(int x, int y) {
    bool ret = false;
    GDALRasterBlock *b = TryGetLockedBlockRef(x, y);
    if (b != NULL) {
        ret = true;
        b->DropLock();
    }
    return ret;
}

void GDALWMSRasterBand::AskMiniDriverForBlock(CPLString *url, int x, int y) {
    GDALWMSImageRequestInfo iri;
    GDALWMSTiledImageRequestInfo tiri;

    const int x0 = MIN(MAX(0, x * nBlockXSize), nRasterXSize);
    const int y0 = MIN(MAX(0, y * nBlockYSize), nRasterYSize);
    const int x1 = MIN(MAX(0, (x + 1) * nBlockXSize), nRasterXSize);
    const int y1 = MIN(MAX(0, (y + 1) * nBlockYSize), nRasterYSize);
    const double rx = (m_parent_dataset->m_data_window.m_x1 - m_parent_dataset->m_data_window.m_x0) / static_cast<double>(nRasterXSize);
    const double ry = (m_parent_dataset->m_data_window.m_y1 - m_parent_dataset->m_data_window.m_y0) / static_cast<double>(nRasterYSize);
    /* Use different method for x0,y0 and x1,y1 to make sure calculated values are exact for corner requests */
    iri.m_x0 = x0 * rx + m_parent_dataset->m_data_window.m_x0;
    iri.m_y0 = y0 * ry + m_parent_dataset->m_data_window.m_y0;
    iri.m_x1 = m_parent_dataset->m_data_window.m_x1 - (nRasterXSize - x1) * rx;
    iri.m_y1 = m_parent_dataset->m_data_window.m_y1 - (nRasterYSize - y1) * ry;
    iri.m_sx = x1 - x0;
    iri.m_sy = y1 - y0;

    int level = m_overview + 1;
    tiri.m_x = (m_parent_dataset->m_data_window.m_tx >> level) + x;
    tiri.m_y = (m_parent_dataset->m_data_window.m_ty >> level) + y;
    tiri.m_level = m_parent_dataset->m_data_window.m_tlevel - level;

    m_parent_dataset->m_mini_driver->TiledImageRequest(url, iri, tiri);
}

CPLErr GDALWMSRasterBand::ReadBlockFromFile(int x, int y, const char *file_name, int to_buffer_band, void *buffer) {
    CPLErr ret = CE_None;
    GDALDataset *ds = 0;
    GByte *color_table = NULL;
    int i;

    /* expected size */
    const int esx = MIN(MAX(0, (x + 1) * nBlockXSize), nRasterXSize) - MIN(MAX(0, x * nBlockXSize), nRasterXSize);
    const int esy = MIN(MAX(0, (y + 1) * nBlockYSize), nRasterYSize) - MIN(MAX(0, y * nBlockYSize), nRasterYSize);
    ds = reinterpret_cast<GDALDataset*>(GDALOpen(file_name, GA_ReadOnly));
    if (ds != NULL) {
        int sx = ds->GetRasterXSize();
        int sy = ds->GetRasterYSize();
        /* Allow bigger than expected so pre-tiled constant size images work on corners */
        if ((sx > nBlockXSize) || (sy > nBlockYSize) || (sx < esx) || (sy < esy)) {
            CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: Incorrect size %d x %d of downloaded block, expected %d x %d, max %d x %d.",
                sx, sy, esx, esy, nBlockXSize, nBlockYSize);
            ret = CE_Failure;
        }
        if (ret == CE_None) {
            if (ds->GetRasterCount() != m_parent_dataset->m_bands_count) {
                /* Maybe its an image with color table */
                bool accepted_as_ct = false;
                if ((eDataType == GDT_Byte) && (ds->GetRasterCount() == 1)) {
                    GDALRasterBand *rb = ds->GetRasterBand(1);
                    if (rb->GetRasterDataType() == GDT_Byte) {
                        GDALColorTable *ct = rb->GetColorTable();
                        if (ct != NULL) {
                            color_table = new GByte[256 * 4];
                            const int count = MIN(256, ct->GetColorEntryCount());
                            for (i = 0; i < count; ++i) {
                                GDALColorEntry ce;
                                ct->GetColorEntryAsRGB(i, &ce);
                                color_table[i] = static_cast<GByte>(ce.c1);
                                color_table[i + 256] = static_cast<GByte>(ce.c2);
                                color_table[i + 512] = static_cast<GByte>(ce.c3);
                                color_table[i + 768] = static_cast<GByte>(ce.c4);
                            }
                            for (i = count; i < 256; ++i) {
                                color_table[i] = 0;
                                color_table[i + 256] = 0;
                                color_table[i + 512] = 0;
                                color_table[i + 768] = 0;
                            }
                            accepted_as_ct = true;
                        }
                    }
                }
                if (!accepted_as_ct) {
                    CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: Incorrect bands count %d in downloaded block, expected %d.",
                        ds->GetRasterCount(), m_parent_dataset->m_bands_count);
                    ret = CE_Failure;
                }
            }
        }
        for (int ib = 1; ib <= m_parent_dataset->nBands; ++ib) {
            if (ret == CE_None) {
                void *p = NULL;
                GDALRasterBlock *b = NULL;
                if ((buffer != NULL) && (ib == to_buffer_band)) {
                    p = buffer;
                } else {
                    GDALWMSRasterBand *band = static_cast<GDALWMSRasterBand *>(m_parent_dataset->GetRasterBand(ib));
                    if (m_overview >= 0) band = static_cast<GDALWMSRasterBand *>(band->GetOverview(m_overview));
                    if (!band->IsBlockInCache(x, y)) {
                        b = band->GetLockedBlockRef(x, y, true);
                        if (b != NULL) {
                            p = b->GetDataRef();
                        }
                    }
                }
                if (p != NULL) {
                    int pixel_space = GDALGetDataTypeSize(eDataType) / 8;
                    int line_space = pixel_space * nBlockXSize;
                    if (color_table == NULL) {
                        if (ds->RasterIO(GF_Read, 0, 0, sx, sy, p, sx, sy, eDataType, 1, &ib, pixel_space, line_space, 0) != CE_None) {
                            CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: RasterIO failed on downloaded block.");
                            ret = CE_Failure;
                        }
                    } else if (ib <= 4) {
                        if (ds->RasterIO(GF_Read, 0, 0, sx, sy, p, sx, sy, eDataType, 1, NULL, pixel_space, line_space, 0) != CE_None) {
                            CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: RasterIO failed on downloaded block.");
                            ret = CE_Failure;
                        }
                        if (ret == CE_None) {
                            GByte *band_color_table = color_table + 256 * (ib - 1);
                            GByte *byte_buffer = reinterpret_cast<GByte *>(p);
                            for (int y = 0; y < sy; ++y) {
                                for (int x = 0; x < sx; ++x) {
                                    const int offset = x + y * line_space;
                                    byte_buffer[offset] = band_color_table[byte_buffer[offset]];
                                }
                            }
                        }
                    } else {
                        CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: Color table supports at most 4 components.");
                        ret = CE_Failure;
                    }
                } else {
                    CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: GetDataRef returned NULL.");
                    ret = CE_Failure;
                }
                if (b != NULL) {
                    b->DropLock();
                }
            }
        }
        GDALClose(ds);
    } else {
        CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: Unable to open downloaded block.");
        ret = CE_Failure;
    }

    if (color_table != NULL) {
        delete[] color_table;
    }

    return ret;
}

CPLErr GDALWMSRasterBand::ZeroBlock(int x, int y, int to_buffer_band, void *buffer) {
    CPLErr ret = CE_None;

    for (int ib = 1; ib <= m_parent_dataset->nBands; ++ib) {
        if (ret == CE_None) {
            void *p = NULL;
            GDALRasterBlock *b = NULL;
            if ((buffer != NULL) && (ib == to_buffer_band)) {
                p = buffer;
            } else {
                GDALWMSRasterBand *band = static_cast<GDALWMSRasterBand *>(m_parent_dataset->GetRasterBand(ib));
                if (m_overview >= 0) band = static_cast<GDALWMSRasterBand *>(band->GetOverview(m_overview));
                if (!band->IsBlockInCache(x, y)) {
                    b = band->GetLockedBlockRef(x, y, true);
                    if (b != NULL) {
                        p = b->GetDataRef();
                    }
                }
            }
            if (p != NULL) {
                unsigned char *b = reinterpret_cast<unsigned char *>(p);
                int block_size = nBlockXSize * nBlockYSize * (GDALGetDataTypeSize(eDataType) / 8);
                for (int i = 0; i < block_size; ++i) b[i] = 0;
            } else {
                CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: GetDataRef returned NULL.");
                ret = CE_Failure;
            }
            if (b != NULL) {
                b->DropLock();
            }
        }
    }

    return ret;
}

CPLErr GDALWMSRasterBand::ReportWMSException(const char *file_name) {
    CPLErr ret = CE_None;
    int reported_errors_count = 0;

    CPLXMLNode *orig_root = CPLParseXMLFile(file_name);
    CPLXMLNode *root = orig_root;
    if (root != NULL) {
        root = CPLGetXMLNode(root, "=ServiceExceptionReport");
    }
    if (root != NULL) {
        CPLXMLNode *n = CPLGetXMLNode(root, "ServiceException");
        while (n != NULL) {
            const char *exception = CPLGetXMLValue(n, "=ServiceException", "");
            const char *exception_code = CPLGetXMLValue(n, "=ServiceException.code", "");
            if (exception[0] != '\0') {
                if (exception_code[0] != '\0') {
                    CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: The server returned exception code '%s': %s", exception_code, exception);
                    ++reported_errors_count;
                } else {
                    CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: The server returned exception: %s", exception);
                    ++reported_errors_count;
                }
            } else if (exception_code[0] != '\0') {
                CPLError(CE_Failure, CPLE_AppDefined, "GDALWMS: The server returned exception code '%s'.", exception_code);
                ++reported_errors_count;
            }

            n = n->psNext;
            if (n != NULL) {
                n = CPLGetXMLNode(n, "=ServiceException");
            }
        }
    } else {
        ret = CE_Failure;
    }
    if (orig_root != NULL) {
        CPLDestroyXMLNode(orig_root);
    }

    if (reported_errors_count == 0) {
        ret = CE_Failure;
    }

    return ret;
}
