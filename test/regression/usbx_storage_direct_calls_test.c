#include "usbx_test_common_storage.h"

#include "tx_thread.h"


#define DUMMY_USBX_MEMORY_SIZE (64*1024)
static UCHAR dummy_usbx_memory[DUMMY_USBX_MEMORY_SIZE];

#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 50
static UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0x81, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00

    };


#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 60
static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x81, 0x07, 0x00, 0x00, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x00, 0x01, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x00, 0x01, 0x00

    };


    /* String Device Framework :
     Byte 0 and 1 : Word containing the language ID : 0x0904 for US
     Byte 2       : Byte containing the index of the descriptor
     Byte 3       : Byte containing the length of the descriptor string
    */

#define STRING_FRAMEWORK_LENGTH 38
static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0a,
        0x46, 0x6c, 0x61, 0x73, 0x68, 0x20, 0x44, 0x69,
        0x73, 0x6b,

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

    //printf("Error on line %d, system_level: %d, system_context: %d, error code: %d\n", __LINE__, system_level, system_context, error_code);
}

static UINT    demo_system_host_change_function(ULONG event_code, UX_HOST_CLASS *class, VOID *instance)
{

UINT    status;


    if (event_code == UX_DEVICE_INSERTION)
    {

        status = ux_utility_semaphore_put(&storage_instance_live_semaphore);
        if (status)
        {

            printf("Error on line %d, error code: %d\n", __LINE__, status);
            test_control_return(1);
        }
    }

    return 0;
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_storage_direct_calls_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR *                          stack_pointer;
CHAR *                          memory_pointer;


    printf("Running Storage Direct Calls Test............................ ");

    status = ux_utility_semaphore_create(&storage_instance_live_semaphore, "storage_instance_live_semaphore", 0);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize FileX.  */
    fx_system_initialize();

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(demo_system_host_change_function);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* Register storage class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_storage_name, ux_host_class_storage_entry);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Reset ram disk memory.  */
    ux_utility_memory_set(global_ram_disk_memory, 0, UX_RAM_DISK_SIZE);

    /* The code below is required for installing the device portion of USBX.
       In this demo, DFU is possible and we have a call back for state change. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Store the number of LUN in this device storage instance.  */
    global_storage_parameter.ux_slave_class_storage_parameter_number_lun = 1;

    /* Initialize the storage class parameters for reading/writing to the Flash Disk.  */
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_last_lba        =  UX_RAM_DISK_LAST_LBA;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_block_length    =  512;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_type            =  0;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_removable_flag  =  0x80;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_read            =  default_device_media_read;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_write           =  default_device_media_write;
    global_storage_parameter.ux_slave_class_storage_parameter_lun[0].ux_slave_class_storage_media_status          =  default_device_media_status;

    /* Initilize the device storage class. The class is connected with interface 0 on configuration 1. */
    status =  ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                                1, 0, (VOID *)&global_storage_parameter);
    if(status!=UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,
            stack_pointer, UX_DEMO_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

}


static UINT ux_hcd_sim_host_entry_filter_transfer_request_failed_test1(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                        status;
UX_TRANSFER                 *transfer_request;
UINT                        is_csw = UX_ERROR;
UX_MEMORY_BLOCK             *dummy_memory_block_first = (UX_MEMORY_BLOCK *)dummy_usbx_memory;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        if (transfer_request->ux_transfer_request_data_pointer != UX_NULL)
            is_csw = ux_utility_memory_compare("IS_CSW", transfer_request -> ux_transfer_request_data_pointer, 6);
    }

    status = _ux_hcd_sim_host_entry(hcd, function, parameter);

    /* Get Port Status requests don't return UX_SUCCESS. */
    if (status != UX_SUCCESS && function != UX_HCD_GET_PORT_STATUS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        /* Is it bulk? */
        if ((transfer_request->ux_transfer_request_endpoint->ux_endpoint_descriptor.bmAttributes & 3) == 2)
        {

            status = ux_utility_semaphore_get(&transfer_request->ux_transfer_request_semaphore, UX_WAIT_FOREVER);
            if (status)
            {

                printf("Error on line %d, error code: %d\n", __LINE__, status);
                test_control_return(1);
            }

            if (transfer_request->ux_transfer_request_completion_code != UX_SUCCESS)
            {

                printf("Error on line %d, error code: %d\n", __LINE__, status);
                test_control_return(1);
            }
        }

        /* Is this the CSW? */
        if (is_csw == UX_SUCCESS &&
            transfer_request -> ux_transfer_request_requested_length == 0x0d &&
            transfer_request -> ux_transfer_request_function == 0 &&
            transfer_request -> ux_transfer_request_type == 0x80 &&
            transfer_request -> ux_transfer_request_value == 0 &&
            transfer_request -> ux_transfer_request_index == 0)
        {

            dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
            dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
            dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
            dummy_memory_block_first -> ux_memory_block_size = 0;
            _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
            _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;
        }

        /* Is it bulk? */
        if ((transfer_request->ux_transfer_request_endpoint->ux_endpoint_descriptor.bmAttributes & 3) == 2)
        {

            status = ux_utility_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);
            if (status)
            {

                printf("Error on line %d, error code: %d\n", __LINE__, status);
                test_control_return(1);
            }
        }
    }

    return status;
}

