# USBX Demonstration

This Readme describes in detail USBX demonstration.

## Index

- Device:
  - HID (Human Interface Device Class)
    - [HID Mouse](#hid-mouse-rtos--standalone)


# HID Mouse (RTOS & Standalone)
  - [Overview](#hid-mouse-overview)
  - [Descriptors](#hid-mouse-descriptors)
  - [Initialization Flow](#hid-mouse-init-flow)
  - [User callback handlers](#hid-mouse-user-callback-handlers)
  - [Demo configuration](#hid-mouse-demo-configuration)
  - [Porting notes](#hid-mouse-porting-notes)
  - [Build and Run](#hid-mouse-build-and-run)

<a id="hid-mouse-overview"></a>
## Overview

- **Purpose:** Demonstrates a USB HID Mouse device implemented with Eclipse ThreadX USBX.
- **Variants:** [demo_device_hid_mouse_rtos.c](demo_device_hid_mouse_rtos.c) and [demo_device_hid_mouse_standalone.c](demo_device_hid_mouse_standalone.c).
- **Targets:** MCU platforms with a USB Device Controller (e.g., STM32H7). Platform glue is provided by your board layer via `board_setup()` and `usb_device_dcd_initialize()`.
- **Device role:** Enumerates as a HID Mouse on a host PC.
- **Movement demo:** Generates cursor movement automatically in a square pattern.
- **Optional absolute mode:** Define `UX_DEMO_MOUSE_ABSOLUTE` to switch from relative to absolute XY reporting.
- **Boot subclass:** Enabled by default (`DEMO_HID_BOOT_DEVICE`).

<a id="hid-mouse-descriptors"></a>
## USB Descriptor

### HID Report Descriptor:
  - Usage page/collection: Generic Desktop → Mouse → Pointer (Application + Physical collections).
  - Buttons: Usage min/max 1..3, logical 0..1, report size 1, count 3 → 3 button bits, followed by 5 padding bits.
  - X/Y axes:
    - Relative mode (default): logical −127..127, report size 8, count 2 → 1 byte X + 1 byte Y, both Relative.
    - Absolute mode (`UX_DEMO_MOUSE_ABSOLUTE`): logical 0..32767, report size 16, count 2 → 2 bytes X (LSB/MSB) + 2 bytes Y, Absolute.
  - Wheel: logical −127..127, report size 8, count 1 → 1 byte Wheel, Relative.
  - Resulting input report layout:
    - Relative: [Byte0] Buttons (3 bits) + 5 pad, [Byte1] X, [Byte2] Y, [Byte3] Wheel.
    - Absolute: [Byte0] Buttons (3 bits) + 5 pad, [Byte1..2] X (LSB,MSB), [Byte3..4] Y (LSB,MSB), [Byte5] Wheel.

<details><summary><b>Relative mode (default)</b></summary>

```c
/* hid_mouse_report[] (relative XY) */
0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
0x09, 0x02,       // USAGE (Mouse)
0xA1, 0x01,       // COLLECTION (Application)
  0x09, 0x01,     //   USAGE (Pointer)
  0xA1, 0x00,     //   COLLECTION (Physical)
    /* 3 buttons */
    0x05, 0x09,   //     USAGE_PAGE (Button)
    0x19, 0x01,   //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,   //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,   //     LOGICAL_MINIMUM (0)
    0x25, 0x01,   //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,   //     REPORT_SIZE (1)
    0x95, 0x03,   //     REPORT_COUNT (3) -> 3 buttons
    0x81, 0x02,   //     INPUT (Data,Var,Abs) -> Buttons
    /* padding to next byte */
    0x75, 0x05,   //     REPORT_SIZE (5)
    0x95, 0x01,   //     REPORT_COUNT (1)
    0x81, 0x03,   //     INPUT (Const,Var,Abs) -> Padding bits
    /* X, Y relative */
    0x05, 0x01,   //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,   //     USAGE (X)
    0x09, 0x31,   //     USAGE (Y)
    0x15, 0x81,   //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F,   //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,   //     REPORT_SIZE (8)
    0x95, 0x02,   //     REPORT_COUNT (2) -> X,Y
    0x81, 0x06,   //     INPUT (Data,Var,Rel) -> X,Y are relative
    /* Wheel */
    0x09, 0x38,   //     USAGE (Wheel)
    0x15, 0x81,   //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F,   //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,   //     REPORT_SIZE (8)
    0x95, 0x01,   //     REPORT_COUNT (1) -> Wheel
    0x81, 0x06,   //     INPUT (Data,Var,Rel) -> Wheel
  0xC0,           //   END_COLLECTION
0xC0              // END_COLLECTION
```
   </details>

<details><summary><b>Absolute mode (`UX_DEMO_MOUSE_ABSOLUTE` defined)</b></summary>

```c
/* hid_mouse_report[] (absolute XY) */
0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
0x09, 0x02,       // USAGE (Mouse)
0xA1, 0x01,       // COLLECTION (Application)
  0x09, 0x01,     //   USAGE (Pointer)
  0xA1, 0x00,     //   COLLECTION (Physical)
    /* 3 buttons */
    0x05, 0x09,   //     USAGE_PAGE (Button)
    0x19, 0x01,   //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,   //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,   //     LOGICAL MINIMUM (0)
    0x25, 0x01,   //     LOGICAL MAXIMUM (1)
    0x75, 0x01,   //     REPORT_SIZE (1)
    0x95, 0x03,   //     REPORT_COUNT (3)
    0x81, 0x02,   //     INPUT (Data,Var,Abs)
    /* padding */
    0x75, 0x05,   //     REPORT_SIZE (5)
    0x95, 0x01,   //     REPORT COUNT (1)
    0x81, 0x03,   //     INPUT (Const,Var,Abs)
    /* X, Y absolute (16-bit each) */
    0x05, 0x01,   //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,   //     USAGE (X)
    0x09, 0x31,   //     USAGE (Y)
    0x16, 0x00, 0x00, //  LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x7F, //  LOGICAL_MAXIMUM (32767)
    0x75, 0x10,   //     REPORT_SIZE (16)
    0x95, 0x02,   //     REPORT_COUNT (2) -> X,Y
    0x81, 0x02,   //     INPUT (Data,Var,Abs) -> Absolute X,Y
    /* Wheel */
    0x09, 0x38,   //     USAGE (Wheel)
    0x15, 0x81,   //     LOGICAL_MINIMUM (-127)
    0x25, 0x7F,   //     LOGICAL MAXIMUM (127)
    0x75, 0x08,   //     REPORT_SIZE (8)
    0x95, 0x01,   //     REPORT_COUNT (1)
    0x81, 0x06,   //     INPUT (Data,Var,Rel)
  0xC0,           //   END_COLLECTION
0xC0              // END_COLLECTION
```
   </details>


### USB Device/Configuration Descriptors:

  - Device descriptor:
    - `bcdUSB = 0x0200` (USB 2.0), class/subclass/protocol = 0 (interface-defined).
    - `bMaxPacketSize0 = 64`.
    - `idVendor = 0x090A`, `idProduct = 0x4036` (demo values), `iManufacturer = 1`, `iProduct = 2`, `iSerialNumber = 3`.
    - `bNumConfigurations = 1`.
  - Device qualifier descriptor (HS build): mirrors device characteristics for the other speed.
  - Configuration descriptor:
    - `wTotalLength = 0x22` (34 bytes across config + interface + HID + endpoint).
    - `bmAttributes = 0xC0` (self-powered), `bMaxPower = 0x32` (100 mA units of 2 mA).
    - `bNumInterfaces = 1`, `bConfigurationValue = 1`.
  - Interface descriptor:
    - `bInterfaceClass = 0x03` (HID), `bInterfaceSubClass = 0x01` (Boot if `DEMO_HID_BOOT_DEVICE`), `bInterfaceProtocol = 0x02` (Mouse).
    - `bNumEndpoints = 1` (INT IN).
  - HID descriptor:
    - `bcdHID = 0x0110` (HID 1.11), `bCountryCode = 33` (US), `bNumDescriptors = 1`.
    - Report descriptor type `0x22`, length matches `sizeof(hid_mouse_report)`.
  - Endpoint descriptor (Interrupt IN):
    - `bEndpointAddress = 0x81` (IN, EP1), `bmAttributes = 0x03` (Interrupt).
    - `wMaxPacketSize = 8` bytes.
    - `bInterval = 8` → FS: 8 ms; HS: 16 ms.

<details><summary><b>USB Device descriptor framework (FS)</b></summary>

```c
/* Device descriptor */
0x12,             // bLength
0x01,             // bDescriptorType (Device)
0x00, 0x02,       // bcdUSB (2.00)
0x00,             // bDeviceClass (interface-defined)
0x00,             // bDeviceSubClass
0x00,             // bDeviceProtocol
0x40,             // bMaxPacketSize0 (64)
0x0A, 0x09,       // idVendor  (0x090A)
0x36, 0x40,       // idProduct (0x4036)
0x00, 0x00,       // bcdDevice
0x01,             // iManufacturer
0x02,             // iProduct
0x03,             // iSerialNumber
0x01,             // bNumConfigurations

/* Configuration */
0x09,             // bLength
0x02,             // bDescriptorType (Configuration)
0x22, 0x00,       // wTotalLength (34)
0x01,             // bNumInterfaces
0x01,             // bConfigurationValue
0x04,             // iConfiguration
0xC0,             // bmAttributes (self-powered)
0x32,             // bMaxPower (100 mA)

/* Interface */
0x09,             // bLength
0x04,             // bDescriptorType (Interface)
0x00,             // bInterfaceNumber
0x00,             // bAlternateSetting
0x01,             // bNumEndpoints
0x03,             // bInterfaceClass (HID)
0x01,             // bInterfaceSubClass (Boot) if enabled
0x02,             // bInterfaceProtocol (Mouse)
0x06,             // iInterface

/* HID */
0x09,             // bLength
0x21,             // bDescriptorType (HID)
0x10, 0x01,       // bcdHID (1.10)
0x21,             // bCountryCode (US)
0x01,             // bNumDescriptors
0x22,             // bReportDescriptorType
/* wDescriptorLength (report size) inserted here: LSB,MSB at build time */
0x00, 0x00,

/* Endpoint (INT IN) */
0x07,             // bLength
0x05,             // bDescriptorType (Endpoint)
0x81,             // bEndpointAddress (IN, EP1)
0x03,             // bmAttributes (Interrupt)
0x08, 0x00,       // wMaxPacketSize (8)
0x08,             // bInterval (8)
```
   </details>

<details><summary><b>USB Device descriptor framework (HS)</b></summary>

```c
/* Device descriptor */
0x12,             // bLength
0x01,             // bDescriptorType (Device)
0x00, 0x02,       // bcdUSB (2.00)
0x00,             // bDeviceClass (interface-defined)
0x00,             // bDeviceSubClass
0x00,             // bDeviceProtocol
0x40,             // bMaxPacketSize0 (64)
0x0A, 0x09,       // idVendor  (0x090A)
0x36, 0x40,       // idProduct (0x4036)
0x00, 0x00,       // bcdDevice
0x01,             // iManufacturer
0x02,             // iProduct
0x03,             // iSerialNumber
0x01,             // bNumConfigurations

/* Device Qualifier */
0x0A,             // bLength
0x06,             // bDescriptorType (Device Qualifier)
0x00, 0x02,       // bcdUSB (2.00)
0x00,             // bDeviceClass (interface-defined)
0x00,             // bDeviceSubClass
0x00,             // bDeviceProtocol
0x40,             // bMaxPacketSize0 (64)
0x01,             // bNumConfigurations
0x00,             // bReserved

/* HS Configuration */
0x09,             // bLength
0x02,             // bDescriptorType (Configuration)
0x22, 0x00,       // wTotalLength (34)
0x01,             // bNumInterfaces
0x01,             // bConfigurationValue
0x05,             // iConfiguration (HS index)
0xC0,             // bmAttributes (self-powered)
0x19,             // bMaxPower (100 mA)

/* Interface */
0x09,             // bLength
0x04,             // bDescriptorType (Interface)
0x00,             // bInterfaceNumber
0x00,             // bAlternateSetting
0x01,             // bNumEndpoints
0x03,             // bInterfaceClass (HID)
0x01,             // bInterfaceSubClass (Boot) if enabled
0x02,             // bInterfaceProtocol (Mouse)
0x06,             // iInterface

/* HID */
0x09,             // bLength
0x21,             // bDescriptorType (HID)
0x10, 0x01,       // bcdHID (1.10)
0x21,             // bCountryCode (US)
0x01,             // bNumDescriptors
0x22,             // bReportDescriptorType
/* wDescriptorLength (report size) inserted by build: LSB,MSB */
0x00, 0x00,

/* Endpoint (INT IN) */
0x07,             // bLength
0x05,             // bDescriptorType (Endpoint)
0x81,             // bEndpointAddress (IN, EP1)
0x03,             // bmAttributes (Interrupt)
0x08, 0x00,       // wMaxPacketSize (8)
0x08,             // bInterval (8)
```

</details>

<a id="hid-mouse-init-flow"></a>
## Initialization Flow

### RTOS
- `board_setup()` → clocks, pins, cache, UART (optional), USB power.
- `tx_kernel_enter()` → ThreadX start.
- `tx_application_define()`:
  - `ux_system_initialize()` with a static pool (`UX_DEVICE_MEMORY_STACK_SIZE`, default 7 KB).
  - `ux_device_stack_initialize()` with HS/FS device + string frameworks.
  - HID class register: `_ux_system_slave_class_hid_name` → `ux_device_class_hid_entry` with callbacks.
  - Create demo thread → `ux_demo_device_hid_thread_entry()`.
- In thread:
  - `usb_device_dcd_initialize(UX_NULL)` to register the DCD.
  - When configured (`UX_DEVICE_CONFIGURED`) and instance ready, send events via `ux_device_class_hid_event_set()`.

### Standalone
- `board_setup()` → clocks, pins, cache, UART (optional), USB power.
- `ux_application_define()`:
  - Same USBX init + HID registration as RTOS variant.
  - `usb_device_dcd_initialize(UX_NULL)`.
- `main()` loop:
  - `ux_system_tasks_run()`; call `ux_demo_device_hid_task()` to emit HID events when configured.

<a id="hid-mouse-user-callback-handlers"></a>
## User Callback Handlers

- **`ux_demo_device_hid_instance_activate`:** Stores the class instance pointer on activation.
- **`ux_demo_device_hid_instance_deactivate`:** Clears the stored instance on deactivation to prevent sending after disconnect or configuration change.
- **`ux_demo_device_hid_callback`:** Handles host-to-device HID transfers (e.g., SET_REPORT for Output/Feature reports). The sample returns `UX_SUCCESS` without handling; adapt to parse `event->ux_device_class_hid_event_buffer` for custom outputs (LEDs, settings).
- **`ux_demo_device_hid_get_callback`:** Provides device-to-host data for GET_REPORT requests if your device supports Feature/Input fetches outside interrupt IN. Sample returns `UX_SUCCESS` without populating; implement to fill `event->ux_device_class_hid_event_buffer` and set `event->ux_device_class_hid_event_length` appropriately.
- **`ux_demo_error_callback`:** Registered via `ux_utility_error_callback_register()`. Use to log and diagnose stack/DCD errors; correlate `level/context/code` with constants in `ux_api.h`.

Notes:
- The sample sets `ux_device_class_hid_parameter_report_id = UX_FALSE` (single report, no Report ID byte). If you enable report IDs, ensure buffers account for the extra ID prefix.
- Keep callbacks non-blocking. For longer work, signal a worker thread/task (RTOS) or defer to your main loop (standalone).

<a id="hid-mouse-demo-configuration"></a>
## Demo configuration
- `UX_DEVICE_MEMORY_STACK_SIZE` (default 7*1024).
- `DEMO_HID_BOOT_DEVICE`: keep enabled for BIOS/UEFI compatibility.
- `UX_DEMO_HID_MOUSE_CURSOR_MOVE` and `_N`: tune speed/pattern.
- `UX_DEMO_MOUSE_ABSOLUTE`: switch to absolute XY; demo draws a rectangle.

### **Demo optimization**
- To optimize your application, user can flow this defines config in `ux_user.h`

```c
#define UX_DEVICE_ENDPOINT_BUFFER_OWNER    1
#define UX_DEVICE_CLASS_HID_ZERO_COPY

#define UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH         8
#define UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE            2

#define UX_DEVICE_ALTERNATE_SETTING_SUPPORT_DISABLE

#define UX_DEVICE_INITIALIZE_FRAMEWORK_SCAN_DISABLE
#define UX_MAX_DEVICE_ENDPOINTS                         1 /* Interrupt endpoint.  */
#define UX_MAX_DEVICE_INTERFACES                        1 /* HID interface.  */

#define UX_MAX_SLAVE_INTERFACES                         1
#define UX_MAX_SLAVE_CLASS_DRIVER                       1

#define UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH             64 /* > 62 for descriptors enumeration.  */
#define UX_SLAVE_REQUEST_DATA_MAX_LENGTH                8

#define UX_NAME_REFERENCED_BY_POINTER
```

<a id="hid-mouse-porting-notes"></a>
## Porting Notes

- Ensure your DCD driver and low-level BSP match your MCU and USB instance (FS/HS, ULPI/Embedded PHY).
- Provide proper NVIC priorities and ISR bindings for the DCD driver.

<a id="hid-mouse-build-and-run"></a>
## Build & Run

### **Build**
- Toolchain/IDE: IAR EWARM, Keil uVision, or Arm GCC + CMake/Ninja.
- Include ThreadX and USBX sources/ports for your Cortex-M core.
- Add one of the demo sources to your application and ensure:
  - `UX_DEVICE_SIDE_ONLY` (and `UX_STANDALONE` for the standalone variant).
  - Board layer implements `board_setup()` and `usb_device_dcd_initialize()`.

Example minimal main (RTOS variant is already self-contained in the sample): see [demo_device_hid_mouse_rtos.c](demo_device_hid_mouse_rtos.c).

### **Running the Demo Application**

The mouse demo does not require anything extra on the PC. You just need to plug the HID device running the mouse demo to the PC and see the screen cursor moving.

- On connection to a PC, the device enumerates as “HID Mouse Demo”.
- Cursor moves automatically. In absolute mode, it traces a rectangle.
- Wheel and buttons are included in the report, the sample does not press them by default.

# References
- USBX code repository: https://github.com/eclipse-threadx/usbx/
- USBX Documentation: https://github.com/eclipse-threadx/rtos-docs/blob/main/rtos-docs/usbx/overview-usbx.md
