/***************************************************************************
 * Copyright (c) 2025-present Eclipse ThreadX Contributors
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** Overview                                                              */
/**                                                                       */
/**  This example works as a USB HID device. It will appear as a USB      */
/**  custom device on PC. This application demo is running in rtos        */
/**  mode.                                                                */
/**                                                                       */
/**  This sample show how to use usbx hid device class as custom device:  */
/**  - Consumer: media + brightness control                               */
/**  This sample can be expanded to support other custom devices.         */
/**                                                                       */
/** Note                                                                  */
/**                                                                       */
/**  This demonstration is not optimized, to optimize application user    */
/**  sould configuer related class flag in ux_user.h and adjust           */
/**  UX_DEVICE_MEMORY_STACK_SIZE                                          */
/**                                                                       */
/**                                                                       */
/**  AUTHOR                                                               */
/**                                                                       */
/**   Mohamed AYED                                                        */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#include "tx_api.h"
#include "ux_api.h"
#include "ux_device_class_hid.h"

#ifndef UX_DEVICE_SIDE_ONLY
#error UX_DEVICE_SIDE_ONLY must be defined
#endif

/**************************************************/
/**  Define constants                             */
/**************************************************/
#define UX_DEVICE_MEMORY_STACK_SIZE     (7*1024)
#define UX_DEMO_THREAD_STACK_SIZE       (1*1024)

#define UX_DEMO_HID_DEVICE_VID          0x070A
#define UX_DEMO_HID_DEVICE_PID          0x4027

#define UX_DEMO_MAX_EP0_SIZE            0x40U
#define UX_DEMO_HID_CONFIG_DESC_SIZE    0x22U

#define UX_DEMO_BCD_USB                 0x0200

#define UX_DEMO_BCD_HID                 0x0110

#define UX_DEMO_HID_ENDPOINT_SIZE       0x08
#define UX_DEMO_HID_ENDPOINT_ADDRESS    0x81
#define UX_DEMO_HID_ENDPOINT_BINTERVAL  0x08

#define UX_CONSUMER_MEDIA               0x00
#define UX_CONSUMER_BRIGHTNESS          0x01

#define UX_CONSUMER_BRIGHTNESS_DOWN     0x00
#define UX_CONSUMER_BRIGHTNESS_UP       0x01
#define UX_CONSUMER_BRIGHTNESS_DONE     0x10

#define UX_CONSUMER_MEDIA_VOLUME_DOWN   0x00
#define UX_CONSUMER_MEDIA_VOLUME_UP     0x01
#define UX_CONSUMER_MEDIA_MUTE          0x02
#define UX_CONSUMER_MEDIA_UNMUTE        0x03
#define UX_CONSUMER_MEDIA_DONE          0x10

#define UX_CONSUMER_FINISH              0x10

/**************************************************/
/**  usbx device hid demo callbacks               */
/**************************************************/
VOID ux_demo_device_hid_instance_activate(VOID *hid_instance);
VOID ux_demo_device_hid_instance_deactivate(VOID *hid_instance);
UINT ux_demo_device_hid_callback(UX_SLAVE_CLASS_HID *hid_instance, UX_SLAVE_CLASS_HID_EVENT *hid_event);
UINT ux_demo_device_hid_get_callback(UX_SLAVE_CLASS_HID *hid_instance, UX_SLAVE_CLASS_HID_EVENT *hid_event);

/**************************************************/
/**  usbx device hid demo thread                  */
/**************************************************/
VOID ux_demo_device_hid_thread_entry(ULONG thread_input);

/**************************************************/
/**  usbx application initialization with RTOS    */
/**************************************************/
VOID tx_application_define(VOID *first_unused_memory);

/**************************************************/
/**  usbx device hid demo consumer                */
/**************************************************/
UINT ux_demo_hid_consumer_media_control(UX_SLAVE_CLASS_HID *device_hid);
UINT ux_demo_hid_consumer_brightness_control(UX_SLAVE_CLASS_HID *device_hid);

/**************************************************/
/**  usbx device hid consumer instance            */
/**************************************************/
UX_SLAVE_CLASS_HID *hid_consumer;

