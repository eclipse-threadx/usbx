/* This is the test control routine of the USBX kernel.  All tests are dispatched from this routine.  */

#include "tx_api.h"
#include "fx_api.h"
#include "nx_api.h"
#include "ux_api.h"
#include <stdio.h>

#include "ux_test.h"
#include "ux_test_hcd_sim_host.h"
#include "ux_test_dcd_sim_slave.h"
#include "ux_test_utility_sim.h"

#define TEST_STACK_SIZE         6144

/* Define the test control USBX objects...  */

TX_THREAD               test_control_thread;
TX_THREAD               test_thread;


/* Define the test control global variables.   */

ULONG           test_control_return_status;
ULONG           test_control_successful_tests;
ULONG           test_control_failed_tests;
ULONG           test_control_system_errors;



/* Remember the start of free memory.  */

UCHAR           *test_free_memory_ptr;


/* Define the function pointer for ISR dispatch.  */

VOID            (*test_isr_dispatch)(void);


UCHAR           test_control_thread_stack[TEST_STACK_SIZE];
UCHAR           tests_memory[1024*1024*1024];

/* Define the external reference for the preempt disable flag.  */

extern volatile UINT   _tx_thread_preempt_disable;
extern volatile ULONG  _tx_thread_system_state;


/* Define test entry pointer type.  */

typedef  struct TEST_ENTRY_STRUCT
{

VOID        (*test_entry)(void *);
} TEST_ENTRY;

/* Demos */

void    usbx_hid_keyboard_key_press_release_demo_application_define(void *);

/* Define the prototypes for the test entry points.  */

void    usbx_dpump_basic_test_application_define(void *);

/* Utility */

void    usbx_ux_utility_descriptor_pack_test_application_define(void *);
void    usbx_ux_utility_descriptor_parse_test_application_define(void *);
void    usbx_ux_utility_event_flags_test_application_define(void *);
void    usbx_ux_utility_memory_safe_test_application_define(void *);
void    usbx_ux_utility_memory_test_application_define(void *);
void    usbx_ux_utility_mutex_test_application_define(void *);
void    usbx_ux_utility_pci_write_test_application_define(void *);
void    usbx_ux_utility_pci_read_test_application_define(void *);
void    usbx_ux_utility_pci_class_scan_test_application_define(void *);
void    usbx_ux_utility_physical_address_test_application_define(void *);
void    usbx_ux_utility_semaphore_test_application_define(void *);
void    usbx_ux_utility_string_length_check_test_application_define(void *);
void    usbx_ux_utility_thread_create_test_application_define(void *);
void    usbx_ux_utility_thread_schedule_other_test_application_define(void *);
void    usbx_ux_utility_thread_suspend_test_application_define(void *);
void    usbx_ux_utility_thread_identify_test_application_define(void *);
void    usbx_ux_utility_timer_test_application_define(void *);
void    usbx_ux_utility_unicode_to_string_test_application_define(void *);

/* General/stack */
void    usbx_host_stack_new_endpoint_create_overage_test_application_define(void*);
void    usbx_host_stack_class_unregister_coverage_test_application_define(void *);
void    usbx_host_device_basic_test_application_define(void *);
void    usbx_host_device_basic_memory_test_application_define(void *);
void    usbx_host_device_initialize_test_application_define(void *);

void    usbx_device_stack_standard_request_test_application_define(void *);
void    usbx_ux_device_stack_class_register_test_application_define(void *);
void    usbx_ux_device_stack_class_unregister_test_application_define(void *);
void    usbx_ux_device_stack_clear_feature_coverage_test_application_define(void *);
void    usbx_ux_host_stack_uninitialize_test_application_define(void *);
void    usbx_ux_host_stack_hcd_unregister_test_application_define(void *);
void    usbx_ux_host_stack_class_unregister_test_application_define(void *);

/* CDC */

void    usbx_cdc_acm_basic_test_original_application_define(void *);
void    usbx_cdc_acm_basic_test_application_define(void *);
void    usbx_cdc_acm_basic_memory_test_application_define(void *);
void    usbx_cdc_acm_configure_test_application_define(void *);
void    usbx_ux_device_class_cdc_acm_activate_test_application_define(void *);
void    usbx_cdc_acm_device_dtr_rts_reset_on_disconnect_test_application_define(void *);
void    usbx_ux_device_class_cdc_acm_deactivate_test_application_define(void *);
void    usbx_ux_device_class_cdc_acm_ioctl_test_application_define(void *);
void    usbx_ux_device_class_cdc_acm_transmission_test_application_define(void *);
void    usbx_ux_device_class_cdc_acm_write_test_application_define(void *);
void    usbx_ux_device_class_cdc_acm_timeout_test_application_define(void *);
void    usbx_ux_host_class_cdc_acm_activate_test_application_define(void *);
void    usbx_ux_host_class_cdc_acm_capabilities_get_test_application_define(void *);
void    usbx_ux_host_class_cdc_acm_deactivate_test_application_define(void *);
void    usbx_ux_host_class_cdc_acm_endpoints_get_test_application_define(void *);
void    usbx_ux_host_class_cdc_acm_entry_test_application_define(void *);
void    usbx_ux_host_class_cdc_acm_read_test_application_define(void *);
void    usbx_ux_host_class_cdc_acm_transfer_request_completed_test_application_define(void *);
void    usbx_ux_device_class_cdc_acm_bulkout_thread_test_application_define(void *);
void    usbx_uxe_device_cdc_acm_test_application_define(void *);

/* HID */

