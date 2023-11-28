#include <stdio.h>
#include "tx_api.h"
#include "ux_api.h"
#include "ux_system.h"
#include "ux_utility.h"
#include "ux_hcd_sim_host.h"
#include "ux_test.h"
#include "ux_test_utility_sim.h"

#define                             UX_DEMO_STACK_SIZE              2048
#define                             UX_DEMO_MEMORY_SIZE             (256*1024)
#define                             UX_DEMO_BUFFER_SIZE             2048

UCHAR usbx_memory[UX_DEMO_MEMORY_SIZE + (UX_DEMO_STACK_SIZE * 2)];

#ifdef CTEST
void test_application_define(void *first_unused_memory)
#else
void    usbx_ux_utility_basic_memory_management_test_application_define(void *first_unused_memory)
#endif
{

UINT status = 0;
CHAR *stack_pointer;
CHAR *memory_pointer;
CHAR *pointer_1;
CHAR *pointer_2;
CHAR *pointer_3;
CHAR *pointer_4;
ULONG memory_size;
ULONG array[20];
UX_MEMORY_BYTE_POOL *pool_ptr;


    /* Inform user.  */
    printf("Running USB Utility Basic Memory Management Converge Coverage Test . ");

    /* Initialize the free memory pointer */
    stack_pointer = (CHAR *) usbx_memory;
    memory_pointer = stack_pointer + (UX_DEMO_STACK_SIZE * 2);


    /* Initialize USBX. Memory */
    status = ux_system_initialize(memory_pointer, UX_DEMO_MEMORY_SIZE, UX_NULL,0);
    if (status != UX_SUCCESS)
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(status);
        return;
    }

    pool_ptr = _ux_system -> ux_system_memory_byte_pool[UX_MEMORY_BYTE_POOL_REGULAR];
#ifdef UX_ENFORCE_SAFE_ALIGNMENT
    memory_size = UX_ALIGN_8 - 1;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);
    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_8;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);
    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_16;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_32;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_64;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_128;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_256;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_512;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_1024;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_2048;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_ALIGN_2048 + 1;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & UX_ALIGN_4096))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);

    memory_size = UX_MAX_SCATTER_GATHER_ALIGNMENT + 1;
    pointer_1 = ux_utility_memory_allocate(UX_SAFE_ALIGN, UX_REGULAR_MEMORY, memory_size);

    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & (UX_MAX_SCATTER_GATHER_ALIGNMENT-1)))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);
#endif

    /* Test NULL pointer release.  */
    ux_utility_memory_free(TX_NULL);

    /* Test another bad block release... no pool pointer!  */
    array[0] =  0;
    array[1] =  0;
    array[2] =  0;
    ux_utility_memory_free(&array[2]);

    /* Test another bad block release.... pool pointer is not a valid pool!  */
    array[0] =  0;
    array[1] =  (ULONG) &array[3];
    array[2] =  0;
    array[3] =  0;
    ux_utility_memory_free(&array[2]);

    /* Re-Release the same block */
    memory_size = UX_ALIGN_8 - 1;
    pointer_1 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);
    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & memory_size))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_1);
    ux_utility_memory_free(pointer_1);

    /* Allocate each block again to make sure everything still
       works.  */

    /* Allocate memory from the pool.  */
    memory_size = 24;
    pointer_1 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    /* Allocate second memory area from the pool.  */
    memory_size = 24;
    pointer_2 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_2 == UX_NULL) || (((ULONG)pointer_2) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }

    /* Allocate third memory area from the pool.  */
    memory_size = 24;
    pointer_3 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_3 == UX_NULL) || (((ULONG)pointer_3) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }

    /* Attempt to allocate fourth memory area from the pool.  This should fail because
       there should be not enough bytes in the pool.  */
    memory_size = 24;
    pointer_4 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_4 == UX_NULL) || (((ULONG)pointer_4) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }

    /* Now release each of the blocks. */
    ux_utility_memory_free(pointer_1);
    ux_utility_memory_free(pointer_2);
    ux_utility_memory_free(pointer_3);

    /* Now allocate a block that should cause all of the blocks to merge
       together.  */
    memory_size = 88;
    pointer_3 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_3 == UX_NULL) || (((ULONG)pointer_3) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }
    ux_utility_memory_free(pointer_3);
    ux_utility_memory_free(pointer_4);

    /* Check the allocated address is the same */
    /* Allocate memory from the pool.  */
    memory_size = 24;
    pointer_1 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_1 == UX_NULL) || (((ULONG)pointer_1) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }

    /* Allocate second memory area from the pool.  */
    memory_size = 24;
    pointer_2 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_2 == UX_NULL) || (((ULONG)pointer_2) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }

    /* Allocate second memory area from the pool.  */
    memory_size = 24;
    pointer_3 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_3 == UX_NULL) || (((ULONG)pointer_3) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }

    /* Release the middle block.  */
    ux_utility_memory_free(pointer_1);

    /* Now allocate the block again.  */
    memory_size = 24;
    pointer_4 = ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, memory_size);

    /* Check status.  */
    if ((pointer_4 == UX_NULL) || (((ULONG)pointer_4) & UX_ALIGN_MIN))
    {
        printf("Line:%s, ux_utility_memory_allocate failed!\n", __LINE__);
        test_control_return(1);
        return;
    }

    if (pointer_1 != pointer_4)
    {
        printf("Line:%s, Allocated address is not the same!\n", __LINE__);
        test_control_return(1);
        return;
    }

    /* Now release the blocks are test the merge with the update of the search pointer.  */
    ux_utility_memory_free(pointer_3);
    ux_utility_memory_free(pointer_2);
    ux_utility_memory_free(pointer_4);

    printf("SUCCESS!\n");

    test_control_return(0);
    return;
}