/**************************************************/
/**  thread object                                */
/**************************************************/
static TX_THREAD ux_hid_thread;
static ULONG ux_hid_thread_stack[UX_DEMO_THREAD_STACK_SIZE / sizeof(ULONG)];

/**************************************************/
/**  usbx callback error                          */
/**************************************************/
static VOID ux_demo_error_callback(UINT system_level, UINT system_context, UINT error_code);

static CHAR ux_system_memory_pool[UX_DEVICE_MEMORY_STACK_SIZE];


#ifndef EXTERNAL_MAIN
extern int board_setup(void);
#endif /* EXTERNAL_MAIN */
extern int usb_device_dcd_initialize(void *param);

/**************************************************/
/**  HID Report descriptor                        */
/**************************************************/
UCHAR hid_consumer_report[] = {
    0x05, 0x0C,         // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,         // USAGE (Consumer)
    0xA1, 0x01,         // COLLECTION (Application)

    0x15, 0x00,         //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x03,   //   LOGICAL_MAXIMUM (0x03FF)
    0x75, 0x10,         //   REPORT_SIZE (16 bits per report)
    0x95, 0x01,         //   REPORT_COUNT (1 event at a time)

    /* Screen Brightness Usages */
    0x09, 0x6F,         //   USAGE (Brightness Up)      -> event number 1
    0x09, 0x70,         //   USAGE (Brightness Down)    -> event number 2

    /* Media Control Usages */
    0x09, 0xE9,         //   USAGE (Volume Up)          -> event number 3
    0x09, 0xEA,         //   USAGE (Volume Down)        -> event number 4
    0x09, 0xE2,         //   USAGE (Mute)               -> event number 5
    0x09, 0xCD,         //   USAGE (Play/Pause)         -> event number 6
    0x09, 0xB5,         //   USAGE (Next Track)         -> event number 7
    0x09, 0xB6,         //   USAGE (Previous Track)     -> event number 8

    0x81, 0x00,         //   INPUT (Data, Array, Absolute) - Sends one event at a time

    0xC0                // End Collection
};

#define UX_HID_CONSUMER_REPORT_LENGTH (sizeof(hid_consumer_report)/sizeof(hid_consumer_report[0]))

