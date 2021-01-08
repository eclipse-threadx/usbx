/**************************************************************************/
/*                                                                        */
/*       Copyright (c) Microsoft Corporation. All rights reserved.        */
/*                                                                        */
/*       This software is licensed under the Microsoft Software License   */
/*       Terms for Microsoft Azure RTOS. Full text of the license can be  */
/*       found in the LICENSE file at https://aka.ms/AzureRTOS_EULA       */
/*       and in the root directory of this software.                      */
/*                                                                        */
/**************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** USBX Component                                                        */ 
/**                                                                       */
/**   HID Class                                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/**************************************************************************/ 
/*                                                                        */ 
/*  COMPONENT DEFINITION                                   RELEASE        */ 
/*                                                                        */ 
/*    ux_device_class_hid.h                               PORTABLE C      */ 
/*                                                           6.1.3        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This file contains all the header and extern functions used by the  */
/*    USBX HID class.                                                     */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            used UX prefix to refer to  */
/*                                            TX symbols instead of using */
/*                                            them directly,              */
/*                                            resulting in version 6.1    */
/*  12-31-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            added Get/Set Protocol      */
/*                                            request support,            */
/*                                            resulting in version 6.1.3  */
/*                                                                        */
/**************************************************************************/

#ifndef UX_DEVICE_CLASS_HID_H
#define UX_DEVICE_CLASS_HID_H

/* Define HID Class constants.  */

#define UX_DEVICE_CLASS_HID_CLASS                                   0x03
#define UX_DEVICE_CLASS_HID_SUBCLASS                                0X00
#define UX_DEVICE_CLASS_HID_PROTOCOL                                0X00

/* Define HID Class commands.  */

#define UX_DEVICE_CLASS_HID_COMMAND_GET_REPORT                      0x01
#define UX_DEVICE_CLASS_HID_COMMAND_GET_IDLE                        0x02
#define UX_DEVICE_CLASS_HID_COMMAND_GET_PROTOCOL                    0x03
#define UX_DEVICE_CLASS_HID_COMMAND_SET_REPORT                      0x09
#define UX_DEVICE_CLASS_HID_COMMAND_SET_IDLE                        0x0A
#define UX_DEVICE_CLASS_HID_COMMAND_SET_PROTOCOL                    0x0B

/* Define HID Class Descriptor types.  */

#define UX_DEVICE_CLASS_HID_DESCRIPTOR_HID                          0x21
#define UX_DEVICE_CLASS_HID_DESCRIPTOR_REPORT                       0x22
#define UX_DEVICE_CLASS_HID_DESCRIPTOR_PHYSICAL                     0x23

/* Define HID Report Types.  */

#define UX_DEVICE_CLASS_HID_REPORT_TYPE_INPUT                       0x1
#define UX_DEVICE_CLASS_HID_REPORT_TYPE_OUTPUT                      0x2
#define UX_DEVICE_CLASS_HID_REPORT_TYPE_FEATURE                     0x3

/* Define HID Protocols.  */

#define UX_DEVICE_CLASS_HID_PROTOCOL_BOOT                           0
#define UX_DEVICE_CLASS_HID_PROTOCOL_REPORT                         1

/* Define HID event info structure.  */

#ifndef UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH
#define UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH                     32
#endif

/* Ensure the event buffer can fit inside the control endpoint's data buffer.  */
#if UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH > UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH
#error "Error: the event buffer cannot fit inside the control endpoint's data buffer. Reduce UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH such that it is less than or equal to UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH."
#endif

/* Ensure the event buffer can fit inside the interrupt endpoint's data buffer.  */
#if UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH > UX_SLAVE_REQUEST_DATA_MAX_LENGTH
#error "Error: the event buffer cannot fit inside the interrupt endpoint's data buffer. Reduce UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH such that it is less than or equal to UX_SLAVE_REQUEST_DATA_MAX_LENGTH."
#endif

#ifndef UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE
#define UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE                        16
#endif

#define UX_DEVICE_CLASS_HID_NEW_EVENT                               1
#define UX_DEVICE_CLASS_HID_NEW_IDLE_RATE                           2
#define UX_DEVICE_CLASS_HID_EVENTS_MASK                             3

typedef struct UX_SLAVE_CLASS_HID_EVENT_STRUCT
{
    ULONG                   ux_device_class_hid_event_report_id;
    ULONG                   ux_device_class_hid_event_report_type;
    UCHAR                   ux_device_class_hid_event_buffer[UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH];
    ULONG                   ux_device_class_hid_event_length;
    
} UX_SLAVE_CLASS_HID_EVENT;

/* Define HID structure.  */

