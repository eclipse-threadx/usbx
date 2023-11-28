
#ifndef UX_HOST_CLASS_DUMMY_H
#define UX_HOST_CLASS_DUMMY_H

/* Define Data Pump Class instance structure.  */

typedef struct UX_HOST_CLASS_DUMMY_STRUCT
{

    struct UX_HOST_CLASS_DUMMY_STRUCT  
                        *ux_host_class_dummy_next_instance;

    UX_HOST_CLASS       *ux_host_class_dummy_class;
    UX_DEVICE           *ux_host_class_dummy_device;
    UX_INTERFACE        *ux_host_class_dummy_interface;

    UINT                ux_host_class_dummy_state;
} UX_HOST_CLASS_DUMMY;

typedef struct UX_HOST_CLASS_DUMMY_QUERY_STRUCT
{
    ULONG           ux_host_class_query_on;                                 /* 0x0 to end list.  */
    UINT            (*ux_host_class_query_entry)(UX_HOST_CLASS_COMMAND *);  /* 0x0 for any.  */
    UINT            ux_host_class_query_usage;                              /* 0x0 for any.  */
    UINT            ux_host_class_query_pid;                                /* 0x0 for any.  */
    UINT            ux_host_class_query_vid;                                /* 0x0 for any.  */
    UINT            ux_host_class_query_class;                              /* 0x0 for any.  */
    UINT            ux_host_class_query_subclass;                           /* 0x0 for any.  */
    UINT            ux_host_class_query_protocol;                           /* 0x0 for any.  */
    UINT            ux_host_class_query_iad_class;                          /* 0x0 for any.  */
    UINT            ux_host_class_query_iad_subclass;                       /* 0x0 for any.  */
    UINT            ux_host_class_query_iad_protocol;                       /* 0x0 for any.  */
} UX_HOST_CLASS_DUMMY_QUERY;

extern UCHAR _ux_host_class_dummy_name[];

UINT _ux_host_class_dummy_entry(UX_HOST_CLASS_COMMAND *command);        /* For function/interface class.  */
UINT _ux_host_class_dummy_device_entry(UX_HOST_CLASS_COMMAND *command); /* For device class.  */

VOID _ux_host_class_dummy_query_reject_unknown_set(UCHAR yes_no);
VOID _ux_host_class_dummy_query_list_set(UX_HOST_CLASS_DUMMY_QUERY *query_list); /* {..., {UX_NULL}}  */

UINT _ux_host_class_dummy_select_interface(UX_HOST_CLASS_DUMMY *dummy,
                                UCHAR interface_number, UCHAR alternate_setting);

UX_ENDPOINT *_ux_host_class_dummy_get_endpoint(UX_HOST_CLASS_DUMMY *dummy,
                                UCHAR endpoint_address, UCHAR alternate_setting);
UX_TRANSFER *_ux_host_class_dummy_get_transfer_request(UX_HOST_CLASS_DUMMY *dummy,
                                UCHAR endpoint_address, UCHAR alternate_setting);

ULONG _ux_host_class_dummy_get_max_packet_size(UX_HOST_CLASS_DUMMY *dummy,
                                UCHAR endpoint_address, UCHAR alternate_setting);
ULONG _ux_host_class_dummy_get_max_payload_size(UX_HOST_CLASS_DUMMY *dummy,
                                UCHAR endpoint_address, UCHAR alternate_setting);

VOID  _ux_host_class_dummy_set_timeout(UX_HOST_CLASS_DUMMY *dummy,
                                UCHAR endpoint_address, UCHAR alternate_setting,
                                ULONG timeout);

UINT  _ux_host_class_dummy_transfer(UX_HOST_CLASS_DUMMY *dummy,
                                UCHAR endpoint_address, UCHAR alternate_setting,
                                UCHAR *buffer, ULONG length, ULONG *actual_length);
UINT  _ux_host_class_dummy_abort(UX_HOST_CLASS_DUMMY *dummy,
                                UCHAR endpoint_address, UCHAR alternate_setting);


#endif
