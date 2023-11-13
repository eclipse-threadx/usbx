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
#include "ux_host_class_dummy.h"

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


/* Define global data structures.  */
static UCHAR                                    usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UCHAR                                    buffer[64];

static UX_HOST_CLASS_DUMMY                      *dummy_control;
static UX_HOST_CLASS_DUMMY                      *dummy_tx;
static UX_HOST_CLASS_DUMMY                      *dummy_rx;

static UX_DEVICE_CLASS_AUDIO                    *slave_audio;
static UX_DEVICE_CLASS_AUDIO_PARAMETER           slave_audio_parameter;
static UX_DEVICE_CLASS_AUDIO_STREAM_PARAMETER    slave_audio_stream_parameter[2];

static UX_DEVICE_CLASS_AUDIO_STREAM             *slave_audio_tx_stream;

static UX_DEVICE_CLASS_AUDIO_STREAM             *slave_audio_rx_stream;
static UX_SLAVE_TRANSFER                        *slave_audio_rx_transfer;

static ULONG                                    device_audio_tx_feedback;
static UCHAR*                                   device_audio_tx_feedback_u8a = (UCHAR*)&device_audio_tx_feedback;
static ULONG                                    device_audio_tx_feedback_count;
static ULONG                                    device_audio_rx_feedback;
static UCHAR*                                   device_audio_rx_feedback_u8a = (UCHAR*)&device_audio_rx_feedback;
static ULONG                                    device_audio_rx_feedback_count;


static UX_HOST_CLASS_AUDIO                      *audio;

static UX_DEVICE_CLASS_AUDIO20_CONTROL          g_slave_audio20_control[2];
static UCHAR                                    audio20_cs_range[] = {
    UX_W0 (    2), UX_W0 (    2), /* Number of subs.  */

    UX_DW0(44100), UX_DW1(44100), UX_DW2(44100), UX_DW3(44100), /* dMIN  */
    UX_DW0(44100), UX_DW1(44100), UX_DW2(44100), UX_DW3(44100), /* dMAX  */
    UX_DW0(    0), UX_DW1(    0), UX_DW2(    0), UX_DW3(    0), /* dRES  */

    UX_DW0( 8000), UX_DW1( 8000), UX_DW2( 8000), UX_DW3( 8000), /* dMIN  */
    UX_DW0(48000), UX_DW1(48000), UX_DW2(48000), UX_DW3(48000), /* dMAX  */
    UX_DW0( 8000), UX_DW1( 8000), UX_DW2( 8000), UX_DW3( 8000), /* dRES  */
};

static ULONG                               error_counter;

static ULONG                               set_cfg_counter;

static ULONG                               rsc_mem_free_on_set_cfg;
static ULONG                               rsc_sem_on_set_cfg;
static ULONG                               rsc_sem_get_on_set_cfg;
static ULONG                               rsc_mutex_on_set_cfg;

static ULONG                               rsc_enum_sem_usage;
static ULONG                               rsc_enum_sem_get_count;
static ULONG                               rsc_enum_mutex_usage;
static ULONG                               rsc_enum_mem_usage;

static ULONG                               rsc_audio_sem_usage;
static ULONG                               rsc_audio_sem_get_count;
static ULONG                               rsc_audio_mutex_usage;
static ULONG                               rsc_audio_mem_usage;

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

/* -------------------------------- Configuration Descriptor *//* 9+8+120+55*2=247 */
/* 0 bLength, bDescriptorType                                */ 9,    0x02,
/* 2 wTotalLength                                            */ D0(247+14),D1(247+14),
/* 4 bNumInterfaces, bConfigurationValue                     */ 3,    1,
/* 6 iConfiguration                                          */ 0,
/* 7 bmAttributes, bMaxPower                                 */ 0x80, 50,

/* ------------------------ Interface Association Descriptor */
/* 0 bLength, bDescriptorType                                */ 8,    0x0B,
/* 2 bFirstInterface, bInterfaceCount                        */ 0,    3,
/* 4 bFunctionClass, bFunctionSubClass, bFunctionProtocol    */ 0x01, 0x00, 0x20,
/* 7 iFunction                                               */ 0,

