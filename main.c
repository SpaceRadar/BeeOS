#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
//#include "stm32f10x_gpio.h"

#include "BeeOS.h"

volatile int a;


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
} Led_TypeDef;
  
volatile unsigned long val1=10;
volatile unsigned long val2=0;

volatile unsigned long* pval1=&val1;
volatile unsigned long* pval2=&val2;


#define LEDn                             1

#define LED_PIN                         GPIO_Pin_12
#define LED_GPIO_PORT                   GPIOD
#define LED_GPIO_CLK                    RCC_AHB1Periph_GPIOD  
  

GPIO_TypeDef* GPIO_PORT[LEDn] = {LED_GPIO_PORT};
const uint16_t GPIO_PIN[LEDn] = {LED_PIN};
const uint32_t GPIO_CLK[LEDn] = {LED_GPIO_CLK};

void LEDInit(Led_TypeDef Led)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* Enable the GPIO_LED Clock */
  RCC_APB2PeriphClockCmd(GPIO_CLK[Led], ENABLE);

  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_PIN[Led];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIO_PORT[Led], &GPIO_InitStructure);
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

void Sleep2(unsigned long SleepTime)
{
  __asm( 
        "     svc   9  \n"
       );  
}

void Task1(void)
{ 
  Sleep(INFINITE);
  while(1)
  {
    Sleep(256);    
    LEDOn(LED1); 
    Sleep(256);    
    LEDOff(LED1);    
  } 
    
  __asm(  
        "     push {r0-r4}\n"
	"     ldr r4, pval1\n"
	"     ldr r1, [r4]\n"          
        "     mov r0, #1\n"
//        "     bl Sleep2\n"   
        "     svc   1  \n"          
        "     mov r2,#1\n"   
        "     add r1,r1,#1\n"  
	"     str r1,[r4]\n"
        "     pop {r0-r4}\n"   
       );  
  
//  while(1);
/*  
  InitializeCriticalSection(&csRead);
  InitializeCriticalSection(&csWrite);  
  EnterCriticalSection(&csRead);
  Sleep(2);
  LeaveCriticalSection(&csRead);  
*/  
  while(1)
  {
//    Sleep(INFINITE);
    Sleep(256);    
    LEDOn(LED1); 
    Sleep(10);    
    LEDOff(LED1);    
  } 
  
}

void Task2(void)
{
/*  
  __asm(  
        "     push {r0-r4}\n"
	"     ldr r4, pval1\n"
	"     ldrex r1, [r4]\n"          
        "     mov r0, #2\n"
//        "     svc   1  \n"       
        "     mov r2,#1\n"   
        "     add r1,r1,#1\n"  
	"     strex r2,r1,[r4]\n"
        "     pop {r0-r4}\n"   
       );  
*/  
//  Sleep(10000);  
  ResumeThread(0);  
  while(1);  

/*  
  EnterCriticalSection(&csRead);  
  while(1)
  {
    Sleep(10);    
    LEDOn(LED1); 
    Sleep(5);    
    LEDOff(LED1);  
  }
*/  
}



unsigned long addr1, addr2;



/*
HANDLE CreateThread(SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress)
{
  
}
*/

void Test()
{
//  val1++;

#if 0  
  __asm(  
        "     push {r0-r4}\n"
	"     ldr r0, pval1\n"
	"     ldr r1, [r0]\n"          
//        "     mov r2,#1\n"   
        "     add r1,r1,#1\n"  
	"     str r1,[r0]\n"
        "     pop {r0-r4}\n"   
       );
#else
  __asm(  
        "     push {r0-r4}\n"
	"     ldr r0, pval1\n"
	"     ldrex r1, [r0]\n"          
	"     ldrex r3, [r0]\n"               
        "     mov r2,#1\n"   
        "     add r1,r1,#1\n"  
	"     strex r2,r1,[r0]\n"
	"     strex r2,r1,[r0]\n"          
        "     pop {r0-r4}\n"   
       );
  
#endif  
}  

static volatile int a;

void myfoo(void)
{
  __asm(   
          "     ldr.n   r4,ma\n"
          "     nop\n"  
          "     ldr.n   r2,[r4]\n"            
          "     BX     LR      \n"  
            "ma:     DC32   a   \n"                         
       );  
}


void main()
{

  a=4;
//  InitTCB(0, (unsigned long)Task1);
//  InitTCB(1, (unsigned long)Task2);
//  Test();
  LEDInit(LED1);

 
//  vConfigureTimerForRunTimeStats();
//	*(portNVIC_SYSPRI2) |= portNVIC_PENDSV_PRI;
//	*(portNVIC_SYSPRI2) |= portNVIC_SYSTICK_PRI;  
//  NVIC_IntEnable(NVIC_TIMER0);
//  NVIC_IntPri(NVIC_TIMER0,HIGHEST_PRIORITY);
  __enable_interrupt();
//  __disable_interrupt();
//	*(portNVIC_SYSTICK_LOAD) = 0xFFFFFF - 1UL;
//	*(portNVIC_SYSTICK_CTRL) = portNVIC_SYSTICK_CLK | portNVIC_SYSTICK_INT | portNVIC_SYSTICK_ENABLE;
//  SetRegs();
  SysTick_Config(SystemCoreClock/1000);        
  StartFirstTask();
}
