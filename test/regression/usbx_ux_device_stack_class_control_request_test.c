#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_device_stack.h"
#include "ux_test_utility_sim.h"


static UX_SLAVE_TRANSFER transfer_request;
static UX_SYSTEM_SLAVE system_slave;
static UX_SLAVE_CLASS  slave_classes[UX_MAX_SLAVE_INTERFACES];
static UX_SLAVE_INTERFACE slave_interfaces[UX_MAX_SLAVE_INTERFACES];

static UINT test_entry_count = 0;
static UINT test_entry_class[UX_MAX_SLAVE_INTERFACES] = {0, 0, 0, 0};
static inline void _test_entry_log_reset(void)
{
    test_entry_class[0] = 0;
    test_entry_class[1] = 0;
    test_entry_class[2] = 0;
    test_entry_class[3] = 0;
    test_entry_count = 0;
}
#define TEST_ENTRY_LOG_SAVE(c) \
    if (test_entry_count < UX_MAX_SLAVE_INTERFACES) { test_entry_class[test_entry_count++] = c; }
#define TEST_ENTRY_LOG_CHECK_FAIL(c1,c2,c3,c4)  \
    (test_entry_class[0] != c1) || (test_entry_class[1] != c2) || \
    (test_entry_class[2] != c3) || (test_entry_class[3] != c4)

typedef struct _REQ_ACCEPTED {
    UCHAR type;
    UCHAR request;
} REQ_ACCEPTED;
typedef struct _CLASS_REQ_ACCEPTED {
    UCHAR bClass;
    UCHAR n_req;
    REQ_ACCEPTED *req;
} CLASS_REQ_ACCEPTED;
static REQ_ACCEPTED cdc_acm_req[] = {
    {0x21, 0x00}, /* SEND_ENCAPSULATED_COMMAND  */
    {0xA1, 0x01}, /* GET_ENCAPSULATED_RESPONSE  */
    {0x21, 0x02}, /* SET_COMM_FEATURE  */
    {0xA1, 0x03}, /* GET_COMM_FEATURE  */
    {0x21, 0x04}, /* CLEAR_COMM_FEATURE  */
    {0x21, 0x05}, /* SET_LINE_CODING  */
    {0xA1, 0x06}, /* GET_LINE_CODING  */
    {0x21, 0x07}, /* SET_CONTROL_LINE_STATE  */
    {0x21, 0x08}, /* SEND_BREAK  */
};
static REQ_ACCEPTED printer_req[] = {
    {0xA1, 0x00}, /* GET_DEVICE_ID  */
    {0xA1, 0x01}, /* GET_PORT_STATUS  */
    {0x21, 0x02}, /* SOFT_RESET  */
};
static REQ_ACCEPTED storage_req[] = {
    {0x21, 0xFF}, /* Bulk-Only Reset  */
    {0xA1, 0xFE}, /* Get Max LUN  */
};
static REQ_ACCEPTED hid_req[] = {
    {0xA1, 0x01}, /* GET_REPORT  */
    {0xA1, 0x02}, /* GET_IDLE  */
    {0xA1, 0x03}, /* GET_PROTOCOL  */
    {0x21, 0x09}, /* SET_REPORT  */
    {0x21, 0x0A}, /* SET_IDLE  */
    {0x21, 0x0B}, /* SET_PROTOCOL  */
};
static CLASS_REQ_ACCEPTED class_req_accepted[] = {
    {0x02, sizeof(cdc_acm_req)/sizeof(REQ_ACCEPTED), cdc_acm_req},
    {0x07, sizeof(printer_req)/sizeof(REQ_ACCEPTED), printer_req},
    {0x08, sizeof(storage_req)/sizeof(REQ_ACCEPTED), storage_req},
    {0x03, sizeof(hid_req)/sizeof(REQ_ACCEPTED), hid_req},
};

