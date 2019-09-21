/**************
 * now problem:
 *	1.use Modeflag to check swap to shell mode from which place ,
 *	  but when back to shell and i want to return ,
 *	  occour segmentation fault
 *
 *	  use a temp context to save,can return back but when i want to add
 *	  the task while stop status,it would wrong
**************/
#include "scheduling_simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#define STACKSIZE 8*1024
#define PIDSIZE 1000
typedef struct Task {
    ucontext_t uctx;
    int Pid;
    int TimeQuantum;
    int Priority;
    int State;//the number of task state
    int Qtime;//queueing time
    char TaskName[10];
    char TaskStack[STACKSIZE];
} Task;

typedef struct HQ {
    Task theHTask;
    struct HQ *next;
} HQ;

typedef struct LQ {
    Task theLTask;
    struct LQ *next;
} LQ;

//global variable
ucontext_t shell,sche,slp;//the contextw of shell and sche
ucontext_t temp;
char stack_sche[STACKSIZE];
char stack_shell[STACKSIZE];
char stack_slp[STACKSIZE];
int PIDRecorder[PIDSIZE]= {0};//0 = notask 1 = Htask 2 = Ltask
int PIDPoint = 0;
struct itimerval new_value,old_value,w_value;
int HorL = 0;//test now is H or L priority H = 1 L = 2
int HPnum = 0;//the number of task in HighQ
int LPnum = 0;//the number of tasl in LowQ

int flag = 0 ;//to check if the task terminal = 0 or stop by signal=1 wait=2
int Modeflag = 0;

int VTime=0;//virturl time
int TimeRecord[PIDSIZE] = {0};//to record the least leave time
int WaitRecord[PIDSIZE] = {0};//to record the time need to wakeup
int Waitnum=0;
//high priority Q and low priority Q
HQ *Hhead,*Hrear,*Htemp;
LQ *Lhead,*Lrear,*Ltemp;
int check = 100;



//the function
void KeyIn();//this is use to get the user command line
int SetPID();//this is to get the pid
int DelPID(int PID);
int AddTask(char* CommandIN);//this is to add the task into queue
void Scheduling();
void BackShell();//this is to handle the signal
void PS();//this function is to show all the state of tasks
void SetQtime(int enterleave);//1 = enter 2 = leave 3 = stop
//to check if there any wait task and wake up and minus the waiting time
void CheckWait();
void SleepWait();//if all task is waiting ,then use sleep 10ms and minus waiting
void Remove(int Removepid);
void LookLQ()
{
    printf("pid:%d\n",Lrear->theLTask.Pid);
    printf("quantum:%d\n",Lrear->theLTask.TimeQuantum);
    printf("priority:%d\n",Lrear->theLTask.Priority);
    printf("State:%d\n",Lrear->theLTask.State);
    printf("Qtime:%d\n",Lrear->theLTask.Qtime);
    printf("Name:%s\n",Lrear->theLTask.TaskName);
}
void LookHQ()
{
    printf("pid:%d\n",Hrear->theHTask.Pid);
    printf("quantum:%d\n",Hrear->theHTask.TimeQuantum);
    printf("priority:%d\n",Hrear->theHTask.Priority);
    printf("State:%d\n",Hrear->theHTask.State);
    printf("Qtime:%d\n",Hrear->theHTask.Qtime);
    printf("Name:%s\n",Hrear->theHTask.TaskName);
}

void signalHandler(int signo)
{
    switch(signo) {
    case SIGALRM:
//			printf("caught the SIGALRM signal\n");
        flag = 1;
        //to check which queue back to sche
        if(HorL == 1) {
//            printf("1\n");
            swapcontext(&(Htemp->theHTask.uctx),&sche);
        } else if(HorL == 2) {
//            printf("2\n");
            swapcontext(&(Ltemp->theLTask.uctx),&sche);
        }
        break;

    case SIGVTALRM:
//		printf("SIGVTALRM\n");
//        printf("3\n");
        swapcontext(&slp,&sche);
        break;
    }
}

void whiletask()
{
    while(1);
}
//four api function
void hw_suspend(int msec_10);

void hw_wakeup_pid(int pid);

int hw_wakeup_taskname(char *task_name);

int hw_task_create(char *task_name);

int main()
{
    //init global variable
//    int ComMode = 0;//command mode etc add
//    char CommandLine[1024]="";
//	int check;
    signal(SIGVTALRM,signalHandler);
    //create the context of scheduling
    getcontext(&sche);
    sche.uc_stack.ss_sp = stack_sche;
    sche.uc_stack.ss_size = sizeof(stack_sche);
    sche.uc_link = &shell;
    makecontext(&sche,Scheduling,0);
    //create the context of shell mode
    getcontext(&shell);
    shell.uc_stack.ss_sp = stack_shell;
    shell.uc_stack.ss_size = sizeof(stack_shell);
    shell.uc_link = &shell;
    makecontext(&shell,KeyIn,0);
    //create the context of sleep mode
    getcontext(&slp);
    slp.uc_stack.ss_sp = stack_slp;
    slp.uc_stack.ss_size = sizeof(stack_slp);
    slp.uc_link = &sche;
    makecontext(&slp,whiletask,0);
    //set to shell
    setcontext(&shell);



    while(1);
    return 0;

}


