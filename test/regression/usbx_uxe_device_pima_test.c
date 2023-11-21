/* This test is designed to test the ux_utility_descriptor_pack.  */

#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_host_stack.h"
#include "ux_device_stack.h"

#include "ux_device_class_pima.h"

#include "ux_test.h"

/* Define USBX test constants.  */

#define UX_TEST_STACK_SIZE      4096
#define UX_TEST_BUFFER_SIZE     2048
#define UX_TEST_MEMORY_SIZE     (64*1024)


/* Define the counters used in the test application...  */

static ULONG                           error_counter = 0;

/* Define USBX test global variables.  */


/* Define prototypes for external Host Controller's (HCDs), classes and clients.  */

static TX_THREAD           ux_test_thread_simulation_0;
static void                ux_test_thread_simulation_0_entry(ULONG);


/* PIMA MTP names ... */
UCHAR device_info_vendor_name[]  =      "Microsoft AzureRTOS";
UCHAR device_info_product_name[] =      "AzureRTOS MTP Device";
UCHAR device_info_serial_no[]    =      "1.1.1.1";
UCHAR device_info_version[]      =      "V1.0";

/* PIMA MTP storage names.  */
UCHAR pima_parameter_volume_description[]        =   "MTP Client Storage Volume";
UCHAR pima_parameter_volume_label[]              =   "MTP Client Storage Label";


/* Define PIMA supported device properties. The last entry MUST be a zero. The DeviceInfoSet command
   will parse this array and compute the number of functions supported and return it to the
   host.  For each declared device property, a dataset must be created in the application.  */
USHORT device_prop_supported[] =   {

    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_UNDEFINED,                                         */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_BATTERY_LEVEL,                                     */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_FUNCTIONALMODE,                                    */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_IMAGE_SIZE,                                        */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_COMPRESSION_SETTING,                               */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_WHITE_BALANCE,                                     */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_RGB_GAIN,                                          */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_F_NUMBER,                                          */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_FOCAL_LENGTH,                                      */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_FOCUS_DISTANCE,                                    */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_FOCUS_MODE,                                        */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_EXPOSURE_METERING_MODE,                            */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_FLASH_MODE,                                        */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_EXPOSURE_TIME,                                     */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_EXPOSURE_PROGRAM_MODE,                             */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_EXPOSURE_INDEX,                                    */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_EXPOSURE_BIAS_COMPENSATION,                        */
    UX_DEVICE_CLASS_PIMA_DEV_PROP_DATE_TIME,
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_CAPTURE_DELAY,                                     */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_STILL_CAPTURE_MODE,                                */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_CONTRAST,                                          */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_SHARPNESS,                                         */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_DIGITAL_ZOOM,                                      */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_EFFECT_MODE,                                       */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_BURST_NUMBER,                                      */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_BURST_INTERVAL,                                    */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_TIME_LAPSE_NUMBER,                                 */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_TIME_LAPSE_INTERVAL,                               */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_FOCUS_METERING_MODE,                               */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_UPLOAD_URL,                                        */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_ARTIST,                                            */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_COPYRIGHT_INFO,                                    */
#ifdef UX_PIMA_WITH_MTP_SUPPORT
    UX_DEVICE_CLASS_PIMA_DEV_PROP_SYNCHRONIZATION_PARTNER,
    UX_DEVICE_CLASS_PIMA_DEV_PROP_DEVICE_FRIENDLY_NAME,
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_VOLUME,                                            */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_SUPPORTED_FORMATS_ORDERED,                         */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_DEVICE_ICON,                                       */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_PLAYBACK_RATE,                                     */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_PLAYBACK_OBJECT,                                   */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_PLAYBACK_CONTAINER,                                */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_SESSION_INITIATOR_VERSION_INFO,                    */
    /*UX_DEVICE_CLASS_PIMA_DEV_PROP_PERCEIVED_DEVICE_TYPE,                             */
#endif
    0
};

/* Define PIMA supported capture formats. The last entry MUST be a zero. The DeviceInfoSet command
   will parse this array and compute the number of functions supported and return it to the
   host.  */
USHORT device_supported_capture_formats[] =       {
    0
};

