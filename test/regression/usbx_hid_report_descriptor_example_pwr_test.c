/* This test ensures that the parser handles the example pwr report descriptor
   from the USB HID report descriptor tool. Why direct: the device's control transfer
   data buffer's default size (256 bytes) is not large enough for this report
   descriptor (over 800 bytes); thus, we reallocate the buffer with a larger size. */

#include "usbx_test_common_hid.h"
#include "ux_host_class_hid_keyboard.h"

static UCHAR hid_report_descriptor[] = {

    0x05, 0x84,                    // USAGE_PAGE (Power Device)
    0x09, 0x04,                    // USAGE (UPS)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x1e,                    //   USAGE (Flow)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x01,                    //     REPORT_ID (1)
    0x09, 0x1f,                    //     USAGE (FlowID)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //     LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x01,                    //     USAGE (iName)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x40,                    //     USAGE (ConfigVoltage)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x67, 0x21, 0xd1, 0xf0, 0x00,  //     UNIT (SI Lin:Volts)
    0x55, 0x07,                    //     UNIT_EXPONENT (7)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xfa, 0x00,              //     LOGICAL_MAXIMUM (250)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x42,                    //     USAGE (ConfigFrequency)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x66, 0x01, 0xf0,              //     UNIT (SI Lin:Hertz)
    0x55, 0x00,                    //     UNIT_EXPONENT (0)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x3c,                    //     LOGICAL_MAXIMUM (60)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x65, 0x00,                    //     UNIT (None)
    0xc0,                          //   END_COLLECTION
    0x09, 0x1e,                    //   USAGE (Flow)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x02,                    //     REPORT_ID (2)
    0x09, 0x1f,                    //     USAGE (FlowID)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //     LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x01,                    //     USAGE (iName)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x40,                    //     USAGE (ConfigVoltage)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x67, 0x21, 0xd1, 0xf0, 0x00,  //     UNIT (SI Lin:Volts)
    0x55, 0x05,                    //     UNIT_EXPONENT (5)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x27, 0xfe, 0xff, 0x00, 0x00,  //     LOGICAL_MAXIMUM (65534)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x42,                    //     USAGE (ConfigFrequency)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x66, 0x01, 0xf0,              //     UNIT (SI Lin:Hertz)
    0x55, 0x00,                    //     UNIT_EXPONENT (0)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x60,                    //     LOGICAL_MAXIMUM (96)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x65, 0x00,                    //     UNIT (None)
    0xc0,                          //   END_COLLECTION
    0x09, 0x1e,                    //   USAGE (Flow)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //     LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x01,                    //     USAGE (iName)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x40,                    //     USAGE (ConfigVoltage)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x67, 0x21, 0xd1, 0xf0, 0x00,  //     UNIT (SI Lin:Volts)
    0x55, 0x07,                    //     UNIT_EXPONENT (7)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xfa, 0x00,              //     LOGICAL_MAXIMUM (250)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x42,                    //     USAGE (ConfigFrequency)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x66, 0x01, 0xf0,              //     UNIT (SI Lin:Hertz)
    0x55, 0x00,                    //     UNIT_EXPONENT (0)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x3c,                    //     LOGICAL_MAXIMUM (60)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x43,                    //     USAGE (ConfigApparentPower)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x66, 0x21, 0xd1,              //     UNIT (SI Lin:Power)
    0x55, 0x07,                    //     UNIT_EXPONENT (7)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x27, 0xfe, 0xff, 0x00, 0x00,  //     LOGICAL_MAXIMUM (65534)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x65, 0x00,                    //     UNIT (None)
    0xc0,                          //   END_COLLECTION
    0x09, 0x10,                    //   USAGE (BatterySystem)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x04,                    //     REPORT_ID (4)
    0x09, 0x11,                    //     USAGE (BatterySystemID)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //     LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x1a,                    //     USAGE (Input)
    0xa1, 0x02,                    //     COLLECTION (Logical)
    0x09, 0x1b,                    //       USAGE (InputID)
    0x09, 0x1f,                    //       USAGE (FlowID)
    0x75, 0x04,                    //       REPORT_SIZE (4)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //       LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x09, 0x02,                    //       USAGE (PresentStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x09, 0x6d,                    //         USAGE (Used)
    0x09, 0x61,                    //         USAGE (Good)
    0x75, 0x01,                    //         REPORT_SIZE (1)
    0x95, 0x02,                    //         REPORT_COUNT (2)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0xb1, 0x03,                    //         FEATURE (Cnst,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0x09, 0x03,                    //       USAGE (ChangedStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x09, 0x6d,                    //         USAGE (Used)
    0x09, 0x61,                    //         USAGE (Good)
    0x75, 0x02,                    //         REPORT_SIZE (2)
    0x95, 0x02,                    //         REPORT_COUNT (2)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //         INPUT (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0xc0,                          //     END_COLLECTION
    0x09, 0x14,                    //     USAGE (Charger)
    0xa1, 0x02,                    //     COLLECTION (Logical)
    0x85, 0x05,                    //       REPORT_ID (5)
    0x09, 0x15,                    //       USAGE (ChargerID)
    0x75, 0x04,                    //       REPORT_SIZE (4)
    0x95, 0x01,                    //       REPORT_COUNT (1)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //       LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0x09, 0x1c,                    //     USAGE (Output)
    0xa1, 0x02,                    //     COLLECTION (Logical)
    0x09, 0x1d,                    //       USAGE (OutputID)
    0x09, 0x1f,                    //       USAGE (FlowID)
    0x75, 0x04,                    //       REPORT_SIZE (4)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //       LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0x09, 0x12,                    //     USAGE (Battery)
    0xa1, 0x02,                    //     COLLECTION (Logical)
    0x85, 0x06,                    //       REPORT_ID (6)
    0x09, 0x13,                    //       USAGE (BatteryID)
    0x85, 0x04,                    //       REPORT_ID (4)
    0x95, 0x01,                    //       REPORT_COUNT (1)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //       LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x05, 0x85,                    //       USAGE_PAGE (Battery System)
    0x09, 0x2c,                    //       USAGE (CapacityMode)
    0x75, 0x01,                    //       REPORT_SIZE (1)
    0x95, 0x01,                    //       REPORT_COUNT (1)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //       LOGICAL_MAXIMUM (1)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x75, 0x03,                    //       REPORT_SIZE (3)
    0x95, 0x01,                    //       REPORT_COUNT (1)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x09, 0x83,                    //       USAGE (DesignCapacity)
    0x75, 0x18,                    //       REPORT_SIZE (24)
    0x95, 0x01,                    //       REPORT_COUNT (1)
    0x67, 0x01, 0x10, 0x10, 0x00,  //       UNIT (SI Lin:Battery Capacity)
    0x55, 0x00,                    //       UNIT_EXPONENT (0)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x27, 0xfe, 0xff, 0xff, 0x00,  //       LOGICAL_MAXIMUM (16777214)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x05, 0x84,                    //       USAGE_PAGE (Power Device)
    0x09, 0x40,                    //       USAGE (ConfigVoltage)
    0x75, 0x10,                    //       REPORT_SIZE (16)
    0x95, 0x01,                    //       REPORT_COUNT (1)
    0x67, 0x21, 0xd1, 0xf0, 0x00,  //       UNIT (SI Lin:Volts)
    0x55, 0x05,                    //       UNIT_EXPONENT (5)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x27, 0xfe, 0xff, 0x00, 0x00,  //       LOGICAL_MAXIMUM (65534)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x05, 0x85,                    //       USAGE_PAGE (Battery System)
    0x09, 0x29,                    //       USAGE (RemainingCapacityLimit)
    0x75, 0x24,                    //       REPORT_SIZE (36)
    0x95, 0x01,                    //       REPORT_COUNT (1)
    0x67, 0x01, 0x10, 0x10, 0x00,  //       UNIT (SI Lin:Battery Capacity)
    0x55, 0x00,                    //       UNIT_EXPONENT (0)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x27, 0xfe, 0xff, 0xff, 0x00,  //       LOGICAL_MAXIMUM (16777214)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x65, 0x00,                    //       UNIT (None)
    0x05, 0x84,                    //       USAGE_PAGE (Power Device)
    0x09, 0x02,                    //       USAGE (PresentStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x05, 0x85,                    //         USAGE_PAGE (Battery System)
    0x0b, 0x61, 0x00, 0x84, 0x00,  //         USAGE (Power Device:Good)
    0x09, 0x42,                    //         USAGE (BelowRemainingCapacityLimit)
    0x09, 0x44,                    //         USAGE (Charging)
    0x09, 0x45,                    //         USAGE (Discharging)
    0x75, 0x01,                    //         REPORT_SIZE (1)
    0x95, 0x04,                    //         REPORT_COUNT (4)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0xb1, 0x02,                    //         FEATURE (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0x05, 0x84,                    //       USAGE_PAGE (Power Device)
    0x09, 0x03,                    //       USAGE (ChangedStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x05, 0x85,                    //         USAGE_PAGE (Battery System)
    0x0b, 0x61, 0x00, 0x84, 0x00,  //         USAGE (Power Device:Good)
    0x09, 0x42,                    //         USAGE (BelowRemainingCapacityLimit)
    0x09, 0x44,                    //         USAGE (Charging)
    0x09, 0x45,                    //         USAGE (Discharging)
    0x75, 0x02,                    //         REPORT_SIZE (2)
    0x95, 0x04,                    //         REPORT_COUNT (4)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //         INPUT (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0xc0,                          //     END_COLLECTION
    0xc0,                          //   END_COLLECTION
    0x05, 0x84,                    //   USAGE_PAGE (Power Device)
    0x09, 0x16,                    //   USAGE (PowerConverter)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x08,                    //     REPORT_ID (8)
    0x09, 0x17,                    //     USAGE (PowerConverterID)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //     LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x1a,                    //     USAGE (Input)
    0xa1, 0x02,                    //     COLLECTION (Logical)
    0x09, 0x1b,                    //       USAGE (InputID)
    0x09, 0x1f,                    //       USAGE (FlowID)
    0x75, 0x04,                    //       REPORT_SIZE (4)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //       LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x09, 0x02,                    //       USAGE (PresentStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x09, 0x6d,                    //         USAGE (Used)
    0x09, 0x61,                    //         USAGE (Good)
    0x75, 0x01,                    //         REPORT_SIZE (1)
    0x95, 0x02,                    //         REPORT_COUNT (2)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0xb1, 0x02,                    //         FEATURE (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0x09, 0x03,                    //       USAGE (ChangedStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x09, 0x6d,                    //         USAGE (Used)
    0x09, 0x61,                    //         USAGE (Good)
    0x75, 0x02,                    //         REPORT_SIZE (2)
    0x95, 0x02,                    //         REPORT_COUNT (2)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //         INPUT (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0xc0,                          //     END_COLLECTION
    0x09, 0x1c,                    //     USAGE (Output)
    0xa1, 0x02,                    //     COLLECTION (Logical)
    0x85, 0x09,                    //       REPORT_ID (9)
    0x09, 0x1d,                    //       USAGE (OutputID)
    0x09, 0x1f,                    //       USAGE (FlowID)
    0x75, 0x04,                    //       REPORT_SIZE (4)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //       LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x09, 0x35,                    //       USAGE (PercentLoad)
    0x75, 0x08,                    //       REPORT_SIZE (8)
    0x95, 0x01,                    //       REPORT_COUNT (1)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //       LOGICAL_MAXIMUM (255)
    0x81, 0x02,                    //       INPUT (Data,Var,Abs)
    0x09, 0x02,                    //       USAGE (PresentStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x09, 0x6d,                    //         USAGE (Used)
    0x09, 0x61,                    //         USAGE (Good)
    0x09, 0x64,                    //         USAGE (FrequencyOutOfRange)
    0x09, 0x69,                    //         USAGE (ShutdownImminent)
    0x75, 0x01,                    //         REPORT_SIZE (1)
    0x95, 0x04,                    //         REPORT_COUNT (4)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0xb1, 0x02,                    //         FEATURE (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0x09, 0x03,                    //       USAGE (ChangedStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x09, 0x6d,                    //         USAGE (Used)
    0x09, 0x61,                    //         USAGE (Good)
    0x09, 0x64,                    //         USAGE (FrequencyOutOfRange)
    0x09, 0x69,                    //         USAGE (ShutdownImminent)
    0x75, 0x02,                    //         REPORT_SIZE (2)
    0x95, 0x04,                    //         REPORT_COUNT (4)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //         INPUT (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0xc0,                          //     END_COLLECTION
    0x09, 0x1a,                    //     USAGE (Input)
    0xa1, 0x02,                    //     COLLECTION (Logical)
    0x85, 0x0a,                    //       REPORT_ID (10)
    0x09, 0x1b,                    //       USAGE (InputID)
    0x09, 0x1f,                    //       USAGE (FlowID)
    0x75, 0x04,                    //       REPORT_SIZE (4)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x15, 0x00,                    //       LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //       LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //       FEATURE (Cnst,Var,Abs)
    0x09, 0x02,                    //       USAGE (PresentStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x09, 0x6d,                    //         USAGE (Used)
    0x09, 0x61,                    //         USAGE (Good)
    0x75, 0x01,                    //         REPORT_SIZE (1)
    0x95, 0x02,                    //         REPORT_COUNT (2)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0xb1, 0x02,                    //         FEATURE (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0x09, 0x03,                    //       USAGE (ChangedStatus)
    0xa1, 0x02,                    //       COLLECTION (Logical)
    0x09, 0x6d,                    //         USAGE (Used)
    0x09, 0x61,                    //         USAGE (Good)
    0x75, 0x02,                    //         REPORT_SIZE (2)
    0x95, 0x02,                    //         REPORT_COUNT (2)
    0x15, 0x00,                    //         LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //         LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //         INPUT (Data,Var,Abs)
    0xc0,                          //       END_COLLECTION
    0xc0,                          //     END_COLLECTION
    0xc0,                          //   END_COLLECTION
    0x09, 0x24,                    //   USAGE (Sink)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x0b,                    //     REPORT_ID (11)
    0x09, 0x25,                    //     USAGE (SinkID)
    0x09, 0x1f,                    //     USAGE (FlowID)
    0x09, 0x1d,                    //     USAGE (OutputID)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //     LOGICAL_MAXIMUM (15)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0x09, 0x1b,                    //     USAGE (InputID)
    0x09, 0x13,                    //     USAGE (BatteryID)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x0f,                    //     LOGICAL_MAXIMUM (15)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x09, 0x35,                    //     USAGE (PercentLoad)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x05, 0x85,                    //     USAGE_PAGE (Battery System)
    0x09, 0x66,                    //     USAGE (RemainingCapacity)
    0x75, 0x18,                    //     REPORT_SIZE (24)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x67, 0x01, 0x10, 0x10, 0x00,  //     UNIT (SI Lin:Battery Capacity)
    0x55, 0x00,                    //     UNIT_EXPONENT (0)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x27, 0xfe, 0xff, 0xff, 0x00,  //     LOGICAL_MAXIMUM (16777214)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x65, 0x00,                    //     UNIT (None)
    0x09, 0x42,                    //     USAGE (BelowRemainingCapacityLimit)
    0x09, 0x44,                    //     USAGE (Charging)
    0x09, 0x45,                    //     USAGE (Discharging)
    0x0b, 0x64, 0x00, 0x84, 0x00,  //     USAGE (Power Device:FrequencyOutOfRange)
    0x0b, 0x69, 0x00, 0x84, 0x00,  //     USAGE (Power Device:ShutdownImminent)
    0x75, 0x02,                    //     REPORT_SIZE (2)
    0x95, 0x05,                    //     REPORT_COUNT (5)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};
#define HID_REPORT_LENGTH sizeof(hid_report_descriptor)/sizeof(hid_report_descriptor[0])


#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 52
static UCHAR device_framework_full_speed[DEVICE_FRAMEWORK_LENGTH_FULL_SPEED] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0x81, 0x0A, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x02, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00,

    /* HID descriptor */
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),
        MSB(HID_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 62
static UCHAR device_framework_high_speed[DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x02, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00,

    /* HID descriptor */
        0x09, 0x21, 0x10, 0x01, 0x21, 0x01, 0x22, LSB(HID_REPORT_LENGTH),
        MSB(HID_REPORT_LENGTH),

    /* Endpoint descriptor (Interrupt) */
        0x07, 0x05, 0x82, 0x03, 0x08, 0x00, 0x08

    };


    /* String Device Framework :
     Byte 0 and 1 : Word containing the language ID : 0x0904 for US
     Byte 2       : Byte containing the index of the descriptor
     Byte 3       : Byte containing the length of the descriptor string
    */

#define STRING_FRAMEWORK_LENGTH 40
static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0c,
        0x55, 0x53, 0x42, 0x20, 0x4b, 0x65, 0x79, 0x62,
        0x6f, 0x61, 0x72, 0x64,

    /* Serial Number string descriptor : Index 3 */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31
    };


    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
