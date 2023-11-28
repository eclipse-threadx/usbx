/* This test is designed to test the simple dpump host/device class operation.  */
/* Compile option requires:
  -DUX_HOST_CLASS_AUDIO_2_SUPPORT
  -DUX_HOST_CLASS_AUDIO_INTERRUPT_SUPPORT
  -DUX_SLAVE_REQUEST_CONTROL_MAX_LENGTH=512
*/

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"

#include "fx_api.h"

#include "ux_device_stack.h"
#include "ux_device_class_dummy.h"

#include "ux_host_class_audio.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"


/* Define constants.  */

#define                             UX_DEMO_REQUEST_MAX_LENGTH \
    ((UX_HCD_SIM_HOST_MAX_PAYLOAD) > (UX_SLAVE_REQUEST_DATA_MAX_LENGTH) ? \
        (UX_HCD_SIM_HOST_MAX_PAYLOAD) : (UX_SLAVE_REQUEST_DATA_MAX_LENGTH))

#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_DEMO_REQUEST_MAX_LENGTH + 1)
#define                             UX_DEMO_MEMORY_SIZE (128*1024)

#define                             UX_TEST_LOG_SIZE    (64)


/* Define local/extern function prototypes.  */
static void        test_thread_entry(ULONG);
static TX_THREAD   tx_test_thread_host_simulation;
static TX_THREAD   tx_test_thread_slave_simulation;
static void        tx_test_thread_host_simulation_entry(ULONG);
static void        tx_test_thread_slave_simulation_entry(ULONG);

static TX_EVENT_FLAGS_GROUP                     tx_test_events;

/* Define global data structures.  */
static UCHAR                                    usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UCHAR                                    buffer[64];

static UX_HOST_CLASS_AUDIO                      *host_audio_tx = UX_NULL;
static UX_HOST_CLASS_AUDIO                      *host_audio_rx = UX_NULL;

static UX_HOST_CLASS_AUDIO                      *audio = UX_NULL;

static UX_DEVICE_CLASS_DUMMY                    *dummy[3];
static UX_DEVICE_CLASS_DUMMY_PARAMETER          dummy_parameter;

static ULONG                               error_counter;

static ULONG                               set_cfg_counter;

static ULONG                               rsc_mem_free_on_set_cfg;
static ULONG                               rsc_sem_on_set_cfg;
static ULONG                               rsc_sem_get_on_set_cfg;
static ULONG                               rsc_mutex_on_set_cfg;

static ULONG                               rsc_mem_alloc_cnt_on_set_cfg;
static ULONG                               rsc_enum_sem_usage;
static ULONG                               rsc_enum_sem_get_count;
static ULONG                               rsc_enum_mutex_usage;
static ULONG                               rsc_enum_mem_usage;
static ULONG                               rsc_enum_mem_alloc_count;

static ULONG                               rsc_audio_sem_usage;
static ULONG                               rsc_audio_sem_get_count;
static ULONG                               rsc_audio_mutex_usage;
static ULONG                               rsc_audio_mem_usage;
static ULONG                               rsc_audio_mem_alloc_count;

static ULONG                               interaction_count;

static UCHAR                               error_callback_ignore = UX_TRUE;
static ULONG                               error_callback_counter;

/* Define device framework.  */

#define D3(d) ((UCHAR)((d) >> 24))
#define D2(d) ((UCHAR)((d) >> 16))
#define D1(d) ((UCHAR)((d) >> 8))
#define D0(d) ((UCHAR)((d) >> 0))

