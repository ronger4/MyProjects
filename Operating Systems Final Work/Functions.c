#include "OS5.h"



int rec_msg(int curr_pid,msg *message,long int msg_type)
{
	return (int)msgrcv(msg_id_arr[curr_pid],message,(sizeof(msg)-sizeof(long)),msg_type,0);
}

int transmit_msg(int sendToPid, msg *message)
{
	return msgsnd(msg_id_arr[sendToPid],message,(sizeof(msg)-sizeof(long)),0);
}

void endSim()
{
	int i;
	
	//waits for threads to end.
	pthread_join(printer_ID,NULL);
	pthread_join(evicter_ID,NULL);
	pthread_join(Mmu_ID,NULL);
	//end_flag = 1;

	//kills processes
	for (i = 0; i < 4; i++)
		kill(pid_arr[i],SIGKILL);
	pthread_mutex_unlock(&lock);
}

void process(int curr_pid)
{
	msg message;
	struct timespec delay;
	//set timer.
	delay.tv_sec = 0;
	delay.tv_nsec = INTER_MEM_ACCS_T;
	while(1)
	{
		nanosleep(&delay, NULL); //sleep
		//set message properties
		message.msg_type = READ_WRITE_TYPE;
		message.action = (rand()%10 < (WR_RATE*10));
		message.processId = curr_pid;

		//message sent to MMU
		if(transmit_msg(MMU,&message) < 0)
		{
			endSim();
			perror(NULL);
		}
		//waiting to receive an ack.
		if(rec_msg(curr_pid,&message,ACK) < 0)
		{
			endSim();
			perror(NULL);
		}
	}
}

void MmuPorc()
{
	while(1)
	{
		pthread_create(&Mmu_ID, NULL, Mmu_Th, NULL);
		//pthread_create(&printer_ID,NULL,(void*) &printer,NULL);
		pthread_join(Mmu_ID,NULL);
	}
}

void* printer()
{
	struct timespec del;
	int i, temp_table[N];
	//del.tv_sec = 0;
	//del.tv_nsec = TIME_BETWEEN_SNAPSHOTS;
	
	//while(1)
	//{
		pthread_mutex_lock(&lock);
		for (i = 0; i < N; i++)
			temp_table[i] = physical_memory.Memory[i];
		pthread_mutex_unlock(&lock);
		//print:
		for (i = 0; i < N; i++)
		{
			if(temp_table[i] == -1)
				printf("%d|-\n", i);
			else
				printf("%d|%d\n", i, temp_table[i]);
		}	
		printf("\n");
		//nanosleep(&del, NULL); //sleep
		//sleep(TIME_BETWEEN_SNAPSHOTS);
	//}
	pthread_exit(0);
}

