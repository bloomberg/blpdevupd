// libdfu_util.h
#ifndef LIBDFU_UTIL_H
#define LIBDFU_UTIL_H

#include "libusb.h"
#include "dfu_util.h"
#include <stdint.h>

typedef void(*libdfu_util_download_cb)(int handle, size_t bytes_sent, size_t bytes_total);

int libdfu_util_download(int handle,
						 dfu_util_t* util, 
						 int block_size, 
						 uint8_t *din, 
						 size_t dlen, 
						 libdfu_util_download_cb cb);

#endif

// ----------------------------------------------------------------------------
// Copyright 2020 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------