/* Define PIMA supported image formats. The last entry MUST be a zero. The DeviceInfoSet command
   will parse this array and compute the number of formats supported and return it to the
   host.  */
USHORT device_supported_image_formats[] =        {
    UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED,
    UX_DEVICE_CLASS_PIMA_OFC_ASSOCIATION,
    /*UX_DEVICE_CLASS_PIMA_OFC_SCRIPT,                                                  */
    /*UX_DEVICE_CLASS_PIMA_OFC_EXECUTABLE,                                              */
    /*UX_DEVICE_CLASS_PIMA_OFC_TEXT,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_HTML,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_DPOF,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_AIFF,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_WAV,*/
    UX_DEVICE_CLASS_PIMA_OFC_MP3,
    /*UX_DEVICE_CLASS_PIMA_OFC_AVI,                                                     */
    /*UX_DEVICE_CLASS_PIMA_OFC_MPEG,                                                    */
    UX_DEVICE_CLASS_PIMA_OFC_ASF,
    /*UX_DEVICE_CLASS_PIMA_OFC_DEFINED,                                                 */
    /*UX_DEVICE_CLASS_PIMA_OFC_EXIF_JPEG,                                               */
    /*UX_DEVICE_CLASS_PIMA_OFC_TIFF_EP,                                                 */
    /*UX_DEVICE_CLASS_PIMA_OFC_FLASHPIX,                                                */
    /*UX_DEVICE_CLASS_PIMA_OFC_BMP,                                                     */
    /*UX_DEVICE_CLASS_PIMA_OFC_CIFF,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED,                                               */
    /*UX_DEVICE_CLASS_PIMA_OFC_GIF,                                                     */
    /*UX_DEVICE_CLASS_PIMA_OFC_JFIF,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_CD,                                                      */
    /*UX_DEVICE_CLASS_PIMA_OFC_PICT,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_PNG,                                                     */
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED,                                               */
    /*UX_DEVICE_CLASS_PIMA_OFC_TIFF,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_TIFF_IT,                                                 */
    /*UX_DEVICE_CLASS_PIMA_OFC_JP2,                                                     */
    /*UX_DEVICE_CLASS_PIMA_OFC_JPX,                                                     */
#ifdef UX_PIMA_WITH_MTP_SUPPORT
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED_FIRMWARE,                                      */
    /*UX_DEVICE_CLASS_PIMA_OFC_WINDOWS_IMAGE_FORMAT,                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED_AUDIO,                                         */
    UX_DEVICE_CLASS_PIMA_OFC_WMA,
    /*UX_DEVICE_CLASS_PIMA_OFC_OGG,                                                     */
    /*UX_DEVICE_CLASS_PIMA_OFC_AAC,                                                     */
    /*UX_DEVICE_CLASS_PIMA_OFC_AUDIBLE,                                                 */
    /*UX_DEVICE_CLASS_PIMA_OFC_FLAC,                                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED_VIDEO,                                         */
    UX_DEVICE_CLASS_PIMA_OFC_WMV,
    /*UX_DEVICE_CLASS_PIMA_OFC_MP4_CONTAINER,                                           */
    /*UX_DEVICE_CLASS_PIMA_OFC_MP2,                                                     */
    /*UX_DEVICE_CLASS_PIMA_OFC_3GP_CONTAINER,                                           */
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED_COLLECTION,                                    */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_MULTIMEDIA_ALBUM,                               */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_IMAGE_ALBUM,                                    */
    UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_AUDIO_ALBUM,
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_VIDEO_ALBUM,                                    */
    UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_AUDIO_AND_VIDEO_PLAYLIST,
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_CONTACT_GROUP,                                  */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_MESSAGE_FOLDER,                                 */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_CHAPTERED_PRODUCTION,                           */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_AUDIO_PLAYLIST,                                 */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_VIDEO_PLAYLIST,                                 */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_MEDIACAST,                                      */
    /*UX_DEVICE_CLASS_PIMA_OFC_WPL_PLAYLIST,                                            */
    /*UX_DEVICE_CLASS_PIMA_OFC_M3U_PLAYLIST,                                            */
    /*UX_DEVICE_CLASS_PIMA_OFC_MPL_PLAYLIST,                                            */
    /*UX_DEVICE_CLASS_PIMA_OFC_ASX_PLAYLIST,                                            */
    /*UX_DEVICE_CLASS_PIMA_OFC_PLS_PLAYLIST,                                            */
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED_DOCUMENT,                                      */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_DOCUMENT,                                       */
    /*UX_DEVICE_CLASS_PIMA_OFC_XML_DOCUMENT,                                            */
    /*UX_DEVICE_CLASS_PIMA_OFC_MICROSOFT_WORD_DOCUMENT,                                 */
    /*UX_DEVICE_CLASS_PIMA_OFC_MHT_COMPILED_HTML_DOCUMENT,                              */
    /*UX_DEVICE_CLASS_PIMA_OFC_MICROSOFT_EXCEL_SPREADSHEET,                             */
    /*UX_DEVICE_CLASS_PIMA_OFC_MICROSOFT_POWERPOINT_PRESENTATION,                       */
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED_MESSAGE,                                       */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_MESSAGE,                                        */
    /*UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED_CONTACT,                                       */
    /*UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_CONTACT,                                        */
    /*UX_DEVICE_CLASS_PIMA_OFC_VCARD2,                                                  */
#endif
    0
};

