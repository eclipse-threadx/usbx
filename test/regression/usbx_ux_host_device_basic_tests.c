/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "fx_api.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_device_stack.h"
#include "ux_host_class_cdc_acm.h"

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
static VOID                                test_cdc_instance_activate(VOID  *cdc_instance);
static VOID                                test_cdc_instance_deactivate(VOID *cdc_instance);
static VOID                                test_cdc_instance_parameter_change(VOID *cdc_instance);

static UINT                                test_slave_change_function(ULONG change);

static VOID                                ux_test_hcd_entry_should_not_be_called(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_disconnect(UX_TEST_ACTION *action, VOID *params);
static VOID                                ux_test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);

static VOID                                ux_test_system_host_enum_hub_function(VOID);

/* Define global data structures.  */
UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
UX_HOST_CLASS                       *class_driver;
UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;
ULONG                               command_received_count;
UCHAR                               cdc_acm_reception_buffer[UX_DEMO_RECEPTION_BUFFER_SIZE];
UCHAR                               cdc_acm_xmit_buffer[UX_DEMO_XMIT_BUFFER_SIZE];
UX_HOST_CLASS_CDC_ACM_RECEPTION     cdc_acm_reception;
UCHAR                               *global_reception_buffer;
ULONG                               global_reception_size;

UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
UCHAR                               cdc_acm_slave_change;
UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;
UCHAR                               buffer[UX_DEMO_BUFFER_SIZE];
ULONG                               echo_mode;

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

/* Define device framework.  */

#define             DEVICE_FRAMEWORK_LENGTH_FULL_SPEED      93
#define             DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED      103
#define             STRING_FRAMEWORK_LENGTH                 47
#define             LANGUAGE_ID_FRAMEWORK_LENGTH            2

static unsigned char device_framework_full_speed[] = {

    /* Device descriptor     18 bytes
       0x02 bDeviceClass:    CDC class code
       0x00 bDeviceSubclass: CDC class sub code
       0x00 bDeviceProtocol: CDC Device protocol

       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x10, 0x01,
    0xEF, 0x02, 0x01,
    0x08,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01,

    /* Configuration 1 descriptor 9 bytes */
    0x09, 0x02, 0x4b, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. 8 bytes.  */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement. 9 bytes.   */
    0x09, 0x04, 0x00,
    0x00,
    0x01,
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor 5 bytes */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor 4 bytes */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor 5 bytes */
    0x05, 0x24, 0x06,
    0x00,                          /* Master interface */
    0x01,                          /* Slave interface  */

    /* Call Management Functional Descriptor 5 bytes */
    0x05, 0x24, 0x01,
    0x03,
    0x01,                          /* Data interface   */

    /* Endpoint 0x83 descriptor 7 bytes */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement 9 bytes */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor 7 bytes */
    0x07, 0x05, 0x81, /* @ 93 - 14 + 2 = 81 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor 7 bytes */
    0x07, 0x05, 0x02, /* @ 93 - 7 + 2 = 88 */
    0x02,
    0x40, 0x00,
    0x00,

};

#define DEVICE_FRAMEWORK_EPA_POS_1_FS (DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 14 + 2)
#define DEVICE_FRAMEWORK_EPA_POS_2_FS (DEVICE_FRAMEWORK_LENGTH_FULL_SPEED - 7 + 2)