static unsigned char device_framework_full_speed[] = {

/* --------------------------------------- Device Descriptor */
/* 0  bLength, bDescriptorType                               */ 18,   0x01,
/* 2  bcdUSB                                                 */ D0(0x200),D1(0x200),
/* 4  bDeviceClass, bDeviceSubClass, bDeviceProtocol         */ 0x00, 0x00, 0x00,
/* 7  bMaxPacketSize0                                        */ 8,
/* 8  idVendor, idProduct                                    */ 0x84, 0x84, 0x02, 0x00,
/* 12 bcdDevice                                              */ D0(0x200),D1(0x200),
/* 14 iManufacturer, iProduct, iSerialNumber                 */ 0,    0,    0,
/* 17 bNumConfigurations                                     */ 1,

/* ----------------------------- Device Qualifier Descriptor *//* @18 */
/* 0 bLength, bDescriptorType                                */ 10,                 0x06,
/* 2 bcdUSB                                                  */ D0(0x200),D1(0x200),
/* 4 bDeviceClass, bDeviceSubClass, bDeviceProtocol          */ 0x00,               0x00, 0x00,
/* 7 bMaxPacketSize0                                         */ 8,
/* 8 bNumConfigurations                                      */ 1,
/* 9 bReserved                                               */ 0,

/* -------------------------------- Configuration Descriptor *//* 9+8+135+55+62=269 *//* @28 */
/* 0 bLength, bDescriptorType                                */ 9,    0x02,
/* 2 wTotalLength                                            */ D0(269),D1(269),
/* 4 bNumInterfaces, bConfigurationValue                     */ 3,    1,
/* 6 iConfiguration                                          */ 0,
/* 7 bmAttributes, bMaxPower                                 */ 0x80, 50,

/* ------------------------ Interface Association Descriptor *//* @37 */
/* 0 bLength, bDescriptorType                                */ 8,    0x0B,
/* 2 bFirstInterface, bInterfaceCount                        */ 0,    3,
/* 4 bFunctionClass, bFunctionSubClass, bFunctionProtocol    */ 0x01, 0x00, 0x20,
/* 7 iFunction                                               */ 0,

/* ------------------------------------ Interface Descriptor *//* 0 Control (9+126=135) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 0,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x01, 0x20,
/* 8 iInterface                                              */ 0,
/* ---------------- Audio 2.0 AC Interface Header Descriptor *//* (9+8+8+7+17*2+18*2+12*2=126) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 9,                   0x24, 0x01,
/* 3 bcdADC, bCategory                                       */ D0(0x200),D1(0x200), 0x08,
/* 6 wTotalLength                                            */ D0(126),D1(126),
/* 8 bmControls                                              */ 0x00,
/* -------------------- Audio 2.0 AC Clock Source Descriptor (0x11) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x24, 0x0A,
/* 3 bClockID, bmAttributes, bmControls                      */ 0x11, 0x05, 0x01,
/* 6 bAssocTerminal, iClockSource                            */ 0x00, 0,
/* -------------------- Audio 2.0 AC Clock Selector Descriptor (1x1, 0x12) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x24, 0x0B,
/* 3 bClockID, bNrInPins, baCSourceID1                       */ 0x12, 0x01, 0x11,
/* 6 bmControls, iClockSelector                              */ 0x01, 0,
/* -------------------- Audio 2.0 AC Clock Multiplier Descriptor (0x10) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 7,    0x24, 0x0C,
/* 3 bClockID, bCSourceID, bmControls                        */ 0x10, 0x12, 0x05,
/* 6 iClockMultiplier                                        */ 0,
/* ------------------- Audio 2.0 AC Input Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 17,   0x24,                   0x02,
/* 3  bTerminalID, wTerminalType                              */ 0x01, D0(0x0201),D1(0x0201),
/* 6  bAssocTerminal, bCSourceID                              */ 0x00, 0x10,
/* 8  bNrChannels, bmChannelConfig                            */ 0x02, D0(0),D1(0),D2(0),D3(0),
/* 13 iChannelNames, bmControls, iTerminal                    */ 0,    D0(0),D1(0),            0,
/* --------------------- Audio 2.0 AC Feature Unit Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 18,   0x24, 0x06,
/* 3  bUnitID, bSourceID                                      */ 0x02, 0x01,
/* 5  bmaControls(0), bmaControls(...) ...                    */ D0(0xF),D1(0xF),D2(0xF),D3(0xF), D0(0),D1(0),D2(0),D3(0), D0(0),D1(0),D2(0),D3(0),
/* .  iFeature                                                */ 0,
/* ------------------ Audio 2.0 AC Output Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 12,          0x24,                 0x03,
/* 3  bTerminalID, wTerminalType                              */ 0x03,        D0(0x0101),D1(0x0101),
/* 6  bAssocTerminal, bSourceID, bCSourceID                   */ 0x00,        0x02,                 0x10,
/* 9  bmControls, iTerminal                                   */ D0(0),D1(0), 0,
/* ------------------- Audio 2.0 AC Input Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 17,   0x24,                   0x02,
/* 3  bTerminalID, wTerminalType                              */ 0x04, D0(0x0101),D1(0x0101),
/* 6  bAssocTerminal, bCSourceID                              */ 0x00, 0x10,
/* 8  bNrChannels, bmChannelConfig                            */ 0x02, D0(0),D1(0),D2(0),D3(0),
/* 13 iChannelNames, bmControls, iTerminal                    */ 0,    D0(0),D1(0),            0,
/* --------------------- Audio 2.0 AC Feature Unit Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 18,   0x24, 0x06,
/* 3  bUnitID, bSourceID                                      */ 0x05, 0x04,
/* 5  bmaControls(0), bmaControls(...) ...                    */ D0(0xF),D1(0xF),D2(0xF),D3(0xF), D0(0),D1(0),D2(0),D3(0), D0(0),D1(0),D2(0),D3(0),
/* .  iFeature                                                */ 0,
/* ------------------ Audio 2.0 AC Output Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 12,          0x24,                 0x03,
/* 3  bTerminalID, wTerminalType                              */ 0x06,        D0(0x0301),D1(0x0301),
/* 6  bAssocTerminal, bSourceID, bCSourceID                   */ 0x00,        0x05,                 0x10,
/* 9  bmControls, iTerminal                                   */ D0(0),D1(0), 0,

/* ------------------------------------ Interface Descriptor *//* 1 Stream IN (9+9+16+6+7+8=55) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 1,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x20,
/* 8 iInterface                                              */ 0,
/* ------------------------------------ Interface Descriptor */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 1,    1,
/* 4 bNumEndpoints                                           */ 1,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x20,
/* 8 iInterface                                              */ 0,
/* ------------------------ Audio 2.0 AS Interface Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 16,  0x24, 0x01,
/* 3  bTerminalLink, bmControls                               */ 0x03,0x00,
/* 5  bFormatType, bmFormats                                  */ 0x01,D0(1),D1(1),D2(1),D3(1),
/* 10 bNrChannels, bmChannelConfig                            */ 2,   D0(0),D1(0),D2(0),D3(0),
/* 15 iChannelNames                                           */ 0,
/* -------------------- Audio 2.0 AS Format Type I Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 6,    0x24, 0x02,
/* 3  bFormatType, bSubslotSize, bBitResolution               */ 0x01, 2,    16,
/* ------------------------------------- Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x81,            0x0D,
/* 4  wMaxPacketSize, bInterval                               */ D0(256),D1(256), 1,
/* ---------- Audio 2.0 AS ISO Audio Data Endpoint Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x25,      0x01,
/* 3  bmAttributes, bmControls                                */ 0x00, 0x00,
/* 5  bLockDelayUnits, wLockDelay                             */ 0x00, D0(0),D1(0),

/* ------------------------------------ Interface Descriptor *//* 2 Stream OUT (9+9+16+6+7+8+7=62) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 2,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x20,
/* 8 iInterface                                              */ 0,
/* ------------------------------------ Interface Descriptor */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 2,    1,
/* 4 bNumEndpoints                                           */ 2,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x20,
/* 8 iInterface                                              */ 0,
/* ------------------------ Audio 2.0 AS Interface Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 16,  0x24, 0x01,
/* 3  bTerminalLink, bmControls                               */ 0x04,0x00,
/* 5  bFormatType, bmFormats                                  */ 0x01,D0(1),D1(1),D2(1),D3(1),
/* 10 bNrChannels, bmChannelConfig                            */ 2,   D0(0),D1(0),D2(0),D3(0),
/* 15 iChannelNames                                           */ 0,
/* ---------------------- Audio 2.0 AS Format Type Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 6,    0x24, 0x02,
/* 3  bFormatType, bSubslotSize, bBitResolution               */ 0x01, 2,    16,
/* ------------------------------------- Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x02,            0x0D,
/* 4  wMaxPacketSize, bInterval                               */ D0(256),D1(256), 1,
/* ---------- Audio 2.0 AS ISO Audio Data Endpoint Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x25,      0x01,
/* 3  bmAttributes, bmControls                                */ 0x00, 0x00,
/* 5  bLockDelayUnits, wLockDelay                             */ 0x00, D0(0),D1(0),
/* ------------------------------------- Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x82,            0x11,
/* 4  wMaxPacketSize, bInterval                               */ D0(4),D1(4), 1,

};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)
#define FRAMEWORK_POS_FULL_SPEED_IAD (37)