/* Object property supported.
 WORD 0      : Object Format Code
 WORD 1      : Number of Prop codes for this Object format
 WORD n      : Prop Codes
 WORD n+2    : Next Object Format code ....

 This array is in whatever endinaness of the system and will be translated
 by the PTP class in little endian.

*/
USHORT object_prop_supported[] = {

        /* Object format code : Undefined.  */
        UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED,

        /* NUmber of objects supported for this format.  */
        9,
        /* Mandatory objects for all formats.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE,

        /* Object format code : Association.  */
        UX_DEVICE_CLASS_PIMA_OFC_ASSOCIATION,

        /* NUmber of objects supported for this format.  */
        9,
        /* Mandatory objects for all formats.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE,

        /* Object format code : Advanced System Format.  */
        UX_DEVICE_CLASS_PIMA_OFC_ASF,

        /* NUmber of objects supported for this format.  */
        9,
        /* Mandatory objects for all formats.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE,

        /* Object format code : Audio Clip.  */
        UX_DEVICE_CLASS_PIMA_OFC_MP3,

        /* NUmber of objects supported for this format.  */
        20,
        /* Mandatory objects for all formats.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE,

        /* Mandatory objects for all audio objects.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ARTIST,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_TRACK,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DATE_AUTHORED,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_USE_COUNT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_NAME,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_ARTIST,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SAMPLE_RATE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NUMBER_OF_CHANNELS,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_WAVE_CODEC,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_BITRATE,

        /* Object format code : Windows Media Audio Clip.  */
        UX_DEVICE_CLASS_PIMA_OFC_WMA,

        /* NUmber of objects supported for this format.  */
        20,
        /* Mandatory objects for all formats.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE,

        /* Mandatory objects for all audio objects.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ARTIST,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_TRACK,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DATE_AUTHORED,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_USE_COUNT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_NAME,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_ARTIST,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SAMPLE_RATE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NUMBER_OF_CHANNELS,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_WAVE_CODEC,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_BITRATE,

        /* Object format code : Windows Media Video.  */
        UX_DEVICE_CLASS_PIMA_OFC_WMV,

        /* NUmber of objects supported for this format.  */
        24,
        /* Mandatory objects for all formats.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE,

        /* Mandatory objects for all video objects.  */
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_WIDTH,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_HEIGHT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DATE_AUTHORED,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_USE_COUNT,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SAMPLE_RATE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NUMBER_OF_CHANNELS,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SCAN_TYPE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_WAVE_CODEC,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_BITRATE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_VIDEO_FOURCC_CODEC,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_VIDEO_BITRATE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_FRAMES_PER_THOUSAND_SECONDS,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_KEYFRAME_DISTANCE,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ENCODING_PROFILE,

        /* Object format code : Abstract Audio Album.  */
        UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_AUDIO_ALBUM,
        2,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_ARTIST,

        /* Object format code : Abstract Audio and Video Playlist.  */
        UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_AUDIO_AND_VIDEO_PLAYLIST,
        2,
        UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE,
           UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_ARTIST,

        0
};


