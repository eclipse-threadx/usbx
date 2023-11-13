/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_class_dummy.h"
#include "ux_device_stack.h"
#include "ux_host_class_video.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE (UX_SLAVE_REQUEST_DATA_MAX_LENGTH + 1)
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (64*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

/* Define local/extern function prototypes.  */
static VOID                                test_thread_entry(ULONG);
static TX_THREAD                           tx_test_thread_host_simulation;
static TX_THREAD                           tx_test_thread_slave_simulation;
static VOID                                tx_test_thread_host_simulation_entry(ULONG);
static VOID                                tx_test_thread_slave_simulation_entry(ULONG);

static VOID                                ux_test_hcd_entry_should_not_be_called(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_disconnect(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

static VOID                                ux_test_system_host_enum_hub_function(VOID);

/* Define global data structures.  */
UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

UX_HOST_CLASS_VIDEO                 *host_video = UX_NULL;

ULONG                               enum_counter;

ULONG                               error_counter;

ULONG                               set_cfg_counter;

ULONG                               rsc_mem_free_on_set_cfg;
ULONG                               rsc_sem_on_set_cfg;
ULONG                               rsc_sem_get_on_set_cfg;
ULONG                               rsc_mutex_on_set_cfg;

ULONG                               rsc_enum_sem_usage;
ULONG                               rsc_enum_sem_get_count;
ULONG                               rsc_enum_mutex_usage;
ULONG                               rsc_enum_mem_usage;

static UX_DEVICE_CLASS_DUMMY                *device_dummy[2] = {UX_NULL, UX_NULL};
static UX_DEVICE_CLASS_DUMMY_PARAMETER      device_dummy_parameter;

/* Define device framework.  */

#define             STRING_FRAMEWORK_LENGTH                 47
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            2

static unsigned char device_framework_full_speed[] = {

    /* Device descriptor     18 bytes
       0xEF bDeviceClass:    Composite class code
       0x02 bDeviceSubclass: class sub code
       0x00 bDeviceProtocol: Device protocol

       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x10, 0x01,
    0xEF, 0x02, 0x01,
    0x08,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01,

    /* Configuration 1 descriptor 9 bytes @ 0x12 */
    0x09, 0x02,
    0xE6, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor @ 0x12+0x09=27 */
    0x08, 0x0b, 0x00, 0x02, 0x0E, 0x03, 0x00, 0x00,

    /* VideoControl Interface Descriptor Requirement @ 0x12+0x09+8=35  */
    0x09, 0x04,
    0x00, 0x00,
    0x01,
    0x0E, 0x01, 0x01,
    0x00,

        /* VC_HEADER Descriptor @ 35+9=44 */
        0x0D, 0x24, 0x01,
        0x50, 0x01,
        0x4D, 0x00, /* wTotalLength:  */
        0x00, 0x00, 0x00, 0x00,
        0x01, /* bInCollection  */
        0x01, /* baInterfaceNr(1)  */

        /* Input Terminal Descriptor (Camera) @ 44+0x0D=57  */
        0x12, 0x24, 0x02,
        0x02, /* bTerminalID  */
        0x01, 0x02, /* wTerminalType ITT_CAMERA  */
        0x00,
        0x00,
        0x00, 0x00, /* wObjectiveFocalLengthMin  */
        0x00, 0x00, /* wObjectiveFocalLengthMax  */
        0x00, 0x00, /* wOcularFocalLength        */
        0x03, /* bControlSize  */
        0x00, 0x02, 0x00, /* bmControls  */

        /* Input Terminal Descriptor (Media Transport) @ 57+0x12=75 */
        0x10, 0x24, 0x02,
        0x03, /* bTerminalID  */
        0x02, 0x02, /* wTerminalType ITT_MEDIA_TRANSPORT_INPUT  */
        0x00,
        0x00,
        0x01, /* bControlSize  */
        0x0D, /* bmControls  */
        0x05, /* bTransportModeSize  */
        0xAF, 0xFF, 0xFF, 0x7F, 0x00, /* bmTransportModes  */

        /* Selector Unit Descriptor @ 75+0x10=91 */
        0x08, 0x24, 0x04,
        0x01, /* bUnitID  */
        0x02, /* bNrInPins  */
        0x02, /* baSourceID(1)  */
        0x03, /* baSourceID(2)  */
        0x00,

        /* Processing Unit Descriptor @ 91+8=99 */
        13, 0x24, 0x05,
        0x05, /* bUnitID  */
        0x01, /* bSourceID  */
        0x00, 0x00, /* wMaxMultiplier  */
        0x03, /* bControlSize  */
        0x00, 0x00, 0x00, /* bmControls  */
        0x00,
        0x00, /* bmVideoStandards  */

        /* Output Terminal Descriptor @ 99+13=112 */
        0x09, 0x24, 0x03,
        0x04, /* bTerminalID  */
        0x01, 0x01, /* wTerminalType TT_STREAMING */
        0x00,
        0x05, /* bSourceID  */
        0x00,

        /* Interrupt Endpoint 0x83 descriptor @ 112+9=121 */
        0x07, 0x05, 0x83,
        0x03,
        0x08, 0x00,
        0x0A,

        /* CS_ENDPOINT @ 121+7=128 */
        0x05, 0x25, 0x03,
        0x20, 0x00, /* wMaxTransferSize  */

    /* VideoStreaming Interface 1.0 @ 128+5=133 */
    0x09, 0x04,
    0x01, 0x00,
    0x01,
    0x0E, 0x02, 0x00,
    0x00,

        /* VS_INPUT_HEADER @ 133+9=142 */
        0x0E, 0x24, 0x01,
        0x01, /* bNumFormats  */
        0x4C, 0x00, /* wTotalLength  */
        0x85, /* bEndpointAddress  */
        0x00,
        0x04, /* bTerminalLink  */
        0x03, /* bStillCaptureMethod  */
        0x00, /* bTriggerSupport  */
        0x00, /* bTriggerUsage  */
        0x01, /* bControlSize  */
        0x00, /* bmaControls   */

        /* VS_FORMAT_MJPEG @ 142+0xE=156 */
        0x0B, 0x24, 0x06,
        0x01, /* bFormatIndex  */
        0x01, /* bNumFrameDescriptors  */
        0x01, /* bmFlags  */
        0x01, /* bDefaultFrameIndex  */
        0x00, 0x00,
        0x02, /* bmInterlaceFlags  */
        0x00, /* bCopyProtect  */

        /* VS_FRAME_MJPEG @ 156+0x0b=167 */
        0x1E, 0x24, 0x07,
        0x01, /* bFrameIndex  */
        0x02, /* bmCapabilities  */
        0xA0, 0x00, /* wWidth  160 */
        0x78, 0x00, /* wHeight 120 */
        0x00, 0x65, 0x04, 0x00, /* dwMinBitRate  */
        0x00, 0xA0, 0x0F, 0x00, /* dwMaxBitRate  */
        0x00, 0x08, 0x00, 0x00, /* dwMaxVideoFrameBufSize  */
        0x2A, 0x2C, 0x0A, 0x00, /* dwDefaultFrameInterval  */
        0x01, /* bFrameIntervalType  */
        0x2A, 0x2C, 0x0A, 0x00, /* dwFrameInterval(1) 666666 */

        /* VS_STILL_FRAME @ 167+0x1e=197 */
        0x0F, 0x24, 0x03,
        0x86, /* bEndpointAddress  */
        0x02, /* bNumImageSizePatterns  */
        0x20, 0x03, /* wWidth 800  */
        0x58, 0x02, /* wHeight 600 */
        0x20, 0x03, /* wWidth */
        0x20, 0x03, /* wHeight */
        0x01, /* bNumCompressionPtr  */
        0x64, /* bCompression 1:100  */

        /* VS_COLORFORMAT @ 197+0xf=212 */
        0x06, 0x24, 0x0D,
        0x00, 0x00, 0x00,

        /* Bulk endpoint 0x86 */
        0x07, 0x05,
        0x86,
        0x02,
        0x40, 0x00,
        0x00,

    /* VideoStreaming Interface 1.1  */
    0x09, 0x04,
    0x01, 0x01,
    0x02,
    0x0E, 0x02, 0x00,
    0x00,

        /* ISO Endpoint 0x85 */
        0x07, 0x05,
        0x85,
        0x05,
        0x80, 0x00, /* 128  */
        0x01,

        /* Bulk Endpoint 0x86  */
        0x07, 0x05,
        0x86,
        0x02,
        0x40, 0x00,
        0x00,
};

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      sizeof(device_framework_full_speed)

static unsigned char device_framework_high_speed[] = {

    /* Device descriptor     18 bytes
       0xEF bDeviceClass:    Composite class code
       0x02 bDeviceSubclass: class sub code
       0x00 bDeviceProtocol: Device protocol

       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x10, 0x01,
    0xEF, 0x02, 0x01,
    0x08,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01,

    /* Configuration 1 descriptor 9 bytes @ 0x12 */
    0x09, 0x02,
    0xE6, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor @ 0x12+0x09=27 */
    0x08, 0x0b, 0x00, 0x02, 0x0E, 0x03, 0x00, 0x00,

    /* VideoControl Interface Descriptor Requirement @ 0x12+0x09+8=35  */
    0x09, 0x04,
    0x00, 0x00,
    0x01,
    0x0E, 0x01, 0x01,
    0x00,

        /* VC_HEADER Descriptor @ 35+9=44 */
        0x0D, 0x24, 0x01,
        0x50, 0x01,
        0x4D, 0x00, /* wTotalLength:  */
        0x00, 0x00, 0x00, 0x00,
        0x01, /* bInCollection  */
        0x01, /* baInterfaceNr(1)  */

        /* Input Terminal Descriptor (Camera) @ 44+0x0D=57  */
        0x12, 0x24, 0x02,
        0x02, /* bTerminalID  */
        0x01, 0x02, /* wTerminalType ITT_CAMERA  */
        0x00,
        0x00,
        0x00, 0x00, /* wObjectiveFocalLengthMin  */
        0x00, 0x00, /* wObjectiveFocalLengthMax  */
        0x00, 0x00, /* wOcularFocalLength        */
        0x03, /* bControlSize  */
        0x00, 0x02, 0x00, /* bmControls  */

        /* Input Terminal Descriptor (Media Transport) @ 57+0x12=75 */
        0x10, 0x24, 0x02,
        0x03, /* bTerminalID  */
        0x02, 0x02, /* wTerminalType ITT_MEDIA_TRANSPORT_INPUT  */
        0x00,
        0x00,
        0x01, /* bControlSize  */
        0x0D, /* bmControls  */
        0x05, /* bTransportModeSize  */
        0xAF, 0xFF, 0xFF, 0x7F, 0x00, /* bmTransportModes  */

        /* Selector Unit Descriptor @ 75+0x10=91 */
        0x08, 0x24, 0x04,
        0x01, /* bUnitID  */
        0x02, /* bNrInPins  */
        0x02, /* baSourceID(1)  */
        0x03, /* baSourceID(2)  */
        0x00,

        /* Processing Unit Descriptor @ 91+8=99 */
        13, 0x24, 0x05,
        0x05, /* bUnitID  */
        0x01, /* bSourceID  */
        0x00, 0x00, /* wMaxMultiplier  */
        0x03, /* bControlSize  */
        0x00, 0x00, 0x00, /* bmControls  */
        0x00,
        0x00, /* bmVideoStandards  */

        /* Output Terminal Descriptor @ 99+13=112 */
        0x09, 0x24, 0x03,
        0x04, /* bTerminalID  */
        0x01, 0x01, /* wTerminalType TT_STREAMING */
        0x00,
        0x05, /* bSourceID  */
        0x00,

        /* Interrupt Endpoint 0x83 descriptor @ 112+9=121 */
        0x07, 0x05, 0x83,
        0x03,
        0x08, 0x08,
        0x01,

        /* CS_ENDPOINT @ 121+7=128 */
        0x05, 0x25, 0x03,
        0x20, 0x00, /* wMaxTransferSize  */

    /* VideoStreaming Interface 1.0 @ 128+5=133 */
    0x09, 0x04,
    0x01, 0x00,
    0x01,
    0x0E, 0x02, 0x00,
    0x00,

        /* VS_INPUT_HEADER @ 133+9=142 */
        0x0E, 0x24, 0x01,
        0x01, /* bNumFormats  */
        0x4C, 0x00, /* wTotalLength  */
        0x85, /* bEndpointAddress  */
        0x00,
        0x04, /* bTerminalLink  */
        0x03, /* bStillCaptureMethod  */
        0x00, /* bTriggerSupport  */
        0x00, /* bTriggerUsage  */
        0x01, /* bControlSize  */
        0x00, /* bmaControls   */

        /* VS_FORMAT_MJPEG @ 142+0xE=156 */
        0x0B, 0x24, 0x06,
        0x01, /* bFormatIndex  */
        0x01, /* bNumFrameDescriptors  */
        0x01, /* bmFlags  */
        0x01, /* bDefaultFrameIndex  */
        0x00, 0x00,
        0x02, /* bmInterlaceFlags  */
        0x00, /* bCopyProtect  */

        /* VS_FRAME_MJPEG @ 156+0x0b=167 */
        0x1E, 0x24, 0x07,
        0x01, /* bFrameIndex  */
        0x02, /* bmCapabilities  */
        0xA0, 0x00, /* wWidth  160 */
        0x78, 0x00, /* wHeight 120 */
        0x00, 0x65, 0x04, 0x00, /* dwMinBitRate  */
        0x00, 0xA0, 0x0F, 0x00, /* dwMaxBitRate  */
        0x00, 0x08, 0x00, 0x00, /* dwMaxVideoFrameBufSize  */
        0x2A, 0x2C, 0x0A, 0x00, /* dwDefaultFrameInterval  */
        0x01, /* bFrameIntervalType  */
        0x2A, 0x2C, 0x0A, 0x00, /* dwFrameInterval(1) 666666  */

        /* VS_STILL_FRAME @ 167+0x1e=197 */
        0x0F, 0x24, 0x03,
        0x86, /* bEndpointAddress  */
        0x02, /* bNumImageSizePatterns  */
        0x20, 0x03, /* wWidth 800  */
        0x58, 0x02, /* wHeight 600 */
        0x20, 0x03, /* wWidth */
        0x20, 0x03, /* wHeight */
        0x01, /* bNumCompressionPtr  */
        0x64, /* bCompression 1:100  */

        /* VS_COLORFORMAT @ 197+0xf=212 */
        0x06, 0x24, 0x0D,
        0x00, 0x00, 0x00,

        /* Bulk endpoint 0x86 */
        0x07, 0x05,
        0x86,
        0x02,
        0x40, 0x00,
        0x00,

    /* VideoStreaming Interface 1.1  */
    0x09, 0x04,
    0x01, 0x01,
    0x02,
    0x0E, 0x02, 0x00,
    0x00,

        /* ISO Endpoint 0x85 */
        0x07, 0x05,
        0x85,
        0x05,
        0x80, 0x08, /* 2 x 128 = 256 */
        0x01,

        /* Bulk Endpoint 0x86  */
        0x07, 0x05,
        0x86,
        0x02,
        0x40, 0x00,
        0x00,
};
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      sizeof(device_framework_high_speed)

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


    /* Multiple languages are supported on the device, to add
       a language besides english, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static unsigned char language_id_framework[] = {

    /* English. */
        0x09, 0x04
    };

static UX_TEST_SETUP _SetAddress = UX_TEST_SETUP_SetAddress;
static UX_TEST_SETUP _GetDeviceDescriptor = UX_TEST_SETUP_GetDevDescr;
static UX_TEST_SETUP _GetConfigDescriptor = UX_TEST_SETUP_GetCfgDescr;
static UX_TEST_SETUP _SetConfigure = UX_TEST_SETUP_SetConfigure;

/* Test interactions */

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

static UX_TEST_HCD_SIM_ACTION normal_enum_replace[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_GetDeviceDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, device_framework_full_speed + 0, 8, 0,
        UX_SUCCESS, UX_NULL},
{   UX_HCD_TRANSFER_REQUEST, &_GetDeviceDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, device_framework_full_speed + 0, 18, 0,
        UX_SUCCESS, UX_NULL},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, device_framework_full_speed + 18, UX_CONFIGURATION_DESCRIPTOR_LENGTH, 0,
        UX_SUCCESS, UX_NULL},
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, 0,
        UX_TEST_SIM_REQ_ANSWER | UX_TEST_SETUP_MATCH_REQ_V, 0, device_framework_full_speed + 18, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 18, 0,
        UX_SUCCESS, UX_NULL},
{   0   }
}
;

