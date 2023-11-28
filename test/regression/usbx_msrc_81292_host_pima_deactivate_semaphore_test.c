/* This test is designed to test the simple dpump host/device class operation.  */

#include <stdio.h>
#include "tx_api.h"
#include "fx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_device_class_pima.h"
#include "ux_host_class_pima.h"

#include "ux_hcd_sim_host.h"
#include "ux_device_stack.h"

#include "ux_test_dcd_sim_slave.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_utility_sim.h"


/* Define constants.  */
#define UX_TEST_STACK_SIZE          (2*1024)
#define UX_TEST_MEMORY_SIZE         (512*1024)

#define UX_TEST_RAM_DISK_SIZE       (128*1024)
#define UX_TEST_RAM_DISK_LAST_LBA   ((UX_TEST_RAM_DISK_SIZE / 512) - 1)

#define UX_TEST_MAX_HANDLES                 16
#define UX_TEST_MAX_DATASET_HEADER          32
#define UX_TEST_MAX_DATASET_SIZE            1024
#define UX_TEST_PIMA_STORAGE_ID             1
#define UX_TEST_VENDOR_REQUEST              0x54

#define UX_TEST_CUSTOM_VID                  0x0000
#define UX_TEST_CUSTOM_PID                  0x0000

#define UX_TEST_AUDIO_CODEC_WAVE_FORMAT_MPEGLAYER3      0x00000055
#define UX_TEST_AUDIO_CODEC_WAVE_FORMAT_MPEG            0x00000050
#define UX_TEST_AUDIO_CODEC_WAVE_FORMAT_RAW_AAC1        0x000000FF


/* Define local/extern function prototypes.  */

extern VOID  _fx_ram_driver(FX_MEDIA *media_ptr);

static void        test_thread_entry(ULONG);

static TX_THREAD   test_thread_host_simulation;
static TX_THREAD   test_thread_device_simulation;
static void        test_thread_host_simulation_entry(ULONG);
static void        test_thread_device_simulation_entry(ULONG);

static VOID        test_hcd_entry_set_cfg(UX_TEST_ACTION *action, VOID *params);
static VOID        test_hcd_entry_interaction_request_sem_put(UX_TEST_ACTION *action, VOID *params);
static VOID        test_hcd_entry_interaction_invoked(UX_TEST_ACTION *action, VOID *params);
static VOID        test_hcd_entry_interaction_wait_transfer_disconnection(UX_TEST_ACTION *action, VOID *params);

static VOID        test_pima_instance_activate(VOID  *pima_instance);
static VOID        test_pima_instance_deactivate(VOID *pima_instance);

UINT pima_device_device_reset();
UINT pima_device_device_prop_desc_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG device_property,
                                                UCHAR **device_prop_dataset,
                                                ULONG *device_prop_dataset_length);
UINT pima_device_device_prop_value_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG device_property,
                                                UCHAR **device_prop_value,
                                                ULONG *device_prop_value_length);
UINT pima_device_device_prop_value_set(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG device_property,
                                                UCHAR *device_prop_value,
                                                ULONG device_prop_value_length);
UINT pima_device_storage_format(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG storage_id);
UINT pima_device_storage_info_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG storage_id);
UINT pima_device_object_number_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_format_code,
                                                ULONG object_association,
                                                ULONG *object_number);
UINT pima_device_object_handles_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                 ULONG object_handles_format_code,
                                                 ULONG object_handles_association,
                                                 ULONG *object_handles_array,
                                                 ULONG object_handles_max_number);
UINT pima_device_object_info_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                UX_SLAVE_CLASS_PIMA_OBJECT **object);
UINT pima_device_object_data_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                UCHAR *object_buffer,
                                                ULONG object_offset,
                                                 ULONG object_length_requested,
                                                ULONG *object_actual_length);
UINT pima_device_object_info_send (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                UX_SLAVE_CLASS_PIMA_OBJECT *object,
                                                ULONG storage_id,
                                                ULONG parent_object_handle,
                                                ULONG *object_handle);
UINT pima_device_object_data_send (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                ULONG phase ,
                                                UCHAR *object_buffer,
                                                ULONG object_offset,
                                                ULONG object_length);
UINT pima_device_object_delete(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle);
UINT pima_device_object_prop_desc_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_property,
                                                ULONG object_format_code,
                                                UCHAR **object_prop_dataset,
                                                ULONG *object_prop_dataset_length);
UINT pima_device_object_prop_value_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                ULONG object_property,
                                                UCHAR **object_prop_value,
                                                ULONG *object_prop_value_length);
UINT pima_device_object_prop_value_set (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                ULONG object_property,
                                                UCHAR *object_prop_value,
                                                ULONG object_prop_value_length);
UINT pima_device_object_references_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                UCHAR **object_references_array,
                                                ULONG *object_references_array_length);
UINT pima_device_object_references_set (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                                ULONG object_handle,
                                                UCHAR *object_references_array,
                                                ULONG object_references_array_length);
UINT pima_device_object_handle_check(ULONG object_handle,
                                                UX_SLAVE_CLASS_PIMA_OBJECT **object,
                                                ULONG *caller_handle_index);
UINT pima_device_device_class_custom_entry(UX_SLAVE_CLASS_COMMAND *command);
UINT pima_device_vendor_request(ULONG request, ULONG request_value, ULONG request_index, ULONG request_length,
                                               UCHAR *transfer_request_buffer,
                                                   ULONG *transfer_request_length);


/* Define global data structures.  */
static UCHAR                               usbx_memory[UX_TEST_MEMORY_SIZE + (UX_TEST_STACK_SIZE * 2)];

static UX_SLAVE_CLASS_PIMA_DEVICE          *pima_device;
static UX_SLAVE_CLASS_PIMA_PARAMETER       pima_device_parameter;

static UX_HOST_CLASS_PIMA                  *pima_host;
static UX_HOST_CLASS_PIMA_SESSION          pima_host_session;
static UX_HOST_CLASS_PIMA_DEVICE           pima_host_device;
static UX_HOST_CLASS_PIMA_OBJECT           pima_host_object;

static ULONG                               host_buffer[4096];
static UCHAR                               *host_buffer8 =  (UCHAR  *)host_buffer;
static USHORT                              *host_buffer16 = (USHORT *)host_buffer;
static ULONG                               *host_buffer32 = (ULONG  *)host_buffer;

static FX_MEDIA                            ram_disk;
static CHAR                                ram_disk_memory[UX_TEST_RAM_DISK_SIZE];
static CHAR                                ram_disk_buffer[2048];


static ULONG                               error_counter;

static ULONG                               set_cfg_counter;

static ULONG                               rsc_mem_free_on_set_cfg;
static ULONG                               rsc_sem_on_set_cfg;
static ULONG                               rsc_sem_get_on_set_cfg;
static ULONG                               rsc_mutex_on_set_cfg;

static ULONG                               rsc_enum_sem_usage;
static ULONG                               rsc_enum_sem_get_count;
static ULONG                               rsc_enum_mutex_usage;
static ULONG                               rsc_enum_mem_usage;

static ULONG                               rsc_cdc_sem_usage;
static ULONG                               rsc_cdc_sem_get_count;
static ULONG                               rsc_cdc_mutex_usage;
static ULONG                               rsc_cdc_mem_usage;

static ULONG                               interaction_count;

static UCHAR                               error_callback_ignore = UX_TRUE;
static ULONG                               error_callback_counter;


/* Structure of the Object property dataset.  This is specific to the local device. */
typedef struct TEST_PIMA_OBJECT_PROP_DATASET_STRUCT
{
    ULONG                        test_pima_object_prop_dataset_storage_id;
    ULONG                        test_pima_object_prop_dataset_object_format;
    ULONG                        test_pima_object_prop_dataset_protection_status;
    ULONG                        test_pima_object_prop_dataset_object_size_low;
    ULONG                        test_pima_object_prop_dataset_object_size_high;
    UCHAR                        test_pima_object_prop_dataset_object_file_name[128];
    ULONG                        test_pima_object_prop_dataset_parent_object;
    ULONG                        test_pima_object_prop_dataset_persistent_unique_object_identifier[4];
    UCHAR                        test_pima_object_prop_dataset_name[128];
    UCHAR                        test_pima_object_prop_dataset_non_consumable;
    UCHAR                        test_pima_object_prop_dataset_artist[128];
    ULONG                        test_pima_object_prop_dataset_track;
    ULONG                        test_pima_object_prop_dataset_use_count;
    UCHAR                        test_pima_object_prop_dataset_date_authored[16];
    UCHAR                        test_pima_object_prop_dataset_genre[128];
    UCHAR                        test_pima_object_prop_dataset_album_name[128];
    UCHAR                        test_pima_object_prop_dataset_album_artist[128];
    ULONG                        test_pima_object_prop_dataset_sample_rate;
    ULONG                        test_pima_object_prop_dataset_number_of_channels;
    ULONG                        test_pima_object_prop_dataset_audio_wave_codec;
    ULONG                        test_pima_object_prop_dataset_audio_bitrate;
    ULONG                        test_pima_object_prop_dataset_duration;
    ULONG                        test_pima_object_prop_dataset_width;
    ULONG                        test_pima_object_prop_dataset_height;
    ULONG                        test_pima_object_prop_dataset_scan_type;
    ULONG                        test_pima_object_prop_dataset_fourcc_codec;
    ULONG                        test_pima_object_prop_dataset_video_bitrate;
    ULONG                        test_pima_object_prop_dataset_frames_per_thousand_seconds;
    ULONG                        test_pima_object_prop_dataset_keyframe_distance;
    UCHAR                        test_pima_object_prop_dataset_encoding_profile[128];

} TEST_PIMA_PROP_DATASET;


/* Define device framework.  */

UCHAR device_framework_full_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0xE8, 0x04, 0xC5, 0x68, 0x00, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Configuration descriptor */
        0x09, 0x02, 0x27, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x03, 0x06, 0x01, 0x01,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Interrupt In) */
        0x07, 0x05, 0x83, 0x03, 0x40, 0x00, 0x04
};
#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED sizeof(device_framework_full_speed)

UCHAR device_framework_high_speed[] = {

    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0xE8, 0x04, 0xC5, 0x68, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x27, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32,

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x03, 0x06, 0x01, 0x01,
        0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x81, 0x02, 0x00, 0x02, 0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x02, 0x02, 0x00, 0x02, 0x00,

    /* Endpoint descriptor (Interrupt In) */
        0x07, 0x05, 0x83, 0x03, 0x40, 0x00, 0x04
};
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED sizeof(device_framework_high_speed)

