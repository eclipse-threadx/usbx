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
#include "ux_test.h"

/* Define constants.  */
#define                             UX_DEMO_DEBUG_SIZE  (4096*8)
#define                             UX_DEMO_STACK_SIZE  1024
#define                             UX_DEMO_BUFFER_SIZE 2048
#define                             UX_DEMO_XMIT_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BUFFER_SIZE 512
#define                             UX_DEMO_FILE_BUFFER_SIZE 512
#define                             UX_DEMO_RECEPTION_BLOCK_SIZE 64
#define                             UX_DEMO_MEMORY_SIZE     (64*1024)
#define                             UX_DEMO_FILE_SIZE       (128 * 1024)
#define                             UX_RAM_DISK_MEMORY      (256 * 1024)

/* Define local/extern function prototypes.  */
static void                                demo_thread_entry(ULONG);
static TX_THREAD                           tx_demo_thread_host_simulation;
static TX_THREAD                           tx_demo_thread_slave_simulation;
static void                                tx_demo_thread_host_simulation_entry(ULONG);
static void                                tx_demo_thread_slave_simulation_entry(ULONG);
static void                                demo_thread_host_reception_callback(UX_HOST_CLASS_CDC_ACM *cdc_acm, UINT status, UCHAR *reception_buffer, ULONG reception_size);
static VOID                                demo_cdc_instance_activate(VOID  *cdc_instance);             
static VOID                                demo_cdc_instance_deactivate(VOID *cdc_instance);
static UINT                                demo_usbx_simulator_cdc_acm_host_send_at_command(UCHAR *string, ULONG length);
static UINT                                tx_demo_thread_slave_simulation_response(UCHAR *string, ULONG length);


/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static UX_HOST_CLASS                       *class_driver;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_control;
static UX_HOST_CLASS_CDC_ACM               *cdc_acm_host_data;
static ULONG                               command_received_count;
static UCHAR                               cdc_acm_reception_buffer[UX_DEMO_RECEPTION_BUFFER_SIZE];
static UCHAR                               cdc_acm_xmit_buffer[UX_DEMO_XMIT_BUFFER_SIZE];
static UX_HOST_CLASS_CDC_ACM_RECEPTION     cdc_acm_reception;
static UCHAR                               *global_reception_buffer;
static ULONG                               global_reception_size;

static UX_SLAVE_CLASS_CDC_ACM              *cdc_acm_slave;
static UX_SLAVE_CLASS_CDC_ACM_PARAMETER    parameter;
static UCHAR                               buffer[UX_DEMO_BUFFER_SIZE];
static ULONG                               echo_mode;

static ULONG                               error_counter;


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

    /* Endpoint 1 descriptor 7 bytes */
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

    /* First alternate setting Endpoint 1 descriptor 7 bytes*/
    0x07, 0x05, 0x02,           
    0x02,                       
    0x40, 0x00,                 
    0x00,                       

    /* Endpoint 2 descriptor 7 bytes */
    0x07, 0x05, 0x81,           
    0x02,                       
    0x40, 0x00,                 
    0x00,                       

};

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
    0x09, 0x04, 0x01,           
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

    /* Endpoint 1 descriptor */
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

    /* First alternate setting Endpoint 1 descriptor */
    0x07, 0x05, 0x02,           
    0x02,                       
    0x40, 0x00,                 
    0x00,                       

    /* Endpoint 2 descriptor */
    0x07, 0x05, 0x81,           
    0x02,                       
    0x40, 0x00,                 
    0x00,                        

};

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




/* Define the ISR dispatch.  */

extern VOID    (*test_isr_dispatch)(void);


/* Prototype for test control return.  */

void  test_control_return(UINT status);


/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}
    

static UINT  demo_class_cdc_acm_get(void)
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

static VOID    demo_cdc_instance_activate(VOID *cdc_instance)   
{

    /* Save the CDC instance.  */
    cdc_acm_slave = (UX_SLAVE_CLASS_CDC_ACM *) cdc_instance;
}           
static VOID    demo_cdc_instance_deactivate(VOID *cdc_instance)
{

    /* Don't Reset the CDC instance. We need to use it after disconnection to
       check for the line state values. */
    //cdc_acm_slave = UX_NULL;
}

