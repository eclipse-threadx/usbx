
## Overview

This example works as a USB HID device. It will appear as a USB mouse device on PC.

  - Move mouse arrow on the PC screen according in a rectangular fashion.
  - Scroll mouse wheel.
  - Click left middle right mouse buttons.

This application demo is running in **standalone** mode (without RTOS).

To test absolute mouse you need to define **UX_DEMO_MOUSE_ABSOLUTE** in **ux_demo_device_descriptors.h**

**Note:** 
- This demonstration is optimized.
- **DEMO_STACK_SIZE** and **UX_DEVICE_MEMORY_STACK_SIZE** memory size should be adjusted.

## USB Specification

- USB 2.0 Universal Serial BUS : https://www.usb.org/document-library/usb-20-specification
- USB HID Class : https://www.usb.org/document-library/device-class-definition-hid-111
- USB HID Usage Tables : https://usb.org/document-library/hid-usage-tables-16

## Report Descriptor

A HID (Human Interface Device) mouse report descriptor defines how data is sent from the device (mouse) to the host (computer). 
Below is a standard USB HID mouse report descriptor for a 3-button mouse with X and Y movement and wheel.

### Report Format Relative

This descriptor defines a 4-byte report format:

|Byte |	Bits  |	Usage |
| --- | ---   | ----- |
| 0	  | 0-2   | Buttons (Bit 0 = Left, Bit 1 = Right, Bit 2 = Middle) |
| 0	  | 3-7   | Padding (unused) |
| 1	  | 0-7   | X-axis movement (-127 to +127, relative) |
| 2	  | 0-7   | Y-axis movement (-127 to +127, relative) |

### Report Format Absolute
This descriptor defines a 6-byte report format:

|Byte |	Bits  |	Usage |
| --- | ---   | ----- |
| 0	  | 0-2	  | Buttons (Bit 0 = Left, Bit 1 = Right, Bit 2 = Middle) |
| 0	  | 3-7	  | Padding (unused) |
| 1-2 |	0-15  | X-axis Position (0 - 32767, absolute) |
| 3-4 |	0-15  | Y-axis Position (0 - 32767, absolute) |

## Run the example
- Create a project and include **USBX** and **THREADX** files.
- Customize **board_setup** with hardware init and **usb_device_interrupt_setup** with usb hardware init.
- Run Project.
- Plug-in the device, which is running HID mouse example, into the PC. A HID-compliant mouse is enumerated in the Device Manager.
