#include "BeeOS.h"

extern void Task1(void);
extern void Task2(void);

unsigned long TaskStack1[64];
unsigned long TaskStack2[64];
unsigned long IdleTaskStack[32];

__weak void IdleTask(void)
{
  while(1);
}

/*
struct TCB_t TCB[]=
{
  {IdleTaskStack,0,IdleTask,sizeof(IdleTaskStack)/sizeof(unsigned long),0},
  {TaskStack1,CREATE_SUSPENDED,Task1,sizeof(TaskStack1)/sizeof(unsigned long),0},
  {TaskStack2,0,Task2,sizeof(TaskStack2)/sizeof(unsigned long),0},

//  {TaskStack1,CREATE_SUSPENDED,Task1,sizeof(TaskStack1)/sizeof(unsigned long),0},
//  {TaskStack2,CREATE_SUSPENDED,Task2,sizeof(TaskStack2)/sizeof(unsigned long),0},
  {0,0,0,0}
  
};
  
*/