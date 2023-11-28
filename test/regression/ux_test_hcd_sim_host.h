/* This test simulator is designed to simulate ux_hcd_ APIs for test. */

#ifndef _UX_TEST_HCD_SIM_HOST_H
#define _UX_TEST_HCD_SIM_HOST_H

#include "ux_test.h"

/* Uses general entry action simulator */
#define _UX_TEST_HCD_SIM_ACTION_STRUCT _UX_TEST_SIM_ENTRY_ACTION_STRUCT
#define UX_TEST_HCD_SIM_ACTION UX_TEST_SIM_ENTRY_ACTION

VOID ux_test_hcd_sim_host_disconnect_no_wait(VOID);
VOID ux_test_hcd_sim_host_disconnect(VOID);
VOID ux_test_hcd_sim_host_connect_no_wait(ULONG speed);
VOID ux_test_hcd_sim_host_connect(ULONG speed);

UINT  _ux_test_hcd_sim_host_initialize(UX_HCD *hcd);

VOID ux_test_hcd_sim_host_cleanup(VOID);

UINT _ux_test_hcd_sim_host_entry(UX_HCD *hcd, UINT function, VOID *parameter);

#endif /* _UX_TEST_HCD_SIM_HOST_H */
