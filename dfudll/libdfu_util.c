#include "libdfu_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#include "libusb.h"

#include "portable.h"
#include "dfu.h"
#include "usb_dfu.h"
#include "dfu_file.h"
#include "dfu_load.h"
#include "quirks.h"

int libdfu_util_download(int handle,
						 dfu_util_t* util, 
						 int block_size, 
						 uint8_t *din, 
						 size_t dlen,
						 libdfu_util_download_cb cb)
{
	int bytes_sent;
	int expected_size;
	unsigned char* buf;
	unsigned short transaction = 0;
	struct dfu_status dst;
	int ret;
	struct dfu_if* dif = util->dfu_root;

	printf("Copying data from PC to DFU device\n");

	buf = din;
	expected_size = dlen;
	bytes_sent = 0;

	//dfu_progress_bar("Download", 0, 1);
	if (cb) {
		cb(handle, bytes_sent, expected_size);
	}
	while (bytes_sent < expected_size) {
		int bytes_left;
		int chunk_size;

		bytes_left = expected_size - bytes_sent;
		if (bytes_left < block_size)
			chunk_size = bytes_left;
		else
			chunk_size = block_size;

		ret = dfu_download(dif->dev_handle, dif->interface,
			chunk_size, transaction++, chunk_size ? buf : NULL);
		if (ret < 0) {
			warnx("Error during download");
			goto out;
		}
		bytes_sent += chunk_size;
		buf += chunk_size;

		do {
			ret = dfu_get_status(dif, &dst);
			if (ret < 0) {
				errx(EX_IOERR, "Error during download get_status");
				goto out;
			}

			if (dst.bState == DFU_STATE_dfuDNLOAD_IDLE ||
				dst.bState == DFU_STATE_dfuERROR)
				break;

			/* Wait while device executes flashing */
			milli_sleep(dst.bwPollTimeout);

		} while (1);
		if (dst.bStatus != DFU_STATUS_OK) {
			printf(" failed!\n");
			printf("state(%u) = %s, status(%u) = %s\n", dst.bState,
				dfu_state_to_string(dst.bState), dst.bStatus,
				dfu_status_to_string(dst.bStatus));
			ret = -1;
			goto out;
		}
		//dfu_progress_bar("Download", bytes_sent, bytes_sent + bytes_left);
		if (cb) {
			cb(handle, bytes_sent, expected_size);
		}
	}

	/* send one zero sized download request to signalize end */
	ret = dfu_download(dif->dev_handle, dif->interface,
		0, transaction, NULL);
	if (ret < 0) {
		errx(EX_IOERR, "Error sending completion packet");
		goto out;
	}

	//dfu_progress_bar("Download", bytes_sent, bytes_sent);
	if (cb) {
		cb(handle, bytes_sent, expected_size);
	}

	if (verbose)
		printf("Sent a total of %i bytes\n", bytes_sent);

get_status:
	/* Transition to MANIFEST_SYNC state */
	ret = dfu_get_status(dif, &dst);
	if (ret < 0) {
		warnx("unable to read DFU status after completion");
		goto out;
	}
	printf("state(%u) = %s, status(%u) = %s\n", dst.bState,
		dfu_state_to_string(dst.bState), dst.bStatus,
		dfu_status_to_string(dst.bStatus));

	milli_sleep(dst.bwPollTimeout);

	/* FIXME: deal correctly with ManifestationTolerant=0 / WillDetach bits */
	switch (dst.bState) {
	case DFU_STATE_dfuMANIFEST_SYNC:
	case DFU_STATE_dfuMANIFEST:
		/* some devices (e.g. TAS1020b) need some time before we
		 * can obtain the status */
		milli_sleep(1000);
		goto get_status;
		break;
	case DFU_STATE_dfuIDLE:
		break;
	}
	printf("Done!\n");

out:
	return bytes_sent;
}
