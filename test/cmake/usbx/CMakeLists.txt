cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0077 NEW)

project(usbx_test LANGUAGES C)

# Use customized ux_user.h
set(UX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ux_user.h)

# Copy files instead of using symlink
# libs/ -> ../libs/
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libs)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../libs/CMakeLists.txt
               ${CMAKE_CURRENT_SOURCE_DIR}/libs/CMakeLists.txt COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../libs/nx_user.h
               ${CMAKE_CURRENT_SOURCE_DIR}/libs/nx_user.h COPYONLY)

# Set build configurations
# TODO: add when available : standalone_otg_build
set(BUILD_CONFIGURATIONS 
  default_build_coverage 
  error_check_build_full_coverage 
  tracex_enable_build 
  device_buffer_owner_build 
  device_zero_copy_build
  nofx_build_coverage 
  optimized_build 
  standalone_device_build_coverage 
  standalone_device_buffer_owner_build 
  standalone_device_zero_copy_build
  standalone_host_build_coverage 
  standalone_build_coverage 
  generic_build 
  otg_support_build
  memory_management_build_coverage
  msrc_rtos_build
  msrc_standalone_build
  )

set(CMAKE_CONFIGURATION_TYPES
    ${BUILD_CONFIGURATIONS}
    CACHE STRING "list of supported configuration types" FORCE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                                             ${CMAKE_CONFIGURATION_TYPES})
list(GET CMAKE_CONFIGURATION_TYPES 0 BUILD_TYPE)
if((NOT CMAKE_BUILD_TYPE) OR (NOT ("${CMAKE_BUILD_TYPE}" IN_LIST
                                   CMAKE_CONFIGURATION_TYPES)))
  set(CMAKE_BUILD_TYPE
      "${BUILD_TYPE}"
      CACHE STRING "Build Type of the project" FORCE)
endif()

message(STATUS "Build for usbx")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}.")

set(default_build_coverage
  -DNX_PHYSICAL_HEADER=20
#  -DUX_SLAVE_REQUEST_CONTROL_MAX_LENGTH=512
#  -DUX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT
#  -DUX_HOST_DEVICE_CLASS_CODE_VALIDATION_ENABLE
# -DUX_DEVICE_CLASS_HID_INTERRUPT_OUT_SUPPORT
# -DUX_HOST_CLASS_HID_INTERRUPT_OUT_SUPPORT
  -DUX_DISABLE_ASSERT
##############################################FeedBack Test Start
#  -DUX_DEVICE_CLASS_AUDIO_FEEDBACK_SUPPORT
#  -DUX_MAX_DEVICE_ENDPOINTS=6
#  -DUX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT
#  -DUX_SLAVE_REQUEST_CONTROL_MAX_LENGTH=512
##############################################FeedBack Test end
  # -DUX_HOST_CLASS_AUDIO_2_SUPPORT
  # -DUX_HOST_CLASS_AUDIO_FEEDBACK_SUPPORT
  # -DUX_HOST_CLASS_AUDIO_DISABLE_CONTROLS
  # -DUX_HOST_CLASS_AUDIO_INTERRUPT_SUPPORT
  # -DUX_DEVICE_CLASS_PRINTER_WRITE_AUTO_ZLP
  # -DUX_DEVICE_CLASS_CDC_ACM_WRITE_AUTO_ZLP
  # -DUX_DEVICE_CLASS_AUDIO_INTERRUPT_SUPPORT
  -DUX_HOST_STACK_CONFIGURATION_INSTANCE_CREATE_CONTROL=0
  -DUX_DEVICE_ENABLE_GET_STRING_WITH_ZERO_LANGUAGE_ID
)

set(error_check_build_full_coverage
  ${default_build_coverage}
  -DUX_ENABLE_ASSERT
  -DUX_ENABLE_ERROR_CHECKING
  -DUX_PIMA_WITH_MTP_SUPPORT
)

