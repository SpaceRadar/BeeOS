#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include <core_cm4.h>

#include "BeeOS.h"
#include "BeeOS_Tasks.h"
//#include "BeeOSIni.h"

#define IDLE_TASK_ID 0
extern struct TCB_t TCB[];

static unsigned long ReadyTasksCurrent=0;
static unsigned long ReadyTasks=0;
static unsigned long SleepTasks=0;
//static unsigned long CriticalSection=0;
static unsigned long CriticalSectionUsing=0xFFFFFFFF;
static CRITICAL_SECTION_t CRITICAL_SECTION_Array[32];


static volatile struct TCB_t* current_task;
static volatile unsigned long current_task_id;

static volatile unsigned long addr;

HANDLE GetCurrentThread()
{
  return current_task_id;
}

void Sleep(unsigned long SleepTime)
{
  __asm( 
        "     svc   1  \n"
       );  
}

void BeyondTheTask(void)
{
  __asm( 
        "     svc   2  \n"
       );  
}

int InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
  __asm( 
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
        "     svc   4  \n"
       );    
  return 0; 
}

int LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection)
{
  __asm( 
        "     svc   5  \n"
       );    
  return 0; 
}

void ResumeThread (HANDLE hThread)
{
  __asm( 
        "     svc   6  \n"
       );    
}

void InitStack(unsigned long taskId, LPTHREAD_START_ROUTINE lpStartAddress)
{
  unsigned long* SP=&TCB[taskId].SP[STACK_SIZE];
  *(--SP)=0x01000000;
  *(--SP)= (unsigned long)lpStartAddress;
  *(--SP)= (unsigned long) BeyondTheTask;
  *(--SP)= 0xCCCCCCCC;
  *(--SP)= 0x33333333;
  *(--SP)= 0x22222222;
  *(--SP)= 0x11111111;
  *(--SP)= 0x00000000;


  *(--SP)= 0xBBBBBBBB;  
  *(--SP)= 0xAAAAAAAA;
  *(--SP)= 0x99999999;  
  *(--SP)= 0x88888888;
  
  *(--SP)= 0x77777777;  
  *(--SP)= 0x66666666;
  *(--SP)= 0x55555555;  
  *(--SP)= 0x44444444;
  
  TCB[taskId].SP=SP;
}

void PendSV_Handler(void)
{
  __asm(   
          "     mrs     r0,psp                  \n"
          "	isb                             \n"            
          "    	stmdb   r0!, {r4-r11}           \n"
          "     ldr.n   r4,current_task_local   \n"         
          "     ldr     r4,[r4]                 \n"            
          "     str     r0,[r4]                 \n" 
          "    	push    {lr}                    \n"
          "     ldr.n   r0,Sheduler_local       \n"               
          "     bx      r0                      \n"  
          "	pop     {lr}                    \n"   
          "     ldr.n   r0,current_task_local   \n"         
          "     ldr     r0,[r0]                 \n"
          "     ldr     r0,[r3]                 \n"        
          "	ldmia   r0!, {r4-r11}           \n"   
          "     msr     psp, r0                 \n"
          "	isb                             \n"
          "     bx      lr                      \n"    
//          "     dc16    0                     \n" 
//          "     .align 2                      \n"
          "Sheduler_local:                      \n"
          "     dc32    Sheduler                \n"         
          "current_task_local:                  \n"
          "     dc32    current_task            \n"                
       );  
}

void Sheduler()
{
  unsigned long SleepTasksTemp;
  unsigned long idx;
  
  SleepTasksTemp=SleepTasks;
  while(SleepTasksTemp)
  {
        idx=__CLZ(SleepTasksTemp);
        SleepTasksTemp^=(0x80000000>>idx); 
        if(INFINITE!=TCB[idx+1].SleepParam)
        {
          TCB[idx+1].SleepParam--;
          if(!TCB[idx+1].SleepParam)
          {
            SleepTasks^=(0x80000000>>idx); 
            ReadyTasks|=(0x80000000>>idx);            
//            ReadyTasksCurrent|=(0x80000000>>idx);            
          }
        }     
  }
  
    if(0==(ReadyTasks | ReadyTasksCurrent))
    {
      current_task=&TCB[IDLE_TASK_ID];          
    }
    else
    {
      if(!ReadyTasksCurrent)
      {
        ReadyTasksCurrent=ReadyTasks;
      }
      current_task_id=__CLZ(ReadyTasksCurrent);
      ReadyTasksCurrent^=(0x80000000>>current_task_id); 
      current_task=&TCB[current_task_id+1];        
    }   
}