/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}

static UINT test_slave_change_function(ULONG change)
{
    return 0;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_VIDEO *video_inst = (UX_HOST_CLASS_VIDEO *) inst;

    switch(event)
    {

        case UX_DEVICE_INSERTION:
            host_video = video_inst;
            break;

        case UX_DEVICE_REMOVAL:
            host_video = UX_NULL;
            break;

        default:
            break;
    }
    return 0;
}

static VOID    test_dummy_instance_activate(VOID *dummy_instance)
{
    if (device_dummy[0] == UX_NULL)
    {
        device_dummy[0] = (UX_DEVICE_CLASS_DUMMY *)dummy_instance;
        return;
    }
    if (device_dummy[1] == UX_NULL)
    {
        device_dummy[1] = (UX_DEVICE_CLASS_DUMMY *)dummy_instance;
        return;
    }
}
static VOID    test_dummy_instance_deactivate(VOID *dummy_instance)
{
    if (device_dummy[0] == dummy_instance)
        device_dummy[0] = UX_NULL;
    if (device_dummy[1] == dummy_instance)
        device_dummy[1] = UX_NULL;
}
static UX_SLAVE_TRANSFER _last_control_transfer;
static UCHAR             _last_control_data[64];
static VOID test_dummy_instance_control_request(UX_DEVICE_CLASS_DUMMY *dummy_instance,
                                        UX_SLAVE_TRANSFER *transfer)
{
USHORT req_length = _ux_utility_short_get(transfer->ux_slave_transfer_request_setup + 6);
#if 0
    printf("D_Video_Req %x: %x %x, %x %x %x; %d/%d\n",
        transfer->ux_slave_transfer_request_phase,
        transfer->ux_slave_transfer_request_setup[0],
        transfer->ux_slave_transfer_request_setup[1],
        _ux_utility_short_get(transfer->ux_slave_transfer_request_setup + 2),
        _ux_utility_short_get(transfer->ux_slave_transfer_request_setup + 4),
        _ux_utility_short_get(transfer->ux_slave_transfer_request_setup + 6),
        transfer->ux_slave_transfer_request_actual_length,
        transfer->ux_slave_transfer_request_requested_length);
#endif
    _ux_utility_memory_copy(&_last_control_transfer, transfer, sizeof(UX_SLAVE_TRANSFER));
    if (transfer->ux_slave_transfer_request_setup[0] & 0x80) /* GET */
    {
        _ux_utility_memory_copy(transfer->ux_slave_transfer_request_data_pointer,
                                _last_control_data,
                                req_length);
        _ux_device_stack_transfer_request(transfer, req_length, req_length);
    }
    else /* SET */
    {
        _ux_utility_memory_copy(_last_control_data,
                               transfer->ux_slave_transfer_request_data_pointer,
                               req_length);
    }
    // _ux_device_stack_endpoint_stall(transfer->ux_slave_transfer_request_endpoint);
}
static VOID test_dummy_instance_change(UX_DEVICE_CLASS_DUMMY *dummy_instance)
{
}

