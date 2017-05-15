#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include <core_cm4.h>

#include "BeeOS.h"
#include "BeeOS_Tasks.h"

#include "heap.h"

#define IDLE_TASK_ID 0
TCB_t* TCB[MAX_TASK_COUNT];

static unsigned long ReadyTasksCurrent=0;
static unsigned long ReadyTasks=0;
static unsigned long SleepTasks=0;
//static unsigned long CriticalSection=0;
//static unsigned long CriticalSectionUsing=0xFFFFFFFF;
//static CRITICAL_SECTION_t CRITICAL_SECTION_Array[32];


static volatile TCB_t* current_task;
static volatile uint32_t task_count=1;
static volatile uint32_t first_wait_task_id=0;
static volatile uint32_t last_wait_task_id=0;
static volatile unsigned long current_task_id;

static volatile unsigned long addr;
/*
HANDLE* GetCurrentThread()
{
  return NULL;//current_task_id;
}
*/
void Sleep(unsigned long SleepTime)
{
  __asm( 
        "     isb      \n"          
        "     svc   1  \n"
       );  
}

void BeyondTheTask(void)
{
  __asm( 
        "     isb      \n"          
        "     svc   2  \n"
       );  
}

int InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
  __asm( 
        "     isb      \n"          
        "     svc   3  \n"
       );
  if (0xFFFFFFFF==*lpCriticalSection)
    return -1;
  else
    return 0; 
}

int EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
  __asm( 
        "     isb      \n"          
        "     svc   4  \n"
       );    
  return 0; 
}

int LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
  __asm( 
        "     isb      \n"          
        "     svc   5  \n"
       );    
  return 0; 
}

void ResumeTask (tid_t tid)
{
  __asm( 
        "     isb      \n"          
        "     svc   6  \n"
       );    
}

void WaitForSingleObject (HANDLE handle)
{
  __asm( 
        "     isb      \n"          
        "     svc   7  \n"
       );    
}

void ReleaseMutex (HANDLE handle)
{
  __asm( 
        "     isb      \n"          
        "     svc   8  \n"
       );    
}

void ReleaseSemaphore (HANDLE handle, uint32_t ReleaseCount)
{
  __asm( 
        "     isb       \n"          
        "     svc   8  \n"
       );    
}

void CreateTaskAdapter(create_task_t* create_task)
{
  __asm( 
        "     isb      \n"          
        "     svc   9  \n"
       );   
}

#pragma diag_suppress=Pe940 
void* AllocateMem(uint32_t size)
{
  __asm( 
        "     isb       \n"          
        "     svc   129 \n"
       );    
}
#pragma diag_default=Pe940


HANDLE CreateMutex(uint32_t InitialOwner)
{
  mutex_t* handle=AllocateMem(sizeof(mutex_t));
  if(handle)
  {
    if(InitialOwner)
        handle->TypeAndOwner=HANDLE_TYPE_MUTEX | current_task_id;
    else
        handle->TypeAndOwner=HANDLE_TYPE_MUTEX | HANDLE_OWNERLESS;  
    handle->waiting_tasks=0;
  }
  return (HANDLE) handle;
}

HANDLE CreateSemaphore(uint32_t InitialCount, uint32_t MaximumCount)
{
  semaphore_t* handle=AllocateMem(sizeof(semaphore_t));
  if(handle && (InitialCount<=MaximumCount) )
  {
      handle->TypeAndOwner=HANDLE_TYPE_SEMAPHORE | HANDLE_OWNERLESS;  
      handle->semaphore_count=InitialCount;
      handle->semaphore_max_count=MaximumCount;
      handle->waiting_tasks=0;
  }
  return (HANDLE) handle;
}


tid_t CreateTask(uint32_t               StackSize, 
                 LPTHREAD_START_ROUTINE StartAddress, 
                 void*                  Parameter,
                 uint32_t               CreationFlags)
{
  create_task_t create_task;
  
  create_task.StackSize=StackSize;
  create_task.StartAddress=StartAddress;
  create_task.Parameter=Parameter;
  create_task.CreationFlags=CreationFlags;
  CreateTaskAdapter(&create_task);  
  
  return create_task.TaskId;
}

