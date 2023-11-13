/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "fx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_device_class_pima.h"
#include "ux_host_class_pima.h"

#include "ux_pictbridge.h"

#include "ux_hcd_sim_host.h"
#include "ux_device_stack.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"

#include "ux_test_jpeg_image.h"

/* Define constants.  */
#define UX_TEST_STACK_SIZE          (2*1024)
#define UX_TEST_MEMORY_SIZE         (512*1024)

/* Define local/extern function prototypes.  */

static void        test_thread_entry(ULONG);

static TX_THREAD   test_thread_host_simulation;
static TX_THREAD   test_thread_device_simulation;
static void        test_thread_host_simulation_entry(ULONG);
static void        test_thread_device_simulation_entry(ULONG);

static VOID        test_pima_instance_activate(VOID  *pima_instance);
static VOID        test_pima_instance_deactivate(VOID *pima_instance);


/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_TEST_MEMORY_SIZE + (UX_TEST_STACK_SIZE * 2)];

static UX_SLAVE_CLASS_PIMA_DEVICE          *pima_device;
static UX_PICTBRIDGE                       pictbridge_device;
static UX_PICTBRIDGE_PRINTINFO             printinfo;
static UX_PICTBRIDGE_JOBINFO               *jobinfo;
static UX_SLAVE_CLASS_PIMA_OBJECT          *object;
static TX_SEMAPHORE                        print_semaphore;
static ULONG                               pictbridge_device_copy_count = 0;

static UX_HOST_CLASS_PIMA                  *pima_host;
static UX_PICTBRIDGE                       pictbridge_host;
static ULONG                               pictbridge_host_copy_count = 0;
static TX_SEMAPHORE                        wait_semaphore;

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

static ULONG                               rsc_cdc_sem_usage;
static ULONG                               rsc_cdc_sem_get_count;
static ULONG                               rsc_cdc_mutex_usage;
static ULONG                               rsc_cdc_mem_usage;

static ULONG                               interaction_count;

static UCHAR                               error_callback_ignore = UX_TRUE;
static ULONG                               error_callback_counter;

/* Define device framework.  */

UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0xE8, 0x04, 0xC5, 0x68, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x27, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x03, 0x06, 0x01, 0x01,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Interrupt In) */
        0x07, 0x05, 0x83, 0x03, 0x40, 0x00, 0x04
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0xE8, 0x04, 0xC5, 0x68, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x27, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x03, 0x06, 0x01, 0x01,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x00, 0x02, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x00, 0x02, 0x00,

    /* Endpoint descriptor (Interrupt In) */
        0x07, 0x05, 0x83, 0x03, 0x40, 0x00, 0x04
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)

/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US    or 0x0000 for none.
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string

    The last string entry can be the optional Microsoft String descriptor.
*/
UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0a,
        0x4d, 0x54, 0x50, 0x20, 0x70, 0x6c, 0x61, 0x79,
        0x65, 0x72,

    /* Serial Number string descriptor : Index 3 */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31,

};
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)

/* Multiple languages are supported on the device, to add
    a language besides english, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)

/* Pima device info manufacture string (Unicode).  */
UCHAR string_pima_manufacturer[] =
{
    0x0C,
    0x45, 0x00, 0x78, 0x00, 0x70, 0x00, 0x72, 0x00,
    0x65, 0x00, 0x73, 0x00, 0x20, 0x00, 0x4c, 0x00,
    0x6f, 0x00, 0x67, 0x00, 0x69, 0x00, 0x63, 0x00
};

/* Pima device info Model string (Unicode).  */
UCHAR string_pima_model[] =
{
    0x0C,
    0x50, 0x00, 0x69, 0x00, 0x6D, 0x00, 0x61, 0x00,
    0x20, 0x00, 0x43, 0x00, 0x61, 0x00, 0x6D, 0x00,
    0x65, 0x00, 0x72, 0x00, 0x61, 0x00, 0x20, 0x00
};