/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_cdc_acm_device_dtr_rts_reset_on_disconnect_test_application_define(void *first_unused_memory)
#endif
{
CHAR *              stack_pointer;
CHAR *              memory_pointer;
UINT                status;
    
    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);

    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("Running CDC ACM Basic Functionality Test............................ ERROR #1\n");
        test_control_return(1);
    }

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(UX_NULL);
    if (status != UX_SUCCESS)
    {

        printf("Running CDC ACM Basic Functionality Test............................ ERROR #2\n");
        test_control_return(1);
    }
    
    /* Register CDC-ACM class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_cdc_acm_name, ux_host_class_cdc_acm_entry);
    if (status != UX_SUCCESS)
    {

        printf("Running CDC ACM Basic Functionality Test............................ ERROR #3\n");
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, ux_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {

        printf("Running CDC ACM Basic Functionality Test............................ ERROR #4\n");
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if(status!=UX_SUCCESS)
    {

        printf("Running CDC ACM Basic Functionality Test............................ ERROR #5\n");
        test_control_return(1);
    }

    /* Set the parameters for callback when insertion/extraction of a CDC device.  */
    parameter.ux_slave_class_cdc_acm_instance_activate   =  demo_cdc_instance_activate;
    parameter.ux_slave_class_cdc_acm_instance_deactivate =  demo_cdc_instance_deactivate;

    /* Initialize the device cdc class. This class owns both interfaces starting with 0. */
     status =  ux_device_stack_class_register(_ux_system_slave_class_cdc_acm_name, ux_device_class_cdc_acm_entry, 
                                                1,0,  &parameter);

    if(status!=UX_SUCCESS)
    {

        printf("Running CDC ACM Basic Functionality Test............................ ERROR #6\n");
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running CDC ACM Basic Functionality Test............................ ERROR #7\n");
        test_control_return(1);
    }
    
    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&tx_demo_thread_host_simulation, "tx demo host simulation", tx_demo_thread_host_simulation_entry, 0,  
            stack_pointer, UX_DEMO_STACK_SIZE, 
            20, 20, 1, TX_AUTO_START);
      
    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("Running CDC ACM Basic Functionality Test............................ ERROR #9\n");
        test_control_return(1);
    }
}

static void  tx_demo_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
UX_SLAVE_CLASS_CDC_ACM_LINE_STATE_PARAMETER         line_state_slave;


    /* Inform user.  */
    printf("Running Device CDC-ACM DTR/RTS Reset on Disconnect Test............. ");

    /* Find the cdc_acm class and wait for the link to be up.  */
    status =  demo_class_cdc_acm_get();
    if (status != UX_SUCCESS)
    {

        /* DPUMP basic test error.  */
        printf("ERROR #10\n");
        test_control_return(1);
    }

    while (!cdc_acm_slave);

    /* Wait for RTS and DTR. */
    do
    {
        UX_TEST_CHECK_SUCCESS(ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE, &line_state_slave));
    }
    while (line_state_slave.ux_slave_class_cdc_acm_parameter_dtr == 0 ||
           line_state_slave.ux_slave_class_cdc_acm_parameter_rts == 0);

    /* Now disconnect. */
    ux_test_disconnect_slave();
    ux_test_disconnect_host_wait_for_enum_completion();

    /* Now make sure DTR and RTS are reset. */
    UX_TEST_CHECK_SUCCESS(ux_device_class_cdc_acm_ioctl(cdc_acm_slave, UX_SLAVE_CLASS_CDC_ACM_IOCTL_GET_LINE_STATE, &line_state_slave));
    UX_TEST_ASSERT(line_state_slave.ux_slave_class_cdc_acm_parameter_dtr == 0 &&
                   line_state_slave.ux_slave_class_cdc_acm_parameter_rts == 0);

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);
}