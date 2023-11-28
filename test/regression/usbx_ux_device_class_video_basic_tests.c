/* This test is designed to test the simple video host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"

#include "fx_api.h"

#include "ux_device_class_video.h"
#include "ux_device_stack.h"

#include "ux_host_class_video.h"
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
#define                             UX_DEMO_ENDPOINT_SIZE 480

#define                             UX_TEST_LOG_SIZE    (64)


/* Define local/extern function prototypes.  */
static void        test_thread_entry(ULONG);
static TX_THREAD   tx_test_thread_host_simulation;
static TX_THREAD   tx_test_thread_slave_simulation;
static void        tx_test_thread_host_simulation_entry(ULONG);
static void        tx_test_thread_slave_simulation_entry(ULONG);


/* Define global data structures.  */
static UCHAR                                    usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

static UX_HOST_CLASS_DUMMY                      *dummy_control;
static UX_HOST_CLASS_DUMMY                      *dummy_tx;
static UX_HOST_CLASS_DUMMY                      *dummy_rx;

static UX_DEVICE_CLASS_VIDEO                    *device_video;
static UX_DEVICE_CLASS_VIDEO_STREAM             *device_video_tx_stream;
static UX_DEVICE_CLASS_VIDEO_STREAM             *device_video_rx_stream;
static UX_DEVICE_CLASS_VIDEO_PARAMETER           device_video_parameter;
static UX_DEVICE_CLASS_VIDEO_STREAM_PARAMETER    device_video_stream_parameter[2];

static UX_SLAVE_TRANSFER                        *device_video_tx_transfer;
static UX_SLAVE_TRANSFER                        *device_video_rx_transfer;

static UX_HOST_CLASS_VIDEO                      *video;

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

static ULONG                               rsc_video_sem_usage;
static ULONG                               rsc_video_sem_get_count;
static ULONG                               rsc_video_mutex_usage;
static ULONG                               rsc_video_mem_usage;

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

#define W(d)    UX_DW0(d), UX_DW1(d)
#define DW(d)   UX_DW0(d), UX_DW1(d), UX_DW2(d), UX_DW3(d)

#define _DEVICE_DESCRIPTOR()                                                                        \
/* --------------------------------------- Device Descriptor */                                     \
/* 0  bLength, bDescriptorType                               */ 18,   0x01,                         \
/* 2  bcdUSB                                                 */ UX_DW0(0x200),UX_DW1(0x200),        \
/* 4  bDeviceClass, bDeviceSubClass, bDeviceProtocol         */ 0x00, 0x00, 0x00,                   \
/* 7  bMaxPacketSize0                                        */ 0x08,                               \
/* 8  idVendor, idProduct                                    */ 0x84, 0x84, 0x01, 0x00,             \
/* 12 bcdDevice                                              */ UX_DW0(0x100),UX_DW1(0x100),        \
/* 14 iManufacturer, iProduct, iSerialNumber                 */ 0,    0,    0,                      \
/* 17 bNumConfigurations                                     */ 1,

#define _DEVICE_QUALIFIER_DESCRIPTOR()                                                              \
/* ----------------------------- Device Qualifier Descriptor */                                     \
/* 0 bLength, bDescriptorType                                */ 10,                 0x06,           \
/* 2 bcdUSB                                                  */ UX_DW0(0x200),UX_DW1(0x200),        \
/* 4 bDeviceClass, bDeviceSubClass, bDeviceProtocol          */ 0x00,               0x00, 0x00,     \
/* 7 bMaxPacketSize0                                         */ 8,                                  \
/* 8 bNumConfigurations                                      */ 1,                                  \
/* 9 bReserved                                               */ 0,

#define _CONFIGURE_DESCRIPTOR(total_len,n_ifc,cfg_v)                                                \
/* -------------------------------- Configuration Descriptor */                                     \
/* 0 bLength, bDescriptorType                                */ 9,    0x02,                         \
/* 2 wTotalLength                                            */ UX_DW0(total_len),UX_DW1(total_len),\
/* 4 bNumInterfaces, bConfigurationValue                     */ (n_ifc), (cfg_v),                   \
/* 6 iConfiguration                                          */ 0,                                  \
/* 7 bmAttributes, bMaxPower                                 */ 0x80, 50,

#define _IAD_DESCRIPTOR(ifc_0,ifc_cnt,cls,sub,protocol)                                             \
/* ------------------------ Interface Association Descriptor */                                     \
/* 0 bLength, bDescriptorType                                */ 8,    0x0B,                         \
/* 2 bFirstInterface, bInterfaceCount                        */ (ifc_0), (ifc_cnt),                 \
/* 4 bFunctionClass, bFunctionSubClass, bFunctionProtocol    */ (cls), (sub), (protocol),           \
/* 7 iFunction                                               */ 0,

#define _INTERFACE_DESCRIPTOR(ifc,alt,n_ep,cls,sub,protocol)                                        \
/* ------------------------------------ Interface Descriptor */                                     \
/* 0 bLength, bDescriptorType                                */ 9,    0x04,                         \
/* 2 bInterfaceNumber, bAlternateSetting                     */ (ifc), (alt),                       \
/* 4 bNumEndpoints                                           */ (n_ep),                             \
/* 5 bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol */ (cls), (sub), (protocol),           \
/* 8 iInterface                                              */ 0,

