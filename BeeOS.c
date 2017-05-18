#include "stm32f4xx.h"
//#include "stm32f4xx_rcc.h"
#include <core_cm4.h>
#include <string.h>

#include "BeeOS.h"
#include "heap.h"

#define IDLE_TASK_ID 0
TCB_t* TCB[MAX_TASK_COUNT];

static volatile task_set_t ReadyTasksCurrent=0;
static volatile task_set_t ReadyTasks=0;
static volatile task_set_t SleepTasks=0;
static volatile task_set_t ActiveTasks=0;


static volatile TCB_t* current_task;
static volatile uint32_t task_count=0;
static volatile uint32_t current_task_id;
static volatile uint32_t beeos_running=0;

static volatile unsigned long addr;

__weak void IdleTask(void* param)
{
  while(1);
}

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


void ResumeTask (tid_t tid)
{
  __asm( 
        "     isb      \n"          
        "     svc   6  \n"
       );    
}

void WaitForSingleObjectAdapter (wait_for_single_object_t* wait_for_single_object)
{
  __asm( 
        "     isb      \n"          
        "     svc   7  \n"
       );    
}

void ReleaseObjectAdapter(release_object_t* release_object)
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

void CreateMailBoxAdapter(create_mailbox_t* create_mailbox)
{
  __asm( 
        "     isb      \n"          
        "     svc   10  \n"
       );   
}

void SendMessageAdapter(send_message_t* send_message)
{
  __asm( 
        "     isb      \n"          
        "     svc   11  \n"
       );   
}