/* Pima device info Device version (Unicode).  */
UCHAR string_pima_device_version[] =
{
    0x04,
    0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x31, 0x00
};

/* Pima device info Device serial number (Unicode).  */
UCHAR string_pima_serial_number[] =
{
    0x04,
    0x30, 0x00, 0x31, 0x00, 0x32, 0x00, 0x33, 0x00
};

UCHAR string_pima_storage_description[] =
{
    0x0b,
    0x56, 0x00, 0x69, 0x00, 0x72, 0x00, 0x74, 0x00,
    0x75, 0x00, 0x61, 0x00, 0x6c, 0x00, 0x20, 0x00,
    0x44, 0x00, 0x69, 0x00, 0x73, 0x00, 0x6b, 0x00
};

UCHAR string_pima_storage_volume_label[] =
{
    0x09,
    0x4d, 0x00, 0x79, 0x00, 0x20, 0x00, 0x56, 0x00,
    0x6f, 0x00, 0x6c, 0x00, 0x75, 0x00, 0x6d, 0x00,
    0x65, 0x00
};

/* Prototype for test control return.  */

void  test_control_return(UINT status);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
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



static UINT test_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{
    // printf("hChg:%lx, %p, %p\n", event, cls, inst);
    switch(event)
    {
        case UX_DEVICE_INSERTION:
            if (cls->ux_host_class_entry_function == ux_host_class_pima_entry)
            {
                pima_host = (UX_HOST_CLASS_PIMA *)inst;
            }
            break;

        case UX_DEVICE_REMOVAL:
            if (cls->ux_host_class_entry_function == ux_host_class_pima_entry)
            {
                if ((VOID*)pima_host == inst)
                    pima_host = UX_NULL;
            }
            break;

        default:
            break;
    }
    return 0;
}

static VOID    test_pima_instance_activate(VOID *instance)
{
    pima_device = (UX_SLAVE_CLASS_PIMA_DEVICE *)instance;
}
static VOID    test_pima_instance_deactivate(VOID *instance)
{
    if ((VOID *)pima_device == instance)
        pima_device = UX_NULL;
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_pictbridge_basic_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;
ULONG                   test_n;

    /* Inform user.  */
    printf("Running Pictbridge Basic Functionality Test......................... ");

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
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL,0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_system_host_change_function);
    UX_TEST_CHECK_SUCCESS(status);

    /* Register PIMA class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_pima_name, ux_host_class_pima_entry);
    UX_TEST_CHECK_SUCCESS(status);

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    UX_TEST_CHECK_SUCCESS(status);

    /* Initialize the Pictbridge string components. */
    ux_utility_memory_copy(pictbridge_device.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_vendor_name, "ExpressLogic",13);
    ux_utility_memory_copy(pictbridge_device.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_product_name, "EL_Pictbridge_Camera",21);
    ux_utility_memory_copy(pictbridge_device.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_serial_no, "ABC_123",7);
    ux_utility_memory_copy(pictbridge_device.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dpsversions, "1.0 1.1",7);
    pictbridge_device.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_vendor_specific_version = 0x0100;

    pictbridge_device.ux_pictbridge_pima_parameter.ux_device_class_pima_instance_activate = test_pima_instance_activate;
    pictbridge_device.ux_pictbridge_pima_parameter.ux_device_class_pima_instance_deactivate = test_pima_instance_deactivate;

    pictbridge_device.ux_pictbridge_pima_parameter.ux_device_class_pima_parameter_device_version = "0.0";

    /* Start the Pictbridge client.  */
    status = ux_pictbridge_dpsclient_start(&pictbridge_device);
    UX_TEST_CHECK_SUCCESS(status);

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Create a semaphore for the demo.  */
    status = tx_semaphore_create(&wait_semaphore,"Wait Semaphore", 0);
    UX_TEST_CHECK_SUCCESS(status);

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&test_thread_host_simulation, "tx demo host simulation", test_thread_host_simulation_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_CHECK_SUCCESS(status);

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&test_thread_device_simulation, "tx demo device simulation", test_thread_device_simulation_entry, 0,
            stack_pointer + UX_TEST_STACK_SIZE, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    UX_TEST_CHECK_SUCCESS(status);
}