/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US    or 0x0000 for none.
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string

    The last string entry can be the optional Microsoft String descriptor.
*/
UCHAR string_framework[] = {

    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c,
        0x45, 0x78, 0x70, 0x72, 0x65, 0x73, 0x20, 0x4c,
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0a,
        0x4d, 0x54, 0x50, 0x20, 0x70, 0x6c, 0x61, 0x79,
        0x65, 0x72,

    /* Serial Number string descriptor : Index 3 */
        0x09, 0x04, 0x03, 0x04,
        0x30, 0x30, 0x30, 0x31,

    /* Microsoft OS string descriptor : Index 0xEE. String is MSFT100.
       The last byte is the vendor code used to filter Vendor specific commands.
       The vendor commands will be executed in the class.
       This code can be anything but must not be 0x66 or 0x67 which are PIMA class commands.  */
        0x00, 0x00, 0xEE, 0x08,
        0x4D, 0x53, 0x46, 0x54,
        0x31, 0x30, 0x30,
        UX_TEST_VENDOR_REQUEST

};
#define STRING_FRAMEWORK_LENGTH sizeof(string_framework)

/* Multiple languages are supported on the device, to add
    a language besides english, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
UCHAR language_id_framework[] = {

    /* English. */
        0x09, 0x04
};
#define LANGUAGE_ID_FRAMEWORK_LENGTH sizeof(language_id_framework)


/* Define PIMA supported device properties. The last entry MUST be a zero. The DeviceInfoSet command
   will parse this array and compute the number of functions supported and return it to the
   host.  For each declared device property, a dataset must be created in the application.  */