static unsigned char device_framework_high_speed[] = {

    /* Device descriptor
       0x02 bDeviceClass:    CDC class code
       0x00 bDeviceSubclass: CDC class sub code
       0x00 bDeviceProtocol: CDC Device protocol

       idVendor & idProduct - http://www.linux-usb.org/usb.ids
    */
    0x12, 0x01, 0x00, 0x02,
    0xEF, 0x02, 0x01,
    0x40,
    0x84, 0x84, 0x00, 0x00,
    0x00, 0x01,
    0x01, 0x02, 03,
    0x01,

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02,
    0x02, 0x00, 0x00,
    0x40,
    0x01,
    0x00,

    /* Configuration 1 descriptor */
    0x09, 0x02, 0x4b, 0x00,
    0x02, 0x01, 0x00,
    0x40, 0x00,

    /* Interface association descriptor. */
    0x08, 0x0b, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,

    /* Communication Class Interface Descriptor Requirement */
    0x09, 0x04, 0x00,
    0x00,
    0x01,
    0x02, 0x02, 0x01,
    0x00,

    /* Header Functional Descriptor */
    0x05, 0x24, 0x00,
    0x10, 0x01,

    /* ACM Functional Descriptor */
    0x04, 0x24, 0x02,
    0x0f,

    /* Union Functional Descriptor */
    0x05, 0x24, 0x06,
    0x00,
    0x01,

    /* Call Management Functional Descriptor */
    0x05, 0x24, 0x01,
    0x00,
    0x01,

    /* Endpoint 0x83 descriptor */
    0x07, 0x05, 0x83,
    0x03,
    0x08, 0x00,
    0xFF,

    /* Data Class Interface Descriptor Requirement */
    0x09, 0x04, 0x01,
    0x00,
    0x02,
    0x0A, 0x00, 0x00,
    0x00,

    /* Endpoint 0x81 descriptor */
    0x07, 0x05, 0x81, /* @ 103 - 14 + 2 = 91 */
    0x02,
    0x40, 0x00,
    0x00,

    /* Endpoint 0x02 descriptor */
    0x07, 0x05, 0x02, /* @ 103 - 7 + 2 = 98 */
    0x02,
    0x40, 0x00,
    0x00,

};

#define DEVICE_FRAMEWORK_EPA_POS_1_HS (DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 14 + 2)
#define DEVICE_FRAMEWORK_EPA_POS_2_HS (DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED - 7 + 2)

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

/* Disconnect on RESET:
 * Host still create the EP0 and expects first SETUP request failure.
 */

static UX_TEST_HCD_SIM_ACTION disconnect_on_reset[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_RESET_PORT, NULL,
        UX_TRUE , UX_TEST_PORT_STATUS_DISC,
        0         , 0, UX_NULL, 0, 0,
        UX_SUCCESS, ux_test_hcd_entry_disconnect},
#if 0
{   UX_HCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR , ux_test_hcd_entry_should_not_be_called},
#endif
{   UX_HCD_TRANSFER_REQUEST, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR , UX_NULL}, /* Request just fail on disconnect */
{   UX_HCD_TRANSFER_REQUEST, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR , UX_NULL},
{   UX_HCD_TRANSFER_REQUEST, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR , UX_NULL},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION endpoint0_create_fail[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR},
{   UX_HCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR},
{   UX_HCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR},
{   UX_HCD_TRANSFER_REQUEST, NULL,
        UX_FALSE, 0,
        0         , 0, UX_NULL, 0, 0,
        UX_ERROR , ux_test_hcd_entry_should_not_be_called},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION disconnect_on_SetAddress[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetAddress,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_ERROR},
{   UX_HCD_TRANSFER_REQUEST, &_SetAddress,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_ERROR},
{   UX_HCD_TRANSFER_REQUEST, &_SetAddress,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_ERROR},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION disconnect_on_GetDevDescr[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_GetDeviceDescriptor,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ_V, 0, UX_NULL, 0, 0,
        UX_ERROR},
{   UX_HCD_TRANSFER_REQUEST, &_GetDeviceDescriptor,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ_V, 0, UX_NULL, 0, 0,
        UX_ERROR},
{   UX_HCD_TRANSFER_REQUEST, &_GetDeviceDescriptor,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ_V, 0, UX_NULL, 0, 0,
        0, UX_NULL,
        UX_TRUE},
{   UX_HCD_TRANSFER_REQUEST, &_GetDeviceDescriptor,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ_V, 0, UX_NULL, 0, 0,
        UX_ERROR},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION disconnect_on_GetCfgDescr[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ_V, 0, UX_NULL, 0, 0,
        UX_ERROR}, /* First try */
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ_V, 0, UX_NULL, 0, 0,
        UX_ERROR}, /* Second try */
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_FALSE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ_V, 0, UX_NULL, 0, 0,
        UX_SUCCESS, UX_NULL,
        UX_TRUE}, /* Last try first run */
{   UX_HCD_TRANSFER_REQUEST, &_GetConfigDescriptor,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ_V, 0, UX_NULL, 0, 0,
        UX_ERROR},
{   0   }
};

static UX_TEST_HCD_SIM_ACTION disconnect_on_SetCfg[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_ERROR}, /* First try */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_ERROR}, /* Second try */
{   UX_HCD_TRANSFER_REQUEST, &_SetConfigure,
        UX_TRUE, UX_TEST_PORT_STATUS_DISC,
        UX_TEST_SETUP_MATCH_REQ, 0, UX_NULL, 0, 0,
        UX_ERROR}, /* Last try */
{   0   }
};

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