tid_t CreateTaskPrev(uint32_t               StackSize, 
                     LPTHREAD_START_ROUTINE StartAddress, 
                     void*                  Parameter,
                     uint32_t               CreationFlags)
{
  create_task_t create_task;
  
  create_task.StackSize=StackSize;
  create_task.StartAddress=StartAddress;
  create_task.Parameter=Parameter;
  create_task.CreationFlags=CreationFlags;  
  SVCHandler_main((uint32_t) &create_task, 9);
  return create_task.TaskId;  
}



void InitStack(TCB_t* TCB)
{
  uint32_t* SP=&TCB->SP[STACK_SIZE>>2];
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)  
  *(--SP)= 0x00000000;
  *(--SP)= 0x02000000;  //FPSCR
  *(--SP)= 0x41700000;
  *(--SP)= 0x41600000;  
  *(--SP)= 0x41500000;
  *(--SP)= 0x41400000;  
  *(--SP)= 0x41300000;
  *(--SP)= 0x41200000;
 
  *(--SP)= 0x41100000;
  *(--SP)= 0x41000000;  
  *(--SP)= 0x40e00000;
  *(--SP)= 0x40c00000;  
  *(--SP)= 0x40a00000;
  *(--SP)= 0x40800000; 
  *(--SP)= 0x40400000;
  *(--SP)= 0x40000000;  
  *(--SP)= 0x3f800000;
  *(--SP)= 0x00000000;  
#endif
  
  *(--SP)=0x01000000;
  *(--SP)= (uint32_t) TCB->StartAddress;
  *(--SP)= (uint32_t) BeyondTheTask;
  *(--SP)= 0xCCCCCCCC;
  *(--SP)= 0x33333333;
  *(--SP)= 0x22222222;
  *(--SP)= 0x11111111;
  *(--SP)= (uint32_t) TCB->Parameter;// 0x00000000;
  

  *(--SP)= 0xBBBBBBBB;  
  *(--SP)= 0xAAAAAAAA;
  *(--SP)= 0x99999999;  
  *(--SP)= 0x88888888;
  
  *(--SP)= 0x77777777;  
  *(--SP)= 0x66666666;
  *(--SP)= 0x55555555;  
  *(--SP)= 0x44444444;
  
  TCB->SP= SP;
}

void SysTick_Handler(void)
{
  *(portNVIC_INT_CTRL) = portNVIC_PENDSVSET;  
}

void PendSV_Handler(void)
{
  __asm(   
          "     mrs     r3,psp                  \n"            
          "     ldr     r1,[r3,#24]             \n"
          "     ldrb    r1,[r1,#-2]             \n"

            
          "     ldr.n   r2,current_task_local   \n"         
          "     ldr     r2,[r2]                 \n"                       
            
          "    	push    {lr}                    \n"             
          "    	stmdb   r3!, {r4-r11}           \n"            
          "     str     r3,[r2]                 \n"   

          "     ldr.n   r3,coreSheduler_local   \n"               
          "     mov     r0,#1                   \n"  
          "     blx     r3                      \n"  
            
          "    	pop     {lr}                    \n"                             
          "     ldr.n   r2,current_task_local   \n"         
          "     ldr     r2,[r2]                 \n"
          "     ldr     r2,[r2]                 \n"            
          "	ldmia   r2!, {r4-r11}           \n"               
          "     msr     psp, r2                 \n"           
          "	orr     r14, r14, #13           \n"
          "     bx      lr                      \n" 
          "     dc16    0                       \n"  
          "coreSheduler_local:                  \n"
          "     dc32    coreSheduler            \n"              
          "current_task_local:                  \n"
          "     dc32    current_task            \n"             
       );  
}

void coreSheduler(unsigned long bSysTymer)
{
  unsigned long SleepTasksTemp;
  unsigned long idx;
  
  SleepTasksTemp=SleepTasks;
  while(bSysTymer && SleepTasksTemp)
  {
    idx=__CLZ(SleepTasksTemp);
    SleepTasksTemp^=(0x80000000>>idx); 
    TCB[idx]->SleepParam--;
    if(!TCB[idx]->SleepParam)
    {
      SleepTasks^=(0x80000000>>idx); 
      ReadyTasks|=(0x80000000>>idx);            
      ReadyTasksCurrent|=(0x80000000>>idx);            
    }     
  }
  
  if(0==(ReadyTasks | ReadyTasksCurrent))
  {
    current_task=TCB[IDLE_TASK_ID];          
  }
  else
  {
    if(!ReadyTasksCurrent)
    {
      ReadyTasksCurrent=ReadyTasks;
    }
    current_task_id=__CLZ(ReadyTasksCurrent);
    ReadyTasksCurrent^=(0x80000000>>current_task_id); 
    current_task=TCB[current_task_id];        
    }   
}