#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(ux_demo_device_framework_full_speed)
UCHAR ux_demo_device_framework_full_speed[] = {
    /* Device descriptor */
    0x12,                       /* bLength */
    0x01,                       /* bDescriptorType */
    UX_W0(UX_DEMO_BCD_USB), UX_W1(UX_DEMO_BCD_USB), /* bcdUSB */
    0x00,                       /* bDeviceClass : 0x00 : Interface-defined */
    0x00,                       /* bDeviceSubClass : 0x00 : Reset */
    0x00,                       /* bDeviceProtocol : 0x00 : Reset */
    UX_DEMO_MAX_EP0_SIZE,       /* bMaxPacketSize0 */
    UX_W0(UX_DEMO_HID_DEVICE_VID), UX_W1(UX_DEMO_HID_DEVICE_VID), /* idVendor : ... */
    UX_W0(UX_DEMO_HID_DEVICE_PID), UX_W1(UX_DEMO_HID_DEVICE_PID), /* idProduct */
    0x00, 0x00,                 /* bcdDevice */
    0x01,                       /* iManufacturer */
    0x02,                       /* iProduct */
    0x03,                       /* iSerialNumber */
    0x01,                       /* bNumConfigurations */

    /* Configuration Descriptor, total 34 */
    0x09,                       /* bLength */
    0x02,                       /* bDescriptorType */
    UX_W0(UX_DEMO_HID_CONFIG_DESC_SIZE), UX_W1(UX_DEMO_HID_CONFIG_DESC_SIZE), /* wTotalLength */
    0x01,                       /* bNumInterfaces */
    0x01,                       /* bConfigurationValue */
    0x04,                       /* iConfiguration */
    0xC0,                       /* bmAttributes */
                                    /* D6 : 0x1 : Self-powered */
                                    /* D5, Remote Wakeup : 0x0 : Not supported */
    0x32,                       /* bMaxPower : 50 : 100mA */

    /* Interface descriptor */
    0x09,                       /* bLength */
    0x04,                       /* bDescriptorType */
    0x00,                       /* bInterfaceNumber */
    0x00,                       /* bAlternateSetting */
    0x01,                       /* bNumEndpoints */
    0x03,                       /* bInterfaceClass : 0x03 : HID */
    0x00,                       /* bInterfaceSubClass : non-boot Subclass */
    0x00,                       /* bInterfaceProtocol : 0x00 : Undefined */
    0x06,                       /* iInterface */

    /* HID Descriptor */
    0x09,                       /* bLength : 9 */
    0x21,                       /* bDescriptorType : 0x21 : HID descriptor */
    0x10, 0x01,                 /* bcdHID : 0x0110 */
    0x21,                       /* bCountryCode : 33 : US */
    0x01,                       /* bNumDescriptors */
    0x22,                       /* bReportDescriptorType1 : 0x22 : Report descriptor */
    UX_W0(UX_HID_CONSUMER_REPORT_LENGTH), UX_W1(UX_HID_CONSUMER_REPORT_LENGTH), /* wDescriptorLength1 */

    /* Endpoint Descriptor */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType */
    UX_DEMO_HID_ENDPOINT_ADDRESS,   /* bEndpointAddress */
                                    /* D7, Direction : 0x01 */
                                    /* D3..0, Endpoint number : 1 */
    0x03,                           /* bmAttributes */
                                        /* D1..0, Transfer Type : 0x3 : Interrupt */
                                        /* D3..2, Synchronization Type : 0x0 : No Synchronization */
                                        /* D5..4, Usage Type : 0x0 : Data endpoint */
    UX_W0(UX_DEMO_HID_ENDPOINT_SIZE), UX_W1(UX_DEMO_HID_ENDPOINT_SIZE), /* wMaxPacketSize */
                                        /* D10..0, Max Packet Size */
                                        /* D12..11, Additional transactions : 0x00 */
    UX_DEMO_HID_ENDPOINT_BINTERVAL, /* bInterval : 8 : 8ms / x128 (FS 128ms/HS 16ms) */
};