USHORT pima_device_prop_supported[] =   {

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
USHORT pima_device_supported_capture_formats[] =       {
    0
};

/* Define PIMA supported image formats. The last entry MUST be a zero. The DeviceInfoSet command
   will parse this array and compute the number of formats supported and return it to the
   host.  */
USHORT pima_device_supported_image_formats[] =        {
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

/* Device property dataset. Here we give the example of the Date/Time dataset.  */
UCHAR pima_device_prop_date_time_dataset[] = {

    /* Device prop code : Date/Time.  */
        0x11, 0x50,                                            /* Prop code */
        0xff, 0xff,                                            /* String    */
        0x01,                                                  /* GET/SET   */
        0x00,                                                  /* Default value : empty string.  */
        0x10,                                                  /* Current value : length of the unicode string.  */
        0x31, 0x00, 0x39, 0x00, 0x38, 0x00,    0x30, 0x00,     /* YYYY */
        0x30, 0x00, 0x31, 0x00,                                /* MM */
        0x30, 0x00, 0x31, 0x00,                                /* DD */
        0x54, 0x00,                                            /* T  */
        0x30, 0x00, 0x30, 0x00,                                /* HH */
        0x30, 0x00, 0x30, 0x00,                                /* MM */
        0x30, 0x00, 0x30, 0x00,                                /* SS */
        0x00, 0x00,                                            /* Unicode terminator.  */
        0x00                                                   /* Form Flag : None.  */
};
#define DEVICE_PROP_DATE_TIME_DATASET_LENGTH sizeof(pima_device_prop_date_time_dataset) /* 40 */

/* Device property dataset. Here we give the example of the synchronization partner dataset.  */
UCHAR pima_device_prop_synchronization_partner_dataset[] = {

    /* Device prop code : Synchronization Partner.  */
        0x01, 0xD4,                                            /* Prop code */
        0xff, 0xff,                                            /* String    */
        0x01,                                                  /* GET/SET   */
        0x00,                                                  /* Default value : empty string.  */
        0x00,                                                  /* Current value : empty string.  */
        0x00                                                   /* Form Flag : None.  */
};
#define DEVICE_PROP_SYNCHRONIZATION_PARTNER_DATASET_LENGTH sizeof(pima_device_prop_synchronization_partner_dataset) /* 8 */

/* Device property dataset. Here we give the example of the device friendly name dataset.  */
UCHAR pima_device_prop_device_friendly_name_dataset[] = {

    /* Device prop code : Device Friendly Name.  */
        0x02, 0xD4,                                            /* Prop code */
        0xff, 0xff,                                            /* String    */
        0x01,                                                  /* GET/SET   */
        0x0E,                                                  /* Default value.  Length of Unicode string. */
        0x45, 0x00, 0x4C, 0x00, 0x20, 0x00, 0x4D, 0x00,        /* Unicode string.  */
        0x54, 0x00, 0x50, 0x00, 0x20, 0x00, 0x44, 0x00,
        0x65, 0x00, 0x76, 0x00, 0x69, 0x00, 0x63, 0x00,
        0x65, 0x00,
        0x00, 0x00,                                            /* Unicode terminator. */
        0x0E,                                                  /* Current value.  Length of Unicode string. */
        0x45, 0x00, 0x4C, 0x00, 0x20, 0x00, 0x4D, 0x00,
        0x54, 0x00, 0x50, 0x00, 0x20, 0x00, 0x44, 0x00,
        0x65, 0x00, 0x76, 0x00, 0x69, 0x00, 0x63, 0x00,        /* Unicode terminator.  */
        0x65, 0x00,
        0x00, 0x00,                                            /* Unicode terminator.  */
        0x00                                                   /* Form Flag : None.  */
};
#define DEVICE_PROP_DEVICE_FRIENDLY_NAME_DATASET_LENGTH sizeof(pima_device_prop_device_friendly_name_dataset) /* 64 */

/* Object property supported.
 WORD 0      : Object Format Code
 WORD 1      : Number of Prop codes for this Object format
 WORD n      : Prop Codes
 WORD n+2    : Next Object Format code ....

 This array is in whatever endinaness of the system and will be translated
 by the PTP class in little endian.

*/
USHORT pima_device_object_prop_supported[] = {

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

/* PIMA MTP names ... */
UCHAR pima_device_info_vendor_name[]  =      "Microsoft AzureRTOS";
UCHAR pima_device_info_product_name[] =      "AzureRTOS MTP Device";
UCHAR pima_device_info_serial_no[]    =      "1.1.1.1";
UCHAR pima_device_info_version[]      =      "V1.0";

/* PIMA MTP storage names.  */
UCHAR pima_parameter_volume_description[]        =   "MTP Client Storage Volume";
UCHAR pima_parameter_volume_label[]              =   "MTP Client Storage Label";

/* Array of handles for the demo.  */
ULONG                                            pima_device_object_number_handles;
ULONG                                            pima_device_object_number_handles_array[UX_TEST_MAX_HANDLES];
UX_SLAVE_CLASS_PIMA_OBJECT                       pima_device_object_info_array[UX_TEST_MAX_HANDLES];
FX_FILE                                          pima_device_object_filex_array[UX_TEST_MAX_HANDLES];
TEST_PIMA_PROP_DATASET                           pima_device_object_property_array[UX_TEST_MAX_HANDLES];

/* Local storage for one Object property. This is a temporary storage to create
   the demanded dataset.  The size of the dataset depends on the type and number of object properties. */
UCHAR    pima_device_object_property_dataset_data_buffer        [UX_TEST_MAX_DATASET_SIZE];

/* This is a 128 bit unique identifier. For the demo we make it 32 bits only as it is easier to manipulate.  */
ULONG    pima_device_object_persistent_unique_identifier        = 1;


/* Prototype for test control return.  */

void  test_control_return(UINT status);

static VOID error_callback(UINT system_level, UINT system_context, UINT error_code)
{

    error_callback_counter ++;

    if (!error_callback_ignore)
    {
        {
            /* Failed test.  */
            printf("Error #%d, system_level: %d, system_context: %d, error_code: 0x%x\n", __LINE__, system_level, system_context, error_code);
            test_control_return(1);
        }
    }
}

static UINT  sleep_break_on_error(VOID)
{

    if (error_callback_counter >= 3)
        return error_callback_counter;

    return UX_SUCCESS;
}

/* Define the ISR dispatch routine.  */

static void    test_isr(void)
{

    /* For further expansion of interrupt-level testing.  */
}



static UINT test_system_host_change_function(ULONG event, UX_HOST_CLASS *cls, VOID *inst)
{
    // printf("hChg:%lx, %p, %p\n", event, cls, inst);
    switch(event)
    {
        case UX_DEVICE_INSERTION:
            if (cls->ux_host_class_entry_function == ux_host_class_pima_entry)
            {
                pima_host = (UX_HOST_CLASS_PIMA *)inst;
            }
            break;

        case UX_DEVICE_REMOVAL:
            if (cls->ux_host_class_entry_function == ux_host_class_pima_entry)
            {
                if ((VOID*)pima_host == inst)
                    pima_host = UX_NULL;
            }
            break;

        default:
            break;
    }
    return 0;
}

static VOID    test_pima_instance_activate(VOID *instance)
{
    pima_device = (UX_SLAVE_CLASS_PIMA_DEVICE *)instance;
}
static VOID    test_pima_instance_deactivate(VOID *instance)
{
    if ((VOID *)pima_device == instance)
        pima_device = UX_NULL;
}


/* Define what the initial system looks like.  */

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_msrc_81292_pima_deactivate_semaphore_test_application_define(void *first_unused_memory)
#endif
{

UINT                    status;
CHAR *                  stack_pointer;
CHAR *                  memory_pointer;
ULONG                   test_n;

    /* Inform user.  */
    printf("Running MSRC 81292 - PIMA Deactivate Semaphore Test................. ");

    /* Reset testing counts. */
    ux_test_utility_sim_mutex_create_count_reset();
    ux_test_utility_sim_sem_create_count_reset();
    ux_test_utility_sim_sem_get_count_reset();
    /* Reset error generations */
    ux_test_utility_sim_sem_error_generation_stop();
    ux_test_utility_sim_mutex_error_generation_stop();
    ux_test_utility_sim_sem_get_error_generation_stop();

    ux_utility_memory_set(ram_disk_memory, 0, UX_TEST_RAM_DISK_SIZE);
    fx_system_initialize();
    status = fx_media_format(&ram_disk, _fx_ram_driver, ram_disk_memory, ram_disk_buffer, 512, "RAM DISK", 2, 512, 0, (UX_TEST_RAM_DISK_SIZE / 512), 512, 4, 1, 1);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
    status = fx_media_open(&ram_disk, "RAM DISK", _fx_ram_driver, ram_disk_memory, ram_disk_buffer, 512);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_TEST_STACK_SIZE * 2);

    /* Initialize USBX Memory */
    status = ux_system_initialize(memory_pointer, UX_TEST_MEMORY_SIZE, UX_NULL,0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register the error callback. */
    _ux_utility_error_callback_register(error_callback);

    /* The code below is required for installing the host portion of USBX */
    status =  ux_host_stack_initialize(test_system_host_change_function);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register PIMA class.  */
    status =  ux_host_stack_class_register(_ux_system_host_class_pima_name, ux_host_class_pima_entry);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* The code below is required for installing the device portion of USBX. No call back for
       device status change in this example. */
    status =  ux_device_stack_initialize(device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
                                       device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
                                       string_framework, STRING_FRAMEWORK_LENGTH,
                                       language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH,UX_NULL);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* MTP requires MTP extensions.  */
    status = _ux_device_stack_microsoft_extension_register(UX_TEST_VENDOR_REQUEST, pima_device_vendor_request);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Set the parameters for PIMA device.  */
    pima_device_parameter.ux_device_class_pima_instance_activate   =  test_pima_instance_activate;
    pima_device_parameter.ux_device_class_pima_instance_deactivate =  test_pima_instance_deactivate;

    /* Initialize the pima device parameter.  */
    pima_device_parameter.ux_device_class_pima_parameter_manufacturer                  = pima_device_info_vendor_name;
    pima_device_parameter.ux_device_class_pima_parameter_model                         = pima_device_info_product_name;
    pima_device_parameter.ux_device_class_pima_parameter_device_version                = pima_device_info_version;
    pima_device_parameter.ux_device_class_pima_parameter_serial_number                 = pima_device_info_serial_no;
    pima_device_parameter.ux_device_class_pima_parameter_storage_id                    = UX_TEST_PIMA_STORAGE_ID;
    pima_device_parameter.ux_device_class_pima_parameter_storage_type                  = UX_DEVICE_CLASS_PIMA_STC_FIXED_RAM;
    pima_device_parameter.ux_device_class_pima_parameter_storage_file_system_type      = UX_DEVICE_CLASS_PIMA_FSTC_GENERIC_FLAT;
    pima_device_parameter.ux_device_class_pima_parameter_storage_access_capability     = UX_DEVICE_CLASS_PIMA_AC_READ_WRITE;
    pima_device_parameter.ux_device_class_pima_parameter_storage_max_capacity_low      = ram_disk.fx_media_total_clusters * ram_disk.fx_media_sectors_per_cluster * ram_disk.fx_media_bytes_per_sector;
    pima_device_parameter.ux_device_class_pima_parameter_storage_max_capacity_high     = 0;
    pima_device_parameter.ux_device_class_pima_parameter_storage_free_space_low        = ram_disk.fx_media_available_clusters * ram_disk.fx_media_sectors_per_cluster * ram_disk.fx_media_bytes_per_sector;
    pima_device_parameter.ux_device_class_pima_parameter_storage_free_space_high       = 0;
    pima_device_parameter.ux_device_class_pima_parameter_storage_free_space_image      = 0xFFFFFFFF;
    pima_device_parameter.ux_device_class_pima_parameter_storage_description           = pima_parameter_volume_description;
    pima_device_parameter.ux_device_class_pima_parameter_storage_volume_label          = pima_parameter_volume_label;
    pima_device_parameter.ux_device_class_pima_parameter_device_properties_list        = pima_device_prop_supported;
    pima_device_parameter.ux_device_class_pima_parameter_supported_capture_formats_list= pima_device_supported_capture_formats;
    pima_device_parameter.ux_device_class_pima_parameter_supported_image_formats_list  = pima_device_supported_image_formats;
    pima_device_parameter.ux_device_class_pima_parameter_object_properties_list        = pima_device_object_prop_supported;

    /* Define the callbacks.  */
    pima_device_parameter.ux_device_class_pima_parameter_device_reset                  = pima_device_device_reset;
    pima_device_parameter.ux_device_class_pima_parameter_device_prop_desc_get          = pima_device_device_prop_desc_get;
    pima_device_parameter.ux_device_class_pima_parameter_device_prop_value_get         = pima_device_device_prop_value_get;
    pima_device_parameter.ux_device_class_pima_parameter_device_prop_value_set         = pima_device_device_prop_value_set;
    pima_device_parameter.ux_device_class_pima_parameter_storage_format                = pima_device_storage_format;
    pima_device_parameter.ux_device_class_pima_parameter_storage_info_get              = pima_device_storage_info_get;
    pima_device_parameter.ux_device_class_pima_parameter_object_number_get             = pima_device_object_number_get;
    pima_device_parameter.ux_device_class_pima_parameter_object_handles_get            = pima_device_object_handles_get;
    pima_device_parameter.ux_device_class_pima_parameter_object_info_get               = pima_device_object_info_get;
    pima_device_parameter.ux_device_class_pima_parameter_object_data_get               = pima_device_object_data_get;
    pima_device_parameter.ux_device_class_pima_parameter_object_info_send              = pima_device_object_info_send;
    pima_device_parameter.ux_device_class_pima_parameter_object_data_send              = pima_device_object_data_send;
    pima_device_parameter.ux_device_class_pima_parameter_object_delete                 = pima_device_object_delete;
    pima_device_parameter.ux_device_class_pima_parameter_object_prop_desc_get          = pima_device_object_prop_desc_get;
    pima_device_parameter.ux_device_class_pima_parameter_object_prop_value_get         = pima_device_object_prop_value_get;
    pima_device_parameter.ux_device_class_pima_parameter_object_prop_value_set         = pima_device_object_prop_value_set;
    pima_device_parameter.ux_device_class_pima_parameter_object_references_get         = pima_device_object_references_get;
    pima_device_parameter.ux_device_class_pima_parameter_object_references_set         = pima_device_object_references_set;

    /* Store the instance owner.  */
    pima_device_parameter.ux_device_class_pima_parameter_application                   = (VOID *) 0;

    /* Initialize the device PIMA class.  */
    status =  ux_device_stack_class_register(_ux_system_slave_class_pima_name, ux_device_class_pima_entry,
                                             1, 0, &pima_device_parameter);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Initialize the simulated device controller.  */
    status =  _ux_dcd_sim_slave_initialize();
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Register all the USB host controllers available in this system */
    status =  ux_host_stack_hcd_register(_ux_system_host_hcd_simulator_name, _ux_test_hcd_sim_host_initialize,0,0);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main host simulation thread.  */
    status =  tx_thread_create(&test_thread_host_simulation, "tx demo host simulation", test_thread_host_simulation_entry, 0,
            stack_pointer, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    /* Create the main slave simulation  thread.  */
    status =  tx_thread_create(&test_thread_device_simulation, "tx demo device simulation", test_thread_device_simulation_entry, 0,
            stack_pointer + UX_TEST_STACK_SIZE, UX_TEST_STACK_SIZE,
            20, 20, 1, TX_AUTO_START);
    if (status != UX_SUCCESS)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }
}

static void test_call_pima_apis(void)
{
UINT  status;
ULONG actual_length;

    status = ux_host_class_pima_device_info_get(pima_host, &pima_host_device);
    UX_TEST_CHECK_SUCCESS(status);

    status = ux_host_class_pima_session_open(pima_host, &pima_host_session);
    UX_TEST_CHECK_SUCCESS(status);

    status = ux_host_class_pima_storage_ids_get(pima_host, &pima_host_session, host_buffer32, 64);
    UX_TEST_CHECK_SUCCESS(status);

    status = ux_host_class_pima_num_objects_get(pima_host, &pima_host_session, 0, UX_HOST_CLASS_PIMA_OFC_UNDEFINED);
    UX_TEST_CHECK_SUCCESS(status);

    status = ux_host_class_pima_object_handles_get(pima_host, &pima_host_session, host_buffer32, 64, 0, UX_HOST_CLASS_PIMA_OFC_UNDEFINED, 0);
    UX_TEST_CHECK_SUCCESS(status);

    status = ux_host_class_pima_object_info_get(pima_host, &pima_host_session, 0, &pima_host_object);
    UX_TEST_CHECK_SUCCESS(status);

    status = ux_host_class_pima_object_open(pima_host, &pima_host_session, 0, &pima_host_object);
    UX_TEST_CHECK_SUCCESS(status);

    status = ux_host_class_pima_object_close(pima_host, &pima_host_session, 0, &pima_host_object);
    UX_TEST_CHECK_SUCCESS(status);

    status = ux_host_class_pima_session_close(pima_host, &pima_host_session);
    UX_TEST_CHECK_SUCCESS(status);
}

void test_disconnect(void)
{
UINT  test, i;
ULONG mem_free = 0xFFFFFFFF;

    for (test = 0; test < 3; test ++)
    {
        stepinfo(">>>>>>>>>>>> Disconnect.%d\n", test);

        /* Disconnect. */
        ux_test_dcd_sim_slave_disconnect();
        ux_test_hcd_sim_host_disconnect();

        UX_TEST_ASSERT_MESSAGE(pima_device == UX_NULL, "Expect device NULL but %p\n", (void*)pima_device);
        UX_TEST_ASSERT_MESSAGE(pima_host == UX_NULL, "Expect host NULL but %p\n", (void*)pima_host);

        if (mem_free == 0xFFFFFFFF)
            mem_free = ux_test_regular_memory_free();
        else
        {
            printf(" %ld <> %ld\n", mem_free, ux_test_regular_memory_free());
            // UX_TEST_ASSERT_MESSAGE(mem_free == ux_test_regular_memory_free(), "Memory leak %ld <> %ld\n", mem_free, ux_test_regular_memory_free());
        }

        stepinfo(">>>>>>>>>>>> Connect.%d\n", test);

        /* Connect. */
        error_callback_counter = 0;
        ux_test_dcd_sim_slave_connect(UX_FULL_SPEED_DEVICE);
        ux_test_hcd_sim_host_connect(UX_FULL_SPEED_DEVICE);

        for (i = 0; i < 100; i ++)
        {
            if (pima_host && pima_device)
                break;
            ux_utility_delay_ms(1);
        }

        UX_TEST_ASSERT_MESSAGE(pima_device != UX_NULL, "Expect device but NULL\n");
        UX_TEST_ASSERT_MESSAGE(pima_host != UX_NULL, "Expect host NULL but NULL\n");
    }
}

void  test_thread_host_simulation_entry(ULONG arg)
{

UINT                                                status;
INT                                                 i;

    for (i = 0; i < 100; i ++)
    {
        if (pima_host && pima_device)
            break;
        ux_utility_delay_ms(1);
    }
    if (pima_host == UX_NULL || pima_device == UX_NULL)
    {
        printf("ERROR #%d\n", __LINE__);
        test_control_return(1);
    }

    test_call_pima_apis();
    test_disconnect();

    stepinfo(">>>>>>>>>>>> All Done\n");

    /* Finally disconnect the device. */
    ux_device_stack_disconnect();

    /* And deinitialize the class.  */
    status =  ux_device_stack_class_unregister(_ux_system_slave_class_pima_name, ux_device_class_pima_entry);

    /* Deinitialize the device side of usbx.  */
    _ux_device_stack_uninitialize();

    /* And finally the usbx system resources.  */
    _ux_system_uninitialize();

    /* Successful test.  */
    printf("SUCCESS!\n");
    test_control_return(0);

}


void  test_thread_device_simulation_entry(ULONG arg)
{

UINT                                                status;

    while(1)
    {

        /* Sleep so ThreadX on Win32 will delete this thread. */
        tx_thread_sleep(10);
    }
}


UINT pima_device_vendor_request(ULONG request, ULONG request_value, ULONG request_index, ULONG request_length,
                       UCHAR *transfer_request_buffer,
                       ULONG *transfer_request_length)
{
UINT    status;
ULONG   length;

    /* Do some sanity check.  The request must be our vendor request. */
    if (request != UX_TEST_VENDOR_REQUEST)

        /* Do not proceed.  */
        return(UX_ERROR);

    /* Check the wIndex value. Values can be :
        0x0001 : Genre
        0x0004 : Extended compatible ID
        0x0005 : Extended properties */
    switch (request_index)
    {

        case    0x0001 :

            /* Not sure what this is for. Windows does not seem to request this. Drop it.  */
            status = UX_ERROR;
            break;

        case    0x0004 :
        case    0x0005 :

            /* Length to return.  */
            length = UX_MIN(0x28, request_length);

            /* Length check.  */
            UX_ASSERT(*transfer_request_length >= length);

            /* At least length should be returned.  */
            if (length < 4)
            {
                status = UX_ERROR;
                break;
            }
            status = UX_SUCCESS;

            /* Return the length.  */
            *transfer_request_length = length;

            /* Reset returned bytes.  */
            ux_utility_memory_set(transfer_request_buffer, 0, length);

            /* Build the descriptor to be returned.  This is not a composite descriptor. Single MTP.
                First dword is length of the descriptor.  */
            ux_utility_long_put(transfer_request_buffer, 0x0028);
            length -= 4;

            /* Then the version. fixed to 0x0100.  */
            if (length < 2)
                break;
            ux_utility_short_put(transfer_request_buffer + 4, 0x0100);
            length -= 2;

            /* Then the descriptor ID. Fixed to 0x0004.  */
            if (length < 2)
                break;
            ux_utility_short_put(transfer_request_buffer + 6, 0x0004);
            length -= 2;

            /* Then the bcount field. Fixed to 0x0001.  */
            if (length < 1)
                break;
            *(transfer_request_buffer + 8) = 0x01;
            length -= 1;

            /* Reset the next 7 bytes.  */
            if (length < 7)
                break;
            ux_utility_memory_set(transfer_request_buffer + 9, 0x00, 7);
            length -= 7;

            /* Last byte of header is the interface number, here 0.  */
            if (length < 1)
                break;
            *(transfer_request_buffer + 16) = 0x00;
            length -= 1;

            /* First byte of descriptor is set to 1.  */
            if (length < 1)
                break;
            *(transfer_request_buffer + 17) = 0x01;
            length -= 1;

            /* Reset the next 8 + 8 + 6 bytes.  */
            if (length < (8+8+6))
                break;
            ux_utility_memory_set(transfer_request_buffer + 18, 0x00, (8 + 8 + 6));
            length -= 8+8+6;

            /* Set the compatible ID to MTP.  */
            if (length < 3)
                break;
            ux_utility_memory_copy(transfer_request_buffer + 18, "MTP", 3);
            length -= 3;

            /* We are done here.  */
            status = UX_SUCCESS;
            break;

        default :
            status = UX_ERROR;
            break;

    }
    /* Return status to device stack.  */
    return(status);
}

UINT    pima_device_device_reset()
{

    /* Do nothing here.  Return Success. */
    return(UX_SUCCESS);

}

UINT pima_device_device_prop_desc_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG device_property,
                                UCHAR **device_prop_dataset,
                                ULONG *device_prop_dataset_length)

{
UINT                            status;

    /* We assume the worst.  */
    status = UX_DEVICE_CLASS_PIMA_RC_DEVICE_PROP_NOT_SUPPORTED;

    /* Check which device property the host is inquiring. It needs
       to be one that we have declared.  */
    switch (device_property)
    {

            case    UX_DEVICE_CLASS_PIMA_DEV_PROP_DATE_TIME :

                /* Buffer length validation.  */
                UX_ASSERT(*device_prop_dataset_length >= DEVICE_PROP_DATE_TIME_DATASET_LENGTH);

                /* The host is inquiring about the date/time dataset.  */
                *device_prop_dataset = pima_device_prop_date_time_dataset;
                *device_prop_dataset_length = DEVICE_PROP_DATE_TIME_DATASET_LENGTH;

                /* We have a successful operation.  */
                status = UX_SUCCESS;

                break;

            case    UX_DEVICE_CLASS_PIMA_DEV_PROP_SYNCHRONIZATION_PARTNER :

                /* Buffer length validation.  */
                UX_ASSERT(*device_prop_dataset_length >= DEVICE_PROP_SYNCHRONIZATION_PARTNER_DATASET_LENGTH);

                /* The host is inquiring about the synchronization partner dataset.  */
                *device_prop_dataset = pima_device_prop_synchronization_partner_dataset;
                *device_prop_dataset_length = DEVICE_PROP_SYNCHRONIZATION_PARTNER_DATASET_LENGTH;

                /* We have a successful operation.  */
                status = UX_SUCCESS;

                break;

            case    UX_DEVICE_CLASS_PIMA_DEV_PROP_DEVICE_FRIENDLY_NAME :

                /* Buffer length validation.  */
                UX_ASSERT(*device_prop_dataset_length >= DEVICE_PROP_DEVICE_FRIENDLY_NAME_DATASET_LENGTH);

                /* The host is inquiring about the device friendly name dataset.  */
                *device_prop_dataset = pima_device_prop_device_friendly_name_dataset;
                *device_prop_dataset_length = DEVICE_PROP_DEVICE_FRIENDLY_NAME_DATASET_LENGTH;

                /* We have a successful operation.  */
                status = UX_SUCCESS;

                break;

            default :

                /* We get here when the Initiator inquires about a property we don't know about.
                   That should never happen as the properties are declared to the initiator. */
                status = UX_DEVICE_CLASS_PIMA_RC_DEVICE_PROP_NOT_SUPPORTED;

                break;
    }

    /* Return what we found.  */
    return(status);

}

UINT pima_device_device_prop_value_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG device_property,
                                UCHAR **device_prop_value,
                                ULONG *device_prop_value_length)
{

UINT                            status;

    /* We assume the worst.  */
    status = UX_DEVICE_CLASS_PIMA_RC_DEVICE_PROP_NOT_SUPPORTED;

    /* Check which device property the host is inquiring. It needs
       to be one that we have declared.  */
    switch (device_property)
    {

            case    UX_DEVICE_CLASS_PIMA_DEV_PROP_DATE_TIME :

                /* Buffer length validation.  */
                UX_ASSERT(*device_prop_value_length >= 33);

                /* The host is inquiring about the date/time value.  */
                *device_prop_value = pima_device_prop_date_time_dataset + 6;
                *device_prop_value_length = 33;

                /* We have a successful operation.  */
                status = UX_SUCCESS;

                break;

            case    UX_DEVICE_CLASS_PIMA_DEV_PROP_SYNCHRONIZATION_PARTNER :

                /* Buffer length validation.  */
                UX_ASSERT(*device_prop_value_length >= 1);

                /* The host is inquiring about the synchronization name dataset.  */
                *device_prop_value = pima_device_prop_synchronization_partner_dataset + 6;
                *device_prop_value_length = 1;

                /* We have a successful operation.  */
                status = UX_SUCCESS;

                break;

            case    UX_DEVICE_CLASS_PIMA_DEV_PROP_DEVICE_FRIENDLY_NAME :

                /* Buffer length validation.  */
                UX_ASSERT(*device_prop_value_length >= 29);

                /* The host is inquiring about the device friendly name dataset.  */
                *device_prop_value = pima_device_prop_device_friendly_name_dataset + 6;
                *device_prop_value_length = 29;

                /* We have a successful operation.  */
                status = UX_SUCCESS;

                break;

            default :

                /* We get here when the Initiator inquires about a property we don't know about.
                   That should never happen as the properties are declared to the initiator. */
                status = UX_DEVICE_CLASS_PIMA_RC_DEVICE_PROP_NOT_SUPPORTED;

                break;
    }

    /* Return what we found.  */
    return(status);

}

UINT pima_device_device_prop_value_set(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG device_property,
                                UCHAR *device_prop_value,
                                ULONG device_prop_value_length)
{

UINT                            status;

    /* We assume the worst.  */
    status = UX_DEVICE_CLASS_PIMA_RC_DEVICE_PROP_NOT_SUPPORTED;

    /* Check which device property the host wants to set. It needs
       to be one that we have declared.  */
    switch (device_property)
    {

            case    UX_DEVICE_CLASS_PIMA_DEV_PROP_DATE_TIME :

                /* The host wants to set time and date value.
                   We only take the first 16 bytes of the Unicode string.  */
                ux_utility_memory_copy (pima_device_prop_date_time_dataset + 6, device_prop_value, 0x10);

                /* We have a successful operation.  */
                status = UX_SUCCESS;

                break;

    }

    /* Return what we found.  */
    return(status);


}


UINT pima_device_storage_format(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG storage_id)
{

UINT                            status;

    /* Check the storage ID.  */
    if (storage_id == UX_TEST_PIMA_STORAGE_ID)
    {

        /* Format the ram drive. */
        status =  fx_media_format(&ram_disk, _fx_ram_driver, ram_disk_memory, ram_disk_buffer, 512, "RAM DISK", 2, 512, 0, UX_TEST_RAM_DISK_SIZE / 512, 512, 4, 1, 1);

        /* Reset the handle counter.  */
        pima_device_object_number_handles = 0;

        /* Is there an error ?  */
        if (status == UX_SUCCESS)

            /* Success. */
            return(status);

        else

            /* Error.  */
            return(UX_DEVICE_CLASS_PIMA_RC_ACCESS_DENIED);
    }
    else

        /* Error, wrong storage ID.  */
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_STORAGE_ID);

}


