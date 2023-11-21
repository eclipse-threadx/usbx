/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"

#include "fx_api.h"

#include "ux_device_class_audio.h"
#include "ux_device_class_audio20.h"
#include "ux_device_stack.h"

#include "ux_host_class_audio.h"
#include "ux_host_class_audio.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"


/* Define constants.  */

#define                             UX_DEMO_REQUEST_MAX_LENGTH \
    ((UX_HCD_SIM_HOST_MAX_PAYLOAD) > (UX_SLAVE_REQUEST_DATA_MAX_LENGTH) ? \
        (UX_HCD_SIM_HOST_MAX_PAYLOAD) : (UX_SLAVE_REQUEST_DATA_MAX_LENGTH))

#define                             UX_DEMO_FEEDBACK (\
    defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT) && \
    defined(UX_DEVICE_CLASS_AUDIO_FEEDBACK_SUPPORT) && \
    defined(UX_HOST_CLASS_AUDIO_FEEDBACK_SUPPORT))

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

static UX_HOST_CLASS_AUDIO                      *host_audio_tx;
static UX_HOST_CLASS_AUDIO                      *host_audio_rx;

static UX_HOST_CLASS_AUDIO_TRANSFER_REQUEST     audio_transfer1 = {0};
static UX_HOST_CLASS_AUDIO_TRANSFER_REQUEST     audio_transfer2 = {0};
UCHAR                                           host_audio_buffer[2][1024 * 3];

static UX_DEVICE_CLASS_AUDIO                    *slave_audio;
static UX_DEVICE_CLASS_AUDIO_PARAMETER           slave_audio_parameter;
static UX_DEVICE_CLASS_AUDIO_STREAM_PARAMETER    slave_audio_stream_parameter[2];

static UX_DEVICE_CLASS_AUDIO_STREAM             *slave_audio_tx_stream;

static UX_DEVICE_CLASS_AUDIO_STREAM             *slave_audio_rx_stream;
static UX_SLAVE_TRANSFER                        *slave_audio_rx_transfer;

static UX_HOST_CLASS_AUDIO                      *audio;

static UX_DEVICE_CLASS_AUDIO20_CONTROL          g_slave_audio20_control[2];

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

static struct BUFFER_LOG_STRUCT {
    ULONG length;
    UCHAR data[256];
} buffer_log[UX_TEST_LOG_SIZE];
static ULONG buffer_log_count = 0;
#define SAVE_BUFFER_LOG(buf,siz) do {                                                      \
    if (buffer_log_count < UX_TEST_LOG_SIZE) {                                             \
        ULONG __local_size__ = ((siz) > 256) ? 256 : (siz);                                \
        buffer_log[buffer_log_count].length = (siz);                                       \
        _ux_utility_memory_copy(buffer_log[buffer_log_count].data, (buf), __local_size__); \
    }                                                                                      \
    buffer_log_count ++;                                                                   \
} while(0)

static ULONG test_tx_ack_count = 0xFFFFFFFF;
static ULONG test_tx_ins_count = 0;
static ULONG test_tx_ins_way   = 0;

static struct CALLBACK_INVOKE_LOG_STRUCT {
    VOID *func;
    VOID *param1;
    VOID *param2;
    VOID *param3;
} callback_invoke_log[UX_TEST_LOG_SIZE];
static ULONG callback_invoke_count = 0;

#define SAVE_CALLBACK_INVOKE_LOG(f,p1,p2,p3) do {                \
    if (callback_invoke_count < UX_TEST_LOG_SIZE) {              \
        callback_invoke_log[callback_invoke_count].func   = (VOID *)(f);  \
        callback_invoke_log[callback_invoke_count].param1 = (VOID *)(p1); \
        callback_invoke_log[callback_invoke_count].param2 = (VOID *)(p2); \
        callback_invoke_log[callback_invoke_count].param3 = (VOID *)(p3); \
        callback_invoke_count++;                                 \
    }                                                            \
} while(0)
#define RESET_CALLBACK_INVOKE_LOG() do { \
    callback_invoke_count = 0;           \
} while(0)

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

static VOID ux_device_class_audio_tx_hook(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *p = (UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *)params;
UX_SLAVE_TRANSFER                                 *transfer = (UX_SLAVE_TRANSFER *)p -> parameter;
UCHAR                                              tmp[32] = {'i','n','s','e','r','t','A',0};


    (void)params;
    // printf("tTX\n");

    /* Acknowledge frame sent.  */
    if (test_tx_ack_count)
    {
        SAVE_BUFFER_LOG(transfer -> ux_slave_transfer_request_data_pointer, transfer -> ux_slave_transfer_request_requested_length);
        transfer -> ux_slave_transfer_request_actual_length = transfer -> ux_slave_transfer_request_requested_length;
        transfer -> ux_slave_transfer_request_completion_code = UX_SUCCESS;
        _ux_utility_semaphore_put(&transfer -> ux_slave_transfer_request_semaphore);
    }
    if (test_tx_ack_count != 0xFFFFFFFF && test_tx_ack_count > 0)
        test_tx_ack_count --;

    /* Insert frames when sent.  */
    if (test_tx_ins_count)
    {
        tmp[6] = (test_tx_ins_count % 26) + 'A';
        if (test_tx_ins_way == 0)
            ux_device_class_audio_frame_write(slave_audio_tx_stream, tmp, 32);
        else
        {

        UCHAR *frame;
        ULONG frame_length;


            ux_device_class_audio_write_frame_get(slave_audio_tx_stream, &frame, &frame_length);
            _ux_utility_memory_copy(frame, tmp, 32);
            ux_device_class_audio_write_frame_commit(slave_audio_tx_stream, 32);
        }
    }
    if (test_tx_ins_count != 0xFFFFFFFF && test_tx_ins_count > 0)
        test_tx_ins_count --;
}

