#ifndef DFU_UTIL_H
#define DFU_UTIL_H

/* USB string descriptor should contain max 126 UTF-16 characters
 * but 253 would even accomodate any UTF-8 encoding */
#define MAX_DESC_STR_LEN 253

enum mode {
	MODE_NONE,
	MODE_VERSION,
	MODE_LIST,
	MODE_DETACH,
	MODE_UPLOAD,
	MODE_DOWNLOAD
};

typedef struct dfu_util_s {
	struct dfu_if* dfu_root;
	libusb_context* ctx;
	char* match_path;
	int match_vendor;
	int match_product;
	int match_vendor_dfu;
	int match_product_dfu;
	int match_config_index;
	int match_iface_index;
	int match_iface_alt_index;
	const char* match_iface_alt_name;
	const char* match_serial;
	const char* match_serial_dfu;
} dfu_util_t;



void probe_devices(dfu_util_t *);
void disconnect_devices(dfu_util_t*);
void print_dfu_if(struct dfu_if *);

void list_dfu_interfaces(dfu_util_t *);

void dfu_util_init(dfu_util_t* util);

#endif /* DFU_UTIL_H */