void    usbx_ux_device_class_hid_basic_memory_test_application_define(void *);
void    usbx_hid_keyboard_basic_test_application_define(void *);
void    usbx_ux_host_class_hid_report_get_test_application_define(void *);
void    usbx_hid_report_descriptor_global_item_test_application_define(void *);
void    usbx_hid_report_descriptor_item_size_test_application_define(void *);
void    usbx_hid_report_descriptor_usages_single_test_application_define(void *);
void    usbx_hid_report_descriptor_usages_min_max_test_application_define(void *);
void    usbx_hid_report_descriptor_multiple_fields_test_application_define(void *);
void    usbx_hid_report_descriptor_single_usage_multiple_data_test_application_define(void *);
void    usbx_hid_report_descriptor_multiple_reports_input_test_application_define(void *);
void    usbx_hid_report_descriptor_multiple_reports_output_test_application_define(void *);
void    usbx_hid_report_descriptor_multiple_reports_feature_test_application_define(void *);
void    usbx_hid_report_descriptor_multiple_fields_and_reports_test(void *);
void    usbx_hid_report_descriptor_previous_report_test_application_define(void *);
void    usbx_hid_report_descriptor_push_pop_test_application_define(void *);
void    usbx_hid_report_descriptor_decompress_test_application_define(void *);
void    usbx_hid_report_descriptor_decompress_array_test_application_define(void *);
void    usbx_hid_report_descriptor_delimiter_test_application_define(void *);
void    usbx_hid_report_descriptor_global_item_persist_test_application_define(void *);
void    usbx_hid_report_descriptor_extended_usage_test_application_define(void *);
void    usbx_hid_report_descriptor_multiple_collections_test_application_define(void *);
void    usbx_hid_report_descriptor_usages_overflow_test_application_define(void *);
void    usbx_hid_report_descriptor_usages_overflow_via_max_test_application_define(void *);
void    usbx_hid_report_descriptor_collection_overflow_test_application_define(void *);
void    usbx_hid_report_descriptor_end_collection_error_test_application_define(void *);
void    usbx_hid_report_descriptor_example_andisplay_test_application_define(void *);
void    usbx_hid_report_descriptor_example_delimit_test_application_define(void *);
void    usbx_hid_report_descriptor_example_digit_test_application_define(void *);
void    usbx_hid_report_descriptor_example_display_test_application_define(void *);
void    usbx_hid_report_descriptor_example_joystk_test_application_define(void *);
void    usbx_hid_report_descriptor_example_keybrd_test_application_define(void *);
void    usbx_hid_report_descriptor_example_monitor_test_application_define(void *);
void    usbx_hid_report_descriptor_example_mouse_test_application_define(void *);
void    usbx_hid_report_descriptor_example_pwr_test_application_define(void *);
void    usbx_hid_report_descriptor_example_remote_test_application_define(void *);
void    usbx_hid_report_descriptor_example_tele_test_application_define(void *);
void    usbx_hid_report_descriptor_get_zero_length_item_data_test_application_define(void *);
void    usbx_hid_report_descriptor_invalid_length_test_application_define(void *);
void    usbx_hid_report_descriptor_invalid_item_test_application_define(void *);
void    usbx_hid_report_descriptor_delimiter_nested_open_test_application_define(void *);
void    usbx_hid_report_descriptor_delimiter_nested_close_test_application_define(void *);
void    usbx_hid_report_descriptor_delimiter_unknown_test_application_define(void *);
void    usbx_hid_report_descriptor_unknown_local_tag_test_application_define(void *);
void    usbx_hid_report_descriptor_unknown_global_tag_test_application_define(void *);
void    usbx_hid_report_descriptor_incoherent_usage_min_max_test_application_define(void *);
void    usbx_hid_report_descriptor_report_size_overflow_test_application_define(void *);
void    usbx_hid_report_descriptor_report_count_overflow_test_application_define(void *);
void    usbx_hid_report_descriptor_push_overflow_tag_test_application_define(void *);
void    usbx_hid_report_descriptor_pop_underflow_tag_test_application_define(void *);
void    usbx_hid_keyboard_extraction_test_application_define(void *);
void    usbx_hid_keyboard_extraction_test2_application_define(void *);
void    usbx_hid_keyboard_callback_test_application_define(void *);
void    usbx_hid_mouse_basic_test_application_define(void *);
void    usbx_hid_mouse_extraction_test_application_define(void *);
void    usbx_hid_mouse_extraction_test2_application_define(void *);
void    usbx_hid_remote_control_tests_application_define(void *);
void    usbx_ux_host_class_hid_remote_control_entry_test2_application_define(void *);
void    usbx_ux_host_class_hid_remote_control_activate_test_application_define(void *);
void    usbx_hid_remote_control_extraction_test_application_define(void *);
void    usbx_hid_remote_control_extraction_test2_application_define(void *);
void    usbx_hid_report_descriptor_compress_test_application_define(void *);
void    usbx_hid_report_descriptor_compress_and_decompress_test_application_define(void *);
void    usbx_hid_transfer_request_completed_test_application_define(void *);
void    usbx_hid_transfer_request_completed_raw_test_application_define(void *);
void    usbx_hid_transfer_request_completed_decompressed_test_application_define(void *);
void    usbx_ux_host_class_hid_client_register_test_application_define(void *);
void    usbx_ux_host_class_hid_report_set_test_application_define(void *);
void    usbx_hid_keyboard_key_get_test_application_define(void *);
void    usbx_ux_host_class_hid_client_search_test_application_define(void *);
void    usbx_ux_device_class_hid_control_request_test_application_define(void *);
void    usbx_ux_device_class_hid_report_set_test_application_define(void *);
void    usbx_ux_device_class_hid_interrupt_thread_test_application_define(void *);
void    usbx_hid_keyboard_key_test_application_define(void *);
void    usbx_hid_keyboard_key_with_report_id_test_application_define(void *);
void    usbx_ux_device_class_hid_descriptor_send_test_application_define(void *);
void    usbx_control_transfer_stall_test(void *);
void    usbx_hid_interrupt_endpoint_get_report_test_application_define(void *);
void    usbx_ux_device_class_hid_event_get_AND_set_test_application_define(void *);
void    usbx_ux_host_class_hid_report_add_test_application_define(void *);
void    usbx_ux_host_class_hid_idle_set_test_application_define(void *);
void    usbx_ux_host_class_hid_interrupt_endpoint_search_test_application_define(void *);
void    usbx_ux_host_class_hid_keyboard_activate_test_application_define(void *);
void    usbx_ux_host_class_hid_report_id_get_test_application_define(void *);
void    usbx_ux_host_class_hid_periodic_report_start_test_application_define(void *);
void    usbx_ux_host_class_hid_descriptor_parse_coverage_test_application_define(void *);
void    usbx_ux_host_class_hid_descriptor_parse_test_application_define(void *);
void    usbx_ux_host_class_hid_descriptor_parse_test2_application_define(void *);
void    usbx_ux_host_class_hid_mouse_activate_test_application_define(void *);
void    usbx_ux_host_class_hid_activate_test_application_define(void *);
void    usbx_ux_host_class_hid_report_callback_register_test_application_define(void *);
void    usbx_ux_host_class_hid_entry_test_application_define(void *);
void    usbx_ux_host_class_hid_periodic_report_stop_test_application_define(void *);
void    usbx_ux_host_class_hid_keyboard_entry_test_application_define(void *);
void    usbx_ux_host_class_hid_keyboard_ioctl_test_application_define(void *);
void    usbx_ux_host_class_hid_mouse_entry_test_application_define(void *);
void    usbx_ux_host_class_hid_mouse_positions_get_test_application_define(void *);
void    usbx_ux_host_class_hid_mouse_buttons_get_test_application_define(void *);
void    usbx_ux_host_class_hid_mouse_wheel_get_test_application_define(void *);
void    usbx_ux_host_class_hid_remote_control_entry_test_application_define(void *);
void    usbx_ux_host_class_hid_remote_control_usage_get_test_application_define(void *);
void    usbx_ux_host_class_hid_configure_test_application_define(void *);
void    usbx_ux_device_class_hid_entry_test_application_define(void *);
void    usbx_ux_device_class_hid_deactivate_test_application_define(void *);
void    usbx_ux_device_class_hid_activate_test_application_define(void *);
void    usbx_ux_host_class_hid_keyboard_thread_test_application_define(void *);
void    usbx_ux_host_class_hid_keyboard_thread_test2_application_define(void *);
void    usbx_ux_host_class_hid_deactivate_test_application_define(void *);
void    usbx_ux_host_class_hid_deactivate_test2_application_define(void *);
void    usbx_ux_host_class_hid_descriptor_parse_test3_application_define(void *);
void    usbx_ux_host_class_hid_descriptor_parse_test4_application_define(void *);
void    usbx_ux_host_class_hid_descriptor_parse_test5_application_define(void *);
void    usbx_ux_host_class_hid_descriptor_parse_test6_application_define(void *);
void    usbx_ux_host_class_hid_descriptor_parse_test7_application_define(void *);
void    usbx_ux_host_class_hid_report_descriptor_get_test_application_define(void *);
void    usbx_ux_host_class_hid_remote_control_callback_test_application_define(void *);
void    usbx_ux_host_class_hid_interrupt_endpoint_search_test2_application_define(void *);
void    usbx_ux_host_class_hid_periodic_report_start_test2_application_define(void *);
void    usbx_ux_host_class_hid_report_get_test2_application_define(void *);
void    usbx_ux_host_class_hid_main_item_parse_test_application_define(void *);
void    usbx_ux_host_class_hid_main_item_parse_test2_application_define(void *);
void    usbx_ux_host_class_hid_mouse_entry_test3_application_define(void *);
void    usbx_ux_host_class_hid_mouse_entry_test2_application_define(void *);
void    usbx_ux_host_class_hid_remote_control_entry_test3_application_define(void *);
void    usbx_ux_host_class_hid_transfer_request_completed_test_application_define(void *);
void    usbx_ux_host_class_hid_keyboard_callback_test2_application_define(void *);
void    usbx_ux_host_class_hid_idle_get_test_application_define(void *);
void    usbx_ux_host_class_hid_remote_control_activate_test2_application_define(void *);
void    usbx_ux_host_class_hid_client_register_test2_application_define(void *);
void    usbx_ux_host_class_hid_report_descriptor_get_test_application_define(void *);
void    usbx_ux_host_class_hid_report_descriptor_get_test2_application_define(void *);
void    usbx_ux_host_class_hid_report_descriptor_get_test3_application_define(void *);
void    usbx_ux_host_class_hid_report_descriptor_get_test4_application_define(void *);
void    usbx_ux_host_class_hid_deactivate_test3_application_define(void *);
void    usbx_ux_host_class_hid_local_item_parse_test_application_define(void *);
void    usbx_ux_device_class_hid_uninitialize_test_application_define(void *);
void    usbx_ux_device_class_hid_initialize_test_application_define(void *);
void    usbx_ux_device_class_hid_report_set_test2_application_define(void *);
void    usbx_ux_device_class_hid_activate_test2_application_define(void *);
void    usbx_ux_device_class_hid_activate_test3_application_define(void *);
void    usbx_ux_device_class_hid_interrupt_thread_test2_application_define(void *);
void    usbx_ux_device_class_hid_report_test_application_define(void *);
void    usbx_hid_report_descriptor_compress_array_test_application_define(void *);
void    usbx_ux_device_class_hid_idle_rate_test_application_define(void *);