/* ------------------------------------ Interface Descriptor *//* 0 Control (9+111=120) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 0,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x01, 0x20,
/* 8 iInterface                                              */ 0,
/* ---------------- Audio 2.0 AC Interface Header Descriptor *//* (9+8+17*2+18*2+12*2=111) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 9,                   0x24, 0x01,
/* 3 bcdADC, bCategory                                       */ D0(0x200),D1(0x200), 0x08,
/* 6 wTotalLength                                            */ D0(111),D1(111),
/* 8 bmControls                                              */ 0x00,
/* -------------------- Audio 2.0 AC Clock Source Descriptor */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x24, 0x0A,
/* 3 bClockID, bmAttributes, bmControls                      */ 0x10, 0x05, 0x01,
/* 6 bAssocTerminal, iClockSource                            */ 0x00, 0,
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
/* 4 bNumEndpoints                                           */ 2,
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
/* ------------------------------------- Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x01,            0x11,
/* 4  wMaxPacketSize, bInterval                               */ D0(4),D1(4), 1,

/* ------------------------------------ Interface Descriptor *//* 2 Stream OUT (9+9+16+6+7+8=55) */
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

/* -------------------------------- Configuration Descriptor *//* 9+8+120+55*2=247 */
/* 0 bLength, bDescriptorType                                */ 9,    0x02,
/* 2 wTotalLength                                            */ D0(247+14),D1(247+14),
/* 4 bNumInterfaces, bConfigurationValue                     */ 3,    1,
/* 6 iConfiguration                                          */ 0,
/* 7 bmAttributes, bMaxPower                                 */ 0x80, 50,

/* ------------------------ Interface Association Descriptor */
/* 0 bLength, bDescriptorType                                */ 8,    0x0B,
/* 2 bFirstInterface, bInterfaceCount                        */ 0,    3,
/* 4 bFunctionClass, bFunctionSubClass, bFunctionProtocol    */ 0x01, 0x00, 0x20,
/* 7 iFunction                                               */ 0,

/* ------------------------------------ Interface Descriptor *//* 0 Control (9+111=120) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 0,    0,
/* 4 bNumEndpoints                                           */ 0,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x01, 0x20,
/* 8 iInterface                                              */ 0,
/* ---------------- Audio 2.0 AC Interface Header Descriptor *//* (9+8+17*2+18*2+12*2=111) */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 9,                   0x24, 0x01,
/* 3 bcdADC, bCategory                                       */ D0(0x200),D1(0x200), 0x08,
/* 6 wTotalLength                                            */ D0(111),D1(111),
/* 8 bmControls                                              */ 0x00,
/* -------------------- Audio 2.0 AC Clock Source Descriptor */
/* 0 bLength, bDescriptorType, bDescriptorSubtype            */ 8,    0x24, 0x0A,
/* 3 bClockID, bmAttributes, bmControls                      */ 0x10, 0x05, 0x01,
/* 6 bAssocTerminal, iClockSource                            */ 0x00, 0,
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
/* 4 bNumEndpoints                                           */ 2,
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
/* ------------------------------------- Endpoint Descriptor */
/* 0  bLength, bDescriptorType                                */ 7,               0x05,
/* 2  bEndpointAddress, bmAttributes                          */ 0x01,            0x11,
/* 4  wMaxPacketSize, bInterval                               */ D0(4),D1(4), 1,

/* ------------------------------------ Interface Descriptor *//* 2 Stream OUT (9+9+16+6+7+8=55) */
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

