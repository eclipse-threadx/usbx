# Purpose
* Build USBX Host Video (UVC) Example from existing GitHub Example, which
  * Enumerates a connected USB 2.0 high speed webcam
  * And start streaming 176x144 MJPEG video

# Steps
## Get MIMXRT1060 (IAR) Examples
* Download: https://github.com/azure-rtos/samples/releases/download/v6.1_rel/Azure_RTOS_6.1_MIMXRT1060_IAR_Samples_2021_11_03.zip
* Extract
* Confirm project `sample_usbx_host_mass_storage` works

## Modifications in `sample_usbx_host_mass_storage.c`
* Add include file
```c
#include "ux_host_class_video.h"
```

* Add global variables
```c
/* Define the number of buffers used in this demo. */
#define VIDEO_BUFFER_NB (UX_HOST_CLASS_VIDEO_TRANSFER_REQUEST_COUNT - 1)

UX_HOST_CLASS_VIDEO                 *video;

#pragma location="NonCacheable"
UCHAR                               video_buffer[UX_HOST_CLASS_VIDEO_TRANSFER_REQUEST_COUNT][3072];

/* This semaphore is used for the callback function to signal application thread
   that video data is received and can be processed. */
TX_SEMAPHORE data_received_semaphore;
```

* Add instance check function (before `demo_thread_entry`)
```c
static UINT  demo_class_video_check()
{

UINT status;
UX_HOST_CLASS               *host_class;
UX_HOST_CLASS_VIDEO         *inst;


    /* Find the main video container.  */
    status = ux_host_stack_class_get(_ux_system_host_class_video_name, &host_class);
    if (status != UX_SUCCESS)
        while(1); /* Error Halt  */

    /* We get the first instance of the video device.  */
    while (1)
    {
        status = ux_host_stack_class_instance_get(host_class, 0, (void **) &inst);
        if (status == UX_SUCCESS)
            break;

        tx_thread_sleep(10);
    }

    /* We still need to wait for the video status to be live */
    while (inst -> ux_host_class_video_state != UX_HOST_CLASS_INSTANCE_LIVE)
    {
        tx_thread_sleep(10);
    }

    video = inst;
    return(UX_SUCCESS);
}
```

* Add video transfer done callback (before `demo_thread_entry`)
  The callback function is referenced in `demo_thread_entry`, by `ux_host_class_video_transfer_callback_set`. It informs application that payload is transferred. We put a semaphore here to unblock the video data processing thread, to handle data payloads.
```c
/* video data received callback function. */
static VOID video_transfer_done (UX_TRANSFER * transfer_request)
{

UINT status;

    status = tx_semaphore_put(&data_received_semaphore);
    if (status != UX_SUCCESS)
        while(1); /* Error Halt.  */
}
```

* Add class registration (in `demo_thread_entry`)
  Find the code that register storage class via `ux_host_stack_class_register` and add following code to register video class.
```c
    /* Register video class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_video_name, _ux_host_class_video_entry);
    if (status != UX_SUCCESS)
        return;
```

* Replace the `while` loop code block in `demo_thread_entry`
  Find the `while(1)` loop and replace the whole block, to
  * Create semaphore mentioned above,
  * wait before webcam is connected,
  * set transfer callback and setup video parameters,
  * start video transfer,
  * prepare video buffers for receiving,
  * then start a loop that polling video payload data, handle them and re-use buffers
```c
UINT i;
UINT buffer_index;
UCHAR *video_buffers[VIDEO_BUFFER_NB];

    /* Assume video points to a valid video instance. */
    /* Create the semaphore for signaling video data received. */
    status = tx_semaphore_create(&data_received_semaphore, "payload semaphore", 0);
    if (status != UX_SUCCESS)
        while(1); /* Error Halt.  */

    /* Wait for camera to be plugged in.  */
    demo_class_video_check();

    /* Set transfer callback. */
    ux_host_class_video_transfer_callback_set(video,
                                              video_transfer_done);

    /* Set video parameters to MJPEG, 640x480 resolution, 30fps. */
    status = ux_host_class_video_frame_parameters_set(video,
                                                      UX_HOST_CLASS_VIDEO_VS_FORMAT_MJPEG, 176, 144, 333333);
    if (status != UX_SUCCESS)
        while(1); /* Error Halt.  */

    /* Start video transfer. */
    status = ux_host_class_video_start(video);
    if (status != UX_SUCCESS)
        while(1); /* Error Halt.  */

    /* Build buffer list.  */
    for (i = 0; i < VIDEO_BUFFER_NB; i ++)
        video_buffers[i] = video_buffer[i];

    /* Issue transfer request list to start streaming.  */
    ux_host_class_video_transfer_buffers_add(video, video_buffers, VIDEO_BUFFER_NB);

    /* Polling video payloads, handle them and re-enable the buffer for transfer.  */
    buffer_index = 0;
    while (1)
    {

        /* Suspend here until a transfer callback is called. */
        status = tx_semaphore_get(&data_received_semaphore, TX_WAIT_FOREVER);
        if (status != UX_SUCCESS)
            while(1); /* Error Halt.  */

        /* Received data. The callback function needs to obtain the actual
           number of bytes received, so the application routine can read the
           correct amount of data from the buffer. */

        /* Application can now consume video data while the video device stores
           the data into the other buffer. */

        /* Add the buffer back for video transfer. */
        status = ux_host_class_video_transfer_buffer_add(video,
                                                         video_buffer[buffer_index]);
        if (status != UX_SUCCESS)
            while(1); /* Error Halt.  */

        /* Increment the buffer_index, and wrap to zero if it exceeds the
           maximum number of buffers. */
        buffer_index = (buffer_index + 1);
        if(buffer_index >= VIDEO_BUFFER_NB)
            buffer_index = 0;
    }
```