void StartFirstTask()
{
//  uint32_t idx;
  CreateTaskPrev(64,IdleTask,(void*) 0, CREATE_IDLE_TASK);
/*  
  for(idx=0;TCB[idx]->SP;++idx)
  {
    InitStack(idx,TCB[idx]->lpStartAddress, TCB[idx]->Param); 
    if(idx>0)
    {
      if ( !(TCB[idx]->State & CREATE_SUSPENDED))
      {        
        ReadyTasks|=0x80000000 >> idx;
      }
    }
  }
  
  ReadyTasksCurrent=ReadyTasks;
*/  
//  Sheduler();
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)  
  *((__IO uint32_t*) 0xE000EF34)|=0x80000000;  
  *((__IO uint32_t*) 0xE000EF34)&=0xBFFFFFFF;    
#endif  
  __asm( 
	"     ldr.n r0, vtor_addr  \n"
	"     ldr r0, [r0]         \n"
	"     ldr r0, [r0]         \n"
	"     msr msp, r0          \n"   
        "     cpsie i              \n"
        "     svc   0              \n"
//        "     dc16  0x0000         \n"          //Just for align data  
//        "     dc16  0x0000         \n"          //Just for align data            
        "vtor_addr:                 \n"
        "     dc32  0xE000ED08     \n"          //Vector Table Offset Register
       );
}

void* coreAllocateMem(uint32_t size)
{
  return malloc(size);
}