static unsigned char device_framework_full_speed_iad_audio10[] = {

/* --------------------------------------- Device Descriptor *//* @0 */
/* 0  bLength, bDescriptorType                               */ 18,   0x01,
/* 2  bcdUSB                                                 */ D0(0x200),D1(0x200),
/* 4  bDeviceClass, bDeviceSubClass, bDeviceProtocol         */ 0x00, 0x00, 0x00,
/* 7  bMaxPacketSize0                                        */ 0x08,
/* 8  idVendor, idProduct                                    */ 0x84, 0x84, 0x01, 0x00,
/* 12 bcdDevice                                              */ D0(0x100),D1(0x100),
/* 14 iManufacturer, iProduct, iSerialNumber                 */ 0,    0,    0,
/* 17 bNumConfigurations                                     */ 1,

/* -------------------------------- Configuration Descriptor *//* 9+8+88+52*2=209 *//* @18 */
/* 0 bLength, bDescriptorType                                */ 9,    0x02,
/* 2 wTotalLength                                            */ D0(209),D1(209),
/* 4 bNumInterfaces, bConfigurationValue                     */ 3,    1,
/* 6 iConfiguration                                          */ 0,
/* 7 bmAttributes, bMaxPower                                 */ 0x80, 50,

/* ------------------------ Interface Association Descriptor *//* @27 */
/* 0 bLength, bDescriptorType                                */ 8,    0x0B,
/* 2 bFirstInterface, bInterfaceCount                        */ 0,    3,
/* 4 bFunctionClass, bFunctionSubClass, bFunctionProtocol    */ 0x01, 0x01, 0x00,
/* 7 iFunction                                               */ 0,

/* ------------------------------------ Interface Descriptor *//* 0 Control (9+72+7=88) *//* @35 */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 0,    0,
/* 4 bNumEndpoints                                           */ 1,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x01, 0x00,
/* 8 iInterface                                              */ 0,
/* ---------------- Audio 1.0 AC Interface Header Descriptor *//* (10+12*2+10*2+9*2=72) *//* @44 */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 10,            0x24, 0x01,
/* 3 bcdADC                                                  */ 0x00,          0x01,
/* 5 wTotalLength, bInCollection                             */ D0(72),D1(72), 2,
/* 8 baInterfaceNr(1) ... baInterfaceNr(n)                   */ 1,             2,
/* ------------------- Audio 1.0 AC Input Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 12,   0x24,                 0x02,
/* 3  bTerminalID, wTerminalType                              */ 0x01, D0(0x0201),D1(0x0201),
/* 6  bAssocTerminal,                                         */ 0x00,
/* 7  bNrChannels, wChannelConfig                             */ 0x02, D0(0),D1(0),
/* 10 iChannelNames, iTerminal                                */ 0,    0,
/* --------------------- Audio 1.0 AC Feature Unit Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 10,   0x24, 0x06,
/* 3  bUnitID, bSourceID                                      */ 0x02, 0x01,
/* 5  bControlSize                                            */ 1,
/* 6  bmaControls(0) ... bmaControls(...) ...                 */ 0x00, 0x00, 0x00,
/* .  iFeature                                                */ 0,
/* ------------------ Audio 1.0 AC Output Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 9,    0x24,                 0x03,
/* 3  bTerminalID, wTerminalType                              */ 0x03, D0(0x0101),D1(0x0101),
/* 6  bAssocTerminal, bSourceID                               */ 0x00, 0x02,
/* 8  iTerminal                                               */ 0,
/* ------------------- Audio 1.0 AC Input Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 12,   0x24,                 0x02,
/* 3  bTerminalID, wTerminalType                              */ 0x04, D0(0x0101),D1(0x0101),
/* 6  bAssocTerminal,                                         */ 0x00,
/* 7  bNrChannels, wChannelConfig                             */ 0x02, D0(0),D1(0),
/* 10 iChannelNames, iTerminal                                */ 0,    0,
/* --------------------- Audio 1.0 AC Feature Unit Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 10,   0x24, 0x06,
/* 3  bUnitID, bSourceID                                      */ 0x05, 0x04,
/* 5  bControlSize                                            */ 1,
/* 6  bmaControls(0) ... bmaControls(...) ...                 */ 0x00, 0x00, 0x00,
/* .  iFeature                                                */ 0,
/* ------------------ Audio 1.0 AC Output Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 9,    0x24,                 0x03,
/* 3  bTerminalID, wTerminalType                              */ 0x06, D0(0x0301),D1(0x0301),
/* 6  bAssocTerminal, bSourceID                               */ 0x00, 0x05,
/* 8  iTerminal                                               */ 0,
/* --------------------- Audio 1.0 AC INT Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x83,            0x03,
/* 4  wMaxPacketSize, bInterval                               */ D0(8),D1(8),     1,

/* ------------------------------------ Interface Descriptor *//* 1 Stream IN (9+9+7+11+9+7=52) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 1,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x00,
/* 8 iInterface                                              */ 0,
/* ------------------------------------ Interface Descriptor */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 1,    1,
/* 4 bNumEndpoints                                           */ 1,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x00,
/* 8 iInterface                                              */ 0,
/* ------------------------ Audio 1.0 AS Interface Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 7,    0x24, 0x01,
/* 3  bTerminalLink                                           */ 0x03,
/* 4  bDelay, wFormatTag                                      */ 0x00, D0(0x0001),D1(0x0001),
/* -------------------------- Audio AS Format Type Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 11,   0x24, 0x02,
/* 3  bFormatType, bNrChannels, bSubframeSize, bBitResolution */ 0x01, 0x02, 0x02, 16,
/* 7  bSamFreqType (n), tSamFreq[1] ... tSamFreq[n]           */ 1,    D0(48000),D1(48000),D2(48000),
/* --------------------- Audio 1.0 AS ISO Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 9,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x81,            0x01,
/* 4  wMaxPacketSize, bInterval, bRefresh, bSynchAddress      */ D0(256),D1(256), 1,    0, 0,
/* ---------- Audio 1.0 AS ISO Audio Data Endpoint Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 7,                0x25, 0x01,
/* 3  bmAttributes                                            */ 0x00,
/* 5  bLockDelayUnits, wLockDelay                             */ 0x00, D0(0),D1(0),

/* ------------------------------------ Interface Descriptor *//* 2 Stream OUT (9+9+7+11+9+7=52) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 2,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x00,
/* 8 iInterface                                              */ 0,
/* ------------------------------------ Interface Descriptor */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 2,    1,
/* 4 bNumEndpoints                                           */ 1,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x00,
/* 8 iInterface                                              */ 0,
/* ------------------------ Audio 1.0 AS Interface Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 7,    0x24, 0x01,
/* 3  bTerminalLink                                           */ 0x04,
/* 4  bDelay, wFormatTag                                      */ 0x00, D0(0x0001),D1(0x0001),
/* -------------------------- Audio AS Format Type Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 11,   0x24, 0x02,
/* 3  bFormatType, bNrChannels, bSubframeSize, bBitResolution */ 0x01, 0x02, 0x02, 16,
/* 7  bSamFreqType (n), tSamFreq[1] ... tSamFreq[n]           */ 1,    D0(48000),D1(48000),D2(48000),
/* --------------------- Audio 1.0 AS ISO Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 9,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x02,            0x01,
/* 4  wMaxPacketSize, bInterval, bRefresh, bSynchAddress      */ D0(256),D1(256), 1,    0, 0,
/* ---------- Audio 1.0 AS ISO Audio Data Endpoint Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 7,                0x25, 0x01,
/* 3  bmAttributes                                            */ 0x00,
/* 5  bLockDelayUnits, wLockDelay                             */ 0x00, D0(0),D1(0),
};
#define FRAMEWORK_POS_FULL_SPEED_AC10 (44)
static UCHAR device_framework_modified[2048];