/* Prototype for test control return.  */

void  test_control_return(UINT status);


UINT demo_device_reset();
UINT demo_device_prop_desc_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG device_property,
                                                UCHAR **device_prop_dataset,
                                                ULONG *device_prop_dataset_length);
UINT demo_device_prop_value_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG device_property,
                                                UCHAR **device_prop_value,
                                                ULONG *device_prop_value_length);
UINT demo_device_prop_value_set(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG device_property,
                                                UCHAR *device_prop_value,
                                                ULONG device_prop_value_length);
UINT demo_storage_format(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG storage_id);
UINT demo_storage_info_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG storage_id);
UINT demo_object_number_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_format_code,
                                                ULONG object_association,
                                                ULONG *object_number);
UINT demo_object_handles_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                 ULONG object_handles_format_code,
                                                 ULONG object_handles_association,
                                                 ULONG *object_handles_array,
                                                 ULONG object_handles_max_number);
UINT demo_object_info_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                UX_SLAVE_CLASS_PIMA_OBJECT **object);
UINT demo_object_data_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                UCHAR *object_buffer,
                                                ULONG object_offset,
                                                 ULONG object_length_requested,
                                                ULONG *object_actual_length);
UINT demo_object_info_send (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                UX_SLAVE_CLASS_PIMA_OBJECT *object,
                                                ULONG storage_id,
                                                ULONG parent_object_handle,
                                                ULONG *object_handle);
UINT demo_object_data_send (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                ULONG phase ,
                                                UCHAR *object_buffer,
                                                ULONG object_offset,
                                                ULONG object_length);
UINT demo_object_delete(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle);
UINT demo_object_prop_desc_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_property,
                                                ULONG object_format_code,
                                                UCHAR **object_prop_dataset,
                                                ULONG *object_prop_dataset_length);
UINT demo_object_prop_value_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                ULONG object_property,
                                                UCHAR **object_prop_value,
                                                ULONG *object_prop_value_length);
UINT demo_object_prop_value_set (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                ULONG object_property,
                                                UCHAR *object_prop_value,
                                                ULONG object_prop_value_length);
UINT demo_object_references_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                UCHAR **object_references_array,
                                                ULONG *object_references_array_length);
UINT demo_object_references_set (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                UCHAR *object_references_array,
                                                ULONG object_references_array_length);
UINT demo_object_handle_check(ULONG object_handle,
                                                UX_SLAVE_CLASS_PIMA_OBJECT **object,
                                                ULONG *caller_handle_index);
