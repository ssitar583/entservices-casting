/**
* Partial unmodified copy of Public libusb header file libusb.h which is:
* Copyright 2001 Johannes Erdfelt <johannes@erdfelt.com>
* Copyright 2007-2008 Daniel Drake <dsd@gentoo.org>
* Copyright 2012 Pete Batard <pete@akeo.ie>
* Copyright 2012-2023 Nathan Hjelm <hjelmn@cs.unm.edu>
* Copyright © 2014-2020 Chris Dickens <christopher.a.dickens@gmail.com>
* For more information, please visit: https://libusb.info
* libusb is licensed under the LGPL License, Version 2.1 or greater
* (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/

#pragma once

#if defined(_MSC_VER)
#pragma warning(push)
/* Disable: warning C4200: nonstandard extension used : zero-sized array in struct/union */
#pragma warning(disable:4200)
/* on MS environments, the inline keyword is available in C++ only */
#if !defined(__cplusplus)
#define inline __inline
#endif
/* ssize_t is also not available */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif /* _SSIZE_T_DEFINED */
#endif /* _MSC_VER */

#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#if !defined(_MSC_VER)
#include <sys/time.h>
#endif
#include <time.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define LIBUSB_FLEXIBLE_ARRAY		/* [] - valid C99 code */
#else
#define LIBUSB_FLEXIBLE_ARRAY	0	/* [0] - non-standard, but usually working code */
#endif /* __STDC_VERSION__ */

/* 'interface' might be defined as a macro on Windows, so we need to
 * undefine it so as not to break the current libusb API, because
 * libusb_config_descriptor has an 'interface' member
 * As this can be problematic if you include windows.h after libusb.h
 * in your sources, we force windows.h to be included first. */
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#if defined(interface)
#undef interface
#endif
#if !defined(__CYGWIN__)
#include <winsock.h>
#endif
#endif /* _WIN32 || __CYGWIN__ */

#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define LIBUSB_DEPRECATED_FOR(f) __attribute__ ((deprecated ("Use " #f " instead")))
#elif defined(__GNUC__) && (__GNUC__ >= 3)
#define LIBUSB_DEPRECATED_FOR(f) __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define LIBUSB_DEPRECATED_FOR(f) __declspec(deprecated("Use " #f " instead"))
#else
#define LIBUSB_DEPRECATED_FOR(f)
#endif /* __GNUC__ */

#if defined(__GNUC__)
#define LIBUSB_PACKED __attribute__ ((packed))
#else
#define LIBUSB_PACKED
#endif /* __GNUC__ */

/** \def LIBUSB_CALL
 * \ingroup libusb_misc
 * libusb's Windows calling convention.
 *
 * Under Windows, the selection of available compilers and configurations
 * means that, unlike other platforms, there is not <em>one true calling
 * convention</em> (calling convention: the manner in which parameters are
 * passed to functions in the generated assembly code).
 *
 * Matching the Windows API itself, libusb uses the WINAPI convention (which
 * translates to the <tt>stdcall</tt> convention) and guarantees that the
 * library is compiled in this way. The public header file also includes
 * appropriate annotations so that your own software will use the right
 * convention, even if another convention is being used by default within
 * your codebase.
 *
 * The one consideration that you must apply in your software is to mark
 * all functions which you use as libusb callbacks with this LIBUSB_CALL
 * annotation, so that they too get compiled for the correct calling
 * convention.
 *
 * On non-Windows operating systems, this macro is defined as nothing. This
 * means that you can apply it to your code without worrying about
 * cross-platform compatibility.
 */
/* LIBUSB_CALL must be defined on both definition and declaration of libusb
 * functions. You'd think that declaration would be enough, but cygwin will
 * complain about conflicting types unless both are marked this way.
 * The placement of this macro is important too; it must appear after the
 * return type, before the function name. See internal documentation for
 * API_EXPORTED.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#define LIBUSB_CALL WINAPI
#define LIBUSB_CALLV WINAPIV
#else
#define LIBUSB_CALL
#define LIBUSB_CALLV
#endif /* _WIN32 || __CYGWIN__ */

/** \def LIBUSB_API_VERSION
 * \ingroup libusb_misc
 * libusb's API version.
 *
 * Since version 1.0.18, to help with feature detection, libusb defines
 * a LIBUSB_API_VERSION macro that gets increased every time there is a
 * significant change to the API, such as the introduction of a new call,
 * the definition of a new macro/enum member, or any other element that
 * libusb applications may want to detect at compilation time.
 *
 * Between versions 1.0.13 and 1.0.17 (inclusive) the older spelling of
 * LIBUSBX_API_VERSION was used.
 *
 * The macro is typically used in an application as follows:
 * \code
 * #if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01001234)
 * // Use one of the newer features from the libusb API
 * #endif
 * \endcode
 *
 * Internally, LIBUSB_API_VERSION is defined as follows:
 * (libusb major << 24) | (libusb minor << 16) | (16 bit incremental)
 *
 * The incremental component has changed as follows:
 * <ul>
 * <li>libusbx version 1.0.13: LIBUSBX_API_VERSION = 0x01000100
 * <li>libusbx version 1.0.14: LIBUSBX_API_VERSION = 0x010000FF
 * <li>libusbx version 1.0.15: LIBUSBX_API_VERSION = 0x01000101
 * <li>libusbx version 1.0.16: LIBUSBX_API_VERSION = 0x01000102
 * <li>libusbx version 1.0.17: LIBUSBX_API_VERSION = 0x01000102
 * <li>libusb version 1.0.18: LIBUSB_API_VERSION = 0x01000102
 * <li>libusb version 1.0.19: LIBUSB_API_VERSION = 0x01000103
 * <li>libusb version 1.0.20: LIBUSB_API_VERSION = 0x01000104
 * <li>libusb version 1.0.21: LIBUSB_API_VERSION = 0x01000105
 * <li>libusb version 1.0.22: LIBUSB_API_VERSION = 0x01000106
 * <li>libusb version 1.0.23: LIBUSB_API_VERSION = 0x01000107
 * <li>libusb version 1.0.24: LIBUSB_API_VERSION = 0x01000108
 * <li>libusb version 1.0.25: LIBUSB_API_VERSION = 0x01000109
 * <li>libusb version 1.0.26: LIBUSB_API_VERSION = 0x01000109
 * <li>libusb version 1.0.27: LIBUSB_API_VERSION = 0x0100010A
 * </ul>
 */
#define LIBUSB_API_VERSION 0x0100010A

/** \def LIBUSBX_API_VERSION
 * \ingroup libusb_misc
 *
 * This is the older spelling, kept for backwards compatibility of code
 * needing to test for older library versions where the newer spelling
 * did not exist. */
#define LIBUSBX_API_VERSION LIBUSB_API_VERSION

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * \ingroup libusb_misc
 * Convert a 16-bit value from host-endian to little-endian format. On
 * little endian systems, this function does nothing. On big endian systems,
 * the bytes are swapped.
 * \param x the host-endian value to convert
 * \returns the value in little-endian byte order
 */
static inline uint16_t libusb_cpu_to_le16(const uint16_t x)
{
	union {
		uint8_t  b8[2];
		uint16_t b16;
	} _tmp;
	_tmp.b8[1] = (uint8_t) (x >> 8);
	_tmp.b8[0] = (uint8_t) (x & 0xff);
	return _tmp.b16;
}