#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(ux_demo_device_framework_high_speed)
UCHAR ux_demo_device_framework_high_speed[] = {
    /* Device descriptor */
    0x12,                       /* bLength */
    0x01,                       /* bDescriptorType */
    UX_W0(UX_DEMO_BCD_USB), UX_W1(UX_DEMO_BCD_USB), /* bcdUSB */
    0x00,                       /* bDeviceClass : 0x00 : Interface-defined */
    0x00,                       /* bDeviceSubClass : 0x00 : Reset */
    0x00,                       /* bDeviceProtocol : 0x00 : Reset */
    UX_DEMO_MAX_EP0_SIZE,       /* bMaxPacketSize0 */
    UX_W0(UX_DEMO_HID_DEVICE_VID), UX_W1(UX_DEMO_HID_DEVICE_VID), /* idVendor : ... */
    UX_W0(UX_DEMO_HID_DEVICE_PID), UX_W1(UX_DEMO_HID_DEVICE_PID), /* idProduct */
    0x01, 0x00,                 /* bcdDevice */
    0x01,                       /* iManufacturer */
    0x02,                       /* iProduct */
    0x03,                       /* iSerialNumber */
    0x01,                       /* bNumConfigurations */

    /* Device qualifier descriptor */
    0x0A,                       /* bLength */
    0x06,                       /* bDescriptorType */
    UX_W0(UX_DEMO_BCD_USB), UX_W1(UX_DEMO_BCD_USB), /* bcdUSB */
    0x00,                       /* bDeviceClass : 0x00 : Interface-defined */
    0x00,                       /* bDeviceSubClass : 0x00 : Reset */
    0x00,                       /* bDeviceProtocol : 0x00 : Reset */
    UX_DEMO_MAX_EP0_SIZE,       /* bMaxPacketSize0 */
    0x01,                       /* bNumConfigurations */
    0x00,                       /* bReserved */

    /* Configuration descriptor */
    0x09,                       /* bLength */
    0x02,                       /* bDescriptorType */
    UX_W0(UX_DEMO_HID_CONFIG_DESC_SIZE), UX_W1(UX_DEMO_HID_CONFIG_DESC_SIZE), /* wTotalLength */
    0x01,                       /* bNumInterfaces */
    0x01,                       /* bConfigurationValue */
    0x05,                       /* iConfiguration */
    0xC0,                       /* bmAttributes */
                                    /* D6 : 0x1 : Self-powered */
                                    /* D5, Remote Wakeup : 0x0 : Not supported */
    0x19,                       /* bMaxPower : 50 : 100mA */

    /* Interface descriptor */
    0x09,                       /* bLength */
    0x04,                       /* bDescriptorType */
    0x00,                       /* bInterfaceNumber */
    0x00,                       /* bAlternateSetting */
    0x01,                       /* bNumEndpoints */
    0x03,                       /* bInterfaceClass : 0x03 : HID */
    0x00,                       /* bInterfaceSubClass : non-boot Subclass */
    0x00,                       /* bInterfaceProtocol : 0x00 : Undefined */
    0x06,                       /* iInterface */

    /* HID Descriptor */
    0x09,                       /* bLength : 9 */
    0x21,                       /* bDescriptorType : 0x21 : HID descriptor */
    UX_W0(UX_DEMO_BCD_HID), UX_W1(UX_DEMO_BCD_HID), /* bcdHID : 0x0110 */
    0x21,                       /* bCountryCode : 33 : US */
    0x01,                       /* bNumDescriptors */
    0x22,                       /* bReportDescriptorType1 : 0x22 : Report descriptor */
    UX_W0(UX_HID_CONSUMER_REPORT_LENGTH), UX_W1(UX_HID_CONSUMER_REPORT_LENGTH), /* wDescriptorLength1  */

    /* Endpoint Descriptor (Interrupt In) */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType */
    UX_DEMO_HID_ENDPOINT_ADDRESS,   /* bEndpointAddress */
                                    /* D7, Direction : 0x01 */
                                    /* D3..0, Endpoint number : 1 */
    0x03,                           /* bmAttributes */
                                        /* D1..0, Transfer Type : 0x3 : Interrupt */
                                        /* D3..2, Synchronization Type : 0x0 : No Synchronization */
                                        /* D5..4, Usage Type : 0x0 : Data endpoint */
    UX_W0(UX_DEMO_HID_ENDPOINT_SIZE), UX_W1(UX_DEMO_HID_ENDPOINT_SIZE), /* wMaxPacketSize */
                                        /* D10..0, Max Packet Size */
                                        /* D12..11, Additional transactions : 0x00 */
    UX_DEMO_HID_ENDPOINT_BINTERVAL, /* bInterval : 8 : 8ms / x128 (FS 128ms/HS 16ms) */
};


/* String Device Framework :
   Byte 0 and 1 : Word containing the language ID : 0x0904 for US
   Byte 2       : Byte containing the index of the descriptor
   Byte 3       : Byte containing the length of the descriptor string
*/
#define STRING_FRAMEWORK_LENGTH sizeof(ux_demo_string_framework)
UCHAR ux_demo_string_framework[] = {

    /* iManufacturer string descriptor : Index 1 */
    0x09, 0x04, 0x01, 12,
    'U', 'S', 'B', 'X', ' ', 'e', 'c', 'l', 'i', 'p', 's', 'e',

    /* iProduct string descriptor : Index 2 */
    0x09, 0x04, 0x02, 17,
    'H', 'I', 'D', ' ', 'C', 'o', 'n', 's', 'u', 'm', 'e', 'r', ' ', 'D', 'e', 'm', 'o',

    /* iSerialNumber Number string descriptor : Index 3 */
    0x09, 0x04, 0x03, 13,
    '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '1',

    /* iConfiguration string descriptor : Index 4 */
    0x09, 0x04, 0x04, 10,
    'F', 'U', 'L', 'L', ' ', 'S', 'P', 'E', 'E', 'D',

    /* iConfiguration string descriptor : Index 5 */
    0x09, 0x04, 0x05, 10,
    'H', 'I', 'G', 'H', ' ', 'S', 'P', 'E', 'E', 'D',

    /* iInterface string descriptor : Index 6 */
    0x09, 0x04, 0x06, 7,
    'C', 'o', 's', 'u', 'm', 'e', 'r'
};