UINT demo_device_class_custom_entry(UX_SLAVE_CLASS_COMMAND *command);
UINT demo_vendor_request(ULONG request, ULONG request_value, ULONG request_index, ULONG request_length,
                                               UCHAR *transfer_request_buffer,
                                                   ULONG *transfer_request_length);


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void usbx_uxe_device_pima_test_application_define(void *first_unused_memory)
#endif
{

UINT status;
CHAR                            *stack_pointer;
CHAR                            *memory_pointer;

    /* Inform user.  */
    printf("Running uxe_device_pima APIs Test................................... ");
#if !defined(UX_DEVICE_CLASS_PRINTER_ENABLE_ERROR_CHECKING)
#warning Tests skipped due to compile option!
    printf("SKIP SUCCESS!\n");
    test_control_return(0);
    return;
#endif

    /* Initialize the free memory pointer.  */
    stack_pointer = (CHAR *) first_unused_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory.  */
    status =  ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL, 0);

    /* Check for error.  */
    if (status != UX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the simulation thread.  */
    status =  tx_thread_create(&ux_test_thread_simulation_0, "test simulation", ux_test_thread_simulation_0_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);

    /* Check for error.  */
    if (status != TX_SUCCESS)
    {

        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}


static void  ux_test_thread_simulation_0_entry(ULONG arg)
{
UINT                                    status;
UX_SLAVE_CLASS_PIMA                     dummy_pima_inst;
UX_SLAVE_CLASS_PIMA                     *dummy_pima = &dummy_pima_inst;
UX_SLAVE_CLASS_COMMAND                  dummy_command;
UX_SLAVE_CLASS_PIMA_PARAMETER           dummy_pima_parameter;
UCHAR                                   *dummy_null_string = "";

    dummy_pima_parameter.ux_device_class_pima_parameter_manufacturer                  = device_info_vendor_name;
    dummy_pima_parameter.ux_device_class_pima_parameter_model                         = device_info_product_name;
    dummy_pima_parameter.ux_device_class_pima_parameter_device_version                = device_info_version;
    dummy_pima_parameter.ux_device_class_pima_parameter_serial_number                 = device_info_serial_no;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_id                    = 1;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_type                  = UX_DEVICE_CLASS_PIMA_STC_FIXED_RAM;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_file_system_type      = UX_DEVICE_CLASS_PIMA_FSTC_GENERIC_FLAT;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_access_capability     = UX_DEVICE_CLASS_PIMA_AC_READ_WRITE;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_max_capacity_low      = 1024 * 1024;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_max_capacity_high     = 0;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_free_space_low        = 800 * 1024;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_free_space_high       = 0;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_free_space_image      = 0xFFFFFFFF;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_description           = pima_parameter_volume_description;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_volume_label          = pima_parameter_volume_label;
    /* Property lists.  */
    dummy_pima_parameter.ux_device_class_pima_parameter_device_properties_list = device_prop_supported;
    dummy_pima_parameter.ux_device_class_pima_parameter_supported_capture_formats_list = device_supported_capture_formats;
    dummy_pima_parameter.ux_device_class_pima_parameter_supported_image_formats_list = device_supported_image_formats;
    /* Callback functions.  */
    /* pima_parameter -> ux_device_class_pima_parameter_cancel can be NULL */
    dummy_pima_parameter.ux_device_class_pima_parameter_device_reset            = demo_device_reset;
    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_desc_get    = demo_device_prop_desc_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_value_get   = demo_device_prop_value_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_value_set   = demo_device_prop_value_set;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_format          = demo_storage_format;
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_info_get        = demo_storage_info_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_number_get       = demo_object_number_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_handles_get      = demo_object_handles_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_info_get         = demo_object_info_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_data_get         = demo_object_data_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_info_send        = demo_object_info_send;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_data_send        = demo_object_data_send;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_delete           = demo_object_delete;
#ifdef UX_PIMA_WITH_MTP_SUPPORT
    dummy_pima_parameter.ux_device_class_pima_parameter_object_properties_list  = object_prop_supported;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_desc_get    = demo_object_prop_desc_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_value_get   = demo_object_prop_value_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_value_set   = demo_object_prop_value_set;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_references_get   = demo_object_references_get;
    dummy_pima_parameter.ux_device_class_pima_parameter_object_references_set   = demo_object_references_set;
#endif

    dummy_command.ux_slave_class_command_parameter = &dummy_pima_parameter;

    /* ux_device_class_pima_entry()  */
    dummy_command.ux_slave_class_command_request = UX_SLAVE_CLASS_COMMAND_INITIALIZE;

    dummy_pima_parameter.ux_device_class_pima_parameter_device_reset            = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_device_reset            = demo_device_reset;

    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_desc_get    = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_desc_get    = demo_device_prop_desc_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_value_get   = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_value_get   = demo_device_prop_value_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_value_set   = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_device_prop_value_set   = demo_device_prop_value_set;

    dummy_pima_parameter.ux_device_class_pima_parameter_storage_format          = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_storage_format          = demo_storage_format;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_number_get       = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_number_get       = demo_object_number_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_handles_get      = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_handles_get      = demo_object_handles_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_info_get         = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_info_get         = demo_object_info_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_data_get         = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_data_get         = demo_object_data_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_info_send        = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_info_send        = demo_object_info_send;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_data_send        = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_data_send        = demo_object_data_send;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_delete           = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_delete           = demo_object_delete;

#ifdef UX_PIMA_WITH_MTP_SUPPORT
    dummy_pima_parameter.ux_device_class_pima_parameter_object_properties_list  = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_properties_list  = object_prop_supported;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_desc_get    = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_desc_get    = demo_object_prop_desc_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_value_get   = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_value_get   = demo_object_prop_value_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_value_set   = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_prop_value_set   = demo_object_prop_value_set;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_references_get   = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_references_get   = demo_object_references_get;

    dummy_pima_parameter.ux_device_class_pima_parameter_object_references_set   = UX_NULL;
    status = ux_device_class_pima_entry(&dummy_command);
    UX_TEST_CHECK_CODE(UX_INVALID_PARAMETER, status);
    dummy_pima_parameter.ux_device_class_pima_parameter_object_references_set   = demo_object_references_set;
#endif


    /* Sleep for a tick to make sure everything is complete.  */
    tx_thread_sleep(1);

    /* Check for errors from other threads.  */
    if (error_counter)
    {

        /* Test error.  */
        printf("ERROR #%d: total %ld errors\n", __LINE__, error_counter);
        test_control_return(1);
    }
    else
    {

        /* Successful test.  */
        printf("SUCCESS!\n");
        test_control_return(0);
    }
}

UINT    demo_device_reset()
{
    return(UX_SUCCESS);
}

UINT demo_device_prop_desc_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG device_property,
                                UCHAR **device_prop_dataset,
                                ULONG *device_prop_dataset_length)