static unsigned char device_framework_high_speed[] = {
/* --------------------------------------- Device Descriptor */
/* 0  bLength, bDescriptorType                               */ 18,   0x01,
/* 2  bcdUSB                                                 */ D0(0x200),D1(0x200),
/* 4  bDeviceClass, bDeviceSubClass, bDeviceProtocol         */ 0x00, 0x00, 0x00,
/* 7  bMaxPacketSize0                                        */ 0x08,
/* 8  idVendor, idProduct                                    */ 0x84, 0x84, 0x02, 0x00,
/* 12 bcdDevice                                              */ D0(0x200),D1(0x200),
/* 14 iManufacturer, iProduct, iSerialNumber                 */ 0,    0,    0,
/* 17 bNumConfigurations                                     */ 1,

/* ----------------------------- Device Qualifier Descriptor */
/* 0 bLength, bDescriptorType                                */ 10,                 0x06,
/* 2 bcdUSB                                                  */ D0(0x200),D1(0x200),
/* 4 bDeviceClass, bDeviceSubClass, bDeviceProtocol          */ 0x00,               0x00, 0x00,
/* 7 bMaxPacketSize0                                         */ 8,
/* 8 bNumConfigurations                                      */ 1,
/* 9 bReserved                                               */ 0,

/* -------------------------------- Configuration Descriptor *//* 9+8+135+55+62=269 */
/* 0 bLength, bDescriptorType                                */ 9,    0x02,
/* 2 wTotalLength                                            */ D0(269),D1(269),
/* 4 bNumInterfaces, bConfigurationValue                     */ 3,    1,
/* 6 iConfiguration                                          */ 0,
/* 7 bmAttributes, bMaxPower                                 */ 0x80, 50,

/* ------------------------ Interface Association Descriptor */
/* 0 bLength, bDescriptorType                                */ 8,    0x0B,
/* 2 bFirstInterface, bInterfaceCount                        */ 0,    3,
/* 4 bFunctionClass, bFunctionSubClass, bFunctionProtocol    */ 0x01, 0x00, 0x20,
/* 7 iFunction                                               */ 0,

/* ------------------------------------ Interface Descriptor *//* 0 Control (9+126=135) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 0,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x01, 0x20,
/* 8 iInterface                                              */ 0,
/* ---------------- Audio 2.0 AC Interface Header Descriptor *//* (9+8+8+7+17*2+18*2+12*2=126) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 9,                   0x24, 0x01,
/* 3 bcdADC, bCategory                                       */ D0(0x200),D1(0x200), 0x08,
/* 6 wTotalLength                                            */ D0(126),D1(126),
/* 8 bmControls                                              */ 0x00,
/* -------------------- Audio 2.0 AC Clock Source Descriptor (0x11) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x24, 0x0A,
/* 3 bClockID, bmAttributes, bmControls                      */ 0x11, 0x05, 0x01,
/* 6 bAssocTerminal, iClockSource                            */ 0x00, 0,
/* -------------------- Audio 2.0 AC Clock Selector Descriptor (1x1, 0x12) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x24, 0x0B,
/* 3 bClockID, bNrInPins, baCSourceID1                       */ 0x12, 0x01, 0x11,
/* 6 bmControls, iClockSelector                              */ 0x01, 0,
/* -------------------- Audio 2.0 AC Clock Multiplier Descriptor (0x10) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 7,    0x24, 0x0C,
/* 3 bClockID, bCSourceID, bmControls                        */ 0x10, 0x12, 0x05,
/* 6 iClockMultiplier                                        */ 0,
/* ------------------- Audio 2.0 AC Input Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 17,   0x24,                   0x02,
/* 3  bTerminalID, wTerminalType                              */ 0x01, D0(0x0201),D1(0x0201),
/* 6  bAssocTerminal, bCSourceID                              */ 0x00, 0x10,
/* 8  bNrChannels, bmChannelConfig                            */ 0x02, D0(0),D1(0),D2(0),D3(0),
/* 13 iChannelNames, bmControls, iTerminal                    */ 0,    D0(0),D1(0),            0,
/* --------------------- Audio 2.0 AC Feature Unit Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 18,   0x24, 0x06,
/* 3  bUnitID, bSourceID                                      */ 0x02, 0x01,
/* 5  bmaControls(0), bmaControls(...) ...                    */ D0(0xF),D1(0xF),D2(0xF),D3(0xF), D0(0),D1(0),D2(0),D3(0), D0(0),D1(0),D2(0),D3(0),
/* .  iFeature                                                */ 0,
/* ------------------ Audio 2.0 AC Output Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 12,          0x24,                 0x03,
/* 3  bTerminalID, wTerminalType                              */ 0x03,        D0(0x0101),D1(0x0101),
/* 6  bAssocTerminal, bSourceID, bCSourceID                   */ 0x00,        0x02,                 0x10,
/* 9  bmControls, iTerminal                                   */ D0(0),D1(0), 0,
/* ------------------- Audio 2.0 AC Input Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 17,   0x24,                   0x02,
/* 3  bTerminalID, wTerminalType                              */ 0x04, D0(0x0101),D1(0x0101),
/* 6  bAssocTerminal, bCSourceID                              */ 0x00, 0x10,
/* 8  bNrChannels, bmChannelConfig                            */ 0x02, D0(0),D1(0),D2(0),D3(0),
/* 13 iChannelNames, bmControls, iTerminal                    */ 0,    D0(0),D1(0),            0,
/* --------------------- Audio 2.0 AC Feature Unit Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 18,   0x24, 0x06,
/* 3  bUnitID, bSourceID                                      */ 0x05, 0x04,
/* 5  bmaControls(0), bmaControls(...) ...                    */ D0(0xF),D1(0xF),D2(0xF),D3(0xF), D0(0),D1(0),D2(0),D3(0), D0(0),D1(0),D2(0),D3(0),
/* .  iFeature                                                */ 0,
/* ------------------ Audio 2.0 AC Output Terminal Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 12,          0x24,                 0x03,
/* 3  bTerminalID, wTerminalType                              */ 0x06,        D0(0x0301),D1(0x0301),
/* 6  bAssocTerminal, bSourceID, bCSourceID                   */ 0x00,        0x05,                 0x10,
/* 9  bmControls, iTerminal                                   */ D0(0),D1(0), 0,

/* ------------------------------------ Interface Descriptor *//* 1 Stream IN (9+9+16+6+7+8=55) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 1,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x20,
/* 8 iInterface                                              */ 0,
/* ------------------------------------ Interface Descriptor */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 1,    1,
/* 4 bNumEndpoints                                           */ 1,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x20,
/* 8 iInterface                                              */ 0,
/* ------------------------ Audio 2.0 AS Interface Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 16,  0x24, 0x01,
/* 3  bTerminalLink, bmControls                               */ 0x03,0x00,
/* 5  bFormatType, bmFormats                                  */ 0x01,D0(1),D1(1),D2(1),D3(1),
/* 10 bNrChannels, bmChannelConfig                            */ 2,   D0(0),D1(0),D2(0),D3(0),
/* 15 iChannelNames                                           */ 0,
/* -------------------- Audio 2.0 AS Format Type I Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 6,    0x24, 0x02,
/* 3  bFormatType, bSubslotSize, bBitResolution               */ 0x01, 2,    16,
/* ------------------------------------- Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x81,            0x0D,
/* 4  wMaxPacketSize, bInterval                               */ D0(256),D1(256), 4,
/* ---------- Audio 2.0 AS ISO Audio Data Endpoint Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x25,      0x01,
/* 3  bmAttributes, bmControls                                */ 0x00, 0x00,
/* 5  bLockDelayUnits, wLockDelay                             */ 0x00, D0(0),D1(0),

/* ------------------------------------ Interface Descriptor *//* 2 Stream OUT (9+9+16+6+7+8+7=62) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 2,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x20,
/* 8 iInterface                                              */ 0,
/* ------------------------------------ Interface Descriptor */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 2,    1,
/* 4 bNumEndpoints                                           */ 2,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x02, 0x20,
/* 8 iInterface                                              */ 0,
/* ------------------------ Audio 2.0 AS Interface Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 16,  0x24, 0x01,
/* 3  bTerminalLink, bmControls                               */ 0x04,0x00,
/* 5  bFormatType, bmFormats                                  */ 0x01,D0(1),D1(1),D2(1),D3(1),
/* 10 bNrChannels, bmChannelConfig                            */ 2,   D0(0),D1(0),D2(0),D3(0),
/* 15 iChannelNames                                           */ 0,
/* ---------------------- Audio 2.0 AS Format Type Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 6,    0x24, 0x02,
/* 3  bFormatType, bSubslotSize, bBitResolution               */ 0x01, 2,    16,
/* ------------------------------------- Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x02,            0x0D,
/* 4  wMaxPacketSize, bInterval                               */ D0(256),D1(256), 4,
/* ---------- Audio 2.0 AS ISO Audio Data Endpoint Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x25,      0x01,
/* 3  bmAttributes, bmControls                                */ 0x00, 0x00,
/* 5  bLockDelayUnits, wLockDelay                             */ 0x00, D0(0),D1(0),
/* ------------------------------------- Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x82,            0x11,
/* 4  wMaxPacketSize, bInterval                               */ D0(4),D1(4), 1,

};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)