static VOID ux_device_class_audio_rx_hook(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *p = (UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *)params;
UX_SLAVE_TRANSFER                                 *transfer = (UX_SLAVE_TRANSFER *)p -> parameter;


    (void)action;
    (void)params;
    (void)p;
    (void)transfer;
    // printf("tRX\n");
    slave_audio_rx_transfer = transfer;
}
static VOID slave_audio_rx_simulate_one_frame(UCHAR *frame, ULONG frame_length)
{
    UX_TEST_ASSERT(slave_audio_rx_transfer);
    if (frame_length)
    {
        _ux_utility_memory_copy(slave_audio_rx_transfer->ux_slave_transfer_request_data_pointer, frame, frame_length);
        slave_audio_rx_transfer->ux_slave_transfer_request_actual_length = frame_length;
        slave_audio_rx_transfer->ux_slave_transfer_request_completion_code = UX_SUCCESS;
    }
    _ux_utility_semaphore_put(&slave_audio_rx_transfer->ux_slave_transfer_request_semaphore);
    _ux_utility_thread_sleep(1);
}

static UX_TEST_ACTION ux_device_class_audio_transfer_hook[] =
{
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .action_func = ux_device_class_audio_tx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x81,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .action_func = ux_device_class_audio_rx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x02,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
{ 0 },
};

static VOID ux_host_class_audio_tx_hook(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{
UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *p = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS*)params;
UX_TRANSFER                                     *transfer = (UX_TRANSFER *)p->parameter;
    SAVE_CALLBACK_INVOKE_LOG(ux_host_class_audio_tx_hook, transfer->ux_transfer_request_endpoint->ux_endpoint_descriptor.bEndpointAddress, transfer->ux_transfer_request_requested_length, 0);
    // printf("hTxHook %lx %ld\n", transfer->ux_transfer_request_endpoint->ux_endpoint_descriptor.bEndpointAddress, transfer->ux_transfer_request_requested_length);
    transfer->ux_transfer_request_actual_length=transfer->ux_transfer_request_requested_length;
    transfer->ux_transfer_request_completion_code=UX_SUCCESS;
    if (transfer->ux_transfer_request_completion_function)
        transfer->ux_transfer_request_completion_function(transfer);
}

static VOID ux_host_class_audio_rx_hook(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{
UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *p = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS*)params;
UX_TRANSFER                                     *transfer = (UX_TRANSFER *)p->parameter;
    SAVE_CALLBACK_INVOKE_LOG(ux_host_class_audio_rx_hook, transfer->ux_transfer_request_endpoint->ux_endpoint_descriptor.bEndpointAddress, transfer->ux_transfer_request_requested_length, 0);
    // printf("hRxHook %lx %ld\n", transfer->ux_transfer_request_endpoint->ux_endpoint_descriptor.bEndpointAddress, transfer->ux_transfer_request_requested_length);
    transfer->ux_transfer_request_actual_length=transfer->ux_transfer_request_requested_length;
    transfer->ux_transfer_request_completion_code=UX_SUCCESS;
    if (transfer->ux_transfer_request_completion_function)
        transfer->ux_transfer_request_completion_function(transfer);
}