#define LANGUAGE_ID_FRAMEWORK_LENGTH 2
static UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
    };


static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    /* Failed test.  */
    printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
    test_control_return(1);
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_hid_report_descriptor_example_pwr_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;
UINT                            descriptor_size = HID_REPORT_LENGTH;
UX_SLAVE_TRANSFER *             slave_transfer_request;


    /* Inform user.  */
    printf("Running HID Report Descriptor Example pwr Test....................... ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    status =  ux_host_stack_class_register(_ux_system_host_class_hid_name, ux_host_class_hid_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the HID client(s).  */
    status =  ux_host_class_hid_client_register(_ux_system_host_class_hid_client_keyboard_name, ux_host_class_hid_keyboard_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                         device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                         string_framework, STRING_FRAMEWORK_LENGTH,
                                         language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH defined in ux_port.h is not sufficient for this
       report descriptor. We need to allocate a larger buffer. */

    slave_transfer_request = &_ux_system_slave -> ux_system_slave_device.ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

    ux_utility_memory_free(slave_transfer_request -> ux_slave_transfer_request_data_pointer);

    slave_transfer_request -> ux_slave_transfer_request_data_pointer = ux_utility_memory_allocate(UX_NO_ALIGN, UX_CACHE_SAFE_MEMORY, 1024);
    if(slave_transfer_request -> ux_slave_transfer_request_data_pointer == UX_NULL)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the hid class parameters.  */
    hid_parameter.ux_device_class_hid_parameter_report_address = hid_report_descriptor;
    hid_parameter.ux_device_class_hid_parameter_report_length  = HID_REPORT_LENGTH;
    hid_parameter.ux_device_class_hid_parameter_callback       = demo_thread_hid_callback;

    /* Initilize the device hid class. The class is connected with interface 2 */
    status =  ux_device_stack_class_register(_ux_system_slave_class_hid_name, ux_device_class_hid_entry,
                                                1,2, (VOID *)&hid_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }


    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }
}


static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                status;
UX_HOST_CLASS_HID_REPORT_GET_ID     report_id;


    /* Find the HID class */
    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Get the input report descriptor. */
    report_id.ux_host_class_hid_report_get_report = UX_NULL;
    report_id.ux_host_class_hid_report_get_type = UX_HOST_CLASS_HID_REPORT_TYPE_INPUT;
    status = ux_host_class_hid_report_id_get(hid, &report_id);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Do minimal error-checking. */
    if (report_id.ux_host_class_hid_report_get_report == UX_NULL)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Now disconnect the device.  */
    _ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_hid_name, ux_device_class_hid_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}

static UINT    demo_thread_hid_callback(UX_SLAVE_CLASS_HID *class, UX_SLAVE_CLASS_HID_EVENT *event)
{
    return(UX_SUCCESS);
}
