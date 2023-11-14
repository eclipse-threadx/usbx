#!/bin/bash

set -e

cd $(dirname $0)
root_path=$(cd ../../../common; pwd)

# Exclude strings
exclude_file_list=" $root_path/*/src/*_class_asix*"
exclude_file_list+=" $root_path/*/src/*_class_audio*"
exclude_file_list+=" $root_path/*/src/*_class_ccid*"
exclude_file_list+=" $root_path/*/src/*_class_dfu*"
exclude_file_list+=" $root_path/*/src/*_class_gser*"
exclude_file_list+=" $root_path/*/src/*_class_pima*"
exclude_file_list+=" $root_path/*/src/*_class_rndis*"
exclude_file_list+=" $root_path/*/src/*_class_printer*"
exclude_file_list+=" $root_path/*/src/*_class_prolific*"
exclude_file_list+=" $root_path/*/src/*_class_swar*"
exclude_file_list+=" $root_path/*/src/*_class_video*"
exclude_file_list+=" $root_path/*/src/*_hnp_*"
exclude_file_list+=" $root_path/*/src/*_role_*"
exclude_file_list+=" $root_path/*/src/ux_utility_set_interrupt_handler.c"
exclude_file_list+=" $root_path/*/src/*dcd_sim_slave*"
exclude_file_list+=" $root_path/*/src/*device_class_dpump*"
exclude_file_list+=" $root_path/*/src/*hcd_sim_host*"
exclude_file_list+=" $root_path/*/src/*host_class_dpump*"
exclude_file_list+=" $root_path/*/src/*ux_network_driver*"

# Device HID interrupt OUT related
exclude_file_list+=" $root_path/*/src/ux_device_class_hid_read.c"
exclude_file_list+=" $root_path/*/src/ux_device_class_hid_receiver*"

# CB/CBI related
exclude_file_list+=" $root_path/*/src/*_storage*_cb.c"
exclude_file_list+=" $root_path/*/src/*_storage*_cbi.c"

# CD/DVD related things
exclude_file_list+=" $root_path/*/src/*_storage_get_status*"
exclude_file_list+=" $root_path/*/src/*_storage_get_configuration*"
exclude_file_list+=" $root_path/*/src/*_storage_get_performance*"
exclude_file_list+=" $root_path/*/src/*_storage_read_disk_information*"
exclude_file_list+=" $root_path/*/src/*_storage_report_key*"
exclude_file_list+=" $root_path/*/src/*_storage_read_dvd*"
exclude_file_list+=" $root_path/*/src/*_storage_read_toc*"

# Obsolete
exclude_file_list+=" $root_path/*/src/ux_device_stack_interface_get.c"
exclude_file_list+=" $root_path/*/src/ux_host_stack_delay_ms.c"

# Host controllers
exclude_file_list+=" $root_path/usbx_host_controllers/src/*"

# Pictbridge related files
exclude_file_list+=" $root_path/usbx_pictbridge/src/*"

# Host related files
exclude_host_list=" $root_path/*/src/*_host_*"

# Device related files
exclude_device_list=" $root_path/*/src/*_device_*"

# RTOS related files
exclude_rtos_list=" $root_path/*/src/*_thread*"
exclude_rtos_list+=" $root_path/*/src/*_event_*"
exclude_rtos_list+=" $root_path/*/src/*_mutex_*"
exclude_rtos_list+=" $root_path/*/src/*_semaphore_*"
exclude_rtos_list+=" $root_path/*/src/*_timer_*"

exclude_rtos_list+=" $root_path/*/src/*_cdc_ecm_*"

exclude_rtos_list+=" $root_path/*/src/*_hub_*"

exclude_rtos_list+=" $root_path/*/src/*_cdc_acm_capabilities_get.c"
exclude_rtos_list+=" $root_path/*/src/*_cdc_acm_configure.c"

exclude_rtos_list+=" $root_path/*/src/*_hid_configure.c"
exclude_rtos_list+=" $root_path/*/src/*_hid_descriptor_parse.c"
exclude_rtos_list+=" $root_path/*/src/*_hid_report_descriptor_get.c"

exclude_rtos_list+=" $root_path/*/src/*_storage_configure.c"
exclude_rtos_list+=" $root_path/*/src/*_storage_media_mount.c"
exclude_rtos_list+=" $root_path/*/src/*_storage_media_open.c"
exclude_rtos_list+=" $root_path/*/src/*_storage_partition_read.c"
exclude_rtos_list+=" $root_path/*/src/*_storage_transport.c"

# Standalone related files
exclude_standalone_list=" $root_path/*/src/*_run.c"

if [[ $1 = *"_full_coverage" ]]; then

    exclude_options=""
else

    exclude_options=""
    for f in $exclude_file_list;do
        exclude_options+=" -e $f"
    done

    if [[ $1 = *"_device_"* ]]; then
        for f in $exclude_host_list;do
            exclude_options+=" -e $f"
        done
    fi

    if [[ $1 = *"_host_"* ]]; then
        for f in $exclude_device_list;do
            exclude_options+=" -e $f"
        done
    fi

    if [[ $1 = "standalone_"* ]]; then
        for f in $exclude_rtos_list;do
            exclude_options+=" -e $f"
        done
    else
        for f in $exclude_standalone_list;do
            exclude_options+=" -e $f"
        done
    fi
fi

mkdir -p coverage_report/$1
gcovr --object-directory=build/$1/usbx/CMakeFiles/usbx.dir/common -r ../../../common $exclude_options --xml-pretty --output coverage_report/$1.xml
gcovr --object-directory=build/$1/usbx/CMakeFiles/usbx.dir/common -r ../../../common $exclude_options --html --html-details --html-high-threshold 100.0 --output coverage_report/$1/index.html