static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
}

static VOID ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params)
{

    set_cfg_counter ++;

    rsc_mem_free_on_set_cfg = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    rsc_sem_on_set_cfg = ux_test_utility_sim_sem_create_count();
    rsc_enum_sem_get_count = ux_test_utility_sim_sem_get_count();
    rsc_mutex_on_set_cfg = ux_test_utility_sim_mutex_create_count();
}

static VOID ux_test_system_host_enum_hub_function(VOID)
{
    enum_counter ++;
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_host_class_video_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    printf("Running Host Class Video Basic Functionality Test................... ");
#if !(UX_TEST_MULTI_IFC_ON && UX_TEST_MULTI_ALT_ON)
    printf("SKIP!\n");
    test_control_return(0);
    return;
#endif

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

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #1\n");
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(test_ux_error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_host_change_function);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #2\n");
        test_control_return(1);
    }

    /* Register CDC-ACM class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_video_name, ux_host_class_video_entry);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #3\n");
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,
                                       test_slave_change_function);
    if(status!=UX_SUCCESS)
    {

        printf(" ERROR #5\n");
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a dummy device.  */
    _ux_utility_memory_set(&device_dummy_parameter, 0, sizeof(device_dummy_parameter));
    device_dummy_parameter.ux_device_class_dummy_parameter_callbacks.
        ux_device_class_dummy_instance_activate          = test_dummy_instance_activate;
    device_dummy_parameter.ux_device_class_dummy_parameter_callbacks.
        ux_device_class_dummy_instance_deactivate        = test_dummy_instance_deactivate;
    device_dummy_parameter.ux_device_class_dummy_parameter_callbacks.
        ux_device_class_dummy_change                     = test_dummy_instance_change;
    device_dummy_parameter.ux_device_class_dummy_parameter_callbacks.
        ux_device_class_dummy_control_request            = test_dummy_instance_control_request;
    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status  = ux_device_stack_class_register(_ux_system_slave_class_dpump_name,
                                             _ux_device_class_dummy_entry,
                                             1, 0, &device_dummy_parameter);
    status |= ux_device_stack_class_register(_ux_system_slave_class_dpump_name,
                                             _ux_device_class_dummy_entry,
                                             1, 1, &device_dummy_parameter);