static UINT ux_hcd_sim_host_entry_filter_transfer_request_failed_test2(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                        status;
UX_TRANSFER                 *transfer_request;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        if (ux_utility_memory_compare("IS_CBW", transfer_request -> ux_transfer_request_data_pointer, 6) == UX_SUCCESS)
        {

            transfer_request -> ux_transfer_request_semaphore.tx_semaphore_id++;

            return UX_SUCCESS;
        }
    }

    status = _ux_hcd_sim_host_entry(hcd, function, parameter);

    /* Get Port Status requests don't return UX_SUCCESS. */
    if (status != UX_SUCCESS && function != UX_HCD_GET_PORT_STATUS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    return status;
}

static UINT ux_hcd_sim_host_entry_filter_transfer_request_failed_test3(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                        status;
UX_TRANSFER                 *transfer_request;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        if (ux_utility_memory_compare("IS_CBW", transfer_request -> ux_transfer_request_data_pointer, 6) == UX_SUCCESS)
        {

            status = ux_utility_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, error code: %d\n", __LINE__, status);
                test_control_return(1);
            }

            transfer_request -> ux_transfer_request_completion_code = UX_SUCCESS;

            return UX_SUCCESS;
        }

        if (ux_utility_memory_compare("IS_DATA", transfer_request -> ux_transfer_request_data_pointer, 7) == UX_SUCCESS)
        {

            transfer_request -> ux_transfer_request_completion_code = UX_SUCCESS;

            transfer_request -> ux_transfer_request_semaphore.tx_semaphore_id++;

            return UX_SUCCESS;
        }
    }

    status = _ux_hcd_sim_host_entry(hcd, function, parameter);

    /* Get Port Status requests don't return UX_SUCCESS. */
    if (status != UX_SUCCESS && function != UX_HCD_GET_PORT_STATUS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    return status;
}

static UINT ux_hcd_sim_host_entry_filter_transfer_request_failed_test4(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                        status;
UX_TRANSFER                 *transfer_request;
static UINT                 csw_retries = 0;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        if (transfer_request -> ux_transfer_request_data_pointer != UX_NULL)
        {

            if (ux_utility_memory_compare("IS_CBW", transfer_request->ux_transfer_request_data_pointer, 6) == UX_SUCCESS ||
                ux_utility_memory_compare("IS_DATA", transfer_request->ux_transfer_request_data_pointer, 7) == UX_SUCCESS)
            {

                /* Release the semaphore because sim_host_entry() won't (doesn't get invoked). */
                status = ux_utility_semaphore_put(&transfer_request->ux_transfer_request_semaphore);
                if (status != UX_SUCCESS)
                {

                    printf("Error on line %d, error code: %d\n", __LINE__, status);
                    test_control_return(1);
                }

                transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;

                return UX_SUCCESS;
            }

            if (ux_utility_memory_compare("IS_CSW", transfer_request->ux_transfer_request_data_pointer, 6) == UX_SUCCESS)
            {

                if (csw_retries++ == 0)
                {

                    transfer_request->ux_transfer_request_semaphore.tx_semaphore_id++;
                }

                transfer_request->ux_transfer_request_completion_code = UX_ERROR;

                return UX_SUCCESS;
            }
        }
        else
        {

            /* Is this a port reset? */
            if (transfer_request->ux_transfer_request_data_pointer == UX_NULL &&
            transfer_request->ux_transfer_request_requested_length == 0 &&
            transfer_request->ux_transfer_request_function == UX_CLEAR_FEATURE &&
            (transfer_request->ux_transfer_request_type == (UX_REQUEST_OUT | UX_REQUEST_TYPE_STANDARD | UX_REQUEST_TARGET_ENDPOINT)) &&
            transfer_request->ux_transfer_request_value == UX_ENDPOINT_HALT)
            {

                transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;

                return UX_SUCCESS;
            }
        }
    }
    else if (function == UX_HCD_RESET_ENDPOINT)
    {

        return UX_SUCCESS;
    }

    status = _ux_hcd_sim_host_entry(hcd, function, parameter);

    /* Get Port Status requests don't return UX_SUCCESS. */
    if (status != UX_SUCCESS && function != UX_HCD_GET_PORT_STATUS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    return status;
}