/* Copy the object data.  */
static UINT test_pictbridge_host_object_data_write(UX_PICTBRIDGE *pictbridge,UCHAR *object_buffer, ULONG offset, ULONG total_length, ULONG length)
{
    pictbridge_host_copy_count ++;

    /* We have copied the requested data. Return OK.  */
    return(UX_SUCCESS);

}

void  test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
INT                                                 i;

    for (i = 0; i < 100; i ++)
    {
        if (pima_host && pima_device)
            break;
        ux_utility_delay_ms(1);
    }
    if (pima_host == UX_NULL || pima_device == UX_NULL)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the dpshost structure with the printer vendor info.  */
    ux_utility_memory_copy(pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_vendor_name, "ExpressLogic",13);
    ux_utility_memory_copy(pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_product_name, "EL_Pictbridge_Printer",21);
    ux_utility_memory_copy(pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_serial_no, "ABC_123",7);

    /* Set supported versions.  */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dpsversions[0] = 0x00010000;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dpsversions[1] = 0x00010001;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dpsversions[2] = 0;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_vendor_specific_version = 0x00010000;

    /* Set print services to TRUE. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_print_service_available = 0x30010000;

    /* Set Qualities. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_qualities[0] = UX_PICTBRIDGE_QUALITIES_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_qualities[1] = UX_PICTBRIDGE_QUALITIES_NORMAL;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_qualities[2] = UX_PICTBRIDGE_QUALITIES_DRAFT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_qualities[3] = UX_PICTBRIDGE_QUALITIES_FINE;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_qualities[4] = 0;

    /* Set Paper Sizes. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papersizes[0] = UX_PICTBRIDGE_PAPER_SIZES_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papersizes[1] = UX_PICTBRIDGE_PAPER_SIZES_4IX6I;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papersizes[2] = UX_PICTBRIDGE_PAPER_SIZES_L;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papersizes[3] = UX_PICTBRIDGE_PAPER_SIZES_2L;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papersizes[4] = UX_PICTBRIDGE_PAPER_SIZES_LETTER;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papersizes[5] = 0;

    /* Set Paper Types. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papertypes[0] = UX_PICTBRIDGE_PAPER_TYPES_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papertypes[1] = UX_PICTBRIDGE_PAPER_TYPES_PLAIN;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papertypes[2] = UX_PICTBRIDGE_PAPER_TYPES_PHOTO;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_papertypes[3] = 0;

    /* Set File Types. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filetypes[0] = UX_PICTBRIDGE_FILE_TYPES_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filetypes[1] = UX_PICTBRIDGE_FILE_TYPES_EXIF_JPEG;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filetypes[2] = UX_PICTBRIDGE_FILE_TYPES_JFIF;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filetypes[3] = UX_PICTBRIDGE_FILE_TYPES_DPOF;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filetypes[4] = 0;

    /* Set Date Prints. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dateprints[0] = UX_PICTBRIDGE_DATE_PRINTS_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dateprints[1] = UX_PICTBRIDGE_DATE_PRINTS_OFF;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dateprints[2] = UX_PICTBRIDGE_DATE_PRINTS_ON;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dateprints[3] = 0;

    /* Set File Name Prints. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filenameprints[0] = UX_PICTBRIDGE_FILE_NAME_PRINTS_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filenameprints[1] = UX_PICTBRIDGE_FILE_NAME_PRINTS_OFF;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filenameprints[2] = UX_PICTBRIDGE_FILE_NAME_PRINTS_ON;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_filenameprints[3] = 0;

    /* Set Image optimizes. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_imageoptimizes[0] = UX_PICTBRIDGE_IMAGE_OPTIMIZES_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_imageoptimizes[1] = UX_PICTBRIDGE_IMAGE_OPTIMIZES_OFF;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_imageoptimizes[2] = UX_PICTBRIDGE_IMAGE_OPTIMIZES_ON;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_imageoptimizes[3] = 0;

    /* Set Layouts. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_layouts[0] = UX_PICTBRIDGE_LAYOUTS_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_layouts[1] = UX_PICTBRIDGE_LAYOUTS_1_UP_BORDER;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_layouts[2] = UX_PICTBRIDGE_LAYOUTS_INDEX_PRINT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_layouts[3] = UX_PICTBRIDGE_LAYOUTS_1_UP_BORDERLESS;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_layouts[4] = 0;

    /* Set Fixed Sizes. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_fixedsizes[0] = UX_PICTBRIDGE_FIXED_SIZE_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_fixedsizes[1] = UX_PICTBRIDGE_FIXED_SIZE_35IX5I;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_fixedsizes[2] = UX_PICTBRIDGE_FIXED_SIZE_4IX6I;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_fixedsizes[3] = UX_PICTBRIDGE_FIXED_SIZE_5IX7I;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_fixedsizes[4] = UX_PICTBRIDGE_FIXED_SIZE_7CMX10CM;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_fixedsizes[5] = UX_PICTBRIDGE_FIXED_SIZE_LETTER;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_fixedsizes[6] = UX_PICTBRIDGE_FIXED_SIZE_A4;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_fixedsizes[7] = 0;

    /* Set croppings. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_croppings[0] = UX_PICTBRIDGE_CROPPINGS_DEFAULT;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_croppings[1] = UX_PICTBRIDGE_CROPPINGS_OFF;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_croppings[2] = UX_PICTBRIDGE_CROPPINGS_ON;
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_croppings[3] = 0;

    /* Set Print Service Status to idle. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_dpsprintservicestatus = UX_PICTBRIDGE_DPS_PRINTSERVICE_STATUS_IDLE;

    /* Set Job End Reason. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_jobendreason = UX_PICTBRIDGE_JOB_END_REASON_NOT_ENDED;

    /* Set Error Status. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_errorstatus = UX_PICTBRIDGE_ERROR_STATUS_NO_ERROR;

    /* Set Error Reason. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_errorreason = UX_PICTBRIDGE_ERROR_REASON_NO_REASON;

    /* Set Disconnection Enable. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_disconnectenable = UX_PICTBRIDGE_DISCONNECT_ENABLE_TRUE;

    /* Set Capability Changed. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_capabilitychanged = UX_PICTBRIDGE_CAPABILITY_CHANGED_FALSE;

    /* Set New Job OK. */
    pictbridge_host.ux_pictbridge_dpslocal.ux_pictbridge_devinfo_newjobok = UX_PICTBRIDGE_NEW_JOB_TRUE;

    /* Set a callback when an object is being received.  */
    pictbridge_host.ux_pictbridge_application_object_data_write = test_pictbridge_host_object_data_write;

    /* Activate the pictbridge dpshost.  */
    status = _ux_pictbridge_dpshost_start(&pictbridge_host, pima_host);
    UX_TEST_CHECK_SUCCESS(status);

    status = tx_semaphore_get(&wait_semaphore, 30000);
    UX_TEST_CHECK_SUCCESS(status);
    tx_thread_sleep(5);

    stepinfo(">>>>>>>>>>>> All Done\n");

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_pima_name, ux_device_class_pima_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}