/* Storage */
void    usbx_host_class_storage_entry_overage_test_application_define(void *);
void    usbx_host_class_storage_max_lun_get_coverage_test_application_define(void *);
void    usbx_storage_basic_memory_test_application_define(void *);
void    usbx_storage_tests_application_define(void *);
void    usbx_storage_multi_lun_test_application_define(void *);
void    usbx_storage_direct_calls_test_application_define(void *);
void    usbx_ux_device_class_storage_vendor_strings_test_application_define(void *);
void    usbx_ux_device_class_storage_test_ready_test_application_define(void *);
void    usbx_ux_device_class_storage_control_request_test_application_define(void *);
void    usbx_ux_device_class_storage_entry_test_application_define(void *);
void    usbx_ux_device_class_storage_format_test_application_define(void *);
void    usbx_ux_device_class_storage_mode_select_test_application_define(void *);
void    usbx_ux_device_class_storage_mode_sense_test_application_define(void *);
void    usbx_ux_device_class_storage_request_sense_test_application_define(void *);
void    usbx_ux_device_class_storage_start_stop_test_application_define(void *);
void    usbx_ux_device_class_storage_prevent_allow_media_removal_test_application_define(void *);
void    usbx_ux_device_class_storage_verify_test_application_define(void *);
void    usbx_ux_device_class_storage_uninitialize_test_application_define(void *);
void    usbx_ux_device_class_storage_inquiry_test_application_define(void *);
void    usbx_ux_device_class_storage_initialize_test_application_define(void *);
void    usbx_ux_device_class_storage_synchronize_cache_test_application_define(void *);
void    usbx_ux_device_class_storage_read_test_application_define(void *);
void    usbx_ux_device_class_storage_write_test_application_define(void *);
void    usbx_ux_device_class_storage_thread_test_application_define(void *);
void    usbx_ux_device_class_storage_request_sense_coverage_test_application_define(void *);
void    usbx_ux_host_class_storage_configure_overage_test_application_define(void *);
void    usbx_ux_host_class_storage_request_sense_test_application_define(void *);
void    usbx_ux_host_class_storage_media_capacity_get_test_application_define(void *);
void    usbx_ux_host_class_storage_max_lun_get_test_application_define(void *);
void    usbx_ux_host_class_storage_configure_test_application_define(void *);
void    usbx_ux_host_class_storage_activate_test_application_define(void *);
void    usbx_ux_host_class_storage_device_support_check_test_application_define(void *);
void    usbx_ux_host_class_storage_device_initialize_test_application_define(void *);
void    usbx_ux_host_class_storage_media_get_test_application_define(void *);
void    usbx_ux_host_class_storage_media_mount_test_application_define(void *);
void    usbx_ux_host_class_storage_media_open_test_application_define(void *);
void    usbx_ux_host_class_storage_media_read_test_application_define(void *);
void    usbx_ux_host_class_storage_media_write_test_application_define(void *);
void    usbx_ux_host_class_storage_media_protection_check_test_application_define(void *);
void    usbx_ux_host_class_storage_media_recovery_sense_get_test_application_define(void *);
void    usbx_ux_host_class_storage_start_stop_test_application_define(void *);
void    usbx_ux_host_class_storage_transport_bo_test_application_define(void *);
void    usbx_ux_host_class_storage_driver_entry_test_application_define(void *);
void    usbx_ux_host_class_storage_entry_test_application_define(void *);

/* RNDIS */

void    usbx_rndis_basic_test_application_define(void *);

/* Host stack */
void    usbx_ux_host_class_stack_device_configuration_reset_coverage_test_application_define(void *);
void    usbx_ux_host_stack_bandwidth_test_application_define(void *);
void    usbx_ux_host_stack_class_device_scan_test_application_define(void *);
void    usbx_ux_host_stack_class_get_test_application_define(void *);
void    usbx_ux_host_stack_class_instance_destroy_test_application_define(void *);
void    usbx_ux_host_stack_class_instance_get_test_application_define(void *);
void    usbx_ux_host_stack_class_instance_verify_test_application_define(void *);
void    usbx_ux_host_stack_class_interface_scan_test_application_define(void *);
void    usbx_ux_host_stack_class_register_test_application_define(void *);
void    usbx_ux_host_stack_configuration_descriptor_parse_test_application_define(void *);
void    usbx_ux_host_stack_configuration_enumerate_test_application_define(void *);
void    usbx_ux_host_stack_configuration_instance_delete_test_application_define(void *);
void    usbx_ux_host_stack_configuration_interface_get_test_application_define(void *);
void    usbx_ux_host_stack_configuration_set_test_application_define(void *);
void    usbx_ux_host_stack_device_address_set_test_application_define(void *);
void    usbx_ux_host_stack_device_get_test_application_define(void *);
void    usbx_ux_host_stack_device_remove_test_application_define(void *);
void    usbx_ux_host_stack_endpoint_instance_create_test_application_define(void *);
void    usbx_ux_host_stack_endpoint_instance_test_application_define(void *);
void    usbx_ux_host_stack_endpoint_reset_test_application_define(void *);
void    usbx_ux_host_stack_hcd_transfer_request_test_application_define(void *);
void    usbx_ux_host_stack_hcd_register_test_application_define(void *);
void    usbx_ux_host_stack_interface_endpoint_get_test_application_define(void *);
void    usbx_ux_host_stack_interface_setting_select_test_application_define(void *);
void    usbx_ux_host_stack_interfaces_scan_test_application_define(void *);
void    usbx_ux_host_stack_new_configuration_create_test_application_define(void *);
void    usbx_ux_host_stack_new_device_get_test_application_define(void *);
void    usbx_ux_host_stack_new_device_create_test_application_define(void *);
void    usbx_ux_host_stack_new_interface_create_test_application_define(void *);
void    usbx_ux_host_stack_rh_change_process_test_application_define(void *);
void    usbx_ux_host_stack_rh_device_insertion_test_application_define(void *);
void    usbx_ux_host_stack_device_configuration_get_test_application_define(void *);
void    usbx_ux_host_stack_device_configuration_reset_select_test_application_define(void *);
void    usbx_ux_host_stack_hcd_thread_entry_test_application_define(void *);
void    usbx_ux_host_stack_transfer_request_test_application_define(void *);
void    usbx_ux_host_class_storage_thread_entry_test_application_define(void *);
//void    usbx_ux_host_stack_transfer_request_abort_test_application_define(void *);

/* Device Stack */
void    usbx_ux_device_stack_alternate_setting_get_test_application_define(void *);
void    usbx_ux_device_stack_alternate_setting_set_test_application_define(void *);
void    usbx_ux_device_stack_configuration_set_test_application_define(void *);
void    usbx_ux_device_stack_control_request_process_coverage_test_application_define(void *);
void    usbx_ux_device_stack_control_request_process_test_application_define(void *);
void    usbx_ux_device_stack_descriptor_send_test_application_define(void *);
void    usbx_ux_host_stack_device_descriptor_read_test_application_define(void *);
void    usbx_ux_device_stack_get_status_test_application_define(void *);
void    usbx_ux_device_stack_interface_delete_test_application_define(void *);
void    usbx_ux_device_stack_interface_set_test_application_define(void *);
void    usbx_ux_device_stack_interface_start_test_application_define(void *);
void    usbx_ux_device_stack_remote_wakeup_test_application_define(void *);
void    usbx_ux_device_stack_set_feature_test_application_define(void *);
void    usbx_ux_device_stack_transfer_request_test_application_define(void *);
void    usbx_ux_device_stack_endpoint_stall_test_application_define(void *);
void    usbx_ux_device_stack_initialize_test_application_define(void *);

/* CDC-ECM */

