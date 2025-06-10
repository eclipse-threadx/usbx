/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation
 *
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 *
 *
 * SPDX-License-Identifier: MIT
 **************************************************************************/

/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** USBX Component                                                        */
/**                                                                       */
/**   User Specific                                                       */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

/**************************************************************************/
/*                                                                        */
/*  PORT SPECIFIC C INFORMATION                            RELEASE        */
/*                                                                        */
/*    ux_user.h                                           PORTABLE C      */
/*                                                           6.3.0        */
/*                                                                        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Chaoqiong Xiao, Microsoft Corporation                               */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This file contains user defines for configuring USBX in specific    */
/*    ways. This file will have an effect only if the application and     */
/*    USBX library are built with UX_INCLUDE_USER_DEFINE_FILE defined.    */
/*    Note that all the defines in this file may also be made on the      */
/*    command line when building USBX library and application objects.    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     Chaoqiong Xiao           Version 6.4.1                 */
/*                                                                        */
/**************************************************************************/

#ifndef UX_USER_H
#define UX_USER_H



#define UX_MAX_SLAVE_CLASS_DRIVER       1
#define UX_MAX_SLAVE_INTERFACES         1


#define UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH             64
#define UX_SLAVE_REQUEST_DATA_MAX_LENGTH                8

#define UX_DEVICE_ENDPOINT_BUFFER_OWNER                 1
#define UX_DEVICE_CLASS_HID_ZERO_COPY


#define UX_DEVICE_CLASS_HID_EVENT_BUFFER_LENGTH         8
#define UX_DEVICE_CLASS_HID_MAX_EVENTS_QUEUE            2

#define UX_THREAD_STACK_SIZE                            512

#define UX_DEVICE_ALTERNATE_SETTING_SUPPORT_DISABLE
#define UX_DEVICE_INITIALIZE_FRAMEWORK_SCAN_DISABLE

#define UX_NAME_REFERENCED_BY_POINTER

#define UX_ENABLE_ERROR_CHECKING

#define UX_DEVICE_SIDE_ONLY


#define UX_MAX_DEVICE_ENDPOINTS           1
#define UX_MAX_DEVICE_INTERFACES          1

#endif  /* UX_USER_H */
