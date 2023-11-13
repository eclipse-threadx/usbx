/* This test simulator is designed to simulate ux_utility_ APIs for test.  */

#include <stdio.h>

#include "tx_api.h"
#include "ux_test.h"

#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"

#include "ux_test_utility_sim.h"

#define FAIL_DISABLE (~0x00ul)

extern void  test_control_return(UINT status);

    static UX_MEMORY_BLOCK fail_memory_block_first =
    {
        .ux_memory_block_next = UX_NULL,
        .ux_memory_byte_pool = UX_NULL,
    };

static UX_MEMORY_BLOCK *original_regular_memory_block;
static UX_MEMORY_BLOCK *original_cache_safe_memory_block;

#define MAX_FLAGGED_MEMORY_ALLOCATION_POINTERS 8*1024
static VOID *flagged_memory_allocation_pointers[2][MAX_FLAGGED_MEMORY_ALLOCATION_POINTERS];
static ULONG num_flagged_memory_allocation_pointers[2];

VOID ux_test_utility_sim_no_overriding_cleanup(VOID)
{

    num_flagged_memory_allocation_pointers[0] = 0;
    num_flagged_memory_allocation_pointers[1] = 0;
}

/* Memory allocation simulator*/

VOID ux_test_utility_sim_mem_alloc_fail_all_start(VOID)
{
    original_regular_memory_block = (UX_MEMORY_BLOCK *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start;
    original_cache_safe_memory_block = (UX_MEMORY_BLOCK *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start;

    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start = (UCHAR*)&fail_memory_block_first;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start = (UCHAR*)&fail_memory_block_first;
}

VOID ux_test_utility_sim_mem_alloc_fail_all_stop(VOID)
{
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start = (UCHAR*)original_regular_memory_block;
    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start = (UCHAR*)original_cache_safe_memory_block;
}


/* When the memory allocator tries to find a block of memory for the requested size, it will add extra stuff to
   accomodate things like having to align the memory. See the allocator for what it adds. We don't take the requested
   size alignment into account - we leave it up to the caller.  */
static ULONG get_amount_added_by_memory_allocator(ULONG memory_alignment, ULONG memory_size_requested)
{

ULONG added;

    if (memory_alignment == UX_SAFE_ALIGN)
        memory_alignment = UX_NO_ALIGN;

    if (memory_alignment <= UX_ALIGN_MIN)
    {
        added = sizeof(UX_MEMORY_BLOCK) +
        /* This is the amount of memory required to ensure the next memory block buffer is properly align. */
        (((sizeof(UX_MEMORY_BLOCK) + UX_ALIGN_MIN) & (~(ULONG)UX_ALIGN_MIN)) - sizeof(UX_MEMORY_BLOCK));
    }
    else
    {
        added = memory_alignment + sizeof(UX_MEMORY_BLOCK) +
        /* This is the amount of memory required to ensure the next memory block buffer is properly align. */
        (((sizeof(UX_MEMORY_BLOCK) + UX_ALIGN_MIN) & (~(ULONG)UX_ALIGN_MIN)) - sizeof(UX_MEMORY_BLOCK));
    }

    return added;
}

/* Returns the amount of memory that should be passed to ux_utility_memory_allocate
   to allocate a specific memory block of the given size. */
static ULONG memory_requested_size_for_block(ULONG memory_alignment, ULONG block_size)
{

ULONG result;
ULONG added;

    added = get_amount_added_by_memory_allocator(memory_alignment, block_size);
    if (added > block_size)
    {
        return 0;
    }

    /* Now subtract everything we added. */
    result = block_size - added;

    /* The allocator will align the requested size to 16 bytes, so if we don't do any alignment now, the allocator
       might add to the size. We can avoid this by just aligning to 16 bytes now. */
    result &= ~(UX_ALIGN_MIN);

    // /* _ux_utility_memory_free_block_best_get does a '>' check, not '>='. */
    // result--;

    return(result);
}

static void add_memory_allocation_pointer(ULONG memory_cache_flag, VOID *allocation_ptr)
{
    UX_TEST_ASSERT(num_flagged_memory_allocation_pointers[memory_cache_flag] < MAX_FLAGGED_MEMORY_ALLOCATION_POINTERS);
    flagged_memory_allocation_pointers[memory_cache_flag][num_flagged_memory_allocation_pointers[memory_cache_flag]++] = allocation_ptr;
}

/* This function simply allocates everything. */
VOID allocate_everything(ULONG memory_cache_flag)
{

// UX_MEMORY_BLOCK *first_memory_block;
UX_MEMORY_BLOCK *memory_block;
ULONG memory_block_size;
ULONG memory_block_requested_size;
VOID *tmp_allocation_ptr;
UX_MEMORY_BYTE_POOL *pool_ptr;

    if (memory_cache_flag == UX_REGULAR_MEMORY)
    {
        pool_ptr = (UX_MEMORY_BYTE_POOL *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR];
    }
    else
    {
        pool_ptr = (UX_MEMORY_BYTE_POOL *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE];
    }

    /* Try to allocate all the blocks according to their size - this is faster than repeatedly allocating 1 byte. */
    memory_block = (UX_MEMORY_BLOCK *)pool_ptr -> ux_byte_pool_start;
    while (memory_block -> ux_memory_block_next > memory_block)
    {
        if ((ULONG)memory_block->ux_memory_byte_pool == UX_BYTE_BLOCK_FREE)
        {
            memory_block_size = (UCHAR *)memory_block->ux_memory_block_next - (UCHAR*)memory_block;
            memory_block_requested_size = memory_requested_size_for_block(UX_NO_ALIGN, memory_block_size);
            if (memory_block_requested_size > 0)
            {
                tmp_allocation_ptr = ux_utility_memory_allocate(UX_NO_ALIGN, memory_cache_flag, memory_block_requested_size);
                UX_TEST_ASSERT(tmp_allocation_ptr != UX_NULL);
                add_memory_allocation_pointer(memory_cache_flag, tmp_allocation_ptr);

                /* memory_block is now the block we just allocated. */
                memory_block = (UX_MEMORY_BLOCK *)UX_UCHAR_POINTER_SUB(tmp_allocation_ptr, sizeof(UX_MEMORY_BLOCK));
            }
        }
        memory_block = memory_block->ux_memory_block_next;
    }

    /* Now allocate everything else. */
    while (1)
    {
        tmp_allocation_ptr = ux_utility_memory_allocate(UX_NO_ALIGN, memory_cache_flag, 1);
        if (tmp_allocation_ptr == NULL)
        {
            break;
        }
        add_memory_allocation_pointer(memory_cache_flag, tmp_allocation_ptr);
    }
}

/* The goal of this function is to ensure that allocating 'target_fail_level' amount of memory fails, and to ensure
   any allocations below 'target_fail_level' succeeds. An example usage is trying to get some allocation to fail in USBX
   where the allocation follows other allocations i.e. you want allocations X and Y to succeed, but Z to fail; by passing
   the sum of the sizes of X, Y, and Z to this function, it ensures X and Y will success and Z to fail.

   The gist of how we do this is to allocate memory for 'target_fail_level - 1', then allocate all other memory, then free
   the 'target_fail_level - 1' allocation. While that's the gist, it's quite oversimplified and there are nuances in the
   allocator that makes this a little more difficult, as is explained throughout. */
VOID  ux_test_utility_sim_mem_allocate_until_align_flagged(ULONG target_fail_level, ULONG memory_alignment, ULONG memory_cache_flag)
{

ULONG amount_added_by_allocator;
ULONG amount_added_by_alignment;
ULONG total_allocation_size;
ULONG memory_used;
UX_MEMORY_BLOCK *cur_memory_block;
UX_MEMORY_BLOCK *next_memory_block;
VOID *next_memory_block_buffer;
VOID *main_allocation_ptr;
UX_MEMORY_BYTE_POOL *pool_ptr;

    // printf("alloc_until(%ld, %ld, %ld)\n", target_fail_level, memory_alignment, memory_cache_flag);
    if (memory_cache_flag == UX_REGULAR_MEMORY)
    {
        pool_ptr = (UX_MEMORY_BYTE_POOL *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR];
    }
    else
    {
        pool_ptr = (UX_MEMORY_BYTE_POOL *)_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE];
    }
    /* Since we're going to generate at least one error, we need to ignore it. */
    ux_test_ignore_all_errors();

    /* Adjust level if invalid. */
    if (target_fail_level == 0)
        target_fail_level++;

    /* The allocator will align the requested size to a 16-byte boundary. This means that, instead of allocating 'target_fail_level - 1',
        we instead allocate 'target_fail_level - 16'. If the target_fail_level is less than 16, then we simply allocate everything. */
    if (target_fail_level <= (UX_ALIGN_MIN + 1))
    {
        allocate_everything(memory_cache_flag);
    }
    else
    {

        /* As previously said, we subtract 16 since the allocator aligns sizes to 16-bytes. */
        target_fail_level -= (UX_ALIGN_MIN + 1);

        /* As The header and the alignment is considered in ux_utility_memory_allocate function, we just need to allocate the request size here */
#if 0
        /* We have to figure out the size we need to allocate for the 'target_fail_level - 1' block. This is tricky because the allocator will
           find a memory block that fits the requested size plus some extra stuff. If the extra stuff isn't used, the allocator will create another
           memory block for it, so when we try to allocate the 'target_fail_level - 1 ' again, the memory block won't be big enough (remember, we allocate
           everything after doing this allocate, so the block of extra stuff will be allocated). So, we need to allocate 'target_fail_level - 1' plus
           the extra stuff so that when it's freed, the memory block will be big enough for 'target_fail_level - 1' to be allocated. */
        amount_added_by_allocator = get_amount_added_by_memory_allocator(memory_alignment, target_fail_level);
        amount_added_by_alignment = ((target_fail_level + UX_ALIGN_MIN) & (~(ULONG)UX_ALIGN_MIN)) - target_fail_level; /* The allocator aligns the requested size i.e. it adds some stuff. Take that into account now. */
        total_allocation_size = (target_fail_level) + amount_added_by_allocator + amount_added_by_alignment;
#else
        total_allocation_size = target_fail_level;
#endif
        main_allocation_ptr = ux_utility_memory_allocate(memory_alignment, memory_cache_flag, total_allocation_size);
        if (main_allocation_ptr != NULL)
        {
            allocate_everything(memory_cache_flag);

            /* The next memory block _must_ be USED so that we don't merge with it, since it will cause the 'target_fail_level - 1' block to become
               larger. The next memory block can be UNUSED if it's too small to be allocated by even a requested size of 1. */
            cur_memory_block = (UX_MEMORY_BLOCK *) main_allocation_ptr - 1;
            next_memory_block = cur_memory_block->ux_memory_block_next;
            if ((ULONG)next_memory_block->ux_memory_byte_pool == UX_BYTE_BLOCK_FREE)
            {
                next_memory_block->ux_memory_byte_pool = pool_ptr;
                next_memory_block_buffer = (VOID *) (next_memory_block + 1);
                add_memory_allocation_pointer(memory_cache_flag, next_memory_block_buffer);

                memory_used = (UCHAR *)next_memory_block->ux_memory_block_next - (UCHAR*)next_memory_block;
                if (_ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_start == _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_start)
                {
                    _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available -= memory_used;
                }
                else
                {
                    switch (memory_cache_flag)
                    {
                        case UX_CACHE_SAFE_MEMORY:
                            _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_CACHE_SAFE] -> ux_byte_pool_available -= memory_used;
                            break;
                        default:
                            _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available -= memory_used;
                            break;
                    }
                }
            }

            /* Now free the 'target_fail_level - 1' block. */
            ux_utility_memory_free(main_allocation_ptr);
        }
    }

    ux_test_unignore_all_errors();
}