#if UX_MAX_SLAVE_CLASS_DRIVER > 1
    if(status != UX_SUCCESS)
    {

        printf(" ERROR #7\n");
        test_control_return(1);
    }
#endif
    /* Initialize the simulated device controller.  */
    status =  _ux_test_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #8\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #4\n");
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_test_thread_host_simulation, "tx test host simulation", tx_test_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #9\n");
        test_control_return(1);
    }

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&tx_test_thread_slave_simulation, "tx test slave simulation", tx_test_thread_slave_simulation_entry, 0,
            stack_pointer + UX_DEMO_STACK_SIZE, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf(" ERROR #10\n");
        test_control_return(1);
    }
}

#define TX_TEST_VIDEO_PARSER_SIZE            32
typedef struct TX_TEST_VIDEO_PARSER_STRUCT
{
    UCHAR                   *descriptor_base;
    ULONG                   parsed_interface[TX_TEST_VIDEO_PARSER_SIZE];
    ULONG                   parsed_entity[TX_TEST_VIDEO_PARSER_SIZE];
    ULONG                   parsed_count;
} TX_TEST_VIDEO_PARSER;

static const TX_TEST_VIDEO_PARSER parser_check =
{
    UX_NULL,
    {17,  17,  17,  17,  17,  17,  17,  17,
     115, 115, 115, 115, 115},
    {26,  39,  57,  73,  81,  94, 103, 110,
     124, 138, 149, 179, 182},
    13
};