set(msrc_rtos_build
  ${error_check_build_full_coverage}
  -DUX_SLAVE_REQUEST_CONTROL_MAX_LENGTH=512
  -DUX_HOST_CLASS_AUDIO_2_SUPPORT
  -DUX_HOST_CLASS_AUDIO_INTERRUPT_SUPPORT
)
set(msrc_standalone_build
  ${msrc_rtos_build}
  -DUX_STANDALONE
)

set(tracex_enable_build
  -DNX_PHYSICAL_HEADER=20
  -DTX_ENABLE_EVENT_TRACE
  -DUX_TRACE_INSERT_MACROS
)

set(device_buffer_owner_build
  -DNX_PHYSICAL_HEADER=20
# -DUX_ENABLE_ASSERT
  -DTX_ENABLE_EVENT_TRACE
  -DUX_TRACE_INSERT_MACROS
  -DUX_SLAVE_REQUEST_CONTROL_MAX_LENGTH=512
  -DUX_DEVICE_ENDPOINT_BUFFER_OWNER=1
  # -DUX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE
  -DUX_DEVICE_CLASS_HID_INTERRUPT_OUT_SUPPORT
  -DUX_DEVICE_CLASS_AUDIO_FEEDBACK_SUPPORT
  -DUX_DEVICE_CLASS_AUDIO_INTERRUPT_SUPPORT
)
set(device_zero_copy_build
  ${device_buffer_owner_build}
  -DUX_DEVICE_CLASS_CDC_ACM_ZERO_COPY
  -DUX_DEVICE_CLASS_CDC_ACM_WRITE_AUTO_ZLP
  -DUX_DEVICE_CLASS_HID_ZERO_COPY
  -DUX_DEVICE_CLASS_CDC_ECM_ZERO_COPY
  -DUX_DEVICE_CLASS_RNDIS_ZERO_COPY
  -DUX_DEVICE_CLASS_PRINTER_ZERO_COPY
)

set(nofx_build_coverage
  -DNX_PHYSICAL_HEADER=20
  -DUX_HOST_CLASS_STORAGE_NO_FILEX
)
set(standalone_build_coverage
  -DNX_PHYSICAL_HEADER=20
  -DUX_STANDALONE
  # -DTX_ENABLE_EVENT_TRACE
)
set(standalone_device_build_coverage
  -DUX_DEVICE_STANDALONE
  # -DUX_DEVICE_CLASS_HID_INTERRUPT_OUT_SUPPORT
  -DUX_SLAVE_REQUEST_CONTROL_MAX_LENGTH=512
##############################################FeedBack Test Start
#  -DUX_DEVICE_CLASS_AUDIO_FEEDBACK_SUPPORT
#  -DUX_MAX_DEVICE_ENDPOINTS=6
#  -DUX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT
##############################################FeedBack Test End
#  -DUX_DEVICE_CLASS_AUDIO_INTERRUPT_SUPPORT
  -DUX_ENABLE_ERROR_CHECKING
)
set(standalone_device_buffer_owner_build
  ${device_buffer_owner_build}
  ${standalone_device_build_coverage}
)
set(standalone_device_zero_copy_build
  ${device_zero_copy_build}
  ${standalone_device_build_coverage}
)
set(standalone_host_build_coverage
  -DUX_HOST_STANDALONE
  # -DUX_HOST_CLASS_HID_INTERRUPT_OUT_SUPPORT
  -DUX_ENABLE_ERROR_CHECKING
)
set(standalone_otg_build
  -DUX_OTG_STANDALONE
)
set(optimized_build
  -DNX_PHYSICAL_HEADER=20
  -DUX_NAME_REFERENCED_BY_POINTER
  -DUX_MAX_HCD=1
  -DUX_MAX_ISO_TD=0
  -DUX_MAX_TD=20
  -DUX_MAX_CLASS_DRIVER=1
  -DUX_MAX_DEVICES=1
  -DUX_HOST_CLASS_STORAGE_MEMORY_BUFFER_SIZE=512
  -DUX_HOST_CLASS_STORAGE_MAX_TRANSFER_SIZE=512
  -DUX_HOST_CLASS_STORAGE_MAX_MEDIA=1
  -DUX_DEVICE_INITIALIZE_FRAMEWORK_SCAN_DISABLE
  -DUX_MAX_DEVICE_ENDPOINTS=2
  -DUX_MAX_DEVICE_INTERFACES=1
  -DUX_MAX_SLAVE_INTERFACES=1
  -DUX_MAX_SLAVE_CLASS_DRIVER=1
  -DUX_MAX_SLAVE_LUN=1
  -DUX_SLAVE_REQUEST_DATA_MAX_LENGTH=512
  -DUX_DEVICE_ALTERNATE_SETTING_SUPPORT_DISABLE
)
set(generic_build
  -DUX_HCD_EHCI_SPLIT_TRANSFER_ENABLE
  -DUX_HOST_CLASS_STORAGE_INCLUDE_LEGACY_PROTOCOL_SUPPORT
  -DUX_SLAVE_CLASS_STORAGE_INCLUDE_MMC
  ############################################## warning check: CDC ACM
  -DUX_DEVICE_CLASS_CDC_ACM_TRANSMISSION_DISABLE
  ############################################## warning check: DFU
  # -DUX_DEVICE_CLASS_DFU_UPLOAD_DISABLE
  # -DUX_DEVICE_CLASS_DFU_ERROR_GET_ENABLE
  -DUX_DEVICE_CLASS_DFU_STATUS_MODE=1 #0/1
  -DUX_DEVICE_CLASS_DFU_STATUS_POLLTIMEOUT=0
  # -DUX_DEVICE_CLASS_DFU_CUSTOM_REQUEST_ENABLE
  -DUX_HOST_STACK_CONFIGURATION_INSTANCE_CREATE_CONTROL=1
)
set(otg_support_build
  -DNX_PHYSICAL_HEADER=20
  -DUX_OTG_SUPPORT=
)
set(memory_management_build_coverage
  ${default_build_coverage}
  -DUX_ENFORCE_SAFE_ALIGNMENT
  -DUX_ENABLE_MEMORY_STATISTICS
  -DUX_ENABLE_MEMORY_POOL_SANITY_CHECK
)
# Control if USBX is static or shared
if($ENV{USBX_STATIC})
  message(STATUS "Building STATIC usbx")
  set(BUILD_SHARED_LIBS OFF)