UINT pima_device_storage_info_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG storage_id)
{

    /* Check the storage ID.  */
    if (storage_id == UX_TEST_PIMA_STORAGE_ID)
    {

        /* We come here when the Initiator needs to update the storage info dataset.
           The PIMA structure has the storage info main dataset. This version only
           support one storage container.  */
        pima -> ux_device_class_pima_storage_max_capacity_low      = ram_disk.fx_media_total_clusters * ram_disk.fx_media_sectors_per_cluster * ram_disk.fx_media_bytes_per_sector;
        pima -> ux_device_class_pima_storage_max_capacity_high     = 0;
        pima -> ux_device_class_pima_storage_free_space_low        = ram_disk.fx_media_available_clusters * ram_disk.fx_media_sectors_per_cluster * ram_disk.fx_media_bytes_per_sector;
        pima -> ux_device_class_pima_storage_free_space_high       = 0;


        /* Success. */
        return( UX_SUCCESS);

    }
    else

        /* Error, wrong storage ID.  */
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_STORAGE_ID);

}

UINT pima_device_object_number_get(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_format_code,
                                ULONG object_association,
                                ULONG *object_number)
{

    /* Return the object number.  */
    *object_number =  pima_device_object_number_handles;

    /* Return success.  */
    return(UX_SUCCESS);
}