UINT _test_class_entry(struct UX_SLAVE_CLASS_COMMAND_STRUCT *cmd)
{
UX_SLAVE_DEVICE     *device =  &_ux_system_slave -> ux_system_slave_device;
UX_SLAVE_CLASS      *cmd_class = cmd -> ux_slave_class_command_class_ptr;
UX_SLAVE_TRANSFER   *transfer =  &transfer_request;
INT                 i, j;

    stepinfo(" entry call: %p (%x)\n", cmd_class, (unsigned)cmd_class -> ux_slave_class_interface->ux_slave_interface_descriptor.bInterfaceClass);
    TEST_ENTRY_LOG_SAVE(cmd_class -> ux_slave_class_interface->ux_slave_interface_descriptor.bInterfaceClass);
    for (i = 0; i < sizeof(class_req_accepted)/sizeof(CLASS_REQ_ACCEPTED); i++)
    {
        if (cmd_class->ux_slave_class_interface->ux_slave_interface_descriptor.bInterfaceClass == class_req_accepted[i].bClass)
        {
            for (j = 0; j < class_req_accepted[i].n_req; j++)
            {
                if (transfer->ux_slave_transfer_request_setup[UX_SETUP_REQUEST] == class_req_accepted[i].req[j].request &&
                    transfer->ux_slave_transfer_request_setup[UX_SETUP_REQUEST_TYPE] == class_req_accepted[i].req[j].type)
                {
                    return(UX_SUCCESS);
                }
            }
        }
    }
    return (UX_NO_CLASS_MATCH);
}

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_device_stack_class_control_request_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status = 0;
UCHAR                   request_type = UX_REQUEST_TYPE_CLASS | UX_REQUEST_TARGET_INTERFACE;
UCHAR                   request = 0;
INT                     i;


    /* Inform user.  */
    printf("Running USB Device Stack Class Control Request Process Test ........ ");
    if (UX_MAX_SLAVE_INTERFACES < 4)
    {
        printf("Supported number of interface: %d (<4)\n", UX_MAX_SLAVE_INTERFACES);
        printf("Skipped\n");
        test_control_return(0);
        return;
    }

    _ux_system_slave = &system_slave;
    for (i = 0; i < UX_MAX_SLAVE_INTERFACES; i++)
    {
        if (i > 3)
        {
            system_slave.ux_system_slave_interface_class_array[i] = UX_NULL;
            continue;
        }
        system_slave.ux_system_slave_interface_class_array[i] = &slave_classes[i];
        slave_classes[i].ux_slave_class_interface = &slave_interfaces[i];
        slave_classes[i].ux_slave_class_entry_function = _test_class_entry;
        slave_interfaces[i].ux_slave_interface_descriptor.bInterfaceClass = 0x00;
    }
    stepinfo("class array: %p %p %p %p ...\n",
           system_slave.ux_system_slave_interface_class_array[0],
           system_slave.ux_system_slave_interface_class_array[1],
           system_slave.ux_system_slave_interface_class_array[2],
           system_slave.ux_system_slave_interface_class_array[3]);

