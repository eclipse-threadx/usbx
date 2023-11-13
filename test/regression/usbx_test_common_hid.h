#include <stdio.h>
#include <stdarg.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_host_class_hid.h"
#include "ux_host_class_hid_keyboard.h"
#include "ux_host_class_hid_mouse.h"
#include "ux_host_class_hid_remote_control.h"
#include "ux_device_class_hid.h"
#include "ux_device_stack.h"

#include "ux_test.h"
#include "usbx_test_common.h"

#define     ARRAY_COUNT(array)  (sizeof(array)/sizeof(array[0]))
#define     LSB(x)              (x & 0xff)
#define     MSB(x)              ((x & 0xff00) >> 8)

/* Define constants.  */
#define                             UX_DEMO_STACK_SIZE  	1024
#define                             UX_DEMO_MEMORY_SIZE     (96*1024)

/* Define local/extern function prototypes.  */
static void                         demo_thread_entry(ULONG);
static UINT                         demo_thread_hid_callback(UX_SLAVE_CLASS_HID *, UX_SLAVE_CLASS_HID_EVENT *);
static UINT                         demo_thread_hid_set_callback(UX_SLAVE_CLASS_HID *, UX_SLAVE_CLASS_HID_EVENT *);
static UINT                         demo_thread_hid_get_callback(UX_SLAVE_CLASS_HID *, UX_SLAVE_CLASS_HID_EVENT *);
static void                         tx_demo_thread_host_simulation_entry(ULONG);
static void                         tx_demo_thread_slave_simulation_entry(ULONG);
static void                         tx_demo_thread_device_simulation_entry(ULONG);

static void                         demo_device_hid_instance_activate(VOID *);
static void                         demo_device_hid_instance_deactivate(VOID *);


/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];
static TX_THREAD                           tx_demo_thread_host_simulation;
static TX_THREAD                           tx_demo_thread_slave_simulation;
static TX_THREAD                           tx_demo_thread_device_simulation;
static UX_HOST_CLASS_HID                   *hid;
static UX_HOST_CLASS_HID_REPORT_CALLBACK   hid_report_callback;
static UX_HOST_CLASS_HID_CLIENT            *hid_client;
static UX_HOST_CLASS_HID_KEYBOARD          *hid_keyboard;
static UX_HOST_CLASS_HID_MOUSE             *hid_mouse;
static UX_HOST_CLASS_HID_REMOTE_CONTROL    *hid_remote_control;
static UX_HOST_CLASS_HID_REPORT_GET_ID     hid_report_id;
static UX_HOST_CLASS_HID_CLIENT_REPORT     client_report;
static UX_SLAVE_CLASS_HID_PARAMETER        hid_parameter;
static UX_SLAVE_CLASS_HID                  *device_hid;
static UX_SLAVE_CLASS_HID_EVENT            device_hid_event;


/* Prototype for test control return.  */
void  test_control_return(UINT status);


static UINT demo_class_hid_get(void)
{

UINT                 status;
UX_HOST_CLASS       *class;


    /* Wait for enum thread to complete. */
    ux_test_wait_for_enum_thread_completion();

    /* Find the main HID container */
    status = ux_host_stack_class_get(_ux_system_host_class_hid_name, &class);
    if (status != UX_SUCCESS)
        return(status);

    /* We get the first instance of the hid device */
    status = ux_host_stack_class_instance_get(class, 0, (void **)&hid);
    if (status != UX_SUCCESS)
        return(status);

    if(hid -> ux_host_class_hid_state != UX_HOST_CLASS_INSTANCE_LIVE)
        return(UX_ERROR);

    /* Get client.  */
    hid_client = hid -> ux_host_class_hid_client;
    return(UX_SUCCESS);
}

static UINT demo_class_hid_keyboard_get(void)
{
UINT                 status;

    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
        return(status);

    /* Check client entry.  */
    if (hid_client -> ux_host_class_hid_client_handler != ux_host_class_hid_keyboard_entry)
        return(UX_ERROR);

    /* Get instance.  */
    hid_keyboard = (UX_HOST_CLASS_HID_KEYBOARD *)hid_client -> ux_host_class_hid_client_local_instance;
    if (hid_keyboard == UX_NULL)
        return(UX_ERROR);

    /* Check instance state.  */
    if (hid_keyboard -> ux_host_class_hid_keyboard_state != UX_HOST_CLASS_INSTANCE_LIVE)
        return(UX_ERROR);

    /* Instance is good.  */
    return(UX_SUCCESS);
}

static UINT demo_class_hid_mouse_get(void)
{
UINT                 status;

    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
        return(status);

    /* Check client entry.  */
    if (hid_client -> ux_host_class_hid_client_handler != ux_host_class_hid_mouse_entry)
        return(UX_ERROR);

    /* Get instance.  */
    hid_mouse = (UX_HOST_CLASS_HID_MOUSE *)hid_client -> ux_host_class_hid_client_local_instance;
    if (hid_mouse == UX_NULL)
        return(UX_ERROR);

    /* Check instance state.  */
    if (hid_mouse -> ux_host_class_hid_mouse_state != UX_HOST_CLASS_INSTANCE_LIVE)
        return(UX_ERROR);

    /* Instance is good.  */
    return(UX_SUCCESS);
}

static UINT demo_class_hid_remote_control_get(void)
{
UINT                 status;

    status = demo_class_hid_get();
    if (status != UX_SUCCESS)
        return(status);

    /* Check client entry.  */
    if (hid_client -> ux_host_class_hid_client_handler != ux_host_class_hid_remote_control_entry)
        return(UX_ERROR);

    /* Get instance.  */
    hid_remote_control = (UX_HOST_CLASS_HID_REMOTE_CONTROL *)hid_client -> ux_host_class_hid_client_local_instance;
    if (hid_remote_control == UX_NULL)
        return(UX_ERROR);

    /* Check instance state.  */
    if (hid_remote_control -> ux_host_class_hid_remote_control_state != UX_HOST_CLASS_INSTANCE_LIVE)
        return(UX_ERROR);

    /* Instance is good.  */
    return(UX_SUCCESS);
}