/** \def libusb_le16_to_cpu
 * \ingroup libusb_misc
 * Convert a 16-bit value from little-endian to host-endian format. On
 * little endian systems, this function does nothing. On big endian systems,
 * the bytes are swapped.
 * \param x the little-endian value to convert
 * \returns the value in host-endian byte order
 */
#define libusb_le16_to_cpu libusb_cpu_to_le16

/* standard USB stuff */

/** \ingroup libusb_desc
 * Device and/or Interface Class codes */
enum libusb_class_code {
	/** In the context of a \ref libusb_device_descriptor "device descriptor",
	 * this bDeviceClass value indicates that each interface specifies its
	 * own class information and all interfaces operate independently.
	 */
	LIBUSB_CLASS_PER_INTERFACE = 0x00,

	/** Audio class */
	LIBUSB_CLASS_AUDIO = 0x01,

	/** Communications class */
	LIBUSB_CLASS_COMM = 0x02,

	/** Human Interface Device class */
	LIBUSB_CLASS_HID = 0x03,

	/** Physical */
	LIBUSB_CLASS_PHYSICAL = 0x05,

	/** Image class */
	LIBUSB_CLASS_IMAGE = 0x06,
	LIBUSB_CLASS_PTP = 0x06, /* legacy name from libusb-0.1 usb.h */

	/** Printer class */
	LIBUSB_CLASS_PRINTER = 0x07,

	/** Mass storage class */
	LIBUSB_CLASS_MASS_STORAGE = 0x08,

	/** Hub class */
	LIBUSB_CLASS_HUB = 0x09,

	/** Data class */
	LIBUSB_CLASS_DATA = 0x0a,

	/** Smart Card */
	LIBUSB_CLASS_SMART_CARD = 0x0b,

	/** Content Security */
	LIBUSB_CLASS_CONTENT_SECURITY = 0x0d,

	/** Video */
	LIBUSB_CLASS_VIDEO = 0x0e,

	/** Personal Healthcare */
	LIBUSB_CLASS_PERSONAL_HEALTHCARE = 0x0f,

	/** Diagnostic Device */
	LIBUSB_CLASS_DIAGNOSTIC_DEVICE = 0xdc,

	/** Wireless class */
	LIBUSB_CLASS_WIRELESS = 0xe0,

	/** Miscellaneous class */
	LIBUSB_CLASS_MISCELLANEOUS = 0xef,

	/** Application class */
	LIBUSB_CLASS_APPLICATION = 0xfe,

	/** Class is vendor-specific */
	LIBUSB_CLASS_VENDOR_SPEC = 0xff
};

/** \ingroup libusb_desc
 * Descriptor types as defined by the USB specification. */
enum libusb_descriptor_type {
	/** Device descriptor. See libusb_device_descriptor. */
	LIBUSB_DT_DEVICE = 0x01,

	/** Configuration descriptor. See libusb_config_descriptor. */
	LIBUSB_DT_CONFIG = 0x02,

	/** String descriptor */
	LIBUSB_DT_STRING = 0x03,

	/** Interface descriptor. See libusb_interface_descriptor. */
	LIBUSB_DT_INTERFACE = 0x04,

	/** Endpoint descriptor. See libusb_endpoint_descriptor. */
	LIBUSB_DT_ENDPOINT = 0x05,

	/** Interface Association Descriptor.
	* See libusb_interface_association_descriptor */
	LIBUSB_DT_INTERFACE_ASSOCIATION = 0x0b,

	/** BOS descriptor */
	LIBUSB_DT_BOS = 0x0f,

	/** Device Capability descriptor */
	LIBUSB_DT_DEVICE_CAPABILITY = 0x10,

	/** HID descriptor */
	LIBUSB_DT_HID = 0x21,

	/** HID report descriptor */
	LIBUSB_DT_REPORT = 0x22,

	/** Physical descriptor */
	LIBUSB_DT_PHYSICAL = 0x23,

	/** Hub descriptor */
	LIBUSB_DT_HUB = 0x29,

	/** SuperSpeed Hub descriptor */
	LIBUSB_DT_SUPERSPEED_HUB = 0x2a,

	/** SuperSpeed Endpoint Companion descriptor */
	LIBUSB_DT_SS_ENDPOINT_COMPANION = 0x30
};

/* Descriptor sizes per descriptor type */
#define LIBUSB_DT_DEVICE_SIZE			18
#define LIBUSB_DT_CONFIG_SIZE			9
#define LIBUSB_DT_INTERFACE_SIZE		9
#define LIBUSB_DT_ENDPOINT_SIZE			7
#define LIBUSB_DT_ENDPOINT_AUDIO_SIZE		9	/* Audio extension */
#define LIBUSB_DT_HUB_NONVAR_SIZE		7
#define LIBUSB_DT_SS_ENDPOINT_COMPANION_SIZE	6
#define LIBUSB_DT_BOS_SIZE			5
#define LIBUSB_DT_DEVICE_CAPABILITY_SIZE	3

/* BOS descriptor sizes */
#define LIBUSB_BT_USB_2_0_EXTENSION_SIZE	7
#define LIBUSB_BT_SS_USB_DEVICE_CAPABILITY_SIZE	10
#define LIBUSB_BT_CONTAINER_ID_SIZE		20
#define LIBUSB_BT_PLATFORM_DESCRIPTOR_MIN_SIZE		20

/* We unwrap the BOS => define its max size */
#define LIBUSB_DT_BOS_MAX_SIZE				\
	(LIBUSB_DT_BOS_SIZE +				\
	 LIBUSB_BT_USB_2_0_EXTENSION_SIZE +		\
	 LIBUSB_BT_SS_USB_DEVICE_CAPABILITY_SIZE +	\
	 LIBUSB_BT_CONTAINER_ID_SIZE)

#define LIBUSB_ENDPOINT_ADDRESS_MASK		0x0f	/* in bEndpointAddress */
#define LIBUSB_ENDPOINT_DIR_MASK		0x80

/** \ingroup libusb_desc
 * Endpoint direction. Values for bit 7 of the
 * \ref libusb_endpoint_descriptor::bEndpointAddress "endpoint address" scheme.
 */
enum libusb_endpoint_direction {
	/** Out: host-to-device */
	LIBUSB_ENDPOINT_OUT = 0x00,

	/** In: device-to-host */
	LIBUSB_ENDPOINT_IN = 0x80
};

#define LIBUSB_TRANSFER_TYPE_MASK		0x03	/* in bmAttributes */

/** \ingroup libusb_desc
 * Endpoint transfer type. Values for bits 0:1 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "endpoint attributes" field.
 */
enum libusb_endpoint_transfer_type {
	/** Control endpoint */
	LIBUSB_ENDPOINT_TRANSFER_TYPE_CONTROL = 0x0,

	/** Isochronous endpoint */
	LIBUSB_ENDPOINT_TRANSFER_TYPE_ISOCHRONOUS = 0x1,

	/** Bulk endpoint */
	LIBUSB_ENDPOINT_TRANSFER_TYPE_BULK = 0x2,

	/** Interrupt endpoint */
	LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT = 0x3
};

/** \ingroup libusb_misc
 * Standard requests, as defined in table 9-5 of the USB 3.0 specifications */
enum libusb_standard_request {
	/** Request status of the specific recipient */
	LIBUSB_REQUEST_GET_STATUS = 0x00,

	/** Clear or disable a specific feature */
	LIBUSB_REQUEST_CLEAR_FEATURE = 0x01,