#define _ENDPOINT_DESCRIPTOR(addr,attr,pkt_siz,interval)                                            \
/* ------------------------------------- Endpoint Descriptor */                                     \
/* 0  bLength, bDescriptorType                                */ 7,               0x05,             \
/* 2  bEndpointAddress, bmAttributes                          */ (addr),          (attr),           \
/* 4  wMaxPacketSize, bInterval                               */ UX_DW0(pkt_siz),UX_DW1(pkt_siz),(interval),

#define _VC_DESCRIPTORS_LEN (14+17+9+17+9)
#define _VC_DESCRIPTORS()                                                                           \
    /*--------------------------- Class VC Interface Descriptor (VC_HEADER).  */                    \
    14, 0x24, 0x01, W(0x150),                                                                       \
    W(_VC_DESCRIPTORS_LEN), /* wTotalLength.  */                                                    \
    DW(6000000),  /* dwClockFrequency.  */                                                          \
    2, /* bInCollection.  */                                                                        \
    1, 2, /* BaInterfaceNr(2).  */                                                                  \
    /*--------------------------- Input Terminal (VC_INPUT_TERMINAL, Camera)  */                    \
    17, 0x24, 0x02,                                                                                 \
    0x01, /* bTerminalID, ITT_CAMERA  */                                                            \
    W(0x201), /* wTerminalType  */                                                                  \
    0x00, 0x00, W(0), W(0), W(0),                                                                   \
    0x02, W(0), /* bControlSize, bmControls  */                                                     \
    /*---------------------------- Output Terminal (VC_OUTPUT_TERMINAL, USB)  */                    \
    9, 0x24, 0x03,                                                                                  \
    0x02, /* bTerminalID  */                                                                        \
    W(0x0101), /* wTerminalType, TT_STREAMING  */                                                   \
    0x00, 0x01/* bSourceID  */, 0x00,                                                               \
    /*--------------------------- Input Terminal (VC_INPUT_TERMINAL, USB)  */                       \
    17, 0x24, 0x02,                                                                                 \
    0x03, /* bTerminalID  */                                                                        \
    W(0x101), /* wTerminalType, TT_STREAMING  */                                                    \
    0x00, 0x00, W(0), W(0), W(0),                                                                   \
    0x02, W(0), /* bControlSize, bmControls  */                                                     \
    /*---------------------------- Output Terminal (VC_OUTPUT_TERMINAL, DISPLAY)  */                \
    9, 0x24, 0x03,                                                                                  \
    0x04, /* bTerminalID  */                                                                        \
    W(0x0301), /* wTerminalType, OTT_DISPLAY  */                                                    \
    0x00, 0x03/* bSourceID  */, 0x00,

#if 0
    /*--------------------------------- Processing Unit (VC_PROCESSING_UNIT)  */                    \
    12, 0x24, 0x05,                                                                                 \
    , /* bUnitID  */                                                                                \
    , /* bSourceID  */                                                                              \
    W(0),                                                                                           \
    3 /* bControlSize  */, 0, 0, 0 /*bmControls  */,                                                \
    0x00, 0x00,                                                                                     \
    /*--------------------------------- Processing Unit (VC_PROCESSING_UNIT)  */                    \
    12, 0x24, 0x05,                                                                                 \
    , /* bUnitID  */                                                                                \
    , /* bSourceID  */                                                                              \
    W(0),                                                                                           \
    3 /* bControlSize  */, 0, 0, 0 /*bmControls  */,                                                \
    0x00, 0x00,                                                                                     \

#endif

#define _VS_IN_DESCRIPTORS_LEN (14+11+38)
#define _VS_IN_DESCRIPTORS()                                                                        \
    /*------------------------- Class VS Header Descriptor (VS_INPUT_HEADER)  */                    \
    14, 0x24, 0x01,                                                                                 \
    0X01, /* bNumFormats  */                                                                        \
    W(_VS_IN_DESCRIPTORS_LEN), /* wTotalLength  */                                                  \
    0x81, /* bEndpointAddress  */                                                                   \
    0x00,                                                                                           \
    0x02, /* bTerminalLink  */                                                                      \
    0x00, /* bStillCaptureMethod  */                                                                \
    0x00, 0x00, /* bTriggerSupport, bTriggerUsage  */                                               \
    0x01, 0x00, /* bControlSize, bmaControls  */                                                    \
    /*------------------------------- VS Format Descriptor (VS_FORMAT_MJPEG)  */                    \
    11, 0x24, 0x06,                                                                                 \
    0x01, /* bFormatIndex  */                                                                       \
    0x01, /* bNumFrameDescriptors  */                                                               \
    0x01, /* bmFlags  */                                                                            \
    0x01, /* bDefaultFrameIndex  */                                                                 \
    0x00, 0x00, 0x00, 0x00,                                                                         \
    /*--------------------------------- VS Frame Descriptor (VS_FRAME_MJPEG)  */                    \
    38, 0x24, 0x07,                                                                                 \
    0x01, /* bFrameIndex  */                                                                        \
    0x03, /* bmCapabilities  */                                                                     \
    W(176), W(144), /* wWidth, wHeight  */                                                          \
    DW(912384), DW(912384), /* dwMinBitRate, dwMaxBitRate  */                                       \
    DW(38016), /* dwMaxVideoFrameBufSize  */                                                        \
    DW(666666), /* dwDefaultFrameInterval  */                                                       \
    0x00, /* bFrameIntervalType  */                                                                 \
    DW(666666), DW(666666), DW(0), /* dwMinFrameInterval, dwMaxFrameInterval, dwFrameIntervalStep  */