static UINT ux_hcd_sim_host_entry_filter_transfer_request_failed_test5(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                        status;
UX_TRANSFER                 *transfer_request;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        if (ux_utility_memory_compare("IS_UFI", transfer_request -> ux_transfer_request_data_pointer, 6) == UX_SUCCESS)
        {

            status = ux_utility_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, error code: %d\n", __LINE__, status);
                test_control_return(1);
            }

            transfer_request -> ux_transfer_request_completion_code = UX_SUCCESS;

            return UX_SUCCESS;
        }

        if (ux_utility_memory_compare("IS_DATA", transfer_request -> ux_transfer_request_data_pointer, 7) == UX_SUCCESS)
        {

            transfer_request -> ux_transfer_request_completion_code = UX_SUCCESS;

            transfer_request -> ux_transfer_request_semaphore.tx_semaphore_id++;

            return UX_SUCCESS;
        }
    }

    status = _ux_hcd_sim_host_entry(hcd, function, parameter);

    /* Get Port Status requests don't return UX_SUCCESS. */
    if (status != UX_SUCCESS && function != UX_HCD_GET_PORT_STATUS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    return status;
}

static UINT ux_hcd_sim_host_entry_filter_transfer_request_failed_test6(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                        status;
UX_TRANSFER                 *transfer_request;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        if (ux_utility_memory_compare("IS_UFI", transfer_request -> ux_transfer_request_data_pointer, 6) == UX_SUCCESS)
        {

            status = ux_utility_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);
            if (status != UX_SUCCESS)
            {

                printf("Error on line %d, error code: %d\n", __LINE__, status);
                test_control_return(1);
            }

            transfer_request -> ux_transfer_request_completion_code = UX_SUCCESS;

            return UX_SUCCESS;
        }

        if (ux_utility_memory_compare("IS_DATA", transfer_request -> ux_transfer_request_data_pointer, 7) == UX_SUCCESS)
        {

            transfer_request -> ux_transfer_request_completion_code = UX_SUCCESS;

            transfer_request -> ux_transfer_request_semaphore.tx_semaphore_id++;

            return UX_SUCCESS;
        }
    }

    status = _ux_hcd_sim_host_entry(hcd, function, parameter);

    /* Get Port Status requests don't return UX_SUCCESS. */
    if (status != UX_SUCCESS && function != UX_HCD_GET_PORT_STATUS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    return status;
}