static UINT test_class_cdc_acm_get(void)
{

UINT                                status;
UX_HOST_CLASS                       *class;
UX_HOST_CLASS_CDC_ACM               *cdc_acm_host;


    /* Find the main cdc_acm container */
    status =  ux_host_stack_class_get(_ux_system_host_class_cdc_acm_name, &class);
    if (status != UX_SUCCESS)
        return(status);

    /* We get the first instance of the cdc_acm device */
    do
    {

        status =  ux_host_stack_class_instance_get(class, 0, (void **) &cdc_acm_host);
        tx_thread_sleep(10);
    } while (status != UX_SUCCESS);

    /* We still need to wait for the cdc_acm status to be live */
    while (cdc_acm_host -> ux_host_class_cdc_acm_state != UX_HOST_CLASS_INSTANCE_LIVE)
        tx_thread_sleep(10);

    /* Isolate both the control and data interfaces.  */
    if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_DATA_CLASS)
    {
        /* This is the data interface.  */
        cdc_acm_host_data = cdc_acm_host;

        /* In that case, the second one should be the control interface.  */
        status =  ux_host_stack_class_instance_get(class, 1, (void **) &cdc_acm_host);

        /* Check error.  */
        if (status != UX_SUCCESS)
            return(status);

        /* Check for the control interfaces.  */
        if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
        {

            /* This is the control interface.  */
            cdc_acm_host_control = cdc_acm_host;

            return(UX_SUCCESS);

        }
    }
    else
    {
        /* Check for the control interfaces.  */
        if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
        {

            /* This is the control interface.  */
            cdc_acm_host_control = cdc_acm_host;

            /* In that case, the second one should be the data interface.  */
            status =  ux_host_stack_class_instance_get(class, 1, (void **) &cdc_acm_host);

            /* Check error.  */
            if (status != UX_SUCCESS)
                return(status);

            /* Check for the data interface.  */
            if (cdc_acm_host -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_DATA_CLASS)
            {

                /* This is the data interface.  */
                cdc_acm_host_data = cdc_acm_host;

                return(UX_SUCCESS);

            }
        }
    }

    /* Return ERROR.  */
    return(UX_ERROR);
}

static UINT test_slave_change_function(ULONG change)
{
    return 0;
}

static UINT test_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{

UX_HOST_CLASS_CDC_ACM *cdc_acm = (UX_HOST_CLASS_CDC_ACM *) inst;

    switch(event)
    {

        case UX_DEVICE_INSERTION:

            if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
                cdc_acm_host_control = cdc_acm;
            else
                cdc_acm_host_data = cdc_acm;
            break;

        case UX_DEVICE_REMOVAL:

            if (cdc_acm -> ux_host_class_cdc_acm_interface -> ux_interface_descriptor.bInterfaceClass == UX_HOST_CLASS_CDC_CONTROL_CLASS)
                cdc_acm_host_control = UX_NULL;
            else
                cdc_acm_host_data = UX_NULL;
            break;

        default:
            break;
    }
    return 0;
}

static VOID    test_cdc_instance_activate(VOID *cdc_instance)
{

    /* Save the CDC instance.  */
    cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}
static VOID    test_cdc_instance_deactivate(VOID *cdc_instance)
{

    /* Reset the CDC instance.  */
    cdc_acm_slave = UX_NULL;
}

static VOID test_cdc_instance_parameter_change(VOID *cdc_instance)
{

    /* Set CDC parameter change flag. */
    cdc_acm_slave_change = UX_TRUE;
}

static VOID test_swap_framework_bulk_ep_descriptors(VOID)
{
UCHAR tmp;

    tmp = device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_1_FS];
    device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_1_FS] = device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_2_FS];
    device_framework_full_speed[DEVICE_FRAMEWORK_EPA_POS_2_FS] = tmp;

    tmp = device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_1_HS];
    device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_1_HS] = device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_2_HS];
    device_framework_high_speed[DEVICE_FRAMEWORK_EPA_POS_2_HS] = tmp;
}


static VOID test_ux_error_callback(UINT system_level, UINT system_context, UINT error_code)
{
    int x = 0;
}

static VOID ux_test_hcd_entry_should_not_be_called(UX_TEST_ACTION *action, VOID *params)
{

    error_counter ++;
}