#define _VS_OUT_DESCRIPTORS_LEN (11+11+38)
#define _VS_OUT_DESCRIPTORS()                                                                       \
    /*------------------------- Class VS Header Descriptor (VS_OUTPUT_HEADER)  */                   \
    11, 0x24, 0x02,                                                                                 \
    0x01, /* bNumFormats  */                                                                        \
    W(_VS_OUT_DESCRIPTORS_LEN), /* wTotalLength  */                                                 \
    0x02, /* bEndpointAddress  */                                                                   \
    0x00,                                                                                           \
    0x03, /* bTerminalLink  */                                                                      \
    0x01, 0x00, /* bControlSize, bmaControls  */                                                    \
    /*------------------------------- VS Format Descriptor (VS_FORMAT_MJPEG)  */                    \
    11, 0x24, 0x06,                                                                                 \
    0x01, /* bFormatIndex  */                                                                       \
    0x01, /* bNumFrameDescriptors  */                                                               \
    0x01, /* bmFlags  */                                                                            \
    0x01, /* bDefaultFrameIndex  */                                                                 \
    0x00, 0x00, 0x00, 0x00,                                                                         \
    /*--------------------------------- VS Frame Descriptor (VS_FRAME_MJPEG)  */                    \
    38, 0x24, 0x07,                                                                                 \
    0x01, /* bFrameIndex  */                                                                        \
    0x03, /* bmCapabilities  */                                                                     \
    W(176), W(144), /* wWidth, wHeight  */                                                          \
    DW(912384), DW(912384), /* dwMinBitRate, dwMaxBitRate  */                                       \
    DW(38016), /* dwMaxVideoFrameBufSize  */                                                        \
    DW(666666), /* dwDefaultFrameInterval  */                                                       \
    0x00, /* bFrameIntervalType  */                                                                 \
    DW(666666), DW(666666), DW(0), /* dwMinFrameInterval, dwMaxFrameInterval, dwFrameIntervalStep  */


#define _CONFIGURE_DESCRIPTORS_LEN (9+ 8+ 9+_VC_DESCRIPTORS_LEN+ 9+_VS_IN_DESCRIPTORS_LEN+9+7+ 9+_VS_OUT_DESCRIPTORS_LEN+9+7)