UINT pima_device_object_handles_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handles_format_code,
                                ULONG object_handles_association,
                                ULONG *object_handles_array,
                                ULONG object_handles_max_number)
{
ULONG                           handle_index;
ULONG                           number_handles;
ULONG                           found_handles;
ULONG                           *object_handles_array_pointer;

    /* Number of max handles we can store in our demo.  */
    number_handles = UX_TEST_MAX_HANDLES;
    if (number_handles > object_handles_max_number)
        number_handles = object_handles_max_number;

    /* We start with no handles found.  */
    found_handles = 0;

    /* We store the handles in the array pointer, skipping the array count. */
    object_handles_array_pointer = object_handles_array + 1;

    /* Store all the handles we have in the media for the specific format code if utilized.  */
    for (handle_index = 0; handle_index < number_handles; handle_index++)
    {

        /* Check if this handle is valid. If 0, it may have been destroyed or unused yet.  */
        if (pima_device_object_number_handles_array[handle_index] != 0)
        {

            /* This handle is populated. Check the format code supplied by the app.
               if 0 or -1, we discard the format code check.  If not 0 or -1 check
               it the stored object matches the format code.  */
            if ((object_handles_format_code == 0) || (object_handles_format_code == 0xFFFFFFFF) ||
                pima_device_object_info_array[handle_index].ux_device_class_pima_object_format == object_handles_format_code)
            {
                /* We have a candidate.  Store the handle. */
                ux_utility_long_put((UCHAR *) object_handles_array_pointer, pima_device_object_number_handles_array[handle_index]);

                /* Next array container.  */
                object_handles_array_pointer++;

                /* We have found one handle more.  */
                found_handles++;

                /* Check if we are reaching the max array of handles.  */
                if (found_handles == object_handles_max_number)
                {

                    /* Array is saturated. Store what we have found.  */
                    ux_utility_long_put((UCHAR *) object_handles_array, found_handles);

                    /* And return to the Pima class.  */
                    return(UX_SUCCESS);

                }
            }
        }

    }

    /* Array is populated. Store what we have found.  */
    ux_utility_long_put((UCHAR *) object_handles_array, found_handles);

    /* And return to the Pima class.  */
    return(UX_SUCCESS);

}

UINT pima_device_object_info_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handle,
                                UX_SLAVE_CLASS_PIMA_OBJECT **object)
{
UINT                            status;
ULONG                           handle_index;

    /* Check the object handle. It must be in the local array.  */
    status = pima_device_object_handle_check(object_handle, object, &handle_index);

    /* Does the object handle exist ?  */
    if (status == UX_SUCCESS)
    {

        /* Yes, the handle is valid. The object pointer has been updated.  */
        return(UX_SUCCESS);
    }
    else

        /* Done here. Return error.  */
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_HANDLE);

}

UINT pima_device_object_data_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handle,
                                UCHAR *object_buffer,
                                ULONG object_offset,
                                ULONG object_length_requested,
                                ULONG *object_actual_length)

{

UINT                            status;
UINT                            status_close;
ULONG                           handle_index;
UX_SLAVE_CLASS_PIMA_OBJECT      *object;
CHAR                            object_filename[64];


    /* Check the object handle. It must be in the local array.  */
    status = pima_device_object_handle_check(object_handle, &object, &handle_index);

    /* Does the object handle exist ?  */
    if (status != UX_SUCCESS)
        return(status);

    /* Check if entire file is read,
       this could happen if last actual length equals to length requested.  */
    if (object_offset >= pima_device_object_filex_array[handle_index].fx_file_current_file_size)
    {

        /* Nothing to read.  */
        *object_actual_length = 0;
        return(UX_SUCCESS);
    }

    /* We are either at the beginning of the transfer or continuing the transfer.
        Check of the filex array handle exist already.  */
    if (pima_device_object_filex_array[handle_index].fx_file_id == 0)
    {

        /* The object file name is in Unicode, decrypt it first because FileX is not using
        unicode format.  */
        _ux_utility_unicode_to_string(object -> ux_device_class_pima_object_filename, (UCHAR *) object_filename);

        /* File not yet opened for this object.  Open the file. */
        status =  fx_file_open(&ram_disk, &pima_device_object_filex_array[handle_index], (CHAR *) object_filename, FX_OPEN_FOR_READ);

        /* Any problems with the opening ? */
        if (status != UX_SUCCESS)
            return(UX_DEVICE_CLASS_PIMA_RC_OBJECT_NOT_OPENED);

    }

    /* Seek to the offset of the object file.  */
    status =  fx_file_seek(&pima_device_object_filex_array[handle_index], object_offset);
    if (status != UX_SUCCESS)
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_REFERENCE);

    /* Read from the file into the media buffer.  */
    status =   fx_file_read(&pima_device_object_filex_array[handle_index], object_buffer, object_length_requested, object_actual_length);

    /* Check the status.  */
    if (status == UX_SUCCESS)

        /* Done. Operation successful.  */
        status = UX_SUCCESS;

    else
    {

        /* See what the error might be.  */
        switch (status)
        {

            case FX_MEDIA_NOT_OPEN :

                /* Problem with media.  */
                status =  UX_DEVICE_CLASS_PIMA_RC_STORE_NOT_AVAILABLE;
                break;

            case FX_NO_MORE_SPACE :

                /* Media is full.  */
                status =  UX_DEVICE_CLASS_PIMA_RC_STORE_FULL;
                break;

            default :
                status =  UX_DEVICE_CLASS_PIMA_RC_ACCESS_DENIED;
                break;

        }
    }

    /* Check if we have read the entire file.  We compare the current position in the file with the file size. */
    if (pima_device_object_filex_array[handle_index].fx_file_current_file_size == pima_device_object_filex_array[handle_index].fx_file_current_file_offset)
    {

        /* This is the end of the transfer for the object. Close it.  */
        status_close =   fx_file_close(&pima_device_object_filex_array[handle_index]);

        /* FX file id is not cleared by fx_file_close.  */
        pima_device_object_filex_array[handle_index].fx_file_id = 0;

        /* Check the status.  */
        if (status_close == UX_SUCCESS && status == UX_SUCCESS)

            /* Done. Operation successful.  */
            return (UX_SUCCESS);

        else
        {

            /* See what the error might be.  */
            switch (status_close)
            {

                case FX_MEDIA_NOT_OPEN :

                    /* Problem with media.  */
                    status_close =  UX_DEVICE_CLASS_PIMA_RC_STORE_NOT_AVAILABLE;
                    break;

                default :
                    status_close =  UX_DEVICE_CLASS_PIMA_RC_ACCESS_DENIED;
                    break;

            }

            /* If status is error. we return status. If status_close is error we return status_close.  */
            if(status != UX_SUCCESS)

                /* We return the status of read operation.  */
                return(status);

            else

                /* Return status from close operation. */
                return(status_close);

        }
    }

    /* Done here. Return status.  */
    return(status);
}

