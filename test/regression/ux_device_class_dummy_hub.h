#ifndef UX_DEVICE_CLASS_DUMMY_HUB_H
#define UX_DEVICE_CLASS_DUMMY_HUB_H

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_device_stack.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_device_class_dummy.h"
#include "ux_host_class_hub.h"

typedef struct UX_DEVICE_CLASS_HUB_PARAMS_STRUCT
{
    UCHAR   *descriptor;
    ULONG   descriptor_length;

    VOID    (*instance_activate)(VOID *);
    VOID    (*instance_deactivate)(VOID *);
} UX_DEVICE_CLASS_HUB_PARAMS;

typedef struct UX_DEVICE_CLASS_HUB_STRUCT
{
    UX_SLAVE_CLASS              *hub_class;
    UX_SLAVE_ENDPOINT           *interrupt_endpoint;

    /* Test dummy not depends on standalone switch.  */
    TX_THREAD                   hub_thread;
    UCHAR                       *hub_thread_stack;

    ULONG                       status_change_bitmap;
    USHORT                      port_status;
    USHORT                      port_change;

    UCHAR                       dont_reset_port_when_commanded_to;

    UX_DEVICE_CLASS_HUB_PARAMS  params;
} UX_DEVICE_CLASS_HUB;

extern UCHAR _ux_device_class_hub_name[];

UINT _ux_device_class_hub_entry(UX_SLAVE_CLASS_COMMAND *);
VOID _ux_device_class_hub_notify_changes(UX_DEVICE_CLASS_HUB *hub, UCHAR *changes, UINT rpt_size);
VOID _ux_device_class_hub_notify_change(UX_DEVICE_CLASS_HUB *hub, UINT change_pos, UINT rpt_size);

#endif