static unsigned char string_framework[] = {

/* Manufacturer string descriptor : Index 1 - "Express Logic" */
    0x09, 0x04, 0x01, 0x0c,
    0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
    0x6f, 0x67, 0x69, 0x63,

/* Product string descriptor : Index 2 - "EL Composite device" */
    0x09, 0x04, 0x02, 0x13,
    0x45, 0x4c, 0x20, 0x43, 0x6f, 0x6d, 0x70, 0x6f,
    0x73, 0x69, 0x74, 0x65, 0x20, 0x64, 0x65, 0x76,
    0x69, 0x63, 0x65,

/* Serial Number string descriptor : Index 3 - "0001" */
    0x09, 0x04, 0x03, 0x04,
    0x30, 0x30, 0x30, 0x31
};
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)


/* Multiple languages are supported on the device, to add
    a language besides English, the Unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
static unsigned char language_id_framework[] = {

/* English. */
    0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)

/* Setup requests */

static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;
static UX_TEST_SETUP _GetCfgDescr  = UX_TEST_SETUP_GetCfgDescr;
static UX_TEST_SETUP _SetAddress = UX_TEST_SETUP_SetAddress;
static UX_TEST_SETUP _GetDeviceDescriptor = UX_TEST_SETUP_GetDevDescr;
static UX_TEST_SETUP _GetConfigDescriptor = UX_TEST_SETUP_GetCfgDescr;