void KeyIn()
{
    while(1) {
        new_value.it_value.tv_usec = 0;
        w_value.it_value.tv_usec = 0;
        setitimer(ITIMER_VIRTUAL,&w_value,&old_value);
        setitimer(ITIMER_REAL,&new_value,&old_value);
//       flag = 1;
        char Linein[1024]="";
        char Ctype[10]="";//command type
        int InComm = 0; //use to check which command
        int pidreceive;
        printf("$ ");
        fgets(Linein,sizeof(Linein),stdin);
//		strcpy(CL,Linein);
        //following is to check which user command want
        //add = 1 start = 2 remove = 3 ps = 4 else =5
        sscanf(Linein,"%s",Ctype);
        if(strncmp(Ctype,"add",3) == 0) {
            pidreceive = AddTask(Linein);
//            printf("pidreceive:%d\n",pidreceive);
            //      if(Lrear != NULL)
            //          LookLQ();
            //      if(Hrear != NULL)
            //          LookHQ();
            //            break;

            InComm = 1;
        } else if(strncmp(Ctype,"start",5) == 0) {
            printf("simulating...\n");
            //to decide swap to shell or scheduling(high or low)
//            printf("Modeflag:%d\n",Modeflag);
            /*
            							if(Modeflag == 1 || Modeflag ==0){
            								swapcontext(&shell,&sche);
            							}
            							else if(Modeflag == 2){
            								swapcontext(&shell,&(Ltemp->theLTask.uctx));
            							}
            							else if(Modeflag ==3){
            								swapcontext(&shell,&(Htemp->theHTask.uctx));
            							}
            */
            //        if(Modeflag == 0)
//            printf("flag:%d\n",flag);
//            printf("flagadd:%p\n",&flag);
//            printf("4\n");
//            printf("shell:%p,sche:%p\n",&shell,&sche);
            check = swapcontext(&shell,&sche);

//            printf("check:%d\n",flag);
            //            else{
            //				printf("->%p\n", Ltemp);
            //               swapcontext(&shell,&(Ltemp->theLTask.uctx));
            //			}

//            printf("back\n");

            InComm = 2;
        } else if(strncmp(Ctype,"remove",6) == 0) {
            int RP;
            sscanf(Linein,"%*s %d",&RP);
//            printf("the pid remove:%d\n",RP);
            Remove(RP);
            InComm = 3;
        } else if(strncmp(Ctype,"ps",2) == 0) {
            PS();
            InComm = 4;
        } else {
            printf("Invalid Command\n");
            InComm = 5;
        }

    }
    return;
}


