#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <syscall.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <mqueue.h>
#include <signal.h>
#include <pthread.h>
#include <memory.h>
#include <errno.h>
#include <unistd.h>





#define INTER_MEM_ACCS_T    10000000
#define MEM_WR_T            300000
#define N                   5
#define WR_RATE             0.99
#define SIM_T          	     1
#define HD_ACCS_T           2500
#define P1                  0
#define P2           	    1
#define MMU            	    2
#define HardDisk            3
#define READ_WRITE_TYPE     1
#define ACK                 2
#define HD_MSG              3
#define HIT_RATE 	   0.9
#define USED_SLOTS_TH	    2
#define TIME_BETWEEN_SNAPSHOTS  200000000




//mem struct
typedef struct Mem_struct
{
	int Memory[N];
	int oldest;
}Mem_struct;

//msg struct
typedef struct msg
{
	long int msg_type;
	int action; //1 to write and 0 to read
	int processId;
}msg;


//functions:
void* Mmu_Th(); //MMU msg handler
void* evicter(); //evictes if full.
int HitOrMiss(msg *message); //checks if hit or miss
int transmit_msg(int sendToPid, msg *message); //send message
int rec_msg(int curr_pid, msg *message,long int msg_type); //rec message
void* printer(); //print func
void endSim(); //exits and cleans resources.
void process(int curr_pid); //runs proccess
void hd();//HD
void MmuPorc(); //MMU



//global ver:
Mem_struct physical_memory;
int msg_id_arr[4],pid_arr[4],end_flag;
pthread_t Mmu_ID, printer_ID, evicter_ID;
pthread_mutex_t lock;