VOID  ux_test_utility_sim_mem_free_all_flagged(ULONG memory_cache_flag)
{

ULONG i;

    for (i = 0; i < num_flagged_memory_allocation_pointers[memory_cache_flag]; i++)
    {
        ux_utility_memory_free(flagged_memory_allocation_pointers[memory_cache_flag][i]);
    }
    num_flagged_memory_allocation_pointers[memory_cache_flag] = 0;
}

VOID ux_test_utility_sim_mem_test(VOID)
{
ULONG mem_level = 400;
ULONG alloc_size;
VOID* tmp_alloc;

    printf("Test memory level: %ld\n", mem_level);
    ux_test_utility_sim_mem_allocate_until(mem_level);

    alloc_size = mem_level;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size %ld: %p\n", alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    alloc_size = mem_level >> 1;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size %ld: %p\n", alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    alloc_size = mem_level - 4;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size -%ld(%ld): %p\n", mem_level - alloc_size, alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    alloc_size = mem_level - 8;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size -%ld(%ld): %p\n", mem_level - alloc_size, alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    alloc_size = mem_level - 16;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size -%ld(%ld): %p\n", mem_level - alloc_size, alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    alloc_size = mem_level - 32;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size -%ld(%ld): %p\n", mem_level - alloc_size, alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    alloc_size = mem_level - 64;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size -%ld(%ld): %p\n", mem_level - alloc_size, alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    alloc_size = mem_level - 128;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size -%ld(%ld): %p\n", mem_level - alloc_size, alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    alloc_size = mem_level - 256;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, alloc_size);
    printf("Allocate size -%ld(%ld): %p\n", mem_level - alloc_size, alloc_size, tmp_alloc);
    if (tmp_alloc) ux_utility_memory_free(tmp_alloc);

    ux_test_utility_sim_mem_free_all();

    printf("alloc resolution:\n");
    mem_level = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, 4);
    printf("alloc 4 bytes: %ld -> %ld (%ld)\n", mem_level, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, mem_level - _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    ux_utility_memory_free(tmp_alloc);

    mem_level = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, 16);
    printf("alloc 16 bytes: %ld -> %ld (%ld)\n", mem_level, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, mem_level - _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    ux_utility_memory_free(tmp_alloc);

    mem_level = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, 64);
    printf("alloc 64 bytes: %ld -> %ld (%ld)\n", mem_level, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, mem_level - _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    ux_utility_memory_free(tmp_alloc);

    mem_level = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, 100);
    printf("alloc 100 bytes: %ld -> %ld (%ld)\n", mem_level, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, mem_level - _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    ux_utility_memory_free(tmp_alloc);

    mem_level = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available;
    tmp_alloc = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, 200);
    printf("alloc 200 bytes: %ld -> %ld (%ld)\n", mem_level, _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available, mem_level - _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR] -> ux_byte_pool_available);
    ux_utility_memory_free(tmp_alloc);

    printf("Halt this thread!\n");
    while(1)
    {

        tx_thread_sleep(10);
    }
}