	/* 0x02 is reserved */

	/** Set or enable a specific feature */
	LIBUSB_REQUEST_SET_FEATURE = 0x03,

	/* 0x04 is reserved */

	/** Set device address for all future accesses */
	LIBUSB_REQUEST_SET_ADDRESS = 0x05,

	/** Get the specified descriptor */
	LIBUSB_REQUEST_GET_DESCRIPTOR = 0x06,

	/** Used to update existing descriptors or add new descriptors */
	LIBUSB_REQUEST_SET_DESCRIPTOR = 0x07,

	/** Get the current device configuration value */
	LIBUSB_REQUEST_GET_CONFIGURATION = 0x08,

	/** Set device configuration */
	LIBUSB_REQUEST_SET_CONFIGURATION = 0x09,

	/** Return the selected alternate setting for the specified interface */
	LIBUSB_REQUEST_GET_INTERFACE = 0x0a,

	/** Select an alternate interface for the specified interface */
	LIBUSB_REQUEST_SET_INTERFACE = 0x0b,

	/** Set then report an endpoint's synchronization frame */
	LIBUSB_REQUEST_SYNCH_FRAME = 0x0c,

	/** Sets both the U1 and U2 Exit Latency */
	LIBUSB_REQUEST_SET_SEL = 0x30,

	/** Delay from the time a host transmits a packet to the time it is
	  * received by the device. */
	LIBUSB_SET_ISOCH_DELAY = 0x31
};

/** \ingroup libusb_misc
 * Request type bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. */
enum libusb_request_type {
	/** Standard */
	LIBUSB_REQUEST_TYPE_STANDARD = (0x00 << 5),

	/** Class */
	LIBUSB_REQUEST_TYPE_CLASS = (0x01 << 5),

	/** Vendor */
	LIBUSB_REQUEST_TYPE_VENDOR = (0x02 << 5),

	/** Reserved */
	LIBUSB_REQUEST_TYPE_RESERVED = (0x03 << 5)
};

/** \ingroup libusb_misc
 * Recipient bits of the
 * \ref libusb_control_setup::bmRequestType "bmRequestType" field in control
 * transfers. Values 4 through 31 are reserved. */
enum libusb_request_recipient {
	/** Device */
	LIBUSB_RECIPIENT_DEVICE = 0x00,

	/** Interface */
	LIBUSB_RECIPIENT_INTERFACE = 0x01,

	/** Endpoint */
	LIBUSB_RECIPIENT_ENDPOINT = 0x02,

	/** Other */
	LIBUSB_RECIPIENT_OTHER = 0x03
};

#define LIBUSB_ISO_SYNC_TYPE_MASK	0x0c

/** \ingroup libusb_desc
 * Synchronization type for isochronous endpoints. Values for bits 2:3 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "bmAttributes" field in
 * libusb_endpoint_descriptor.
 */
enum libusb_iso_sync_type {
	/** No synchronization */
	LIBUSB_ISO_SYNC_TYPE_NONE = 0x0,

	/** Asynchronous */
	LIBUSB_ISO_SYNC_TYPE_ASYNC = 0x1,

	/** Adaptive */
	LIBUSB_ISO_SYNC_TYPE_ADAPTIVE = 0x2,

	/** Synchronous */
	LIBUSB_ISO_SYNC_TYPE_SYNC = 0x3
};

#define LIBUSB_ISO_USAGE_TYPE_MASK	0x30

/** \ingroup libusb_desc
 * Usage type for isochronous endpoints. Values for bits 4:5 of the
 * \ref libusb_endpoint_descriptor::bmAttributes "bmAttributes" field in
 * libusb_endpoint_descriptor.
 */
enum libusb_iso_usage_type {
	/** Data endpoint */
	LIBUSB_ISO_USAGE_TYPE_DATA = 0x0,

	/** Feedback endpoint */
	LIBUSB_ISO_USAGE_TYPE_FEEDBACK = 0x1,

	/** Implicit feedback Data endpoint */
	LIBUSB_ISO_USAGE_TYPE_IMPLICIT = 0x2
};

/** \ingroup libusb_desc
 * Supported speeds (wSpeedSupported) bitfield. Indicates what
 * speeds the device supports.
 */
enum libusb_supported_speed {
	/** Low speed operation supported (1.5MBit/s). */
	LIBUSB_LOW_SPEED_OPERATION = (1 << 0),

	/** Full speed operation supported (12MBit/s). */
	LIBUSB_FULL_SPEED_OPERATION = (1 << 1),

	/** High speed operation supported (480MBit/s). */
	LIBUSB_HIGH_SPEED_OPERATION = (1 << 2),

	/** Superspeed operation supported (5000MBit/s). */
	LIBUSB_SUPER_SPEED_OPERATION = (1 << 3)
};

/** \ingroup libusb_desc
 * Masks for the bits of the
 * \ref libusb_usb_2_0_extension_descriptor::bmAttributes "bmAttributes" field
 * of the USB 2.0 Extension descriptor.
 */
enum libusb_usb_2_0_extension_attributes {
	/** Supports Link Power Management (LPM) */
	LIBUSB_BM_LPM_SUPPORT = (1 << 1)
};

/** \ingroup libusb_desc
 * Masks for the bits of the
 * \ref libusb_ss_usb_device_capability_descriptor::bmAttributes "bmAttributes" field
 * field of the SuperSpeed USB Device Capability descriptor.
 */
enum libusb_ss_usb_device_capability_attributes {
	/** Supports Latency Tolerance Messages (LTM) */
	LIBUSB_BM_LTM_SUPPORT = (1 << 1)
};

/** \ingroup libusb_desc
 * USB capability types
 */
enum libusb_bos_type {
	/** Wireless USB device capability */
	LIBUSB_BT_WIRELESS_USB_DEVICE_CAPABILITY = 0x01,

	/** USB 2.0 extensions */
	LIBUSB_BT_USB_2_0_EXTENSION = 0x02,

	/** SuperSpeed USB device capability */
	LIBUSB_BT_SS_USB_DEVICE_CAPABILITY = 0x03,

	/** Container ID type */
	LIBUSB_BT_CONTAINER_ID = 0x04,

	/** Platform descriptor */
	LIBUSB_BT_PLATFORM_DESCRIPTOR = 0x05
};

/** \ingroup libusb_desc
 * A structure representing the standard USB device descriptor. This
 * descriptor is documented in section 9.6.1 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_device_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE LIBUSB_DT_DEVICE in this
	 * context. */
	uint8_t  bDescriptorType;

	/** USB specification release number in binary-coded decimal. A value of
	 * 0x0200 indicates USB 2.0, 0x0110 indicates USB 1.1, etc. */
	uint16_t bcdUSB;

	/** USB-IF class code for the device. See \ref libusb_class_code. */
	uint8_t  bDeviceClass;

	/** USB-IF subclass code for the device, qualified by the bDeviceClass
	 * value */
	uint8_t  bDeviceSubClass;

	/** USB-IF protocol code for the device, qualified by the bDeviceClass and
	 * bDeviceSubClass values */
	uint8_t  bDeviceProtocol;

	/** Maximum packet size for endpoint 0 */
	uint8_t  bMaxPacketSize0;

	/** USB-IF vendor ID */
	uint16_t idVendor;

	/** USB-IF product ID */
	uint16_t idProduct;

	/** Device release number in binary-coded decimal */
	uint16_t bcdDevice;

	/** Index of string descriptor describing manufacturer */
	uint8_t  iManufacturer;

	/** Index of string descriptor describing product */
	uint8_t  iProduct;

	/** Index of string descriptor containing device serial number */
	uint8_t  iSerialNumber;

	/** Number of possible configurations */
	uint8_t  bNumConfigurations;
};

