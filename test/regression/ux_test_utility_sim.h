/* This test simulator is designed to simulate ux_utility_ APIs for test.  */

#ifndef _UX_TEST_UTILITY_SIM_H
#define _UX_TEST_UTILITY_SIM_H

VOID  ux_test_sim_inp_sequence_set(ULONG* seq, ULONG size);
VOID  ux_test_sim_outp_logbuf_set(ULONG* buf, ULONG size);
ULONG ux_test_sim_outp_log_get(ULONG seq, ULONG *addr, ULONG *value);
ULONG ux_test_sim_outp_log_count(VOID);
VOID  ux_test_sim_outp_log_reset(VOID);

VOID  ux_test_utility_sim_sem_create_count_reset    (VOID);
ULONG ux_test_utility_sim_sem_create_count          (VOID);

VOID  ux_test_utility_sim_sem_error_generation_start(ULONG fail_after);
VOID  ux_test_utility_sim_sem_error_generation_stop (VOID);

VOID  ux_test_utility_sim_sem_get_count_reset    (VOID);
ULONG ux_test_utility_sim_sem_get_count          (VOID);

VOID  ux_test_utility_sim_sem_get_error_exception_reset(VOID);
VOID  ux_test_utility_sim_sem_get_error_exception_add(TX_SEMAPHORE *semaphore, ULONG semaphore_signal);
VOID  ux_test_utility_sim_sem_get_error_generation_start(ULONG fail_after);
VOID  ux_test_utility_sim_sem_get_error_generation_stop (VOID);

VOID  ux_test_utility_sim_mutex_create_count_reset    (VOID);
ULONG ux_test_utility_sim_mutex_create_count          (VOID);

VOID  ux_test_utility_sim_mutex_error_generation_start(ULONG fail_after);
VOID  ux_test_utility_sim_mutex_error_generation_stop (VOID);

#if 0 /* Current Mutex ON/OFF has no return code. */
VOID  ux_test_utility_sim_mutex_on_count_reset    (VOID);
ULONG ux_test_utility_sim_mutex_on_count          (VOID);
VOID  ux_test_utility_sim_mutex_on_error_generation_start(ULONG fail_after);
VOID  ux_test_utility_sim_mutex_on_error_generation_stop (VOID);
#endif

VOID  ux_test_utility_sim_event_create_count_reset    (VOID);
ULONG ux_test_utility_sim_event_create_count          (VOID);
VOID  ux_test_utility_sim_event_error_generation_start(ULONG fail_after);
VOID  ux_test_utility_sim_event_error_generation_stop (VOID);

VOID  ux_test_utility_sim_thread_create_count_reset    (VOID);
ULONG ux_test_utility_sim_thread_create_count          (VOID);
VOID  ux_test_utility_sim_thread_error_generation_start(ULONG fail_after);
VOID  ux_test_utility_sim_thread_error_generation_stop (VOID);

VOID ux_test_utility_sim_mem_allocate_until_align_flagged(ULONG target_fail_level, ULONG memory_alignment, ULONG memory_cache_flag);

#define ux_test_utility_sim_mem_allocate_until_flagged(fail_level, cache_flag) ux_test_utility_sim_mem_allocate_until_align_flagged(fail_level, UX_NO_ALIGN, cache_flag)

VOID ux_test_utility_sim_mem_free_all_flagged      (ULONG memory_cache_flag);

#define ux_test_utility_sim_mem_allocate_until(l) ux_test_utility_sim_mem_allocate_until_flagged((l), UX_REGULAR_MEMORY)
#define ux_test_utility_sim_mem_free_all()        ux_test_utility_sim_mem_free_all_flagged(UX_REGULAR_MEMORY)

VOID  ux_test_utility_sim_mem_alloc_log_enable(UCHAR enable_disable);
VOID  ux_test_utility_sim_mem_alloc_log_lock(VOID);
ULONG ux_test_utility_sim_mem_alloc_count(VOID);
VOID  ux_test_utility_sim_mem_alloc_count_reset(VOID);
VOID  ux_test_utility_sim_mem_alloc_error_generation_start(ULONG fail_after);
VOID  ux_test_utility_sim_mem_alloc_error_generation_stop(VOID);
UINT  ux_test_utility_sim_mem_alloc_error_generation_active(VOID);

VOID ux_test_utility_sim_mem_alloc_fail_all_start(VOID);
VOID ux_test_utility_sim_mem_alloc_fail_all_stop(VOID);

VOID ux_test_utility_sim_cleanup(VOID);

#endif /* _UX_TEST_UTILITY_SIM_H */
