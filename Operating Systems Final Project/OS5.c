#include "OS5.h"


//global ver:
Mem_struct physical_memory;
int msg_id_arr[4],pid_arr[4];
pthread_t Mmu_ID, printer_ID, evicter_ID;
pthread_mutex_t lock;
int end_flag = 0;


int main()
{
	int i;
	srand(time(NULL)); //establish seed
	pthread_mutex_unlock(&lock);//init lock
	//init:
	for (i = 0; i < N; i++)
	{
		physical_memory.Memory[i] = -1;
	}
	physical_memory.oldest = 0;

	//queue for every process in order to communicate with each other.
	for (i=0; i < 4; i++)
		msg_id_arr[i] = msgget(IPC_PRIVATE, 0600|IPC_CREAT);

	//crate processes with fork.
	for (i = 0; i < 4; i++)
	{
		pid_arr[i] = fork();
		if (pid_arr[i] == -1)
		{
			endSim();
			perror(NULL);
		}
		//only child processes run.
		else if (!pid_arr[i])
		{
			switch (i)
			{
			case P1://process 1 
				process(P1);
				break;
			case P2://process 2
				process(P2);
				break;
			case MMU://MMU process
				MmuPorc();
				break;
			case HardDisk://HD process
				hd();
				break;
			}
		}
	}
	sleep(SIM_T);//time of simulation.
	printf("Successfully finished sim\n");
	endSim();
	return 0;
}