static VOID ux_device_class_audio_feedback_hook(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *p = (UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *)params;
UX_SLAVE_TRANSFER                                 *transfer = (UX_SLAVE_TRANSFER *)p -> parameter;
ULONG                                             length;

    (void)action;
    (void)params;
    (void)p;
    (void)transfer;
    length = UX_MIN(4, transfer->ux_slave_transfer_request_requested_length);
    if (transfer->ux_slave_transfer_request_endpoint->ux_slave_endpoint_descriptor.bEndpointAddress & 0x80)
    {
        device_audio_rx_feedback_count ++;
        /* IN for OUT(RX/read) feedback.  */
        device_audio_rx_feedback = 0;
        _ux_utility_memory_copy(device_audio_rx_feedback_u8a,
            transfer->ux_slave_transfer_request_data_pointer, length);
        transfer -> ux_slave_transfer_request_completion_code = UX_SUCCESS;
        transfer -> ux_slave_transfer_request_actual_length = length;
    }
    else
    {
        device_audio_tx_feedback_count ++;
        /* OUT for IN(TX/write) feedback.  */
        _ux_utility_memory_copy(transfer->ux_slave_transfer_request_data_pointer,
           device_audio_tx_feedback_u8a, length);
        transfer -> ux_slave_transfer_request_completion_code = UX_SUCCESS;
        transfer -> ux_slave_transfer_request_actual_length = length;
    }
    _ux_utility_semaphore_put(&transfer->ux_slave_transfer_request_semaphore);
    _ux_utility_thread_sleep(1);
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
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .action_func = ux_device_class_audio_feedback_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x82,
        .req_status = UX_SUCCESS,
        .status = UX_SUCCESS,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .action_func = ux_device_class_audio_feedback_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x01,
        .req_status = UX_SUCCESS,
        .status = UX_SUCCESS,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
{ 0 },
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

UX_HOST_CLASS_DUMMY *dummy = (UX_HOST_CLASS_DUMMY *) inst;


    switch(event)
    {

        case UX_DEVICE_INSERTION:

            // printf("hINS:%p,%p:%ld\n", cls, inst, dummy -> ux_host_class_dummy_interface -> ux_interface_descriptor.bInterfaceNumber);
            switch(dummy -> ux_host_class_dummy_interface -> ux_interface_descriptor.bInterfaceNumber)
            {
            case 0: dummy_control = dummy; break;
            case 1: dummy_rx      = dummy; break;
            case 2: dummy_tx      = dummy; break;
            }
            break;

        case UX_DEVICE_REMOVAL:

            // printf("hRMV:%p,%p:%ld\n", cls, inst, dummy -> ux_host_class_dummy_interface -> ux_interface_descriptor.bInterfaceNumber);
            switch(dummy -> ux_host_class_dummy_interface -> ux_interface_descriptor.bInterfaceNumber)
            {
            case 0: dummy_control = UX_NULL; break;
            case 1: dummy_rx      = UX_NULL; break;
            case 2: dummy_tx      = UX_NULL; break;
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

    rsc_mem_free_on_set_cfg = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_audio20_device_feedback_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    /* Inform user.  */
    printf("Running Audio 2.0 Device Feedback Functionality Test................ ");
#if !UX_TEST_MULTI_IFC_ON || !UX_TEST_MULTI_ALT_ON || !UX_TEST_MULTI_CLS_ON || !UX_TEST_MULTI_EP_OVER(4) ||\
    !defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT) ||\
    (UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH < (247 + 14))
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
    status  = ux_host_stack_class_register(_ux_host_class_dummy_name, _ux_host_class_dummy_entry);
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

    /* Initialize the device Audio class. This class owns interfaces starting with 0, 1, 2. */
    status  = ux_device_stack_class_register(_ux_system_slave_class_audio_name, ux_device_class_audio_entry,
                                             1, 0,  &slave_audio_parameter);
    UX_TEST_CHECK_SUCCESS(status);

    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();
    UX_TEST_CHECK_SUCCESS(status);

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
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

void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               test_n;
UX_DEVICE                                           *device;
UX_CONFIGURATION                                    *configuration;
UX_INTERFACE                                        *interface;
UX_INTERFACE                                        *interface_inst[3][2];
UX_ENDPOINT                                         *control_endpoint;
UX_TRANSFER                                         *transfer_request;
UCHAR                                               test_tmp[32];
ULONG                                               temp;


    /* Test connect.  */
    status  = test_wait_until_not_null((void**)&dummy_control, 100);
    status |= test_wait_until_not_null((void**)&dummy_tx, 100);
    status |= test_wait_until_not_null((void**)&dummy_rx, 100);
    status |= test_wait_until_not_null((void**)&slave_audio_tx_stream, 100);
    status |= test_wait_until_not_null((void**)&slave_audio_rx_stream, 100);
    UX_TEST_CHECK_SUCCESS(status);

    /* Get testing instances.  */

    status = ux_host_stack_device_get(0, &device);
    UX_TEST_CHECK_SUCCESS(status);

    control_endpoint = &device->ux_device_control_endpoint;
    transfer_request = &control_endpoint->ux_endpoint_transfer_request;

    status = ux_host_stack_device_configuration_get(device, 0, &configuration);
    UX_TEST_CHECK_SUCCESS(status);

    interface = configuration -> ux_configuration_first_interface;
    while(interface)
    {
        // printf("Interface: %ld.%ld\n", interface -> ux_interface_descriptor.bInterfaceNumber, interface -> ux_interface_descriptor.bAlternateSetting);
        interface_inst[interface -> ux_interface_descriptor.bInterfaceNumber][interface -> ux_interface_descriptor.bAlternateSetting] = interface;
        interface = interface -> ux_interface_next_interface;
    }

    ux_test_link_hooks_from_array(ux_device_class_audio_transfer_hook);

    /* Test interface change.  */
    /* Reset log.  */
    RESET_CALLBACK_INVOKE_LOG();
    status  = ux_host_stack_interface_setting_select(interface_inst[1][1]);
    status |= ux_host_stack_interface_setting_select(interface_inst[1][0]);
    status |= ux_host_stack_interface_setting_select(interface_inst[1][1]);

    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 3);
    UX_TEST_ASSERT(callback_invoke_log[0].func == slave_audio_tx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[0].param1 == slave_audio_tx_stream);
    UX_TEST_ASSERT(callback_invoke_log[0].param2 == (VOID*)1);
    UX_TEST_ASSERT(callback_invoke_log[1].func == slave_audio_tx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[1].param1 == slave_audio_tx_stream);
    UX_TEST_ASSERT(callback_invoke_log[1].param2 == 0);

    RESET_CALLBACK_INVOKE_LOG();
    status  = ux_host_stack_interface_setting_select(interface_inst[2][1]);
    status |= ux_host_stack_interface_setting_select(interface_inst[2][0]);
    status |= ux_host_stack_interface_setting_select(interface_inst[2][1]);

    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 3);
    UX_TEST_ASSERT(callback_invoke_log[0].func == slave_audio_rx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[0].param1 == slave_audio_rx_stream);
    UX_TEST_ASSERT(callback_invoke_log[0].param2 == (VOID*)1);
    UX_TEST_ASSERT(callback_invoke_log[1].func == slave_audio_rx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[1].param1 == slave_audio_rx_stream);
    UX_TEST_ASSERT(callback_invoke_log[1].param2 == 0);

    /**************************************************************************/

    /* Control requests test.  */

    transfer_request -> ux_transfer_request_data_pointer =      buffer;

    test_tmp[0] = 2;
    test_tmp[1] = 5;

    for (test_n = 0; test_n < 2; test_n ++)
    {
        transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_CUR;
        transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;

        /* Issue SetFeatureMute(test_tmp[test_n], 0). */
        transfer_request -> ux_transfer_request_requested_length =  1;
        transfer_request -> ux_transfer_request_index =             (test_tmp[test_n] << 8) | 0;
        transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_FU_MUTE_CONTROL << 8) | 0;
        buffer[0] = 0;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_changed == 0);

        /* Issue SetFeatureMute(test_tmp[test_n], 1). */
        buffer[0] = 1;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_MUTE_CHANGED);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_mute[0] == 1);

        /* Issue SetFeatureMute(test_tmp[test_n], 0). */
        buffer[0] = 0;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_MUTE_CHANGED);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_mute[0] == 0);

        /* Issue SetFeatureMute(test_tmp[test_n], 1). */
        buffer[0] = 1;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_MUTE_CHANGED);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_mute[0] == 1);

        /* Issue SetFeatureVolume(test_tmp[test_n], 0x0003). */
        transfer_request -> ux_transfer_request_requested_length =  2;
        transfer_request -> ux_transfer_request_index =             (test_tmp[test_n] << 8) | 0;
        transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_FU_VOLUME_CONTROL << 8) | 0;
        buffer[0] = 3; buffer[1] = 0;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_VOLUME_CHANGED);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_volume[0] == 0x0003);

        /* Issue SetFeatureVolume(test_tmp[test_n], 0x0103). */
        buffer[1] = 1;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_VOLUME_CHANGED);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_volume[0] == 0x0103);

        /* Issue SetFeatureVolume(test_tmp[test_n], 0x0103). */
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(g_slave_audio20_control[test_n].ux_device_class_audio20_control_changed == 0);

        /* Issue GetFeatureMute(test_tmp[test_n]).  */
        transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_CUR;
        transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;

        transfer_request -> ux_transfer_request_requested_length =  1;
        transfer_request -> ux_transfer_request_index =             (test_tmp[test_n] << 8) | 0;
        transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_FU_MUTE_CONTROL << 8) | 0;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(transfer_request -> ux_transfer_request_actual_length == 1);
        UX_TEST_ASSERT(buffer[0] == 1);

        /* Issue GetFeatureMute(test_tmp[test_n]).  */
        transfer_request -> ux_transfer_request_requested_length =  2;
        transfer_request -> ux_transfer_request_index =             (test_tmp[test_n] << 8) | 0;
        transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_FU_VOLUME_CONTROL << 8) | 0;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(transfer_request -> ux_transfer_request_actual_length == 2);
        UX_TEST_ASSERT(buffer[0] == 3);
        UX_TEST_ASSERT(buffer[1] == 1);

        /* Issue GetClockSamplingFrequency(0x10).  */
        transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_CUR;
        transfer_request -> ux_transfer_request_requested_length =  4;
        transfer_request -> ux_transfer_request_index =             (0x10 << 8) | 0;
        transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8) | 0;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(transfer_request -> ux_transfer_request_actual_length == 4);
        UX_TEST_ASSERT(buffer[0] == D0(48000));
        UX_TEST_ASSERT(buffer[1] == D1(48000));
        UX_TEST_ASSERT(buffer[2] == D2(48000));
        UX_TEST_ASSERT(buffer[3] == D3(48000));

        /* Issue GetClockSamplingFrequencyRange(0x10).  */
        transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_RANGE;
        transfer_request -> ux_transfer_request_requested_length =  14;
        transfer_request -> ux_transfer_request_index =             (0x10 << 8) | 0;
        transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8) | 0;
        status = ux_host_stack_transfer_request(transfer_request);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(transfer_request -> ux_transfer_request_actual_length == 14);
        UX_TEST_ASSERT(buffer[ 0] == D0(1));
        UX_TEST_ASSERT(buffer[ 1] == D1(1));
        UX_TEST_ASSERT(buffer[ 2] == D0(48000));
        UX_TEST_ASSERT(buffer[ 3] == D1(48000));
        UX_TEST_ASSERT(buffer[ 4] == D2(48000));
        UX_TEST_ASSERT(buffer[ 5] == D3(48000));
        UX_TEST_ASSERT(buffer[ 6] == D0(48000));
        UX_TEST_ASSERT(buffer[ 7] == D1(48000));
        UX_TEST_ASSERT(buffer[ 8] == D2(48000));
        UX_TEST_ASSERT(buffer[ 9] == D3(48000));
    }

    /* Issue SetClockSamplingFrequency(0x10) - STALL.  */
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_CUR;
    transfer_request -> ux_transfer_request_index =             (0x10 << 8) | 0;
    transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8) | 0;
    status = ux_host_stack_transfer_request(transfer_request);
    UX_TEST_CHECK_CODE(UX_TRANSFER_STALLED, status);

    /* Prepare new RANGE settings.  */
    g_slave_audio20_control[0].ux_device_class_audio20_control_sampling_frequency = 0;
    g_slave_audio20_control[0].ux_device_class_audio20_control_sampling_frequency_cur = 44100;
    g_slave_audio20_control[0].ux_device_class_audio20_control_sampling_frequency_range = audio20_cs_range;

    /* Issue GetClockSamplingFrequency(0x10).  */
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_CUR;
    transfer_request -> ux_transfer_request_requested_length =  4;
    transfer_request -> ux_transfer_request_index =             (0x10 << 8) | 0;
    transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8) | 0;
    status = ux_host_stack_transfer_request(transfer_request);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(transfer_request -> ux_transfer_request_actual_length == 4);
    UX_TEST_ASSERT(buffer[0] == D0(44100));
    UX_TEST_ASSERT(buffer[1] == D1(44100));
    UX_TEST_ASSERT(buffer[2] == D2(44100));
    UX_TEST_ASSERT(buffer[3] == D3(44100));

    /* Issue GetClockSamplingFrequencyRange(0x10).  */
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_RANGE;
    transfer_request -> ux_transfer_request_requested_length =  14;
    transfer_request -> ux_transfer_request_index =             (0x10 << 8) | 0;
    transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8) | 0;
    status = ux_host_stack_transfer_request(transfer_request);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(transfer_request -> ux_transfer_request_actual_length == sizeof(audio20_cs_range));
    UX_TEST_ASSERT(buffer[ 0] == D0(2));
    UX_TEST_ASSERT(buffer[ 1] == D1(2));
    UX_TEST_ASSERT(buffer[ 2] == D0(44100));
    UX_TEST_ASSERT(buffer[ 3] == D1(44100));
    UX_TEST_ASSERT(buffer[ 4] == D2(44100));
    UX_TEST_ASSERT(buffer[ 5] == D3(44100));
    UX_TEST_ASSERT(buffer[ 6] == D0(44100));
    UX_TEST_ASSERT(buffer[ 7] == D1(44100));
    UX_TEST_ASSERT(buffer[ 8] == D2(44100));
    UX_TEST_ASSERT(buffer[ 9] == D3(44100));
    UX_TEST_ASSERT(UX_SUCCESS == ux_utility_memory_compare(audio20_cs_range, buffer, sizeof(audio20_cs_range)));

    /* Issue SetClockSamplingFrequency(0x10) - STALL.  */
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_CUR;
    transfer_request -> ux_transfer_request_index =             (0x10 << 8) | 0;
    transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8) | 0;
    ux_utility_long_put(transfer_request -> ux_transfer_request_data_pointer, 64000);
    status = ux_host_stack_transfer_request(transfer_request);
    UX_TEST_CHECK_CODE(UX_TRANSFER_STALLED, status);

    /* Issue SetClockSamplingFrequency(0x10) - OK.  */
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_OUT | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_CUR;
    transfer_request -> ux_transfer_request_index =             (0x10 << 8) | 0;
    transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8) | 0;
    ux_utility_long_put(transfer_request -> ux_transfer_request_data_pointer, 48000);
    status = ux_host_stack_transfer_request(transfer_request);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(g_slave_audio20_control[0].ux_device_class_audio20_control_changed == UX_DEVICE_CLASS_AUDIO20_CONTROL_FREQUENCY_CHANGED);
    UX_TEST_ASSERT(g_slave_audio20_control[0].ux_device_class_audio20_control_sampling_frequency_cur == 48000);

    /* Issue GetClockSamplingFrequency(0x10) - OK.  */
    transfer_request -> ux_transfer_request_type =              UX_REQUEST_IN | UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
    transfer_request -> ux_transfer_request_function =          UX_DEVICE_CLASS_AUDIO20_CUR;
    transfer_request -> ux_transfer_request_index =             (0x10 << 8) | 0;
    transfer_request -> ux_transfer_request_value =             (UX_DEVICE_CLASS_AUDIO20_CS_SAM_FREQ_CONTROL << 8) | 0;
    status = ux_host_stack_transfer_request(transfer_request);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(transfer_request -> ux_transfer_request_actual_length == 4);
    UX_TEST_ASSERT(buffer[0] == D0(48000));
    UX_TEST_ASSERT(buffer[1] == D1(48000));
    UX_TEST_ASSERT(buffer[2] == D2(48000));
    UX_TEST_ASSERT(buffer[3] == D3(48000));

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

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
    }
}
