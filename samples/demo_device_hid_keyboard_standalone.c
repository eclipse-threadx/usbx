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
/**  keyboard  device on PC. This application demo is running in          */
/**  standalone mode.                                                     */
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

#include "ux_api.h"
#include "ux_device_class_hid.h"

#ifndef UX_DEVICE_SIDE_ONLY
#error UX_DEVICE_SIDE_ONLY must be defined
#endif

#ifndef UX_STANDALONE
#warning UX_STANDALONE must be define for this sample.
#endif

#if UX_PERIODIC_RATE != 1000
#warning UX_PERIODIC_RATE should be 1000 for 1ms tick.
#endif

#if (UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH < 8)
#error HID Keyboard event buffer length must be more then 8
#endif


/* Defined the keyboard will act as boot device. */
/* define DEMO_HID_BOOT_DEVICE */

/**************************************************/
/**  Define constants                             */
/**************************************************/
#define UX_DEVICE_MEMORY_STACK_SIZE     (7*1024)

#define UX_DEMO_HID_DEVICE_VID          0x070A
#define UX_DEMO_HID_DEVICE_PID          0x4090
#define UX_DEMO_MAX_EP0_SIZE            0x40U
#define UX_DEMO_HID_CONFIG_DESC_SIZE    0x22U

#define UX_DEMO_BCD_USB                 0x0200

#define UX_DEMO_BCD_HID                 0x0110

#define UX_DEMO_HID_ENDPOINT_SIZE       0x08
#define UX_DEMO_HID_ENDPOINT_ADDRESS    0x81
#define UX_DEMO_HID_ENDPOINT_BINTERVAL  0x08

#ifdef DEMO_HID_BOOT_DEVICE
#define UX_DEMO_HID_SUBCLASS            0x01
#else /* DEMO_HID_BOOT_DEVICE */
#define UX_DEMO_HID_SUBCLASS            0x00
#endif

#define UX_HID_NUM_LOCK_MASK            0x01
#define UX_HID_CAPS_LOCK_MASK           0x02

/**************************************************/
/**  usbx device hid demo callbacks               */
/**************************************************/
VOID ux_demo_device_hid_instance_activate(VOID *hid_instance);
VOID ux_demo_device_hid_instance_deactivate(VOID *hid_instance);
UINT ux_demo_device_hid_callback(UX_SLAVE_CLASS_HID *hid_instance, UX_SLAVE_CLASS_HID_EVENT *hid_event);
UINT ux_demo_device_hid_get_callback(UX_SLAVE_CLASS_HID *hid_instance, UX_SLAVE_CLASS_HID_EVENT *hid_event);

/**************************************************/
/**  usbx device hid keyboard instance            */
/**************************************************/
UX_SLAVE_CLASS_HID      *hid_keyboard;
UX_SLAVE_CLASS_HID_EVENT device_hid_event;

VOID ux_application_define(VOID);
VOID ux_demo_device_hid_task(VOID);

/**************************************************/
/**  usbx device hid keyboard                     */
/**************************************************/
ULONG num_lock_flag  = UX_FALSE;
ULONG caps_lock_flag = UX_FALSE;

/**************************************************/
/**  usbx callback error                          */
/**************************************************/
static VOID ux_demo_error_callback(UINT system_level, UINT system_context, UINT error_code);
static VOID demo_delay_with_tasks_running(ULONG ms_wait);

static CHAR ux_system_memory_pool[UX_DEVICE_MEMORY_STACK_SIZE];

#ifndef EXTERNAL_MAIN
extern int board_setup(void);
#endif /* EXTERNAL_MAIN */
extern int usb_device_dcd_initialize(void *param);

