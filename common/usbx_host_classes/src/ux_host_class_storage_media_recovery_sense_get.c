/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
 * 
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 * 
 * SPDX-License-Identifier: MIT
 **************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** USBX Component                                                        */ 
/**                                                                       */
/**   Storage Class                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


/* Include necessary system files.  */

#define UX_SOURCE_CODE

#include "ux_api.h"
#include "ux_host_class_storage.h"
#include "ux_host_stack.h"


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _ux_host_class_storage_media_recovery_sense_get     PORTABLE C      */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function will send a MODE_SENSE command to recover from a read */
/*    and write error.                                                    */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    storage                               Pointer to storage class      */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    Completion Status                                                   */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    _ux_host_class_storage_cbw_initialize Initialize CBW                */ 
/*    _ux_host_class_storage_transport      Send command                  */ 
/*    _ux_utility_memory_allocate           Allocate memory block         */ 
/*    _ux_utility_memory_free               Release memory block          */ 
/*    _ux_utility_short_put_big_endian      Put short value               */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Storage Class                                                       */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */ 
/*                                                                        */ 
/*  05-19-2020     Chaoqiong Xiao           Initial Version 6.0           */
/*  09-30-2020     Chaoqiong Xiao           Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
UINT  _ux_host_class_storage_media_recovery_sense_get(UX_HOST_CLASS_STORAGE *storage)
{

UINT            status;
UCHAR           *cbw;
UCHAR           *mode_sense_response;
UINT            command_length;


    /* Use a pointer for the cbw, easier to manipulate.  */
    cbw =  (UCHAR *) storage -> ux_host_class_storage_cbw;

    /* Get the Write Command Length.  */
#ifdef UX_HOST_CLASS_STORAGE_INCLUDE_LEGACY_PROTOCOL_SUPPORT
    if (storage -> ux_host_class_storage_interface -> ux_interface_descriptor.bInterfaceSubClass == UX_HOST_CLASS_STORAGE_SUBCLASS_UFI)
        command_length =  UX_HOST_CLASS_STORAGE_MODE_SENSE_COMMAND_LENGTH_UFI;
    else
        command_length =  UX_HOST_CLASS_STORAGE_MODE_SENSE_COMMAND_LENGTH_SBC;
#else
    command_length =  UX_HOST_CLASS_STORAGE_MODE_SENSE_COMMAND_LENGTH_SBC;
#endif

    /* Initialize the CBW for this command.  */
    _ux_host_class_storage_cbw_initialize(storage, UX_HOST_CLASS_STORAGE_DATA_IN, UX_HOST_CLASS_STORAGE_MODE_SENSE_TP_PAGE, command_length);
    
    /* Prepare the MODE_SENSE command block.  Distinguish between SUBCLASSES. */
    switch (storage -> ux_host_class_storage_interface -> ux_interface_descriptor.bInterfaceSubClass)
    {
    
        case UX_HOST_CLASS_STORAGE_SUBCLASS_RBC         :
#ifdef UX_HOST_CLASS_STORAGE_INCLUDE_LEGACY_PROTOCOL_SUPPORT
        case UX_HOST_CLASS_STORAGE_SUBCLASS_UFI         :
#endif
            *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_MODE_SENSE_OPERATION) =  UX_HOST_CLASS_STORAGE_SCSI_MODE_SENSE;
            break;

        default                                         :            
            *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_MODE_SENSE_OPERATION) =  UX_HOST_CLASS_STORAGE_SCSI_MODE_SENSE_SHORT;
            break;
    }
    
    /* We ask for a specific page page but we put the buffer to maximum size.  */
    *(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_MODE_SENSE_PC_PAGE_CODE) = UX_HOST_CLASS_STORAGE_MODE_SENSE_TP_PAGE;
    
    /* Store the length of the Mode Sense Response.  */
    _ux_utility_short_put_big_endian(cbw + UX_HOST_CLASS_STORAGE_CBW_CB + UX_HOST_CLASS_STORAGE_MODE_SENSE_PARAMETER_LIST_LENGTH, UX_HOST_CLASS_STORAGE_MODE_SENSE_ALL_PAGE_LENGTH);

    /* Obtain a block of memory for the answer.  */
    mode_sense_response =  _ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_CACHE_SAFE_MEMORY, UX_HOST_CLASS_STORAGE_MODE_SENSE_ALL_PAGE_LENGTH);
    if (mode_sense_response == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);
    
    /* Send the command to transport layer.  */
    status =  _ux_host_class_storage_transport(storage, mode_sense_response);
    
    /* Free the memory resource used for the command response.  */
    _ux_utility_memory_free(mode_sense_response);
    
    /* Return completion status.  */
    return(status);                                            
}

