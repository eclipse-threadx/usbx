/* This test is designed to test the ux_utility_memory_....  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_cdc_acm.h"
#include "ux_host_class_cdc_acm.h"

#include "ux_host_class_dpump.h"
#include "ux_device_class_dpump.h"

#include "ux_host_class_hid.h"
#include "ux_device_class_hid.h"

#include "ux_host_class_storage.h"
#include "ux_device_class_storage.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"


/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_RUN             1
#define UX_TEST_MEMORY_SIZE     (64*1024)

#define     LSB(x) ( (x) & 0x00ff)
#define     MSB(x) (((x) & 0xff00) >> 8)

/* Configuration descriptor 9 bytes */
#define CFG_DESC(wTotalLength, bNumInterfaces, bConfigurationValue)\
    /* Configuration 1 descriptor 9 bytes */\
    0x09, 0x02, LSB(wTotalLength), MSB(wTotalLength),\
    (bNumInterfaces), (bConfigurationValue), 0x00,\
    0x40, 0x00,
#define CFG_DESC_LEN 9

/* DPUMP interface descriptors. */
#define DPUMP_IFC_DESC(ifc, alt, nb_ep) \
    /* Interface descriptor */\
    0x09, 0x04, (ifc), (alt), (nb_ep), 0x99, 0x99, 0x99, 0x00,

#define DPUMP_IFC_EP_DESC(epaddr, eptype, epsize) \
    /* Endpoint descriptor */\
    0x07, 0x05, (epaddr), (eptype), LSB(epsize), MSB(epsize), 0x01,

#define DPUMP_IFC_DESC_ALL_LEN(nb_ep) (9 + (nb_ep) * 7)

#define CFG_DESC_ALL_LEN (CFG_DESC_LEN + DPUMP_IFC_DESC_ALL_LEN(4))

#define CFG_DESC_ALL \
    CFG_DESC(CFG_DESC_ALL_LEN, 1, 1)\
    DPUMP_IFC_DESC(0, 0, 4)\
    DPUMP_IFC_EP_DESC(0x81, 2, 64)\
    DPUMP_IFC_EP_DESC(0x02, 2, 64)\
    DPUMP_IFC_EP_DESC(0x83, 1, 64)\
    DPUMP_IFC_EP_DESC(0x84, 3, 64)\

extern UCHAR ux_test_speed_up_mem_allocate_until_flagged;

/* Define the counters used in the test application...  */

static ULONG                           thread_0_counter;
static ULONG                           thread_1_counter;
static ULONG                           error_counter;

static UCHAR                           error_callback_ignore = UX_FALSE;
static ULONG                           error_callback_counter;

static UCHAR                           buffer[UX_TEST_BUFFER_SIZE];

/* Define USBX test global variables.  */

static UX_HOST_CLASS                   *class_driver;
static UX_HOST_CLASS_DPUMP             *dpump;
static UX_SLAVE_CLASS_DPUMP            *dpump_slave = UX_NULL;

static UCHAR device_framework_full_speed[] = {

    /* Device descriptor 18 bytes */
    0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
    0xec, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x01,

    CFG_DESC_ALL
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

static UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
    0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
    0x03, 0x01,

    /* Device qualifier descriptor */
    0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
    0x01, 0x00,

    CFG_DESC_ALL
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)

/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string
*/

static UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
    0x09, 0x04, 0x01, 0x0c,
    0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c,
    0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
    0x09, 0x04, 0x02, 0x0c,
    0x44, 0x61, 0x74, 0x61, 0x50, 0x75, 0x6d, 0x70,
    0x44, 0x65, 0x6d, 0x6f,

    /* Serial Number string descriptor : Index 3 */
    0x09, 0x04, 0x03, 0x04,
    0x30, 0x30, 0x30, 0x31
};
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)

    /* Multiple languages are supported on the device, to add
       a language besides English, the unicode language code must
       be appended to the language_id_framework array and the length
       adjusted accordingly. */