/**************************************************/
/**  HID Report descriptor                        */
/**************************************************/
UCHAR hid_keyboard_report[] = {
    0x05, 0x01,         // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,         // USAGE (Keyboard)
    0xa1, 0x01,         // COLLECTION (Application)

    /* Modifier Keys (Shift, Ctrl, Alt, GUI) */
    0x05, 0x07,         //     USAGE_PAGE (Key Codes)
    0x19, 0xE0,         //     USAGE_MINIMUM (Left Control)
    0x29, 0xE7,         //     USAGE_MAXIMUM (Right GUI)
    0x15, 0x00,         //     LOGICAL_MINIMUM (0)
    0x25, 0x01,         //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,         //     REPORT_SIZE (1)
    0x95, 0x08,         //     REPORT_COUNT (8)
    0x81, 0x02,         //     INPUT (Data, Variable, Absolute)

    /* Reserved byte */
    0x75, 0x08,         //     REPORT_SIZE (8)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x81, 0x01,         //     INPUT (Constant)

    /* Key Array (6-Key Rollover) */
    0x05, 0x07,         //     USAGE_PAGE (Key Codes)
    0x19, 0x00,         //     USAGE_MINIMUM (0)
    0x29, 0x65,         //     USAGE_MAXIMUM (101)
    0x15, 0x00,         //     LOGICAL_MINIMUM (0)
    0x25, 0x65,         //     LOGICAL_MAXIMUM (101)
    0x75, 0x08,         //     REPORT_SIZE (8)
    0x95, 0x06,         //     REPORT_COUNT (6)
    0x81, 0x00,         //     INPUT (Data, Array) - Key array

    /* LED Output (Caps Lock, Num Lock, etc.) */
    0x05, 0x08,         //     Usage Page (LEDs)
    0x19, 0x01,         //     Usage Minimum (Num Lock)
    0x29, 0x05,         //     Usage Maximum (Kana)
    0x15, 0x00,         //     Logical Minimum (0)
    0x25, 0x01,         //     Logical Maximum (1)
    0x75, 0x01,         //     Report Size (1)
    0x95, 0x05,         //     Report Count (5)
    0x91, 0x02,         //     OUTPUT (Data, Variable, Absolute)

    /* Padding to make LED section byte-aligned */
    0x75, 0x03,         //     Report Size (3)
    0x95, 0x01,         //     Report Count (1)
    0x91, 0x01,         //     OUTPUT (Constant, Array, Absolute)

    0xc0                // End Collection
};

#define UX_HID_KEYBOARD_REPORT_LENGTH (sizeof(hid_keyboard_report)/sizeof(hid_keyboard_report[0]))

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
    UX_DEMO_HID_SUBCLASS,       /* bInterfaceSubClass : ... : Boot/non-boot Subclass */
    0x01,                       /* bInterfaceProtocol : 0x00 : Undefined */
    0x06,                       /* iInterface */

    /* HID Descriptor */
    0x09,                       /* bLength : 9 */
    0x21,                       /* bDescriptorType : 0x21 : HID descriptor */
    0x10, 0x01,                 /* bcdHID : 0x0110 */
    0x21,                       /* bCountryCode : 33 : US */
    0x01,                       /* bNumDescriptors */
    0x22,                       /* bReportDescriptorType1 : 0x22 : Report descriptor */
    UX_W0(UX_HID_KEYBOARD_REPORT_LENGTH), UX_W1(UX_HID_KEYBOARD_REPORT_LENGTH), /* wDescriptorLength1 */

    /* Endpoint Descriptor */
    0x07,                           /* bLength */
    0x05,                           /* bDescriptorType */
    UX_DEMO_HID_ENDPOINT_ADDRESS,   /* bEndpointAddress */
                                    /* D7, Direction : 0x01 */
                                    /* D3..0, Endpoint number : 2 */
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
    UX_DEMO_HID_SUBCLASS,       /* bInterfaceSubClass : ... : Boot/non-boot Subclass */
    0x01,                       /* bInterfaceProtocol : 0x00 : Undefined */
    0x06,                       /* iInterface */

    /* HID Descriptor */
    0x09,                       /* bLength : 9 */
    0x21,                       /* bDescriptorType : 0x21 : HID descriptor */
    UX_W0(UX_DEMO_BCD_HID), UX_W1(UX_DEMO_BCD_HID), /* bcdHID : 0x0110 */
    0x21,                       /* bCountryCode : 33 : US */
    0x01,                       /* bNumDescriptors */
    0x22,                       /* bReportDescriptorType1 : 0x22 : Report descriptor */
    UX_W0(UX_HID_KEYBOARD_REPORT_LENGTH), UX_W1(UX_HID_KEYBOARD_REPORT_LENGTH), /* wDescriptorLength1  */

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
    'H', 'I', 'D', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd', ' ', 'D', 'e', 'm', 'o',

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
    0x09, 0x04, 0x06, 8,
    'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd'
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

    ux_application_define();

    while (1)
    {
        ux_system_tasks_run();
        ux_demo_device_hid_task();
    }
}
#endif /* EXTERNAL_MAIN */