void Scheduling()
{
//    int i = 0;
//    float j = 0.01;
    signal(SIGTSTP,BackShell);
    signal(SIGALRM,signalHandler);
    //	Htemp = Hhead;
    //	Ltemp = Lhead;
    while(1) {
        //		printf("simulating...\n");
        //test high or low priority or just waiting or nothing
        if(HPnum != 0) {
            HorL = 1;
        } else if(LPnum != 0) {
            HorL = 2;
        } else if(Waitnum != 0) {
            HorL = 3;
        } else {
            HorL = 0;
        }
        //		printf("Hn:%d Ln:%d HORL:%d\n",HPnum,LPnum,HorL);
        //run high or low queue or terminate
        switch(HorL) {
        case 1:
            if(Htemp == NULL) {
                return;
            }
            //find the next task to do
            while(Htemp->theHTask.State == TASK_TERMINATED || Htemp->theHTask.State == TASK_WAITING) {
//                printf("whiele\n");
                Htemp = Htemp->next;
            }
//			printf("outwhie\n");
            //set the timer
            new_value.it_value.tv_sec = 0;
            new_value.it_value.tv_usec = Htemp->theHTask.TimeQuantum * 1000;
            new_value.it_interval.tv_sec = 0;
            new_value.it_interval.tv_usec = 0;
            //set the Timequantum and VTime
//            SetQtime(1);
            Htemp->theHTask.State = TASK_RUNNING;
            //set the modeflag to high = 3
//            Modeflag = 3;
            //swap to high task
            flag = 0;//*************************************************
            setitimer(ITIMER_REAL,&new_value,&old_value);
//            printf("5\n");
            swapcontext(&sche,&(Htemp->theHTask.uctx));
            //swap back and set the modeflag to sche = 1
            //				Modeflag = 0;
            //settime =>leave
//            SetQtime(2);
            SetQtime(3);
            CheckWait();
            //back by signal
            if(flag == 1) {
                Htemp->theHTask.State = TASK_READY;
                flag = 0;
            }
            //back by it self
            else if (flag == 0) {
                Htemp->theHTask.State = TASK_TERMINATED;
                //beacuse the task is terminate -> num--
                HPnum--;
                flag = 0;
                Modeflag = 0;
            }
            //back by suspend
            else if(flag == 2) {
                Htemp->theHTask.State = TASK_WAITING;
                HPnum--;
                Waitnum++;
                flag = 0;
            }
            //to stop the timer
            new_value.it_value.tv_usec = 0;
            setitimer(ITIMER_REAL,&new_value,&old_value);

            Htemp = Htemp->next;
            break;
        case 2:
            if(Ltemp == NULL) {
                return;
            }
            //find the next task to do
            while(Ltemp->theLTask.State == TASK_TERMINATED || Ltemp->theLTask.State == TASK_WAITING) {
                Ltemp = Ltemp->next;
            }
            //set the timer
            new_value.it_value.tv_sec = 0;
            new_value.it_value.tv_usec = Ltemp->theLTask.TimeQuantum * 1000;
            new_value.it_interval.tv_sec = 0;
            new_value.it_interval.tv_usec = 0;
            //set Timequantum and Vtime
//            SetQtime(1);
            Ltemp->theLTask.State = TASK_RUNNING;
            //set the mode flag to low = 2
//            Modeflag = 2;
            //swap to low task
            flag = 0;//*******************************************
            setitimer(ITIMER_REAL,&new_value,&old_value);
//            printf("6\n");
            swapcontext(&sche,&(Ltemp->theLTask.uctx));
            //swap back and set the mode flag to sche = 1
//			Modeflag = 0;
            //settime =>leave
//            SetQtime(2);
            SetQtime(3);
            CheckWait();
            //back by signal
//				printf("flag:%d\n",flag);////////****
            if(flag == 1) {
                Ltemp->theLTask.State = TASK_READY;
//                printf("hi\n");
                flag = 0;
            }
            //back by it self
            else if (flag == 0) {
//					printf("ho98\n");
                Ltemp->theLTask.State = TASK_TERMINATED;
                //beacuse the task is terminate -> num--
                LPnum--;
//                printf("hi98\n");
                flag = 0;
                Modeflag = 0;
            }
            //back by suspend
            else if(flag == 2) {
                Ltemp->theLTask.State = TASK_WAITING;
                LPnum--;
                Waitnum++;
                flag = 0;
            }
            //to stop the timer
            new_value.it_value.tv_usec = 0;
            setitimer(ITIMER_REAL,&new_value,&old_value);

            Ltemp = Ltemp->next;
            break;
        case 3:
//			printf("justwait\n");
            w_value.it_value.tv_usec = 10000;
            setitimer(ITIMER_VIRTUAL,&w_value,NULL);
//            printf("7\n");
            swapcontext(&sche,&slp);
//            printf("100\n");
            SleepWait();
//            printf("101\n");
            break;
        case 0:
//            printf("8\n");
            swapcontext(&sche,&shell);
            break;
        }
    }
    return ;
}