static unsigned char device_framework_full_speed[] = {
    _DEVICE_DESCRIPTOR()
     _CONFIGURE_DESCRIPTOR(_CONFIGURE_DESCRIPTORS_LEN,3,1)
      _IAD_DESCRIPTOR(0,3,0x0E,0x03,0x00)

       _INTERFACE_DESCRIPTOR(0,0,0,0x0E,0x01,0x01)
        _VC_DESCRIPTORS()

       _INTERFACE_DESCRIPTOR(1,0,0,0x0E,0x02,0x00)
        _VS_IN_DESCRIPTORS()
        _INTERFACE_DESCRIPTOR(1,1,1,0x0E,0x02,0x00)
        _ENDPOINT_DESCRIPTOR(0x81,0x05,UX_DEMO_ENDPOINT_SIZE,0x01)

       _INTERFACE_DESCRIPTOR(2,0,0,0x0E,0x02,0x00)
        _VS_OUT_DESCRIPTORS()
        _INTERFACE_DESCRIPTOR(2,1,1,0x0E,0x02,0x00)
        _ENDPOINT_DESCRIPTOR(0x02,0x05,UX_DEMO_ENDPOINT_SIZE,0x01)
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

static unsigned char device_framework_high_speed[] = {
    _DEVICE_DESCRIPTOR()
     _DEVICE_QUALIFIER_DESCRIPTOR()
     _CONFIGURE_DESCRIPTOR(_CONFIGURE_DESCRIPTORS_LEN,3,1)
      _IAD_DESCRIPTOR(0,3,0x0E,0x03,0x00)

       _INTERFACE_DESCRIPTOR(0,0,0,0x0E,0x01,0x01)
        _VC_DESCRIPTORS()

       _INTERFACE_DESCRIPTOR(1,0,0,0x0E,0x02,0x00)
        _VS_IN_DESCRIPTORS()
        _INTERFACE_DESCRIPTOR(1,1,1,0x0E,0x02,0x00)
        _ENDPOINT_DESCRIPTOR(0x81,0x05,UX_DEMO_ENDPOINT_SIZE,0x01)

       _INTERFACE_DESCRIPTOR(2,0,0,0x0E,0x02,0x00)
        _VS_OUT_DESCRIPTORS()
        _INTERFACE_DESCRIPTOR(2,1,1,0x0E,0x02,0x00)
        _ENDPOINT_DESCRIPTOR(0x02,0x05,UX_DEMO_ENDPOINT_SIZE,0x01)
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

#ifndef ux_device_class_video_payload_write
static UINT ux_device_class_video_payload_write(UX_DEVICE_CLASS_VIDEO_STREAM *stream, UCHAR *payload, ULONG length)
{

UX_SLAVE_ENDPOINT           *endpoint;
UX_SLAVE_DEVICE             *device;
UCHAR                       *next_payload_buffer;
ULONG                       payload_buffer_size;


    /* Get the pointer to the device.  */
    device =  &_ux_system_slave -> ux_system_slave_device;

    /* As long as the device is in the CONFIGURED state.  */
    if (device -> ux_slave_device_state != UX_DEVICE_CONFIGURED)
    {

        /* Cannot proceed with command, the interface is down.  */
        return(UX_CONFIGURATION_HANDLE_UNKNOWN);
    }

    /* Check if endpoint is available.  */
    endpoint = stream -> ux_device_class_video_stream_endpoint;
    if (endpoint == UX_NULL)
        return(UX_ERROR);

    /* Check if endpoint direction is OK (IN).  */
    if ((endpoint -> ux_slave_endpoint_descriptor.bEndpointAddress & UX_ENDPOINT_DIRECTION) == UX_ENDPOINT_OUT)
        return(UX_ERROR);

    /* Check payload length.  */
    payload_buffer_size = stream -> ux_device_class_video_stream_payload_buffer_size;
    if ((payload_buffer_size - 4) < length)
        return(UX_ERROR);

    /* Check overflow!!  */
    if (stream -> ux_device_class_video_stream_access_pos == stream -> ux_device_class_video_stream_transfer_pos &&
        stream -> ux_device_class_video_stream_access_pos -> ux_device_class_video_payload_length != 0)
        return(UX_BUFFER_OVERFLOW);

    /* Calculate next payload buffer.  */
    next_payload_buffer = (UCHAR *)stream -> ux_device_class_video_stream_access_pos;
    next_payload_buffer += payload_buffer_size;
    if (next_payload_buffer >= stream -> ux_device_class_video_stream_buffer + stream -> ux_device_class_video_stream_buffer_size)
        next_payload_buffer = stream -> ux_device_class_video_stream_buffer;

    /* Copy payload.  */
    _ux_utility_memory_copy(stream -> ux_device_class_video_stream_access_pos -> ux_device_class_video_payload_data, payload, length); /* Use case of memcpy is verified. */
    stream -> ux_device_class_video_stream_access_pos -> ux_device_class_video_payload_length = length;

    /* Move payload position.  */
    stream -> ux_device_class_video_stream_access_pos = (UX_DEVICE_CLASS_VIDEO_PAYLOAD *)next_payload_buffer;

    return(UX_SUCCESS);
}
#endif

/* Hooks define */

static VOID ux_device_class_video_tx_hook(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *p = (UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *)params;
UX_SLAVE_TRANSFER                                 *transfer = (UX_SLAVE_TRANSFER *)p -> parameter;
UCHAR                                              tmp[32] = {'i','n','s','e','r','t','A',0};


    (void)params;
    // printf("%s:%d tTX\n", UX_TEST_FILE, __LINE__);

    /* Acknowledge payload sent.  */
    if (test_tx_ack_count)
    {
        SAVE_BUFFER_LOG(transfer -> ux_slave_transfer_request_data_pointer, transfer -> ux_slave_transfer_request_requested_length);
        transfer -> ux_slave_transfer_request_actual_length = transfer -> ux_slave_transfer_request_requested_length;
        transfer -> ux_slave_transfer_request_completion_code = UX_SUCCESS;
        ux_test_dcd_sim_slave_transfer_done(transfer, UX_SUCCESS);
    }
    if (test_tx_ack_count != 0xFFFFFFFF && test_tx_ack_count > 0)
        test_tx_ack_count --;

    /* Insert payloads when sent.  */
    if (test_tx_ins_count)
    {
        tmp[6] = (test_tx_ins_count % 26) + 'A';
        if (test_tx_ins_way == 0)
            ux_device_class_video_payload_write(device_video_tx_stream, tmp, 32);
        else
        {

        UCHAR *payload;
        ULONG payload_length;


            ux_device_class_video_write_payload_get(device_video_tx_stream, &payload, &payload_length);
            _ux_utility_memory_copy(payload, tmp, 32);
            ux_device_class_video_write_payload_commit(device_video_tx_stream, 32);
        }
    }
    if (test_tx_ins_count != 0xFFFFFFFF && test_tx_ins_count > 0)
        test_tx_ins_count --;
}

static VOID ux_device_class_video_rx_hook(struct UX_TEST_ACTION_STRUCT *action, VOID *params)
{

UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *p = (UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION_PARAMS *)params;
UX_SLAVE_TRANSFER                                 *transfer = (UX_SLAVE_TRANSFER *)p -> parameter;


    (void)action;
    (void)params;
    (void)p;
    (void)transfer;
    // printf("tRX\n");
    device_video_rx_transfer = transfer;
}
static VOID device_video_rx_simulate_one_payload(UCHAR *payload, ULONG payload_length)
{
    UX_TEST_ASSERT(device_video_rx_transfer);
    if (payload_length)
    {
        _ux_utility_memory_copy(device_video_rx_transfer->ux_slave_transfer_request_data_pointer, payload, payload_length);
        device_video_rx_transfer->ux_slave_transfer_request_actual_length = payload_length;
        device_video_rx_transfer->ux_slave_transfer_request_completion_code = UX_SUCCESS;
    }
    ux_test_dcd_sim_slave_transfer_done(device_video_rx_transfer, UX_SUCCESS);
    _ux_utility_thread_sleep(1);
}

static UX_TEST_ACTION ux_device_class_video_transfer_hook[] =
{
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .action_func = ux_device_class_video_tx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x81,
        .do_after = UX_FALSE,
        .no_return = UX_FALSE,
    },
    {
        .usbx_function = UX_TEST_OVERRIDE_UX_DCD_SIM_SLAVE_FUNCTION,
        .function = UX_DCD_TRANSFER_REQUEST,
        .action_func = ux_device_class_video_rx_hook,
        .req_setup = UX_NULL,
        .req_action = UX_TEST_MATCH_EP,
        .req_ep_address = 0x02,
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

static VOID    device_video_activate(VOID *video_instance)
{
    device_video = (UX_DEVICE_CLASS_VIDEO *)video_instance;
    ux_device_class_video_stream_get(device_video, 0, &device_video_tx_stream);
    ux_device_class_video_stream_get(device_video, 1, &device_video_rx_stream);
    // printf("sVID:%p,%p,%p\n", video_instance, device_video_tx_stream, device_video_rx_stream);
}
static VOID    device_video_deactivate(VOID *video_instance)
{
    if ((VOID *)device_video == video_instance)
    {
        device_video = UX_NULL;
        device_video_tx_stream = UX_NULL;
        device_video_rx_stream = UX_NULL;
    }
}
static VOID    device_video_tx_stream_change(UX_DEVICE_CLASS_VIDEO_STREAM *video, ULONG alt)
{
    SAVE_CALLBACK_INVOKE_LOG(device_video_tx_stream_change, video, (ALIGN_TYPE)alt, 0);
}
static VOID    device_video_rx_stream_change(UX_DEVICE_CLASS_VIDEO_STREAM *video, ULONG alt)
{
    SAVE_CALLBACK_INVOKE_LOG(device_video_rx_stream_change, video, (ALIGN_TYPE)alt, 0);

    device_video_rx_transfer = UX_NULL;
}
static UINT    device_video_vc_control_process(UX_DEVICE_CLASS_VIDEO *video, UX_SLAVE_TRANSFER *transfer)
{
    SAVE_CALLBACK_INVOKE_LOG(device_video_vc_control_process, video, transfer, 0);
    return(UX_ERROR);
}
static UINT    device_video_vs_control_process(UX_DEVICE_CLASS_VIDEO_STREAM *video, UX_SLAVE_TRANSFER *transfer)
{
    SAVE_CALLBACK_INVOKE_LOG(device_video_vs_control_process, video, transfer, 0);
    return(UX_ERROR);
}
static VOID    device_video_tx_done(UX_DEVICE_CLASS_VIDEO_STREAM *video, ULONG length)
{
    SAVE_CALLBACK_INVOKE_LOG(device_video_tx_done, video, (ALIGN_TYPE)length, 0);
}
static VOID    device_video_rx_done(UX_DEVICE_CLASS_VIDEO_STREAM *video, ULONG length)
{
    SAVE_CALLBACK_INVOKE_LOG(device_video_rx_done, video, (ALIGN_TYPE)length, 0);
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
void    usbx_ux_device_video_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    /* Inform user.  */
    printf("Running Video Device Basic Functionality Test....................... ");

#if !UX_TEST_MULTI_IFC_ON || !UX_TEST_MULTI_ALT_ON || !UX_TEST_MULTI_CLS_ON || \
    (_CONFIGURE_DESCRIPTORS_LEN  > UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH)
#warning Tests skipped due to compile option!
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

    /* Set the parameters for callback when insertion/extraction of a Video device, with IAD.  */
#if defined(UX_DEVICE_STANDALONE)
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_task_function = ux_device_class_video_write_task_function;
#else
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_thread_entry = ux_device_class_video_write_thread_entry;
#endif
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_change = device_video_tx_stream_change;
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_payload_done = device_video_tx_done;
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_request = device_video_vs_control_process;
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_max_payload_buffer_size = UX_DEMO_ENDPOINT_SIZE;
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_max_payload_buffer_nb   = 8;
#if defined(UX_DEVICE_STANDALONE)
    device_video_stream_parameter[1].ux_device_class_video_stream_parameter_task_function = ux_device_class_video_read_task_function;
#else
    device_video_stream_parameter[1].ux_device_class_video_stream_parameter_thread_entry = ux_device_class_video_read_thread_entry;
#endif
    device_video_stream_parameter[1].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_change = device_video_rx_stream_change;
    device_video_stream_parameter[1].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_payload_done = device_video_rx_done;
    device_video_stream_parameter[1].ux_device_class_video_stream_parameter_callbacks.ux_device_class_video_stream_request = device_video_vs_control_process;
    device_video_stream_parameter[1].ux_device_class_video_stream_parameter_max_payload_buffer_size = UX_DEMO_ENDPOINT_SIZE;
    device_video_stream_parameter[1].ux_device_class_video_stream_parameter_max_payload_buffer_nb   = 8;
    device_video_parameter.ux_device_class_video_parameter_streams = device_video_stream_parameter;
    device_video_parameter.ux_device_class_video_parameter_streams_nb = 2;
    device_video_parameter.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_activate   = device_video_activate;
    device_video_parameter.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_deactivate = device_video_deactivate;
    device_video_parameter.ux_device_class_video_parameter_callbacks.ux_device_class_video_request = device_video_vc_control_process;
    device_video_parameter.ux_device_class_video_parameter_callbacks.ux_device_class_video_arg             = UX_NULL;

#if 0
    printf("Memory requirement UX_DEVICE_CLASS_:\n");
    printf(" per _VIDEO: %d bytes\n", sizeof(UX_DEVICE_CLASS_VIDEO));
    printf(" per _VIDEO_STREAM: %d bytes\n", sizeof(UX_DEVICE_CLASS_VIDEO_STREAM));
    printf(" per _VIDEO_CONTROL: %d bytes\n", sizeof(UX_DEVICE_CLASS_VIDEO_CONTROL));
    printf("Dynamic memory allocation:\n");
    temp = (device_video_tx_stream_parameter.ux_device_class_video_stream_parameter_max_payload_buffer_size+8) *
            device_video_tx_stream_parameter.ux_device_class_video_stream_parameter_max_payload_buffer_nb;
    printf(" per _payload_buffer_size: (%ld + 8) * %ld = %ld\n",
        device_video_tx_stream_parameter.ux_device_class_video_stream_parameter_max_payload_buffer_size,
        device_video_tx_stream_parameter.ux_device_class_video_stream_parameter_max_payload_buffer_nb,
        temp);
    temp += sizeof(UX_DEVICE_CLASS_VIDEO_STREAM);
    printf(" per _stream: _VIDEO_STREAM + _payload_buffer_size = %ld\n", temp);
    temp += sizeof(UX_DEVICE_CLASS_VIDEO);
    temp *= 2;
    printf(" per _video: (_VIDEO + _stream * 1 + _CONTROL * 0) * 2 = %ld\n", temp);
#endif

    /* Initialize the device Audio class. This class owns interfaces starting with 1, 2. */
    status  = ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry,
                                             1, 0,  &device_video_parameter);
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
// ULONG                                               mem_free;
UX_DEVICE                                           *device;
UX_CONFIGURATION                                    *configuration;
UX_INTERFACE                                        *interface;
UX_INTERFACE                                        *interface_inst[3][2];
// UX_ENDPOINT                                         *control_endpoint;
// UX_TRANSFER                                         *transfer_request;
UCHAR                                               test_cmp[32];
ULONG                                               temp;
UCHAR                                               *temp_buf;


    /* Test connect.  */
    status  = test_wait_until_not_null((void**)&dummy_control, 100);
    status |= test_wait_until_not_null((void**)&dummy_tx, 100);
    status |= test_wait_until_not_null((void**)&dummy_rx, 100);
    status |= test_wait_until_not_null((void**)&device_video, 100);
    status |= test_wait_until_not_null((void**)&device_video_rx_stream, 100);
    status |= test_wait_until_not_null((void**)&device_video_tx_stream, 100);
    UX_TEST_CHECK_SUCCESS(status);

    // /* Test disconnect. */
    // ux_test_dcd_sim_slave_disconnect();
    // ux_test_hcd_sim_host_disconnect();

    // /* Reset testing counts. */
    // ux_test_utility_sim_mutex_create_count_reset();
    // ux_test_utility_sim_sem_create_count_reset();
    // ux_test_hcd_sim_host_set_actions(log_on_SetCfg);
    // /* Save free memory usage. */
    // mem_free = _ux_system->ux_system_regular_memory_pool_free;
    // ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    // ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    // tx_thread_sleep(100);
    // /* Log create counts for further tests. */
    // rsc_enum_mutex_usage = rsc_mutex_on_set_cfg;
    // rsc_enum_sem_usage = rsc_sem_on_set_cfg;
    // rsc_enum_mem_usage = mem_free - rsc_mem_free_on_set_cfg;
    // /* Log create counts when instances active for further tests. */
    // rsc_cdc_mutex_usage = ux_test_utility_sim_mutex_create_count() - rsc_enum_mutex_usage;
    // rsc_cdc_sem_usage = ux_test_utility_sim_sem_create_count() - rsc_enum_sem_usage;
    // rsc_cdc_mem_usage = rsc_mem_free_on_set_cfg - _ux_system->ux_system_regular_memory_pool_free;
    // stepinfo("mem free: %ld\n", _ux_system->ux_system_regular_memory_pool_free);

    /* Validate configuration descriptors.  */
    /* ... */

    /* Get testing instances.  */

    status = ux_host_stack_device_get(0, &device);
    UX_TEST_CHECK_SUCCESS(status);

    // control_endpoint = &device->ux_device_control_endpoint;
    // transfer_request = &control_endpoint->ux_endpoint_transfer_request;

    status = ux_host_stack_device_configuration_get(device, 0, &configuration);
    UX_TEST_CHECK_SUCCESS(status);

    interface = configuration -> ux_configuration_first_interface;
    while(interface)
    {
        // printf("Interface: %ld.%ld\n", interface -> ux_interface_descriptor.bInterfaceNumber, interface -> ux_interface_descriptor.bAlternateSetting);
        interface_inst[interface -> ux_interface_descriptor.bInterfaceNumber][interface -> ux_interface_descriptor.bAlternateSetting] = interface;
        interface = interface -> ux_interface_next_interface;
    }

    /* Test interface change.  */
    /* Reset log.  */
    RESET_CALLBACK_INVOKE_LOG();
    status  = ux_host_stack_interface_setting_select(interface_inst[1][1]);
    status |= ux_host_stack_interface_setting_select(interface_inst[1][0]);
    status |= ux_host_stack_interface_setting_select(interface_inst[1][1]);

    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 3);
    UX_TEST_ASSERT(callback_invoke_log[0].func == device_video_tx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[0].param1 == device_video_tx_stream);
    UX_TEST_ASSERT(callback_invoke_log[0].param2 == (VOID*)1);
    UX_TEST_ASSERT(callback_invoke_log[1].func == device_video_tx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[1].param1 == device_video_tx_stream);
    UX_TEST_ASSERT(callback_invoke_log[1].param2 == 0);

    RESET_CALLBACK_INVOKE_LOG();
    status  = ux_host_stack_interface_setting_select(interface_inst[2][1]);
    status |= ux_host_stack_interface_setting_select(interface_inst[2][0]);
    status |= ux_host_stack_interface_setting_select(interface_inst[2][1]);

    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(callback_invoke_count == 3);
    UX_TEST_ASSERT(callback_invoke_log[0].func == device_video_rx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[0].param1 == device_video_rx_stream);
    UX_TEST_ASSERT(callback_invoke_log[0].param2 == (VOID*)1);
    UX_TEST_ASSERT(callback_invoke_log[1].func == device_video_rx_stream_change);
    UX_TEST_ASSERT(callback_invoke_log[1].param1 == device_video_rx_stream);
    UX_TEST_ASSERT(callback_invoke_log[1].param2 == 0);

    /* Test data streaming.  */

    /* Wrong interface!  */
    status  = ux_device_class_video_transmission_start(device_video_rx_stream);
    status |= ux_device_class_video_reception_start(device_video_tx_stream);
    UX_TEST_CHECK_NOT_SUCCESS(status);

    ux_test_link_hooks_from_array(ux_device_class_video_transfer_hook);

    /* ------------------ Write test.  */

    status  = ux_device_class_video_payload_write(device_video_tx_stream, "test0", 16);
    status |= ux_device_class_video_payload_write(device_video_tx_stream, "test1", 16);
    status |= ux_device_class_video_payload_write(device_video_tx_stream, "test2", 16);
    status |= ux_device_class_video_payload_write(device_video_tx_stream, "test3", 16);
    status |= ux_device_class_video_payload_write(device_video_tx_stream, "test4", 16);
    status |= ux_device_class_video_payload_write(device_video_tx_stream, "test5", 16);
    status |= ux_device_class_video_payload_write(device_video_tx_stream, "test6", 16);
    status |= ux_device_class_video_payload_write(device_video_tx_stream, "test7", 16);
    UX_TEST_CHECK_SUCCESS(status);

    error_callback_counter = 0;
    status = ux_device_class_video_payload_write(device_video_tx_stream, "test8", 16);
    UX_TEST_CHECK_CODE(UX_BUFFER_OVERFLOW, status);

    /*
     Keep sending until under-run.
     No buffer appended while running.
     */

    buffer_log_count = 0;
    test_tx_ack_count = 10;
    status  = ux_device_class_video_transmission_start(device_video_tx_stream);
    UX_TEST_CHECK_SUCCESS(status);

    /* Delay to let thread runs.  */
    _ux_utility_delay_ms(100);

    /* Prepare data for checking.  */
    _ux_utility_memory_set(test_cmp, 0, 16);
    _ux_utility_memory_copy(test_cmp, "test", 4);

    /* 8 frame should be available.  */
    for (test_n = 0; test_n < 8; test_n ++)
    {
        // printf("%ld: %ld: %s\n", test_n, buffer_log[test_n].length, buffer_log[test_n].data);
        test_cmp[4] = (UCHAR)(test_n + '0');

        UX_TEST_ASSERT(buffer_log[test_n].length == 16);

        status = _ux_utility_memory_compare(buffer_log[test_n].data, test_cmp, 6);
        UX_TEST_CHECK_SUCCESS(status);
    }
    /* Then under-run, 0 length packets.  */
    for(;test_n < buffer_log_count; test_n ++)
    {
        // printf("%ld: %ld\n", test_n, buffer_log[test_n].length);
        UX_TEST_ASSERT(buffer_log[test_n].length == 0);
    }

    /*
     Keep sending until under-run.
     Specific number of buffer appended while running.
     */

    for (test_tx_ins_way = 0; test_tx_ins_way < 2; test_tx_ins_way ++)
    {

        /* Switch interface to clean up status.  */
        status  = ux_host_stack_interface_setting_select(interface_inst[1][0]);
        status |= ux_host_stack_interface_setting_select(interface_inst[1][1]);

        status  = ux_device_class_video_payload_write(device_video_tx_stream, "test0", 20);
        status |= ux_device_class_video_payload_write(device_video_tx_stream, "test1", 20);
        status |= ux_device_class_video_payload_write(device_video_tx_stream, "test2", 20);
        status |= ux_device_class_video_payload_write(device_video_tx_stream, "test3", 20);
        UX_TEST_CHECK_SUCCESS(status);

        test_tx_ins_count = 10;
        test_tx_ack_count = 20;
        buffer_log_count  = 0;
        status  = ux_device_class_video_transmission_start(device_video_tx_stream);
        UX_TEST_CHECK_SUCCESS(status);

        /* Delay to let thread runs.  */
        _ux_utility_delay_ms(300);

        /* Prepare data for checking.  */
        _ux_utility_memory_set(test_cmp, 0, 32);

        /* 4 frame should be available.  */
        _ux_utility_memory_copy(test_cmp, "test", 4);
        for (test_n = 0; test_n < 4; test_n ++)
        {
            // printf("%ld: %ld: %s\n", test_n, buffer_log[test_n].length, buffer_log[test_n].data);
            test_cmp[4] = (UCHAR)(test_n + '0');

            UX_TEST_ASSERT(buffer_log[test_n].length == 20);

            status = _ux_utility_memory_compare(buffer_log[test_n].data, test_cmp, 6);
            UX_TEST_CHECK_SUCCESS(status);
        }
        /* 10 more frame should be available.  */
        _ux_utility_memory_copy(test_cmp, "insert", 6);
        for (; test_n < 14; test_n ++)
        {
            // printf("%ld: %ld: %s\n", test_n, buffer_log[test_n].length, buffer_log[test_n].data);
            test_cmp[6] = (UCHAR)(((10 - (test_n - 4)) % 26) + 'A');

            UX_TEST_ASSERT(buffer_log[test_n].length == 32);

            status = _ux_utility_memory_compare(buffer_log[test_n].data, test_cmp, 8);
            UX_TEST_CHECK_SUCCESS(status);
        }
        /* Then under-run, 0 length packets.  */
        for(;test_n < buffer_log_count; test_n ++)
        {
            // printf("%ld: %ld\n", test_n, buffer_log[test_n].length);
            UX_TEST_ASSERT(buffer_log[test_n].length == 0);
        }
    }

    UX_TEST_ASSERT(device_video_tx_stream->ux_device_class_video_stream_transfer_pos == device_video_tx_stream->ux_device_class_video_stream_access_pos);


    /* ------------------ Read test.  */

    UX_TEST_ASSERT(device_video_rx_transfer == UX_NULL);

    temp = 0;
    status = ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp);
    UX_TEST_CHECK_CODE(UX_BUFFER_OVERFLOW, status);

    status = ux_device_class_video_reception_start(device_video_rx_stream);
    UX_TEST_CHECK_SUCCESS(status);
    _ux_utility_thread_sleep(1);

    UX_TEST_ASSERT(device_video_rx_transfer != UX_NULL);

    status = ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp);
    UX_TEST_CHECK_CODE(UX_BUFFER_OVERFLOW, status);

    RESET_CALLBACK_INVOKE_LOG();

    device_video_rx_simulate_one_payload("012345", 6);
    status = ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp);
    UX_TEST_CHECK_SUCCESS(status);
    for (test_n = 0; test_n < 6; test_n ++)
    {
        UX_TEST_ASSERT(temp_buf[test_n] == (test_n + '0'));
    }
    ux_device_class_video_read_payload_free(device_video_rx_stream);

    device_video_rx_simulate_one_payload("012345", 6);
    device_video_rx_simulate_one_payload("67", 2);
    device_video_rx_simulate_one_payload("89", 2);

    status = ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(temp == 6);
    for (test_n = 0; test_n < 6; test_n ++)
    {
        UX_TEST_ASSERT(temp_buf[test_n] == (test_n + '0'));
    }
    ux_device_class_video_read_payload_free(device_video_rx_stream);
    status = ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(temp == 2);
    for (; test_n < 8; test_n ++)
    {
        UX_TEST_ASSERT(temp_buf[test_n-6] == (test_n + '0'));
    }
    ux_device_class_video_read_payload_free(device_video_rx_stream);
    status = ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp);
    UX_TEST_CHECK_SUCCESS(status);
    UX_TEST_ASSERT(temp == 2);
    for (; test_n < 10; test_n ++)
    {
        UX_TEST_ASSERT(temp_buf[test_n-8] == (test_n + '0'));
    }
    ux_device_class_video_read_payload_free(device_video_rx_stream);

    UX_TEST_ASSERT(callback_invoke_count == 4);
    UX_TEST_ASSERT(callback_invoke_log[0].func == device_video_rx_done);
    UX_TEST_ASSERT(callback_invoke_log[0].param1 == device_video_rx_stream);
    UX_TEST_ASSERT(callback_invoke_log[1].func == device_video_rx_done);
    UX_TEST_ASSERT(callback_invoke_log[1].param1 == device_video_rx_stream);
    UX_TEST_ASSERT(callback_invoke_log[2].func == device_video_rx_done);
    UX_TEST_ASSERT(callback_invoke_log[2].param1 == device_video_rx_stream);
    UX_TEST_ASSERT(callback_invoke_log[3].func == device_video_rx_done);
    UX_TEST_ASSERT(callback_invoke_log[3].param1 == device_video_rx_stream);

    RESET_CALLBACK_INVOKE_LOG();
    error_callback_counter = 0;
    for (test_n = 0; test_n < 10; test_n ++)
    {
        test_cmp[0] = (UCHAR)(test_n + '0');
        device_video_rx_simulate_one_payload(test_cmp, 1);
    }

    UX_TEST_ASSERT(callback_invoke_count == 10);
    UX_TEST_ASSERT(error_callback_counter == 3);
    for (test_n = 0; test_n < 7; test_n ++)
    {
        status = ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp);
        UX_TEST_CHECK_SUCCESS(status);
        UX_TEST_ASSERT(temp == 1);
        UX_TEST_ASSERT(temp_buf[0] == (test_n + '0'));
        ux_device_class_video_read_payload_free(device_video_rx_stream);
    }
    UX_TEST_CHECK_SUCCESS(ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp));
    UX_TEST_ASSERT(temp == 1);
    UX_TEST_ASSERT(temp_buf[0] == '9');
    ux_device_class_video_read_payload_free(device_video_rx_stream);
    UX_TEST_CHECK_CODE(UX_BUFFER_OVERFLOW, ux_device_class_video_read_payload_get(device_video_rx_stream, &temp_buf, &temp));


    /* Wait pending threads.  */
    _ux_utility_thread_sleep(1);

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    ux_device_stack_class_unregister(_ux_system_device_class_video_name, ux_device_class_video_entry);

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

#if defined(UX_DEVICE_STANDALONE)

        /* Standalone background task.  */
        ux_system_tasks_run();
#else

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
#endif
    }
}