static UINT ux_hcd_sim_host_entry_filter_transfer_request_failed_test7(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                        status;
UX_TRANSFER                 *transfer_request;


    if (function == UX_HCD_TRANSFER_REQUEST)
    {

        transfer_request = parameter;

        if (transfer_request -> ux_transfer_request_data_pointer != UX_NULL)
        {

            if (ux_utility_memory_compare("IS_UFI", transfer_request->ux_transfer_request_data_pointer, 6) == UX_SUCCESS ||
                ux_utility_memory_compare("IS_DATA", transfer_request->ux_transfer_request_data_pointer, 7) == UX_SUCCESS)
            {

                /* Release the semaphore because sim_host_entry() won't (doesn't get invoked). */
                status = ux_utility_semaphore_put(&transfer_request->ux_transfer_request_semaphore);
                if (status != UX_SUCCESS)
                {

                    printf("Error on line %d, error code: %d\n", __LINE__, status);
                    test_control_return(1);
                }

                transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;

                return UX_SUCCESS;
            }

            /* Is this the status request via interrupt endpoint? */
            if (ux_utility_memory_compare("IS_INTERRUPT", transfer_request->ux_transfer_request_data_pointer, 12) == UX_SUCCESS)
            {

                /* We never created the interrupt endpoint's semaphore, so it will fail without having to do this. */
                //transfer_request->ux_transfer_request_semaphore.tx_semaphore_id++;

                transfer_request->ux_transfer_request_completion_code = UX_SUCCESS;

                return UX_SUCCESS;
            }
        }
    }

    status = _ux_hcd_sim_host_entry(hcd, function, parameter);

    /* Get Port Status requests don't return UX_SUCCESS. */
    if (status != UX_SUCCESS && function != UX_HCD_GET_PORT_STATUS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    return status;
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                            status;
UX_HOST_CLASS                   host_class;
UX_DEVICE                       device;
UX_CONFIGURATION                configuration;
UX_MEMORY_BLOCK                 *dummy_memory_block_first = (UX_MEMORY_BLOCK *)dummy_usbx_memory;
UX_MEMORY_BLOCK                 *original_regular_memory_block = _ux_system -> ux_system_regular_memory_pool_start;
UX_MEMORY_BLOCK                 *original_cache_safe_memory_block = _ux_system -> ux_system_cache_safe_memory_pool_start;
UX_INTERFACE                    interface;
UX_HOST_CLASS_COMMAND           command;
UX_HOST_CLASS_STORAGE           *storage_instance;
TX_SEMAPHORE                    storage_instance_semaphore_copy;
ALIGN_TYPE                      tmp;
UCHAR                           thread_stack[UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE];
UX_HCD                          *hcd;
UCHAR                           data_pointer[128];
UX_ENDPOINT                     interrupt_endpoint;
UCHAR                           interrupt_endopint_data_pointer[128];
UX_TRANSFER                     *transfer_request_ptr;


    /* Format the ram drive. */
    status =  fx_media_format(&global_ram_disk, _fx_ram_driver, global_ram_disk_memory, global_ram_disk_working_buffer, 512, "RAM DISK", 2, 512, 0, UX_RAM_DISK_SIZE/512, 512, 4, 1, 1);

    /* Check the media format status.  */
    if (status != FX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Open the ram_disk.  */
    status =  fx_media_open(&global_ram_disk, "RAM DISK", _fx_ram_driver, global_ram_disk_memory, global_ram_disk_working_buffer, 512);

    /* Check the media open status.  */
    if (status != FX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Find the storage class */
    status =  host_storage_instance_get();
    if (status != UX_SUCCESS)
    {

        /* Storage basic test error.  */
        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }
#ifdef UX_HOST_CLASS_STORAGE_INCLUDE_LEGACY_PROTOCOL_SUPPORT
    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_transport_cbi() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT); (first one) fails **/
    /**************************************************/

    /* Swap the hcd entry function. */
    hcd = UX_DEVICE_HCD_GET(global_storage -> ux_host_class_storage_device);
    tmp = (ULONG)hcd -> ux_hcd_entry_function;
    hcd -> ux_hcd_entry_function = ux_hcd_sim_host_entry_filter_transfer_request_failed_test6;

    /* Make sure we use the bulk out endpoint. */
    global_storage -> ux_host_class_storage_cbw[UX_HOST_CLASS_STORAGE_CBW_FLAGS] = UX_HOST_CLASS_STORAGE_DATA_OUT;

    /* Make the UFI portion easily detectable from filter function. */
    ux_utility_memory_copy(global_storage -> ux_host_class_storage_cbw + UX_HOST_CLASS_STORAGE_CBW_CB, "IS_UFI", 6);

    /* Make the data something easily detectable from filter function. */
    ux_utility_memory_copy(data_pointer, "IS_DATA", 7);

    /* Make sure storage_thread_entry() doesn't send a request at the same time. */
    status =  _ux_utility_semaphore_get(&global_storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status = _ux_host_class_storage_transport_cbi(global_storage, data_pointer);
    if(status != UX_TRANSFER_TIMEOUT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  _ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    hcd -> ux_hcd_entry_function = (UINT (*) (struct UX_HCD_STRUCT *, UINT, VOID *))tmp;
    global_storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request.ux_transfer_request_semaphore.tx_semaphore_id--;

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT); (second one) fails **/
    /**************************************************/

    /* Swap the hcd entry function. */
    hcd = UX_DEVICE_HCD_GET(global_storage -> ux_host_class_storage_device);
    tmp = (ULONG)hcd -> ux_hcd_entry_function;
    hcd -> ux_hcd_entry_function = ux_hcd_sim_host_entry_filter_transfer_request_failed_test7;

    /* The framework doesn't describe an interrupt endpoint, so give it one.  */
    transfer_request_ptr = &interrupt_endpoint.ux_endpoint_transfer_request;
    transfer_request_ptr->ux_transfer_request_data_pointer = interrupt_endopint_data_pointer;
    transfer_request_ptr->ux_transfer_request_endpoint = &interrupt_endpoint;
    interrupt_endpoint.ux_endpoint_device = global_storage -> ux_host_class_storage_device;
    interrupt_endpoint.ux_endpoint_descriptor.bmAttributes = UX_INTERRUPT_ENDPOINT_IN;
    global_storage -> ux_host_class_storage_interrupt_endpoint = &interrupt_endpoint;
    ux_utility_memory_copy(transfer_request_ptr -> ux_transfer_request_data_pointer, "IS_INTERRUPT", 12);

    /* Make sure we use the bulk out endpoint. */
    global_storage -> ux_host_class_storage_cbw[UX_HOST_CLASS_STORAGE_CBW_FLAGS] = UX_HOST_CLASS_STORAGE_DATA_OUT;

    /* Make the UFI portion easily detectable from filter function. */
    ux_utility_memory_copy(global_storage -> ux_host_class_storage_cbw + UX_HOST_CLASS_STORAGE_CBW_CB, "IS_UFI", 6);

    /* Make the data something easily detectable from filter function. */
    ux_utility_memory_copy(data_pointer, "IS_DATA", 7);

    /* Make sure storage_thread_entry() doesn't send a request at the same time. */
    status =  _ux_utility_semaphore_get(&global_storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status = _ux_host_class_storage_transport_cbi(global_storage, data_pointer);
    if(status != TX_SEMAPHORE_ERROR)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  _ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    hcd -> ux_hcd_entry_function = (UINT (*) (struct UX_HCD_STRUCT *, UINT, VOID *))tmp;
    global_storage -> ux_host_class_storage_interrupt_endpoint = UX_NULL;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_transport_cb() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT); fails **/
    /**************************************************/

    /* Swap the hcd entry function. */
    hcd = UX_DEVICE_HCD_GET(global_storage -> ux_host_class_storage_device);
    tmp = (ULONG)hcd -> ux_hcd_entry_function;
    hcd -> ux_hcd_entry_function = ux_hcd_sim_host_entry_filter_transfer_request_failed_test5;

    /* Make sure we use the bulk out endpoint. */
    global_storage -> ux_host_class_storage_cbw[UX_HOST_CLASS_STORAGE_CBW_FLAGS] = UX_HOST_CLASS_STORAGE_DATA_OUT;

    /* Make the UFI portion easily detectable from filter function. */
    ux_utility_memory_copy(global_storage -> ux_host_class_storage_cbw + UX_HOST_CLASS_STORAGE_CBW_CB, "IS_UFI", 6);

    /* Make the data something easily detectable from filter function. */
    ux_utility_memory_copy(data_pointer, "IS_DATA", 7);

    /* Make sure storage_thread_entry() doesn't send a request at the same time. */
    status =  _ux_utility_semaphore_get(&global_storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status = _ux_host_class_storage_transport_cb(global_storage, data_pointer);
    if(status != UX_TRANSFER_TIMEOUT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  _ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    hcd -> ux_hcd_entry_function = (UINT (*) (struct UX_HCD_STRUCT *, UINT, VOID *))tmp;
    global_storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request.ux_transfer_request_semaphore.tx_semaphore_id--;
#endif
    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_transport_bo() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT); (first one) fails **/
    /**************************************************/

    /* Swap the hcd entry function. */
    hcd = UX_DEVICE_HCD_GET(global_storage -> ux_host_class_storage_device);
    tmp = (ALIGN_TYPE)hcd -> ux_hcd_entry_function;
    hcd -> ux_hcd_entry_function = ux_hcd_sim_host_entry_filter_transfer_request_failed_test2;

    /* Make the CBW something easily detectable from filter function. */
    ux_utility_memory_copy(global_storage -> ux_host_class_storage_cbw, "IS_CBW", 6);

    /* Make sure storage_thread_entry() doesn't send a request at the same time. */
    status =  _ux_utility_semaphore_get(&global_storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status = _ux_host_class_storage_transport_bo(global_storage, UX_NULL);
    if(status != UX_TRANSFER_TIMEOUT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  _ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    hcd -> ux_hcd_entry_function = (UINT (*) (struct UX_HCD_STRUCT *, UINT, VOID *))tmp;
    global_storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request.ux_transfer_request_semaphore.tx_semaphore_id--;

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT); (second one) fails **/
    /**************************************************/

    /* Swap the hcd entry function. */
    hcd = UX_DEVICE_HCD_GET(global_storage -> ux_host_class_storage_device);
    tmp = (ALIGN_TYPE)hcd -> ux_hcd_entry_function;
    hcd -> ux_hcd_entry_function = ux_hcd_sim_host_entry_filter_transfer_request_failed_test3;

    /* Make the CBW something easily detectable from filter function. */
    ux_utility_memory_copy(global_storage -> ux_host_class_storage_cbw, "IS_CBW", 6);

    /* Make sure we use the bulk out endpoint. */
    global_storage -> ux_host_class_storage_cbw[UX_HOST_CLASS_STORAGE_CBW_FLAGS] = UX_HOST_CLASS_STORAGE_DATA_OUT;

    /* Make the data something easily detectable from filter function. */
    ux_utility_memory_copy(data_pointer, "IS_DATA", 7);

    _ux_utility_long_put(global_storage -> ux_host_class_storage_cbw + UX_HOST_CLASS_STORAGE_CBW_DATA_LENGTH, 10);

    /* Make sure storage_thread_entry() doesn't send a request at the same time. */
    status =  _ux_utility_semaphore_get(&global_storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status = _ux_host_class_storage_transport_bo(global_storage, data_pointer);
    if(status != UX_TRANSFER_TIMEOUT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  _ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    hcd -> ux_hcd_entry_function = (UINT (*) (struct UX_HCD_STRUCT *, UINT, VOID *))tmp;
    global_storage -> ux_host_class_storage_bulk_out_endpoint -> ux_endpoint_transfer_request.ux_transfer_request_semaphore.tx_semaphore_id--;

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, UX_HOST_CLASS_STORAGE_TRANSFER_TIMEOUT); (third one) fails **/
    /**************************************************/

    /* Swap the hcd entry function. */
    hcd = UX_DEVICE_HCD_GET(global_storage -> ux_host_class_storage_device);
    tmp = (ALIGN_TYPE)hcd -> ux_hcd_entry_function;
    hcd -> ux_hcd_entry_function = ux_hcd_sim_host_entry_filter_transfer_request_failed_test4;

    /* Make the CBW something easily detectable from filter function. */
    ux_utility_memory_copy(global_storage -> ux_host_class_storage_cbw, "IS_CBW", 6);

    /* Make the data something easily detectable from filter function. */
    ux_utility_memory_copy(data_pointer, "IS_DATA", 7);

    /* Make sure we use the bulk out endpoint. */
    global_storage -> ux_host_class_storage_cbw[UX_HOST_CLASS_STORAGE_CBW_FLAGS] = UX_HOST_CLASS_STORAGE_DATA_OUT;

    /* Make the CSW something easily detectable from filter function. */
    ux_utility_memory_copy(global_storage -> ux_host_class_storage_csw, "IS_CSW", 6);

    _ux_utility_long_put(global_storage -> ux_host_class_storage_cbw + UX_HOST_CLASS_STORAGE_CBW_DATA_LENGTH, 10);

    /* Make sure storage_thread_entry() doesn't send a request at the same time. */
    status =  _ux_utility_semaphore_get(&global_storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status = _ux_host_class_storage_transport_bo(global_storage, data_pointer);
    if(status != UX_ERROR)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  _ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    hcd -> ux_hcd_entry_function = (UINT (*) (struct UX_HCD_STRUCT *, UINT, VOID *))tmp;
    global_storage -> ux_host_class_storage_bulk_in_endpoint -> ux_endpoint_transfer_request.ux_transfer_request_semaphore.tx_semaphore_id--;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_media_capacity_get() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: capacity_response =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_READ_FORMAT_RESPONSE_LENGTH); fails **/
    /**************************************************/

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_media_capacity_get(global_storage);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /** Test case: capacity_response =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_READ_CAPACITY_RESPONSE_LENGTH); fails **/
    /** How: "But Nick, that's impossible! It's simply too dangerous! You'll die!" Modify the memory blocks _after_ the first memory allocate via transfer request. **/
    /**************************************************/

    ux_utility_memory_copy(global_storage -> ux_host_class_storage_csw, "IS_CSW", 6);

    /* Swap the hcd entry function. */
    hcd = UX_DEVICE_HCD_GET(global_storage -> ux_host_class_storage_device);
    tmp = (ALIGN_TYPE)hcd -> ux_hcd_entry_function;
    hcd -> ux_hcd_entry_function = ux_hcd_sim_host_entry_filter_transfer_request_failed_test1;

    /* Make sure storage_thread_entry() doesn't send a request at the same time. */
    status =  _ux_utility_semaphore_get(&global_storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status = _ux_host_class_storage_media_capacity_get(global_storage);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    status =  _ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if(status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;
    hcd -> ux_hcd_entry_function = (UINT (*) (struct UX_HCD_STRUCT *, UINT, VOID *))tmp;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_request_sense() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: request_sense_response =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_REQUEST_SENSE_RESPONSE_LENGTH); fails **/
    /**************************************************/

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_request_sense(global_storage);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_media_open() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: storage_media -> ux_host_class_storage_media_memory =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE); fails **/
    /**************************************************/

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_media_open(global_storage, 0);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_media_mount() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: sector_memory =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, storage -> ux_host_class_storage_sector_size); fails **/
    /**************************************************/

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_media_mount(global_storage, 0);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_media_format_capacity_get() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: read_format_capacity_response =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_READ_FORMAT_RESPONSE_LENGTH); fails **/
    /**************************************************/

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_media_format_capacity_get(global_storage);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_media_characteristics_get() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: inquiry_response =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_INQUIRY_RESPONSE_LENGTH); fails **/
    /**************************************************/

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_media_characteristics_get(global_storage);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_max_lun_get() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: storage_data_buffer =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, 1); fails **/
    /**************************************************/

    /* ux_host_class_storage_max_lun is set to 0 at start of max_lun_get() in case of error. */
    tmp = global_storage -> ux_host_class_storage_max_lun;

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_max_lun_get(global_storage);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    global_storage -> ux_host_class_storage_max_lun = tmp;
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_entry() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack =
                    _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE); fails **/
    /**************************************************/

    host_class.ux_host_class_thread_stack = UX_NULL;
    command.ux_host_class_command_class_ptr = &host_class;
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE;
    command.ux_host_class_command_class_ptr -> ux_host_class_thread_stack = UX_NULL;

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_entry(&command);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /** Test case: command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack =
                    _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE); fails **/
    /**************************************************/

    host_class.ux_host_class_thread_stack = UX_NULL;
    command.ux_host_class_command_class_ptr = &host_class;
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE;
    command.ux_host_class_command_class_ptr -> ux_host_class_thread_stack = UX_NULL;

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = calculate_final_memory_request_size(1, UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE);
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_entry(&command);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */

    /* Delete thread that was created during call. */
    status = ux_utility_thread_delete(&command.ux_host_class_command_class_ptr -> ux_host_class_thread);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

    /**************************************************/
    /** Test case: status =  _ux_utility_thread_create(&command -> ux_host_class_command_class_ptr -> ux_host_class_thread,
                                    "ux_storage_thread", _ux_host_class_storage_thread_entry,
                                    (ULONG) command -> ux_host_class_command_class_ptr,
                                    command -> ux_host_class_command_class_ptr -> ux_host_class_thread_stack,
                                    UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE,
                                    UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS,
                                    UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS,
                                    TX_NO_TIME_SLICE, TX_AUTO_START); fails **/
    /**************************************************/

    host_class.ux_host_class_thread_stack = UX_NULL;
    command.ux_host_class_command_class_ptr = &host_class;
    command.ux_host_class_command_request = UX_HOST_CLASS_COMMAND_ACTIVATE;
    command.ux_host_class_command_class_ptr -> ux_host_class_thread_stack = thread_stack;

    /* Create thread that storage_entry() will try to create. */
    status =  _ux_utility_thread_create(&command.ux_host_class_command_class_ptr -> ux_host_class_thread,
                                        "ux_storage_thread", _ux_host_class_storage_thread_entry,
                                        (ULONG) (ALIGN_TYPE) command.ux_host_class_command_class_ptr,
                                        command.ux_host_class_command_class_ptr -> ux_host_class_thread_stack,
                                        UX_HOST_CLASS_STORAGE_THREAD_STACK_SIZE,
                                        UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS,
                                        UX_HOST_CLASS_STORAGE_THREAD_PRIORITY_CLASS,
                                        TX_NO_TIME_SLICE, TX_DONT_START);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }
    UX_THREAD_EXTENSION_PTR_SET(&command.ux_host_class_command_class_ptr -> ux_host_class_thread, command.ux_host_class_command_class_ptr)
    tx_thread_resume(&command.ux_host_class_command_class_ptr -> ux_host_class_thread);

    command.ux_host_class_command_class_ptr -> ux_host_class_thread_stack = UX_NULL;

    status = _ux_host_class_storage_entry(&command);
    if (status != UX_THREAD_ERROR)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    status = ux_utility_thread_delete(&command.ux_host_class_command_class_ptr -> ux_host_class_thread);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_activate() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: storage =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, sizeof(UX_HOST_CLASS_STORAGE)); fails **/
    /**************************************************/

    command.ux_host_class_command_container = &interface;

    /* Set up the dummy memory block. */
    dummy_memory_block_first -> ux_memory_block_next = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_previous = UX_NULL;
    dummy_memory_block_first -> ux_memory_block_status = UX_MEMORY_UNUSED;
    dummy_memory_block_first -> ux_memory_block_size = 0;
    _ux_system -> ux_system_regular_memory_pool_start = dummy_memory_block_first;
    _ux_system -> ux_system_cache_safe_memory_pool_start = dummy_memory_block_first;

    status = _ux_host_class_storage_activate(&command);
    if (status != UX_MEMORY_INSUFFICIENT)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state for next test. */
    _ux_system -> ux_system_regular_memory_pool_start = original_regular_memory_block;
    _ux_system -> ux_system_cache_safe_memory_pool_start = original_cache_safe_memory_block;