UINT pima_device_object_info_send (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                UX_SLAVE_CLASS_PIMA_OBJECT *object,
                                ULONG storage_id,
                                ULONG parent_object_handle,
                                ULONG *object_handle)
{
ULONG                           handle_index;
CHAR                            object_filename[64];
UINT                            status;

    /* Make sure we can accommodate a new object here.  */
    if (pima_device_object_number_handles < UX_TEST_MAX_HANDLES)
    {

        /* The object file name is in Unicode, decrypt it first because FileX is not using
           unicode format.  */
        ux_utility_unicode_to_string(object -> ux_device_class_pima_object_filename, (UCHAR *) object_filename);

        /* The object can be either an association object (a directory) or a regular file such
           a photo, music file, video file ... */
        if (object -> ux_device_class_pima_object_format == UX_DEVICE_CLASS_PIMA_OFC_ASSOCIATION)
        {

            /* The object info refers to a association. We treat it as a folder.  */
            status =  fx_directory_create(&ram_disk, object_filename);

            /* Check status. If error, identify the problem and convert FileX error code into PIMA error code.  */
            if (status != UX_SUCCESS)
            {

                /* See what the error might be.  */
                switch (status)
                {

                    case FX_MEDIA_NOT_OPEN :

                        /* Problem with media.  */
                        status =  UX_DEVICE_CLASS_PIMA_RC_STORE_NOT_AVAILABLE;
                        break;

                    case FX_NO_MORE_SPACE :

                        /* Media is full.  */
                        status =  UX_DEVICE_CLASS_PIMA_RC_STORE_FULL;
                        break;

                    case FX_WRITE_PROTECT :

                        /* Media is write protected.  */
                        status =  UX_DEVICE_CLASS_PIMA_RC_STORE_READ_ONLY;
                        break;

                    default :
                        status =  UX_DEVICE_CLASS_PIMA_RC_ACCESS_DENIED;
                        break;

                }

                /* Could not create the object on the local media, return an error.  */
                return (status);
            }
        }
        else
        {
            /* The format is for another object.  */
            /* Create the destination file. */
            status =  fx_file_create(&ram_disk, object_filename);

            /* Check status. If error, identify the problem and convert FileX error code into PIMA error code.  */
            if (status != UX_SUCCESS)
            {

                /* See what the error might be.  */
                switch (status)
                {

                    case FX_MEDIA_NOT_OPEN :

                        /* Problem with media.  */
                        status =  UX_DEVICE_CLASS_PIMA_RC_STORE_NOT_AVAILABLE;
                        break;

                    case FX_NO_MORE_SPACE :

                        /* Media is full.  */
                        status =  UX_DEVICE_CLASS_PIMA_RC_STORE_FULL;
                        break;

                    case FX_WRITE_PROTECT :

                        /* Media is write protected.  */
                        status =  UX_DEVICE_CLASS_PIMA_RC_STORE_READ_ONLY;
                        break;

                    default :
                        status =  UX_DEVICE_CLASS_PIMA_RC_ACCESS_DENIED;
                        break;

                }

                /* Could not create the object on the local media, return an error.  */
                return (status);
            }
        }

        /* The object is created.  Store the object handle. Find a spot. */
        for (handle_index = 0; handle_index < UX_TEST_MAX_HANDLES; handle_index++)
        {

            /* Check for an empty slot.  */
            if (pima_device_object_number_handles_array[handle_index] == 0)
            {

                /* We have found the place to store the handle and the object info. */
                ux_utility_memory_copy(&pima_device_object_info_array[handle_index], object, sizeof(UX_SLAVE_CLASS_PIMA_OBJECT));
                if (pima_device_object_info_array[handle_index].ux_device_class_pima_object_storage_id == 0)
                    pima_device_object_info_array[handle_index].ux_device_class_pima_object_storage_id = storage_id;
                if (pima_device_object_info_array[handle_index].ux_device_class_pima_object_parent_object == 0xFFFFFFFF)
                    pima_device_object_info_array[handle_index].ux_device_class_pima_object_parent_object = 0;

                /* Remember the object handle locally.  */
                pima_device_object_number_handles_array[handle_index] =  handle_index + 1;

                /* Extract from the object the MTP dataset information we need : StorageID.
                   if the storage id in the object info dataset is 0, the Initiator leaves it to the responder to store the object.  */
                if (object -> ux_device_class_pima_object_storage_id != 0)
                    pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_storage_id = object -> ux_device_class_pima_object_storage_id;
                else
                    /* Take the storage ID given as a parameter by the function.  */
                    pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_storage_id = storage_id;

                /* Extract from the object the MTP dataset information we need : Format.  */
                pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_format = object -> ux_device_class_pima_object_format;

                /* Extract from the object the MTP dataset information we need : Protection Status.  */
                pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_protection_status = object -> ux_device_class_pima_object_protection_status;

                /* Extract from the object the MTP dataset information we need : Size.  */
                pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_size_low  = object -> ux_device_class_pima_object_compressed_size;
                pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_size_high = 0;

                /* Extract from the object the MTP dataset information we need : Parent Object.  */
                if (object -> ux_device_class_pima_object_parent_object == 0xFFFFFFFF)
                    pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_parent_object = 0;
                else
                    pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_parent_object  = object -> ux_device_class_pima_object_parent_object;

                /* Keep the file name in ASCIIZ.  */
                ux_utility_unicode_to_string(object -> ux_device_class_pima_object_filename, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_file_name);
                ux_utility_unicode_to_string(object -> ux_device_class_pima_object_filename, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_name);

                /* Keep the unique object identifier for this object. This is incremented each time we have a new object.
                   This number is unique to the MTP device for every object stored, even after being deleted.
                   There is a hack here. We only keep track of the first dword. A full implementation should ensure all 128 bits are used.
                   The identifier is incremented as soon as it is being used.  */
                pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_persistent_unique_object_identifier[0] = pima_device_object_persistent_unique_identifier++;

                /* Return the object handle to the application.  */
                *object_handle = handle_index + 1;

                /* Increment the number of known handles.  */
                pima_device_object_number_handles++;

                /* We are done here.  Return success.  */
                return(UX_SUCCESS);
            }
        }

        /* We should never get here. */
        return(UX_DEVICE_CLASS_PIMA_RC_STORE_FULL);

    }

    /* No more space for handle. Return storage full.  */
    return(UX_DEVICE_CLASS_PIMA_RC_STORE_FULL);

}


UINT pima_device_object_data_send (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handle,
                                ULONG phase ,
                                UCHAR *object_buffer,
                                ULONG object_offset,
                                ULONG object_length)
{

UINT                            status;
ULONG                           handle_index;
UX_SLAVE_CLASS_PIMA_OBJECT      *object;
CHAR                            object_filename[64];

    /* Check the object handle. It must be in the local array.  */
    status = pima_device_object_handle_check(object_handle, &object, &handle_index);

    /* Does the object handle exist ?  */
    if (status == UX_SUCCESS)
    {

        /* Check the phase. Either Active or Complete.  */
        switch (phase)
        {

            case     UX_DEVICE_CLASS_PIMA_OBJECT_TRANSFER_PHASE_ACTIVE        :

                /* We are either at the beginning of the transfer or continuing the transfer.  */
                if (object_offset == 0)
                {

                    /* The object file name is in Unicode, decrypt it first because FileX is not using
                       unicode format.  */
                    ux_utility_unicode_to_string(object -> ux_device_class_pima_object_filename, (UCHAR *) object_filename);

                    /* Open the file on the media since we expect a SendObject.  */
                    status =  fx_file_open(&ram_disk, &pima_device_object_filex_array[handle_index], object_filename, FX_OPEN_FOR_WRITE);
                    if (status != UX_SUCCESS)
                        return(UX_DEVICE_CLASS_PIMA_RC_OBJECT_NOT_OPENED);

                    /* Seek to the beginning of the object file.  */
                    status =  fx_file_seek(&pima_device_object_filex_array[handle_index], 0);
                    if (status != UX_SUCCESS)
                        return(UX_DEVICE_CLASS_PIMA_RC_OBJECT_NOT_OPENED);
                }

                /*   We write the object data to the media.  */
                status =   fx_file_write(&pima_device_object_filex_array[handle_index], object_buffer, object_length);

                /* Check the status.  */
                if (status == UX_SUCCESS)

                    /* Done. Operation successful.  */
                    return (UX_SUCCESS);

                else
                {

                    /* See what the error might be.  */
                    switch (status)
                    {

                        case FX_MEDIA_NOT_OPEN :

                            /* Problem with media.  */
                            status =  UX_DEVICE_CLASS_PIMA_RC_STORE_NOT_AVAILABLE;
                            break;

                        case FX_NO_MORE_SPACE :

                            /* Media is full.  */
                            status =  UX_DEVICE_CLASS_PIMA_RC_STORE_FULL;
                            break;

                        case FX_WRITE_PROTECT :

                            /* Media is write protected.  */
                            status =  UX_DEVICE_CLASS_PIMA_RC_STORE_READ_ONLY;
                            break;

                        default :
                            status =  UX_DEVICE_CLASS_PIMA_RC_ACCESS_DENIED;
                            break;

                    }
                    /* Return the error.  */
                    return(status);

                }
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_TRANSFER_PHASE_COMPLETED    :

                /* Save final object size.  */
                pima_device_object_info_array[handle_index].ux_device_class_pima_object_compressed_size =
                    pima_device_object_filex_array[handle_index].fx_file_current_file_size;

                /* This is the end of the transfer for the object. Close it.  */
                status = fx_file_close(&pima_device_object_filex_array[handle_index]);
                pima_device_object_filex_array[handle_index].fx_file_id = 0;

                /* Check the status.  */
                if (status == UX_SUCCESS)

                    /* Done. Operation successful.  */
                    return (UX_SUCCESS);

                else
                {

                    /* See what the error might be.  */
                    switch (status)
                    {

                        case FX_MEDIA_NOT_OPEN :

                            /* Problem with media.  */
                            status =  UX_DEVICE_CLASS_PIMA_RC_STORE_NOT_AVAILABLE;
                            break;

                        default :
                            status =  UX_DEVICE_CLASS_PIMA_RC_ACCESS_DENIED;
                            break;

                    }
                    /* Return the error.  */
                    return(status);

                }
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_TRANSFER_PHASE_COMPLETED_ERROR    :

                /* Close and delete the object.  */
                pima_device_object_delete(pima, object_handle);

                /* We return OK no matter what.  */
                return(UX_SUCCESS);
        }
    }

    /* Done here. Return error.  */
    return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_HANDLE);

}

UINT pima_device_object_delete(struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_handle)
{
UX_SLAVE_CLASS_PIMA_OBJECT      *object;
ULONG                           handle_index;
CHAR                            object_filename[64];
UINT                            status;

    /* Check the object handle. It must be in the local array.  */
    status = pima_device_object_handle_check(object_handle, &object, &handle_index);

    /* Does the object handle exist ?  */
    if (status == UX_SUCCESS)
    {

        /* The object file name is in Unicode, decrypt it first because FileX is not using
           unicode format.  */
        ux_utility_unicode_to_string(object -> ux_device_class_pima_object_filename, (UCHAR *) object_filename);

        /* Yes, the handle is valid. The object pointer has been updated.  */
        /* The object may still be opened, try to close the handle first.  */
        status =  fx_file_close(&pima_device_object_filex_array[handle_index]);

        /* Delete the destination file. */
        status =  fx_file_delete(&ram_disk, object_filename);

        /* Check if we had an error. */
        if (status != UX_SUCCESS)
        {

            /* See what the error might be.  */
            switch (status)
            {

                case FX_MEDIA_NOT_OPEN :

                    /* Problem with media.  */
                    status =  UX_DEVICE_CLASS_PIMA_RC_STORE_NOT_AVAILABLE;
                    break;

                case FX_WRITE_PROTECT :

                    /* Media is write protected.  */
                    status =  UX_DEVICE_CLASS_PIMA_RC_STORE_READ_ONLY;
                    break;

                default :
                    status =  UX_DEVICE_CLASS_PIMA_RC_ACCESS_DENIED;
                    break;
            }

            /* Return the error code.  */
            return(status);

        }
        else
        {

            /* The object was deleted on disk. Now update the internal application array tables.  */
            pima_device_object_number_handles_array[handle_index] = 0;

            /* Update the number of handles in the system.  */
            pima_device_object_number_handles--;

            /* We are done here.  */
            return(UX_SUCCESS);
        }

    }
    else

        /* Done here. Return error.  */
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_HANDLE);
}