int AddTask(char* CommandIN)
{
    int TaskID = 0;//theree are six tasks //if = 0 means the task is not exsist
    int TaskPID = -1;
    int i;
    int TestPri=0;//1 = L 2 = H
    int TestTime=0;//1 = S 2 = L
    char Command[1024]="";
    char Info[5][10];
    char* temp;

    HQ *Hnewnode;
    LQ *Lnewnode;
    sscanf(CommandIN,"%[^'\n']",Command);
    //set the info defoult
    strcpy(Info[1],"-t");
    strcpy(Info[2],"S");
    strcpy(Info[3],"-p");
    strcpy(Info[4],"L");
    //put the command line into info
    temp = strtok(Command," ");
    temp = strtok(NULL," ");
    i = 0;
    while( temp != NULL ) {
        strcpy(Info[i],temp);
        temp = strtok(NULL," ");
        i++;
    }



    //to test which test user want to create
    if( strncmp(Info[0],"task1",5) == 0) {
        TaskID = 1;
    } else if( strncmp(Info[0],"task2",5) == 0) {
        TaskID = 2;
    } else if( strncmp(Info[0],"task3",5) == 0) {
        TaskID = 3;
    } else if( strncmp(Info[0],"task4",5) == 0) {
        TaskID = 4;
    } else if( strncmp(Info[0],"task5",5) == 0) {
        TaskID = 5;
    } else if( strncmp(Info[0],"task6",5) == 0) {
        TaskID = 6;
    } else {
        TaskID = 0;
        TaskPID = -1;
        printf("Not exist this task!\n");
        return TaskPID;
    }

    //check priority and time quantum
    //info[1]
    if( strncmp(Info[1],"-t",2) == 0) {
        if( strncmp(Info[2],"S",1) == 0) {
            TestTime = 1;
        } else if( strncmp(Info[2],"L",1) == 0) {
            TestTime = 2;
        }
    } else if( strncmp(Info[1],"-p",2) == 0) {
        if( strncmp(Info[2],"L",1) == 0) {
            TestPri = 1;
        } else if( strncmp(Info[2],"H",1) == 0) {
            TestPri = 2;
        }
    }
    //info[3]
    if( strncmp(Info[3],"-t",2) == 0) {
        if( strncmp(Info[4],"S",1) == 0) {
            TestTime = 1;
        } else if( strncmp(Info[4],"L",1) == 0) {
            TestTime = 2;
        }
    } else if( strncmp(Info[3],"-p",2) == 0) {
        if( strncmp(Info[4],"L",1) == 0) {
            TestPri = 1;
        } else if( strncmp(Info[4],"H",1) == 0) {
            TestPri = 2;
        }
    }


    //to create the task
    TaskPID = SetPID();//give the pid
    //the time add into queue
    TimeRecord[TaskPID-1] = VTime;
    //put high or low priority
    switch(TestPri) {
    case 1:
        //to record what the priority of pid
        PIDRecorder[TaskPID-1] = 2;
        Lnewnode = (LQ*)malloc(sizeof(LQ));

        Lnewnode->theLTask.Pid = TaskPID;
        //set time quantum
        if(TestTime == 1) {
            Lnewnode->theLTask.TimeQuantum = 10;
        } else if(TestTime == 2) {
            Lnewnode->theLTask.TimeQuantum = 20;
        }
        //set priority
        Lnewnode->theLTask.Priority = TestPri;
        //set state
        Lnewnode->theLTask.State = TASK_READY;
        //set queue time and init to 0
        Lnewnode->theLTask.Qtime = 0;
        //set taskname
        strcpy(Lnewnode->theLTask.TaskName,Info[0]);
        //set the context
        getcontext(&(Lnewnode->theLTask.uctx));
        Lnewnode->theLTask.uctx.uc_stack.ss_sp = Lnewnode->theLTask.TaskStack;
        Lnewnode->theLTask.uctx.uc_stack.ss_size = sizeof(Lnewnode->theLTask.TaskStack);
        Lnewnode->theLTask.uctx.uc_link = &sche;
        switch(TaskID) {
        case 1:
            makecontext(&(Lnewnode->theLTask.uctx),task1,0);
            break;
        case 2:
            makecontext(&(Lnewnode->theLTask.uctx),task2,0);
            break;
        case 3:
            makecontext(&(Lnewnode->theLTask.uctx),task3,0);
            break;
        case 4:
            makecontext(&(Lnewnode->theLTask.uctx),task4,0);
            break;
        case 5:
            makecontext(&(Lnewnode->theLTask.uctx),task5,0);
            break;
        case 6:
            makecontext(&(Lnewnode->theLTask.uctx),task6,0);
            break;
        default:
            break;
        }
        //add this tasknode to queue
        //Lnewnode->next = NULL;
        if(Lhead == NULL) {
            Lhead = Lnewnode;
            Lrear = Lnewnode;
            Ltemp = Lnewnode;
        } else {
            Lrear->next = Lnewnode;
            Lrear = Lnewnode;
        }
        Lnewnode->next = Lhead;
        //add the number of task in low q
        LPnum++;
        break;
    case 2:
        Hnewnode = (HQ*)malloc(sizeof(HQ));

        Hnewnode->theHTask.Pid = TaskPID;
        //set time quantum
        if(TestTime == 1) {
            Hnewnode->theHTask.TimeQuantum = 10;
        } else if(TestTime == 2) {
            Hnewnode->theHTask.TimeQuantum = 20;
        }
        //set priority
        Hnewnode->theHTask.Priority = TestPri;
        //set state
        Hnewnode->theHTask.State = TASK_READY;
        //set queue time and init to 0
        Hnewnode->theHTask.Qtime = 0;
        //set taskname
        strcpy(Hnewnode->theHTask.TaskName,Info[0]);
        //set the context
        getcontext(&(Hnewnode->theHTask.uctx));
        Hnewnode->theHTask.uctx.uc_stack.ss_sp = Hnewnode->theHTask.TaskStack;
        Hnewnode->theHTask.uctx.uc_stack.ss_size = sizeof(Hnewnode->theHTask.TaskStack);
        Hnewnode->theHTask.uctx.uc_link = &sche;
        switch(TaskID) {
        case 1:
            makecontext(&(Hnewnode->theHTask.uctx),task1,0);
            break;
        case 2:
            makecontext(&(Hnewnode->theHTask.uctx),task2,0);
            break;
        case 3:
            makecontext(&(Hnewnode->theHTask.uctx),task3,0);
            break;
        case 4:
            makecontext(&(Hnewnode->theHTask.uctx),task4,0);
            break;
        case 5:
            makecontext(&(Hnewnode->theHTask.uctx),task5,0);
            break;
        case 6:
            makecontext(&(Hnewnode->theHTask.uctx),task6,0);
            break;
        default:
            break;
        }
        //add this tasknode to queue
        //Hnewnode->next = NULL;
        if(Hhead == NULL) {
            Hhead = Hnewnode;
            Hrear = Hnewnode;
            Htemp = Hnewnode;
        } else {
            Hrear->next = Hnewnode;
            Hrear = Hnewnode;
        }
        Hnewnode->next = Hhead;
        //add the number of task in high q
        HPnum++;

        break;
    default:
        break;
    }

    return TaskPID;

}

