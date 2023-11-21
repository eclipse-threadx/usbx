
#ifndef UX_DEVICE_CLASS_DUMMY_H
#define UX_DEVICE_CLASS_DUMMY_H

/* Define Data Pump Class instance structure.  */

struct UX_DEVICE_CLASS_DUMMY_STRUCT;
typedef struct UX_DEVICE_CLASS_DUMMY_CALLBACKS_STRUCT
{
    VOID (*ux_device_class_dummy_initialize)(VOID *);
    VOID (*ux_device_class_dummy_uninitialize)(VOID *);

    VOID (*ux_device_class_dummy_instance_activate)(VOID *);
    VOID (*ux_device_class_dummy_instance_deactivate)(VOID *);

    VOID (*ux_device_class_dummy_change)(struct UX_DEVICE_CLASS_DUMMY_STRUCT *);
    VOID (*ux_device_class_dummy_control_request)(struct UX_DEVICE_CLASS_DUMMY_STRUCT *, UX_SLAVE_TRANSFER *);

    VOID *ux_device_class_dummy_arg;
} UX_DEVICE_CLASS_DUMMY_CALLBACKS;

typedef struct UX_DEVICE_CLASS_DUMMY_PARAMETER_STRUCT
{
    UX_DEVICE_CLASS_DUMMY_CALLBACKS ux_device_class_dummy_parameter_callbacks;
} UX_DEVICE_CLASS_DUMMY_PARAMETER;

typedef struct UX_DEVICE_CLASS_DUMMY_STRUCT
{

    UX_SLAVE_CLASS                  *ux_device_class_dummy_class;
    UX_SLAVE_DEVICE                 *ux_device_class_dummy_device;
    UX_SLAVE_INTERFACE              *ux_device_class_dummy_interface;

    VOID                            *ux_device_class_dummy_instance;

    UX_DEVICE_CLASS_DUMMY_CALLBACKS ux_device_class_dummy_callbacks;
} UX_DEVICE_CLASS_DUMMY;

extern UCHAR _ux_device_class_dummy_name[];

UINT  _ux_device_class_dummy_entry(UX_SLAVE_CLASS_COMMAND *command);

VOID *_ux_device_class_dummy_get_arg(UX_DEVICE_CLASS_DUMMY *dummy);

UX_SLAVE_ENDPOINT *_ux_device_class_dummy_get_endpoint(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address);
UX_SLAVE_TRANSFER *_ux_device_class_dummy_get_transfer_request(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address);

ULONG _ux_device_class_dummy_get_max_packet_size(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address);

UINT  _ux_device_class_dummy_transfer(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address, UCHAR *buffer, ULONG length, ULONG *actual_length);
UINT  _ux_device_class_dummy_abort(UX_DEVICE_CLASS_DUMMY *dummy, UCHAR endpoint_address);

#endif