UINT pima_device_object_prop_desc_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                ULONG object_property,
                                ULONG object_format_code,
                                UCHAR **object_prop_dataset,
                                ULONG *object_prop_dataset_length)
{
UINT                            status;
UCHAR                           *object_property_dataset_data;
ULONG                           object_property_dataset_data_length;

    /* Check the object format belongs to the list. 3 categories : generic, audio, video */
    switch (object_format_code)
    {

        case    UX_DEVICE_CLASS_PIMA_OFC_UNDEFINED                            :
        case    UX_DEVICE_CLASS_PIMA_OFC_ASSOCIATION                          :
        case    UX_DEVICE_CLASS_PIMA_OFC_MP3                                  :
        case    UX_DEVICE_CLASS_PIMA_OFC_ASF                                  :
        case    UX_DEVICE_CLASS_PIMA_OFC_WMA                                  :
        case    UX_DEVICE_CLASS_PIMA_OFC_WMV                                  :
        case    UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_AUDIO_ALBUM                 :
        case    UX_DEVICE_CLASS_PIMA_OFC_ABSTRACT_AUDIO_AND_VIDEO_PLAYLIST    :

            /* Set the pointer to the dataset_buffer.  */
            object_property_dataset_data = pima_device_object_property_dataset_data_buffer;

            /* Isolate the property. That will determine the dataset header.  */
            switch (object_property)
            {
                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID            :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID);

                    /* Data type is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT16);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 2, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 6) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 12;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT);

                    /* Data type is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT16);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 3.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 2, 3);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 6) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 12;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS);

                    /* Data type is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT16);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 4.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 2, 4);

                    /* Form Flag ENUM.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 6) = 2;

                    /* Number of elements in Enum array.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 7, 2);

                    /* Elements in Enum array.  Here we store only No protection and Read-Only protection values. This can be extended with
                       Read-only data and Non transferrable data. Spec talks about MTP vendor extension range as well. Not used here.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 0);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 11, 1);

                    /* Set the length.  */
                    object_property_dataset_data_length = 18;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE);

                    /* Data type is UINT64.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT64);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT64.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 0);

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 12) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 18;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME    :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME);

                    /* Data type is STRING.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_STR);

                    /* GetSet value is GETSET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Store a empty Unicode string.   */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 14;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER);

                    /* Data type is UINT128.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT128);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT128.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 0);
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8, 0);
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 12, 0);

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 16, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 20) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 26;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME    :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME);

                    /* Data type is STRING.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_STR);

                    /* GetSet value is GETSET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Store a empty Unicode string.   */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is 2.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 2);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE);

                    /* Data type is UINT8.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT8);

                    /* GetSet value is GET/SET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Default value is UINT8.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is 2.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 2);

                    /* Form Flag ENUM.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 2;

                    /* Number of elements in Enum array.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 6, 2);

                    /* Elements in Enum array.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) =  0;
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9) =  1;

                    /* Set the length.  */
                    object_property_dataset_data_length = 15;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ARTIST                :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ARTIST);

                    /* Data type is STRING.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_STR);

                    /* GetSet value is GETSET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Store a empty Unicode string.   */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_TRACK            :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_TRACK);

                    /* Data type is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT16);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 3.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 2, 3);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 6) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 12;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_USE_COUNT            :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_USE_COUNT);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET/SET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 1.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 14;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DATE_AUTHORED        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DATE_AUTHORED);

                    /* Data type is STRING.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_STR);

                    /* GetSet value is GETSET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Store a empty Unicode string.   */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 0);

                    /* Form Flag is 3.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 3;

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE                :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE);

                    /* Data type is STRING.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_STR);

                    /* GetSet value is GETSET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Store a empty Unicode string.   */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_NAME        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_NAME);

                    /* Data type is STRING.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_STR);

                    /* GetSet value is GETSET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Store a empty Unicode string.   */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_ARTIST        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_ARTIST);

                    /* Data type is STRING.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_STR);

                    /* GetSet value is GETSET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Store a empty Unicode string.   */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SAMPLE_RATE        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SAMPLE_RATE);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 0);

                    /* Form Flag RANGE.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 1;

                    /* Minimum range in array is 0KHZ.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 0x00000000);

                    /* Maximum range in array is KHZ.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 13,0x0002EE00 );

                    /* Range step size is 32HZ.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 17,0x00000020 );

                    /* Set the length.  */
                    object_property_dataset_data_length = 26;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NUMBER_OF_CHANNELS        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NUMBER_OF_CHANNELS);

                    /* Data type is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT16);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 2, 0);

                    /* Form Flag ENUM.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 6) = 2;

                    /* Number of elements in Enum array.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 7, 3);

                    /* Elements in Enum array.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 0);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 11, 1);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 13, 2);

                    /* Set the length.  */
                    object_property_dataset_data_length = 20;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_WAVE_CODEC        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_WAVE_CODEC);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 0);

                    /* Form Flag ENUM.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 2;

                    /* Number of elements in Enum array.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 3);

                    /* Elements in Enum array.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 11, UX_TEST_AUDIO_CODEC_WAVE_FORMAT_MPEGLAYER3);
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 15, UX_TEST_AUDIO_CODEC_WAVE_FORMAT_MPEG);
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 19, UX_TEST_AUDIO_CODEC_WAVE_FORMAT_RAW_AAC1);

                    /* Set the length.  */
                    object_property_dataset_data_length = 28;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_BITRATE        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_BITRATE);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0x0000FA00);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 0);

                    /* Form Flag RANGE.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 1;

                    /* Minimum range in array is 1 bit per second.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 0x00000001);

                    /* Maximum range in array is 1,500,000 bit per second.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 13,0x0016E360 );

                    /* Range step size is 1 bit per second.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 17,0x00000001 );

                    /* Set the length.  */
                    object_property_dataset_data_length = 26;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DURATION            :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DURATION);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 0);

                    /* Form Flag is 1.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 1;

                    /* Set the length.  */
                    object_property_dataset_data_length = 14;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_WIDTH        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_WIDTH);

                    /* Data type is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT16);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0x0000);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 2, 0);

                    /* Form Flag RANGE.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4) = 1;

                    /* Minimum range in array is customer defined.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5, 0x0000);

                    /* Maximum range in array is customer defined.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 7, 0x0000);

                    /* Range step size is customer defined.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 0x0000);

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_HEIGHT        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_HEIGHT);

                    /* Data type is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT16);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0x0000);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 2, 0);

                    /* Form Flag RANGE.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4) = 1;

                    /* Minimum range in array is customer defined.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5, 0x0000);

                    /* Maximum range in array is customer defined.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 7, 0x0000);

                    /* Range step size is customer defined.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 0x0000);

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;


                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SCAN_TYPE        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SCAN_TYPE);

                    /* Data type is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT16);

                    /* GetSet value is GET/SET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Default value is UINT16.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 2, 2);

                    /* Form Flag ENUM.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 2;

                    /* Number of elements in Enum array.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 7, 8);

                    /* Elements in Enum array.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9,  0x0000);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 11, 0x0001);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 13, 0x0002);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 15, 0x0003);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 17, 0x0004);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 19, 0x0005);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 21, 0x0006);
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 23, 0x0007);

                    /* Set the length.  */
                    object_property_dataset_data_length = 29;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_VIDEO_FOURCC_CODEC :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_VIDEO_FOURCC_CODEC);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET/SET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 2);

                    /* Form Flag ENUM.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 2;

                    /* Number of elements in Enum array.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 1);

                    /* Elements in Enum array.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 13,  0x00000000);

                    /* Set the length.  */
                    object_property_dataset_data_length = 22;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_VIDEO_BITRATE :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_VIDEO_BITRATE);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 2);

                    /* Form Flag RANGE.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 1;

                    /* Minimum range in array is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 0x00000000);

                    /* Maximum range in array is 0xFFFFFFFF.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 13,0xFFFFFFFF );

                    /* Range step size is 1.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 17,0x00000001 );

                    /* Set the length.  */
                    object_property_dataset_data_length = 26;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_FRAMES_PER_THOUSAND_SECONDS :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_FRAMES_PER_THOUSAND_SECONDS);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET/SET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 2);

                    /* Form Flag ENUM.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 2;

                    /* Number of elements in Enum array.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 1);

                    /* Elements in Enum array.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 13,  0x00000000);

                    /* Set the length.  */
                    object_property_dataset_data_length = 22;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_KEYFRAME_DISTANCE :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_KEYFRAME_DISTANCE);

                    /* Data type is UINT32.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_UINT32);

                    /* GetSet value is GET/SET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Default value is UINT32.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE, 0);

                    /* Group code is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 4, 2);

                    /* Form Flag RANGE.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 8) = 1;

                    /* Minimum range in array is 0.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 9, 0x00000000);

                    /* Maximum range in array is FFFFFFFF.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 13,0x0000FFFF );

                    /* Range step size is 1.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 17,0x00000001 );

                    /* Set the length.  */
                    object_property_dataset_data_length = 26;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ENCODING_PROFILE        :

                    /* Add the property code.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_CODE, UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ENCODING_PROFILE);

                    /* Data type is STRING.  */
                    ux_utility_short_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_DATATYPE, UX_DEVICE_CLASS_PIMA_TYPES_STR);

                    /* GetSet value is GETSET.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_GETSET) = UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE_GETSET;

                    /* Store a empty Unicode string.   */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE) = 0;

                    /* Group code is NULL.  */
                    ux_utility_long_put(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 1, 0);

                    /* Form Flag is not used.  */
                    *(object_property_dataset_data + UX_DEVICE_CLASS_PIMA_OBJECT_PROPERTY_DATASET_VALUE + 5) = 0;

                    /* Set the length.  */
                    object_property_dataset_data_length = 11;

                    /* We could create this property. */
                    status = UX_SUCCESS;

                    /* Done here.  */
                    break;

                default :

                    /* Error, prop code is not valid.  */
                    status = UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_PROP_CODE;

            }

        /* Check the status of the operation.  */
        if (status == UX_SUCCESS)
        {

            /* The property exist and its dataset created.  Return its pointer to MTP.  */
            *object_prop_dataset = object_property_dataset_data;

            /* And the length of the dataset.  */
            *object_prop_dataset_length = object_property_dataset_data_length;

            /* Done here.  */
            return(UX_SUCCESS);
        }
        else

            /* Done here. Return error.  */
            return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_PROP_CODE);

        break;

        default :

            /* We get here when we have the wrong format code.  */
            return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_FORMAT_CODE);
    }

}