static uint32_t SVCHandler_main(uint32_t param, uint32_t svc_id)
{
//  uint32_t idx;
//  uint32_t cs_idx;
//  uint32_t WakingTask;
  uint32_t task_id;
  uint32_t result=0; 
  switch(svc_id)
  {
    //Sleep
    case 1:
      if(param)
      {  
        ReadyTasks&=~(0x80000000>>current_task_id);          
        if(INFINITE==param)
        {
          current_task->State|=SUSPEND_TASK;
        }
        else
        {          
          current_task->State|=SLEEP_TASK;          
          current_task->SleepParam=param;
          SleepTasks|=0x80000000>>current_task_id;     
        }
      }  
      ReadyTasksCurrent&=~(0x80000000>>current_task_id);
      result=0xFFFFFFFF; 
      break;
    case 2:
      current_task->State=ZOMBIE_TASK;
//      SleepTasks|=0x80000000>>current_task_id; 
      ReadyTasksCurrent&=~(0x80000000>>current_task_id);
      ReadyTasks&=~(0x80000000>>current_task_id);      
      result=0xFFFFFFFF;       
      break;
/*      
    case 3:
      
        cs_idx=__CLZ(CriticalSectionUsing);
        if(cs_idx<32)
        {
          CriticalSectionUsing&=~(0x80000000>>cs_idx);
          *((LPCRITICAL_SECTION) param)=HANDLE_ID_CRITICAL_SECTION | cs_idx;
          CRITICAL_SECTION_Array[cs_idx].LockCounter=0;          
          CRITICAL_SECTION_Array[cs_idx].Owner=0xFFFFFFFF; 
          CRITICAL_SECTION_Array[cs_idx].LowestWaitPriority=0;
          CRITICAL_SECTION_Array[cs_idx].TopWaitPriority=0;
        }
        else
        {
          *((LPCRITICAL_SECTION) param)=0xFFFFFFFF;          
        }
      break;
    case 4:
//      cs_idx=*(LPCRITICAL_SECTION)param & TASK_ID_MASK;
      if(CRITICAL_SECTION_Array[cs_idx].Owner==0xFFFFFFFF || 
         CRITICAL_SECTION_Array[cs_idx].Owner==current_task_id) 
      {  
          CRITICAL_SECTION_Array[cs_idx].Owner=current_task_id;
          ++CRITICAL_SECTION_Array[cs_idx].LockCounter;
      }
      else
      {
        CRITICAL_SECTION_Array[cs_idx].TaskWaiting|=(0x8000000>>current_task_id);
        ReadyTasksCurrent&=~(0x80000000>>current_task_id);
        ReadyTasks&=~(0x80000000>>current_task_id);  
        current_task->WaitPriority=CRITICAL_SECTION_Array[cs_idx].LowestWaitPriority++;
        result=0xFFFFFFFF;        
      }
      break;         
    case 5:
      //cs_idx=*(LPCRITICAL_SECTION)param & TASK_ID_MASK;
      if(CRITICAL_SECTION_Array[cs_idx].Owner==current_task_id) 
      {          
        if(CRITICAL_SECTION_Array[cs_idx].LockCounter)
        {  
          if(!(--CRITICAL_SECTION_Array[cs_idx].LockCounter))
          {
            CRITICAL_SECTION_Array[cs_idx].Owner=0xFFFFFFFF;
            ++CRITICAL_SECTION_Array[cs_idx].LockCounter;
            WakingTask=0;
            while(!WakingTask)
            {
              idx=__CLZ(CRITICAL_SECTION_Array[cs_idx].TaskWaiting);              
              if(TCB[idx].WaitPriority==CRITICAL_SECTION_Array[cs_idx].TopWaitPriority)
              {
                WakingTask=0x80000000>>idx;
              }              
            }
          }
        }
      }
      break; 
*/      
    case 6:
      if(TCB[param]->State & SLEEP_TASK)
      {
        TCB[param]->SleepParam=0;      
      }  
      TCB[param]->State&=~(SLEEP_TASK | SUSPEND_TASK);      
      SleepTasks&=~(0x80000000>>param); 
      if(READY_TASK==TCB[param]->State);
      {
        ReadyTasks|=(0x80000000>>param);      
        ReadyTasksCurrent|=(0x80000000>>param);           
      }  
      break;
    case 7: 
      //WaiteForSingleObject
      if(param)
      {  
        switch( *((uint32_t*) param) & HANDLE_TYPE_MASK )
        {  
        case HANDLE_TYPE_SEMAPHORE:
          if( ((semaphore_t*) param)->semaphore_count)
          {
            --((semaphore_t*) param)->semaphore_count;
            result=0x00000000;               
          }
          else
          {
            current_task->State=WAITING_HANDLE;
            ReadyTasksCurrent&=~(0x80000000>>current_task_id);
            ReadyTasks&=~(0x80000000>>current_task_id);  
            ((semaphore_t*) param)->waiting_tasks|=(0x80000000>>current_task_id);
            result=0xFFFFFFFF;              
          }
          break;
        case HANDLE_TYPE_MUTEX:
          if( ((*((uint32_t*) param)  & HANDLE_TASK_ID_MASK) == HANDLE_OWNERLESS) || ((*((uint32_t*) param)  & HANDLE_TASK_ID_MASK) == current_task_id) )
          {
            *((uint32_t*) param)= HANDLE_TYPE_MUTEX | current_task_id;
            result=0x00000000;  
          }
          else
          {
            current_task->State=WAITING_HANDLE;
            ReadyTasksCurrent&=~(0x80000000>>current_task_id);
            ReadyTasks&=~(0x80000000>>current_task_id);  
            ((mutex_t*) param)->waiting_tasks|=(0x80000000>>current_task_id);
            result=0xFFFFFFFF;              
          }
          break;
        }
      }
    break;  
    case 8: 
      //ReleaseMutex, ReleaseSemaphore
      if(param)
      {
        switch( *((uint32_t*) param) & HANDLE_TYPE_MASK )
        {
        case HANDLE_TYPE_SEMAPHORE:
          if( ((semaphore_t*) param)->semaphore_count+1<= ((semaphore_t*) param)->semaphore_max_count)
          {  
            ((semaphore_t*) param)->semaphore_count+=1;
          }
          if(((semaphore_t*) param)->semaphore_count)
          {
            task_id=__CLZ(((semaphore_t*) param)->waiting_tasks & ~SleepTasks);
            if(task_id<32)
            {
              ((semaphore_t*) param)->waiting_tasks&=~(0x80000000>>task_id);    
              ReadyTasksCurrent|=(0x80000000>>task_id);
              ReadyTasks|=(0x80000000>>task_id);  
              --((semaphore_t*) param)->semaphore_count;
            }
            result=0x00000000;               
          }
          break;
          
        case HANDLE_TYPE_MUTEX:
          if((*((uint32_t*) param)  & HANDLE_TASK_ID_MASK) == current_task_id)
          {
            task_id=__CLZ(((mutex_t*) param)->waiting_tasks & ~SleepTasks);
            if(task_id<32)
            {
              *((uint32_t*) param)= HANDLE_TYPE_MUTEX | task_id;
              ((mutex_t*) param)->waiting_tasks&=~(0x80000000>>task_id);    
              ReadyTasksCurrent|=(0x80000000>>task_id);
              ReadyTasks|=(0x80000000>>task_id);              
            }
            else
            {
              *((uint32_t*) param)= HANDLE_TYPE_MUTEX | HANDLE_OWNERLESS;
            }
            result=0x00000000;  
          }
          break;
        }
      }
      break;
      case 9:
        if(CREATE_IDLE_TASK== ((create_task_t*) param)->CreationFlags)
        {
          task_id=0;
        }
        else
        {
          if(task_count<MAX_TASK_COUNT)
            task_id=task_count;
          else
            task_id=INVALID_TID;
        }
        if(INVALID_TID!=task_id)
        {
          TCB[task_id]=malloc(sizeof(TCB_t));
          if(TCB[task_id])
          {
//            memcpy(TCB[task_count],param,sizeof(TCB_t));
            TCB[task_id]->StartAddress = ((create_task_t*) param)->StartAddress;
            TCB[task_id]->Parameter = ((create_task_t*) param)->Parameter;
            TCB[task_id]->StackSize = ((create_task_t*) param)->StackSize;
            TCB[task_id]->SP=malloc(((create_task_t*) param)->StackSize);
            if(TCB[task_id]->SP)
            {
              InitStack(TCB[task_count]);
              ((create_task_t*) param)->TaskId=(uint32_t) TCB[task_id];
              if(!task_id)
                task_count++;
            }
            else
            {
              ((create_task_t*) param)->TaskId=INVALID_TID;
              free(TCB[task_id]);
            }            
          }
          else
          {
              ((create_task_t*) param)->TaskId=INVALID_TID;            
          }
        }        
      break;
    default:while(1);;break;  
  }
  return result;
}