static UINT test_pictbridge_device_object_data_copy(UX_PICTBRIDGE *pictbridge, ULONG object_handle, UCHAR *object_buffer, ULONG object_offset, ULONG object_length, ULONG *actual_length)
{
    pictbridge_device_copy_count ++;

    /* Copy the demanded object data portion.  */
    ux_utility_memory_copy(object_buffer, ux_test_jpeg_image + object_offset, object_length);

    /* Update the actual length.  */
    *actual_length = object_length;

    /* We have copied the requested data. Return OK.  */
    return(UX_SUCCESS);

}


UINT  test_pictbridge_device_event_callback(struct UX_PICTBRIDGE_STRUCT *pictbridge, UINT event_flag)
{

    /* Check if we received NotifyDeviceStatus event.  */
    if (event_flag & UX_PICTBRIDGE_EVENT_FLAG_NOTIFY_DEVICE_STATUS)
    {

        /* Check if the printer can accept new job.  */
        if (pictbridge -> ux_pictbridge_dpsclient.ux_pictbridge_devinfo_newjobok == UX_PICTBRIDGE_NEW_JOB_TRUE)
        {

            /* Let the demo thread to send a new job.  */
            tx_semaphore_put(&print_semaphore);
        }
    }

    return UX_SUCCESS;
}


