#ifndef BEEOS_H
#define BEEOS_H

#define STACK_SIZE 64


#define RUN_TASK              0x00000000
#define SLEEP_TASK            0x00010000
#define CRITICAL_SECTION_TASK 0x00020000
#define CREATE_SUSPENDED      0x00030000
#define ZOMBIE_TASK           0x00040000

#define HANDLE_MASK                0x0000FFFF
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
#define portNVIC_PENDSV_PRI			( ( ( unsigned long ) configKERNEL_INTERRUPT_PRIORITY ) << 16 )
#define portNVIC_SYSTICK_PRI		( ( ( unsigned long ) configKERNEL_INTERRUPT_PRIORITY ) << 24 )

#define configPRIO_BITS       5        /* 32 priority levels */


typedef void (*LPTHREAD_START_ROUTINE) (void);
typedef unsigned long HANDLE;
struct TCB_t
{
  unsigned long* SP;
  unsigned long State;
  LPTHREAD_START_ROUTINE lpStartAddress;
  unsigned long StackSize;
  unsigned long SleepParam;
  unsigned long WaitPriority;  
};

typedef struct
{
  unsigned long Owner;
  int LockCounter;
  unsigned long TaskWaiting;
  unsigned long LowestWaitPriority;  
  unsigned long TopWaitPriority;    
}CRITICAL_SECTION_t;

typedef unsigned long   CRITICAL_SECTION;
typedef unsigned long* LPCRITICAL_SECTION;

void InitStack(unsigned long taskId, LPTHREAD_START_ROUTINE lpStartAddress);
void StartFirstTask();


void Sleep(unsigned long SleepTime);

int InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
int EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
int LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);

HANDLE GetCurrentThread();
void ResumeThread (HANDLE hThread);

#endif