void GetMessageAdapter(get_message_t* get_message)
{
  __asm( 
        "     isb      \n"          
        "     svc   12  \n"
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

int32_t WaitForSingleObject (HANDLE handle, uint32_t time_out)
{
  wait_for_single_object_t wait_for_single_object;
  if(handle && (INVALID_HANDLE!=handle))
  {
    wait_for_single_object.handle=handle;
    wait_for_single_object.time_out=time_out;
    WaitForSingleObjectAdapter(&wait_for_single_object);
  }
  else
  {
    wait_for_single_object.result=E_INVALID_HANDLE;
  }
  return wait_for_single_object.result;
}

int32_t ReleaseSemaphore (HANDLE handle, uint32_t ReleaseCount)
{
  release_object_t release_object;
  if(handle && (INVALID_HANDLE!=handle))
  {
    release_object.handle=handle;
    release_object.handle_type=HANDLE_TYPE_SEMAPHORE;
    release_object.release_count=ReleaseCount;  
    ReleaseObjectAdapter(&release_object);
  }
  else
  {
    release_object.result=E_INVALID_HANDLE;    
  }
  return release_object.result;
}

int32_t ReleaseMutex (HANDLE handle)
{
  release_object_t release_object;
  if(handle && (INVALID_HANDLE!=handle))
  {
    release_object.handle=handle;
    release_object.handle_type=HANDLE_TYPE_MUTEX;
    ReleaseObjectAdapter(&release_object);
  }
  else
  {
    release_object.result=E_INVALID_HANDLE;
  }
  return release_object.result;
}

HANDLE CreateMutex(uint32_t InitialOwner)
{
  mutex_t* handle=AllocateMem(sizeof(mutex_t));
  if(handle)
  {
    if(InitialOwner)
        handle->base.TypeAndOwner=HANDLE_TYPE_MUTEX | current_task_id;
    else
        handle->base.TypeAndOwner=HANDLE_TYPE_MUTEX | HANDLE_OWNERLESS;  
    handle->base.waiting_tasks=0;
  }
  else
  {
    handle=(mutex_t*)E_INVALID_HANDLE;
  }
  return (HANDLE) handle;
}

HANDLE CreateSemaphore(uint32_t InitialCount, uint32_t MaximumCount)
{
  semaphore_t* handle=AllocateMem(sizeof(semaphore_t));
  if(handle && (InitialCount<=MaximumCount) )
  {
      handle->base.TypeAndOwner=HANDLE_TYPE_SEMAPHORE | HANDLE_OWNERLESS;  
      handle->base.waiting_tasks=0;      
      handle->semaphore_count=InitialCount;
      handle->semaphore_max_count=MaximumCount;
  }
  else
  {
    handle=(semaphore_t*) E_INVALID_HANDLE;
  }  
  return (HANDLE) handle;
}

HANDLE CreateMailBox(uint32_t maxmsg, uint32_t msgsize)
{
  create_mailbox_t create_mailbox;
  create_mailbox.maxmsg=maxmsg;
  create_mailbox.msgsize=msgsize;
  
  if(beeos_running)
    CreateMailBoxAdapter(&create_mailbox);  
  else
    SVCHandler_main((uint32_t) &create_mailbox, 10);  
  return create_mailbox.handle;
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
  if(beeos_running)
    CreateTaskAdapter(&create_task);  
  else
    SVCHandler_main((uint32_t) &create_task, 9);
  return create_task.TaskId;
}

int32_t SendMessage(HANDLE handle,uint32_t size, void* buffer)
{
  send_message_t send_message;
  send_message.handle=handle;
  send_message.buffer=buffer;
  send_message.size=size;
  SendMessageAdapter(&send_message);
  return send_message.result;
}

int32_t GetMessage(HANDLE handle,uint32_t size, void* buffer)
{
  get_message_t get_message;
  get_message.handle=handle;
  get_message.buffer=buffer;
  get_message.size=size;
  GetMessageAdapter(&get_message);
  return get_message.result;
}


void InitStack(TCB_t* TCB)
{
  uint32_t* SP;
  SP=(uint32_t*) ((uint32_t)TCB->Stack+TCB->StackSize);
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
      TCB[idx]->State&= ~(SLEEP_TASK | WAITING_HANDLE);
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
  CreateTask(64,IdleTask,(void*) 0, CREATE_IDLE_TASK);
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
  beeos_running=1;
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

void inc_mailbox_write_pointer(mailbox_t* mailbox)  
{
  ++mailbox->write_idx;   
  if( mailbox->write_idx < mailbox->maxmsg)
  {    
    //move write pointer on one step
    mailbox->write_packet= (mailbox_packet_t*)((uint32_t) mailbox->write_packet + mailbox->paket_size);               
  }
  else
  {
    //move write pointer to first position              
    mailbox->write_packet=mailbox->buffer;
    mailbox->write_idx=0;              
  }    
  ++mailbox->counter;  
}
        
void inc_mailbox_read_pointer(mailbox_t* mailbox)  
{
  ++mailbox->read_idx;   
  if( mailbox->read_idx < mailbox->maxmsg)
  {    
    //move write pointer on one step
    mailbox->read_packet= (mailbox_packet_t*)((uint32_t) mailbox->read_packet + mailbox->paket_size);               
  }
  else
  {
    //move write pointer to first position              
    mailbox->read_packet=mailbox->buffer;
    mailbox->read_idx=0;              
  } 
  --mailbox->counter;
}

void check_waiting_handle_tasks(release_object_t* release_object)
{
  
}
        


static uint32_t SVCHandler_main(uint32_t param, uint32_t svc_id)
{
//  uint32_t idx;
//  uint32_t cs_idx;
//  uint32_t WakingTask;
  uint32_t task_id;
  uint32_t result=0; 
  mailbox_t* mailbox;
  
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
      ((wait_for_single_object_t*) param)->result=E_TIME_OUT;
      switch( ((handle_base_t*)((wait_for_single_object_t*) param)->handle)->TypeAndOwner & HANDLE_TYPE_MASK )
      {  
        case HANDLE_TYPE_SEMAPHORE:
          if( ((semaphore_t*)((wait_for_single_object_t*) param)->handle)->semaphore_count )
          {
            --((semaphore_t*)((wait_for_single_object_t*) param)->handle)->semaphore_count;
            ((wait_for_single_object_t*) param)->result=E_OK; 
          }
        break;
        
        case HANDLE_TYPE_MUTEX:
          if( ((((handle_base_t*)((wait_for_single_object_t*) param)->handle)->TypeAndOwner & HANDLE_TASK_ID_MASK) == HANDLE_OWNERLESS) ||
              ((((handle_base_t*)((wait_for_single_object_t*) param)->handle)->TypeAndOwner & HANDLE_TASK_ID_MASK) == current_task_id)
            )
          {
            *((uint32_t*)((wait_for_single_object_t*) param)->handle)= HANDLE_TYPE_MUTEX | current_task_id;
            ((wait_for_single_object_t*) param)->result=E_OK;              
          }
        break;
         
       default: ((wait_for_single_object_t*) param)->result=E_INVALID_HANDLE; break;
      }
      
      if( E_TIME_OUT==((wait_for_single_object_t*) param)->result )
      {
        if( ((wait_for_single_object_t*) param)->time_out )
        {
          current_task->SleepParam=((wait_for_single_object_t*) param)->time_out;              
          if( INFINITE==((wait_for_single_object_t*) param)->time_out )
          {
            current_task->State=WAITING_HANDLE;   
          }
          else
          {
            current_task->State=WAITING_HANDLE | SLEEP_TASK;
            SleepTasks|=0x80000000>>current_task_id;  
          }
          ReadyTasksCurrent&=~(0x80000000>>current_task_id);
          ReadyTasks&=~(0x80000000>>current_task_id);
          current_task->RequestStruct=(void*)param;
          result=0xFFFFFFFF;              
        }
      }
    break;  
    case 8: 
      //ReleaseMutex, ReleaseSemaphore
      switch(  *((uint32_t*)((release_object_t*) param)->handle) & HANDLE_TYPE_MASK )
      {
        case HANDLE_TYPE_SEMAPHORE:
          if( HANDLE_TYPE_SEMAPHORE==((release_object_t*) param)->handle_type)
          {
            if( ((semaphore_t*)((release_object_t*) param)->handle)->semaphore_count+
                ((release_object_t*) param)->release_count<=
                ((semaphore_t*)((release_object_t*) param)->handle)->semaphore_max_count )
            {              
              ((semaphore_t*)((release_object_t*) param)->handle)->semaphore_count+=((release_object_t*) param)->release_count;
              ((release_object_t*) param)->result=E_OK;              
                
              if(((semaphore_t*)((release_object_t*) param)->handle)->semaphore_count)
              {
                task_id=__CLZ(((semaphore_t*)((release_object_t*) param)->handle)->base.waiting_tasks & ActiveTasks);
                if(task_id<32)
                {
                  ((semaphore_t*)((release_object_t*) param)->handle)->base.waiting_tasks&=~(0x80000000>>task_id);    
                  ReadyTasksCurrent|=(0x80000000>>task_id);
                  ReadyTasks|=(0x80000000>>task_id);  
                  ((wait_for_single_object_t*)TCB[task_id]->RequestStruct)->result=E_OK;
                  TCB[task_id]->State&=~(WAITING_HANDLE | SLEEP_TASK);
                  --((semaphore_t*)((release_object_t*) param)->handle)->semaphore_count;
                }
              }                
            }
            else
            {
              ((release_object_t*) param)->result=E_INVALID_VALUE;
            }          
          }
          else
          {
            ((release_object_t*) param)->result=E_INVALID_HANDLE;            
          }
          result=0x00000000;               
        break;
          
        case HANDLE_TYPE_MUTEX:
          if( HANDLE_TYPE_MUTEX==((release_object_t*) param)->handle_type)
          {
            if( (*((uint32_t*)((release_object_t*) param)->handle) & HANDLE_TASK_ID_MASK) == current_task_id)
            {              
              task_id=__CLZ(((mutex_t*)((release_object_t*) param)->handle)->base.waiting_tasks & ActiveTasks) & 0x0000001F;
              if(task_id)
              {
                *((uint32_t*)((release_object_t*) param)->handle)=HANDLE_TYPE_MUTEX | task_id;
                  
                  ((mutex_t*)((release_object_t*) param)->handle)->base.waiting_tasks&=~(0x80000000>>task_id);    
                  ReadyTasksCurrent|=(0x80000000>>task_id);
                  ReadyTasks|=(0x80000000>>task_id);  
                  ((wait_for_single_object_t*)TCB[task_id]->RequestStruct)->result=E_OK;
                  TCB[task_id]->State&=~(WAITING_HANDLE | SLEEP_TASK);
              }                
              else
              {
                *((uint32_t*)((release_object_t*) param)->handle)=HANDLE_TYPE_MUTEX | HANDLE_OWNERLESS;                
              }
              ((release_object_t*) param)->result=E_OK;              
            }
            else
            {
              ((release_object_t*) param)->result=E_INVALID_VALUE;
            }          
          }
          else
          {
            ((release_object_t*) param)->result=E_INVALID_HANDLE;            
          }
          result=0x00000000;               
        break;
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
            task_id=task_count+1;
          else
            task_id=INVALID_TID;
        }
        if(INVALID_TID!=task_id)
        {
          TCB[task_id]=malloc(sizeof(TCB_t));
          if(TCB[task_id])
          {
            TCB[task_id]->StartAddress = ((create_task_t*) param)->StartAddress;
            TCB[task_id]->Parameter = ((create_task_t*) param)->Parameter;
            TCB[task_id]->StackSize = ((create_task_t*) param)->StackSize;         
            TCB[task_id]->Stack=malloc(((create_task_t*) param)->StackSize);
            if(TCB[task_id]->Stack)
            {
              InitStack(TCB[task_id]);
              if ( !(((create_task_t*) param)->CreationFlags & CREATE_SUSPENDED) && task_id)
              {        
                ReadyTasks|=0x80000000 >> task_id;
                ReadyTasksCurrent|=0x80000000 >> task_id;                
                ActiveTasks|=0x80000000 >> task_id;                
                TCB[task_id]->State=0;
              }
              else
              {
                TCB[task_id]->State=SUSPEND_TASK;
              }
                
              ((create_task_t*) param)->TaskId=task_id;
              if(task_id)
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
      case 10:
        mailbox=malloc(sizeof(mailbox_t));
        if(mailbox)
        {
          mailbox->TypeAndOwner=HANDLE_TYPE_MAILBOX;
          mailbox->maxmsg= ((create_mailbox_t*)param)->maxmsg;
          mailbox->msgsize= ((create_mailbox_t*)param)->msgsize;
          mailbox->paket_size=(mailbox->msgsize+7) & 0xFFFFFFFC;
          mailbox->buffer=malloc(mailbox->maxmsg*mailbox->paket_size);
          if(mailbox->buffer)
          {
            mailbox->read_idx=0;
            mailbox->write_idx=0;
            mailbox->counter=0; 
            mailbox->read_packet=mailbox->buffer;
            mailbox->write_packet=mailbox->buffer;            
          }
          else
          {
            free(mailbox);
            mailbox=0;
          }
        }
        ((create_mailbox_t*)param)->handle=(uint32_t)mailbox;
      break;
      case 11:
      //SendMessage 
      if(  ((send_message_t*)param)->handle &&  (*((uint32_t*) ((send_message_t*)param)->handle)==HANDLE_TYPE_MAILBOX) )
      {
        if( ((send_message_t*)param)->size<= ((mailbox_t*)((send_message_t*)param)->handle)->msgsize )
        {   
          task_id=__CLZ(((mailbox_t*)((send_message_t*)param)->handle)->read_waiting_tasks & ActiveTasks) & 0x0000001F;
          if(task_id)
          {            
            //same task is waiting this message 
            ((mailbox_t*)((send_message_t*)param)->handle)->read_waiting_tasks&=~(0x80000000>>task_id);    
            ReadyTasksCurrent|=(0x80000000>>task_id);
            ReadyTasks|=(0x80000000>>task_id); 
            TCB[task_id]->State&=~WAITING_HANDLE;            
            if( ((get_message_t*)TCB[task_id]->RequestStruct)->size>= ((get_message_t*)param)->size ) 
            {                        
                memcpy( ((get_message_t*)TCB[task_id]->RequestStruct)->buffer,
                        ((send_message_t*)param)->buffer,
                        ((send_message_t*)param)->size
                      );             
                ((get_message_t*)TCB[task_id]->RequestStruct)->result=((send_message_t*)param)->size;
            }
            else
            {
                //buffer too small
                ((get_message_t*)TCB[task_id]->RequestStruct)->result=E_BUFFER_TOO_SMALL;              
                task_id=0;
            }            
          } 
          if(!task_id)
          {
            //No one task is waiting this message 
            if( ((mailbox_t*)((send_message_t*)param)->handle)->counter< ((mailbox_t*)((send_message_t*)param)->handle)->maxmsg )
            {
              //There is a room for this message
              ((mailbox_t*)((send_message_t*)param)->handle)->write_packet->size=((send_message_t*)param)->size;
              memcpy( ((mailbox_t*)((send_message_t*)param)->handle)->write_packet->data,
                      ((send_message_t*)param)->buffer,
                      ((send_message_t*)param)->size); 

              inc_mailbox_write_pointer( (mailbox_t*)((send_message_t*)param)->handle);                
            }
            else
            {
              //Mailbox is full. Send to sleep the task
              current_task->State=WAITING_HANDLE;
              current_task->RequestStruct=(void*) param;
              ReadyTasksCurrent&=~(0x80000000>>current_task_id);
              ReadyTasks&=~(0x80000000>>current_task_id);              
              ((mailbox_t*)((send_message_t*)param)->handle)->write_waiting_tasks|=(0x80000000>>current_task_id);
              result=0xFFFFFFFF;               
            }            
          }
          //All OK
          ((send_message_t*) param)->result=E_OK;
        }
        else
        {
          //Too long message
          ((send_message_t*) param)->result=E_BUFFER_TOO_SMALL;          
        }
      }
      else
      {
        //Invalid or not a MailBox handle
        ((send_message_t*) param)->result=E_INVALID_HANDLE;
      }
      break;
      case 12:
      //GetMessage  
      if(  ((get_message_t*)param)->handle &&  (*((uint32_t*) ((get_message_t*)param)->handle)==HANDLE_TYPE_MAILBOX) )
      {
         if(((mailbox_t*)((get_message_t*)param)->handle)->counter)
         {
           if( ((mailbox_t*)((get_message_t*)param)->handle)->read_packet->size <= ((get_message_t*)param)->size)
           {
             
              memcpy(((get_message_t*)param)->buffer, 
                     ((mailbox_t*)((get_message_t*)param)->handle)->read_packet->data,                   
                     ((mailbox_t*)((get_message_t*)param)->handle)->read_packet->size); 

              ((get_message_t*) param)->result=((mailbox_t*)((get_message_t*)param)->handle)->read_packet->size;
             
              inc_mailbox_read_pointer( (mailbox_t*)((get_message_t*)param)->handle);                
               
              task_id=__CLZ(((mailbox_t*)((get_message_t*)param)->handle)->write_waiting_tasks & ActiveTasks);
              if(task_id<32)
              {
                ((mailbox_t*)((get_message_t*)param)->handle)->write_packet->size=((send_message_t*)TCB[task_id]->RequestStruct)->size;
                memcpy( ((mailbox_t*)((send_message_t*)param)->handle)->write_packet->data,
                        ((send_message_t*)TCB[task_id]->RequestStruct)->buffer,
                        ((send_message_t*)TCB[task_id]->RequestStruct)->size); 

                inc_mailbox_write_pointer( (mailbox_t*)((get_message_t*)param)->handle);                 
                ((mailbox_t*)((get_message_t*)param)->handle)->write_waiting_tasks&=~(0x80000000>>task_id);    
                ReadyTasksCurrent|=(0x80000000>>task_id);
                ReadyTasks|=(0x80000000>>task_id); 
                TCB[task_id]->State&=~WAITING_HANDLE;
              }                
           }
           else
           {
             //output buffer is too small
             ((get_message_t*) param)->result=E_BUFFER_TOO_SMALL;
           }           
         }
         else
         {
           //Empty box. Send to sleep the task
            current_task->State=WAITING_HANDLE;
            current_task->RequestStruct=(void*)param;
            ReadyTasksCurrent&=~(0x80000000>>current_task_id);
            ReadyTasks&=~(0x80000000>>current_task_id);              
            ((mailbox_t*)((get_message_t*)param)->handle)->read_waiting_tasks|=(0x80000000>>current_task_id);
            result=0xFFFFFFFF;            
         }
      }
      else
      {
        //Invalid or not a MailBox handle
        ((get_message_t*) param)->result=E_INVALID_HANDLE;
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



