#include "includes.h"

#define TASK_STK_SIZE 256
#define MAX_CARS 10
#define MAX_MSG 5
#define MSG_LENGTH 100

OS_STK  TaskStartStack[TASK_STK_SIZE];
OS_STK  TaskDriveInStack[TASK_STK_SIZE];
OS_STK  TaskCleanStack[TASK_STK_SIZE];
OS_STK  TaskDryStack[TASK_STK_SIZE];
OS_STK  TaskWaxStack[TASK_STK_SIZE];
OS_STK  TaskDriveOutStack[TASK_STK_SIZE];
OS_STK  TaskCarStack[10][TASK_STK_SIZE];
OS_STK  TaskPPrintStack[TASK_STK_SIZE]; 
OS_STK  TaskPrintStack[TASK_STK_SIZE];

void TaskDriveIn(void *pdata);	//	task: a car may enter the carwash
void TaskClean(void *pdata);	//	task: clean the car
void TaskDry(void *pdata);		//	task: dry the car
void TaskWax(void *pdata);		//	task: wax the car
void TaskDriveOut(void *pdata);	//	task: the car may leave the carwash
void TaskCar(void *pdata);
void preparePrint(char *s);
void TaskStart(void *pdata);
void TaskPPrint(void *pdata);
void TaskPrint(void *pdata);

OS_EVENT *sem_busy;
OS_EVENT *mqueu;
OS_EVENT *mail;

OS_FLAG_GRP *flags; 
OS_FLAG_GRP *flag2; 

boolean cars[MAX_CARS];

int main(void)
{
	INT8U err;
	
	OSInit();				// Initialize uCOS-II. 
	
	sem_busy = OSSemCreate(1);
	
	flags = OSFlagCreate(0x00, &err);
	flag2 = OSFlagCreate(0x00, &err);
	
	void * Messages[MAX_MSG];				// message pool pointers
	mqueu = OSQCreate(&Messages[0], MAX_MSG);
	
	mail = OSMboxCreate(NULL);

	OSTaskCreate(TaskStart, (void *) 0, &TaskStartStack[TASK_STK_SIZE - 1], 5);		//	create at least one task

	OSStart();				// Start multitasking.  

	return 0;				// NEVER EXECUTED
}

void TaskStart(void *pdata)			//	TaskOne
{	
	INT8U err;
	OSTaskCreate(TaskDriveIn, (void *) 0, &TaskDriveInStack[TASK_STK_SIZE - 1], 20);
	OSTaskCreate(TaskClean, (void *) 0, &TaskCleanStack[TASK_STK_SIZE - 1], 21);
	OSTaskCreate(TaskDriveOut, (void *) 0, &TaskDriveOutStack[TASK_STK_SIZE - 1], 22);
	OSTaskCreate(TaskDry, (void *) 0, &TaskDryStack[TASK_STK_SIZE - 1], 23);
	OSTaskCreate(TaskWax, (void *) 0, &TaskWaxStack[TASK_STK_SIZE - 1], 24);
	OSTaskCreate(TaskPPrint, (void *) 0, &TaskWaxStack[TASK_STK_SIZE - 1], 25);
	OSTaskCreate(TaskPrint, (void *) 0, &TaskWaxStack[TASK_STK_SIZE - 1], 26);

	int i;
	for(i=0;i<MAX_CARS;i++)
		cars[i] = 1;
	
	printf("Display started\n");
	OSFlagPost(flags, 0x01, OS_FLAG_SET, &err);
	
	while (1)
	{
		INT16U key;
		BOOLEAN avail = PC_GetKey(&key);
		if(avail == TRUE)
		{
			if(key == 27)
				exit(0);
			if(key == 'c' || key == 'C')
			{
				for(i=0;i<MAX_CARS; i++)
				{
					if(cars[i])
					{
						cars[i] = 0;
						OSTaskCreate(TaskCar, (void *) i, &TaskCarStack[i][TASK_STK_SIZE - 1], 10+i);
						break;
					}			
				}
			}
		}
		OSTimeDlyHMSM(0,0,0,100);  
	}
}

void TaskPPrint(void *pdata)
{
	INT8U err;
	void * msg;	
	while(1)
	{
		msg = OSQPend(mqueu, 0, &err);
		OSMboxPost(mail, msg);
	}
	
}