/** \ingroup libusb_desc
 * A structure representing the standard USB endpoint descriptor. This
 * descriptor is documented in section 9.6.6 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_endpoint_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_ENDPOINT LIBUSB_DT_ENDPOINT in
	 * this context. */
	uint8_t  bDescriptorType;

	/** The address of the endpoint described by this descriptor. Bits 0:3 are
	 * the endpoint number. Bits 4:6 are reserved. Bit 7 indicates direction,
	 * see \ref libusb_endpoint_direction. */
	uint8_t  bEndpointAddress;

	/** Attributes which apply to the endpoint when it is configured using
	 * the bConfigurationValue. Bits 0:1 determine the transfer type and
	 * correspond to \ref libusb_endpoint_transfer_type. Bits 2:3 are only used
	 * for isochronous endpoints and correspond to \ref libusb_iso_sync_type.
	 * Bits 4:5 are also only used for isochronous endpoints and correspond to
	 * \ref libusb_iso_usage_type. Bits 6:7 are reserved. */
	uint8_t  bmAttributes;

	/** Maximum packet size this endpoint is capable of sending/receiving. */
	uint16_t wMaxPacketSize;

	/** Interval for polling endpoint for data transfers. */
	uint8_t  bInterval;

	/** For audio devices only: the rate at which synchronization feedback
	 * is provided. */
	uint8_t  bRefresh;

	/** For audio devices only: the address if the synch endpoint */
	uint8_t  bSynchAddress;

	/** Extra descriptors. If libusb encounters unknown endpoint descriptors,
	 * it will store them here, should you wish to parse them. */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes. Must be non-negative. */
	int extra_length;
};

/** \ingroup libusb_desc
 * A structure representing the standard USB interface association descriptor.
 * This descriptor is documented in section 9.6.4 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_interface_association_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	* \ref libusb_descriptor_type::LIBUSB_DT_INTERFACE_ASSOCIATION
	* LIBUSB_DT_INTERFACE_ASSOCIATION in this context. */
	uint8_t  bDescriptorType;

	/** Interface number of the first interface that is associated
	* with this function */
	uint8_t  bFirstInterface;

	/** Number of contiguous interfaces that are associated with
	* this function */
	uint8_t  bInterfaceCount;

	/** USB-IF class code for this function.
	* A value of zero is not allowed in this descriptor.
	* If this field is 0xff, the function class is vendor-specific.
	* All other values are reserved for assignment by the USB-IF.
	*/
	uint8_t  bFunctionClass;

	/** USB-IF subclass code for this function.
	* If this field is not set to 0xff, all values are reserved
	* for assignment by the USB-IF
	*/
	uint8_t  bFunctionSubClass;

	/** USB-IF protocol code for this function.
	* These codes are qualified by the values of the bFunctionClass
	* and bFunctionSubClass fields.
	*/
	uint8_t  bFunctionProtocol;

	/** Index of string descriptor describing this function */
	uint8_t  iFunction;
};

/** \ingroup libusb_desc
 * Structure containing an array of 0 or more interface association
 * descriptors
 */
struct libusb_interface_association_descriptor_array {
	/** Array of interface association descriptors. The size of this array
	 * is determined by the length field.
	 */
	const struct libusb_interface_association_descriptor *iad;

	/** Number of interface association descriptors contained. Read-only. */
	int length;
};

/** \ingroup libusb_desc
 * A structure representing the standard USB interface descriptor. This
 * descriptor is documented in section 9.6.5 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_interface_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_INTERFACE LIBUSB_DT_INTERFACE
	 * in this context. */
	uint8_t  bDescriptorType;

	/** Number of this interface */
	uint8_t  bInterfaceNumber;

	/** Value used to select this alternate setting for this interface */
	uint8_t  bAlternateSetting;

	/** Number of endpoints used by this interface (excluding the control
	 * endpoint). */
	uint8_t  bNumEndpoints;

	/** USB-IF class code for this interface. See \ref libusb_class_code. */
	uint8_t  bInterfaceClass;

	/** USB-IF subclass code for this interface, qualified by the
	 * bInterfaceClass value */
	uint8_t  bInterfaceSubClass;

	/** USB-IF protocol code for this interface, qualified by the
	 * bInterfaceClass and bInterfaceSubClass values */
	uint8_t  bInterfaceProtocol;

	/** Index of string descriptor describing this interface */
	uint8_t  iInterface;

	/** Array of endpoint descriptors. This length of this array is determined
	 * by the bNumEndpoints field. */
	const struct libusb_endpoint_descriptor *endpoint;

	/** Extra descriptors. If libusb encounters unknown interface descriptors,
	 * it will store them here, should you wish to parse them. */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes. Must be non-negative. */
	int extra_length;
};

/** \ingroup libusb_desc
 * A collection of alternate settings for a particular USB interface.
 */
struct libusb_interface {
	/** Array of interface descriptors. The length of this array is determined
	 * by the num_altsetting field. */
	const struct libusb_interface_descriptor *altsetting;

	/** The number of alternate settings that belong to this interface.
	 * Must be non-negative. */
	int num_altsetting;
};

/** \ingroup libusb_desc
 * A structure representing the standard USB configuration descriptor. This
 * descriptor is documented in section 9.6.3 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_config_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_CONFIG LIBUSB_DT_CONFIG
	 * in this context. */
	uint8_t  bDescriptorType;

	/** Total length of data returned for this configuration */
	uint16_t wTotalLength;

	/** Number of interfaces supported by this configuration */
	uint8_t  bNumInterfaces;

	/** Identifier value for this configuration */
	uint8_t  bConfigurationValue;

	/** Index of string descriptor describing this configuration */
	uint8_t  iConfiguration;

	/** Configuration characteristics */
	uint8_t  bmAttributes;

	/** Maximum power consumption of the USB device from this bus in this
	 * configuration when the device is fully operation. Expressed in units
	 * of 2 mA when the device is operating in high-speed mode and in units
	 * of 8 mA when the device is operating in super-speed mode. */
	uint8_t  MaxPower;

	/** Array of interfaces supported by this configuration. The length of
	 * this array is determined by the bNumInterfaces field. */
	const struct libusb_interface *interface;

	/** Extra descriptors. If libusb encounters unknown configuration
	 * descriptors, it will store them here, should you wish to parse them. */
	const unsigned char *extra;

	/** Length of the extra descriptors, in bytes. Must be non-negative. */
	int extra_length;
};

/** \ingroup libusb_desc
 * A structure representing the superspeed endpoint companion
 * descriptor. This descriptor is documented in section 9.6.7 of
 * the USB 3.0 specification. All multiple-byte fields are represented in
 * host-endian format.
 */
struct libusb_ss_endpoint_companion_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_SS_ENDPOINT_COMPANION in
	 * this context. */
	uint8_t  bDescriptorType;

	/** The maximum number of packets the endpoint can send or
	 *  receive as part of a burst. */
	uint8_t  bMaxBurst;

	/** In bulk EP: bits 4:0 represents the maximum number of
	 *  streams the EP supports. In isochronous EP: bits 1:0
	 *  represents the Mult - a zero based value that determines
	 *  the maximum number of packets within a service interval  */
	uint8_t  bmAttributes;

	/** The total number of bytes this EP will transfer every
	 *  service interval. Valid only for periodic EPs. */
	uint16_t wBytesPerInterval;
};

