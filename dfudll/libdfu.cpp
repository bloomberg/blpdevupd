/*
 * This code referenced to the original main.c in dfu-util project, the
 * original header is pasted below.
 */
/*
 * dfu-util
 *
 * Copyright 2007-2008 by OpenMoko, Inc.
 * Copyright 2010-2012 Stefan Schmidt
 * Copyright 2013-2014 Hans Petter Selasky <hps@bitfrost.no>
 * Copyright 2010-2020 Tormod Volden
 *
 * Originally written by Harald Welte <laforge@openmoko.org>
 *
 * Based on existing code of dfu-programmer-0.4
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include "libusb.h"
#include <unordered_map>
#include <memory>

// Include C header from dfu-util project
extern "C"
{
#include "dfu_file.h"
#include "dfu.h"
#include "dfu_load.h"
#include "dfuse_mem.h"
#include "dfuse.h"
#include "dfu_util.h"
#include "portable.h"
#include "libdfu_util.h"
}

/* Must define this in application*/

int verbose = 0;

//static dfu_util_t dfu_util;
static int g_handle = 0;
static std::unordered_map<int, std::shared_ptr<dfu_util_t>> deviceMap(5);


extern "C" int open_device(uint16_t vid, uint16_t pid)
{
	int ret = 0;
	std::shared_ptr<dfu_util_t> dfu_util = std::make_shared<dfu_util_t>();
	dfu_util_init(dfu_util.get());

	if ((0 != (ret = libusb_init(&(dfu_util->ctx))))) {
		printf("unable to initialize libusb: %i", ret);
		ret = -1;
		goto done;
	}

	if (vid != 0) {
		dfu_util->match_vendor_dfu = vid;
		dfu_util->match_product_dfu = pid;
		dfu_util->match_vendor = 0x10000;
		dfu_util->match_product = 0x10000;
	}
	
	probe_devices(dfu_util.get());

	list_dfu_interfaces(dfu_util.get());

	if (NULL == dfu_util->dfu_root) {
		printf("No DFU capable USB device available\n");
		ret = -1;
		goto done;
	}

	printf("Opening DFU capable USB device...\n");
	ret = libusb_open(dfu_util->dfu_root->dev, &dfu_util->dfu_root->dev_handle);
	if (ret || !dfu_util->dfu_root->dev_handle) {
		printf("Cannot open device: %s", libusb_error_name(ret));
		goto done;
	}

	printf("ID %04x:%04x\n", dfu_util->dfu_root->vendor, dfu_util->dfu_root->product);

	printf("Run-time device DFU version %04x\n",
		libusb_le16_to_cpu(dfu_util->dfu_root->func_dfu.bcdDFUVersion));

	deviceMap[++g_handle] = dfu_util;
	ret = g_handle;

done:
	return ret;
}