void PS()
{
    int Maxpid_1 = PIDPoint;//to get the max pid
    int i;
    HQ *Hpspoint;
    LQ *Lpspoint;
    Hpspoint = Hhead;
    Lpspoint = Lhead;
//    printf("the number:%d \n",Maxpid_1);
    //to travel all pid
    for(i = 0 ; i < Maxpid_1 ; i++) {
        //to sure there is no null node enter
        if(Hpspoint != NULL || Lpspoint != NULL) {
            //if the pid is high priority

            if(PIDRecorder[i] == 1) {
                printf("%-4d ",Hpspoint->theHTask.Pid);
                printf("%s ",Hpspoint->theHTask.TaskName);
                //to printf the state of task in ready queue
                switch(Hpspoint->theHTask.State) {
                case TASK_RUNNING:
                    printf("TASK_RUNNING     ");
                    break;
                case TASK_READY:
                    printf("TASK_READY       ");
                    break;
                case TASK_WAITING:
                    printf("TASK_WAITING     ");
                    break;
                case TASK_TERMINATED:
                    printf("TASK_TERMINATED  ");
                    break;
                }
                printf("%-7d",Hpspoint->theHTask.Qtime);
                //print the priority because it in high Q
                printf("H ");
                //print the time quantum
                switch(Hpspoint->theHTask.TimeQuantum) {
                case 10:
                    printf("S\n");
                    break;
                case 20:
                    printf("L\n");
                    break;

                }
                //to point the Hpspoint to next nope
                Hpspoint = Hpspoint->next;
            }
            //if the task is low priority

            else if(PIDRecorder[i] == 2) {
                printf("%-4d ",Lpspoint->theLTask.Pid);
                printf("%s ",Lpspoint->theLTask.TaskName);
                //to printf the state of task in ready queue
                switch(Lpspoint->theLTask.State) {
                case TASK_RUNNING:
                    printf("TASK_RUNNING     ");
                    break;
                case TASK_READY:
                    printf("TASK_READY       ");
                    break;
                case TASK_WAITING:
                    printf("TASK_WAITING     ");
                    break;
                case TASK_TERMINATED:
                    printf("TASK_TERMINATED  ");
                    break;
                }
                printf("%-7d",Lpspoint->theLTask.Qtime);
                //print the priority because it in high Q
                printf("L ");
                //print the time quantum
                switch(Lpspoint->theLTask.TimeQuantum) {
                case 10:
                    printf("S\n");
                    break;
                case 20:
                    printf("L\n");
                    break;
                }
                //to point the Lpspoint to next nope
                Lpspoint = Lpspoint->next;

            }
        }
    }


}

int SetPID()
{
    int Temp = PIDPoint;//temp pidpoint
    int ReturnTPID;//return test PID
    int i;
    //test which pid has not been used
    for(i = 0 ; i < PIDSIZE ; i++) {
        if(PIDRecorder[Temp] == 0) {
            ReturnTPID = Temp + 1;
            PIDRecorder[Temp] = 1;
            Temp = (Temp+1) % PIDSIZE;
            PIDPoint = Temp;//set the new PIDPoint
            break;
        } else {
            Temp = (Temp+1) % PIDSIZE;
            if(i == (PIDSIZE-1) && Temp == PIDPoint) {
                ReturnTPID = -1;//the pidRecoder is full
            }
        }
    }

    return ReturnTPID;

}

int DelPID(int PID)
{
    int TempPID = PID - 1;
    int success = 1;
    if(PIDRecorder[TempPID] == 1) {
        PIDRecorder[TempPID] = 0;
    } else if(PIDRecorder[TempPID] == 0) {
        success = -1;
    }

    return success;


}

void BackShell()
{
    printf("\n");
    w_value.it_value.tv_usec = 0;
    new_value.it_value.tv_usec = 0;
    setitimer(ITIMER_VIRTUAL,&w_value,NULL);
    setitimer(ITIMER_REAL,&new_value,&old_value);
    //to check back to shell from which place
    //scheduling to shell
    /* 	if(Modeflag == 1){
      		swapcontext(&sche,&shell);
      	}
      	//low priority to shell
      	else if(Modeflag == 2){
      		swapcontext(&(Ltemp->theLTask.uctx),&shell);
      		printf("HIL\n");
      	}
       //high priority to shell
      	else if(Modeflag == 3){
      		swapcontext(&(Htemp->theHTask.uctx),&shell);
      		printf("HIH\n");
      	}
    */
//    flag = 1;
//    printf("fuck:%d\n",flag);
//    if(Ltemp ==NULL) {
//        printf("sdf\n");
//    }

//    SetQtime(3);
//    printf("HI\n");
    if(HorL != 3)
        flag = 1;
    else if(HorL == 3)
        flag = 2;

    //swapcontext(&(temp),&shell);
//    printf("9\n");
    setcontext(&shell);
}