/** \ingroup libusb_desc
 * A generic representation of a BOS Device Capability descriptor. It is
 * advised to check bDevCapabilityType and call the matching
 * libusb_get_*_descriptor function to get a structure fully matching the type.
 */
struct libusb_bos_dev_capability_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context. */
	uint8_t  bDescriptorType;

	/** Device Capability type */
	uint8_t  bDevCapabilityType;

	/** Device Capability data (bLength - 3 bytes) */
	uint8_t  dev_capability_data[LIBUSB_FLEXIBLE_ARRAY];
};

/** \ingroup libusb_desc
 * A structure representing the Binary Device Object Store (BOS) descriptor.
 * This descriptor is documented in section 9.6.2 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_bos_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_BOS LIBUSB_DT_BOS
	 * in this context. */
	uint8_t  bDescriptorType;

	/** Length of this descriptor and all of its sub descriptors */
	uint16_t wTotalLength;

	/** The number of separate device capability descriptors in
	 * the BOS */
	uint8_t  bNumDeviceCaps;

	/** bNumDeviceCap Device Capability Descriptors */
	struct libusb_bos_dev_capability_descriptor *dev_capability[LIBUSB_FLEXIBLE_ARRAY];
};

/** \ingroup libusb_desc
 * A structure representing the USB 2.0 Extension descriptor
 * This descriptor is documented in section 9.6.2.1 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_usb_2_0_extension_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context. */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_BT_USB_2_0_EXTENSION
	 * LIBUSB_BT_USB_2_0_EXTENSION in this context. */
	uint8_t  bDevCapabilityType;

	/** Bitmap encoding of supported device level features.
	 * A value of one in a bit location indicates a feature is
	 * supported; a value of zero indicates it is not supported.
	 * See \ref libusb_usb_2_0_extension_attributes. */
	uint32_t bmAttributes;
};

/** \ingroup libusb_desc
 * A structure representing the SuperSpeed USB Device Capability descriptor
 * This descriptor is documented in section 9.6.2.2 of the USB 3.0 specification.
 * All multiple-byte fields are represented in host-endian format.
 */
struct libusb_ss_usb_device_capability_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context. */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_BT_SS_USB_DEVICE_CAPABILITY
	 * LIBUSB_BT_SS_USB_DEVICE_CAPABILITY in this context. */
	uint8_t  bDevCapabilityType;

	/** Bitmap encoding of supported device level features.
	 * A value of one in a bit location indicates a feature is
	 * supported; a value of zero indicates it is not supported.
	 * See \ref libusb_ss_usb_device_capability_attributes. */
	uint8_t  bmAttributes;

	/** Bitmap encoding of the speed supported by this device when
	 * operating in SuperSpeed mode. See \ref libusb_supported_speed. */
	uint16_t wSpeedSupported;

	/** The lowest speed at which all the functionality supported
	 * by the device is available to the user. For example if the
	 * device supports all its functionality when connected at
	 * full speed and above then it sets this value to 1. */
	uint8_t  bFunctionalitySupport;

	/** U1 Device Exit Latency. */
	uint8_t  bU1DevExitLat;

	/** U2 Device Exit Latency. */
	uint16_t bU2DevExitLat;
};

/** \ingroup libusb_desc
 * A structure representing the Container ID descriptor.
 * This descriptor is documented in section 9.6.2.3 of the USB 3.0 specification.
 * All multiple-byte fields, except UUIDs, are represented in host-endian format.
 */
struct libusb_container_id_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context. */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_BT_CONTAINER_ID
	 * LIBUSB_BT_CONTAINER_ID in this context. */
	uint8_t  bDevCapabilityType;

	/** Reserved field */
	uint8_t  bReserved;

	/** 128 bit UUID */
	uint8_t  ContainerID[16];
};

/** \ingroup libusb_desc
 * A structure representing a Platform descriptor.
 * This descriptor is documented in section 9.6.2.4 of the USB 3.2 specification.
 */
struct libusb_platform_descriptor {
	/** Size of this descriptor (in bytes) */
	uint8_t  bLength;

	/** Descriptor type. Will have value
	 * \ref libusb_descriptor_type::LIBUSB_DT_DEVICE_CAPABILITY
	 * LIBUSB_DT_DEVICE_CAPABILITY in this context. */
	uint8_t  bDescriptorType;

	/** Capability type. Will have value
	 * \ref libusb_capability_type::LIBUSB_BT_PLATFORM_DESCRIPTOR
	 * LIBUSB_BT_CONTAINER_ID in this context. */
	uint8_t  bDevCapabilityType;

	/** Reserved field */
	uint8_t  bReserved;

	/** 128 bit UUID */
	uint8_t  PlatformCapabilityUUID[16];

	/** Capability data (bLength - 20) */
	uint8_t  CapabilityData[LIBUSB_FLEXIBLE_ARRAY];
};

/** \ingroup libusb_asyncio
 * Setup packet for control transfers. */
#if defined(_MSC_VER) || defined(__WATCOMC__)
#pragma pack(push, 1)
#endif
struct libusb_control_setup {
	/** Request type. Bits 0:4 determine recipient, see
	 * \ref libusb_request_recipient. Bits 5:6 determine type, see
	 * \ref libusb_request_type. Bit 7 determines data transfer direction, see
	 * \ref libusb_endpoint_direction.
	 */
	uint8_t  bmRequestType;

	/** Request. If the type bits of bmRequestType are equal to
	 * \ref libusb_request_type::LIBUSB_REQUEST_TYPE_STANDARD
	 * "LIBUSB_REQUEST_TYPE_STANDARD" then this field refers to
	 * \ref libusb_standard_request. For other cases, use of this field is
	 * application-specific. */
	uint8_t  bRequest;

	/** Value. Varies according to request */
	uint16_t wValue;

	/** Index. Varies according to request, typically used to pass an index
	 * or offset */
	uint16_t wIndex;

	/** Number of bytes to transfer */
	uint16_t wLength;
} LIBUSB_PACKED;
#if defined(_MSC_VER) || defined(__WATCOMC__)
#pragma pack(pop)
#endif

#define LIBUSB_CONTROL_SETUP_SIZE (sizeof(struct libusb_control_setup))

/* libusb */

struct libusb_context;
struct libusb_device;

/** \ingroup libusb_lib
 * Structure providing the version of the libusb runtime
 */
struct libusb_version {
	/** Library major version. */
	const uint16_t major;

	/** Library minor version. */
	const uint16_t minor;

	/** Library micro version. */
	const uint16_t micro;

	/** Library nano version. */
	const uint16_t nano;

	/** Library release candidate suffix string, e.g. "-rc4". */
	const char *rc;

	/** For ABI compatibility only. */
	const char *describe;
};

/** \ingroup libusb_lib
 * Structure representing a libusb session. The concept of individual libusb
 * sessions allows for your program to use two libraries (or dynamically
 * load two modules) which both independently use libusb. This will prevent
 * interference between the individual libusb users - for example
 * libusb_set_option() will not affect the other user of the library, and
 * libusb_exit() will not destroy resources that the other user is still
 * using.
 *
 * Sessions are created by libusb_init_context() and destroyed through libusb_exit().
 * If your application is guaranteed to only ever include a single libusb
 * user (i.e. you), you do not have to worry about contexts: pass NULL in
 * every function call where a context is required, and the default context
 * will be used. Note that libusb_set_option(NULL, ...) is special, and adds
 * an option to a list of default options for new contexts.
 *
 * For more information, see \ref libusb_contexts.
 */