typedef struct UX_SLAVE_CLASS_HID_STRUCT
{

    UX_SLAVE_INTERFACE              *ux_slave_class_hid_interface;
    UX_SLAVE_ENDPOINT               *ux_device_class_hid_interrupt_endpoint;
    UINT                            ux_device_class_hid_state;
    UINT                            (*ux_device_class_hid_callback)(struct UX_SLAVE_CLASS_HID_STRUCT *hid, UX_SLAVE_CLASS_HID_EVENT *);
    UINT                            (*ux_device_class_hid_get_callback)(struct UX_SLAVE_CLASS_HID_STRUCT *hid, UX_SLAVE_CLASS_HID_EVENT *);
    VOID                            (*ux_slave_class_hid_instance_activate)(VOID *);
    VOID                            (*ux_slave_class_hid_instance_deactivate)(VOID *);
    UCHAR                           *ux_device_class_hid_report_address;
    ULONG                           ux_device_class_hid_report_id;
    ULONG                           ux_device_class_hid_report_length;
    UX_EVENT_FLAGS_GROUP            ux_device_class_hid_event_flags_group;
    ULONG                           ux_device_class_hid_event_idle_rate;
    ULONG                           ux_device_class_hid_event_wait_timeout;
    ULONG                           ux_device_class_hid_protocol;
    UX_SLAVE_CLASS_HID_EVENT        *ux_device_class_hid_event_array;
    UX_SLAVE_CLASS_HID_EVENT        *ux_device_class_hid_event_array_head;
    UX_SLAVE_CLASS_HID_EVENT        *ux_device_class_hid_event_array_tail;
    UX_SLAVE_CLASS_HID_EVENT        *ux_device_class_hid_event_array_end;
                                                                
} UX_SLAVE_CLASS_HID;

/* Define HID initialization command structure.  */

typedef struct UX_SLAVE_CLASS_HID_PARAMETER_STRUCT
{

    VOID                    (*ux_slave_class_hid_instance_activate)(VOID *);
    VOID                    (*ux_slave_class_hid_instance_deactivate)(VOID *);
    UCHAR                   *ux_device_class_hid_parameter_report_address;
    ULONG                   ux_device_class_hid_parameter_report_id;
    ULONG                   ux_device_class_hid_parameter_report_length;
    UINT                    (*ux_device_class_hid_parameter_callback)(struct UX_SLAVE_CLASS_HID_STRUCT *hid, UX_SLAVE_CLASS_HID_EVENT *);
    UINT                    (*ux_device_class_hid_parameter_get_callback)(struct UX_SLAVE_CLASS_HID_STRUCT *hid, UX_SLAVE_CLASS_HID_EVENT *);

} UX_SLAVE_CLASS_HID_PARAMETER;


/* Define HID Class function prototypes.  */
UINT  _ux_device_class_hid_descriptor_send(UX_SLAVE_CLASS_HID *hid, ULONG descriptor_type, 
                                            ULONG request_index, ULONG host_length);
UINT  _ux_device_class_hid_activate(UX_SLAVE_CLASS_COMMAND *command);
UINT  _ux_device_class_hid_deactivate(UX_SLAVE_CLASS_COMMAND *command);
UINT  _ux_device_class_hid_control_request(UX_SLAVE_CLASS_COMMAND *command);
UINT  _ux_device_class_hid_entry(UX_SLAVE_CLASS_COMMAND *command);
VOID  _ux_device_class_hid_interrupt_thread(ULONG hid_class);
UINT  _ux_device_class_hid_initialize(UX_SLAVE_CLASS_COMMAND *command);
UINT  _ux_device_class_hid_uninitialize(UX_SLAVE_CLASS_COMMAND *command);
UINT  _ux_device_class_hid_event_set(UX_SLAVE_CLASS_HID *hid, 
                                      UX_SLAVE_CLASS_HID_EVENT *hid_event);
UINT  _ux_device_class_hid_event_get(UX_SLAVE_CLASS_HID *hid, 
                                      UX_SLAVE_CLASS_HID_EVENT *hid_event);
UINT  _ux_device_class_hid_report_set(UX_SLAVE_CLASS_HID *hid, ULONG descriptor_type, 
                                            ULONG request_index, ULONG host_length);
UINT  _ux_device_class_hid_report_get(UX_SLAVE_CLASS_HID *hid, ULONG descriptor_type, 
                                            ULONG request_index, ULONG host_length);

/* Define Device HID Class API prototypes.  */

#define ux_device_class_hid_entry        _ux_device_class_hid_entry   
#define ux_device_class_hid_event_set    _ux_device_class_hid_event_set
#define ux_device_class_hid_event_get    _ux_device_class_hid_event_get
#define ux_device_class_hid_report_set   _ux_device_class_hid_report_set
#define ux_device_class_hid_report_get   _ux_device_class_hid_report_get

#define ux_device_class_hid_protocol_get(hid) (hid -> ux_device_class_hid_protocol)


#endif