void SetQtime(int enterleave)
{
    HQ *THtemp;
    LQ *TLtemp;
    //count the least leave to now
    if(enterleave == 1) {
        switch(HorL) {
        case 1:
            Htemp->theHTask.Qtime += VTime - TimeRecord[Htemp->theHTask.Pid-1];
            break;
        case 2:
            Ltemp->theLTask.Qtime += VTime - TimeRecord[Ltemp->theLTask.Pid-1];
            break;

        }
        //while leave the task then add the time and refreash the leaving time
    } else if(enterleave == 2) {
        switch(HorL) {
        case 1:
            VTime += Htemp->theHTask.TimeQuantum;
            TimeRecord[Htemp->theHTask.Pid-1] = VTime;
            break;
        case 2:
            VTime += Ltemp->theLTask.TimeQuantum;
            TimeRecord[Ltemp->theLTask.Pid-1] = VTime;
            break;

        }
    } else if(enterleave == 3) {
        //refreash HQ
        if(Htemp == NULL && Hhead == NULL && Hrear == NULL) {
//            printf("no H\n");
        } else {
            THtemp = Hhead;
            while(1) {
                if(THtemp->theHTask.State == TASK_READY) {
                    if(HorL == 1)
                        THtemp->theHTask.Qtime += Htemp->theHTask.TimeQuantum;
                    else if(HorL == 2)
                        THtemp->theHTask.Qtime += Ltemp->theLTask.TimeQuantum;
                }
                //terminate or waiting or running no need
                if(THtemp == Hrear) {
                    break;
                } else {
                    THtemp = THtemp->next;
                }

            }
        }
        //refreash LQ
        if(Ltemp == NULL && Lhead == NULL && Lrear == NULL) {
//            printf("no L\n");
        } else {
            TLtemp = Lhead;
            while(1) {
                if(TLtemp->theLTask.State == TASK_READY) {
                    if(HorL == 1)
                        TLtemp->theLTask.Qtime += Htemp->theHTask.TimeQuantum;
                    else if(HorL == 2)
                        TLtemp->theLTask.Qtime += Ltemp->theLTask.TimeQuantum;
                }
                //terminate or waiting or running no need

                if(TLtemp == Lrear) {
                    break;
                } else {
                    TLtemp = TLtemp->next;
                }

            }
        }
    }

}
void CheckWait()
{
    HQ *THtemp;
    LQ *TLtemp;
    if(Waitnum > 0) {
        if(Htemp == NULL && Hhead == NULL && Hrear == NULL) {
//            printf("no H\n");
        } else {
            THtemp = Hhead;
            while(1) {
                if(THtemp->theHTask.State == TASK_WAITING) {
                    if(HorL == 1)
                        WaitRecord[THtemp->theHTask.Pid-1] -= Htemp->theHTask.TimeQuantum;
                    else if(HorL == 2)
                        WaitRecord[THtemp->theHTask.Pid-1] -= Ltemp->theLTask.TimeQuantum;
//					printf("%d waiting : %d\n",THtemp->theHTask.Pid,WaitRecord[THtemp->theHTask.Pid-1]);
                    //if wait time < 0 then wackup
                    if(WaitRecord[THtemp->theHTask.Pid-1] <= 0) {
//						printf("11111\n");
//						while(1);
                        THtemp->theHTask.State = TASK_READY;
                        WaitRecord[THtemp->theHTask.Pid-1] = 0;
                        Waitnum--;
                        HPnum++;
                    }

                }
                //terminate or ready or running no need
                if(THtemp == Hrear) {
                    break;
                } else {
                    THtemp = THtemp->next;
                }

            }
        }
        //refreash LQ
        if(Ltemp == NULL && Lhead == NULL && Lrear == NULL) {
//            printf("no L\n");
        } else {
            TLtemp = Lhead;
            while(1) {
                if(TLtemp->theLTask.State == TASK_WAITING) {
                    if(HorL == 1)
                        WaitRecord[TLtemp->theLTask.Pid-1] -= Htemp->theHTask.TimeQuantum;
                    else if(HorL == 2)
                        WaitRecord[TLtemp->theLTask.Pid-1] -= Ltemp->theLTask.TimeQuantum;
                    //if wake time > 0 then wack up
//					printf("%d waiting : %d\n",TLtemp->theLTask.Pid,WaitRecord[TLtemp->theLTask.Pid-1]);
                    if(WaitRecord[TLtemp->theLTask.Pid-1] <= 0) {
                        TLtemp->theLTask.State = TASK_READY;
                        WaitRecord[TLtemp->theLTask.Pid-1] = 0;
                        Waitnum--;
                        LPnum++;
                    }

                }
                //terminate or waiting or running no need

                if(TLtemp == Lrear) {
                    break;
                } else {
                    TLtemp = TLtemp->next;
                }

            }
        }

    }
}
void SleepWait()
{
//while(1){
    HQ *THtemp;
    LQ *TLtemp;
    if(Waitnum > 0) {
        if(Htemp == NULL && Hhead == NULL && Hrear == NULL) {
//            printf("no H\n");
        } else {
            THtemp = Hhead;
            while(1) {
                if(THtemp->theHTask.State == TASK_WAITING) {
                    WaitRecord[THtemp->theHTask.Pid-1] -= 10;
//                    printf("%d waiting : %d\n",THtemp->theHTask.Pid,WaitRecord[THtemp->theHTask.Pid-1]);
                    //if wait time < 0 then wackup
                    if(WaitRecord[THtemp->theHTask.Pid-1] <= 0) {
//                        printf("11111\n");
//						while(1);
                        THtemp->theHTask.State = TASK_READY;
                        WaitRecord[THtemp->theHTask.Pid-1] = 0;
                        flag = 1;
                        Waitnum--;
                        HPnum++;
                    }

                }
                //terminate or ready or running no need
                if(THtemp == Hrear) {
                    break;
                } else {
                    THtemp = THtemp->next;
                }

            }
        }
        //refreash LQ
        if(Ltemp == NULL && Lhead == NULL && Lrear == NULL) {
//            printf("no L\n");
        } else {
            TLtemp = Lhead;
            while(1) {
                if(TLtemp->theLTask.State == TASK_WAITING) {
                    WaitRecord[TLtemp->theLTask.Pid-1] -= 10;
                    //if wake time > 0 then wack up
//                    printf("%d waiting : %d\n",TLtemp->theLTask.Pid,WaitRecord[TLtemp->theLTask.Pid-1]);
                    if(WaitRecord[TLtemp->theLTask.Pid-1] <= 0) {
                        TLtemp->theLTask.State = TASK_READY;
                        WaitRecord[TLtemp->theLTask.Pid-1] = 0;
                        flag = 1;
                        Waitnum--;
                        LPnum++;
                    }

                }
                //terminate or waiting or running no need

                if(TLtemp == Lrear) {
                    break;
                } else {
                    TLtemp = TLtemp->next;
                }

            }
        }

    }
//	usleep(10000);
//	swapcontext(&slp,&sche);
//}
}
void Remove(int Removepid)
{
//    printf("HroL:%d\n",HorL);
    int success;
    //check the pid need to remove in which queue;
    HQ *Hcur;
    HQ *Hpre;
    LQ *Lcur;
    LQ *Lpre;
    int taskstate;
    switch(PIDRecorder[Removepid-1]) {
    //the pid is in H
    case 1:
        Hcur = Hhead;
        //test pid in which node
        while(Hcur != NULL && Hcur->theHTask.Pid != Removepid) {
            Hpre = Hcur;
            Hcur = Hcur->next;
        }
        //get the task state
        taskstate = Hcur->theHTask.State;

        //if the pid is at head
        if(Hcur == Hhead) {
            //reset the head
            Hhead = Hhead->next;
            //reset the circular
            Hrear->next = Hhead;
            //delete the task
            free(Hcur);
            Hcur = NULL;
        }
        //id the pid is not at head
        else {
            Hpre->next = Hcur->next;
            if(Hcur == Hrear) {
                Hrear = Hpre;
            }
            free(Hcur);
            Hcur = NULL;
        }
        success = DelPID(Removepid);
//        printf("success:%d\n",success);
        PIDRecorder[Removepid-1] = 0;
        TimeRecord[Removepid-1] = 0;
        WaitRecord[Removepid-1] = 0;
        //when task is terminate
        if(taskstate == TASK_READY || taskstate == TASK_RUNNING) {
            HPnum--;
        } else if(taskstate == TASK_WAITING) {
//            HPnum--;
            Waitnum--;
        }


        break;
    //the pid is in L
    case 2:
        Lcur = Lhead;
        //test pid in which node
        while(Lcur != NULL && Lcur->theLTask.Pid != Removepid) {
            Lpre = Lcur;
            Lcur = Lcur->next;
        }
        //get the task state
        taskstate = Lcur->theLTask.State;

        //if the pid is at head
        if(Lcur == Lhead) {
            //reset the head
            Lhead = Lhead->next;
            //reset the circular
            Lrear->next = Lhead;
            //delete the task
            free(Lcur);
            Lcur = NULL;
        }
        //id the pid is not at head
        else {
            Lpre->next = Lcur->next;
            if(Lcur == Lrear) {
                Lrear = Lpre;
            }
            free(Lcur);
            Lcur = NULL;
        }
        success = DelPID(Removepid);
//        printf("success:%d\n",success);
        PIDRecorder[Removepid-1] = 0;
        TimeRecord[Removepid-1] = 0;
        WaitRecord[Removepid-1] = 0;
        //when task is terminate
        if(taskstate == TASK_READY || taskstate == TASK_RUNNING) {
            LPnum--;
        } else if(taskstate == TASK_WAITING) {
//            HPnum--;
            Waitnum--;
        }



        break;
    //the pid is not create
    case 0:
        printf("not exist this process\n");
        break;
    case 3:
        printf("HI waiting\n");
        break;
    }




}
void hw_suspend(int msec_10)
{
    new_value.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL,&new_value,&old_value);
    switch(HorL) {
    case 1:
        //set the flag to waiting = 2
        flag = 2;
        //set the wake time
        WaitRecord[Htemp->theHTask.Pid-1] = (msec_10*10);
        //swap to scheduling
//        printf("10\n");
        swapcontext(&(Htemp->theHTask.uctx),&sche);

        break;
    case 2:
        //set the flag to 2 =waiting
        flag = 2;
        //set the wake time
        WaitRecord[Ltemp->theLTask.Pid-1] = (msec_10*10);
        //swap to scheduling
//        printf("11\n");
        swapcontext(&(Ltemp->theLTask.uctx),&sche);
        break;
    }
}