/* Multiple languages are supported on the device, to add  a language besides english,
   the unicode language code must be appended to the ux_demo_language_id_framework array and the length
   adjusted accordingly.
*/
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(ux_demo_language_id_framework)
UCHAR ux_demo_language_id_framework[] = {
    /* English. */
    0x09, 0x04
};

#ifndef EXTERNAL_MAIN
int main(void)
{
    /* Initialize the board.  */
    board_setup();

    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}
#endif /* EXTERNAL_MAIN */

VOID tx_application_define(VOID *first_unused_memory)
{
CHAR                            *memory_pointer;
UINT                            status;
UX_SLAVE_CLASS_HID_PARAMETER    hid_consumer_parameter;


    UX_PARAMETER_NOT_USED(first_unused_memory);

    /* Use static memory block.  */
    memory_pointer = ux_system_memory_pool;

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEVICE_MEMORY_STACK_SIZE, UX_NULL, 0);

    if(status != UX_SUCCESS)
        return;

    /* Install the device portion of USBX.  */
    status =  ux_device_stack_initialize(ux_demo_device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                         ux_demo_device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                         ux_demo_string_framework, STRING_FRAMEWORK_LENGTH,
                                         ux_demo_language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,
                                         UX_NULL);

    if(status != UX_SUCCESS)
        return;

    /* Initialize the hid consumer class parameters for the device */
    hid_consumer_parameter.ux_slave_class_hid_instance_activate         = ux_demo_device_hid_instance_activate;
    hid_consumer_parameter.ux_slave_class_hid_instance_deactivate       = ux_demo_device_hid_instance_deactivate;
    hid_consumer_parameter.ux_device_class_hid_parameter_report_address = hid_consumer_report;
    hid_consumer_parameter.ux_device_class_hid_parameter_report_length  = UX_HID_CONSUMER_REPORT_LENGTH;
    hid_consumer_parameter.ux_device_class_hid_parameter_report_id      = UX_FALSE;
    hid_consumer_parameter.ux_device_class_hid_parameter_callback       = ux_demo_device_hid_callback;
    hid_consumer_parameter.ux_device_class_hid_parameter_get_callback   = ux_demo_device_hid_get_callback;

    /* Initialize the device storage class. The class is connected with interface 0 on configuration 1. */
    status = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                            1, 0, (VOID *)&hid_consumer_parameter);

    if(status != UX_SUCCESS)
        return;

    /* Create the main demo thread.  */
    status = ux_utility_thread_create(&ux_hid_thread, "hid_usbx_app_thread_entry",
                                      ux_demo_device_hid_thread_entry, 0, ux_hid_thread_stack,
                                      UX_DEMO_THREAD_STACK_SIZE, 20, 20, 1, TX_AUTO_START);

    if(status != UX_SUCCESS)
        return;

    /* Register error callback.  */
    ux_utility_error_callback_register(ux_demo_error_callback);
}

/********************************************************************/
/**  ux_demo_device_hid_instance_activate                           */
/********************************************************************/
VOID ux_demo_device_hid_instance_activate(VOID *hid_instance)
{
    if (hid_consumer == UX_NULL)
        hid_consumer = (UX_SLAVE_CLASS_HID*) hid_instance;
}

/********************************************************************/
/**  ux_demo_device_hid_instance_deactivate                         */
/********************************************************************/
VOID ux_demo_device_hid_instance_deactivate(VOID *hid_instance)
{
    if (hid_instance == (VOID *)hid_consumer)
        hid_consumer = UX_NULL;
}