typedef struct libusb_context libusb_context;

/** \ingroup libusb_dev
 * Structure representing a USB device detected on the system. This is an
 * opaque type for which you are only ever provided with a pointer, usually
 * originating from libusb_get_device_list() or libusb_hotplug_register_callback().
 *
 * Certain operations can be performed on a device, but in order to do any
 * I/O you will have to first obtain a device handle using libusb_open().
 *
 * Devices are reference counted with libusb_ref_device() and
 * libusb_unref_device(), and are freed when the reference count reaches 0.
 * New devices presented by libusb_get_device_list() have a reference count of
 * 1, and libusb_free_device_list() can optionally decrease the reference count
 * on all devices in the list. libusb_open() adds another reference which is
 * later destroyed by libusb_close().
 */
typedef struct libusb_device libusb_device;


/** \ingroup libusb_dev
 * Structure representing a handle on a USB device. This is an opaque type for
 * which you are only ever provided with a pointer, usually originating from
 * libusb_open().
 *
 * A device handle is used to perform I/O and other operations. When finished
 * with a device handle, you should call libusb_close().
 */

/** \ingroup libusb_dev
 * Speed codes. Indicates the speed at which the device is operating.
 */
enum libusb_speed {
	/** The OS doesn't report or know the device speed. */
	LIBUSB_SPEED_UNKNOWN = 0,

	/** The device is operating at low speed (1.5MBit/s). */
	LIBUSB_SPEED_LOW = 1,

	/** The device is operating at full speed (12MBit/s). */
	LIBUSB_SPEED_FULL = 2,

	/** The device is operating at high speed (480MBit/s). */
	LIBUSB_SPEED_HIGH = 3,

	/** The device is operating at super speed (5000MBit/s). */
	LIBUSB_SPEED_SUPER = 4,

	/** The device is operating at super speed plus (10000MBit/s). */
	LIBUSB_SPEED_SUPER_PLUS = 5
};

/** \ingroup libusb_misc
 * Error codes. Most libusb functions return 0 on success or one of these
 * codes on failure.
 * You can call libusb_error_name() to retrieve a string representation of an
 * error code or libusb_strerror() to get an end-user suitable description of
 * an error code.
 */
enum libusb_error {
	/** Success (no error) */
	LIBUSB_SUCCESS = 0,

	/** Input/output error */
	LIBUSB_ERROR_IO = -1,

	/** Invalid parameter */
	LIBUSB_ERROR_INVALID_PARAM = -2,

	/** Access denied (insufficient permissions) */
	LIBUSB_ERROR_ACCESS = -3,

	/** No such device (it may have been disconnected) */
	LIBUSB_ERROR_NO_DEVICE = -4,

	/** Entity not found */
	LIBUSB_ERROR_NOT_FOUND = -5,

	/** Resource busy */
	LIBUSB_ERROR_BUSY = -6,

	/** Operation timed out */
	LIBUSB_ERROR_TIMEOUT = -7,

	/** Overflow */
	LIBUSB_ERROR_OVERFLOW = -8,

	/** Pipe error */
	LIBUSB_ERROR_PIPE = -9,

	/** System call interrupted (perhaps due to signal) */
	LIBUSB_ERROR_INTERRUPTED = -10,

	/** Insufficient memory */
	LIBUSB_ERROR_NO_MEM = -11,

	/** Operation not supported or unimplemented on this platform */
	LIBUSB_ERROR_NOT_SUPPORTED = -12,

	/* NB: Remember to update LIBUSB_ERROR_COUNT below as well as the
	   message strings in strerror.c when adding new error codes here. */

	/** Other error */
	LIBUSB_ERROR_OTHER = -99
};

/* Total number of error codes in enum libusb_error */
#define LIBUSB_ERROR_COUNT 14

/** \ingroup libusb_asyncio
 * Transfer type */
enum libusb_transfer_type {
	/** Control transfer */
	LIBUSB_TRANSFER_TYPE_CONTROL = 0U,

	/** Isochronous transfer */
	LIBUSB_TRANSFER_TYPE_ISOCHRONOUS = 1U,

	/** Bulk transfer */
	LIBUSB_TRANSFER_TYPE_BULK = 2U,

	/** Interrupt transfer */
	LIBUSB_TRANSFER_TYPE_INTERRUPT = 3U,

	/** Bulk stream transfer */
	LIBUSB_TRANSFER_TYPE_BULK_STREAM = 4U
};

/** \ingroup libusb_asyncio
 * Transfer status codes */
enum libusb_transfer_status {
	/** Transfer completed without error. Note that this does not indicate
	 * that the entire amount of requested data was transferred. */
	LIBUSB_TRANSFER_COMPLETED,

	/** Transfer failed */
	LIBUSB_TRANSFER_ERROR,

	/** Transfer timed out */
	LIBUSB_TRANSFER_TIMED_OUT,

	/** Transfer was cancelled */
	LIBUSB_TRANSFER_CANCELLED,

	/** For bulk/interrupt endpoints: halt condition detected (endpoint
	 * stalled). For control endpoints: control request not supported. */
	LIBUSB_TRANSFER_STALL,

	/** Device was disconnected */
	LIBUSB_TRANSFER_NO_DEVICE,

	/** Device sent more data than requested */
	LIBUSB_TRANSFER_OVERFLOW

	/* NB! Remember to update libusb_error_name()
	   when adding new status codes here. */
};

/** \ingroup libusb_asyncio
 * libusb_transfer.flags values */
enum libusb_transfer_flags {
	/** Report short frames as errors */
	LIBUSB_TRANSFER_SHORT_NOT_OK = (1U << 0),

	/** Automatically free() transfer buffer during libusb_free_transfer().
	 * Note that buffers allocated with libusb_dev_mem_alloc() should not
	 * be attempted freed in this way, since free() is not an appropriate
	 * way to release such memory. */
	LIBUSB_TRANSFER_FREE_BUFFER = (1U << 1),

	/** Automatically call libusb_free_transfer() after callback returns.
	 * If this flag is set, it is illegal to call libusb_free_transfer()
	 * from your transfer callback, as this will result in a double-free
	 * when this flag is acted upon. */
	LIBUSB_TRANSFER_FREE_TRANSFER = (1U << 2),

	/** Terminate transfers that are a multiple of the endpoint's
	 * wMaxPacketSize with an extra zero length packet. This is useful
	 * when a device protocol mandates that each logical request is
	 * terminated by an incomplete packet (i.e. the logical requests are
	 * not separated by other means).
	 *
	 * This flag only affects host-to-device transfers to bulk and interrupt
	 * endpoints. In other situations, it is ignored.
	 *
	 * This flag only affects transfers with a length that is a multiple of
	 * the endpoint's wMaxPacketSize. On transfers of other lengths, this
	 * flag has no effect. Therefore, if you are working with a device that
	 * needs a ZLP whenever the end of the logical request falls on a packet
	 * boundary, then it is sensible to set this flag on <em>every</em>
	 * transfer (you do not have to worry about only setting it on transfers
	 * that end on the boundary).
	 *
	 * This flag is currently only supported on Linux.
	 * On other systems, libusb_submit_transfer() will return
	 * \ref LIBUSB_ERROR_NOT_SUPPORTED for every transfer where this
	 * flag is set.
	 *
	 * Available since libusb-1.0.9.
	 */
	LIBUSB_TRANSFER_ADD_ZERO_PACKET = (1U << 3)
};