#if UX_DEMO_FEEDBACK
static UX_TRANSFER  *feedback_transfer;
static ULONG        feedback_data = 0;
static VOID ux_host_class_audio_feedback_hook(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{
UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS   *p = (UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY_PARAMS*)params;
UX_TRANSFER                                     *transfer = (UX_TRANSFER *)p->parameter;
    // printf("hFeedbackHook %lx %ld, %lx\n", transfer->ux_transfer_request_endpoint->ux_endpoint_descriptor.bEndpointAddress, transfer->ux_transfer_request_requested_length, feedback_data);
    feedback_transfer = transfer;
    *(ULONG *)transfer -> ux_transfer_request_data_pointer = feedback_data;
    if (transfer -> ux_transfer_request_endpoint -> ux_endpoint_device -> ux_device_speed == UX_HIGH_SPEED_DEVICE)
        transfer -> ux_transfer_request_actual_length = 4;
    else
        transfer -> ux_transfer_request_actual_length = 3;
    tx_event_flags_set(&tx_test_events, 0x01, TX_OR);
}
#endif

static UX_TEST_ACTION ux_host_class_audio_transfer_hook[] =
{
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
        .function = UX_HCD_TRANSFER_REQUEST,
        .action_func = ux_host_class_audio_tx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x02,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
        .function = UX_HCD_TRANSFER_REQUEST,
        .action_func = ux_host_class_audio_rx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x81,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
#if UX_DEMO_FEEDBACK
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
        .function = UX_HCD_TRANSFER_REQUEST,
        .action_func = ux_host_class_audio_feedback_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x82,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
#endif
{0},
};
static UX_TEST_ACTION ux_host_class_audio_tx_action[] =
{
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
        .function = UX_HCD_TRANSFER_REQUEST,
        .action_func = ux_host_class_audio_tx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x02,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
        .function = UX_HCD_TRANSFER_REQUEST,
        .action_func = ux_host_class_audio_tx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x02,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
{0},
};
static UX_TEST_ACTION ux_host_class_audio_rx_action[] =
{
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
        .function = UX_HCD_TRANSFER_REQUEST,
        .action_func = ux_host_class_audio_rx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x81,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_HCD_SIM_HOST_ENTRY,
        .function = UX_HCD_TRANSFER_REQUEST,
        .action_func = ux_host_class_audio_rx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x81,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
{0},
};

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;
    // printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);

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

static VOID    slave_audio_activate(VOID *audio_instance)
{
    slave_audio = (UX_DEVICE_CLASS_AUDIO *)audio_instance;
    ux_device_class_audio_stream_get(slave_audio, 0, &slave_audio_tx_stream);
    ux_device_class_audio_stream_get(slave_audio, 1, &slave_audio_rx_stream);
    // printf("sAUD:%p;%p,%p\n", audio_instance, slave_audio_tx_stream, slave_audio_rx_stream);
}
static VOID    slave_audio_deactivate(VOID *audio_instance)
{
    if ((VOID *)slave_audio == audio_instance)
    {
        slave_audio = UX_NULL;
        slave_audio_tx_stream = UX_NULL;
        slave_audio_rx_stream = UX_NULL;
    }
}
static VOID    slave_audio_tx_stream_change(UX_DEVICE_CLASS_AUDIO_STREAM *audio, ULONG alt)
{
    SAVE_CALLBACK_INVOKE_LOG(slave_audio_tx_stream_change, audio, (ALIGN_TYPE)alt, 0);
}
static VOID    slave_audio_rx_stream_change(UX_DEVICE_CLASS_AUDIO_STREAM *audio, ULONG alt)
{
    SAVE_CALLBACK_INVOKE_LOG(slave_audio_rx_stream_change, audio, (ALIGN_TYPE)alt, 0);

    slave_audio_rx_transfer = UX_NULL;
}
static UINT    slave_audio_control_process(UX_DEVICE_CLASS_AUDIO *audio, UX_SLAVE_TRANSFER *transfer)
{


UINT                                    status;
UX_DEVICE_CLASS_AUDIO20_CONTROL_GROUP   group =
    {2, g_slave_audio20_control};


    SAVE_CALLBACK_INVOKE_LOG(slave_audio_control_process, audio, transfer, 0);

    /* For sampling frequency support.  */
    {
        UCHAR                           *setup = transfer -> ux_slave_transfer_request_setup;
        UCHAR                           *buffer = transfer -> ux_slave_transfer_request_data_pointer;
        UCHAR                           bmRequestType = setup[UX_SETUP_REQUEST_TYPE];
        UCHAR                           bRequest      = setup[UX_SETUP_REQUEST];
        UCHAR                           wValue_CN     = setup[UX_SETUP_VALUE];
        UCHAR                           wValue_CS     = setup[UX_SETUP_VALUE + 1];
        UCHAR                           wIndex_iface  = setup[UX_SETUP_INDEX];
        UCHAR                           wIndex_ID     = setup[UX_SETUP_INDEX + 1];
        ULONG                           wLength       = _ux_utility_long_get(setup + UX_SETUP_LENGTH);
        /* AC Interface request.  */
        if (audio->ux_device_class_audio_interface->ux_slave_interface_descriptor.bInterfaceNumber == wIndex_iface)
        {
            /* AC Get request.  */
            if (bmRequestType == (UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE))
            {

                /* Clock Selector CUR.  */
                if (wIndex_ID == 0x12 &&
                    wValue_CS == UX_DEVICE_CLASS_AUDIO20_CX_CLOCK_SELECTOR_CONTROL &&
                    bRequest == UX_DEVICE_CLASS_AUDIO20_CUR)
                {
                    if (wLength < 1)
                        return(UX_ERROR);
                    *buffer = 1;
                    ux_device_stack_transfer_request(transfer, 1, wLength);
                    return(UX_SUCCESS);
                }
                /* Clock Multiplier Numerator, Denominator CUR.  */
                if (wIndex_ID == 0x10)
                {
                    if (wLength < 2)
                        return(UX_ERROR);
                    if (wValue_CS == UX_DEVICE_CLASS_AUDIO20_CM_NUMERATOR_CONTROL)
                    {
                        ux_utility_short_put(buffer, 12);
                        ux_device_stack_transfer_request(transfer, 2, wLength);
                        return(UX_SUCCESS);
                    }
                    if (wValue_CS == UX_DEVICE_CLASS_AUDIO20_CM_DENOMINATOR_CONTROL)
                    {
                        ux_utility_short_put(buffer, 2);
                        ux_device_stack_transfer_request(transfer, 2, wLength);
                        return(UX_SUCCESS);
                    }
                }
                /* Clock Source, sampling control.  */
                if (wIndex_ID == 0x11 && wValue_CS == UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL)
                {
                    if (bRequest == UX_DEVICE_CLASS_AUDIO20_CUR)
                    {
                        if (wLength < 4)
                            return(UX_ERROR);
                        ux_utility_long_put(buffer, 8000);
                        ux_device_stack_transfer_request(transfer, 4, wLength);
                        return(UX_SUCCESS);
                    }
                    if (bRequest == UX_DEVICE_CLASS_AUDIO20_RANGE)
                    {
                        if (wLength < 2)
                            return(UX_ERROR);
                        ux_utility_long_put(buffer +  0, 1);        /* wNumSubRanges  */
                        ux_utility_long_put(buffer +  2, 8000);     /* dMIN  */
                        ux_utility_long_put(buffer +  6, 8000);     /* dMAX  */
                        ux_utility_long_put(buffer + 10, 0);        /* dRES  */
                        ux_device_stack_transfer_request(transfer, UX_MIN(2+4*3, wLength), wLength);
                        return(UX_SUCCESS);
                    }
                }
            }
        }
    }

    status = ux_device_class_audio20_control_process(audio, transfer, &group);
    if (status == UX_SUCCESS)
    {
        if (g_slave_audio20_control[0].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_MUTE_CHANGED)
        {
            /* Mute change! */
        }
        if (g_slave_audio20_control[0].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_VOLUME_CHANGED)
        {
            /* Volume change! */
        }
        if (g_slave_audio20_control[1].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_MUTE_CHANGED)
        {
            /* Mute change! */
        }
        if (g_slave_audio20_control[1].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_VOLUME_CHANGED)
        {
            /* Volume change! */
        }
    }
    return(status);
}
static VOID    slave_audio_tx_done(UX_DEVICE_CLASS_AUDIO_STREAM *audio, ULONG length)
{
    SAVE_CALLBACK_INVOKE_LOG(slave_audio_tx_done, audio, (ALIGN_TYPE)length, 0);
}
static VOID    slave_audio_rx_done(UX_DEVICE_CLASS_AUDIO_STREAM *audio, ULONG length)
{
    SAVE_CALLBACK_INVOKE_LOG(slave_audio_rx_done, audio, (ALIGN_TYPE)length, 0);
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_AUDIO *audio = (UX_HOST_CLASS_AUDIO *) inst;


    switch(event)
    {

        case UX_DEVICE_INSERTION:

            // printf("hINS:%p,%p:%ld\n", cls, inst, ux_host_class_audio_type_get(audio));
            if (ux_host_class_audio_type_get(audio) == UX_HOST_CLASS_AUDIO_INPUT)
                host_audio_rx = audio;
            else
                host_audio_tx = audio;
            break;

        case UX_DEVICE_REMOVAL:

            // printf("hRMV:%p,%p:%ld\n", cls, inst, ux_host_class_audio_type_get(audio));
            if (audio == host_audio_rx)
                host_audio_rx = UX_NULL;
            if (audio == host_audio_tx)
                host_audio_tx = UX_NULL;
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

    rsc_mem_free_on_set_cfg = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
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
void    usbx_audio20_device_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    /* Inform user.  */
    printf("Running Audio 2.0 Host Basic Functionality Test..................... ");

#if !UX_TEST_MULTI_IFC_ON || !UX_TEST_MULTI_ALT_ON || !UX_TEST_MULTI_CLS_ON || \
    !defined(UX_HOST_CLASS_AUDIO_2_SUPPORT)                                 || \
    !defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT)                      || \
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

    /* Set the parameters for callback when insertion/extraction of a Audio 2.0 device, no IAD.  */
    slave_audio_stream_parameter[0].ux_device_class_audio_stream_parameter_thread_entry = ux_device_class_audio_write_thread_entry;
    slave_audio_stream_parameter[0].ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_change = slave_audio_tx_stream_change;
    slave_audio_stream_parameter[0].ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_frame_done = slave_audio_tx_done;
    slave_audio_stream_parameter[0].ux_device_class_audio_stream_parameter_max_frame_buffer_size = 256;
    slave_audio_stream_parameter[0].ux_device_class_audio_stream_parameter_max_frame_buffer_nb   = 8;
#if defined(UX_DEVICE_CLASS_AUDIO_FEEDBACK_SUPPORT)
    slave_audio_stream_parameter[1].ux_device_class_audio_stream_parameter_feedback_thread_entry = ux_device_class_audio_feedback_thread_entry;
#endif
    slave_audio_stream_parameter[1].ux_device_class_audio_stream_parameter_thread_entry = ux_device_class_audio_read_thread_entry;
    slave_audio_stream_parameter[1].ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_change = slave_audio_rx_stream_change;
    slave_audio_stream_parameter[1].ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_frame_done = slave_audio_rx_done;
    slave_audio_stream_parameter[1].ux_device_class_audio_stream_parameter_max_frame_buffer_size = 256;
    slave_audio_stream_parameter[1].ux_device_class_audio_stream_parameter_max_frame_buffer_nb   = 8;
    slave_audio_parameter.ux_device_class_audio_parameter_streams = slave_audio_stream_parameter;
    slave_audio_parameter.ux_device_class_audio_parameter_streams_nb = 2;
    slave_audio_parameter.ux_device_class_audio_parameter_callbacks.ux_slave_class_audio_instance_activate   = slave_audio_activate;
    slave_audio_parameter.ux_device_class_audio_parameter_callbacks.ux_slave_class_audio_instance_deactivate = slave_audio_deactivate;
    slave_audio_parameter.ux_device_class_audio_parameter_callbacks.ux_device_class_audio_control_process = slave_audio_control_process;
    slave_audio_parameter.ux_device_class_audio_parameter_callbacks.ux_device_class_audio_arg             = UX_NULL;

#if defined(UX_DEVICE_CLASS_AUDIO_INTERRUPT_SUPPORT)
    slave_audio_parameter.ux_device_class_audio_parameter_status_queue_size = 2;
    slave_audio_parameter.ux_device_class_audio_parameter_status_size = 6;
#endif

    g_slave_audio20_control[0].ux_device_class_audio20_control_cs_id                = 0x10;
    g_slave_audio20_control[0].ux_device_class_audio20_control_sampling_frequency   = 48000;
    g_slave_audio20_control[0].ux_device_class_audio20_control_fu_id                = 2;
    g_slave_audio20_control[0].ux_device_class_audio20_control_mute[0]              = 0;
    g_slave_audio20_control[0].ux_device_class_audio20_control_volume[0]            = 0;
    g_slave_audio20_control[1].ux_device_class_audio20_control_cs_id                = 0x10;
    g_slave_audio20_control[1].ux_device_class_audio20_control_sampling_frequency   = 48000;
    g_slave_audio20_control[1].ux_device_class_audio20_control_fu_id                = 5;
    g_slave_audio20_control[1].ux_device_class_audio20_control_mute[0]              = 0;
    g_slave_audio20_control[1].ux_device_class_audio20_control_volume[0]            = 0;

#if 0
    printf("Memory requirement UX_HOST_CLASS_:\n");
    printf(" per _AUDIO: %d bytes\n", sizeof(UX_HOST_CLASS_AUDIO));
    printf(" per _AUDIO_TRANSFER_REQUEST: %d bytes\n", sizeof(UX_HOST_CLASS_AUDIO_TRANSFER_REQUEST));
    printf(" per _AUDIO_CONTROL: %d bytes\n", sizeof(UX_HOST_CLASS_AUDIO_CONTROL));
    printf(" per _AUDIO_SAMPLING: %d bytes\n", sizeof(UX_HOST_CLASS_AUDIO_SAMPLING));
    printf(" per _AUDIO_SAMPLING_ATTR: %d bytes\n", sizeof(UX_HOST_CLASS_AUDIO_SAMPLING_CHARACTERISTICS));
#endif

    /* Initialize the device Audio class. This class owns interfaces starting with 0, 1, 2. */
    status  = ux_device_stack_class_register(_ux_system_slave_class_audio_name, ux_device_class_audio_entry,
                                             1, 0,  &slave_audio_parameter);
    UX_TEST_CHECK_SUCCESS(status);

    /* Hook ISO transfers.  */
    ux_test_link_hooks_from_array(ux_host_class_audio_transfer_hook);

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
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
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
    rsc_audio_mem_usage = rsc_mem_free_on_set_cfg - _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    rsc_audio_mem_alloc_count = ux_test_utility_sim_mem_alloc_count() - rsc_enum_mem_alloc_count;
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    /* Lock log base for tests. */
    ux_test_utility_sim_mem_alloc_log_lock();

    stepinfo("enum mem: %ld\n", rsc_enum_mem_alloc_count);
    stepinfo("cdc mem : %ld\n", rsc_audio_mem_alloc_count);
    stepinfo("mem free: %ld, %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available);

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
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #11.%ld: Memory level different after re-enumerations %ld <> %ld\n", test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
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
            mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
        else if (mem_free != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
        {

            printf("ERROR #11.%ld: Memory level different after re-enumerations %ld <> %ld\n", test_n, mem_free, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
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
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_mem_alloc_error_generation_stop();
    if (rsc_audio_mem_alloc_count) stepinfo("\n");
}

static void _feature_control_tests(void)
{
UX_HOST_CLASS_AUDIO_CONTROL                 audio_control;
UINT                                        status;
UCHAR                                       buffer[32];
ULONG                                       actual_length;

#if !defined(UX_HOST_CLASS_AUDIO_DISABLE_CONTROLS)
    RESET_CALLBACK_INVOKE_LOG();

    audio_control.ux_host_class_audio_control_channel =  1;
    audio_control.ux_host_class_audio_control =  UX_HOST_CLASS_AUDIO_VOLUME_CONTROL;
    status =  ux_host_class_audio_control_get(host_audio_tx, &audio_control);
    // UX_TEST_ASSERT(status == UX_TRANSFER_STALLED);
    UX_TEST_ASSERT(callback_invoke_count == 1);
    UX_TEST_ASSERT(callback_invoke_log[0].func == slave_audio_control_process);
    UX_TEST_ASSERT(callback_invoke_log[0].param1 == slave_audio_rx_stream->ux_device_class_audio_stream_audio);

    audio_control.ux_host_class_audio_control_channel =  1;
    audio_control.ux_host_class_audio_control =  UX_HOST_CLASS_AUDIO_VOLUME_CONTROL;
    audio_control.ux_host_class_audio_control_cur =  0xfff0;
    status =  ux_host_class_audio_control_value_set(host_audio_tx, &audio_control);
    // UX_TEST_ASSERT(status == UX_TRANSFER_STALLED);
    UX_TEST_ASSERT(callback_invoke_count == 2);
    UX_TEST_ASSERT(callback_invoke_log[1].func == slave_audio_control_process);
    UX_TEST_ASSERT(callback_invoke_log[1].param1 == slave_audio_rx_stream->ux_device_class_audio_stream_audio);

    audio_control.ux_host_class_audio_control_channel =  2;
    audio_control.ux_host_class_audio_control =  UX_HOST_CLASS_AUDIO_VOLUME_CONTROL;
    status =  ux_host_class_audio_control_get(host_audio_tx, &audio_control);
    // UX_TEST_ASSERT(status == UX_TRANSFER_STALLED);
    UX_TEST_ASSERT(callback_invoke_count == 3);
    UX_TEST_ASSERT(callback_invoke_log[2].func == slave_audio_control_process);
    UX_TEST_ASSERT(callback_invoke_log[2].param1 == slave_audio_rx_stream->ux_device_class_audio_stream_audio);

    audio_control.ux_host_class_audio_control_channel =  2;
    audio_control.ux_host_class_audio_control =  UX_HOST_CLASS_AUDIO_VOLUME_CONTROL;
    audio_control.ux_host_class_audio_control_cur =  0xfff0;
    status =  ux_host_class_audio_control_value_set(host_audio_tx, &audio_control);
    // UX_TEST_ASSERT(status == UX_TRANSFER_STALLED);
    UX_TEST_ASSERT(callback_invoke_count == 4);
    UX_TEST_ASSERT(callback_invoke_log[3].func == slave_audio_control_process);
    UX_TEST_ASSERT(callback_invoke_log[3].param1 == slave_audio_rx_stream->ux_device_class_audio_stream_audio);
#endif

    RESET_CALLBACK_INVOKE_LOG();

    audio_control.ux_host_class_audio_control_entity = 0x05;
    audio_control.ux_host_class_audio_control_size = 2;
    audio_control.ux_host_class_audio_control =  UX_HOST_CLASS_AUDIO_VOLUME_CONTROL;

    audio_control.ux_host_class_audio_control_channel =  1;
    status =  ux_host_class_audio_entity_control_get(host_audio_tx, &audio_control);
    UX_TEST_ASSERT(callback_invoke_count == 1);
    UX_TEST_ASSERT(callback_invoke_log[0].func == slave_audio_control_process);
    UX_TEST_ASSERT(callback_invoke_log[0].param1 == slave_audio_rx_stream->ux_device_class_audio_stream_audio);

    audio_control.ux_host_class_audio_control_cur =  0xfff0;
    status =  ux_host_class_audio_entity_control_value_set(host_audio_tx, &audio_control);
    UX_TEST_ASSERT(callback_invoke_count == 2);
    UX_TEST_ASSERT(callback_invoke_log[1].func == slave_audio_control_process);
    UX_TEST_ASSERT(callback_invoke_log[1].param1 == slave_audio_rx_stream->ux_device_class_audio_stream_audio);

    audio_control.ux_host_class_audio_control_channel =  2;
    status =  ux_host_class_audio_entity_control_get(host_audio_tx, &audio_control);
    UX_TEST_ASSERT(callback_invoke_count == 3);
    UX_TEST_ASSERT(callback_invoke_log[2].func == slave_audio_control_process);
    UX_TEST_ASSERT(callback_invoke_log[2].param1 == slave_audio_rx_stream->ux_device_class_audio_stream_audio);

    audio_control.ux_host_class_audio_control_cur =  0xfff0;
    status =  ux_host_class_audio_entity_control_value_set(host_audio_tx, &audio_control);
    UX_TEST_ASSERT(callback_invoke_count == 4);
    UX_TEST_ASSERT(callback_invoke_log[3].func == slave_audio_control_process);
    UX_TEST_ASSERT(callback_invoke_log[3].param1 == slave_audio_rx_stream->ux_device_class_audio_stream_audio);

#if defined(UX_HOST_CLASS_AUDIO_2_SUPPORT) /* Class consts included.  */

    buffer[0] = 0x5a;
    status = ux_host_class_audio_control_request(host_audio_tx, 0,
        0x21, UX_CLASS_AUDIO20_CUR,
        0 | (UX_CLASS_AUDIO20_FU_MUTE_CONTROL << 8),
        0x05,
        buffer, 1, &actual_length);
    UX_TEST_CHECK_SUCCESS(status);

    buffer[0] = 0;
    status = ux_host_class_audio_control_request(host_audio_tx, 0,
        0xa1, UX_CLASS_AUDIO20_CUR,
        0 | (UX_CLASS_AUDIO20_FU_MUTE_CONTROL << 8),
        0x05,
        buffer, 1, &actual_length);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(buffer[0] == 0x5a);

    status = ux_host_class_audio_control_request(host_audio_tx, 0,
        0xa1, UX_CLASS_AUDIO20_CUR,
        0 | (UX_CLASS_AUDIO20_CM_DENOMINATOR_CONTROL << 8),
        0x10,
        buffer, 2, &actual_length);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(buffer[0] == 2 && buffer[1] == 0);

    status = ux_host_class_audio_control_request(host_audio_tx, 0,
        0xa1, UX_CLASS_AUDIO20_CUR,
        0 | (UX_CLASS_AUDIO20_CM_NUMERATOR_CONTROL << 8),
        0x10,
        buffer, 2, &actual_length);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(buffer[0] == 12 && buffer[1] == 0);

    status = ux_host_class_audio_control_request(host_audio_tx, 0,
        0xa1, UX_CLASS_AUDIO20_CUR,
        0 | (UX_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8),
        0x11,
        buffer, 4, &actual_length);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(*(ULONG*)buffer == 8000);
#endif
}

static void _sampling_control_tests(void)
{
UINT                                        status;
UX_HOST_CLASS_AUDIO_SAMPLING                sampling;

    RESET_CALLBACK_INVOKE_LOG();

    sampling.ux_host_class_audio_sampling_channels =   2;
    sampling.ux_host_class_audio_sampling_frequency =  44100;
    sampling.ux_host_class_audio_sampling_resolution = 16;
    status =  ux_host_class_audio_streaming_sampling_set(host_audio_tx, &sampling);
    UX_TEST_CHECK_NOT_SUCCESS(status);

    sampling.ux_host_class_audio_sampling_channels =   4;
    sampling.ux_host_class_audio_sampling_frequency =  48000;
    sampling.ux_host_class_audio_sampling_resolution = 16;
    status =  ux_host_class_audio_streaming_sampling_set(host_audio_tx, &sampling);
    UX_TEST_CHECK_NOT_SUCCESS(status);

    sampling.ux_host_class_audio_sampling_channels =   2;
    sampling.ux_host_class_audio_sampling_frequency =  48000;
    sampling.ux_host_class_audio_sampling_resolution = 32;
    status =  ux_host_class_audio_streaming_sampling_set(host_audio_tx, &sampling);
    UX_TEST_CHECK_NOT_SUCCESS(status);

    sampling.ux_host_class_audio_sampling_channels =   2;
    sampling.ux_host_class_audio_sampling_frequency =  48000;
    sampling.ux_host_class_audio_sampling_resolution = 16;
    status =  ux_host_class_audio_streaming_sampling_set(host_audio_tx, &sampling);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count > 1); /* There could be multiple requests.  */
    UX_TEST_ASSERT(callback_invoke_log[callback_invoke_count-1].func == slave_audio_rx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[callback_invoke_count-1].param1 == slave_audio_rx_stream);
    UX_TEST_ASSERT(ux_host_class_audio_max_packet_size_get(host_audio_tx) == 256);
    UX_TEST_ASSERT(host_audio_tx->ux_host_class_audio_packet_fraction == 0);
    UX_TEST_ASSERT(host_audio_tx->ux_host_class_audio_packet_freq == 1000);
    UX_TEST_ASSERT(host_audio_tx->ux_host_class_audio_packet_size == 192);

    sampling.ux_host_class_audio_sampling_channels =   2;
    sampling.ux_host_class_audio_sampling_frequency =  48000;
    sampling.ux_host_class_audio_sampling_resolution = 16;
    status =  ux_host_class_audio_streaming_sampling_set(host_audio_rx, &sampling);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_log[callback_invoke_count-1].func == slave_audio_tx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[callback_invoke_count-1].param1 == slave_audio_tx_stream);
    UX_TEST_ASSERT(ux_host_class_audio_max_packet_size_get(host_audio_rx) == 256);
    UX_TEST_ASSERT(host_audio_rx->ux_host_class_audio_packet_fraction == 0);
    UX_TEST_ASSERT(host_audio_rx->ux_host_class_audio_packet_freq == 1000);
    UX_TEST_ASSERT(host_audio_rx->ux_host_class_audio_packet_size == 192);
}

static void _audio_request_completion(UX_HOST_CLASS_AUDIO_TRANSFER_REQUEST *transfer)
{
    UX_PARAMETER_NOT_USED(transfer);
    SAVE_CALLBACK_INVOKE_LOG(_audio_request_completion, transfer, 0, 0);
}
static void _audio_requests_tests(void)
{
UINT        status;

    RESET_CALLBACK_INVOKE_LOG();

    /* Prepare the 2 audio transfer_requests */
    audio_transfer1.ux_host_class_audio_transfer_request_completion_function = _audio_request_completion;
    audio_transfer2.ux_host_class_audio_transfer_request_completion_function = _audio_request_completion;
    audio_transfer1.ux_host_class_audio_transfer_request_class_instance =  host_audio_tx;
    audio_transfer2.ux_host_class_audio_transfer_request_class_instance =  host_audio_tx;
    audio_transfer1.ux_host_class_audio_transfer_request_next_audio_transfer_request =  &audio_transfer1;
    audio_transfer2.ux_host_class_audio_transfer_request_next_audio_transfer_request =  UX_NULL;

    audio_transfer1.ux_host_class_audio_transfer_request_data_pointer =  host_audio_buffer[0];
    audio_transfer1.ux_host_class_audio_transfer_request_requested_length =  sizeof(host_audio_buffer[0]);
    audio_transfer1.ux_host_class_audio_transfer_request.ux_transfer_request_packet_length =  192;

    audio_transfer2.ux_host_class_audio_transfer_request_data_pointer =  host_audio_buffer[1];
    audio_transfer2.ux_host_class_audio_transfer_request_requested_length =  sizeof(host_audio_buffer[1]);
    audio_transfer2.ux_host_class_audio_transfer_request.ux_transfer_request_packet_length =  192;

    // ux_test_set_main_action_list_from_array(ux_host_class_audio_tx_action);

    status =  ux_host_class_audio_write(host_audio_tx, &audio_transfer1);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 2);
    UX_TEST_ASSERT(callback_invoke_log[0].func == ux_host_class_audio_tx_hook);
    UX_TEST_ASSERT(callback_invoke_log[0].param1 == (void*)0x02);
    UX_TEST_ASSERT(callback_invoke_log[0].param2 == (void*)sizeof(host_audio_buffer[0]));
    UX_TEST_ASSERT(callback_invoke_log[1].func == _audio_request_completion);
    UX_TEST_ASSERT(callback_invoke_log[1].param1 == &audio_transfer1);

    status =  ux_host_class_audio_write(host_audio_tx, &audio_transfer2);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 4);
    UX_TEST_ASSERT(callback_invoke_log[2].func == ux_host_class_audio_tx_hook);
    UX_TEST_ASSERT(callback_invoke_log[2].param1 == (void*)0x02);
    UX_TEST_ASSERT(callback_invoke_log[2].param2 == (void*)sizeof(host_audio_buffer[1]));
    UX_TEST_ASSERT(callback_invoke_log[3].func == _audio_request_completion);
    UX_TEST_ASSERT(callback_invoke_log[3].param1 == &audio_transfer2);

    // ux_test_set_main_action_list_from_array(ux_host_class_audio_rx_action);

    status =  ux_host_class_audio_read(host_audio_rx, &audio_transfer1);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 6);
    UX_TEST_ASSERT(callback_invoke_log[4].func == ux_host_class_audio_rx_hook);
    UX_TEST_ASSERT(callback_invoke_log[4].param1 == (void*)0x81);
    UX_TEST_ASSERT(callback_invoke_log[4].param2 == (void*)256);
    UX_TEST_ASSERT(callback_invoke_log[5].func == _audio_request_completion);
    UX_TEST_ASSERT(callback_invoke_log[5].param1 == &audio_transfer1);

    status =  ux_host_class_audio_read(host_audio_rx, &audio_transfer2);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 8);
    UX_TEST_ASSERT(callback_invoke_log[6].func == ux_host_class_audio_rx_hook);
    UX_TEST_ASSERT(callback_invoke_log[6].param1 == (void*)0x81);
    UX_TEST_ASSERT(callback_invoke_log[6].param2 == (void*)256);
    UX_TEST_ASSERT(callback_invoke_log[7].func == _audio_request_completion);
    UX_TEST_ASSERT(callback_invoke_log[7].param1 == &audio_transfer2);
}
#if UX_DEMO_FEEDBACK
static void _audio_feedback_tests(void)
{
ULONG       host_feedback;
UINT        status;

    tx_thread_suspend(&tx_test_thread_slave_simulation);

    host_feedback = 0;
    status = ux_host_class_audio_feedback_set(host_audio_tx, (UCHAR *)&host_feedback);
    UX_TEST_CHECK_SUCCESS(status);

    host_feedback = 0xFF;
    status = ux_host_class_audio_feedback_get(host_audio_tx, (UCHAR *)&host_feedback);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(0 == host_feedback);

    tx_thread_resume(&tx_test_thread_slave_simulation);


    feedback_data = 0x123456;
    ux_utility_delay_ms(20);
    status = ux_host_class_audio_feedback_get(host_audio_tx, (UCHAR *)&host_feedback);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(feedback_data == host_feedback);

    feedback_data = 0x654321;
    ux_utility_delay_ms(20);
    status = ux_host_class_audio_feedback_get(host_audio_tx, (UCHAR *)&host_feedback);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(feedback_data == host_feedback);
}
#endif