/* Interaction define */

/* Hooks define */


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;
    printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            test_control_return(1);
        }
    }
}

static UINT  sleep_break_on_error(VOID)
{

    if (error_callback_counter >= 3)
        return error_callback_counter;

    return UX_SUCCESS;
}

/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_AUDIO *audio = (UX_HOST_CLASS_AUDIO *) inst;

    // printf("CHG:%lx,%p,%p\n", event, cls, inst);
    switch(event)
    {

        case UX_DEVICE_INSERTION:

            // printf("hINS:%p,%p:%lx\n", cls, inst, ux_host_class_audio_type_get(audio));
            if (ux_host_class_audio_subclass_get(audio) != UX_HOST_CLASS_AUDIO_CLASS)
            {
                if (ux_host_class_audio_type_get(audio) == UX_HOST_CLASS_AUDIO_INPUT)
                    host_audio_rx = audio;
                else
                    host_audio_tx = audio;
            }
            break;

        case UX_DEVICE_REMOVAL:

            // printf("hRMV:%p,%p:%lx\n", cls, inst, ux_host_class_audio_type_get(audio));
            if (ux_host_class_audio_subclass_get(audio) != UX_HOST_CLASS_AUDIO_CLASS)
            {
                if (audio == host_audio_rx)
                    host_audio_rx = UX_NULL;
                if (audio == host_audio_tx)
                    host_audio_tx = UX_NULL;
            }
            break;

        default:
            break;
    }
    return 0;
}


static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params)
{

    set_cfg_counter ++;

    rsc_mem_alloc_cnt_on_set_cfg = ux_test_utility_sim_mem_alloc_count();

    rsc_mem_free_on_set_cfg = ux_test_regular_memory_free();
    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}
static UX_TEST_HCD_SIM_ACTION log_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_set_cfg,
        UX_TRUE}, /* Invoke callback & continue */
{   0   }
};


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_msrc_81184_host_audio_desc_validate_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    /* Inform user.  */
    printf("Running Audio 2.0 Host Basic Functionality Test..................... ");

#if !UX_TEST_MULTI_IFC_ON || !UX_TEST_MULTI_ALT_ON || !UX_TEST_MULTI_CLS_ON || \
    !defined(UX_HOST_CLASS_AUDIO_2_SUPPORT)                               || \
    !defined(UX_HOST_CLASS_AUDIO_INTERRUPT_SUPPORT)                         || \
    (UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH < 260)
    printf("SKIP SUCCESS!\n");
    test_control_return(0);
    return;