/** \ingroup libusb_asyncio
 * Isochronous packet descriptor. */
struct libusb_iso_packet_descriptor {
	/** Length of data to request in this packet */
	unsigned int length;

	/** Amount of data that was actually transferred */
	unsigned int actual_length;

	/** Status code for this packet */
	enum libusb_transfer_status status;
};

struct libusb_transfer;

/** \ingroup libusb_asyncio
 * Asynchronous transfer callback function type. When submitting asynchronous
 * transfers, you pass a pointer to a callback function of this type via the
 * \ref libusb_transfer::callback "callback" member of the libusb_transfer
 * structure. libusb will call this function later, when the transfer has
 * completed or failed. See \ref libusb_asyncio for more information.
 * \param transfer The libusb_transfer struct the callback function is being
 * notified about.
 */
typedef void (LIBUSB_CALL *libusb_transfer_cb_fn)(struct libusb_transfer *transfer);

/** \ingroup libusb_asyncio
 * The generic USB transfer structure. The user populates this structure and
 * then submits it in order to request a transfer. After the transfer has
 * completed, the library populates the transfer with the results and passes
 * it back to the user.
 */
struct libusb_transfer {

	/** A bitwise OR combination of \ref libusb_transfer_flags. */
	uint8_t flags;

	/** Address of the endpoint where this transfer will be sent. */
	unsigned char endpoint;

	/** Type of the transfer from \ref libusb_transfer_type */
	unsigned char type;

	/** Timeout for this transfer in milliseconds. A value of 0 indicates no
	 * timeout. */
	unsigned int timeout;

	/** The status of the transfer. Read-only, and only for use within
	 * transfer callback function.
	 *
	 * If this is an isochronous transfer, this field may read COMPLETED even
	 * if there were errors in the frames. Use the
	 * \ref libusb_iso_packet_descriptor::status "status" field in each packet
	 * to determine if errors occurred. */
	enum libusb_transfer_status status;

	/** Length of the data buffer. Must be non-negative. */
	int length;

	/** Actual length of data that was transferred. Read-only, and only for
	 * use within transfer callback function. Not valid for isochronous
	 * endpoint transfers. */
	int actual_length;

	/** Callback function. This will be invoked when the transfer completes,
	 * fails, or is cancelled. */
	libusb_transfer_cb_fn callback;

	/** User context data. Useful for associating specific data to a transfer
	 * that can be accessed from within the callback function.
	 *
	 * This field may be set manually or is taken as the `user_data` parameter
	 * of the following functions:
	 * - libusb_fill_bulk_transfer()
	 * - libusb_fill_bulk_stream_transfer()
	 * - libusb_fill_control_transfer()
	 * - libusb_fill_interrupt_transfer()
	 * - libusb_fill_iso_transfer() */
	void *user_data;

	/** Data buffer */
	unsigned char *buffer;

	/** Number of isochronous packets. Only used for I/O with isochronous
	 * endpoints. Must be non-negative. */
	int num_iso_packets;

	/** Isochronous packet descriptors, for isochronous transfers only. */
	struct libusb_iso_packet_descriptor iso_packet_desc[LIBUSB_FLEXIBLE_ARRAY];
};

enum libusb_log_level {
	/** (0) : No messages ever emitted by the library (default) */
	LIBUSB_LOG_LEVEL_NONE = 0,

	/** (1) : Error messages are emitted */
	LIBUSB_LOG_LEVEL_ERROR = 1,

	/** (2) : Warning and error messages are emitted */
	LIBUSB_LOG_LEVEL_WARNING = 2,

	/** (3) : Informational, warning and error messages are emitted */
	LIBUSB_LOG_LEVEL_INFO = 3,

	/** (4) : All messages are emitted */
	LIBUSB_LOG_LEVEL_DEBUG = 4
};

/** \ingroup libusb_misc
 * Capabilities supported by an instance of libusb on the current running
 * platform. Test if the loaded library supports a given capability by calling
 * \ref libusb_has_capability().
 */
enum libusb_capability {
	/** The libusb_has_capability() API is available. */
	LIBUSB_CAP_HAS_CAPABILITY = 0x0000U,

	/** Hotplug support is available on this platform. */
	LIBUSB_CAP_HAS_HOTPLUG = 0x0001U,

	/** The library can access HID devices without requiring user intervention.
	 * Note that before being able to actually access an HID device, you may
	 * still have to call additional libusb functions such as
	 * \ref libusb_detach_kernel_driver(). */
	LIBUSB_CAP_HAS_HID_ACCESS = 0x0100U,

	/** The library supports detaching of the default USB driver, using
	 * \ref libusb_detach_kernel_driver(), if one is set by the OS kernel */
	LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER = 0x0101U
};

typedef void (LIBUSB_CALL *libusb_log_cb)(libusb_context *ctx,
	enum libusb_log_level level, const char *str);

struct libusb_device {
//	usbi_atomic_t refcnt;

	struct libusb_context *ctx;
	struct libusb_device *parent_dev;

	uint8_t bus_number;
	uint8_t port_number;
	uint8_t device_address;
	enum libusb_speed speed;

//	struct list_head list;
	unsigned long session_data;

	struct libusb_device_descriptor device_descriptor;
//	usbi_atomic_t attached;
};

#if 0
int libusb_init_context(libusb_context **ctx, const struct libusb_init_option options[], int num_options);
void libusb_exit(libusb_context *ctx);
int libusb_has_capability(uint32_t capability);
int libusb_handle_events(libusb_context *ctx);
int libusb_get_device_descriptor(libusb_device *dev,
	struct libusb_device_descriptor *desc);
#endif

typedef int libusb_hotplug_callback_handle;

typedef enum {
	LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED = (1 << 0),
	LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT = (1 << 1)
} libusb_hotplug_event;


typedef enum {
	LIBUSB_HOTPLUG_ENUMERATE = (1 << 0)
} libusb_hotplug_flag;


struct libusb_device_handle {
	unsigned long claimed_interfaces;

//	struct list_head list;
	struct libusb_device *dev;
	int auto_detach_kernel_driver;
};

#define LIBUSB_HOTPLUG_MATCH_ANY -1

typedef int (LIBUSB_CALL *libusb_hotplug_callback_fn)(libusb_context *ctx,
	libusb_device *device, libusb_hotplug_event event, void *user_data);

#if 0
int libusb_hotplug_register_callback(libusb_context *ctx,
	int events, int flags,
	int vendor_id, int product_id, int dev_class,
	libusb_hotplug_callback_fn cb_fn, void *user_data,
	libusb_hotplug_callback_handle *callback_handle);

void libusb_hotplug_deregister_callback(libusb_context *ctx,
	libusb_hotplug_callback_handle callback_handle);
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#if defined(__cplusplus)
}
#endif

class  libusbApiImpl {
public:
    virtual ~ libusbApiImpl() = default;