static UINT tx_test_video_descriptors_parser(VOID  *arg,
                              UCHAR *packed_interface_descriptor,
                              UCHAR *packed_entity_descriptor)
{
TX_TEST_VIDEO_PARSER        *parser;

    parser = (TX_TEST_VIDEO_PARSER *)arg;
    parser->parsed_interface[parser->parsed_count] = packed_interface_descriptor - parser->descriptor_base;
    parser->parsed_entity   [parser->parsed_count] = packed_entity_descriptor    - parser->descriptor_base;
    parser->parsed_count ++;
}


void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               test_n;
ULONG                                               mem_free;
ULONG                                               loop;
TX_TEST_VIDEO_PARSER                                parser;
ULONG                                               parameter_u32[64/4];
USHORT                                              *parameter_u16 = (USHORT*)parameter_u32;
UCHAR                                               *parameter_u8 = (UCHAR*)parameter_u32;
UX_ENDPOINT                                         *iso_ep;
UX_ENDPOINT                                         *int_ep;


    stepinfo("\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

    /* Find the video class and wait for the link to be up.  */
    for (loop = 0; loop < 100; loop ++)
        if (host_video != UX_NULL)
            break;
    if (host_video == UX_NULL)
    {
        printf("ERROR #%d: enum fail", __LINE__);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_feature_unit_id != 5)
    {
        printf("ERROR #%d: wrong feature bUnitID 0x%lx\n", __LINE__, host_video->ux_host_class_video_feature_unit_id);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_terminal_id != 2)
    {
        printf("ERROR #%d: wrong input bTerminalID 0x%lx\n", __LINE__, host_video->ux_host_class_video_terminal_id);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_number_formats != 1)
    {
        printf("ERROR #%d: wrong VS_HEADER bNumFormats %ld\n", __LINE__, host_video->ux_host_class_video_number_formats);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_length_formats != 0x004C)
    {
        printf("ERROR #%d: wrong VS_HEADER wTotalLength 0x%lx\n", __LINE__, host_video->ux_host_class_video_length_formats);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_format_address !=
        (host_video->ux_host_class_video_configuration_descriptor + 124))
    {
        printf("ERROR #%d: wrong VS_HEADER addr %p <> %p\n", __LINE__,
            host_video->ux_host_class_video_format_address,
            (host_video->ux_host_class_video_configuration_descriptor + 124));
        test_control_return(1);
    }
    int_ep = host_video->ux_host_class_video_device->
                            ux_device_current_configuration->
                            ux_configuration_first_interface->
                            ux_interface_first_endpoint;
    if (int_ep->ux_endpoint_transfer_request.ux_transfer_request_packet_length != 8)
    {
        printf("ERROR #%d: wrong INT max packet length %ld\n", __LINE__, iso_ep->ux_endpoint_transfer_request.ux_transfer_request_packet_length);
        test_control_return(1);
    }
    iso_ep = host_video->ux_host_class_video_device->
                            ux_device_current_configuration->
                            ux_configuration_first_interface->
                            ux_interface_next_interface->
                            ux_interface_next_interface->ux_interface_first_endpoint;
    if (iso_ep->ux_endpoint_transfer_request.ux_transfer_request_packet_length != 128)
    {
        printf("ERROR #%d: wrong ISO max packet length %ld\n", __LINE__, iso_ep->ux_endpoint_transfer_request.ux_transfer_request_packet_length);
        test_control_return(1);
    }

    /* Re-connect with high speed.  */
    ux_test_hcd_sim_host_disconnect();
    ux_test_dcd_sim_slave_disconnect();

    ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);
    /* Find the video class and wait for the link to be up.  */
    for (loop = 0; loop < 100; loop ++)
        if (host_video != UX_NULL)
            break;
    if (host_video == UX_NULL)
    {
        printf("ERROR #%d: enum fail", __LINE__);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_feature_unit_id != 5)
    {
        printf("ERROR #%d: wrong feature bUnitID 0x%lx\n", __LINE__, host_video->ux_host_class_video_feature_unit_id);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_terminal_id != 2)
    {
        printf("ERROR #%d: wrong input bTerminalID 0x%lx\n", __LINE__, host_video->ux_host_class_video_terminal_id);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_number_formats != 1)
    {
        printf("ERROR #%d: wrong VS_HEADER bNumFormats %ld\n", __LINE__, host_video->ux_host_class_video_number_formats);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_length_formats != 0x004C)
    {
        printf("ERROR #%d: wrong VS_HEADER wTotalLength 0x%lx\n", __LINE__, host_video->ux_host_class_video_length_formats);
        test_control_return(1);
    }
    if (host_video->ux_host_class_video_format_address !=
        (host_video->ux_host_class_video_configuration_descriptor + 124))
    {
        printf("ERROR #%d: wrong VS_HEADER addr %p <> %p\n", __LINE__,
            host_video->ux_host_class_video_format_address,
            (host_video->ux_host_class_video_configuration_descriptor + 124));
        test_control_return(1);
    }
    int_ep = host_video->ux_host_class_video_device->
                            ux_device_current_configuration->
                            ux_configuration_first_interface->
                            ux_interface_first_endpoint;
    if (int_ep->ux_endpoint_transfer_request.ux_transfer_request_packet_length != 8*2)
    {
        printf("ERROR #%d: wrong INT max packet length %ld\n", __LINE__, iso_ep->ux_endpoint_transfer_request.ux_transfer_request_packet_length);
        test_control_return(1);
    }
    iso_ep = host_video->ux_host_class_video_device->
                            ux_device_current_configuration->
                            ux_configuration_first_interface->
                            ux_interface_next_interface->
                            ux_interface_next_interface->ux_interface_first_endpoint;
    if (iso_ep->ux_endpoint_transfer_request.ux_transfer_request_packet_length != 128*2)
    {
        printf("ERROR #%d: wrong ISO max packet length %ld\n", __LINE__, iso_ep->ux_endpoint_transfer_request.ux_transfer_request_packet_length);
        test_control_return(1);
    }

    /* Test entities parse.  */
    ux_utility_memory_set(&parser, 0, sizeof(parser));
    parser.descriptor_base = host_video->ux_host_class_video_configuration_descriptor;
    status = _ux_host_class_video_entities_parse(host_video, tx_test_video_descriptors_parser, &parser);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: parse error 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    for(loop = 0; loop < parser.parsed_count; loop++)
    {
        if (parser.parsed_interface[loop] != parser_check.parsed_interface[loop])
        {
            printf("ERROR #%d: wrong interface: %ld <> %ld\n", __LINE__,
                parser.parsed_interface[loop], parser_check.parsed_interface[loop]);
            test_control_return(1);
        }
        if (parser.parsed_entity[loop] != parser_check.parsed_entity[loop])
        {
            printf("ERROR #%d: wrong entity descriptor: %ld <> %ld\n", __LINE__,
                parser.parsed_entity[loop], parser_check.parsed_entity[loop]);
            test_control_return(1);
        }
    }

    /* Test control request.  */

    /* UX_HOST_CLASS_VIDEO_SET_CUR :: UX_HOST_CLASS_VIDEO_TCS_PU_BRIGHTNESS_CONTROL */
    _ux_utility_memory_set(_last_control_data, 0, 2);
    parameter_u8[0] = '0';
    parameter_u8[1] = '1';
    status = _ux_host_class_video_control_request(host_video,
        UX_HOST_CLASS_VIDEO_SET_CUR, 0,
        host_video -> ux_host_class_video_feature_unit_id,
        UX_HOST_CLASS_VIDEO_TCS_PU_BRIGHTNESS_CONTROL,
        parameter_u8, 2);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: request error 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (_last_control_transfer.ux_slave_transfer_request_setup[0] != 0x21 ||
        _last_control_transfer.ux_slave_transfer_request_setup[1] != UX_HOST_CLASS_VIDEO_SET_CUR ||
        _last_control_transfer.ux_slave_transfer_request_setup[3] != UX_HOST_CLASS_VIDEO_TCS_PU_BRIGHTNESS_CONTROL ||
        _last_control_transfer.ux_slave_transfer_request_setup[4] != 0 /* interface number */ ||
        _last_control_transfer.ux_slave_transfer_request_setup[5] != 5 /* entity id */
        )
    {
        printf("ERROR #%d: setup incorrect\n", __LINE__);
        printf(" %x %x",
            _last_control_transfer.ux_slave_transfer_request_setup[0],
            _last_control_transfer.ux_slave_transfer_request_setup[1]);
        printf(" %x %x %x",
            _last_control_transfer.ux_slave_transfer_request_setup[3],
            _last_control_transfer.ux_slave_transfer_request_setup[4],
            _last_control_transfer.ux_slave_transfer_request_setup[5]);
        test_control_return(1);
    }
    if (_last_control_data[0] != '0' || _last_control_data[1] != '1')
    {
        printf("ERROR #%d: control data not sent\n", __LINE__);
        test_control_return(1);
    }

    /* UX_HOST_CLASS_VIDEO_GET_CUR :: UX_HOST_CLASS_VIDEO_TCS_PU_BRIGHTNESS_CONTROL */
    _ux_utility_memory_set(parameter_u8, 0, sizeof(parameter_u32));
    status = _ux_host_class_video_control_request(host_video,
        UX_HOST_CLASS_VIDEO_GET_CUR, 0,
        host_video -> ux_host_class_video_feature_unit_id,
        UX_HOST_CLASS_VIDEO_TCS_PU_BRIGHTNESS_CONTROL,
        parameter_u8, 2);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: request error 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (_last_control_transfer.ux_slave_transfer_request_setup[0] != 0xA1 ||
        _last_control_transfer.ux_slave_transfer_request_setup[1] != UX_HOST_CLASS_VIDEO_GET_CUR ||
        _last_control_transfer.ux_slave_transfer_request_setup[3] != UX_HOST_CLASS_VIDEO_TCS_PU_BRIGHTNESS_CONTROL ||
        _last_control_transfer.ux_slave_transfer_request_setup[4] != 0 /* interface number */ ||
        _last_control_transfer.ux_slave_transfer_request_setup[5] != 5 /* entity id */
        )
    {
        printf("ERROR #%d: setup incorrect\n", __LINE__);
        printf(" %x %x",
            _last_control_transfer.ux_slave_transfer_request_setup[0],
            _last_control_transfer.ux_slave_transfer_request_setup[1]);
        printf(" %x %x %x",
            _last_control_transfer.ux_slave_transfer_request_setup[3],
            _last_control_transfer.ux_slave_transfer_request_setup[4],
            _last_control_transfer.ux_slave_transfer_request_setup[5]);
        test_control_return(1);
    }
    if (parameter_u8[0] != '0' || parameter_u8[1] != '1')
    {
        printf("ERROR #%d: control data not sent\n", __LINE__);
        test_control_return(1);
    }

    /* UX_HOST_CLASS_VIDEO_SET_CUR :: UX_HOST_CLASS_VIDEO_TCS_VS_PROBE_CONTROL */
    _ux_utility_memory_set(_last_control_data, 0, 34);
    for(loop = 0; loop < 34; loop++) parameter_u8[loop] = loop;
    status = _ux_host_class_video_control_request(host_video,
        UX_HOST_CLASS_VIDEO_SET_CUR, 1,
        0,
        UX_HOST_CLASS_VIDEO_TCS_VS_PROBE_CONTROL,
        parameter_u8, 34);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: request error 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (_last_control_transfer.ux_slave_transfer_request_setup[0] != 0x21 ||
        _last_control_transfer.ux_slave_transfer_request_setup[1] != UX_HOST_CLASS_VIDEO_SET_CUR ||
        _last_control_transfer.ux_slave_transfer_request_setup[3] != UX_HOST_CLASS_VIDEO_TCS_VS_PROBE_CONTROL ||
        _last_control_transfer.ux_slave_transfer_request_setup[4] != 1 /* interface number */ ||
        _last_control_transfer.ux_slave_transfer_request_setup[5] != 0 /* entity id */
        )
    {
        printf("ERROR #%d: setup incorrect\n", __LINE__);
        printf(" %x %x",
            _last_control_transfer.ux_slave_transfer_request_setup[0],
            _last_control_transfer.ux_slave_transfer_request_setup[1]);
        printf(" %x %x %x",
            _last_control_transfer.ux_slave_transfer_request_setup[3],
            _last_control_transfer.ux_slave_transfer_request_setup[4],
            _last_control_transfer.ux_slave_transfer_request_setup[5]);
        test_control_return(1);
    }
    for (loop = 0; loop < 34; loop ++)
    {
        if (_last_control_data[loop] != loop)
        {
            printf("ERROR #%d: control data not correct\n", __LINE__);
            test_control_return(1);
        }
    }

    /* UX_HOST_CLASS_VIDEO_GET_CUR :: UX_HOST_CLASS_VIDEO_TCS_VS_PROBE_CONTROL */
    _ux_utility_memory_set(parameter_u8, 0, sizeof(parameter_u32));
    status = _ux_host_class_video_control_request(host_video,
        UX_HOST_CLASS_VIDEO_GET_CUR, 1,
        0,
        UX_HOST_CLASS_VIDEO_TCS_VS_PROBE_CONTROL,
        parameter_u8, 34);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d: request error 0x%x\n", __LINE__, status);
        test_control_return(1);
    }
    if (_last_control_transfer.ux_slave_transfer_request_setup[0] != 0xA1 ||
        _last_control_transfer.ux_slave_transfer_request_setup[1] != UX_HOST_CLASS_VIDEO_GET_CUR ||
        _last_control_transfer.ux_slave_transfer_request_setup[3] != UX_HOST_CLASS_VIDEO_TCS_VS_PROBE_CONTROL ||
        _last_control_transfer.ux_slave_transfer_request_setup[4] != 1 /* interface number */ ||
        _last_control_transfer.ux_slave_transfer_request_setup[5] != 0 /* entity id */
        )
    {
        printf("ERROR #%d: setup incorrect\n", __LINE__);
        printf(" %x %x",
            _last_control_transfer.ux_slave_transfer_request_setup[0],
            _last_control_transfer.ux_slave_transfer_request_setup[1]);
        printf(" %x %x %x",
            _last_control_transfer.ux_slave_transfer_request_setup[3],
            _last_control_transfer.ux_slave_transfer_request_setup[4],
            _last_control_transfer.ux_slave_transfer_request_setup[5]);
        test_control_return(1);
    }
    for (loop = 0; loop < 34; loop ++)
    {
        if (parameter_u8[loop] != loop)
        {
            printf("ERROR #%d: control data not correct\n", __LINE__);
            test_control_return(1);
        }
    }

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    if (host_video != UX_NULL)
    {

        printf("ERROR #13: instance not removed when disconnect");
        test_control_return(1);
    }

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status  = ux_device_stack_class_unregister(_ux_system_slave_class_dpump_name, _ux_device_class_dummy_entry);
    status |= ux_device_stack_class_unregister(_ux_system_slave_class_dpump_name, _ux_device_class_dummy_entry);

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
