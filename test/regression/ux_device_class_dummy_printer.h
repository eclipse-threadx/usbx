#ifndef UX_DEVICE_CLASS_DUMMY_PRINTER_H
#define UX_DEVICE_CLASS_DUMMY_PRINTER_H

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
#include "ux_host_class_printer.h"

typedef struct UX_DEVICE_CLASS_PRINTER_PARAMS_STRUCT
{
    UCHAR   *device_id;
    ULONG   device_id_length;

    VOID    (*instance_activate)(VOID *);
    VOID    (*instance_deactivate)(VOID *);
} UX_DEVICE_CLASS_PRINTER_PARAMS;

typedef struct UX_DEVICE_CLASS_PRINTER_STRUCT
{
    UX_SLAVE_INTERFACE              *interface;
    UX_SLAVE_ENDPOINT               *bulk_in_endpoint;
    UX_SLAVE_ENDPOINT               *bulk_out_endpoint;
    UX_DEVICE_CLASS_PRINTER_PARAMS  params;
    UCHAR                           port_status;
    UCHAR                           soft_reset;
    UCHAR                           reserved[2];

} UX_DEVICE_CLASS_PRINTER;

extern UCHAR _ux_device_class_printer_name[];

UINT _ux_device_class_printer_entry(UX_SLAVE_CLASS_COMMAND *);

#endif