{
    return(UX_ERROR);
}

UINT demo_device_prop_value_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG device_property,
                                UCHAR **device_prop_value,
                                ULONG *device_prop_value_length)
{
    return(UX_ERROR);
}

UINT demo_device_prop_value_set(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG device_property,
                                UCHAR *device_prop_value,
                                ULONG device_prop_value_length)
{
    return(UX_ERROR);
}


UINT demo_storage_format(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG storage_id)
{
    return(UX_ERROR);
}


UINT demo_storage_info_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG storage_id)
{
    return(UX_ERROR);
}

UINT demo_object_number_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_format_code,
                                ULONG object_association,
                                ULONG *object_number)
{
    return(UX_ERROR);
}

UINT demo_object_handles_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handles_format_code,
                                ULONG object_handles_association,
                                ULONG *object_handles_array,
                                ULONG object_handles_max_number)
{
    return(UX_ERROR);
}

UINT demo_object_info_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handle,
                                UX_SLAVE_CLASS_PIMA_OBJECT **object)
{
    return(UX_ERROR);
}

UINT demo_object_data_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handle,
                                UCHAR *object_buffer,
                                ULONG object_offset,
                                ULONG object_length_requested,
                                ULONG *object_actual_length)

{
    return(UX_ERROR);
}

UINT demo_object_info_send (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                UX_SLAVE_CLASS_PIMA_OBJECT *object,
                                ULONG storage_id,
                                ULONG parent_object_handle,
                                ULONG *object_handle)
{
    return(UX_ERROR);
}


UINT demo_object_data_send (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handle,
                                ULONG phase ,
                                UCHAR *object_buffer,
                                ULONG object_offset,
                                ULONG object_length)
{
    return(UX_ERROR);
}

UINT demo_object_delete(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handle)
{
    return(UX_ERROR);
}


UINT demo_object_prop_desc_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_property,
                                ULONG object_format_code,
                                UCHAR **object_prop_dataset,
                                ULONG *object_prop_dataset_length)
{
    return(UX_ERROR);
}


UINT demo_object_prop_value_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                    ULONG object_handle,
                                    ULONG object_property,
                                    UCHAR **object_prop_value,
                                    ULONG *object_prop_value_length)
{
    return(UX_ERROR);
}


UINT demo_object_prop_value_set (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                    ULONG object_handle,
                                    ULONG object_property,
                                    UCHAR *object_prop_value,
                                    ULONG object_prop_value_length)
{
    return(UX_ERROR);
}


UINT demo_object_references_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                    ULONG object_handle,
                                    UCHAR **object_references_array,
                                    ULONG *object_references_array_length)
{
    return(UX_ERROR);
}


UINT demo_object_references_set (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                    ULONG object_handle,
                                    UCHAR *object_references_array,
                                    ULONG object_references_array_length)
{
    return(UX_ERROR);
}


UINT demo_object_handle_check(ULONG object_handle,
                                    UX_SLAVE_CLASS_PIMA_OBJECT **object,
                                    ULONG *caller_handle_index)
{
    return(UX_ERROR);
}
