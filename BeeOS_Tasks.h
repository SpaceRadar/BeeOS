extern void MainTask(void);
extern void Task1(void);
extern void Task2(void);
extern void Task3(void);
extern void Task4(void);
extern void Test(void);

unsigned long MainTaskStack[64];
unsigned long TaskStack1[64];
unsigned long TaskStack2[64];
unsigned long TaskStack3[64];
unsigned long TaskStack4[64];
unsigned long IdleTaskStack[64];

static volatile unsigned long counter=0;
__weak void IdleTask(void)
{
  while(1)
  {
    counter++;
  }
}


TCB_t TCB[]=
{
  {IdleTaskStack,0,IdleTask,sizeof(IdleTaskStack)/sizeof(unsigned long),0},
//  {MainTaskStack,0,Test,sizeof(MainTaskStack)/sizeof(unsigned long),0},  
  {MainTaskStack,0,MainTask,sizeof(MainTaskStack)/sizeof(unsigned long),0},  
  {TaskStack1,CREATE_SUSPENDED,Task1,sizeof(TaskStack1)/sizeof(unsigned long),0},
  {TaskStack2,CREATE_SUSPENDED,Task2,sizeof(TaskStack2)/sizeof(unsigned long),0},
  {TaskStack3,CREATE_SUSPENDED,Task3,sizeof(TaskStack3)/sizeof(unsigned long),0},
  {TaskStack4,CREATE_SUSPENDED,Task4,sizeof(TaskStack4)/sizeof(unsigned long),0},
//  {TaskStack1,CREATE_SUSPENDED,Task1,sizeof(TaskStack1)/sizeof(unsigned long),0},
//  {TaskStack2,CREATE_SUSPENDED,Task2,sizeof(TaskStack2)/sizeof(unsigned long),0},
  {0,0,0,0}
  
};
  