void    usbx_cdc_ecm_basic_test_application_define(void *);
void    usbx_cdc_ecm_basic_ipv6_test_application_define(void *);
void    usbx_cdc_ecm_disconnect_and_reconnect_test_application_define(void *);
void    usbx_cdc_ecm_alternate_setting_change_to_zero_test_application_define(void *);
void    usbx_ux_host_class_cdc_ecm_transmission_callback_test_application_define(void *);
void    usbx_cdc_ecm_host_bulk_out_transfer_arming_during_link_down_test_application_define(void *);
void    usbx_cdc_ecm_host_bulk_out_transfer_fail_test_application_define(void *);
void    usbx_ux_host_class_cdc_ecm_write_test_application_define(void *);
void    usbx_cdc_ecm_host_bulk_in_transfer_arming_fails_due_to_link_down_and_thread_waiting_test_application_define(void *);
void    usbx_cdc_ecm_host_non_ip_packet_received_test_application_define(void *);
void    usbx_cdc_ecm_host_bulk_in_transfer_fail_test_application_define(void *);
void    usbx_cdc_ecm_host_thread_link_down_before_transfer_test_application_define(void *);
void    usbx_cdc_ecm_host_thread_packet_allocate_fail_test_application_define(void *);
void    usbx_cdc_ecm_mac_address_test_application_define(void *);
void    usbx_cdc_ecm_mac_address_invalid_length_test_application_define(void *);
void    usbx_cdc_ecm_no_functional_descriptor_test_application_define(void *);
void    usbx_ux_host_class_cdc_ecm_mac_address_get_test_application_define(void *);
void    usbx_cdc_ecm_host_bulk_in_transfer_arming_during_link_down_test_application_define(void *);
void    usbx_ux_host_class_cdc_ecm_interrupt_notification_test_application_define(void *);
void    usbx_cdc_ecm_link_down_while_ongoing_transfers_test_application_define(void *);
void    usbx_ux_host_class_cdc_ecm_entry_test_application_define(void *);
void    usbx_cdc_ecm_host_bulk_in_transfer_arming_during_deactivate_test_application_define(void *);
void    usbx_cdc_ecm_host_first_interrupt_transfer_fail_test_application_define(void *);
void    usbx_cdc_ecm_host_packet_pool_create_fail_test_application_define(void *);
void    usbx_cdc_ecm_host_thread_create_fail_test_application_define(void *);
void    usbx_cdc_ecm_host_interrupt_notification_semaphore_create_fail_test_application_define(void *);
void    usbx_cdc_ecm_host_bulk_out_semaphore_create_fail_test_application_define(void *);
void    usbx_cdc_ecm_host_bulk_in_semaphore_create_fail_test_application_define(void *);
void    usbx_cdc_ecm_control_interface_no_interrupt_endpoint_test_application_define(void *);
void    usbx_cdc_ecm_data_interface_no_bulk_in_endpoint_test_application_define(void *);
void    usbx_cdc_ecm_data_interface_non_bulk_out_and_non_bulk_in_endpoint_test_application_define(void *);
void    usbx_cdc_ecm_data_interface_no_bulk_out_endpoint_test_application_define(void *);
void    usbx_cdc_ecm_data_interface_setting_select_fails_test_application_define(void *);
void    usbx_cdc_ecm_invalid_alternate_setting_after_zero_endpoint_data_interface_test_application_define(void *);
void    usbx_cdc_ecm_non_data_interface_after_zero_endpoint_data_interface_test_application_define(void *);
void    usbx_cdc_ecm_one_data_interface_with_no_endpoints_test_application_define(void *);
void    usbx_cdc_ecm_no_control_interface_test_application_define(void *);
void    usbx_cdc_ecm_interface_before_control_interface_test_application_define(void *);
void    usbx_cdc_ecm_basic_memory_test_application_define(void *);
void    usbx_ux_host_class_cdc_ecm_activate_test_application_define(void *);
void    usbx_cdc_ecm_default_data_interface_with_endpoints_test_application_define(void *);
void    usbx_ux_host_class_cdc_ecm_write_test_application_define(void *);
void    usbx_ux_host_class_cdc_ecm_interrupt_notification_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_uninitialize_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_deactivate_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_initialize_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_activate_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_interrupt_thread_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_bulkin_thread_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_bulkout_thread_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_control_request_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_change_test_application_define(void *);
void    usbx_ux_device_class_cdc_ecm_entry_test_application_define(void *);

/* Hub */
void    usbx_host_class_hub_port_change_connection_process_coverage_test_application_define(void *);
void    usbx_hub_basic_test_application_define(void *);
void    usbx_hub_basic_memory_test_application_define(void *);
void    usbx_hub_get_status_fails_during_configuration_test_application_define(void *);
void    usbx_bus_powered_hub_connected_to_self_and_bus_powered_hub_test_application_define(void *);
void    usbx_hub_invalid_hub_descriptor_length_test_application_define(void *);
void    usbx_hub_full_speed_hub_test_application_define(void *);
void    usbx_hub_multiple_tt_test_application_define(void *);
void    usbx_hub_invalid_device_protocol_test_application_define(void *);
void    usbx_ux_host_class_hub_descriptor_get_coverage_test_application_define(void *);
void    usbx_ux_host_class_hub_entry_test_application_define(void *);
void    usbx_hub_request_to_hub_itself_test_application_define(void *);
void    usbx_hub_no_endpoints_test_application_define(void *);
void    usbx_hub_interrupt_out_endpoint_test_application_define(void *);
void    usbx_hub_non_interrupt_in_endpoint_test_application_define(void *);
void    usbx_hub_hub_device_disconnect_test_application_define(void *);
void    usbx_hub_quick_hub_device_reconnection_test_application_define(void *);
void    usbx_hub_hub_device_enumeration_keeps_failing_test_application_define(void *);
void    usbx_hub_port_reset_fails_during_hub_device_enumeration_test_application_define(void *);
void    usbx_hub_get_port_status_fails_during_hub_device_enumeration_test_application_define(void *);
void    usbx_hub_low_speed_hub_device_test_application_define(void *);
void    usbx_hub_full_speed_hub_device_test_application_define(void *);
void    usbx_hub_quick_hub_device_disconnection_test_application_define(void *);
void    usbx_hub_port_change_enable_test_application_define(void *);
void    usbx_hub_get_hub_status_fails_during_hub_device_enumeration_test_application_define(void *);
void    usbx_hub_port_change_suspend_test_application_define(void *);
void    usbx_hub_port_change_over_current_test_application_define(void *);
void    usbx_hub_port_change_reset_test_application_define(void *);
void    usbx_hub_port_reset_fails_due_to_unset_port_enabled_bit_test_application_define(void *);
void    usbx_hub_get_hub_status_fails_during_port_reset_test_application_define(void *);
void    usbx_hub_no_power_switching_test_application_define(void *);
void    usbx_ux_host_class_hub_status_get_test_application_define(void *);
void    usbx_hub_hub_status_get_invalid_length_test_application_define(void *);
void    usbx_ux_host_class_hub_transfer_request_completed_test_application_define(void *);

/* Pictbridge */

void usbx_pictbridge_basic_test_application_define(void *);

/* Audio */

void    usbx_audio10_device_basic_test_application_define(void *);
void    usbx_audio10_iad_device_basic_test_application_define(void *);
void    usbx_audio10_iad_device_control_test_application_define(void *);
void    usbx_audio20_device_basic_test_application_define(void *);
void    usbx_uxe_device_audio_test_application_define(void *);

/* Video */

void    usbx_host_class_video_basic_test_application_define(void *);

/* Printer */

void    usbx_host_class_printer_basic_test_application_define(void *);
void    usbx_uxe_device_printer_test_application_define(void *);


/* CTest application define  */

void test_application_define(void *first_unused_memory);

/* Define the array of test entry points.  */