#endif

    stepinfo("\n");

    /* Reset testing counts. */
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_host_change_function);
    UX_TEST_CHECK_SUCCESS(status);

    /* Register Audio class.  */
    status  = ux_host_stack_class_register(_ux_system_host_class_audio_name, ux_host_class_audio_entry);
    UX_TEST_CHECK_SUCCESS(status);

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    UX_TEST_CHECK_SUCCESS(status);

    /* Set the parameters for DUMMY.  */
    ux_utility_memory_set(&dummy_parameter, 0, sizeof(UX_DEVICE_CLASS_DUMMY_PARAMETER));

    /* Initialize the device DUMMY class. This class owns interfaces starting with 0, 1, 2. */
    status  = ux_device_stack_class_register(_ux_device_class_dummy_name, _ux_device_class_dummy_entry,
                                             1, 0,  &dummy_parameter);
    UX_TEST_CHECK_SUCCESS(status);

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();
    UX_TEST_CHECK_SUCCESS(status);

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Create the main event group.  */
    status = tx_event_flags_create(&tx_test_events, "tx_test_events");
    UX_TEST_CHECK_SUCCESS(status);

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx demo host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_CHECK_SUCCESS(status);

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx demo slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_CHECK_SUCCESS(status);
}

static UINT test_wait_until_expected(VOID **ptr, ULONG loop, VOID *expected)
{
    while(loop)
    {
        _ux_utility_delay_ms(10);
        if (*ptr == expected)
            return UX_SUCCESS;
    }
    return UX_ERROR;
}
static UINT test_wait_until_not_expected(VOID **ptr, ULONG loop, VOID *expected)
{
    while(loop)
    {
        _ux_utility_delay_ms(10);
        if (*ptr != expected)
            return UX_SUCCESS;
    }
    return UX_ERROR;
}
#define test_wait_until_not_null(ptr, loop) test_wait_until_not_expected(ptr, loop, UX_NULL)
#define test_wait_until_null(ptr, loop) test_wait_until_expected(ptr, loop, UX_NULL)