VOID ux_application_define(VOID)
{
CHAR                            *memory_pointer;
UINT                            status;
UX_SLAVE_CLASS_HID_PARAMETER    hid_keyboard_parameter;

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

    /* Initialize the hid keyboard class parameters for the device */
    hid_keyboard_parameter.ux_slave_class_hid_instance_activate         = ux_demo_device_hid_instance_activate;
    hid_keyboard_parameter.ux_slave_class_hid_instance_deactivate       = ux_demo_device_hid_instance_deactivate;
    hid_keyboard_parameter.ux_device_class_hid_parameter_report_address = hid_keyboard_report;
    hid_keyboard_parameter.ux_device_class_hid_parameter_report_length  = UX_HID_KEYBOARD_REPORT_LENGTH;
    hid_keyboard_parameter.ux_device_class_hid_parameter_report_id      = UX_FALSE;
    hid_keyboard_parameter.ux_device_class_hid_parameter_callback       = ux_demo_device_hid_callback;
    hid_keyboard_parameter.ux_device_class_hid_parameter_get_callback   = ux_demo_device_hid_get_callback;

    /* Initialize the device hid class. The class is connected with interface 0 on configuration 1. */
    status = ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                            1, 0, (VOID *)&hid_keyboard_parameter);

    if(status != UX_SUCCESS)
        return;

    /* Register error callback */
    ux_utility_error_callback_register(ux_demo_error_callback);

    /* Register the USB device controllers available in this system.  */
    usb_device_dcd_initialize(UX_NULL);
}

/********************************************************************/
/**  ux_demo_device_hid_instance_activate                           */
/********************************************************************/
VOID ux_demo_device_hid_instance_activate(VOID *hid_instance)
{
    if (hid_keyboard == UX_NULL)
        hid_keyboard = (UX_SLAVE_CLASS_HID*) hid_instance;
}

/********************************************************************/
/**  ux_demo_device_hid_instance_deactivate                         */
/********************************************************************/
VOID ux_demo_device_hid_instance_deactivate(VOID *hid_instance)
{
    if (hid_instance == (VOID *)hid_keyboard)
        hid_keyboard = UX_NULL;
}

