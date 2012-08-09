/*
    0xFFFF - Open Free Fiasco Firmware Flasher
    Copyright (C) 2012  Pali Rohár <pali.rohar@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <usb.h>

#include "device.h"

enum usb_flash_protocol {
	FLASH_UNKN = 0,
	FLASH_NOLO,
	FLASH_COLD,
	FLASH_MKII,
	FLASH_DISK,
};

struct usb_flash_device {
	uint16_t vendor;
	uint16_t product;
	int interface;
	int alternate;
	int configuration;
	enum usb_flash_protocol protocol;
	enum device devices[DEVICE_COUNT];
};

struct usb_device_info {
	usb_dev_handle * dev;
	struct usb_flash_device * info;
};

const char * usb_flash_protocol_to_string(enum usb_flash_protocol protocol);
struct usb_device_info * usb_wait_for_device(void);
void usb_close_device(struct usb_device_info * dev);