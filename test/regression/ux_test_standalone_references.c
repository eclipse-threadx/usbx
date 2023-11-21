
/* Pure UX configurations, isolate other headers configurations.  */
#include "ux_api.h"

#if defined(UX_HOST_STANDALONE) && !defined(_ux_utility_time_get)
extern ULONG _tx_time_get(VOID);
ULONG _ux_utility_time_get(VOID)
{
    return(_tx_time_get());
}
#endif
#if defined(UX_STANDALONE) && !defined(_ux_utility_interrupt_disable)
extern ALIGN_TYPE _tx_thread_interrupt_disable(VOID);
ALIGN_TYPE _ux_utility_interrupt_disable(VOID)
{
    return _tx_thread_interrupt_disable();
}
#endif
#if defined(UX_STANDALONE) && !defined(_ux_utility_interrupt_restore)
extern VOID _tx_thread_interrupt_restore(ALIGN_TYPE flags);
VOID _ux_utility_interrupt_restore(ALIGN_TYPE flags)
{
    _tx_thread_interrupt_restore(flags);
}
#endif
