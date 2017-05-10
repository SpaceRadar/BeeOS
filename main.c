#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
//#include "stm32f10x_gpio.h"
#include "heap.h"

#include "BeeOS.h"

volatile unsigned long a,b,c;


#define UNUSED(x) ((void)(x))

#define __HAL_RCC_GPIOA_CLK_ENABLE()   do { \
                                        __IO uint32_t tmpreg = 0x00U; \
                                        SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);\
                                        /* Delay after an RCC peripheral clock enabling */ \
                                        tmpreg = READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);\
                                        UNUSED(tmpreg); \
                                          } while(0U)


#if 0
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
#endif


volatile unsigned long task=0;
volatile unsigned long current_task_count=0;

#define STACK_SIZE 64
#define MAX_TASK_COUNT 4

extern void SetRegs(void);



typedef enum 
{
  LED1 = 0,
  LED2  
} Led_TypeDef;
  
volatile unsigned long val1=10;
volatile unsigned long val2=0;

volatile unsigned long* pval1=&val1;
volatile unsigned long* pval2=&val2;


#define LEDn                             2

#define LED_PIN1                         GPIO_Pin_6
#define LED_GPIO_PORT1                   GPIOA
#define LED_GPIO_CLK1                    RCC_AHB1Periph_GPIOA  
  

#define LED_PIN2                         GPIO_Pin_7
#define LED_GPIO_PORT2                   GPIOA
#define LED_GPIO_CLK2                    RCC_AHB1Periph_GPIOA 


GPIO_TypeDef* GPIO_PORT[LEDn] = {LED_GPIO_PORT1,LED_GPIO_PORT2};
const uint16_t GPIO_PIN[LEDn] = {LED_PIN1,LED_PIN2};
const uint32_t GPIO_CLK[LEDn] = {LED_GPIO_CLK1,LED_GPIO_CLK2};

void LEDInit(Led_TypeDef Led)
{
  
  
__HAL_RCC_GPIOA_CLK_ENABLE();  
  
  
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* Enable the GPIO_LED Clock */
//  RCC_APB1PeriphClockCmd(GPIO_CLK[Led], ENABLE);

  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_PIN[0]|GPIO_PIN[1];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIO_PORT[Led], &GPIO_InitStructure);
  GPIO_SetBits(GPIO_PORT[0],GPIO_Pin_6|GPIO_Pin_7);
  GPIO_ResetBits(GPIO_PORT[0],GPIO_Pin_6|GPIO_Pin_7);
  



  //GPIO_InitTypeDef  GPIO_InitStructure;
#if 0  
  /* Enable the GPIO_LED Clock */
  RCC_APB1PeriphClockCmd(GPIO_CLK[Led], ENABLE);

  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_PIN[Led];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_PORT[Led], &GPIO_InitStructure);  
#endif
}

void LEDOn(Led_TypeDef Led)
{
  GPIO_PORT[Led]->BSRRL = GPIO_PIN[Led];
}

void LEDOff(Led_TypeDef Led)
{
  GPIO_PORT[Led]->BSRRH = GPIO_PIN[Led];
}

void Delay(unsigned long del)
{
  while(del--);
}

volatile unsigned long delay_counter;
void Delay_ms(unsigned long del)
{
  delay_counter=del;
  while(delay_counter);
}



int led=0; 

void SysTick_Handler(void)
{
//  task=!task;
  //SCB-> ICSR = SCB_ICSR_PENDSVSET;
  *(portNVIC_INT_CTRL) = portNVIC_PENDSVSET;  
}

CRITICAL_SECTION csRead;  
CRITICAL_SECTION csWrite; 
CRITICAL_SECTION cs;

void Sleep2(unsigned long SleepTime)
{
  __asm( 
        "     svc   9  \n"
       );  
}




void lock_mutex(void* mutex)
{
#if 0  
  __asm(
//        "       LDR r1, =#1           \n"
          "       MOV r1, #1           \n"        
          "m1:       LDREX r2, [r0]    \n"
          "       CMP r2, r1           \n" // ; Test if mutex is locked or unlocked
          "       ITEE  EQ             \n" //; If locked - wait for it to be released, from 2            
//          "       bx   m2               \n" //; If locked - wait for it to be released, from 2
          "       STREX r2, r1, [r0] \n" //Not locked, attempt to lock it
          "       CMP r2, #1         \n" //Check if Store-Exclusive failed
//          "       BEQ m1              \n" //Failed - retry from 1

//          "       DMB                  \n" //Required before accessing protected resource
//          "       BX lr                \n"
          "m2:                  \n" //Retry from 1
       );
#endif  
}  

HANDLE hMutex;
HANDLE hSem1, hSem2;
void MainTask(void)
{ 
//  InitializeCriticalSection(&cs);
//  CreateMutex(&hMutex, 1);
  hSem1=CreateSemaphore(0, 1);
  hSem2=CreateSemaphore(0, 1);
  Sleep(1000);
  ResumeTask(2);  
  ResumeTask(3);
  ReleaseSemaphore(hSem1,1);
  Sleep(INFINITE);
  while(1);
}

void Task1(void)
{ 
  while(1)
  {
    WaitForSingleObject(hSem1);  
    Sleep(500);
    LEDOn(LED1); 
    ReleaseSemaphore(hSem2,1);    
  }
}

void Task2(void)
{
  while(1)
  {
    WaitForSingleObject(hSem2);
    Sleep(500);
    LEDOff(LED1); 
    ReleaseSemaphore(hSem1,1);    
  }    
}

void Task3(void)
{
  WaitForSingleObject(hSem1);
//  Sleep(15);
//  ReleaseMutex(&hMutex);  
  Sleep(INFINITE);  
  while(1);    
}

void Task4(void)
{
  WaitForSingleObject(hSem1);
  Sleep(15);
  Sleep(INFINITE);  
  while(1);    
}

unsigned long addr1, addr2;



/*
HANDLE CreateThread(SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress)
{
  
}
*/
void Test()
{
  //unsigned long val=0;
//  unsigned long* pval=&val;
  

#if 0  
  __asm(  
        "     push {r0-r4}\n"
	"     ldr r0, pval\n"
	"     ldr r1, [r0]\n"          
//        "     mov r2,#1\n"   
        "     add r1,r1,#1\n"  
	"     str r1,[r0]\n"
        "     pop {r0-r4}\n"   
       );
#else
  __asm(  
        "     push {r0-r4}\n"
	"     ldr.n r0, pval1\n"
	"     ldr.n r4, pval2\n"

        "     mov r7,#1\n"            
	"     ldrex r1, [r0]\n"  
	"     ldrex r5, [r4]\n"               
 
	"     strex r2,r7,[r0]\n"          
        "     isb      \n"   
        "     dmb      \n"             
        "     dsb      \n"             
          
//	"     ldrex r5, [r4]\n"            
        "     strex r6,r7,[r4]\n"
          
        "     pop {r0-r4}\n"   
        "     bx lr \n"  
        "     dc16    0                       \n"           
        "pval1:                  \n"
        "     dc32    val1            \n"             
        "pval2:                  \n"
        "     dc32    val2            \n"               
       );
  
#endif  
}  

volatile float x,y,z;
volatile unsigned long v;
uint8_t* p;
void main()
{
  LEDInit(LED1);
  __enable_interrupt();
  SysTick_Config(SystemCoreClock/1000);
  StartFirstTask();
}
