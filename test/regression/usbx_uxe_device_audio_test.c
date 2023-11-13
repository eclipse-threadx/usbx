/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_audio.h"
#include "ux_device_class_audio10.h"
#include "ux_device_class_audio20.h"

#include "ux_test.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;

static UX_DEVICE_CLASS_AUDIO                    *slave_audio;
static UX_DEVICE_CLASS_AUDIO_PARAMETER           slave_audio_parameter;
static UX_DEVICE_CLASS_AUDIO_STREAM_PARAMETER    slave_audio_stream_parameter[2];

static UX_DEVICE_CLASS_AUDIO                    *slave_audio_tx;
static UX_DEVICE_CLASS_AUDIO_STREAM             *slave_audio_tx_stream;
static UX_DEVICE_CLASS_AUDIO_PARAMETER           slave_audio_tx_parameter;
static UX_DEVICE_CLASS_AUDIO_STREAM_PARAMETER    slave_audio_tx_stream_parameter;

static UX_DEVICE_CLASS_AUDIO                    *slave_audio_rx;
static UX_DEVICE_CLASS_AUDIO_STREAM             *slave_audio_rx_stream;
static UX_DEVICE_CLASS_AUDIO_PARAMETER           slave_audio_rx_parameter;
static UX_DEVICE_CLASS_AUDIO_STREAM_PARAMETER    slave_audio_rx_stream_parameter;
static UX_SLAVE_TRANSFER                        *slave_audio_rx_transfer;

/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);

/* Prototype for test control return.  */

void  test_control_return(UINT status);

#define D3(d) ((UCHAR)((d) >> 24))
#define D2(d) ((UCHAR)((d) >> 16))
#define D1(d) ((UCHAR)((d) >> 8))
#define D0(d) ((UCHAR)((d) >> 0))

