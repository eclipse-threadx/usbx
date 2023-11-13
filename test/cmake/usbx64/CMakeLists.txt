cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0077 NEW)

project(usbx_test LANGUAGES C)

set(UX_USER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ux_user.h)
include(${CMAKE_CURRENT_SOURCE_DIR}/../usbx/CMakeLists.txt)

# Set build configurations
set(BUILD_CONFIGURATIONS default_build_coverage)

# Remove files not needed from 64 bit build
get_target_property(SOURCES_LIST usbx SOURCES)
get_target_property(SOURCE_DIR usbx SOURCE_DIR)
foreach(EXCLUDE_DIR common/usbx_host_controllers/src)
  aux_source_directory(${SOURCE_DIR}/${EXCLUDE_DIR} EXCLUDE_SOURCES)
  list(REMOVE_ITEM SOURCES_LIST ${EXCLUDE_SOURCES})
endforeach()
set_target_properties(usbx PROPERTIES SOURCES "${SOURCES_LIST}")