struct _test_struct {
    /* Interface */
    UCHAR           ifc_class[4]; /* 2:CDC, 7:PRINT, 8:MSC, 3:HID  */
    /* Input */
    UCHAR           setup[8];
    /* Output check */
    UINT            status;
    UCHAR           cmd_class[4]; /* 2:CDC, 7:PRINT, 8:MSC, 3:HID  */
} tests[] = {
    /* class                      type  req   value       index       length       return               class       */
    {  {0x07, 0x02, 0x03, 0x08}, {0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, UX_SUCCESS,          {0x07, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0xA1, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, UX_SUCCESS,          {0x07, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0xA1, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}, UX_ERROR,            {0x00, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0xA1, 0x00, 0x00, 0x00, 0x05, 0x01, 0x00, 0x00}, UX_ERROR,            {0x00, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, UX_NO_CLASS_MATCH,   {0x07, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, UX_SUCCESS,          {0x02, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0x21, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00}, UX_NO_CLASS_MATCH,   {0x03, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0x21, 0x0A, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00}, UX_SUCCESS,          {0x03, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0xA1, 0xFE, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00}, UX_SUCCESS,          {0x08, 0x00, 0x00, 0x00}},
    {  {0x07, 0x02, 0x03, 0x08}, {0xA1, 0xFE, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00}, UX_ERROR,            {0x00, 0x00, 0x00, 0x00}},

    /* class                      type  req   value       index       length       return               class       */
    {  {0x02, 0x07, 0x03, 0x08}, {0xA1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, UX_NO_CLASS_MATCH,   {0x02, 0x00, 0x00, 0x00}},
    {  {0x02, 0x07, 0x03, 0x08}, {0xA1, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}, UX_SUCCESS,          {0x02, 0x07, 0x00, 0x00}},
    {  {0x02, 0x07, 0x03, 0x08}, {0x21, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}, UX_SUCCESS,          {0x02, 0x00, 0x00, 0x00}},
    {  {0x02, 0x07, 0x03, 0x08}, {0xA1, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00}, UX_SUCCESS,          {0x07, 0x00, 0x00, 0x00}},
    {  {0x02, 0x07, 0x03, 0x08}, {0xA1, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00}, UX_NO_CLASS_MATCH,   {0x02, 0x00, 0x00, 0x00}},
    {  {0x02, 0x07, 0x03, 0x08}, {0xA1, 0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00}, UX_SUCCESS,          {0x03, 0x00, 0x00, 0x00}},
    {  {0x02, 0x07, 0x03, 0x08}, {0xA1, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00}, UX_ERROR,            {0x00, 0x00, 0x00, 0x00}},

    /* class                      type  req   value       index       length       return               class       */
    {  {0x03, 0x08, 0x07, 0x02}, {0xA1, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00}, UX_SUCCESS,          {0x03, 0x07, 0x00, 0x00}},
    {  {0x03, 0x08, 0x07, 0x02}, {0xA1, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00}, UX_NO_CLASS_MATCH,   {0x03, 0x00, 0x00, 0x00}},
    {  {0x03, 0x08, 0x07, 0x02}, {0xA1, 0x02, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00}, UX_ERROR,            {0x00, 0x00, 0x00, 0x00}},
};
    for (i = 0; i < sizeof(tests)/sizeof(struct _test_struct); i ++)
    {

        _test_entry_log_reset();

        slave_interfaces[0].ux_slave_interface_descriptor.bInterfaceClass = tests[i].ifc_class[0];
        slave_interfaces[1].ux_slave_interface_descriptor.bInterfaceClass = tests[i].ifc_class[1];
        slave_interfaces[2].ux_slave_interface_descriptor.bInterfaceClass = tests[i].ifc_class[2];
        slave_interfaces[3].ux_slave_interface_descriptor.bInterfaceClass = tests[i].ifc_class[3];

        transfer_request.ux_slave_transfer_request_completion_code = UX_SUCCESS;
        transfer_request.ux_slave_transfer_request_setup[0] =                   tests[i].setup[0];
        transfer_request.ux_slave_transfer_request_setup[UX_SETUP_REQUEST] =    tests[i].setup[UX_SETUP_REQUEST];
        transfer_request.ux_slave_transfer_request_setup[UX_SETUP_INDEX] =      tests[i].setup[UX_SETUP_INDEX];
        transfer_request.ux_slave_transfer_request_setup[UX_SETUP_INDEX + 1] =  tests[i].setup[UX_SETUP_INDEX + 1];

        stepinfo(" test #%2d: {%x %x %x %x} {%x %x .. %x %x ..} %x {%x %x %x %x}\n", i,
                tests[i].ifc_class[0], tests[i].ifc_class[1], tests[i].ifc_class[2], tests[i].ifc_class[3],
                tests[i].setup[0], tests[i].setup[UX_SETUP_REQUEST], tests[i].setup[UX_SETUP_INDEX], tests[i].setup[UX_SETUP_INDEX + 1],
                tests[i].status, tests[i].cmd_class[0], tests[i].cmd_class[1], tests[i].cmd_class[2], tests[i].cmd_class[3]);

        status = _ux_device_stack_control_request_process(&transfer_request);
        if (status != tests[i].status)
        {
            printf("ERROR #%d: %2d {%x %x %x %x} {%x %x .. %x %x ..}, status = %x, expected %x\n", __LINE__,
                i, tests[i].ifc_class[0], tests[i].ifc_class[1], tests[i].ifc_class[2], tests[i].ifc_class[3],
                tests[i].setup[0], tests[i].setup[UX_SETUP_REQUEST], tests[i].setup[UX_SETUP_INDEX], tests[i].setup[UX_SETUP_INDEX + 1],
                status, tests[i].status);
            test_control_return(1);
            return;
        }
        if (TEST_ENTRY_LOG_CHECK_FAIL(tests[i].cmd_class[0], tests[i].cmd_class[1], tests[i].cmd_class[2], tests[i].cmd_class[3]))
        {
            printf("ERROR #%d: %2d {%x %x %x %x} {%x %x .. %x %x ..}, class call {%x %x %x %x}, expected {%x %x %x %x}\n", __LINE__,
                i, tests[i].ifc_class[0], tests[i].ifc_class[1], tests[i].ifc_class[2], tests[i].ifc_class[3],
                tests[i].setup[0], tests[i].setup[UX_SETUP_REQUEST], tests[i].setup[UX_SETUP_INDEX], tests[i].setup[UX_SETUP_INDEX + 1],
                test_entry_class[0], test_entry_class[1], test_entry_class[2], test_entry_class[3],
                tests[i].cmd_class[0], tests[i].cmd_class[1], tests[i].cmd_class[2], tests[i].cmd_class[3]);
            test_control_return(1);
            return;
        }
    }
    
    printf("SUCCESS!\n");
    test_control_return(0);

    return;
}