else()
  message(STATUS "Building usbx BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
endif()
if(NOT BUILD_SHARED_LIBS)
  if(CMAKE_BUILD_TYPE MATCHES ".*_coverage")
    add_link_options(-fprofile-arcs)
    add_link_options(-lgcov)
  endif()
endif()

# Control if it's for 64 bit or 32 bit
if($ENV{ENABLE_64})
  message(STATUS "Building for 64bit")
else()
  add_compile_options(-m32)
  add_link_options(-m32)
  message(STATUS "Building for 32bit")
endif()
add_compile_options(
  -std=c99
  -ggdb
  -g3
  -gdwarf-2
  -fdiagnostics-color
  -DUX_USE_IO_INSTRUCTIONS
  #-DUX_DEVICE_CLASS_DFU_CUSTOM_REQUEST_ENABLE
  #-DUX_DEVICE_CLASS_DFU_STATUS_MODE=1
  ${${CMAKE_BUILD_TYPE}})

enable_testing()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../ usbx)
if(CMAKE_BUILD_TYPE STREQUAL "generic_build")
  add_test(fake_test true)
else()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/regression regression)
endif()

# TODO: Unmask after adding sample for STANDALONE
if(NOT (CMAKE_BUILD_TYPE MATCHES "standalone.*"))
  add_subdirectory(../usbx/samples samples)
endif()

# Coverage
if(CMAKE_BUILD_TYPE MATCHES ".*_coverage")
  target_compile_options(usbx PRIVATE -fprofile-arcs -ftest-coverage)
  target_link_options(usbx PRIVATE -fprofile-arcs -ftest-coverage)
endif()

# Build ThreadX library once
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run.sh build_libs)
add_custom_target(build_libs ALL COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
                                         build_libs)

add_dependencies(usbx build_libs)
target_include_directories(usbx PUBLIC ${CMAKE_BINARY_DIR}/../libs/inc)