void *Mmu_Th()
{
	msg mR, mS;
	int phyAdd,  sendingProcess,full_flag = 1,j,temp;
	struct timespec delay;

	//rec message
	if(rec_msg(MMU,&mR,READ_WRITE_TYPE) < 0)
	{
		endSim();
		perror(NULL);
	}
	sendingProcess = mR.processId;

	//MISS
	phyAdd = HitOrMiss(&mR);
	if ((phyAdd) <0)
	{
		pthread_mutex_lock(&lock);
		for(j=0;j<N;j++)
		{
			if(physical_memory.Memory[j] == -1)//check if mem full or not.
			{
				full_flag = 0;
				break;
			}
		}
		//Mem isn't full.
		if (full_flag == 0)
		{
			pthread_mutex_unlock(&lock);

			//sends message to HD.
			mS.msg_type = HD_MSG;
			mS.processId = MMU;
			if(transmit_msg(HardDisk,&mS) < 0)
			{
				endSim();
				perror(NULL);
			}
			//rec message from HD.
			if(rec_msg(MMU,&mR,ACK) < 0)
			{
				endSim();
				perror(NULL);
			}

			if (mR.msg_type == ACK)
			{
				//update memory
				pthread_mutex_lock(&lock);

				if( physical_memory.Memory[j] == -1)
				{
					physical_memory.Memory[j] = 0;//writes no matter if was read/write.
				}
				pthread_mutex_unlock(&lock);
			}
		}
		else
		{
			pthread_mutex_unlock(&lock);
			//call evicter.
			pthread_create(&evicter_ID, NULL, &evicter, NULL);
			pthread_join(evicter_ID, NULL);

			//send message to HD.
			mS.msg_type = HD_MSG;
			mS.processId = MMU;
			if(transmit_msg(HardDisk,&mS) < 0)
			{
				endSim();
				perror(NULL);
			}

			//rec message from MMU
			if(rec_msg(MMU,&mR,ACK) < 0)
			{
				endSim();
				perror(NULL);
			}
			if (mR.msg_type == ACK)
			{
				//update Memory.
				pthread_mutex_lock(&lock);
				for(j=0;j<N;j++)
				{
					if(physical_memory.Memory[j] == -1)//search empty page.
					{
						physical_memory.Memory[j] = 0;
						break;
					}
				}
				pthread_mutex_unlock(&lock);
			}
		}
		pthread_create(&printer_ID,NULL,(void*) &printer,NULL);
		pthread_join(printer_ID, NULL);
	}
	/* HIT WRITE*/
	else
	{
		if(mR.action == 1)//1 = WRITE
		{
			//set sleep timer.
			delay.tv_nsec = MEM_WR_T;
			delay.tv_sec = 0;
			nanosleep(&delay, NULL);

			pthread_mutex_lock(&lock);
			//activate dirty randomly.
			temp = (rand()%N);
			while(physical_memory.Memory[temp] == -1)
			{
				temp = (rand()%N);
			}
			physical_memory.Memory[temp] = 1;
			pthread_mutex_unlock(&lock);
		}
	}
	//sending ack.
	mS.processId = MMU;
	mS.msg_type = ACK;
	if(transmit_msg(sendingProcess,&mS) < 0)
	{
		endSim();
		perror(NULL);
	}
	pthread_exit(0);
}

int HitOrMiss(msg *message)
{
	int i,empty_flag=0;
	if(message->processId == P1 || message->processId == P2 )//full memory.
	{
		pthread_mutex_lock(&lock);
		for(i = 0; i<N;i++)
		{

			if(physical_memory.Memory[i] != -1)
				empty_flag = 1;
		}
		pthread_mutex_unlock(&lock);
		if(!empty_flag)
		{
			return -1;
		}
	}
	if(!((rand()%10) < (HIT_RATE*10)))//if mem isnt empty, choose Hit or Write
	{
		return -1;
	}
	else //HIT
	{
		return 1;
	}
	printf("Error in HitOrMiss function");
	return -1;
}


void *evicter()
{
	int index,i,dirty = 0;
	msg message;
	for(i=0;i<=(N-(USED_SLOTS_TH));i++)
	{
		pthread_mutex_lock(&lock);
		index = physical_memory.oldest;//oldest page in memory.
		if (physical_memory.Memory[index] == 1)
		{
			dirty = 1;
		}
		physical_memory.Memory[index] = -1;//erase mem in the oldest page.
		if (index == N-1)//do modN
		{
			physical_memory.oldest = 0;
		}
		else
			physical_memory.oldest++;
		pthread_mutex_unlock(&lock);

		//if was dirty , has to send message to HD to update it.
		if (dirty)
		{
			message.processId = MMU;
			message.msg_type = HD_MSG;
			if(transmit_msg(HardDisk,&message) < 0)
			{
				endSim();
				perror(NULL);
			}

			//receive message from MMU.
			if(rec_msg(MMU,&message,ACK) < 0)
			{
				endSim();
				perror(NULL);
			}
		}
	}
	pthread_exit(0);
}

void hd()
{
	msg message;
	struct timespec delay;
	while(1)
	{
		if(rec_msg(HardDisk,&message,HD_MSG) < 0)//get message.
		{
			endSim();
			perror(NULL);
		}

		//set timer.
		delay.tv_nsec = HD_ACCS_T;
		delay.tv_sec = 0;
		nanosleep(&delay,NULL);

		message.msg_type = ACK;
		message.processId = HardDisk;

		if(transmit_msg(MMU,&message) < 0)//send message to MMU
		{
			endSim();
			perror(NULL);
		}
	}
}