TEST_ENTRY  test_control_tests[] =
{
#ifdef CTEST
    test_application_define,
#else

    /* MSC */
    // usbx_storage_tests_application_define,
    usbx_host_class_storage_entry_overage_test_application_define,
    usbx_host_class_storage_max_lun_get_coverage_test_application_define,

    usbx_host_stack_new_endpoint_create_overage_test_application_define,
    usbx_host_stack_class_unregister_coverage_test_application_define,    
    usbx_storage_basic_memory_test_application_define,
    usbx_storage_multi_lun_test_application_define,
    usbx_ux_device_class_storage_request_sense_coverage_test_application_define,
    usbx_ux_device_class_storage_vendor_strings_test_application_define,
    usbx_ux_device_class_storage_test_ready_test_application_define,
    usbx_ux_device_class_storage_control_request_test_application_define,
    usbx_ux_device_class_storage_entry_test_application_define,
    usbx_ux_device_class_storage_format_test_application_define,
    usbx_ux_device_class_storage_mode_select_test_application_define,
    usbx_ux_device_class_storage_mode_sense_test_application_define,
    usbx_ux_device_class_storage_request_sense_test_application_define,
    usbx_ux_device_class_storage_start_stop_test_application_define,
    usbx_ux_device_class_storage_prevent_allow_media_removal_test_application_define,
    usbx_ux_device_class_storage_verify_test_application_define,
    usbx_ux_device_class_storage_uninitialize_test_application_define,
    usbx_ux_device_class_storage_inquiry_test_application_define,
    usbx_ux_device_class_storage_initialize_test_application_define,
    usbx_ux_device_class_storage_synchronize_cache_test_application_define,
    usbx_ux_device_class_storage_read_test_application_define,

    usbx_ux_device_class_storage_write_test_application_define,
    usbx_ux_device_class_storage_thread_test_application_define,
    usbx_ux_host_class_storage_configure_overage_test_application_define, 
    usbx_ux_host_class_storage_request_sense_test_application_define,
    usbx_ux_host_class_storage_media_capacity_get_test_application_define,
    usbx_ux_host_class_storage_max_lun_get_test_application_define,
    usbx_ux_host_class_storage_configure_test_application_define,
    usbx_ux_host_class_storage_activate_test_application_define,
    usbx_ux_host_class_storage_device_support_check_test_application_define,
    usbx_ux_host_class_storage_device_initialize_test_application_define,
    usbx_ux_host_class_storage_media_mount_test_application_define,
    usbx_ux_host_class_storage_media_open_test_application_define,
    usbx_ux_host_class_storage_media_read_test_application_define,
    usbx_ux_host_class_storage_media_write_test_application_define,
    usbx_ux_host_class_storage_media_protection_check_test_application_define,
    usbx_ux_host_class_storage_media_recovery_sense_get_test_application_define,
    usbx_ux_host_class_storage_start_stop_test_application_define,
    usbx_ux_host_class_storage_transport_bo_test_application_define,
    usbx_ux_host_class_storage_driver_entry_test_application_define,
    usbx_ux_host_class_storage_thread_entry_test_application_define,
    usbx_ux_host_class_storage_entry_test_application_define,

    /* Utility */

    usbx_ux_utility_descriptor_pack_test_application_define,
    usbx_ux_utility_descriptor_parse_test_application_define,
    usbx_ux_utility_event_flags_test_application_define,
    usbx_ux_utility_memory_safe_test_application_define,
    usbx_ux_utility_memory_test_application_define,
    usbx_ux_utility_mutex_test_application_define,
    usbx_ux_utility_pci_write_test_application_define,
    usbx_ux_utility_pci_read_test_application_define,
    usbx_ux_utility_pci_class_scan_test_application_define,
    usbx_ux_utility_physical_address_test_application_define,
    usbx_ux_utility_semaphore_test_application_define,
    usbx_ux_utility_string_length_check_test_application_define,
    usbx_ux_utility_thread_create_test_application_define,
    usbx_ux_utility_thread_schedule_other_test_application_define,
    usbx_ux_utility_thread_suspend_test_application_define,
    usbx_ux_utility_thread_identify_test_application_define,
    usbx_ux_utility_timer_test_application_define,
    usbx_ux_utility_unicode_to_string_test_application_define,

    /* Host stack */

    usbx_ux_host_stack_uninitialize_test_application_define,
    usbx_ux_host_stack_hcd_unregister_test_application_define,
    usbx_ux_host_stack_class_unregister_test_application_define,

    //usbx_ux_host_stack_transfer_request_abort_test_application_define,
    usbx_ux_host_class_stack_device_configuration_reset_coverage_test_application_define,
    usbx_ux_host_stack_bandwidth_test_application_define,
    usbx_ux_host_stack_class_device_scan_test_application_define,
    usbx_ux_host_stack_class_get_test_application_define,
    usbx_ux_host_stack_class_instance_destroy_test_application_define,
    usbx_ux_host_stack_class_instance_get_test_application_define,
    usbx_ux_host_stack_class_interface_scan_test_application_define,
    usbx_ux_host_stack_class_register_test_application_define,
    usbx_ux_host_stack_configuration_descriptor_parse_test_application_define,
    usbx_ux_host_stack_configuration_enumerate_test_application_define,
    usbx_ux_host_stack_configuration_instance_delete_test_application_define,
    usbx_ux_host_stack_configuration_interface_get_test_application_define,
    usbx_ux_host_stack_configuration_set_test_application_define,
    usbx_ux_host_stack_device_address_set_test_application_define,
    usbx_ux_host_stack_device_get_test_application_define,
    usbx_ux_host_stack_device_remove_test_application_define,
    usbx_ux_host_stack_endpoint_instance_create_test_application_define,
    usbx_ux_host_stack_endpoint_instance_test_application_define,
    usbx_ux_host_stack_hcd_transfer_request_test_application_define,
    usbx_ux_host_stack_hcd_register_test_application_define,
    usbx_ux_host_stack_interface_endpoint_get_test_application_define,
    usbx_ux_host_stack_interfaces_scan_test_application_define,
    usbx_ux_host_stack_new_device_get_test_application_define,
    usbx_ux_host_stack_rh_device_insertion_test_application_define,
    usbx_ux_host_stack_device_configuration_get_test_application_define,
    usbx_ux_host_stack_device_configuration_reset_select_test_application_define,
    usbx_ux_host_stack_hcd_thread_entry_test_application_define,
    usbx_ux_host_stack_transfer_request_test_application_define,

    /* Device Stack */

    usbx_ux_device_stack_alternate_setting_get_test_application_define,
    usbx_ux_device_stack_alternate_setting_set_test_application_define,
    usbx_ux_device_stack_class_register_test_application_define,
    usbx_ux_device_stack_class_unregister_test_application_define,
    usbx_ux_device_stack_configuration_set_test_application_define,
    usbx_ux_device_stack_control_request_process_coverage_test_application_define,
    usbx_ux_device_stack_control_request_process_test_application_define,
    usbx_ux_host_stack_device_descriptor_read_test_application_define,
    usbx_ux_device_stack_get_status_test_application_define,
    usbx_ux_device_stack_interface_delete_test_application_define,
    usbx_ux_device_stack_interface_set_test_application_define,
    usbx_ux_device_stack_interface_start_test_application_define,
    usbx_ux_device_stack_set_feature_test_application_define,
    usbx_ux_device_stack_transfer_request_test_application_define,
    usbx_ux_device_stack_endpoint_stall_test_application_define,
    usbx_ux_device_stack_initialize_test_application_define,
    usbx_ux_device_stack_clear_feature_coverage_test_application_define,

#if !defined(UX_DEVICE_STANDALONE) && !defined(UX_HOST_STANDALONE)

    /* MSC */
    usbx_storage_tests_application_define,

    /* Stack (with CDC ACM)  */
    usbx_ux_host_stack_class_instance_verify_test_application_define,
    usbx_ux_host_stack_endpoint_reset_test_application_define,
    usbx_ux_host_stack_interface_setting_select_test_application_define,
    usbx_ux_host_stack_new_configuration_create_test_application_define,
    usbx_ux_host_stack_new_device_create_test_application_define,
    usbx_ux_host_stack_new_interface_create_test_application_define,
    usbx_ux_host_stack_rh_change_process_test_application_define,

    usbx_device_stack_standard_request_test_application_define,
    usbx_ux_device_stack_descriptor_send_test_application_define,
    usbx_ux_device_stack_remote_wakeup_test_application_define,

    /* Audio */
    usbx_audio10_device_basic_test_application_define,
    usbx_audio10_iad_device_basic_test_application_define,
    usbx_audio10_iad_device_control_test_application_define,
    usbx_audio20_device_basic_test_application_define,
    usbx_uxe_device_audio_test_application_define,

    /* Hub */
    usbx_host_class_hub_port_change_connection_process_coverage_test_application_define,
    usbx_ux_host_class_hub_transfer_request_completed_test_application_define,
    usbx_hub_hub_status_get_invalid_length_test_application_define,
    usbx_ux_host_class_hub_status_get_test_application_define,
    usbx_hub_no_power_switching_test_application_define,
    usbx_hub_get_hub_status_fails_during_port_reset_test_application_define,
    usbx_hub_port_reset_fails_due_to_unset_port_enabled_bit_test_application_define,
    usbx_hub_port_change_reset_test_application_define,
    usbx_hub_port_change_over_current_test_application_define,
    usbx_hub_port_change_suspend_test_application_define,
    usbx_hub_get_hub_status_fails_during_hub_device_enumeration_test_application_define,
    usbx_hub_port_change_enable_test_application_define,
    usbx_hub_quick_hub_device_disconnection_test_application_define,
    usbx_hub_full_speed_hub_device_test_application_define,
    usbx_hub_low_speed_hub_device_test_application_define,
    usbx_hub_port_reset_fails_during_hub_device_enumeration_test_application_define,
    usbx_hub_get_port_status_fails_during_hub_device_enumeration_test_application_define,
    usbx_hub_hub_device_enumeration_keeps_failing_test_application_define,
    usbx_hub_quick_hub_device_reconnection_test_application_define,
    usbx_hub_hub_device_disconnect_test_application_define,
    usbx_hub_non_interrupt_in_endpoint_test_application_define,
    usbx_hub_interrupt_out_endpoint_test_application_define,
    usbx_hub_no_endpoints_test_application_define,
    usbx_hub_request_to_hub_itself_test_application_define,
    usbx_ux_host_class_hub_descriptor_get_coverage_test_application_define,
    usbx_ux_host_class_hub_entry_test_application_define,
    usbx_hub_invalid_device_protocol_test_application_define,
    usbx_hub_basic_test_application_define,
    usbx_hub_basic_memory_test_application_define,
    usbx_hub_get_status_fails_during_configuration_test_application_define,
    usbx_bus_powered_hub_connected_to_self_and_bus_powered_hub_test_application_define,
    usbx_hub_invalid_hub_descriptor_length_test_application_define,
    usbx_hub_full_speed_hub_test_application_define,
    usbx_hub_multiple_tt_test_application_define,

    /* CDC-ECM */

    usbx_cdc_ecm_disconnect_and_reconnect_test_application_define,
    usbx_cdc_ecm_alternate_setting_change_to_zero_test_application_define,
    usbx_ux_host_class_cdc_ecm_interrupt_notification_test_application_define,
    usbx_ux_host_class_cdc_ecm_transmission_callback_test_application_define,
    usbx_cdc_ecm_host_bulk_out_transfer_arming_during_link_down_test_application_define,
    usbx_cdc_ecm_host_bulk_out_transfer_fail_test_application_define,
    usbx_ux_host_class_cdc_ecm_write_test_application_define,
    usbx_cdc_ecm_host_bulk_in_transfer_arming_fails_due_to_link_down_and_thread_waiting_test_application_define,
    usbx_cdc_ecm_host_non_ip_packet_received_test_application_define,
    usbx_cdc_ecm_host_bulk_in_transfer_fail_test_application_define,
    usbx_cdc_ecm_host_thread_packet_allocate_fail_test_application_define,
    usbx_cdc_ecm_host_thread_link_down_before_transfer_test_application_define,
    usbx_cdc_ecm_host_packet_pool_create_fail_test_application_define,
    usbx_cdc_ecm_mac_address_test_application_define,
    usbx_cdc_ecm_basic_test_application_define,
    usbx_cdc_ecm_basic_ipv6_test_application_define,
    usbx_cdc_ecm_mac_address_invalid_length_test_application_define,
    usbx_cdc_ecm_no_functional_descriptor_test_application_define,
    usbx_ux_host_class_cdc_ecm_mac_address_get_test_application_define,
    usbx_cdc_ecm_host_bulk_in_transfer_arming_during_link_down_test_application_define,
    usbx_ux_host_class_cdc_ecm_entry_test_application_define,
    usbx_cdc_ecm_host_bulk_in_transfer_arming_during_deactivate_test_application_define,
    usbx_cdc_ecm_host_first_interrupt_transfer_fail_test_application_define,
    usbx_cdc_ecm_host_thread_create_fail_test_application_define,
    usbx_cdc_ecm_host_interrupt_notification_semaphore_create_fail_test_application_define,
    usbx_cdc_ecm_host_bulk_out_semaphore_create_fail_test_application_define,
    usbx_cdc_ecm_host_bulk_in_semaphore_create_fail_test_application_define,
    usbx_cdc_ecm_control_interface_no_interrupt_endpoint_test_application_define,
    usbx_cdc_ecm_data_interface_setting_select_fails_test_application_define,
    usbx_cdc_ecm_data_interface_no_bulk_in_endpoint_test_application_define,
    usbx_cdc_ecm_data_interface_non_bulk_out_and_non_bulk_in_endpoint_test_application_define,
    usbx_cdc_ecm_data_interface_no_bulk_out_endpoint_test_application_define,
    usbx_cdc_ecm_one_data_interface_with_no_endpoints_test_application_define,
    usbx_cdc_ecm_invalid_alternate_setting_after_zero_endpoint_data_interface_test_application_define,
    usbx_cdc_ecm_non_data_interface_after_zero_endpoint_data_interface_test_application_define,
    usbx_cdc_ecm_default_data_interface_with_endpoints_test_application_define,
    usbx_cdc_ecm_no_control_interface_test_application_define,
    usbx_cdc_ecm_interface_before_control_interface_test_application_define,
    usbx_cdc_ecm_basic_memory_test_application_define,
    usbx_ux_device_class_cdc_ecm_uninitialize_test_application_define,
    usbx_ux_device_class_cdc_ecm_deactivate_test_application_define,
    usbx_ux_device_class_cdc_ecm_initialize_test_application_define,
    usbx_ux_device_class_cdc_ecm_activate_test_application_define,
    usbx_ux_device_class_cdc_ecm_interrupt_thread_test_application_define,
    usbx_ux_device_class_cdc_ecm_bulkin_thread_test_application_define,
    usbx_ux_device_class_cdc_ecm_bulkout_thread_test_application_define,
    usbx_ux_device_class_cdc_ecm_control_request_test_application_define,
    usbx_ux_device_class_cdc_ecm_change_test_application_define,
    usbx_ux_device_class_cdc_ecm_entry_test_application_define,
    usbx_cdc_ecm_link_down_while_ongoing_transfers_test_application_define,

    /* RNDIS */

    usbx_rndis_basic_test_application_define,

    /* CDC-ACM. */

    usbx_cdc_acm_basic_test_application_define,
    usbx_cdc_acm_basic_memory_test_application_define,
    usbx_cdc_acm_configure_test_application_define,
    usbx_cdc_acm_device_dtr_rts_reset_on_disconnect_test_application_define,
    usbx_ux_device_class_cdc_acm_activate_test_application_define,
    usbx_ux_device_class_cdc_acm_deactivate_test_application_define,
    usbx_ux_device_class_cdc_acm_ioctl_test_application_define,
    usbx_ux_device_class_cdc_acm_transmission_test_application_define,
    usbx_ux_device_class_cdc_acm_write_test_application_define,
    usbx_ux_device_class_cdc_acm_timeout_test_application_define,
    usbx_ux_host_class_cdc_acm_activate_test_application_define,
    usbx_ux_host_class_cdc_acm_capabilities_get_test_application_define,
    usbx_ux_host_class_cdc_acm_deactivate_test_application_define,
    usbx_ux_host_class_cdc_acm_endpoints_get_test_application_define,
    usbx_ux_host_class_cdc_acm_entry_test_application_define,
    usbx_ux_host_class_cdc_acm_read_test_application_define,
    usbx_ux_host_class_cdc_acm_transfer_request_completed_test_application_define,
    usbx_ux_device_class_cdc_acm_bulkout_thread_test_application_define,
    usbx_uxe_device_cdc_acm_test_application_define,

    /* DPUMP. */

    usbx_dpump_basic_test_application_define,

    /* Printer */

    usbx_host_class_printer_basic_test_application_define,
    usbx_uxe_device_printer_test_application_define,

    /* Host & Device basic. */

    usbx_host_device_basic_test_application_define,
    usbx_host_device_basic_memory_test_application_define,
    usbx_host_device_initialize_test_application_define,

    /* HID */

    usbx_hid_report_descriptor_compress_array_test_application_define,
    usbx_hid_remote_control_tests_application_define,
    usbx_ux_host_class_hid_remote_control_entry_test_application_define,
    usbx_ux_host_class_hid_remote_control_entry_test2_application_define,
    usbx_ux_host_class_hid_remote_control_entry_test3_application_define,
    usbx_ux_host_class_hid_remote_control_usage_get_test_application_define,
    usbx_hid_keyboard_basic_test_application_define,
    usbx_hid_keyboard_key_test_application_define,
    usbx_ux_device_class_hid_activate_test_application_define,
    usbx_ux_device_class_hid_control_request_test_application_define,
    usbx_ux_device_class_hid_deactivate_test_application_define,
    usbx_ux_device_class_hid_descriptor_send_test_application_define,
    usbx_ux_device_class_hid_entry_test_application_define,
    usbx_ux_device_class_hid_event_get_AND_set_test_application_define,
    usbx_ux_device_class_hid_initialize_test_application_define,
    usbx_ux_device_class_hid_interrupt_thread_test_application_define,
    usbx_ux_device_class_hid_interrupt_thread_test2_application_define,
    usbx_ux_device_class_hid_report_test_application_define,
    usbx_ux_host_class_hid_activate_test_application_define,
    usbx_ux_host_class_hid_client_register_test_application_define,
    usbx_ux_host_class_hid_client_register_test2_application_define,
    usbx_ux_host_class_hid_client_search_test_application_define,
    usbx_ux_host_class_hid_configure_test_application_define,
    usbx_ux_host_class_hid_deactivate_test_application_define,
    usbx_ux_host_class_hid_deactivate_test3_application_define,
    usbx_ux_host_class_hid_descriptor_parse_coverage_test_application_define,
    usbx_ux_host_class_hid_descriptor_parse_test_application_define,
    usbx_ux_host_class_hid_descriptor_parse_test2_application_define,
    usbx_ux_host_class_hid_descriptor_parse_test4_application_define,
    usbx_ux_host_class_hid_descriptor_parse_test5_application_define,
    usbx_ux_host_class_hid_entry_test_application_define,
    usbx_ux_host_class_hid_idle_get_test_application_define,
    usbx_ux_host_class_hid_idle_set_test_application_define,
    usbx_ux_host_class_hid_keyboard_activate_test_application_define,
    usbx_ux_host_class_hid_keyboard_entry_test_application_define,
    usbx_ux_host_class_hid_keyboard_ioctl_test_application_define,
    usbx_ux_host_class_hid_keyboard_thread_test_application_define,
    usbx_ux_host_class_hid_keyboard_thread_test2_application_define,
    usbx_ux_host_class_hid_main_item_parse_test_application_define,
    usbx_ux_host_class_hid_main_item_parse_test2_application_define,
    usbx_ux_host_class_hid_mouse_activate_test_application_define,
    usbx_ux_host_class_hid_mouse_entry_test_application_define,
    usbx_ux_host_class_hid_mouse_entry_test3_application_define,
    usbx_ux_host_class_hid_mouse_buttons_get_test_application_define,
    usbx_ux_host_class_hid_mouse_positions_get_test_application_define,
    usbx_ux_host_class_hid_mouse_wheel_get_test_application_define,
    usbx_ux_host_class_hid_periodic_report_start_test_application_define,
    usbx_ux_host_class_hid_periodic_report_start_test2_application_define,
    usbx_ux_host_class_hid_periodic_report_stop_test_application_define,
    usbx_ux_host_class_hid_remote_control_activate_test_application_define,
    usbx_ux_host_class_hid_report_add_test_application_define,
    usbx_ux_host_class_hid_report_callback_register_test_application_define,
    usbx_ux_host_class_hid_report_get_test_application_define,
    usbx_ux_host_class_hid_report_get_test2_application_define,
    usbx_ux_host_class_hid_report_id_get_test_application_define,
    usbx_ux_host_class_hid_report_set_test_application_define,
    usbx_ux_host_class_hid_transfer_request_completed_test_application_define,
    usbx_hid_keyboard_key_get_test_application_define,
    usbx_hid_transfer_request_completed_decompressed_test_application_define,
    usbx_hid_transfer_request_completed_test_application_define,
    usbx_hid_transfer_request_completed_raw_test_application_define,
    usbx_hid_report_descriptor_compress_test_application_define,
    usbx_hid_report_descriptor_compress_and_decompress_test_application_define,
    usbx_hid_report_descriptor_get_zero_length_item_data_test_application_define,
    usbx_hid_report_descriptor_global_item_test_application_define,
    usbx_hid_report_descriptor_global_item_persist_test_application_define,
    usbx_hid_report_descriptor_report_size_overflow_test_application_define,
    usbx_hid_report_descriptor_report_count_overflow_test_application_define,
    usbx_hid_report_descriptor_pop_underflow_tag_test_application_define,
    usbx_hid_report_descriptor_push_overflow_tag_test_application_define,
    usbx_hid_report_descriptor_push_pop_test_application_define,
    usbx_hid_report_descriptor_unknown_global_tag_test_application_define,
    usbx_hid_report_descriptor_end_collection_error_test_application_define,
    usbx_hid_report_descriptor_collection_overflow_test_application_define,
    usbx_hid_report_descriptor_multiple_collections_test_application_define,
    usbx_hid_report_descriptor_unknown_local_tag_test_application_define,
    usbx_hid_report_descriptor_incoherent_usage_min_max_test_application_define,
    usbx_hid_report_descriptor_delimiter_unknown_test_application_define,
    usbx_hid_report_descriptor_delimiter_nested_close_test_application_define,
    usbx_hid_report_descriptor_delimiter_nested_open_test_application_define,
    usbx_hid_report_descriptor_invalid_length_test_application_define,
    usbx_hid_report_descriptor_invalid_item_test_application_define,
    usbx_ux_host_class_hid_local_item_parse_test_application_define,
    usbx_hid_mouse_basic_test_application_define,
    usbx_ux_device_class_hid_basic_memory_test_application_define,
    usbx_ux_device_class_hid_idle_rate_test_application_define,

    usbx_host_class_video_basic_test_application_define,

#endif /* !defined(UX_DEVICE_STANDALONE) && !defined(UX_HOST_STANDALONE) */
#endif /* CTEST */
    TX_NULL,
};

