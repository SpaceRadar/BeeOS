#ifndef BEEOS_H
#define BEEOS_H

#define STACK_SIZE      64
#define MAX_TASK_COUNT  32

#define INVALID_TID     0xFFFFFFFF

#define READY_TASK            0x00000000
#define SLEEP_TASK            0x00010000
#define WAITING_HANDLE        0x00020000
#define SUSPEND_TASK          0x00040000
#define IDLE_TASK             0x00080000
#define ZOMBIE_TASK           0x80000000

#define CREATE_SUSPENDED      SUSPEND_TASK
#define CREATE_IDLE_TASK      IDLE_TASK


#define HANDLE_TYPE_MUTEX     0x20000000
#define HANDLE_TYPE_SEMAPHORE 0x40000000


#define HANDLE_OWNERLESS           0x00000000
#define HANDLE_TASK_ID_MASK        0x0000FFFF
#define HANDLE_TYPE_MASK           0xFFFF0000
#define HANDLE_ID_CRITICAL_SECTION CRITICAL_SECTION_TASK

#define INFINITE 0xFFFFFFFF  

/* Constants required to manipulate the NVIC. */
#define portNVIC_SYSTICK_CTRL		( ( volatile unsigned long *) 0xe000e010 )
#define portNVIC_SYSTICK_LOAD		( ( volatile unsigned long *) 0xe000e014 )
#define portNVIC_INT_CTRL			( ( volatile unsigned long *) 0xe000ed04 )
#define portNVIC_SYSPRI2			( ( volatile unsigned long *) 0xe000ed20 )
#define portNVIC_SYSTICK_CLK		0x00000004
#define portNVIC_SYSTICK_INT		0x00000002
#define portNVIC_SYSTICK_ENABLE		0x00000001
#define portNVIC_PENDSVSET			0x10000000
//#define portNVIC_PENDSV_PRI			( ( ( unsigned long ) configKERNEL_INTERRUPT_PRIORITY ) << 16 )
#define portNVIC_SYSTICK_PRI		( ( ( unsigned long ) configKERNEL_INTERRUPT_PRIORITY ) << 24 )

#define configPRIO_BITS       5        /* 32 priority levels */

#ifndef NULL
#define NULL 0
#endif


typedef void (*LPTHREAD_START_ROUTINE) (void*);
typedef uint32_t tid_t;
typedef uint32_t HANDLE;
typedef struct
{
  uint32_t* SP;
  uint32_t State;
  LPTHREAD_START_ROUTINE StartAddress;
  unsigned long StackSize;
  void* Parameter;
  uint32_t SleepParam; 
} TCB_t;

typedef struct
{
  unsigned long Owner;
  int LockCounter;
  unsigned long TaskWaiting;
  unsigned long LowestWaitPriority;  
  unsigned long TopWaitPriority;    
}CRITICAL_SECTION_t;

typedef struct
{
  uint32_t TypeAndOwner;
  uint32_t waiting_tasks; 
}mutex_t;

typedef struct
{
  uint32_t TypeAndOwner;
  uint32_t waiting_tasks; 
  uint32_t semaphore_count;
  uint32_t semaphore_max_count;
}semaphore_t;

typedef struct
{
  uint32_t               StackSize;
  LPTHREAD_START_ROUTINE StartAddress;
  void*                  Parameter;
  uint32_t               CreationFlags;
  uint32_t               TaskId;
}create_task_t;

typedef unsigned long  CRITICAL_SECTION;
typedef unsigned long* LPCRITICAL_SECTION;

void InitStack(TCB_t* TCB);
void StartFirstTask();


void Sleep(unsigned long SleepTime);

int InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
int EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
int LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);


static uint32_t SVCHandler_main(uint32_t param, uint32_t svc_id);


//HANDLE GetCurrentThread();

void ResumeTask (tid_t tid);
void WaitForSingleObject (HANDLE handle);

HANDLE CreateMutex(uint32_t InitialOwner);
void ReleaseMutex (HANDLE handle);

HANDLE CreateSemaphore (uint32_t InitialCount, uint32_t MaximumCount);
void ReleaseSemaphore (HANDLE handle, uint32_t ReleaseCount);

tid_t CreateTask(uint32_t               StackSize, 
                 LPTHREAD_START_ROUTINE StartAddress, 
                 void*                  Parameter,
                 uint32_t               CreationFlags);

tid_t CreateTaskPrev(uint32_t               StackSize, 
                     LPTHREAD_START_ROUTINE StartAddress, 
                     void*                  Parameter,
                     uint32_t               CreationFlags);

tid_t CreateIdleTask(uint32_t               StackSize, 
                     LPTHREAD_START_ROUTINE StartAddress, 
                     void*                  Parameter,
                     uint32_t               CreationFlags,
                     uint32_t               TaskId);


#endif