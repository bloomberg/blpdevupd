// SPDX-FileCopyrightText: 2020 Bloomberg Finance LP
// SPDX-License-Identifier: GPL2.0-or-later

// dfutransport.cpp
#include "dfutransport.h"

#include <iostream>
#include <vector>
#include <unordered_map>



static std::unordered_map<int, DFUTransport*> s_transMap;

DFUTransport::DFUTransport()
    :dl(nullptr)
    , dfu_open(nullptr)
    , dfu_close(nullptr)
    , inited(false)
    , handle(-1)
    , hinstLib(nullptr)
    , dl_cb(nullptr)
{

}

int DFUTransport::init()
{
    int ret = 0;
    inited = false;
    hinstLib = LoadLibrary(TEXT("libdfu.dll"));
    if (hinstLib == NULL) {
        std::cout << "Fail to load dll" << std::endl;
        std::cout << GetLastError() << std::endl;

        ret = -1;
        goto done;
    }
    dl = (f_download_t)GetProcAddress(hinstLib, "download");
    if (NULL == dl) {
        std::cout << "Fail to find download method" << std::endl;
        ret = -1;
        goto done;

    }
    dfu_open = (dfu_open_t)GetProcAddress(hinstLib, "open_device");
    if (NULL == dfu_open) {
        std::cout << "Fail to find open method" << std::endl;
        ret = -1;
        goto done;
    }

    dfu_close = (dfu_close_t)GetProcAddress(hinstLib, "close_device");
    if (NULL == dfu_close) {
        ret = -1;
        goto done;
    }
    inited = true;
done:
    return ret;
}

int DFUTransport::open(uint16_t vid, uint16_t pid)
{
    if (!inited) {
        return -1;
    }
    int ret = dfu_open(vid, pid);
    if (ret < 0) {
        std::cout << "Fail to open device" << std::endl;
        return -1;
    }
    handle = ret;
    s_transMap[handle] = this;
    return 0;

}

int DFUTransport::download(std::vector<uint8_t> data, std::function<void(int, int)> cb)
{
    if (!inited) {
        return -1;
    }
    dl_cb = cb;
    int ret = dl(handle, data.data(), data.size(), [](int handle, size_t sent, size_t total) {
        s_transMap[handle]->dl_cb(sent, total);
        });
    if (ret < 0) {
        return -1;
    }
    else {
        return ret;
    }
}

int DFUTransport::close()
{
    if (!inited) {
        return -1;
    }
    int ret = dfu_close(handle);
    if (ret < 0) {
        ret = -1;
    }
    return ret;
}