static void _memory_tests(void)
{
ULONG                                               test_n;
ULONG                                               mem_free;

    /* Test disconnect. */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();

    /* Reset testing counts. */
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_hcd_sim_host_set_actions(log_on_SetCfg);
    /* Save free memory usage. */
    mem_free = ux_test_regular_memory_free();
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    /* Log create counts for further tests. */
    rsc_enum_mutex_usage = rsc_mutex_on_set_cfg;
    rsc_enum_sem_usage = rsc_sem_on_set_cfg;
    rsc_enum_mem_usage = mem_free - rsc_mem_free_on_set_cfg;
    rsc_enum_mem_alloc_count = rsc_mem_alloc_cnt_on_set_cfg;
    /* Log create counts when instances active for further tests. */
    rsc_audio_mutex_usage = ux_test_utility_sim_mutex_create_count() - rsc_enum_mutex_usage;
    rsc_audio_sem_usage = ux_test_utility_sim_sem_create_count() - rsc_enum_sem_usage;
    rsc_audio_mem_usage = rsc_mem_free_on_set_cfg - ux_test_regular_memory_free();
    rsc_audio_mem_alloc_count = ux_test_utility_sim_mem_alloc_count() - rsc_enum_mem_alloc_count;
    stepinfo("mem free: %ld\n", ux_test_regular_memory_free());

    /* Lock log base for tests. */
    ux_test_utility_sim_mem_alloc_log_lock();

    stepinfo("enum mem: %ld\n", rsc_enum_mem_alloc_count);
    stepinfo("cdc mem : %ld\n", rsc_audio_mem_alloc_count);
    stepinfo("mem free: %ld, %ld\n", ux_test_regular_memory_free(), _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available);

    /* Simulate detach and attach for FS enumeration,
       and check if there is memory error in normal enumeration.
     */
    stepinfo(">>>>>>>>>>>> Enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < 3; test_n++)
    {
        stepinfo("%4ld / 2\n", test_n);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = ux_test_regular_memory_free();
        else if (mem_free != ux_test_regular_memory_free())
        {

            printf("ERROR #11.%ld: Memory level different after re-enumerations %ld <> %ld\n", test_n, mem_free, ux_test_regular_memory_free());
            test_control_return(1);
        }

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on error. */
        ux_test_breakable_sleep(100, sleep_break_on_error);

        /* Check */
        if (!host_audio_tx || !host_audio_rx)
        {

            printf("ERROR #12.%ld: Enumeration fail\n", test_n);
            test_control_return(1);
        }
    }

    /* Simulate detach and attach for FS enumeration,
       and test possible memory allocation error handlings.
     */
    if (rsc_audio_mem_alloc_count) stepinfo(">>>>>>>>>>>> Memory errors enumeration test\n");
    mem_free = (~0);
    for (test_n = 0; test_n < rsc_audio_mem_alloc_count; test_n ++)
    {

        stepinfo("%4ld / %4ld\n", test_n, rsc_audio_mem_alloc_count - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Update memory free level (disconnect) */
        if (mem_free == (~0))
            mem_free = ux_test_regular_memory_free();
        else if (mem_free != ux_test_regular_memory_free())
        {

            printf("ERROR #11.%ld: Memory level different after re-enumerations %ld <> %ld\n", test_n, mem_free, ux_test_regular_memory_free());
            test_control_return(1);
        }

        /* Set memory error generation */
        ux_test_utility_sim_mem_alloc_error_generation_start(test_n + rsc_enum_mem_alloc_count);

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Wait and break on errors. */
        ux_test_breakable_sleep(100, sleep_break_on_error);

        /* Check error */
        if (host_audio_tx && host_audio_rx)
        {

            printf("ERROR #12.%ld: device detected when there is memory error\n", test_n);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", ux_test_regular_memory_free());
    }
    ux_test_utility_sim_mem_alloc_error_generation_stop();
    if (rsc_audio_mem_alloc_count) stepinfo("\n");
}

static void _audio_desc_tests(void)
{
ULONG mem_free;

    /* Disconnect. */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    UX_TEST_ASSERT(host_audio_rx == UX_NULL && host_audio_tx == UX_NULL);

    /* Set framework.  */
    _ux_system_slave -> ux_system_slave_device_framework_full_speed = device_framework_modified;

    stepinfo(">>>>>>>>>> Fake descriptor tests - IAD::bInterfaceCount\n");

    /* Prepare descriptor IAD.  */
    _ux_utility_memory_copy(device_framework_modified, device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED);
    UX_TEST_ASSERT(device_framework_modified[FRAMEWORK_POS_FULL_SPEED_IAD] == 0x08);
    UX_TEST_ASSERT(device_framework_modified[FRAMEWORK_POS_FULL_SPEED_IAD + 1] == 0x0B);
    UX_TEST_ASSERT(device_framework_modified[FRAMEWORK_POS_FULL_SPEED_IAD + 3] >= 0x02);/* bInterfaceCount  */

    /* Log memory.  */
    mem_free = ux_test_regular_memory_free();

    stepinfo(">>>>>>>>>> Fake descriptor tests - IAD::bInterfaceCount = 0\n");
    device_framework_modified[FRAMEWORK_POS_FULL_SPEED_IAD + 3] = 0;
    /* Connect. */
    error_callback_counter = 0;
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    /* Wait and break on error. */
    ux_test_breakable_sleep(100, sleep_break_on_error);
    /* Check connection.  */
    UX_TEST_ASSERT(host_audio_rx == UX_NULL && host_audio_tx == UX_NULL);
    /* Disconnect. */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    /* Check memory.  */
    UX_TEST_ASSERT(mem_free == ux_test_regular_memory_free());

    stepinfo(">>>>>>>>>> Fake descriptor tests - IAD::bInterfaceCount = 1\n");
    device_framework_modified[FRAMEWORK_POS_FULL_SPEED_IAD + 3] = 1;
    /* Connect. */
    error_callback_counter = 0;
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    /* Wait and break on error. */
    ux_test_breakable_sleep(100, sleep_break_on_error);
    /* Check connection.  */
    UX_TEST_ASSERT(host_audio_rx == UX_NULL && host_audio_tx == UX_NULL);
    /* Disconnect. */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    /* Check memory.  */
    UX_TEST_ASSERT(mem_free == ux_test_regular_memory_free());

    /* Prepare descriptor AC Header.  */
    _ux_utility_memory_copy(device_framework_modified, device_framework_full_speed_iad_audio10, sizeof(device_framework_full_speed_iad_audio10));
    UX_TEST_ASSERT(device_framework_modified[FRAMEWORK_POS_FULL_SPEED_AC10] == 0x0a); /* bLength */
    UX_TEST_ASSERT(device_framework_modified[FRAMEWORK_POS_FULL_SPEED_AC10 + 1] == 0x24);
    UX_TEST_ASSERT(device_framework_modified[FRAMEWORK_POS_FULL_SPEED_AC10 + 7] == 0x02);/* bInCollection  */

    stepinfo(">>>>>>>>>> Fake descriptor tests - AC_Header::bInCollection = 0\n");
    device_framework_modified[FRAMEWORK_POS_FULL_SPEED_AC10 + 7] = 0;
    /* Connect. */
    error_callback_counter = 0;
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    /* Wait and break on error. */
    ux_test_breakable_sleep(100, sleep_break_on_error);
    /* Check connection.  */
    UX_TEST_ASSERT(host_audio_rx == UX_NULL && host_audio_tx == UX_NULL);
    /* Disconnect. */
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    /* Check memory.  */
    UX_TEST_ASSERT(mem_free == ux_test_regular_memory_free());

    /* Set framework back.  */
    _ux_system_slave -> ux_system_slave_device_framework_full_speed = device_framework_full_speed;
}

void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               test_n;
UX_DEVICE                                           *device;
UX_CONFIGURATION                                    *configuration;
UX_INTERFACE                                        *interface;
UCHAR                                               test_tmp[32];
ULONG                                               temp;

    /* Test connect.  */
    status  = test_wait_until_not_null((void**)&host_audio_rx, 100);
    status |= test_wait_until_not_null((void**)&host_audio_tx, 100);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(ux_host_class_audio_protocol_get(host_audio_rx) == UX_HOST_CLASS_AUDIO_PROTOCOL_IP_VERSION_02_00);
    UX_TEST_ASSERT(ux_host_class_audio_type_get(host_audio_rx) == UX_HOST_CLASS_AUDIO_INPUT);
    UX_TEST_ASSERT(ux_host_class_audio_speed_get(host_audio_rx) == UX_FULL_SPEED_DEVICE);

    _audio_desc_tests();
    _memory_tests();

    /* Wait pending threads.  */
    _ux_utility_thread_sleep(1);

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    ux_device_stack_class_unregister(_ux_device_class_dummy_name, _ux_device_class_dummy_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}



void  tx_test_thread_slave_simulation_entry(ULONG arg)
{
    while(1)
    {

#if UX_DEMO_FEEDBACK
        UINT    status;
        ULONG   events;
        status = tx_event_flags_get(&tx_test_events, 0x1u, TX_OR_CLEAR, &events, 10);
        if (status == UX_SUCCESS)
        {
            /* Simulate ISO transfer done event.  */
            feedback_transfer -> ux_transfer_request_completion_code = UX_SUCCESS;
            if (feedback_transfer -> ux_transfer_request_completion_function)
                feedback_transfer -> ux_transfer_request_completion_function(feedback_transfer);
            _ux_host_semaphore_put(&feedback_transfer -> ux_transfer_request_semaphore);
            ux_utility_delay_ms(1);
        }
#else
        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
#endif
    }
}