    virtual int libusb_init_context(libusb_context **ctx, const struct libusb_init_option options[], int num_options) = 0;
    virtual int libusb_has_capability(uint32_t capability) = 0;
    virtual int libusb_hotplug_register_callback(libusb_context *ctx,
                                                                                  int events, int flags,
                                                                                  int vendor_id, int product_id, int dev_class,
                                                                                  libusb_hotplug_callback_fn cb_fn, void *user_data,
                                                                                  libusb_hotplug_callback_handle *callback_handle) = 0;
    virtual void libusb_exit(libusb_context *ctx) = 0;
    virtual void libusb_hotplug_deregister_callback(libusb_context *ctx,  libusb_hotplug_callback_handle callback_handle) = 0;
    virtual int libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc) = 0;
    virtual int libusb_open(libusb_device *dev, libusb_device_handle **dev_handle) = 0;
    virtual void libusb_close(libusb_device_handle *dev_handle) = 0;
    virtual int libusb_get_string_descriptor(libusb_device_handle *dev_handle,
	                                                                                uint8_t desc_index, uint16_t langid, unsigned char *data, int length) = 0;
    virtual int libusb_get_string_descriptor_ascii(libusb_device_handle *dev_handle,
	                                                                                uint8_t desc_index, unsigned char *data, int length) = 0;
    virtual int libusb_get_active_config_descriptor(libusb_device *dev,
	                                                                                struct libusb_config_descriptor **config) = 0;
    virtual int libusb_get_device_speed(libusb_device *dev) = 0;
    virtual void libusb_free_config_descriptor(struct libusb_config_descriptor *config) = 0;
    virtual int libusb_get_config_descriptor(libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config) = 0;
    virtual int libusb_detach_kernel_driver(libusb_device_handle *dev_handle, int interface_number) = 0;
    virtual int libusb_kernel_driver_active(libusb_device_handle *dev_handle, int interface_number) = 0;
    virtual int libusb_attach_kernel_driver(libusb_device_handle *dev_handle, int interface_number) = 0;
    virtual uint8_t libusb_get_port_number(libusb_device *dev) = 0;
    virtual uint8_t libusb_get_device_address(libusb_device *dev) = 0;
    virtual uint8_t libusb_get_bus_number(libusb_device *dev) = 0;
    virtual int libusb_get_port_numbers(libusb_device *dev, uint8_t *port_numbers, int port_numbers_len) = 0;
    virtual const char *libusb_strerror(int errcode) = 0;
    virtual ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list) = 0;
    virtual void libusb_free_device_list(libusb_device **list, int unref_devices) = 0;
};

class libusbApi {
protected:
    static libusbApiImpl* impl;

public:

    libusbApi();
    libusbApi(const libusbApi &obj) = delete;
    static void setImpl(libusbApiImpl* newImpl);
    static int libusb_init_context(libusb_context **ctx, const struct libusb_init_option options[], int num_options);
    static int libusb_has_capability(uint32_t capability);
    static int libusb_hotplug_register_callback(libusb_context *ctx,
                                                                                  int events, int flags,
                                                                                  int vendor_id, int product_id, int dev_class,
                                                                                  libusb_hotplug_callback_fn cb_fn, void *user_data,
                                                                                  libusb_hotplug_callback_handle *callback_handle);
    static void libusb_exit(libusb_context *ctx);
    static void libusb_hotplug_deregister_callback(libusb_context *ctx,  libusb_hotplug_callback_handle callback_handle);
    static int libusb_handle_events(libusb_context *ctx);
    static int libusb_open(libusb_device *dev, libusb_device_handle **dev_handle);
    static void libusb_close(libusb_device_handle *dev_handle);
    static int libusb_get_string_descriptor(libusb_device_handle *dev_handle,
	                                                                                uint8_t desc_index, uint16_t langid, unsigned char *data, int length);
    static int libusb_get_string_descriptor_ascii(libusb_device_handle *dev_handle,
	                                                                                uint8_t desc_index, unsigned char *data, int length);
    static int libusb_get_active_config_descriptor(libusb_device *dev,
	                                                                                struct libusb_config_descriptor **config);
    static int libusb_get_device_speed(libusb_device *dev);
    static int libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc);
    static int libusb_kernel_driver_active(libusb_device_handle *dev_handle, int interface_number);
    static int libusb_detach_kernel_driver(libusb_device_handle *dev_handle, int interface_number);
    static int libusb_attach_kernel_driver(libusb_device_handle *dev_handle, int interface_number);
    static void libusb_free_config_descriptor(struct libusb_config_descriptor *config);
    static int libusb_get_config_descriptor(libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config);
    static uint8_t libusb_get_port_number(libusb_device *dev);
    static uint8_t libusb_get_device_address(libusb_device *dev);
    static uint8_t libusb_get_bus_number(libusb_device *dev);
    static int libusb_get_port_numbers(libusb_device *dev, uint8_t *port_numbers, int port_numbers_len);
    static const char *libusb_strerror(int errcode);
    static ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list);
    static void libusb_free_device_list(libusb_device **list, int unref_devices);
};

extern int (*libusb_init_context)(libusb_context **ctx, const struct libusb_init_option options[], int num_options);
extern int (*libusb_has_capability)(uint32_t capability);
extern int (*libusb_hotplug_register_callback)(libusb_context *ctx,
                                                                             int events, int flags,
                                                                             int vendor_id, int product_id, int dev_class,
                                                                             libusb_hotplug_callback_fn cb_fn, void *user_data,
                                                                             libusb_hotplug_callback_handle *callback_handle);
extern void (*libusb_exit)(libusb_context *ctx);
extern void (*libusb_hotplug_deregister_callback)(libusb_context *ctx,  libusb_hotplug_callback_handle callback_handle);
extern int (*libusb_handle_events)(libusb_context *ctx);
extern int (*libusb_open)(libusb_device *dev, libusb_device_handle **dev_handle);
extern void (*libusb_close)(libusb_device_handle *dev_handle);
extern int (*libusb_get_string_descriptor)(libusb_device_handle *dev_handle, uint8_t desc_index, uint16_t langid, unsigned char *data, int length);
extern int (*libusb_get_string_descriptor_ascii)(libusb_device_handle *dev_handle, uint8_t desc_index, unsigned char *data, int length);
extern int (*libusb_get_active_config_descriptor)(libusb_device *dev, struct libusb_config_descriptor **config);
extern int (*libusb_get_device_speed)(libusb_device *dev);
extern int (*libusb_get_device_descriptor)(libusb_device *dev, struct libusb_device_descriptor *desc);
extern void (*libusb_free_config_descriptor)(struct libusb_config_descriptor *config);
extern int (*libusb_kernel_driver_active)(libusb_device_handle *dev_handle, int interface_number);
extern int (*libusb_detach_kernel_driver)(libusb_device_handle *dev_handle, int interface_number);
extern int (*libusb_attach_kernel_driver)(libusb_device_handle *dev_handle, int interface_number);
extern int (*libusb_get_config_descriptor)(libusb_device *dev, uint8_t config_index, struct libusb_config_descriptor **config);
extern uint8_t (*libusb_get_port_number)(libusb_device *dev);
extern uint8_t (*libusb_get_device_address)(libusb_device *dev);
extern uint8_t (*libusb_get_bus_number)(libusb_device *dev);
extern int (*libusb_get_port_numbers)(libusb_device *dev, uint8_t *port_numbers, int port_numbers_len);
extern const char* (*libusb_strerror)(int errcode);
extern ssize_t (*libusb_get_device_list)(libusb_context *ctx, libusb_device ***list);
extern void (*libusb_free_device_list)(libusb_device **list, int unref_devices);