/* Define thread prototypes.  */

void  test_control_thread_entry(ULONG thread_input);
void  test_thread_entry(ULONG thread_input);
void  test_control_return(UINT status);
void  test_control_cleanup(void);


/* Define necessary external references.  */

#ifdef __ghs
extern TX_MUTEX                 __ghLockMutex;
#endif

extern TX_TIMER                 *_tx_timer_created_ptr;
extern ULONG                    _tx_timer_created_count;
#ifndef TX_TIMER_PROCESS_IN_ISR
extern TX_THREAD                _tx_timer_thread;
#endif
extern TX_THREAD                *_tx_thread_created_ptr;
extern ULONG                    _tx_thread_created_count;
extern TX_SEMAPHORE             *_tx_semaphore_created_ptr;
extern ULONG                    _tx_semaphore_created_count;
extern TX_QUEUE                 *_tx_queue_created_ptr;
extern ULONG                    _tx_queue_created_count;
extern TX_MUTEX                 *_tx_mutex_created_ptr;
extern ULONG                    _tx_mutex_created_count;
extern TX_EVENT_FLAGS_GROUP     *_tx_event_flags_created_ptr;
extern ULONG                    _tx_event_flags_created_count;
extern TX_BYTE_POOL             *_tx_byte_pool_created_ptr;
extern ULONG                    _tx_byte_pool_created_count;
extern TX_BLOCK_POOL            *_tx_block_pool_created_ptr;
extern ULONG                    _tx_block_pool_created_count;