typedef void(*libdfu_download_cb)(int handle, size_t bytes_sent, size_t bytes_total);
extern "C"  int download(int handle, uint8_t *din, size_t ilen, libdfu_download_cb cb)
{
	int ret = 0;
	
	struct dfu_file file;
	struct dfu_if dif;
	struct memsegment mem;
	struct memsegment *pmem;
	uint16_t runtime_vendor;
	uint16_t runtime_product;

	libusb_device_handle *hdl;
	struct dfu_if dfuif;
	struct dfu_status status;
	int dfuse_device = 0;
	unsigned int transfer_size = 0;

	auto it = deviceMap.find(handle);

	if (deviceMap.end() == it) {
		return -1;
	}
	auto dfu_util = it->second;

	memset(&file, 0, sizeof(file));


	runtime_vendor = dfu_util->match_vendor < 0 ? dfu_util->dfu_root->vendor : dfu_util->match_vendor;
	runtime_product = dfu_util->match_product < 0 ? dfu_util->dfu_root->product : dfu_util->match_product;

	printf("Claiming USB DFU Interface...\n");
	ret = libusb_claim_interface(dfu_util->dfu_root->dev_handle, dfu_util->dfu_root->interface);
	if (ret < 0) {
		printf("Cannot claim interface - %s", libusb_error_name(ret));
		goto done;
	}

	printf("Setting Alternate Setting #%d ...\n", dfu_util->dfu_root->altsetting);
	ret = libusb_set_interface_alt_setting(dfu_util->dfu_root->dev_handle, dfu_util->dfu_root->interface, dfu_util->dfu_root->altsetting);
	if (ret < 0) {
		printf("Cannot set alternate interface: %s", libusb_error_name(ret));
		goto done;
	}

status_again:
	printf("Determining device status: ");
	ret = dfu_get_status(dfu_util->dfu_root, &status);
	if (ret < 0) {
		printf("error get_status: %s\n", libusb_error_name(ret));
	}
	printf("state = %s, status = %d\n",
		dfu_state_to_string(status.bState), status.bStatus);

	milli_sleep(status.bwPollTimeout);

	switch (status.bState) {
	case DFU_STATE_appIDLE:
	case DFU_STATE_appDETACH:
		printf("Device still in Runtime Mode!\n");
		break;
	case DFU_STATE_dfuERROR:
		printf("dfuERROR, clearing status\n");
        ret = dfu_clear_status(dfu_util->dfu_root->dev_handle, dfu_util->dfu_root->interface);
        if (ret < 0) {
            printf("error clear_status, ret = %s\n", libusb_error_name(ret));
            // If device is in bad status, abort retry to avoid looping
            return ret;
        }
        goto status_again;
		break;
	case DFU_STATE_dfuDNLOAD_IDLE:
	case DFU_STATE_dfuUPLOAD_IDLE:
		printf("aborting previous incomplete transfer\n");
        ret = dfu_abort(dfu_util->dfu_root->dev_handle, dfu_util->dfu_root->interface);
        if (ret < 0) {
            printf("can't send DFU_ABORT, ret = %s\n", libusb_error_name(ret));
            return ret;
        }
		goto status_again;
		break;
	case DFU_STATE_dfuIDLE:
		printf("dfuIDLE, continuing\n");
		break;
	default:
		break;
	}

	if (DFU_STATUS_OK != status.bStatus) {
		printf("WARNING: DFU Status: '%s'\n",
			dfu_status_to_string(status.bStatus));
		/* Clear our status & try again. */
		if (dfu_clear_status(dfu_util->dfu_root->dev_handle, dfu_util->dfu_root->interface) < 0)
			printf("USB communication error");
		if (dfu_get_status(dfu_util->dfu_root, &status) < 0)
			printf("USB communication error");
		if (DFU_STATUS_OK != status.bStatus)
			printf("Status is not OK: %d", status.bStatus);

		milli_sleep(status.bwPollTimeout);
	}

	printf("DFU mode device DFU version %04x\n",
		libusb_le16_to_cpu(dfu_util->dfu_root->func_dfu.bcdDFUVersion));

	if (dfu_util->dfu_root->func_dfu.bcdDFUVersion == libusb_cpu_to_le16(0x11a))
		dfuse_device = 1;

	transfer_size = 4096;
	file.firmware = din;
	file.size.total = ilen;

	//ret = dfuload_do_dnload(dfu_util->dfu_root, transfer_size, &file);
	ret = libdfu_util_download(handle, dfu_util.get(), transfer_size, din, ilen, cb);
	printf("dfuload_do_dnload return: %d", ret);
done:
	
	return ret;
}

extern "C" int close_device(int handle)
{
	auto it = deviceMap.find(handle);

	if (deviceMap.end() == it) {
		return -1;
	}
	auto dfu_util = it->second;
	libusb_close(dfu_util->dfu_root->dev_handle);
	dfu_util->dfu_root->dev_handle = NULL;
	libusb_exit(dfu_util->ctx);
	deviceMap.erase(handle);
	return 0;
}
