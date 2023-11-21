/* This test is designed to test the ux_utility_descriptor_parse.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_test.h"

#include "ux_device_class_dfu.h"
#include "ux_host_class_audio.h"
#include "ux_host_class_cdc_ecm.h"
#include "ux_host_class_hid.h"
#include "ux_host_class_hub.h"
#include "ux_host_class_pima.h"
#include "ux_host_class_video.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;


/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                ux_test_instance_activate(VOID  *dpump_instance);
static VOID                ux_test_instance_deactivate(VOID *dpump_instance);

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}


static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
            // test_control_return(1);
        }
    }
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_utility_descriptor_struct_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running ux_utility_descriptor_ structures Test...................... ");

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

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

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

    /* Print STD framework descriptor's sizes  */
    printf("\n");
    printf("Descriptor    %7s %7s %7s\n", "nFields", "Size4", "SizeNew");
    printf("Device        %7d %7d %7u\n", UX_DEVICE_DESCRIPTOR_ENTRIES, UX_DEVICE_DESCRIPTOR_ENTRIES * 4, (unsigned)_ux_utility_descriptor_parse_size(_ux_system_device_descriptor_structure, UX_DEVICE_DESCRIPTOR_ENTRIES, 0x3u));
    printf("Configuration %7d %7d %7u\n", UX_CONFIGURATION_DESCRIPTOR_ENTRIES, UX_CONFIGURATION_DESCRIPTOR_ENTRIES * 4, (unsigned)_ux_utility_descriptor_parse_size(_ux_system_configuration_descriptor_structure, UX_CONFIGURATION_DESCRIPTOR_ENTRIES, 0x3u));
    printf("Interface     %7d %7d %7u\n", UX_INTERFACE_DESCRIPTOR_ENTRIES, UX_INTERFACE_DESCRIPTOR_ENTRIES * 4, (unsigned)_ux_utility_descriptor_parse_size(_ux_system_interface_descriptor_structure, UX_INTERFACE_DESCRIPTOR_ENTRIES, 0x3u));
    printf("Endpoint      %7d %7d %7u\n", UX_ENDPOINT_DESCRIPTOR_ENTRIES, UX_ENDPOINT_DESCRIPTOR_ENTRIES * 4, (unsigned)_ux_utility_descriptor_parse_size(_ux_system_endpoint_descriptor_structure, UX_ENDPOINT_DESCRIPTOR_ENTRIES, 0x3u));

    /* Test struct parse sizes.  */
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_endpoint_descriptor_structure, UX_ENDPOINT_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_ENDPOINT_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_device_descriptor_structure, UX_DEVICE_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_DEVICE_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_configuration_descriptor_structure, UX_CONFIGURATION_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_CONFIGURATION_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_interface_descriptor_structure, UX_INTERFACE_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_INTERFACE_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_interface_association_descriptor_structure, UX_INTERFACE_ASSOCIATION_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_INTERFACE_ASSOCIATION_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_string_descriptor_structure, UX_STRING_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_STRING_DESCRIPTOR));

    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_dfu_functional_descriptor_structure, UX_DFU_FUNCTIONAL_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_DFU_FUNCTIONAL_DESCRIPTOR));

    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_bos_descriptor_structure, UX_BOS_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_BOS_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_usb_2_0_extension_descriptor_structure, UX_USB_2_0_EXTENSION_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_USB_2_0_EXTENSION_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_container_id_descriptor_structure, UX_CONTAINER_ID_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_CONTAINER_ID_DESCRIPTOR));

    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_class_audio_interface_descriptor_structure, UX_HOST_CLASS_AUDIO_INTERFACE_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HOST_CLASS_AUDIO_INTERFACE_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_class_audio_input_terminal_descriptor_structure, UX_HOST_CLASS_AUDIO_INPUT_TERMINAL_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HOST_CLASS_AUDIO_INPUT_TERMINAL_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_class_audio_output_terminal_descriptor_structure, UX_HOST_CLASS_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HOST_CLASS_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_class_audio_feature_unit_descriptor_structure, UX_HOST_CLASS_AUDIO_FEATURE_UNIT_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HOST_CLASS_AUDIO_FEATURE_UNIT_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_class_audio_streaming_interface_descriptor_structure, UX_HOST_CLASS_AUDIO_STREAMING_INTERFACE_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HOST_CLASS_AUDIO_STREAMING_INTERFACE_DESCRIPTOR));
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_class_audio_streaming_endpoint_descriptor_structure, UX_HOST_CLASS_AUDIO_STREAMING_ENDPOINT_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HOST_CLASS_AUDIO_STREAMING_ENDPOINT_DESCRIPTOR));

    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_hid_descriptor_structure, UX_HID_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HID_DESCRIPTOR));

    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_class_pima_storage_structure, 8, 0x3u)) == 4*7);
    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_class_pima_object_structure, 15, 0x3u)) == 4*14);

    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_ecm_interface_descriptor_structure, UX_HOST_CLASS_CDC_ECM_INTERFACE_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HOST_CLASS_ECM_INTERFACE_DESCRIPTOR));

    UX_TEST_ASSERT((_ux_utility_descriptor_parse_size(_ux_system_hub_descriptor_structure, UX_HUB_DESCRIPTOR_ENTRIES, 0x3u)) == sizeof(UX_HUB_DESCRIPTOR));

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