void SVC_Handler(void)
{
  __asm(   
          "     tst     LR,#4                   \n"
          "     ite     eq                      \n"
          "     mrseq   r3,msp                  \n"
          "     mrsne   r3,psp                  \n"                      
          "     ldr     r1,[r3,#24]             \n"            
          "     ldrb    r1,[r1,#-2]             \n"

          "     tst     r1,#128                 \n"
          "     itt     ne                      \n" 
          "     ldr.n   r2,ExtendRequest_local  \n"
          "     bx     r2                       \n"  
            
          "     ldr.n   r2,current_task_local   \n"         
          "     ldr     r2,[r2]                 \n"                                   
          "    	push    {lr}                    \n" 
            
          "     cmp     r1,#0                   \n"            
          "     itttt   ne                      \n"
          "    	stmdb   r3!, {r4-r11}           \n"            
          "     str     r3,[r2]                 \n"   
          "     ldr.n   r3,SVCHandler_main_local\n"               
          "     blx     r3                      \n"


          "     ldr.n   r3,coreSheduler_local   \n"               
          "     mvns    r1,r1                   \n"                           
          "     tst     r1,r0                   \n"  
          "     itt     ne                      \n" 
          "     mov     r0,#0                   \n"  
          "     blx     r3                      \n"  
            
          "    	pop     {lr}                    \n"                             
          "     ldr.n   r2,current_task_local   \n"         
          "     ldr     r2,[r2]                 \n"
          "     ldr     r2,[r2]                 \n"            
          "	ldmia   r2!, {r4-r11}           \n"               
          "     msr     psp, r2                 \n"
          "	orr     r14, r14, #13           \n"
          "     bx      lr                      \n" 
          "     dc16    0                       \n"             
          "ExtendRequest:                       \n"  
          "     push    {lr}                    \n"
          "     ldr.n   r2,coreAllocateMem_local\n"            
          "     blx     r2                      \n"
          "     pop     {lr}                    \n"
          "     mrs     r3,psp                  \n"            
          "     str     r0,[r3,#0]              \n"  
          "     bx      lr                      \n"             
          "     dc16    0                       \n" 
//          "  ALIGNROM 1     \n"           
//          "     .align  2              \n"
          "SVCHandler_main_local:               \n"
          "     dc32    SVCHandler_main         \n"    
          "coreSheduler_local:                  \n"
          "     dc32    coreSheduler            \n"              
          "current_task_local:                  \n"
          "     dc32    current_task            \n"
          "coreAllocateMem_local:               \n"
          "     dc32    coreAllocateMem         \n" 
          "ExtendRequest_local:                 \n"  
          "     dc32    ExtendRequest           \n"               
       );   
}