void TaskPrint(void *pdata)
{
	INT8U err;
	void * msg;
	while(1)
	{
		msg = OSMboxPend(mail, 0, &err);
		printf(msg);
		free(msg);
	}
}

void preparePrint(char *s)
{
	char * msg = (char*)malloc(MSG_LENGTH);
	sprintf(msg, "-------------------------------------->%s\n", s);
	OSQPost(mqueu, msg);
}

void TaskCar(void *pdata)
{
	INT8U err;
	char msg[MSG_LENGTH];
	
	int car = (int)pdata;
	
		sprintf(msg, "Car %d created\n", car);
		OSQPost(mqueu, &msg);
		
		OSTimeDlyHMSM(0,0,3,0);
		
		sprintf(msg, "Car %d waiting for carwash\n", car);
		OSQPost(mqueu, &msg);
		
		OSTimeDlyHMSM(0,0,3,0);
		
		OSSemPend(sem_busy,0,&err);
		
		sprintf(msg, "Car %d ready for entering carwash\n", car);
		OSQPost(mqueu, &msg);
		
		OSTimeDlyHMSM(0,0,3,0);
		OSFlagPost(flags, 0x01, OS_FLAG_SET, &err);
		
		sprintf(msg, "Car %d did enter carwash\n", car);
		OSQPost(mqueu, &msg);
		
		OSFlagPend(flag2, 0x01, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);
		
		sprintf(msg, "Car %d left carwash\n", car);
		OSQPost(mqueu, &msg);
	
		OSTimeDlyHMSM(0,0,3,0);
		
		cars[car] = 1;
		sprintf(msg, "Car %d left the system\n", car);
		OSQPost(mqueu, &msg);
		
		OSTimeDlyHMSM(0,0,3,0);
		OSSemPost(sem_busy);
		
	OSTaskDel(OS_PRIO_SELF);
}

void TaskDriveIn(void *pdata)
{
	INT8U err;
	char msg[MSG_LENGTH];
	int i,countCars = 0;
	while(1)
	{
		OSFlagPend(flags, 0x01, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);
		
		for(i=0;i<MAX_CARS;i++)
		{
			if(!cars[i])
				countCars++;
		}
		
		if(countCars > 0)
		{
			preparePrint("Car allowed to drive in");
			OSTimeDlyHMSM(0,0,3,0);
			OSFlagPost(flags, 0x02, OS_FLAG_SET, &err);
		}
		else
		{
			preparePrint("No waiting cars for this washing machine");
			OSTimeDlyHMSM(0,0,3,0);
		}
	
		
	}
}

void TaskClean(void *pdata)
{
	char msg[MSG_LENGTH];
	INT8U err;
	
	while(1)
	{
		OSFlagPend(flags, 0x02, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);
		preparePrint("Car is being cleaned");
		OSQPost(mqueu, &msg);
		OSTimeDlyHMSM(0,0,3,0);
		OSFlagPost(flags, 0x04, OS_FLAG_SET, &err);
	}
}

void TaskDry(void *pdata)
{
	char msg[MSG_LENGTH];
	INT8U err;
	
	while(1)
	{
		OSFlagPend(flags, 0x04, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);
		preparePrint("Car is drying");
		OSQPost(mqueu, &msg);
		OSTimeDlyHMSM(0,0,3,0);
		OSFlagPost(flags, 0x08, OS_FLAG_SET, &err);
	}
}

void TaskWax(void *pdata)
{
	char msg[MSG_LENGTH];
	INT8U err;
	
	while(1)
	{
		OSFlagPend(flags, 0x08, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);
		preparePrint("Car is being waxed");
		OSQPost(mqueu, &msg);
		OSTimeDlyHMSM(0,0,3,0);
		OSFlagPost(flags, 0x10, OS_FLAG_SET, &err);
	}
}

void TaskDriveOut(void *pdata)
{
	char msg[MSG_LENGTH];
	INT8U err;
	
	while(1)
	{
		OSFlagPend(flags, 0x10, OS_FLAG_WAIT_SET_ANY + OS_FLAG_CONSUME, 0, &err);
		preparePrint("Car is allowed to drive out");
		OSQPost(mqueu, &msg);
		OSTimeDlyHMSM(0,0,3,0);
		OSFlagPost(flag2, 0x01, OS_FLAG_SET, &err);
	}
}