UINT pima_device_object_prop_value_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                    ULONG object_handle,
                                    ULONG object_property,
                                    UCHAR **object_prop_value,
                                    ULONG *object_prop_value_length)
{

ULONG                               handle_index;
UINT                                status;
UCHAR                               *object_property_dataset_data;
ULONG                               object_property_value_data_length;
UX_SLAVE_CLASS_PIMA_OBJECT          *object;

    /* Check the object handle. It must be in the local array.  */
    status = pima_device_object_handle_check(object_handle, &object, &handle_index);

    /* Does the object handle exist ?  */
    if (status == UX_SUCCESS)
    {

        /* Set the pointer to the dataset_buffer.  */
        object_property_dataset_data = pima_device_object_property_dataset_data_buffer;

        /* Isolate the property. That will determine were we fetch the value.  We use the dataset storage area to build the value.  */
        switch (object_property)
        {
            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID            :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_storage_id);

                /* Set the length.  */
                object_property_value_data_length = 4;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT        :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_format);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PROTECTION_STATUS        :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_protection_status);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE        :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data , pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_size_low);
                ux_utility_long_put(object_property_dataset_data  + 4, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_size_high);

                /* Set the length.  */
                object_property_value_data_length = 8;

                /* We could create this property. */
                status = UX_SUCCESS;


                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME    :

                /* Store the file name in unicode format.  */
                ux_utility_string_to_unicode(pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_file_name, object_property_dataset_data);

                /* Set the length.  First Unicode string data.  */
                object_property_value_data_length = (ULONG) *(object_property_dataset_data) * 2 + 1;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT        :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_parent_object);

                /* Set the length.  */
                object_property_value_data_length = 4;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PERSISTENT_UNIQUE_OBJECT_IDENTIFIER        :

                /* Copy the value itself.  */
                ux_utility_memory_copy(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_persistent_unique_object_identifier,16);

                /* Set the length.  */
                object_property_value_data_length = 16;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME    :

                /* Store the file name in unicode format.  */
                ux_utility_string_to_unicode(pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_name, object_property_dataset_data);

                /* Set the length.  First Unicode string data.  */
                object_property_value_data_length = (ULONG) *(object_property_dataset_data) * 2  + 1;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NON_CONSUMABLE        :

                /* Copy the value itself.  */
                *object_property_dataset_data = pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_non_consumable;

                /* Set the length.  */
                object_property_value_data_length = 1;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ARTIST                :

                /* Store the file name in unicode format.  */
                ux_utility_string_to_unicode(pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_artist, object_property_dataset_data);

                /* Set the length.  First Unicode string data.  */
                object_property_value_data_length = (ULONG) *(object_property_dataset_data) * 2  + 1;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_TRACK        :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_track);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_USE_COUNT        :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_use_count);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DATE_AUTHORED        :

                /* Store the file name in unicode format.  */
                ux_utility_string_to_unicode(pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_date_authored, object_property_dataset_data);

                /* Set the length.  First Unicode string data.  */
                object_property_value_data_length = (ULONG) *(object_property_dataset_data) * 2  + 1;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE                :

                /* Store the file name in unicode format.  */
                ux_utility_string_to_unicode(pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_genre, object_property_dataset_data);

                /* Set the length.  First Unicode string data.  */
                object_property_value_data_length = (ULONG) *(object_property_dataset_data) * 2  + 1;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_NAME                :

                /* Store the file name in unicode format.  */
                ux_utility_string_to_unicode(pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_album_name, object_property_dataset_data);

                /* Set the length.  First Unicode string data.  */
                object_property_value_data_length = (ULONG) *(object_property_dataset_data) * 2  + 1;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ALBUM_ARTIST                :

                /* Store the file name in unicode format.  */
                ux_utility_string_to_unicode(pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_album_artist, object_property_dataset_data);

                /* Set the length.  First Unicode string data.  */
                object_property_value_data_length = (ULONG) *(object_property_dataset_data) * 2  + 1;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SAMPLE_RATE        :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_sample_rate);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NUMBER_OF_CHANNELS        :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_number_of_channels);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_WAVE_CODEC        :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_audio_wave_codec);

                /* Set the length.  */
                object_property_value_data_length = 4;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_AUDIO_BITRATE        :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_audio_bitrate);

                /* Set the length.  */
                object_property_value_data_length = 4;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DURATION        :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_duration);

                /* Set the length.  */
                object_property_value_data_length = 4;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_WIDTH            :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_width);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_HEIGHT            :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_height);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_SCAN_TYPE            :

                /* Copy the value itself.  */
                ux_utility_short_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_scan_type);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_VIDEO_FOURCC_CODEC            :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_fourcc_codec);

                /* Set the length.  */
                object_property_value_data_length = 2;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_VIDEO_BITRATE            :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_video_bitrate);

                /* Set the length.  */
                object_property_value_data_length = 4;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_FRAMES_PER_THOUSAND_SECONDS            :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_frames_per_thousand_seconds);

                /* Set the length.  */
                object_property_value_data_length = 4;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_KEYFRAME_DISTANCE            :

                /* Copy the value itself.  */
                ux_utility_long_put(object_property_dataset_data, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_keyframe_distance);

                /* Set the length.  */
                object_property_value_data_length = 4;

                /* We could create this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ENCODING_PROFILE            :

                /* Store the file name in unicode format.  */
                ux_utility_string_to_unicode(pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_encoding_profile, object_property_dataset_data);

                /* Set the length.  First Unicode string data.  */
                object_property_value_data_length = (ULONG) *(object_property_dataset_data) * 2  + 1;

                /* Done here.  */
                break;

            default :

                /* Error, prop code is not valid.  */
                status =  UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_PROP_CODE;

        }

        /* Check the status of the operation.  */
        if (status == UX_SUCCESS)
        {

            /* The property exist and its value created.  Return its pointer to MTP.  */
            *object_prop_value = object_property_dataset_data;

            /* And the length of the dataset.  */
            *object_prop_value_length = object_property_value_data_length;

            /* Done here.  */
            return(UX_SUCCESS);
        }
        else

            /* Done here. Return error.  */
            return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_PROP_CODE);
    }
    else

        /* Done here. Return error.  */
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_HANDLE);
}


UINT pima_device_object_prop_value_set (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                    ULONG object_handle,
                                    ULONG object_property,
                                    UCHAR *object_prop_value,
                                    ULONG object_prop_value_length)
{

ULONG                               handle_index;
UINT                                status;
UX_SLAVE_CLASS_PIMA_OBJECT          *object;

    /* Check the object handle. It must be in the local array.  */
    status = pima_device_object_handle_check(object_handle, &object, &handle_index);

    /* Does the object handle exist ?  */
    if (status == UX_SUCCESS)
    {

        /* Isolate the property.  This is SET. So the properties that are GET only will not be changed.  */
        switch (object_property)
        {
            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_STORAGEID            :
            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FORMAT        :
            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_SIZE        :
            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_PARENT_OBJECT        :
            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DURATION            :

                /* Object is write protected.  */
                status = UX_DEVICE_CLASS_PIMA_RC_OBJECT_WRITE_PROTECTED;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_OBJECT_FILE_NAME    :

                /* Copy the file name after translate from Unicode to ASCIIZ. */
                ux_utility_unicode_to_string(object_prop_value, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_object_file_name);

                /* We could set this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_NAME    :

                /* Copy the name after translate from Unicode to ASCIIZ. */
                ux_utility_unicode_to_string(object_prop_value, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_name);

                /* We could set this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_ARTIST                :

                /* Copy the artist name after translate from Unicode to ASCIIZ. */
                ux_utility_unicode_to_string(object_prop_value, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_artist);

                /* We could set this property. */
                status = UX_SUCCESS;

                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_DATE_AUTHORED        :

                /* Copy the date authored after translate from Unicode to ASCIIZ. */
                ux_utility_unicode_to_string(object_prop_value, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_date_authored);

                /* We could set this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            case    UX_DEVICE_CLASS_PIMA_OBJECT_PROP_GENRE                :

                /* Copy the genre after translate from Unicode to ASCIIZ. */
                ux_utility_unicode_to_string(object_prop_value, pima_device_object_property_array[handle_index].test_pima_object_prop_dataset_genre);

                /* We could set this property. */
                status = UX_SUCCESS;

                /* Done here.  */
                break;

            default :

                /* Error, prop code is not valid.  */
                status = UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_PROP_CODE;
        }


        /* Done here. Return status.  */
        return(status);
    }
    else

        /* Done here. Return error.  */
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_HANDLE);
}


UINT pima_device_object_references_get (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                    ULONG object_handle,
                                    UCHAR **object_references_array,
                                    ULONG *object_references_array_length)
{

ULONG                               handle_index;
UINT                                status;
UX_SLAVE_CLASS_PIMA_OBJECT          *object;
ULONG                               references_array;

    /* Check the object handle. It must be in the local array.  */
    status = pima_device_object_handle_check(object_handle, &object, &handle_index);

    /* Does the object handle exist ?  */
    if (status == UX_SUCCESS)
    {

        /* The property exist. Not sure what to do with references in this release.
           Here we simply create an empty array. */
           references_array = 0;

           /* Return its pointer to MTP.  */
        *object_references_array = (UCHAR *) &references_array;

        /* And the length of the dataset.  */
        *object_references_array_length = sizeof(ULONG);

        /* Done here.  */
        return(UX_SUCCESS);

    }

    else

        /* Done here. Return error.  */
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_HANDLE);
}


UINT pima_device_object_references_set (struct UX_SLAVE_CLASS_PIMA_STRUCT *pima,
                                    ULONG object_handle,
                                    UCHAR *object_references_array,
                                    ULONG object_references_array_length)
{

ULONG                               handle_index;
UINT                                status;
UX_SLAVE_CLASS_PIMA_OBJECT          *object;

    /* Check the object handle. It must be in the local array.  */
    status = pima_device_object_handle_check(object_handle, &object, &handle_index);

    /* Does the object handle exist ?  */
    if (status == UX_SUCCESS)
    {

        /* The property exist. Not sure what to do with references in this release. */

        /* Done here.  */
        return(UX_SUCCESS);

    }

    else

        /* Done here. Return error.  */
        return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_HANDLE);
}


UINT pima_device_object_handle_check(ULONG object_handle,
                                    UX_SLAVE_CLASS_PIMA_OBJECT **object,
                                    ULONG *caller_handle_index)
{
ULONG                               handle_index;

    /* Parse all the handles we have in the media.  */
    for (handle_index = 0; handle_index < UX_TEST_MAX_HANDLES; handle_index++)
    {

        /* Check if we have the correct handle. */
        if (pima_device_object_number_handles_array[handle_index] == object_handle)
        {

            /* We have found the right handle.  Now retrieve its object info dataset. */
            *object = &pima_device_object_info_array[handle_index];

            /* Update the caller index.  */
            *caller_handle_index = handle_index;

            /* We are done here.  Return success.  */
            return(UX_SUCCESS);
        }
    }

    /* We get here when the handle is unknown. Return error.  */
    return(UX_DEVICE_CLASS_PIMA_RC_INVALID_OBJECT_HANDLE);

}
