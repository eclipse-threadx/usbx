/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"
#include "ux_host_class_video.h"
#include "ux_host_class_dummy.h"

#include "ux_device_class_video.h"

#include "ux_test.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           error_counter = 0;

/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static TX_THREAD           ux_test_thread_simulation_0;
static void                ux_test_thread_simulation_0_entry(ULONG);


/* Prototype for test control return.  */

void  test_control_return(UINT status);

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

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_uxe_device_video_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_device_video APIs Test.................................. ");
#if !defined(UX_DEVICE_CLASS_VIDEO_ENABLE_ERROR_CHECKING)
#warning Tests skipped due to compile option!
    printf("SKIP SUCCESS!\n");
    test_control_return(0);
    return;
#endif  /* UX_DEVICE_CLASS_VIDEO_ENABLE_ERROR_CHECKING */

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
    // device_video_parameter.ux_device_class_video_parameter_streams = device_video_stream_parameter;
    device_video_parameter.ux_device_class_video_parameter_streams_nb = 2;
    device_video_parameter.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_activate   = device_video_activate;
    device_video_parameter.ux_device_class_video_parameter_callbacks.ux_slave_class_video_instance_deactivate = device_video_deactivate;
    device_video_parameter.ux_device_class_video_parameter_callbacks.ux_device_class_video_request = device_video_vc_control_process;
    device_video_parameter.ux_device_class_video_parameter_callbacks.ux_device_class_video_arg             = UX_NULL;

    status  = ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry,
                                             1, 0,  UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    device_video_parameter.ux_device_class_video_parameter_streams = UX_NULL;
    status  = ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry,
                                             1, 0,  &device_video_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    device_video_parameter.ux_device_class_video_parameter_streams = device_video_stream_parameter;
    device_video_parameter.ux_device_class_video_parameter_streams_nb = 0;
    status  = ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry,
                                             1, 0,  &device_video_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    device_video_parameter.ux_device_class_video_parameter_streams = device_video_stream_parameter;
    device_video_parameter.ux_device_class_video_parameter_streams_nb = 2;
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_max_payload_buffer_size = 0;
    status  = ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry,
                                             1, 0,  &device_video_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

    device_video_parameter.ux_device_class_video_parameter_streams = device_video_stream_parameter;
    device_video_parameter.ux_device_class_video_parameter_streams_nb = 2;
    device_video_stream_parameter[0].ux_device_class_video_stream_parameter_max_payload_buffer_size = UX_DEMO_ENDPOINT_SIZE;
    device_video_stream_parameter[1].ux_device_class_video_stream_parameter_max_payload_buffer_nb   = 0;
    status  = ux_device_stack_class_register(_ux_system_device_class_video_name, ux_device_class_video_entry,
                                             1, 0,  &device_video_parameter);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER ,status);

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
UX_DEVICE_CLASS_VIDEO_STREAM            dummy_video_stream;
ULONG                                   dummy_length;
UCHAR payload_data[64];
ULONG payload_length;
UCHAR **dummy_payload = &payload_data;

    dummy_length = ux_device_class_video_max_payload_length(UX_NULL);
    if (dummy_length != 0)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    status = ux_device_class_video_reception_start(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_read_payload_get(UX_NULL, dummy_payload, &payload_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_read_payload_get(&dummy_video_stream, UX_NULL, &payload_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_read_payload_get(&dummy_video_stream, dummy_payload, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_read_payload_free(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_transmission_start(UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_write_payload_get(UX_NULL, dummy_payload, &payload_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_write_payload_get(&dummy_video_stream, UX_NULL, &payload_length);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_write_payload_get(&dummy_video_stream, dummy_payload, UX_NULL);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_write_payload_commit(UX_NULL, 8);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_write_payload_commit(&dummy_video_stream, 0);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);

    status = ux_device_class_video_ioctl(UX_NULL, 0, 0);
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