void hw_wakeup_pid(int pid)
{
    HQ *THtemp;
    LQ *TLtemp;
    if(Waitnum > 0) {
        if(Htemp == NULL && Hhead == NULL && Hrear == NULL) {
//            printf("no H\n");
        } else {
            THtemp = Hhead;
            while(1) {
                if(THtemp->theHTask.State == TASK_WAITING && THtemp->theHTask.Pid == pid) {
                    THtemp->theHTask.State = TASK_READY;
                    WaitRecord[THtemp->theHTask.Pid-1] = 0;
                    Waitnum--;
                    HPnum++;
                }
                //terminate or ready or running no need
                if(THtemp == Hrear) {
                    break;
                } else {
                    THtemp = THtemp->next;
                }

            }
        }
        //refreash LQ
        if(Ltemp == NULL && Lhead == NULL && Lrear == NULL) {
//            printf("no L\n");
        } else {
            TLtemp = Lhead;
            while(1) {
                if(TLtemp->theLTask.State == TASK_WAITING && TLtemp->theLTask.Pid == pid) {
                    TLtemp->theLTask.State = TASK_READY;
                    WaitRecord[TLtemp->theLTask.Pid-1] = 0;
                    Waitnum--;
                    LPnum++;

                }
                //terminate or waiting or running no need

                if(TLtemp == Lrear) {
                    break;
                } else {
                    TLtemp = TLtemp->next;
                }

            }
        }

    }

    return;
}