add_library(threadx SHARED IMPORTED GLOBAL)
add_library("azrtos::threadx" ALIAS threadx)
set_target_properties(
  threadx PROPERTIES IMPORTED_LOCATION
                     ${CMAKE_BINARY_DIR}/../libs/threadx/libthreadx.so)

add_library(netxduo SHARED IMPORTED GLOBAL)
add_library("azrtos::netxduo" ALIAS netxduo)
set_target_properties(
  netxduo PROPERTIES IMPORTED_LOCATION
                     ${CMAKE_BINARY_DIR}/../libs/netxduo/libnetxduo.so)

add_library(filex SHARED IMPORTED GLOBAL)
add_library("azrtos::filex" ALIAS filex)
set_target_properties(
  filex PROPERTIES IMPORTED_LOCATION
                   ${CMAKE_BINARY_DIR}/../libs/filex/libfilex.so)

target_compile_options(
  usbx
  PRIVATE -Werror
          -Wall
          -Wextra
          -pedantic
          -fmessage-length=0
          -fsigned-char
          -ffunction-sections
          -fdata-sections
          -Wunused
          -Wuninitialized
          -Wmissing-declarations
          -Wconversion
          -Wpointer-arith
          -Wshadow
          -Wlogical-op
          -Waggregate-return
          -Wfloat-equal
          -DFX_ENABLE_EXFAT)

# Remove includes and files not needed from usbx build
set(UX_STANDALONE_HOST_EXCLUDES
  # ux_host_class_asix
  # ux_host_class_audio
  # ux_host_class_cdc_acm
  # ux_host_class_cdc_ecm
  # ux_host_class_gser
  # ux_host_class_hid
  # ux_host_class_hub
  # ux_host_class_pima
  # ux_host_class_printer
  # ux_host_class_prolific
  # ux_host_class_storage
  # ux_host_class_swar
  # ux_host_class_video
  # ux_hcd_ehci
  # ux_hcd_ohci
)
set(UX_STANDALONE_DEVICE_EXCLUDES
  # ux_device_class_audio
  # ux_device_class_cdc_acm
  # ux_device_class_cdc_ecm
  # ux_device_class_dfu
  # ux_device_class_hid
  # ux_device_class_pima
  # ux_device_class_rndis
  # ux_device_class_ccid
  # ux_device_class_printer
  # ux_device_class_video
)
set(UX_STANDALONE_UTILITY_EXCLUDES
  # ux_utility_event
  # ux_utility_delay
  # ux_utility_mutex
  # ux_utility_semaphore
  # ux_utility_thread
  # ux_utility_timer
)
set(UX_STANDALONE_PICTBRIDGE_EXCLUDES
  # ux_pictbridge
)
set(UX_STANDALONE_NX_EXCLUDES
  # ux_network_driver
)
set(UX_STANDALONE_FX_EXCLUDES
  # ux_host_class_storage_driver_entry
  usbx_ux_host_class_storage_fx_driver
)
if(CMAKE_BUILD_TYPE MATCHES "standalone.*")
  get_target_property(test_utility_SOURCES_LIST test_utility SOURCES)
  get_target_property(SOURCES_LIST usbx SOURCES)
  get_target_property(INCLUDES_LIST usbx INCLUDE_DIRECTORIES)

  if("${CMAKE_BUILD_TYPE}" STREQUAL "standalone_build_coverage")
    # TODO: enable when STANDALONE implement is done
    # Remove files not support STANDALONE yet
    foreach(EXCLUDE_VAL ${UX_STANDALONE_HOST_EXCLUDES}
                        ${UX_STANDALONE_DEVICE_EXCLUDES}
                        ${UX_STANDALONE_UTILITY_EXCLUDES}
                        ${UX_STANDALONE_PICTBRIDGE_EXCLUDES}
                        ${UX_STANDALONE_NX_EXCLUDES}
                        ${UX_STANDALONE_FX_EXCLUDES})
      list(FILTER test_utility_SOURCES_LIST EXCLUDE REGEX ".*${EXCLUDE_VAL}.*")
      list(FILTER SOURCES_LIST EXCLUDE REGEX ".*${EXCLUDE_VAL}.*")
    endforeach()
    # Update includes to remove TX,FX,NX libs
    list(FILTER INCLUDES_LIST EXCLUDE REGEX ".*externals.*")
    list(FILTER INCLUDES_LIST EXCLUDE REGEX ".*/libs/inc.*")
    # Update library links to remove TX,FX,NX libs
    set_target_properties(usbx PROPERTIES LINK_LIBRARIES "")
    set_target_properties(usbx PROPERTIES INTERFACE_LINK_LIBRARIES "")

  elseif("${CMAKE_BUILD_TYPE}" STREQUAL "standalone_device_build_coverage")
    # Only device part linked with STANDALONE
    # Remove files not support STANDALONE yet
    foreach(EXCLUDE_VAL ${UX_STANDALONE_DEVICE_EXCLUDES})
      list(FILTER SOURCES_LIST EXCLUDE REGEX ".*${EXCLUDE_VAL}.*")
      list(FILTER test_utility_SOURCES_LIST EXCLUDE REGEX ".*${EXCLUDE_VAL}.*")
    endforeach()

  elseif("${CMAKE_BUILD_TYPE}" STREQUAL "standalone_host_build_coverage")
    # Only host part linked with STANDALONE
    # Remove files not support STANDALONE yet
    foreach(EXCLUDE_VAL ${UX_STANDALONE_HOST_EXCLUDES})
      list(FILTER SOURCES_LIST EXCLUDE REGEX ".*${EXCLUDE_VAL}.*")
      list(FILTER test_utility_SOURCES_LIST EXCLUDE REGEX ".*${EXCLUDE_VAL}.*")
    endforeach()

  elseif("${CMAKE_BUILD_TYPE}" STREQUAL "standalone_otg_build")
    # Only some stack file linked with STANDALONE
    # No file list change for now
  endif()

  # Commit sources and includes changes
  set_target_properties(test_utility PROPERTIES SOURCES "${test_utility_SOURCES_LIST}")
  set_target_properties(usbx PROPERTIES SOURCES "${SOURCES_LIST}")
  set_target_properties(usbx PROPERTIES INCLUDE_DIRECTORIES "${INCLUDES_LIST}")
