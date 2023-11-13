/* This test simulator is designed to simulate ux_hcd_ APIs for test. */

#ifndef _UX_TEST_DCD_SIM_SLAVE_H
#define _UX_TEST_DCD_SIM_SLAVE_H

#include "ux_test.h"

/* Uses general entry action simulator */
#define _UX_TEST_DCD_SIM_ACTION_STRUCT _UX_TEST_SIM_ENTRY_ACTION_STRUCT
#define UX_TEST_DCD_SIM_ACTION UX_TEST_SIM_ENTRY_ACTION

VOID ux_test_dcd_sim_slave_disconnect(VOID);
VOID ux_test_dcd_sim_slave_connect   (ULONG speed);
VOID ux_test_dcd_sim_slave_connect_framework(UCHAR * framework, ULONG framework_length);

UINT _ux_test_dcd_sim_slave_initialize(VOID);

VOID ux_test_dcd_sim_slave_cleanup(VOID);

UINT _ux_test_dcd_sim_slave_function(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter);

VOID ux_test_dcd_sim_slave_transfer_done(UX_SLAVE_TRANSFER *transfer, UINT code);

#endif /* _UX_TEST_DCD_SIM_SLAVE_H */