/********************************************************************/
/**  ux_demo_device_hid_callback                                    */
/********************************************************************/
UINT ux_demo_device_hid_callback(UX_SLAVE_CLASS_HID *hid_instance, UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
    UX_PARAMETER_NOT_USED(hid_instance);
    UX_PARAMETER_NOT_USED(hid_event);

    return UX_SUCCESS;
}

/********************************************************************/
/**  ux_demo_device_hid_get_callback                                */
/********************************************************************/
UINT ux_demo_device_hid_get_callback(UX_SLAVE_CLASS_HID *hid_instance, UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
    UX_PARAMETER_NOT_USED(hid_instance);
    UX_PARAMETER_NOT_USED(hid_event);

    return UX_SUCCESS;
}

/********************************************************************/
/**  ux_demo_device_hid_thread_entry: hid demo thread               */
/********************************************************************/
VOID ux_demo_device_hid_thread_entry(ULONG thread_input)
{
UINT            status;
UINT            demo_state = 0;

    UX_PARAMETER_NOT_USED(thread_input);

    /* Register the USB device controllers available in this system.  */
    usb_device_dcd_initialize(UX_NULL);

    while (1)
    {
      /* Check if the device state already configured.  */
      if ((UX_SLAVE_DEVICE_CHECK_STATE(UX_DEVICE_CONFIGURED)) && (hid_consumer != UX_NULL))
      {
        switch(demo_state)
        {

        case UX_CONSUMER_MEDIA:

          /* Control media.  */
          status = ux_demo_hid_consumer_media_control(hid_consumer);

          if (status == UX_CONSUMER_MEDIA_DONE)
              demo_state = UX_CONSUMER_BRIGHTNESS;

          break;

        case UX_CONSUMER_BRIGHTNESS:

          /* Control brightness.  */
          status = ux_demo_hid_consumer_brightness_control(hid_consumer);

          if (status == UX_CONSUMER_BRIGHTNESS_DONE)
              demo_state = UX_CONSUMER_FINISH;

          break;

        default:

          ux_utility_delay_ms(MS_TO_TICK(10));

          break;
        }
      }
      else
      {
        /* Sleep thread for 10ms.  */
        ux_utility_delay_ms(MS_TO_TICK(10));
      }
    }
}