void StartFirstTask()
{
  unsigned long idx;
  for(idx=0;TCB[idx].SP;++idx)
  {
    InitStack(idx,TCB[idx].lpStartAddress); 
    if(idx>0)
    {
      if ( !(TCB[idx].State & CREATE_SUSPENDED))
      {        
        ReadyTasks|=0x80000000 >> (idx-1);
      }
      else
      {
        TCB[idx].SleepParam=INFINITE;
      }
    }
  }
  ReadyTasksCurrent=ReadyTasks;
  Sheduler();
  __asm( 
	"     ldr.n r0, vtor_addr  \n"
	"     ldr r0, [r0]         \n"
	"     ldr r0, [r0]         \n"
	"     msr msp, r0          \n"   
        "     cpsie i              \n"
        "     svc   0              \n"
        "     dc16  0x0000         \n"          //Just for align data                 
        "vtor_addr:                 \n"
        "     dc32  0xE000ED08     \n"          //Vector Table Offset Register
       );
}


void SVCHandler_main(unsigned long param, unsigned long svc_id)
{
  unsigned long idx;
  unsigned long cs_idx;
  unsigned long WakingTask;
  switch(svc_id)
  {
    case 1:
      current_task->SleepParam=param;
      SleepTasks|=0x80000000>>current_task_id; 
      ReadyTasksCurrent&=~(0x80000000>>current_task_id);
      ReadyTasks&=~(0x80000000>>current_task_id);      
//      *(portNVIC_INT_CTRL) = portNVIC_PENDSVSET; 
      PendSV_Handler();   
      break;
    case 2:
      current_task->State=ZOMBIE_TASK;
//      SleepTasks|=0x80000000>>current_task_id; 
      ReadyTasksCurrent&=~(0x80000000>>current_task_id);
      ReadyTasks&=~(0x80000000>>current_task_id);      
//      *(portNVIC_INT_CTRL) = portNVIC_PENDSVSET;   
      PendSV_Handler();          
      break;
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
      cs_idx=*(LPCRITICAL_SECTION)param & HANDLE_MASK;
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
        PendSV_Handler();        
      }
      break;         
    case 5:
      cs_idx=*(LPCRITICAL_SECTION)param & HANDLE_MASK;
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
    case 6:
      TCB[param+1].SleepParam=0;      
      SleepTasks&=~(0x80000000>>current_task_id); 
      ReadyTasks|=(0x80000000>>param);      
      break;      
    default:;break;  
  }
}


void SVC_Handler(void)
{
  __asm(   
          "     tst     LR,#4                   \n"
          "     ite     eq                      \n"
          "     mrseq   r1,msp                  \n"
          "     mrsne   r1,psp                  \n"            
          "     ldr     r1,[r1,#24]             \n"
          "     ldrb    r1,[r1,#-2]             \n"
          "     cmp     r1,#0                   \n"
          "     itt     ne                      \n"            
          "     ldr.n   r0,SVCHandler_main_local\n"               
          "     bx      r0                      \n"               
          "     ldr.n   r0,current_task_local   \n"         
          "     ldr     r0,[r0]                 \n"
          "     ldr     r0,[r0]                 \n"            
          "	ldmia   r0!, {r4-r11}           \n"               
          "     msr     psp, r0                 \n"           
          "	orr     r14, r14, #253            \n"
          "     bx      lr                      \n" 
//          "     dc16    0                       \n" 
//          "     .align 2                \n"
          "SVCHandler_main_local:               \n"
          "     dc32    SVCHandler_main         \n"         
          "current_task_local:                  \n"
          "     dc32    current_task            \n"             
       );   
}