/********************************************************************/
/**  ux_demo_device_hid_callback                                    */
/********************************************************************/
UINT ux_demo_device_hid_callback(UX_SLAVE_CLASS_HID *hid_instance, UX_SLAVE_CLASS_HID_EVENT *hid_event)
{
    UX_PARAMETER_NOT_USED(hid_instance);

    /* There was an event.  Analyze it.  Is it NUM LOCK ? */
    if ((hid_event -> ux_device_class_hid_event_buffer[0] & UX_HID_NUM_LOCK_MASK) &&
        (hid_event -> ux_device_class_hid_event_report_type == UX_DEVICE_CLASS_HID_REPORT_TYPE_OUTPUT))

        /* Set the Num lock flag.  */
        num_lock_flag = UX_TRUE;
    else
        /* Reset the Num lock flag.  */
        num_lock_flag = UX_FALSE;

    /* There was an event.  Analyze it.  Is it CAPS LOCK ? */
    if ((hid_event -> ux_device_class_hid_event_buffer[0] & UX_HID_CAPS_LOCK_MASK) &&
        (hid_event -> ux_device_class_hid_event_report_type == UX_DEVICE_CLASS_HID_REPORT_TYPE_OUTPUT))
        /* Set the Caps lock flag.  */
        caps_lock_flag = UX_TRUE;
    else
        /* Reset the Caps lock flag.  */
        caps_lock_flag = UX_FALSE;

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
/**  ux_demo_device_hid_task: hid demo task                         */
/********************************************************************/
VOID ux_demo_device_hid_task(VOID)
{
UCHAR           status;
static UCHAR    key = 0x04;

    /* Reset the HID event structure.  */
    ux_utility_memory_set(&device_hid_event, 0, sizeof(UX_SLAVE_CLASS_HID_EVENT));

    /* Check if the device state already configured.  */
    if ((UX_SLAVE_DEVICE_CHECK_STATE(UX_DEVICE_CONFIGURED)) && (hid_keyboard != UX_NULL))
    {
      if (key != 0)
      {
          /* delay for 10ms.  */
          demo_delay_with_tasks_running(10);

          /* Then insert a key into the keyboard event.  Length is fixed to 8.  */
          device_hid_event.ux_device_class_hid_event_report_id = 0;
          device_hid_event.ux_device_class_hid_event_report_type = UX_DEVICE_CLASS_HID_REPORT_TYPE_INPUT;
          device_hid_event.ux_device_class_hid_event_length = 8;
          device_hid_event.ux_device_class_hid_event_buffer[0] = 0;     /* 0x02: Left Shift modifier */
          device_hid_event.ux_device_class_hid_event_buffer[1] = 0;
          device_hid_event.ux_device_class_hid_event_buffer[2] = key;   /* key */
          device_hid_event.ux_device_class_hid_event_buffer[3] = 0;
          device_hid_event.ux_device_class_hid_event_buffer[4] = 0;
          device_hid_event.ux_device_class_hid_event_buffer[5] = 0;
          device_hid_event.ux_device_class_hid_event_buffer[6] = 0;
          device_hid_event.ux_device_class_hid_event_buffer[7] = 0;

          /* Set the keyboard event.  */
          status = ux_device_class_hid_event_set(hid_keyboard, &device_hid_event);

          if (status != UX_SUCCESS)
              return;

          /* Next event has the key depressed.  */
          device_hid_event.ux_device_class_hid_event_buffer[2] = 0;     /* 0x28: ENTER key */

          /* Set the keyboard event.  */
          status = ux_device_class_hid_event_set(hid_keyboard, &device_hid_event);

          if (status != UX_SUCCESS)
              return;

          /* Are we at the end of alphabet ?  */
          if (key != (0x04 + 25))
              /* Next key.  */
              key++;
          else
              key = 0;
      }
    }
}

/********************************************************************/
/**  demo_delay_with_tasks_running: delay with tasks               */
/********************************************************************/
static VOID demo_delay_with_tasks_running(ULONG ms_wait)
{
ULONG ticks;

    /* Get current time.  */
    ticks = ux_utility_time_get();

    /* Wait until timeout.  */
    while(ux_utility_time_elapsed(ticks, ux_utility_time_get()) < UX_MS_TO_TICK_NON_ZERO(ms_wait))
    {
        ux_system_tasks_run();
    }
}

static VOID ux_demo_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    /*
     * Refer to ux_api.h. For example,
     * UX_SYSTEM_LEVEL_INTERRUPT, UX_SYSTEM_CONTEXT_DCD, UX_DEVICE_HANDLE_UNKNOWN
     */
    printf("USBX error: system level(%d), context(%d), error code(0x%x)\r\n", system_level, system_context, error_code);
}