static UCHAR language_id_framework[] = {

/* English. */
    0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)

/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static VOID                ux_test_instance_activate(VOID  *dpump_instance);
static VOID                ux_test_instance_deactivate(VOID *dpump_instance);

UINT                       _ux_host_class_dpump_entry(UX_HOST_CLASS_COMMAND *command);
UINT                       ux_hcd_sim_initialize(UX_HCD *hcd);
UINT                       _ux_host_class_dpump_write(UX_HOST_CLASS_DPUMP *dpump, UCHAR * data_pointer,
                                    ULONG requested_length, ULONG *actual_length);
UINT                       _ux_host_class_dpump_read (UX_HOST_CLASS_DPUMP *dpump, UCHAR *data_pointer,
                                    ULONG requested_length, ULONG *actual_length);

static TX_THREAD           ux_test_thread_simulation_0;
static TX_THREAD           ux_test_thread_simulation_1;
static void                ux_test_thread_simulation_0_entry(ULONG);
static void                ux_test_thread_simulation_1_entry(ULONG);


/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);

/* Simulator actions. */

static UX_TEST_HCD_SIM_ACTION endpoint0x83_create_del_skip[] = {
/* function, request to match,
   port action, port status,
   request action, request EP, request data, request actual length, request status,
   status, additional callback,
   no_return */
{   UX_HCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        UX_TEST_MATCH_EP, 0x83, UX_NULL, 0, 0,
        UX_SUCCESS},
{   UX_HCD_CREATE_ENDPOINT, NULL,
        UX_FALSE, 0,
        UX_TEST_MATCH_EP, 0x83, UX_NULL, 0, 0,
        UX_SUCCESS},
{   0   }
};

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

static UINT break_on_dpump_ready(VOID)
{

UINT             status;
UX_HOST_CLASS   *class;

    /* Find the main data pump container.  */
    status =  ux_host_stack_class_get(_ux_system_host_class_dpump_name, &class);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return UX_SUCCESS;

    /* Find the instance. */
    status =  ux_host_stack_class_instance_get(class, 0, (VOID **) &dpump);
    if (status != UX_SUCCESS)
        /* Do not break. */
        return UX_SUCCESS;

    if (dpump -> ux_host_class_dpump_state != UX_HOST_CLASS_INSTANCE_LIVE)
        /* Do not break. */
        return UX_SUCCESS;

    return 1;
}

static UINT break_on_removal(VOID)
{

UINT                     status;
UX_DEVICE               *device;

    status = ux_host_stack_device_get(0, &device);
    if (status == UX_SUCCESS)
        /* Do not break. */
        return UX_SUCCESS;

    return 1;
}


static UINT test_ux_device_class_dpump_entry(UX_SLAVE_CLASS_COMMAND *command)
{
    switch(command->ux_slave_class_command_request)
    {
        case UX_SLAVE_CLASS_COMMAND_INITIALIZE:
        case UX_SLAVE_CLASS_COMMAND_QUERY:
        case UX_SLAVE_CLASS_COMMAND_CHANGE:
            return UX_SUCCESS;

        default:
            return UX_NO_CLASS_MATCH;
    }
}