#if 0
    /**************************************************/
    /** Test case: _ux_host_class_hid_configure() fails. **/
    /** Why direct: When called normally, the device is already configured (via ux_host_stack_class_interface_scan, which always happens before hid_activate()), so _ux_host_class_hid_configure() returns immediately. **/
    /**************************************************/

    /* Make sure we pass _ux_host_stack_class_instance_create(). */
    host_class.ux_host_class_first_instance = UX_NULL;

    /* Make sure _ux_host_class_hid_configure() fails. */
    device.ux_device_state = ~UX_DEVICE_CONFIGURED;
    device.ux_device_handle = 0;

    configuration.ux_configuration_device = &device;
    interface.ux_interface_configuration = &configuration;
    command.ux_host_class_command_container = &interface;
    command.ux_host_class_command_class_ptr = &host_class;

    status = _ux_host_class_storage_activate(&command);
    if (status != UX_CONFIGURATION_HANDLE_UNKNOWN)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }
#endif

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_create(&storage -> ux_host_class_storage_semaphore, "ux_host_class_storage_semaphore", 1); fails **/
    /**************************************************/

    /* Allocate a keyboard instance like storage_activate(). */
    storage_instance =  (UX_HOST_CLASS_STORAGE *) _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HOST_CLASS_STORAGE));
    if (storage_instance == UX_NULL)
    {

        printf("Error on line %d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the semaphore like storage_activate(). */
    status =  _ux_utility_semaphore_create(&storage_instance -> ux_host_class_storage_semaphore, "ux_host_class_storage_semaphore", 0);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Make a copy of the semaphore. */
    storage_instance_semaphore_copy = storage_instance -> ux_host_class_storage_semaphore;

    /* Free the memory so storage_activate() uses the same memory as us, which will cause threadx to detect a semaphore duplicate. */
    ux_utility_memory_free(storage_instance);

    /* Make sure we pass _ux_host_stack_class_instance_create(). */
    host_class.ux_host_class_first_instance = UX_NULL;

    device.ux_device_state = UX_DEVICE_CONFIGURED;
    configuration.ux_configuration_device = &device;
    interface.ux_interface_configuration = &configuration;
    command.ux_host_class_command_container = &interface;

    status = _ux_host_class_storage_activate(&command);
    if (status != UX_SEMAPHORE_ERROR)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* ux_utility_memory_allocate() zero'd out our keyboard instance semaphore! Good thing we made a copy! */
    storage_instance -> ux_host_class_storage_semaphore = storage_instance_semaphore_copy;

    /* Restore state for next test. */
    status = ux_utility_semaphore_delete(&storage_instance -> ux_host_class_storage_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_driver_entry() **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER); fails **/
    /**************************************************/

    /* Make sure it fails. */
    global_storage -> ux_host_class_storage_semaphore.tx_semaphore_id++;

    command.ux_host_class_command_instance = global_storage;

    /* Returns VOID. */
    _ux_host_class_storage_driver_entry(global_media);

    /* Restore state. */
    global_storage -> ux_host_class_storage_semaphore.tx_semaphore_id--;

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_configure() **/
    /** Why direct: When called normally, the device is already configured (via ux_host_stack_class_interface_scan, which always happens before hid_activate()), so _ux_host_class_hid_configure() returns immediately. **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /**************************************************/
    /** Testing _ux_host_class_storage_deactivate() **/
    /** NOTE: This should be last. **/
    /**************************************************/
    /**************************************************/

    /**************************************************/
    /** Test case: status =  _ux_utility_semaphore_get(&storage -> ux_host_class_storage_semaphore, UX_WAIT_FOREVER); fails **/
    /**************************************************/

    /* Make sure it fails. */
    global_storage -> ux_host_class_storage_semaphore.tx_semaphore_id++;

    command.ux_host_class_command_instance = global_storage;

    status = _ux_host_class_storage_deactivate(&command);
    if(status != TX_SEMAPHORE_ERROR)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Restore state. */
    global_storage -> ux_host_class_storage_semaphore.tx_semaphore_id--;

    /* The function did a get() on the hid semaphore and never put() it since we error'd out, so we must do it! */
    status = ux_utility_semaphore_put(&global_storage -> ux_host_class_storage_semaphore);
    if (status != UX_SUCCESS)
    {

        printf("Error on line %d, error code: %d\n", __LINE__, status);
        test_control_return(1);
    }

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}


static UINT    default_device_media_read(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT status;


    if(lba == 0)
    {
        global_ram_disk.fx_media_driver_logical_sector =  0;
        global_ram_disk.fx_media_driver_sectors =  1;
        global_ram_disk.fx_media_driver_request =  FX_DRIVER_BOOT_READ;
        global_ram_disk.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&global_ram_disk);
        *(data_pointer) =  0xeb;
        *(data_pointer+1) =  0x3c;
        *(data_pointer+2) =  0x90;
        *(data_pointer+21) =  0xF8;

        *(data_pointer+24) =  0x01;
        *(data_pointer+26) =  0x10;
        *(data_pointer+28) =  0x01;

        *(data_pointer+510) =  0x55;
        *(data_pointer+511) =  0xaa;
        ux_utility_memory_copy(data_pointer+0x36,"FAT12",5);


        status = global_ram_disk.fx_media_driver_status;
    }
    else
    {
        while(number_blocks--)
        {
            status =  fx_media_read(&global_ram_disk,lba,data_pointer);
            data_pointer+=512;
            lba++;
        }
    }
    return(status);
}

static UINT    default_device_media_write(VOID *storage, ULONG lun, UCHAR * data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{

UINT status;

    if(lba == 0)
    {
        global_ram_disk.fx_media_driver_logical_sector =  0;
        global_ram_disk.fx_media_driver_sectors =  1;
        global_ram_disk.fx_media_driver_request =  FX_DRIVER_BOOT_WRITE;
        global_ram_disk.fx_media_driver_buffer =  data_pointer;
        _fx_ram_driver(&global_ram_disk);

        status = global_ram_disk.fx_media_driver_status;

    }
    else
    {

        while(number_blocks--)
        {
            status =  fx_media_write(&global_ram_disk,lba,data_pointer);
            data_pointer+=512;
            lba++;
        }
        return(status);
    }
    return(status);
}

UINT    default_device_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{

    /* The ATA drive never fails. This is just for demo only !!!! */
    return(UX_SUCCESS);
}
