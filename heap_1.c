#include <stdint.h>
#include "heap.h"

#define configTOTAL_HEAP_SIZE 4096
//#define portBYTE_ALIGNMENT_MASK 0x00000003
//#define portBYTE_ALIGNMENT 4

#define portBYTE_ALIGNMENT 8
#define portBYTE_ALIGNMENT_MASK ( 0x0007 )

#define portPOINTER_SIZE_TYPE uint32_t

/* A few bytes might be lost to byte aligning the heap start address. */
#define configADJUSTED_HEAP_SIZE	( configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT )

/* Allocate the memory for the heap. */
/* Allocate the memory for the heap. */
#if( configAPPLICATION_ALLOCATED_HEAP == 1 )
	/* The application writer has already defined the array used for the RTOS
	heap - probably so it can be placed in a special segment or address. */
	extern uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#else
	static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif /* configAPPLICATION_ALLOCATED_HEAP */

static uint32_t xNextFreeByte = ( uint32_t ) 0;

/*-----------------------------------------------------------*/

void* malloc( uint32_t xWantedSize )
{
void *pvReturn = NULL;
static uint8_t *pucAlignedHeap = NULL;

	/* Ensure that blocks are always aligned to the required number of bytes. */
	#if( portBYTE_ALIGNMENT != 1 )
	{
		if( xWantedSize & portBYTE_ALIGNMENT_MASK );
		{
			/* Byte alignment required. */
			xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
		}
	}
	#endif

//	vTaskSuspendAll();
	{
		if( pucAlignedHeap == NULL )
		{
			/* Ensure the heap starts on a correctly aligned boundary. */
			pucAlignedHeap = ( uint8_t * ) ( ( ( portPOINTER_SIZE_TYPE ) &ucHeap[ portBYTE_ALIGNMENT ] ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );                  
		}

		/* Check there is enough room left for the allocation. */
		if( ( ( xNextFreeByte + xWantedSize ) < configADJUSTED_HEAP_SIZE ) &&
			( ( xNextFreeByte + xWantedSize ) > xNextFreeByte )	)/* Check for overflow. */
		{
			/* Return the next free byte then increment the index past this
			block. */
			pvReturn = pucAlignedHeap + xNextFreeByte;
			xNextFreeByte += xWantedSize;
		}

//		traceMALLOC( pvReturn, xWantedSize );
	}
//	( void ) xTaskResumeAll();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
	}
	#endif

	return pvReturn;
}
/*-----------------------------------------------------------*/

void free( void *pv )
{
	( void ) pv;

	/* Force an assert as it is invalid to call this function. */
//	configASSERT( pv == NULL );
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
	/* Only required when static memory is not cleared. */
	xNextFreeByte = ( uint32_t ) 0;
}
/*-----------------------------------------------------------*/

uint32_t xPortGetFreeHeapSize( void )
{
	return ( configADJUSTED_HEAP_SIZE - xNextFreeByte );
}