static unsigned char device_framework_full_speed[] = {

/* --------------------------------------- Device Descriptor */
/* 0  bLength, bDescriptorType                               */ 18,   0x01,
/* 2  bcdUSB                                                 */ D0(0x200),D1(0x200),
/* 4  bDeviceClass, bDeviceSubClass, bDeviceProtocol         */ 0x00, 0x00, 0x00,
/* 7  bMaxPacketSize0                                        */ 0x08,
/* 8  idVendor, idProduct                                    */ 0x84, 0x84, 0x01, 0x00,
/* 12 bcdDevice                                              */ D0(0x100),D1(0x100),
/* 14 iManufacturer, iProduct, iSerialNumber                 */ 0,    0,    0,
/* 17 bNumConfigurations                                     */ 1,

/* -------------------------------- Configuration Descriptor *//* 9+8+88+52*2=209 */
/* 0 bLength, bDescriptorType                                */ 9,    0x02,
/* 2 wTotalLength                                            */ D0(209),D1(209),
/* 4 bNumInterfaces, bConfigurationValue                     */ 3,    1,
/* 6 iConfiguration                                          */ 0,
/* 7 bmAttributes, bMaxPower                                 */ 0x80, 50,

/* ------------------------ Interface Association Descriptor */
/* 0 bLength, bDescriptorType                                */ 8,    0x0B,
/* 2 bFirstInterface, bInterfaceCount                        */ 0,    3,
/* 4 bFunctionClass, bFunctionSubClass, bFunctionProtocol    */ 0x01, 0x01, 0x00,
/* 7 iFunction                                               */ 0,

/* ------------------------------------ Interface Descriptor *//* 0 Control (9+72+7=88) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 0,    0,
/* 4 bNumEndpoints                                           */ 1,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x01, 0x00,
/* 8 iInterface                                              */ 0,
/* ---------------- Audio 1.0 AC Interface Header Descriptor *//* (10+12*2+10*2+9*2=72) */
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
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

static unsigned char device_framework_high_speed[] = {
/* --------------------------------------- Device Descriptor */
/* 0  bLength, bDescriptorType                               */ 18,   0x01,
/* 2  bcdUSB                                                 */ D0(0x200),D1(0x200),
/* 4  bDeviceClass, bDeviceSubClass, bDeviceProtocol         */ 0x00, 0x00, 0x00,
/* 7  bMaxPacketSize0                                        */ 8,
/* 8  idVendor, idProduct                                    */ 0x84, 0x84, 0x01, 0x00,
/* 12 bcdDevice                                              */ D0(0x100),D1(0x100),
/* 14 iManufacturer, iProduct, iSerialNumber                 */ 0,    0,    0,
/* 17 bNumConfigurations                                     */ 1,

/* ----------------------------- Device Qualifier Descriptor */
/* 0 bLength, bDescriptorType                                */ 10,                 0x06,
/* 2 bcdUSB                                                  */ D0(0x200),D1(0x200),
/* 4 bDeviceClass, bDeviceSubClass, bDeviceProtocol          */ 0x00,               0x00, 0x00,
/* 7 bMaxPacketSize0                                         */ 8,
/* 8 bNumConfigurations                                      */ 1,
/* 9 bReserved                                               */ 0,

/* -------------------------------- Configuration Descriptor *//* 9+8+88+52*2=209 */
/* 0 bLength, bDescriptorType                                */ 9,    0x02,
/* 2 wTotalLength                                            */ D0(209),D1(209),
/* 4 bNumInterfaces, bConfigurationValue                     */ 3,    1,
/* 6 iConfiguration                                          */ 0,
/* 7 bmAttributes, bMaxPower                                 */ 0x80, 50,

/* ------------------------ Interface Association Descriptor */
/* 0 bLength, bDescriptorType                                */ 8,    0x0B,
/* 2 bFirstInterface, bInterfaceCount                        */ 0,    3,
/* 4 bFunctionClass, bFunctionSubClass, bFunctionProtocol    */ 0x01, 0x01, 0x00,
/* 7 iFunction                                               */ 0,

/* ------------------------------------ Interface Descriptor *//* 0 Control (9+72+7=88) */
/* 0 bLength, bDescriptorType                                */ 9,    0x04,
/* 2 bInterfaceNumber, bAlternateSetting                     */ 0,    0,
/* 4 bNumEndpoints                                           */ 1,
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ 0x01, 0x01, 0x00,
/* 8 iInterface                                              */ 0,
/* ---------------- Audio 1.0 AC Interface Header Descriptor *//* (10+12*2+10*2+9*2=72) */
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
/* 4  wMaxPacketSize, bInterval                               */ D0(8),D1(8),     4,

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
/* 4  wMaxPacketSize, bInterval, bRefresh, bSynchAddress      */ D0(256),D1(256), 4,    0, 0,
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
/* 4  wMaxPacketSize, bInterval, bRefresh, bSynchAddress      */ D0(256),D1(256), 4,    0, 0,
/* ---------- Audio 1.0 AS ISO Audio Data Endpoint Descriptor */
/* 0  bLength, bDescriptorType, bDescriptorSubtype            */ 7,                0x25, 0x01,
/* 3  bmAttributes                                            */ 0x00,
/* 5  bLockDelayUnits, wLockDelay                             */ 0x00, D0(0),D1(0),
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

static VOID    slave_audio_tx_activate(VOID *audio_instance){}
static VOID    slave_audio_deactivate(VOID *audio_instance){}
static VOID    slave_audio_tx_stream_change(UX_DEVICE_CLASS_AUDIO_STREAM *audio, ULONG alt){}
static UINT    slave_audio_control_process(UX_DEVICE_CLASS_AUDIO *audio, UX_SLAVE_TRANSFER *transfer)
{
    return(UX_ERROR);
}
static VOID    slave_audio_tx_done(UX_DEVICE_CLASS_AUDIO_STREAM *audio, ULONG length){}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_uxe_device_audio_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_device_audio APIs Test.................................. ");
#if !defined(UX_DEVICE_CLASS_AUDIO_ENABLE_ERROR_CHECKING)
#warning Tests skipped due to compile option!
    printf("SKIP SUCCESS!\n");
    test_control_return(0);
    return;
#endif

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    UX_TEST_CHECK_SUCCESS(status);

#if !defined(UX_DEVICE_STANDALONE)
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_thread_entry = ux_device_class_audio_write_thread_entry;
#else
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_task_function = _ux_device_class_audio_write_task_function;
#endif
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_change = slave_audio_tx_stream_change;
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_callbacks.ux_device_class_audio_stream_frame_done = slave_audio_tx_done;
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_max_frame_buffer_size = 256;
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_max_frame_buffer_nb   = 8;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams = &slave_audio_tx_stream_parameter;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams_nb = 1;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_callbacks.ux_slave_class_audio_instance_activate   = slave_audio_tx_activate;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_callbacks.ux_slave_class_audio_instance_deactivate = slave_audio_deactivate;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_callbacks.ux_device_class_audio_control_process = slave_audio_control_process;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_callbacks.ux_device_class_audio_arg             = UX_NULL;

    status  = ux_device_stack_class_register(_ux_system_slave_class_audio_name, ux_device_class_audio_entry,
                                             1, 1,  NX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams = NX_NULL;
    status  = ux_device_stack_class_register(_ux_system_slave_class_audio_name, ux_device_class_audio_entry,
                                             1, 1,  &slave_audio_tx_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams = &slave_audio_tx_stream_parameter;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams_nb = 0;
    status  = ux_device_stack_class_register(_ux_system_slave_class_audio_name, ux_device_class_audio_entry,
                                            1, 1,  &slave_audio_tx_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams = &slave_audio_tx_stream_parameter;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams_nb = 1;
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_max_frame_buffer_nb   = 0;
    status  = ux_device_stack_class_register(_ux_system_slave_class_audio_name, ux_device_class_audio_entry,
                                            1, 1,  &slave_audio_tx_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams = &slave_audio_tx_stream_parameter;
    slave_audio_tx_parameter.ux_device_class_audio_parameter_streams_nb = 1;
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_max_frame_buffer_nb   = 1;
    slave_audio_tx_stream_parameter.ux_device_class_audio_stream_parameter_max_frame_buffer_size = 0;
    status  = ux_device_stack_class_register(_ux_system_slave_class_audio_name, ux_device_class_audio_entry,
                                            1, 1,  &slave_audio_tx_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* Create the simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test simulation", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}


static void  ux_test_thread_simulation_0_entry(ULONG arg)
{
UINT                                    status;
UX_DEVICE_CLASS_AUDIO                   dummy_audio_inst;
UX_DEVICE_CLASS_AUDIO                   *dummy_audio = &dummy_audio_inst;
UX_DEVICE_CLASS_AUDIO_STREAM            dummy_stream_inst;
UX_DEVICE_CLASS_AUDIO_STREAM            *dummy_stream = &dummy_stream_inst;
UCHAR                                   *dummy_buffer;
ULONG                                   dummy_dw;
UX_SLAVE_TRANSFER                       dummy_transfer; 
UX_DEVICE_CLASS_AUDIO10_CONTROL_GROUP   dummy_group10;
UX_DEVICE_CLASS_AUDIO20_CONTROL_GROUP   dummy_group20;

    /* ux_device_class_audio_stream_get()  */
    status = ux_device_class_audio_stream_get(UX_NULL, 0, &dummy_stream);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_stream_get(dummy_audio, 0, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_stream = &dummy_stream_inst;

    /* ux_device_class_audio_reception_start()  */
    status = ux_device_class_audio_reception_start(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_sample_read8()  */
    status = ux_device_class_audio_sample_read8(UX_NULL, (UCHAR *)&dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_sample_read16()  */
    status = ux_device_class_audio_sample_read16(UX_NULL, (USHORT *)&dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_sample_read24()  */
    status = ux_device_class_audio_sample_read24(UX_NULL, &dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_sample_read32()  */
    status = ux_device_class_audio_sample_read32(UX_NULL, &dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_read_frame_get()  */
    status = ux_device_class_audio_read_frame_get(UX_NULL, &dummy_buffer, &dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_read_frame_get(dummy_stream, UX_NULL, &dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_read_frame_get(dummy_stream, &dummy_buffer, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_read_frame_free()  */
    status = ux_device_class_audio_read_frame_free(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_transmission_start()  */
    status = ux_device_class_audio_transmission_start(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_frame_write()  */
    status = ux_device_class_audio_frame_write(UX_NULL, dummy_buffer, 4);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_frame_write(dummy_stream, UX_NULL, 4);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_frame_write(dummy_stream, dummy_buffer, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_write_frame_get()  */
    status = ux_device_class_audio_write_frame_get(UX_NULL, &dummy_buffer, &dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_write_frame_get(dummy_stream, UX_NULL, &dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_write_frame_get(dummy_stream, &dummy_buffer, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_write_frame_commit()  */
    status = ux_device_class_audio_write_frame_commit(UX_NULL, 1);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_write_frame_commit(dummy_stream, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_ioctl()  */
    status = ux_device_class_audio_ioctl(UX_NULL, 0, &dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_feedback_get()  */
    status = ux_device_class_audio_feedback_get(UX_NULL, (UCHAR *)&dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_feedback_get(dummy_stream, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_feedback_set()  */
    status = ux_device_class_audio_feedback_set(UX_NULL, (UCHAR *)&dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_feedback_set(dummy_stream, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio_interrupt_send()  */
    status = ux_device_class_audio_interrupt_send(UX_NULL, (UCHAR *)&dummy_dw);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio_interrupt_send(dummy_audio, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio10_control_process()  */
    status = ux_device_class_audio10_control_process(UX_NULL, &dummy_transfer, &dummy_group10);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio10_control_process(dummy_audio, UX_NULL, &dummy_group10);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio10_control_process(dummy_audio, &dummy_transfer, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* ux_device_class_audio20_control_process()  */
    status = ux_device_class_audio20_control_process(UX_NULL, &dummy_transfer, &dummy_group20);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio20_control_process(dummy_audio, UX_NULL, &dummy_group20);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    status = ux_device_class_audio20_control_process(dummy_audio, &dummy_transfer, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }
}
