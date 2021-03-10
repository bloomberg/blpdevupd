// SPDX-FileCopyrightText: 2020 Bloomberg Finance LP
// SPDX-License-Identifier: GPL2.0-or-later

// dfutransport.h
#ifndef DFUTRANSPORT_H
#define DFUTRANSPORT_H

#include <windows.h>
#include <cstdint>
#include <vector>
#include <functional>

class DFUTransport
{
public:
	DFUTransport();
	int init();
	int open(uint16_t vid, uint16_t pid);
	int download(std::vector<uint8_t> data, std::function<void(int, int)>);
	int close();
	std::function<void(int, int)> dl_cb;
private:
	
	HINSTANCE hinstLib;
	typedef void(*download_cb)(int handle, size_t bytes_sent, size_t bytes_total);
	typedef int(*f_download_t)(int, uint8_t *din, size_t ilen, download_cb cb);
	typedef int(*dfu_open_t)(uint16_t vid, uint16_t pid);
	typedef int(*dfu_close_t)(int);
	f_download_t dl;
	dfu_open_t dfu_open;
	dfu_close_t dfu_close;
	bool inited;
	int handle;

};

#endif // !DFUTRANSPORT_H