static UINT test_ux_host_class_dpump_entry(UX_HOST_CLASS_COMMAND *command)
{
    switch (command -> ux_host_class_command_request)
    {
        case UX_HOST_CLASS_COMMAND_QUERY:
        default:
            return _ux_host_class_dpump_entry(command);
    }
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_utility_memory_test_application_define(void *first_unused_memory)
#endif
{

UINT                            status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;
CHAR                            *rpool_start;
CHAR                            *cpool_start;
ULONG                            rpool_size;
ULONG                            cpool_size;
ULONG                            rpool_free[2];
ULONG                            cpool_free[2];
VOID                            *ptr;
UINT                             n, i, j;
const CHAR                       flags[] = {
    UX_REGULAR_MEMORY, UX_CACHE_SAFE_MEMORY, 0xFF
};
const CHAR                       expect_error[] = {
    UX_FALSE, UX_FALSE, UX_TRUE
};
const ULONG                      aligns[] = {
    UX_NO_ALIGN, /* 0 */
    UX_ALIGN_MIN, /* 0xf */
    UX_SAFE_ALIGN, /* 0xffffffff */
    UX_ALIGN_32,
    UX_ALIGN_64,
    UX_ALIGN_128,
    UX_ALIGN_256,
    UX_ALIGN_512,
    UX_ALIGN_1024,
    UX_ALIGN_2048,
    UX_ALIGN_4096,
    UX_MAX_SCATTER_GATHER_ALIGNMENT,
};

    /* Inform user.  */
    printf("Running ux_utility_memory_... Test.................................. ");

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    for (n = 0; n < 3; n ++)
    {

        switch(n)
        {
        case 0:
            rpool_start = memory_pointer;
            rpool_size  = UX_TEST_MEMORY_SIZE;
            cpool_start = memory_pointer + UX_TEST_MEMORY_SIZE;
            cpool_size  = UX_TEST_MEMORY_SIZE;
            break;
        case 1:
            rpool_start = memory_pointer + UX_TEST_MEMORY_SIZE;
            rpool_size  = UX_TEST_MEMORY_SIZE;
            cpool_start = memory_pointer;
            cpool_size  = UX_TEST_MEMORY_SIZE;
            break;
        default:
            rpool_start = memory_pointer;
            rpool_size  = UX_TEST_MEMORY_SIZE * 2;
            cpool_start = UX_NULL;
            cpool_size  = 0;
        }

        /* Initialize USBX Memory.  */
        status =  ux_system_initialize(rpool_start, rpool_size, cpool_start, cpool_size);

        /* Check for error.  */
        if (status != UX_SUCCESS)
        {

            printf("ERROR #%d.%d\n", __LINE__, n);
            test_control_return(1);
        }

        /* Register the error callback. */
        _ux_utility_error_callback_register(error_callback);
        for (j = 0; j < sizeof(aligns)/sizeof(aligns[0]); j ++)
        {
            /* Save memory level. */
            rpool_free[0] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
            cpool_free[0] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;

            error_callback_ignore = UX_TRUE;

            /* Allocate all. */
            ux_test_utility_sim_mem_allocate_until_align_flagged(0, aligns[j], UX_REGULAR_MEMORY);
            ux_test_utility_sim_mem_allocate_until_align_flagged(0, aligns[j], UX_CACHE_SAFE_MEMORY);

            rpool_free[1] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
            cpool_free[1] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;

            /* Check. */
            if (rpool_free[0] <= rpool_free[1])
            {

                printf("ERROR #%d.%d.%d: Expect regular pool level down\n", __LINE__, n, j);
                error_counter ++;
            }

            if (cpool_free[0] <= cpool_free[1] && cpool_start)
            {

                printf("ERROR #%d.%d.%d: Expect cache safe pool level down\n", __LINE__, n, j);
                error_counter ++;
            }

            error_callback_ignore = UX_FALSE;

            /* Free All. */
            ux_test_utility_sim_mem_free_all_flagged(UX_REGULAR_MEMORY);
            ux_test_utility_sim_mem_free_all_flagged(UX_CACHE_SAFE_MEMORY);

            rpool_free[1] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
            cpool_free[1] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;

            /* Check. */
            if (rpool_free[0] != rpool_free[1])
            {

                printf("ERROR #%d.%d.%d: Regular pool level diff %lu -> %lu\n", __LINE__, n, j, rpool_free[0], rpool_free[1]);
                error_counter ++;
            }
            if (cpool_free[0] != cpool_free[1])
            {

                printf("ERROR #%d.%d.%d: Cache safe pool level diff %lu -> %lu\n", __LINE__, n, j, cpool_free[0], cpool_free[1]);
                error_counter ++;
            }

            for (i = 0; i < sizeof(flags); i ++)
            {

                /* Save pool level. */
                rpool_free[0] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
                cpool_free[0] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;

                /* Error callback setting. */
                error_callback_ignore = expect_error[i];

                /* Allocate. */
                ptr = ux_utility_memory_allocate(aligns[j], flags[i], 8);

                /* Error case. */
                if (expect_error[i])
                {

                    if (ptr != UX_NULL)
                    {

                        printf("ERROR #%d.%d.%d.%d: Expect fail\n", __LINE__, n, j, i);
                        error_counter ++;
                    }
                }
                else
                {

                    /* No error. */
                    if (ptr == UX_NULL)
                    {

                        printf("ERROR #%d.%d.%d.%d: memory allocate fail\n", __LINE__, n, j, i);
                        error_counter ++;
                    }

                    /* Save pool level. */
                    rpool_free[1] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
                    cpool_free[1] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;

                    if (!(rpool_free[1] < rpool_free[0] || cpool_free[1] < cpool_free[0]))
                    {

                        printf("ERROR #%d.%d.%d.%d: Expect pool level down\n", __LINE__, n, j, i);
                        error_counter ++;
                    }

                    ux_utility_memory_free(ptr);
                }

                /* Save pool level. */
                rpool_free[1] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
                cpool_free[1] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available;

                if (rpool_free[0] != rpool_free[1])
                {

                    printf("ERROR #%d.%d.%d.%d: Expect no regular pool change but %lu -> %lu\n", __LINE__, n, j, i, rpool_free[0], rpool_free[1]);
                    error_counter ++;
                }
                if (cpool_free[0] != cpool_free[1])
                {

                    printf("ERROR #%d.%d.%d.%d: Expect no cache safe pool change but %lu -> %lu\n", __LINE__, n, j, i, cpool_free[0], cpool_free[1]);
                    error_counter ++;
                }

            }
        }

        /* Uninitialize */
        ux_system_uninitialize();

    }

    /* Test the case where there isn't enough left over memory for a new memory block after needing to do an alignment. */
    {
        static UCHAR dummy_memory[1024];

        ALIGN_TYPE int_ptr = (ALIGN_TYPE) dummy_memory;

        int_ptr += 2*sizeof(UX_MEMORY_BLOCK);

        int_ptr += 31;
        int_ptr &= ~(31);

        int_ptr += 1;
        int_ptr -= sizeof(UX_MEMORY_BLOCK);

        UX_MEMORY_BLOCK *dummy_block = (UX_MEMORY_BLOCK *) (int_ptr - sizeof(UX_MEMORY_BLOCK));
        // dummy_block->ux_memory_block_next = UX_NULL;
        // dummy_block->ux_memory_block_previous = UX_NULL;
        // dummy_block->ux_memory_block_size = (16 + 8 + 31 + ((sizeof(UCHAR *)) + (sizeof(ALIGN_TYPE))) + 1;
        // dummy_block->ux_memory_block_status = UX_MEMORY_UNUSED;

        // _ux_system->ux_system_regular_memory_pool_start = dummy_block;
        // _ux_system->ux_system_regular_memory_pool_size = dummy_block->ux_memory_block_size + sizeof(UX_MEMORY_BLOCK);
        ULONG dummy_block_size = (16 + 8 + 31 + sizeof(UX_MEMORY_BLOCK)) + 1;
        _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start = (UCHAR*)dummy_block;
        _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available = dummy_block_size;

        ux_utility_memory_allocate(31, UX_REGULAR_MEMORY, 16);
    }

    /* Test allocate memory of size 0.  */
    rpool_free[0] = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    ptr = _ux_utility_memory_allocate_mulc_safe(UX_NO_ALIGN, UX_REGULAR_MEMORY, 1, 0);
    if (ptr)
    {
        _ux_utility_memory_free(ptr);
    }
    if (rpool_free[0] != _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available)
    {
        printf("ERROR %d : expect no pool level change but %ld -> %ld\n", __LINE__, rpool_free[0], _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
        error_counter ++;
    }

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