/********************************************************************/
/**  ux_demo_hid_consumer_media_control:                            */
/**             mute, unmute, volume up/down, play/pause,           */
/********************************************************************/
UINT ux_demo_hid_consumer_media_control(UX_SLAVE_CLASS_HID *device_hid)
{
UCHAR                     status;
UX_SLAVE_CLASS_HID_EVENT  device_hid_event;
static UCHAR              media_event;
static UCHAR              volume_level = 100;


    /* Initialize mouse event.  */
    device_hid_event.ux_device_class_hid_event_report_id = 0;
    device_hid_event.ux_device_class_hid_event_report_type = UX_DEVICE_CLASS_HID_REPORT_TYPE_INPUT;
    device_hid_event.ux_device_class_hid_event_length = 2;


    switch(media_event)
    {
    case UX_CONSUMER_MEDIA_VOLUME_DOWN:

        /* Sleep thread for 50ms.  */
        ux_utility_delay_ms(MS_TO_TICK(50));

        device_hid_event.ux_device_class_hid_event_buffer[0] = 0x04;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        /* Send release event */
        device_hid_event.ux_device_class_hid_event_buffer[0] = 0;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Sleep thread for 10ms.  */
        ux_utility_delay_ms(MS_TO_TICK(10));

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        volume_level -= 2;

        if (volume_level == 0)
            media_event = UX_CONSUMER_MEDIA_VOLUME_UP;

        break;

    case UX_CONSUMER_MEDIA_VOLUME_UP:

        /* Sleep thread for 50ms.  */
        ux_utility_delay_ms(MS_TO_TICK(50));

        device_hid_event.ux_device_class_hid_event_buffer[0] = 0x03;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        /* Send release event */
        device_hid_event.ux_device_class_hid_event_buffer[0] = 0;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Sleep thread for 10ms.  */
        ux_utility_delay_ms(MS_TO_TICK(10));

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        volume_level += 2;

        if (volume_level == 100)
            media_event = UX_CONSUMER_MEDIA_MUTE;

        break;

    case UX_CONSUMER_MEDIA_MUTE:

        /* Sleep thread for 10ms.  */
        ux_utility_delay_ms(MS_TO_TICK(10));

        device_hid_event.ux_device_class_hid_event_buffer[0] = 0x05;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        /* Send release event */
        device_hid_event.ux_device_class_hid_event_buffer[0] = 0;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Sleep thread for 10ms.  */
        ux_utility_delay_ms(MS_TO_TICK(10));

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        media_event = UX_CONSUMER_MEDIA_UNMUTE;

        break;

    case UX_CONSUMER_MEDIA_UNMUTE:

        /* Sleep thread for 100ms.  */
        ux_utility_delay_ms(MS_TO_TICK(100));

        device_hid_event.ux_device_class_hid_event_buffer[0] = 0x05;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        /* Send release event */
        device_hid_event.ux_device_class_hid_event_buffer[0] = 0;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Sleep thread for 10ms.  */
        ux_utility_delay_ms(MS_TO_TICK(10));

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        media_event = UX_CONSUMER_MEDIA_DONE;

        break;

    default:

        ux_utility_memory_set(&device_hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

        break;
    }

    return media_event;
}

/********************************************************************/
/**  ux_demo_hid_consumer_brightness_control:                       */
/**             brightness up/down                                  */
/********************************************************************/
UINT ux_demo_hid_consumer_brightness_control(UX_SLAVE_CLASS_HID *device_hid)
{
UCHAR                     status;
UX_SLAVE_CLASS_HID_EVENT  device_hid_event;
static UCHAR              brightness_event;
static UCHAR              brightness_level = 100;


    /* Initialize mouse event.  */
    device_hid_event.ux_device_class_hid_event_report_id = 0;
    device_hid_event.ux_device_class_hid_event_report_type = UX_DEVICE_CLASS_HID_REPORT_TYPE_INPUT;
    device_hid_event.ux_device_class_hid_event_length = 2;


    switch(brightness_event)
    {
    case UX_CONSUMER_BRIGHTNESS_DOWN:

        /* Sleep thread for 500ms.  */
        ux_utility_delay_ms(MS_TO_TICK(500));

        device_hid_event.ux_device_class_hid_event_buffer[0] = 0x02;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        /* Send release event */
        device_hid_event.ux_device_class_hid_event_buffer[0] = 0;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Sleep thread for 10ms.  */
        ux_utility_delay_ms(MS_TO_TICK(10));

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        brightness_level -= 2;

        if (brightness_level == 0)
            brightness_event = UX_CONSUMER_BRIGHTNESS_UP;

        break;

    case UX_CONSUMER_BRIGHTNESS_UP:

        /* Sleep thread for 500ms.  */
        ux_utility_delay_ms(MS_TO_TICK(500));

        device_hid_event.ux_device_class_hid_event_buffer[0] = 0x01;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        /* Send release event */
        device_hid_event.ux_device_class_hid_event_buffer[0] = 0;
        device_hid_event.ux_device_class_hid_event_buffer[1] = 0;

        /* Sleep thread for 10ms.  */
        ux_utility_delay_ms(MS_TO_TICK(10));

        status = ux_device_class_hid_event_set(device_hid, &device_hid_event);

        if(status != UX_SUCCESS)
            return UX_ERROR;

        brightness_level += 2;

        if (brightness_level == 100)
            brightness_event = UX_CONSUMER_BRIGHTNESS_DONE;

        break;

    default:

        ux_utility_memory_set(&device_hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

        break;
    }

    return brightness_event;
}

static VOID ux_demo_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    /*
     * Refer to ux_api.h. For example,
     * UX_SYSTEM_LEVEL_INTERRUPT, UX_SYSTEM_CONTEXT_DCD, UX_DEVICE_HANDLE_UNKNOWN
     */
    printf("USBX error: system level(%d), context(%d), error code(0x%x)\r\n", system_level, system_context, error_code);
}