extern NX_PACKET_POOL *         _nx_packet_pool_created_ptr;
extern ULONG                    _nx_packet_pool_created_count;
extern NX_IP *                  _nx_ip_created_ptr;
extern ULONG                    _nx_ip_created_count;

#ifdef EXTERNAL_EXIT
void external_exit(UINT code);
#endif


/* Define the interrupt processing dispatcher.  The individual tests will set this up when they desire
   asynchrony processing for testing purposes.  */

void test_interrupt_dispatch(void)
{

#ifndef TX_DISABLE_ERROR_CHECKING

    /* Test calling tx_thread_relinquish from ISR to see if the error checking throws it out.  */
    tx_thread_relinquish();
#endif

    /* Check for something to run... */
    if (test_isr_dispatch)
    {

        (test_isr_dispatch)();
    }
}


/* Define init timer entry.  */

static void   init_timer_entry(ULONG timer_input)
{

}


/* Define main entry point.  */
#ifndef EXTERNAL_MAIN
void main()
{

    /* Enter the USBX kernel.  */
    tx_kernel_enter();
}
#endif


/* Define what the initial system looks like.  */

void    tx_application_define(void *first_unused_memory)
{

    /* Initialize the test error/success counters.  */
    test_control_successful_tests =  0;
    test_control_failed_tests =      0;
    test_control_system_errors =     0;

    /* Create the test control thread.  */
    tx_thread_create(&test_control_thread, "test control thread", test_control_thread_entry, 0,
            test_control_thread_stack, TEST_STACK_SIZE,
            17, 15, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Remember the free memory pointer.  */
    test_free_memory_ptr =  &tests_memory[0];
}



/* Define the test control thread.  This thread is responsible for dispatching all of the
   tests in the USBX test suite.  */

void  test_control_thread_entry(ULONG thread_input)
{

ULONG   previous_test_control_failed_tests;
ULONG   previous_thread_created_count;
UINT    i;

    /* Raise the priority of the control thread to 0.  */
    tx_thread_priority_change(&test_control_thread, 0, &i);

    /* Print out banner.  */
    printf("********************** USBX Validation/Regression Test Suite *********************************\n\n");

    /* Print version id.  */
    printf("Version: %s Data width: x%i\n\n", _ux_version_id, (int)sizeof(void*) * 8);

    /* Print out the tests... */
    printf("Running validation/regression test:\n\n");

    /* Loop to process all tests...  */
    i =  0;
    while (test_control_tests[i].test_entry != TX_NULL)
    {

        /* Clear the ISR dispatch.  */
        test_isr_dispatch =  TX_NULL;

        /* Save the number of failed tests for comparison. */
        previous_test_control_failed_tests = test_control_failed_tests;

        /* Save previous thread count.  */
        previous_thread_created_count = _tx_thread_created_count;

        /* Dispatch the test.  */
        (test_control_tests[i++].test_entry)(test_free_memory_ptr);

        /* Clear the ISR dispatch.  */
        test_isr_dispatch =  TX_NULL;

        /* Did the test entry run successfully? */
        if (test_control_failed_tests == previous_test_control_failed_tests &&
            previous_thread_created_count != _tx_thread_created_count)

            /* Suspend control test to allow test to run.  */
            tx_thread_suspend(&test_control_thread);

        /* Test finished, cleanup in preparation for the next test.  */
        test_control_cleanup();
    }

    /* Finished with all tests, print results and return!  */
    printf("**** Testing Complete ****\n");
    printf("**** Test Summary:  Tests Passed:  %lu   Tests Failed:  %lu    System Errors:   %lu\n", test_control_successful_tests, test_control_failed_tests, test_control_system_errors);

#ifndef EXTERNAL_EXIT
    exit(test_control_failed_tests);
#else
    external_exit(0);
#endif
}

static VOID disable_test_actions(VOID)
{

    /* Note: There shouldn't be any actions left. */
    ux_test_cleanup_everything();

    ux_test_utility_sim_cleanup();
    ux_test_hcd_sim_host_cleanup();
    ux_test_dcd_sim_slave_cleanup();
}


void  test_control_return(UINT status)
{

UINT    old_posture =  TX_INT_ENABLE;

    disable_test_actions();

    /* Save the status in a global.  */
    test_control_return_status =  status;

    /* Ensure interrupts are enabled.  */
    old_posture =  tx_interrupt_control(TX_INT_ENABLE);

    /* Determine if it was successful or not.  */
    if (status)
        test_control_failed_tests++;
    else
        test_control_successful_tests++;

    /* Now check for system errors.  */

    /* Is preempt disable flag set?  */
    if (_tx_thread_preempt_disable)
    {

        /* System error - preempt disable should never be set inside of a thread!  */
        printf("    ***** SYSTEM ERROR ***** _tx_thread_preempt_disable is non-zero!\n");
        test_control_system_errors++;
    }

    /* Is system state set?  */
    if (_tx_thread_system_state)
    {

        /* System error - system state should never be set inside of a thread!  */
        printf("    ***** SYSTEM ERROR ***** _tx_thread_system_state is non-zero!\n");
        test_control_system_errors++;
    }

    /* Are interrupts disabled?  */
    if (old_posture == TX_INT_DISABLE)
    {

        /* System error - interrupts should always be enabled in our test threads!  */
        printf("    ***** SYSTEM ERROR ***** test returned with interrupts disabled!\n");
        test_control_system_errors++;
    }

    /* Resume the control thread to fully exit the test.  */
    tx_thread_resume(&test_control_thread);
}


void  test_control_cleanup(void)
{

TX_MUTEX    *mutex_ptr;
TX_THREAD   *thread_ptr;


    /* FIXME: Cleanup of FileX, and USBX resources should be added here... and perhaps some checks for memory leaks, etc.  */

    /* Disable testing actions generation */
    disable_test_actions();

    /* Delete all IP instances.   */
    while (_nx_ip_created_ptr)
    {

        /* Delete all UDP sockets.  */
        while (_nx_ip_created_ptr -> nx_ip_udp_created_sockets_ptr)
        {

            /* Make sure the UDP socket is unbound.  */
            nx_udp_socket_unbind(_nx_ip_created_ptr -> nx_ip_udp_created_sockets_ptr);

            /* Delete the UDP socket.  */
            nx_udp_socket_delete(_nx_ip_created_ptr -> nx_ip_udp_created_sockets_ptr);
        }

        /* Delete all TCP sockets.  */
        while (_nx_ip_created_ptr -> nx_ip_tcp_created_sockets_ptr)
        {

            /* When disconnecting TCP sockets, NetX sends a RST packet. Since the link
               is down, the driver will report an error. So, turn of errors during this
               time.  */
            ux_test_ignore_all_errors();

            /* Disconnect.  */
            nx_tcp_socket_disconnect(_nx_ip_created_ptr -> nx_ip_tcp_created_sockets_ptr, NX_NO_WAIT);

            /* Re-enable errors.  */
            ux_test_unignore_all_errors();

            /* Make sure the TCP client socket is unbound.  */
            nx_tcp_client_socket_unbind(_nx_ip_created_ptr -> nx_ip_tcp_created_sockets_ptr);

            /* Make sure the TCP server socket is unaccepted.  */
            nx_tcp_server_socket_unaccept(_nx_ip_created_ptr -> nx_ip_tcp_created_sockets_ptr);

            /* Delete the TCP socket.  */
            nx_tcp_socket_delete(_nx_ip_created_ptr -> nx_ip_tcp_created_sockets_ptr);
        }

        /* Clear all listen requests.  */
        while (_nx_ip_created_ptr -> nx_ip_tcp_active_listen_requests)
        {

            /* Make sure the TCP server socket is unlistened.  */
            nx_tcp_server_socket_unlisten(_nx_ip_created_ptr, (_nx_ip_created_ptr -> nx_ip_tcp_active_listen_requests) -> nx_tcp_listen_port);
        }

        /* Delete the IP instance.  */
        nx_ip_delete(_nx_ip_created_ptr);
    }

    /* Delete all the packet pools.  */
    while (_nx_packet_pool_created_ptr)
    {
        nx_packet_pool_delete(_nx_packet_pool_created_ptr);
    }

    /* Delete all queues.  */
    while(_tx_queue_created_ptr)
    {

        /* Delete queue.  */
        tx_queue_delete(_tx_queue_created_ptr);
    }

    /* Delete all semaphores.  */
    while(_tx_semaphore_created_ptr)
    {

        /* Delete semaphore.  */
        tx_semaphore_delete(_tx_semaphore_created_ptr);
    }

    /* Delete all event flag groups.  */
    while(_tx_event_flags_created_ptr)
    {

        /* Delete event flag group.  */
        tx_event_flags_delete(_tx_event_flags_created_ptr);
    }

    /* Delete all byte pools.  */
    while(_tx_byte_pool_created_ptr)
    {

        /* Delete byte pool.  */
        tx_byte_pool_delete(_tx_byte_pool_created_ptr);
    }

    /* Delete all block pools.  */
    while(_tx_block_pool_created_ptr)
    {

        /* Delete block pool.  */
        tx_block_pool_delete(_tx_block_pool_created_ptr);
    }

    /* Delete all timers.  */
    while(_tx_timer_created_ptr)
    {

        /* Deactivate timer.  */
        tx_timer_deactivate(_tx_timer_created_ptr);

        /* Delete timer.  */
        tx_timer_delete(_tx_timer_created_ptr);
    }

    /* Delete all mutexes (except for system mutex).  */
    while(_tx_mutex_created_ptr)
    {

        /* Setup working mutex pointer.  */
        mutex_ptr =  _tx_mutex_created_ptr;

#ifdef __ghs

        /* Determine if the mutex is the GHS system mutex.  If so, don't delete!  */
        if (mutex_ptr == &__ghLockMutex)
        {

            /* Move to next mutex.  */
            mutex_ptr =  mutex_ptr -> tx_mutex_created_next;
        }

        /* Determine if there are no more mutexes to delete.  */
        if (_tx_mutex_created_count == 1)
            break;
#endif

        /* Delete mutex.  */
        tx_mutex_delete(mutex_ptr);
    }

    /* Delete all threads, except for timer thread, and test control thread.  */
    while (_tx_thread_created_ptr)
    {

        /* Setup working pointer.  */
        thread_ptr =  _tx_thread_created_ptr;


#ifdef TX_TIMER_PROCESS_IN_ISR

        /* Determine if there are more threads to delete.  */
        if (_tx_thread_created_count == 1)
            break;

        /* Determine if this thread is the test control thread.  */
        if (thread_ptr == &test_control_thread)
        {

            /* Move to the next thread pointer.  */
            thread_ptr =  thread_ptr -> tx_thread_created_next;
        }
#else

        /* Determine if there are more threads to delete.  */
        if (_tx_thread_created_count == 2)
            break;

        /* Move to the thread not protected.  */
        while ((thread_ptr == &_tx_timer_thread) || (thread_ptr == &test_control_thread))
        {

            /* Yes, move to the next thread.  */
            thread_ptr =  thread_ptr -> tx_thread_created_next;
        }
#endif

        /* First terminate the thread to ensure it is ready for deletion.  */
        tx_thread_terminate(thread_ptr);

        /* Delete the thread.  */
        tx_thread_delete(thread_ptr);
    }

    /* At this point, only the test control thread and the system timer thread and/or mutex should still be
       in the system.  */
}