endif()

# Remove files not compatible with optimized build
set(UX_OPTIMIZED_EXCLUDES
  ux_device_class_rndis # buffer smaller than message size
)
if(CMAKE_BUILD_TYPE STREQUAL "optimized_build")
  get_target_property(test_utility_SOURCES_LIST test_utility SOURCES)
  get_target_property(SOURCES_LIST usbx SOURCES)

  # Remove files not support STANDALONE yet
  foreach(EXCLUDE_VAL ${UX_OPTIMIZED_EXCLUDES})
    list(FILTER test_utility_SOURCES_LIST EXCLUDE REGEX ".*${EXCLUDE_VAL}.*")
    list(FILTER SOURCES_LIST EXCLUDE REGEX ".*${EXCLUDE_VAL}.*")
  endforeach()

  # Commit sources and includes changes
  set_target_properties(test_utility PROPERTIES SOURCES "${test_utility_SOURCES_LIST}")
  set_target_properties(usbx PROPERTIES SOURCES "${SOURCES_LIST}")
endif()

# Use generic port file for generic build.
if(CMAKE_BUILD_TYPE STREQUAL "generic_build")
  get_target_property(INCLUDES_LIST usbx INCLUDE_DIRECTORIES)
  set(NEW_INCLUDES_LIST "")
  foreach(INCLUDE_LIST ${INCLUDES_LIST})
    string(REPLACE "ports/linux/gnu" "ports/generic" INCLUDE_LIST ${INCLUDE_LIST})
    list(APPEND NEW_INCLUDES_LIST "${INCLUDE_LIST}")
  endforeach()
  set_target_properties(usbx PROPERTIES INCLUDE_DIRECTORIES "${NEW_INCLUDES_LIST}")
endif()