static VOID ux_test_hcd_entry_disconnect(UX_TEST_ACTION *action, VOID *params)
{

    ux_test_dcd_sim_slave_disconnect();
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
void    usbx_host_device_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;


    printf("Running Host & Device Basic Functionality Test...................... ");

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
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    if (status != UX_SUCCESS)
    {

        printf(" ERROR #3\n");
        test_control_return(1);
    }

    /* Simulates hub enum function */
    enum_counter = 0;
#if UX_MAX_DEVICES > 1
    _ux_system_host->ux_system_host_enum_hub_function = ux_test_system_host_enum_hub_function;
#endif

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

    /* Set the parameters for callback when insertion/extraction of a CDC device.  */
    parameter.ux_slave_class_cdc_acm_instance_activate   =  test_cdc_instance_activate;
    parameter.ux_slave_class_cdc_acm_instance_deactivate =  test_cdc_instance_deactivate;
    parameter.ux_slave_class_cdc_acm_parameter_change    =  test_cdc_instance_parameter_change;

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry,
                                             1,0,  &parameter);

    if(status!=UX_SUCCESS)
    {

        printf(" ERROR #7\n");
        test_control_return(1);
    }

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

void  tx_test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               test_n;
ULONG                                               mem_free;

    stepinfo("\n");

    /* Find the cdc_acm class and wait for the link to be up.  */
    status =  test_class_cdc_acm_get();
    if (status != UX_SUCCESS)
    {

        /* DPUMP basic test error.  */
        printf("ERROR #11: class not found\n");
        test_control_return(1);
    }
    if (!cdc_acm_host_control && !cdc_acm_host_data)
    {

        printf("ERROR #12: instance not detected\n");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    /* Test disconnect. */
    stepinfo(">>>>>>>>>>>>>>>> Test disconnect\n");
    ux_test_dcd_sim_slave_disconnect();
    ux_test_hcd_sim_host_disconnect();
    if (cdc_acm_host_control || cdc_acm_host_data)
    {

        printf("ERROR #13: instance not removed when disconnect");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    /* Reset testing counts. */
    stepinfo(">>>>>>>>>>>>>>>> Test connect & connection resource\n");
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    ux_test_hcd_sim_host_set_actions(log_on_SetCfg);
    mem_free = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    /* Log create counts when SetConfigure for further tests. */
    rsc_enum_mutex_usage = rsc_mutex_on_set_cfg;
    rsc_enum_sem_usage = rsc_sem_on_set_cfg;
    rsc_enum_sem_get_count = rsc_sem_get_on_set_cfg;
    rsc_enum_mem_usage = mem_free - rsc_mem_free_on_set_cfg;
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    /* Reset enum counter */
    enum_counter = 0;

    stepinfo(">>>>>>>>>>>>>>>> Test disconnect on reset\n");
    ux_test_hcd_sim_host_disconnect();
    ux_test_hcd_sim_host_set_actions(disconnect_on_reset);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    if (cdc_acm_host_control || cdc_acm_host_data)
    {

        printf("ERROR #13: instance installed when disconnect on reset");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
#if UX_MAX_DEVICES > 1
    /* Enumeration should be processed */
    if (enum_counter == 0)
    {

        printf("ERROR #%d: enum entry not invoked\n", __LINE__);
        test_control_return(1);
    }
#endif
    stepinfo(">>>>>>>>>>>>>>>> Test EP0 fail\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_disconnect();
    ux_test_hcd_sim_host_set_actions(endpoint0_create_fail);
    error_counter = 0;
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    if (error_counter)
    {

        /* DPUMP basic test error.  */
        printf("ERROR #14: detect EP transfer when EP0 creation fail\n");
        test_control_return(1);
    }
    if (cdc_acm_host_control || cdc_acm_host_data)
    {

        printf("ERROR #15: instance installed when EP0 creation fail\n");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    stepinfo(">>>>>>>>>>>>>>>> Test disconnect on SetAddress\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_disconnect();
    ux_test_hcd_sim_host_set_actions(disconnect_on_SetAddress);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    if (cdc_acm_host_control)
    {

        printf("ERROR #14: detect device when disconnect on SetAddress\n");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    stepinfo(">>>>>>>>>>>>>>>> Test disconnect on GetDeviceDescriptor\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_disconnect();
    ux_test_hcd_sim_host_set_actions(disconnect_on_GetDevDescr);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    if (cdc_acm_host_control)
    {

        printf("ERROR #15: detect device when disconnect on GetDeviceDescriptor\n");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    stepinfo(">>>>>>>>>>>>>>>> Test disconnect on GetConfigureDescriptor\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_disconnect();
    ux_test_hcd_sim_host_set_actions(disconnect_on_GetCfgDescr);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    if (cdc_acm_host_control)
    {

        printf("ERROR #16: detect device when disconnect on GetConfigureDescriptor\n");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    stepinfo(">>>>>>>>>>>>>>>> Test disconnect on SetConfigure\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_disconnect();
    ux_test_hcd_sim_host_set_actions(disconnect_on_SetCfg);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    if (cdc_acm_host_control)
    {

        printf("ERROR #17: detect device when disconnect on SetConfigure\n");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    stepinfo(">>>>>>>>>>>>>>>> Normal replace descriptor test\n");
    ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
    ux_test_hcd_sim_host_disconnect();
    ux_test_hcd_sim_host_set_actions(normal_enum_replace);
    ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);
    tx_thread_sleep(100);
    if (!cdc_acm_host_control)
    {

        printf("ERROR #18: no device detected when replacing DevDescr and CfgDescr\n");
        test_control_return(1);
    }
    stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);

    /* Swap bulk IN/OUT endpoint position.
       Simulate detach and attach for HS enumeration,
       and test possible mutex creation error handlings.
     */
    if (rsc_enum_mutex_usage) stepinfo(">>>>>>>>>>>>>>>> Mutex errors enumeration test\n");
    for (test_n = 0; test_n < rsc_enum_mutex_usage; test_n ++)
    {

        stepinfo("%ld / %ld\n", test_n, rsc_enum_mutex_usage - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Swap EP address. */
        test_swap_framework_bulk_ep_descriptors();

        /* Generate error while the test_n and after mutex are requested */
        ux_test_utility_sim_mutex_error_generation_start(test_n);

        /* Count SetConfigure */
        set_cfg_counter = 0;
        ux_test_hcd_sim_host_set_actions(log_on_SetCfg);

        /* Connect. */
        ux_test_dcd_sim_slave_connect(UX_HIGH_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_HIGH_SPEED_DEVICE);

        if (set_cfg_counter)
        {

            printf("ERROR #19.%ld: device detected when there is mutex error\n", test_n);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_mutex_error_generation_stop();
    if (rsc_enum_mutex_usage) stepinfo("\n");

    /* Simulate detach and attach for FS enumeration,
       and test possible semaphore creation error handlings.
       Note CDC ACM has two semaphore for bulk endpoints, not tested here.
     */
    if (rsc_enum_sem_usage) stepinfo(">>>>>>>>>>>>>>>> Semaphore errors enumeration test\n");
    for (test_n = 0; test_n < rsc_enum_sem_usage; test_n ++)
    {

        stepinfo("%2ld / %2ld\n", test_n, rsc_enum_sem_usage - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Generate error while the test_n and after semaphore are requested */
        ux_test_utility_sim_sem_error_generation_start(test_n);

        /* Count SetConfigure */
        set_cfg_counter = 0;
        ux_test_hcd_sim_host_set_actions(log_on_SetCfg);

        /* Connect. */
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        /* Check error */
        if (set_cfg_counter)
        {

            printf("ERROR #21.%ld: device detected when there is semaphore error\n", test_n);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_sem_error_generation_stop();
    if (rsc_enum_sem_usage) stepinfo("\n");

    /* Simulate detach and attach for FS enumeration,
       and test possible semaphore get error handlings.
     */
    if (rsc_enum_sem_get_count) stepinfo(">>>>>>>>>>>>>>>> Semaphore GET errors enumeration test\n");
    for (test_n = 0; test_n < rsc_enum_sem_get_count; test_n ++)
    {

        stepinfo("%2ld / %2ld\n", test_n, rsc_enum_sem_get_count - 1);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        /* Generate error while the test_n and after semaphore are requested */
        ux_test_utility_sim_sem_get_error_generation_start(test_n);

        /* Count SetConfigure */
        set_cfg_counter = 0;
        ux_test_hcd_sim_host_set_actions(log_on_SetCfg);

        /* Connect. */
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        if (set_cfg_counter)
        {

            printf("ERROR #24.%ld: device detected when there is semaphore GET error\n", test_n);
            test_control_return(1);
        }
        stepinfo("mem free: %ld\n", _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    }
    ux_test_utility_sim_sem_get_error_generation_stop();
    if (rsc_enum_sem_usage) stepinfo("\n");

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry);

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