void  test_thread_device_simulation_entry(ULONG arg)
{

UINT                                                status;
ULONG                                               actual_flags;

    /* Create a semaphore for the demo.  */
    status = tx_semaphore_create(&print_semaphore,"Print Semaphore", 0);
    UX_TEST_CHECK_SUCCESS(status);

    while(1)
    {

        /* We should wait for the host and the client to discover one another.  */
        status =  ux_utility_event_flags_get(&pictbridge_device.ux_pictbridge_event_flags_group, UX_PICTBRIDGE_EVENT_FLAG_DISCOVERY,
                                        TX_AND_CLEAR, &actual_flags, UX_PICTBRIDGE_EVENT_TIMEOUT);

        /* Check status.  */
        if (status == UX_SUCCESS)
        {

            /* Check if the pictbridge state machine has changed to discovery complete.  */
            if (pictbridge_device.ux_pictbridge_discovery_state ==  UX_PICTBRIDGE_DPSCLIENT_DISCOVERY_COMPLETE)
            {

                /* We can now communicate using XML scripts with the printer. First get information on capabilities.  */
                status = ux_pictbridge_dpsclient_api_configure_print_service(&pictbridge_device);
                UX_TEST_CHECK_SUCCESS(status);

                /* Check status.  */
                if (status == UX_SUCCESS)
                {

                    /* Get the printer capabilities : Qualities.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_QUALITIES, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Get the printer capabilities : PaperSizes.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_PAPER_SIZES, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Get the printer capabilities : FileTypes.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_FILE_TYPES, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Get the printer capabilities : DatePrints.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_DATE_PRINTS, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Get the printer capabilities : FileNamePrints.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_FILE_NAME_PRINTS, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Get the printer capabilities : ImageOptimizes.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_IMAGE_OPTIMIZES, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Get the printer capabilities : Layouts.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_LAYOUTS, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Get the printer capabilities : FixedSizes.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_FIXED_SIZES, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Get the printer capabilities : Croppings.  */
                    status = ux_pictbridge_dpsclient_api_capability(&pictbridge_device, UX_PICTBRIDGE_API_CROPPINGS, 0);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* We have all the printer capabilities, get the device status.   */
                    status = ux_pictbridge_dpsclient_api_device_status(&pictbridge_device);
                    UX_TEST_CHECK_SUCCESS(status);

                    /* Check status.  */
                    if (status == UX_SUCCESS)
                    {

                        /* Check if the printer is ready for a pring job.  */
                        if (pictbridge_device.ux_pictbridge_dpsclient.ux_pictbridge_devinfo_newjobok == UX_PICTBRIDGE_NEW_JOB_TRUE)
                        {

                            /* We can start a new job. Fill in the JobConfig and PrintInfo structures. */
                            jobinfo = &pictbridge_device.ux_pictbridge_jobinfo;

                            /* Attach a printinfo structure to the job.  */
                            jobinfo -> ux_pictbridge_jobinfo_printinfo_start = &printinfo;


                            /* Set the default values for print job.  */
                            jobinfo -> ux_pictbridge_jobinfo_quality       =  UX_PICTBRIDGE_QUALITIES_DEFAULT;
                            jobinfo -> ux_pictbridge_jobinfo_papersize     =  UX_PICTBRIDGE_PAPER_SIZES_DEFAULT;
                            jobinfo -> ux_pictbridge_jobinfo_papertype     =  UX_PICTBRIDGE_PAPER_TYPES_DEFAULT;
                            jobinfo -> ux_pictbridge_jobinfo_filetype      =  UX_PICTBRIDGE_FILE_TYPES_DEFAULT;
                            jobinfo -> ux_pictbridge_jobinfo_dateprint     =  UX_PICTBRIDGE_DATE_PRINTS_DEFAULT;
                            jobinfo -> ux_pictbridge_jobinfo_filenameprint =  UX_PICTBRIDGE_FILE_NAME_PRINTS_DEFAULT;
                            jobinfo -> ux_pictbridge_jobinfo_imageoptimize =  UX_PICTBRIDGE_IMAGE_OPTIMIZES_OFF;
                            jobinfo -> ux_pictbridge_jobinfo_layout        =  UX_PICTBRIDGE_LAYOUTS_DEFAULT;
                            jobinfo -> ux_pictbridge_jobinfo_fixedsize     =  UX_PICTBRIDGE_FIXED_SIZE_DEFAULT;
                            jobinfo -> ux_pictbridge_jobinfo_cropping      =  UX_PICTBRIDGE_CROPPINGS_DEFAULT;

                            /* Program the callback function for reading the object data.  */
                            jobinfo -> ux_pictbridge_jobinfo_object_data_read        =  test_pictbridge_device_object_data_copy;

                            /* This is a demo, the fileID is hardwired (1 and 2 for scripts, 3 for photo to be printed.  */
                            printinfo.ux_pictbridge_printinfo_fileid    = UX_PICTBRIDGE_OBJECT_HANDLE_PRINT;
                            ux_utility_memory_copy(printinfo.ux_pictbridge_printinfo_filename, "Pictbridge demo file", 20);
                            ux_utility_memory_copy(printinfo.ux_pictbridge_printinfo_date, "01/01/2008", 10);

                            /* Fill in the object info to be printed.  First get the pointer to the object container in the job info structure. */
                            object = (UX_SLAVE_CLASS_PIMA_OBJECT *) jobinfo -> ux_pictbridge_jobinfo_object;

                            /* Store the object format : JPEG picture.  */
                            object -> ux_device_class_pima_object_format            =  UX_DEVICE_CLASS_PIMA_OFC_EXIF_JPEG;
                            object -> ux_device_class_pima_object_compressed_size   =  UX_TEST_JPEG_IMAGE_LENGTH;
                            object -> ux_device_class_pima_object_offset            =  0;
                            object -> ux_device_class_pima_object_handle_id         =  UX_PICTBRIDGE_OBJECT_HANDLE_PRINT;
                            object -> ux_device_class_pima_object_length            =  UX_TEST_JPEG_IMAGE_LENGTH;

                            /* File name is in Unicode.  */
                            ux_utility_string_to_unicode("JPEG Image", object -> ux_device_class_pima_object_filename);

                            /* And start the job.  */
                            status = ux_pictbridge_dpsclient_api_start_job(&pictbridge_device);
                            UX_TEST_CHECK_SUCCESS(status);

                            /* Register the callback function to receive events from the printer.  */
                            ux_pictbridge_dpsclient_register_event_callback_function(&pictbridge_device, test_pictbridge_device_event_callback);

                            /* Wait for the job to complete.  */
                            status = tx_semaphore_get(&print_semaphore, 30000);
                            UX_TEST_CHECK_SUCCESS(status);

                            if (status == TX_SUCCESS)
                            {

                                /* Print the job again to demo the use of callback function.  */
                                status = ux_pictbridge_dpsclient_api_start_job(&pictbridge_device);
                                UX_TEST_CHECK_SUCCESS(status);

                                /* Let host thread run to end.  */
                                tx_semaphore_put(&wait_semaphore);
                            }

                            /* Unregister the callback function by passing a Null pointer.  */
                            ux_pictbridge_dpsclient_register_event_callback_function(&pictbridge_device, UX_NULL);
                        }
                    }
                }
            }
        }
    }
}