static void _audio_stop_test(void)
{
UINT        status;
    RESET_CALLBACK_INVOKE_LOG();
    status = ux_host_class_audio_stop(host_audio_tx);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 1);
    UX_TEST_ASSERT(callback_invoke_log[callback_invoke_count-1].func == slave_audio_rx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[callback_invoke_count-1].param1 == slave_audio_rx_stream);

    status = ux_host_class_audio_stop(host_audio_rx);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 2);
    UX_TEST_ASSERT(callback_invoke_log[callback_invoke_count-1].func == slave_audio_tx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[callback_invoke_count-1].param1 == slave_audio_tx_stream);
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
    status |= test_wait_until_not_null((void**)&slave_audio_tx_stream, 100);
    status |= test_wait_until_not_null((void**)&slave_audio_rx_stream, 100);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(ux_host_class_audio_protocol_get(host_audio_rx) == UX_HOST_CLASS_AUDIO_PROTOCOL_IP_VERSION_02_00);
    UX_TEST_ASSERT(ux_host_class_audio_type_get(host_audio_rx) == UX_HOST_CLASS_AUDIO_INPUT);
    UX_TEST_ASSERT(ux_host_class_audio_speed_get(host_audio_rx) == UX_FULL_SPEED_DEVICE);

    /* Check enumeration information.  */
    UX_TEST_ASSERT(host_audio_rx->ux_host_class_audio_control_interface_number == 0);
    UX_TEST_ASSERT(host_audio_rx->ux_host_class_audio_streaming_interface->ux_interface_descriptor.bInterfaceNumber == 1);
    UX_TEST_ASSERT(host_audio_tx->ux_host_class_audio_control_interface_number == 0);
    UX_TEST_ASSERT(host_audio_tx->ux_host_class_audio_streaming_interface->ux_interface_descriptor.bInterfaceNumber == 2);

    _memory_tests();
    _feature_control_tests();
    _sampling_control_tests();
    _audio_requests_tests();
#if UX_DEMO_FEEDBACK
    _audio_feedback_tests();
#endif
    _audio_stop_test();

    /* Wait pending threads.  */
    _ux_utility_thread_sleep(1);

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    ux_device_stack_class_unregister(_ux_system_slave_class_audio_name, ux_device_class_audio_entry);

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
