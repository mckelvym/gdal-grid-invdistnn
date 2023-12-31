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

static volatile GDALWMSMiniDriverManager *g_mini_driver_manager = NULL;
static void *g_mini_driver_manager_mutex = NULL;

GDALWMSMiniDriver::GDALWMSMiniDriver() {
    m_parent_dataset = 0;
}

GDALWMSMiniDriver::~GDALWMSMiniDriver() {
}

CPLErr GDALWMSMiniDriver::Initialize(CPLXMLNode *config) {
    return CE_None;
}

void GDALWMSMiniDriver::GetCapabilities(GDALWMSMiniDriverCapabilities *caps) {
}

void GDALWMSMiniDriver::ImageRequest(CPLString *url, const GDALWMSImageRequestInfo &iri) {
}

void GDALWMSMiniDriver::TiledImageRequest(CPLString *url, const GDALWMSImageRequestInfo &iri, const GDALWMSTiledImageRequestInfo &tiri) {
}

const char *GDALWMSMiniDriver::GetProjectionInWKT() {
    return NULL;
}

GDALWMSMiniDriverFactory::GDALWMSMiniDriverFactory() {
}

GDALWMSMiniDriverFactory::~GDALWMSMiniDriverFactory() {
}

GDALWMSMiniDriverManager *GetGDALWMSMiniDriverManager() {
    if (g_mini_driver_manager == 0) {
        CPLMutexHolderD(&g_mini_driver_manager_mutex);
        if (g_mini_driver_manager == 0) {
            g_mini_driver_manager = new GDALWMSMiniDriverManager();
        }
        CPLAssert(g_mini_driver_manager != NULL);
    }
    return const_cast<GDALWMSMiniDriverManager *>(g_mini_driver_manager);
}

void DestroyWMSMiniDriverManager()

{
    CPLMutexHolderD(&g_mini_driver_manager_mutex);

    if( g_mini_driver_manager != 0 )
    {
        delete g_mini_driver_manager;
        g_mini_driver_manager = NULL;
    }
}

GDALWMSMiniDriverManager::GDALWMSMiniDriverManager() {
}

GDALWMSMiniDriverManager::~GDALWMSMiniDriverManager() {
    for (std::list<GDALWMSMiniDriverFactory *>::iterator it = m_mdfs.begin(); 
         it != m_mdfs.end(); ++it) {
        GDALWMSMiniDriverFactory *mdf = *it;
        delete mdf;
    }
}

void GDALWMSMiniDriverManager::Register(GDALWMSMiniDriverFactory *mdf) {
    CPLMutexHolderD(&g_mini_driver_manager_mutex);

    m_mdfs.push_back(mdf);
}

GDALWMSMiniDriverFactory *GDALWMSMiniDriverManager::Find(const CPLString &name) {
    CPLMutexHolderD(&g_mini_driver_manager_mutex);

    for (std::list<GDALWMSMiniDriverFactory *>::iterator it = m_mdfs.begin(); it != m_mdfs.end(); ++it) {
        GDALWMSMiniDriverFactory *const mdf = *it;
        if (EQUAL(mdf->GetName().c_str(), name.c_str())) return mdf;
    }
    return NULL;
}
