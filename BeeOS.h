#ifndef BEEOS_H
#define BEEOS_H

#define MAX_TASK_COUNT  32

#define INVALID_TID     0xFFFFFFFF


#define E_OK                  (0)
#define E_INVALID_HANDLE      (-1)
#define E_INVALID_TID         (-2)
#define E_BUFFER_TOO_SMALL    (-3)
#define E_TIME_OUT            (-4)
#define E_INVALID_VALUE       (-5)
#define E_OUT_OF_MEMORY       (-6)

#define INVALID_HANDLE         ((uint32_t) E_INVALID_HANDLE)

#define READY_TASK            0x00000000
#define SLEEP_TASK            0x00010000
#define WAITING_HANDLE        0x00020000
#define SUSPEND_TASK          0x00040000
#define IDLE_TASK             0x00080000
#define ZOMBIE_TASK           0x80000000

#define CREATE_SUSPENDED      SUSPEND_TASK
#define CREATE_IDLE_TASK      IDLE_TASK


#define HANDLE_TYPE_MUTEX           0x10000000
#define HANDLE_TYPE_SEMAPHORE       0x20000000
#define HANDLE_TYPE_MAILBOX         0x30000000
#define HANDLE_TYPE_MAILBOX_READ    0x40000000
#define HANDLE_TYPE_MAILBOX_WRITE   0x50000000


#define HANDLE_OWNERLESS           0x00000000
#define HANDLE_TASK_ID_MASK        0x0000FFFF
#define HANDLE_TYPE_MASK           0xFFFF0000
#define HANDLE_ID_CRITICAL_SECTION CRITICAL_SECTION_TASK

#define INFINITE 0xFFFFFFFF  

#define WAIT_FOR_SINGLE_OBJECT    0
#define WAIT_FOR_MULTIPLE_OBJECTS 1

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
typedef uint32_t task_set_t;

typedef struct
{
  uint32_t type;
  uint32_t time_out;
  int32_t result;
  union
  {
    HANDLE   handle;
    HANDLE*  handles;
  };
  uint32_t waiting_handles;
  uint32_t still_waiting_handles;  
  uint32_t size;
  union
  {  
    uint32_t wait_all; 
    void*    buffer;
  };
} wait_for_object_t;

typedef struct
{
  uint32_t* SP;
  uint32_t  State;
  uint32_t* Stack;
  LPTHREAD_START_ROUTINE StartAddress;
  unsigned long StackSize;
  void*    Parameter;
  uint32_t SleepParam;
  wait_for_object_t*    RequestStruct;
  int32_t  result;
} TCB_t;

typedef struct
{
  uint32_t   type;
  task_set_t waiting_tasks; 
  task_set_t waiting_for_multiple_tasks;   
}handle_base_t;


typedef struct
{
  handle_base_t base;
  tid_t      owner;  
}mutex_t;

typedef struct
{
  handle_base_t base; 
  uint32_t   semaphore_count;
  uint32_t   semaphore_max_count;
}semaphore_t;

typedef struct
{
  HANDLE   handle; 
  uint32_t handle_type;
  uint32_t release_count;
  int32_t  result;
}release_object_t;

typedef struct
{
  uint32_t               StackSize;
  LPTHREAD_START_ROUTINE StartAddress;
  void*                  Parameter;
  uint32_t               CreationFlags;
  uint32_t               TaskId;
}create_task_t;

typedef struct
{
  uint32_t size;
  uint32_t data[];
}mailbox_packet_t;

typedef struct
{ 
  uint32_t  maxmsg;
  uint32_t  msgsize;
  uint32_t  paket_size;
  void*     buffer;
  uint32_t  counter;
  mailbox_packet_t*  read_packet;
  mailbox_packet_t*  write_packet;  
}mailbox_base_t;

typedef struct mailbox_slot_t
{  
  handle_base_t           base; 
  mailbox_base_t*         mailbox_base;  
  uint32_t                idx;
  tid_t                   owner; 
  struct mailbox_slot_t*  opposite_slot;
  mailbox_packet_t*       packet;  
} mailbox_slot_t;



typedef struct
{
  mailbox_base_t mailbox_base;
  mailbox_slot_t read_slot;
  mailbox_slot_t write_slot;  
}mailbox_t;

typedef struct
{
  uint32_t maxmsg;
  uint32_t msgsize;
  HANDLE*  read_slot;
  HANDLE*  write_slot;
  int32_t  result;
}create_mailbox_t;

void InitStack(TCB_t* TCB);
void StartFirstTask();


void Sleep(unsigned long SleepTime);


static uint32_t SVCHandler_main(uint32_t param, uint32_t svc_id);


//HANDLE GetCurrentThread();

void ResumeTask (tid_t tid);
int32_t WaitForSingleObject (HANDLE handle, uint32_t time_out);
int32_t WaitForMultipleObjects (uint32_t count, HANDLE* handle_array,uint32_t wait_all, uint32_t time_out);


HANDLE CreateMutex(uint32_t InitialOwner);
int32_t ReleaseMutex (HANDLE handle);

HANDLE CreateSemaphore (uint32_t InitialCount, uint32_t MaximumCount);
int32_t ReleaseSemaphore (HANDLE handle, uint32_t ReleaseCount);

int32_t CreateMailBox(HANDLE* read_slot, HANDLE* write_slot, uint32_t maxmsg, uint32_t msgsize);
int32_t SendMessage(HANDLE handle,uint32_t size, void* buffer, uint32_t time_out);
int32_t GetMessage(HANDLE handle,uint32_t size, void* buffer, uint32_t time_out);

tid_t CreateTask(uint32_t               StackSize, 
                 LPTHREAD_START_ROUTINE StartAddress, 
                 void*                  Parameter,
                 uint32_t               CreationFlags);


/////////////////////////////////////////////////////////////////////////////
void coreSheduler(unsigned long bSysTymer);


#endif