int hw_wakeup_taskname(char *task_name)
{
    int wakenum = 0;
    HQ *THtemp;
    LQ *TLtemp;
    if(Waitnum > 0) {
        if(Htemp == NULL && Hhead == NULL && Hrear == NULL) {
//            printf("no H\n");
        } else {
            THtemp = Hhead;
            while(1) {
                if(THtemp->theHTask.State == TASK_WAITING && strncmp(THtemp->theHTask.TaskName,task_name,5) == 0 ) {
                    THtemp->theHTask.State = TASK_READY;
                    WaitRecord[THtemp->theHTask.Pid-1] = 0;
                    Waitnum--;
                    HPnum++;
                    wakenum++;
                }
                //terminate or ready or running no need
                if(THtemp == Hrear) {
                    break;
                } else {
                    THtemp = THtemp->next;
                }

            }
        }
        //refreash LQ
        if(Ltemp == NULL && Lhead == NULL && Lrear == NULL) {
//           printf("no L\n");
        } else {
            TLtemp = Lhead;
            while(1) {
                if(TLtemp->theLTask.State == TASK_WAITING && strncmp(TLtemp->theLTask.TaskName,task_name,5) == 0) {
                    TLtemp->theLTask.State = TASK_READY;
                    WaitRecord[TLtemp->theLTask.Pid-1] = 0;
                    Waitnum--;
                    LPnum++;
                    wakenum++;

                }
                //terminate or waiting or running no need

                if(TLtemp == Lrear) {
                    break;
                } else {
                    TLtemp = TLtemp->next;
                }

            }
        }

    }

    return wakenum;
}

int hw_task_create(char *task_name)
{
    int ReturnPID = -1;
    char CommandInFunction[20] ="add ";//this is the container of task name
    //to let the taskname to be the command line
    strcat(CommandInFunction,task_name);
    ReturnPID = AddTask(CommandInFunction);
    return ReturnPID; // the pid of created task name
}

