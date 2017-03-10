/*
 * QEMU USB API
 * 
 * Copyright (c) 2005 Fabrice Bellard
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define USB_TOKEN_SETUP 0x2d
#define USB_TOKEN_IN    0x69 /* device -> host */
#define USB_TOKEN_OUT   0xe1 /* host -> device */

/* specific usb messages, also sent in the 'pid' parameter */
#define USB_MSG_ATTACH   0x100
#define USB_MSG_DETACH   0x101
#define USB_MSG_RESET    0x102

#define USB_RET_NODEV   (-1) 
#define USB_RET_NAK     (-2)
#define USB_RET_STALL   (-3)
#define USB_RET_BABBLE  (-4)
#define USB_RET_IOERROR (-5)
//#define USB_RET_ASYNC   (-6)

#define USB_SPEED_LOW   0
#define USB_SPEED_FULL  1
#define USB_SPEED_HIGH  2

#define USB_SPEED_MASK_LOW   (1 << USB_SPEED_LOW)
#define USB_SPEED_MASK_FULL  (1 << USB_SPEED_FULL)

#define USB_STATE_NOTATTACHED 0
#define USB_STATE_ATTACHED    1
//#define USB_STATE_POWERED     2
#define USB_STATE_DEFAULT     3
//#define USB_STATE_ADDRESS     4
//#define	USB_STATE_CONFIGURED  5
#define USB_STATE_SUSPENDED   6

#define USB_CLASS_AUDIO			1
#define USB_CLASS_COMM			2
#define USB_CLASS_HID			3
#define USB_CLASS_PHYSICAL		5
#define USB_CLASS_STILL_IMAGE		6
#define USB_CLASS_PRINTER		7
#define USB_CLASS_MASS_STORAGE		8
#define USB_CLASS_HUB			9
#define USB_CLASS_CDC_DATA		0x0a
#define USB_CLASS_CSCID			0x0b
#define USB_CLASS_CONTENT_SEC		0x0d
#define USB_CLASS_APP_SPEC		0xfe
#define USB_CLASS_VENDOR_SPEC		0xff

#define USB_DIR_OUT			0
#define USB_DIR_IN			0x80

#define USB_TYPE_MASK			(0x03 << 5)
#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)

#define USB_RECIP_MASK			0x1f
#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE		0x01
#define USB_RECIP_ENDPOINT		0x02
#define USB_RECIP_OTHER			0x03

#define DeviceRequest ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8) //0x8000
#define DeviceOutRequest ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8) // 0x0000
#define VendorDeviceRequest ((USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE)<<8) // 0xC000
#define VendorDeviceOutRequest \
        ((USB_DIR_OUT|USB_TYPE_VENDOR|USB_RECIP_DEVICE)<<8) //0x4000
#define InterfaceRequest \
        ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8) // 0x8100
#define InterfaceOutRequest \
        ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8) //0x0100
#define EndpointRequest ((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_ENDPOINT)<<8) //0x8200
#define EndpointOutRequest \
        ((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_ENDPOINT)<<8) //0x0200

#define ClassInterfaceRequest \
        ((USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_INTERFACE)<<8) //0xA100
#define ClassInterfaceOutRequest \
        ((USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE)<<8) //0x2100

#define ClassEndpointRequest \
        ((USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_ENDPOINT)<<8) //0xA200
#define ClassEndpointOutRequest \
        ((USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_ENDPOINT)<<8) //0x2200

#define USB_REQ_GET_STATUS		0x00
#define USB_REQ_CLEAR_FEATURE		0x01
#define USB_REQ_SET_FEATURE		0x03
#define USB_REQ_SET_ADDRESS		0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		0x0A
#define USB_REQ_SET_INTERFACE		0x0B
#define USB_REQ_SYNCH_FRAME		0x0C

#define USB_DEVICE_SELF_POWERED		0
#define USB_DEVICE_REMOTE_WAKEUP	1

#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05
#define USB_DT_CLASS			0x24

/* HID interface requests */
#define GET_REPORT   0xa101
#define GET_IDLE     0xa102
#define GET_PROTOCOL 0xa103
#define SET_IDLE     0x210a
#define SET_PROTOCOL 0x210b

#define USB_DEVICE_DESC_SIZE        18
#define USB_CONFIGURATION_DESC_SIZE 9
#define USB_INTERFACE_DESC_SIZE     9
#define USB_ENDPOINT_DESC_SIZE      7

typedef struct USBPort USBPort;
typedef struct USBDevice USBDevice;

/* definition of a USB device */
struct USBDevice {
    USBPort *port;
    int (*handle_packet)(USBDevice *dev, int pid, 
                         uint8_t devaddr, uint8_t devep,
                         uint8_t *data, int len);
    void (*handle_destroy)(USBDevice *dev);

	//might be useful
	int (*open)(USBDevice *dev);
	void (*close)(USBDevice *dev);

    int speed;
    
    /* The following fields are used by the generic USB device
       layer. They are here just to avoid creating a new structure for
       them. */
    void (*handle_reset)(USBDevice *dev);
    int (*handle_control)(USBDevice *dev, int request, int value,
                          int index, int length, uint8_t *data);
    int (*handle_data)(USBDevice *dev, int pid, uint8_t devep,
                       uint8_t *data, int len);
    uint8_t addr;
    
    int state;
    uint8_t setup_buf[8];
    uint8_t data_buf[1024];
    int remote_wakeup;
    int setup_state;
    int setup_len;
    int setup_index;
};

typedef struct USBPortOps {
    void (*attach)(USBPort *port, USBDevice *dev);
    //void (*detach)(USBPort *port);
    /*
     * This gets called when a device downstream from the device attached to
     * the port (iow attached through a hub) gets detached.
     */
    //void (*child_detach)(USBPort *port, USBDevice *child);
    void (*wakeup)(USBPort *port);
    /*
     * Note that port->dev will be different then the device from which
     * the packet originated when a hub is involved.
     */
    //void (*complete)(USBPort *port, USBPacket *p);
} USBPortOps;

/* USB port on which a device can be connected */
struct USBPort {
    USBDevice *dev;
    int speedmask;
    USBPortOps *ops;
    void *opaque;
    int index; /* internal port index, may be used with the opaque */
    //struct USBPort *next; /* Used internally by qemu.  */
};

int usb_generic_handle_packet(USBDevice *s, int pid, 
                              uint8_t devaddr, uint8_t devep,
                              uint8_t *data, int len);
int set_usb_string(uint8_t *buf, const char *str);
int set_usb_string(uint8_t *buf, const char *str, int len);
void usb_device_reset(USBDevice *dev);

/* usb hub */
USBDevice *usb_hub_init(int nb_ports);

/* usb-ohci.c */
void usb_ohci_init(void *bus, int num_ports, int devfn);

/* usb-hid.c */
USBDevice *usb_mouse_init(void);

/* usb-kbd.c */
USBDevice *usb_keyboard_init(void);
