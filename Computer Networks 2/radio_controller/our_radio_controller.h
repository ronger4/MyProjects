/*
 * our_radio_controller.h
 *
 *  Created on: Dec 30, 2018
 *      Author: ubu
 */


#include "defines.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>

//global variables
int curr_stat=0, other_station_flag=0;
int listening_station=0,last_data=0 ;
int myTCPsocket = 0 ,send_succ_flag = 0  , counter=0;
/*unsigned int*/int station_number =0;
int upload_flag = 0; //1 - in upload \ 0 - not in upload
char *song_name; //song file name to upload
char stdin_user[50] = {0};
/*long */ int up_iter = 0; //packets number
struct timeval timev;
struct in_addr first_mc_addr;
struct in_addr curr_mc_channel;
pthread_t tid; //thread id
fd_set fdr;



void TCP_And_Interface();
void* play_song(void* UDP);
int upsong();
int TCP_messages();
int input_handler();


typedef struct {
	char multic_ip [15];
	uint16_t multic_port;
} mc_port_and_ip;


void* play_song(void* PortnIP)
{
	int myUDPsocket,udp_len, reuse_ver = 1;
	mc_port_and_ip double_arg = *(mc_port_and_ip*)PortnIP; //get the argument struct
	struct ip_mreq mc_req;
	struct sockaddr_in mc_port_and_ip = {0};
	char  buff[buffer_size];
	socklen_t mc_struct_size;
	FILE * fp;


	myUDPsocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (myUDPsocket == -1)
	{
		printf("problem opening udp sock");
		close(myUDPsocket);
		pthread_exit(&tid);
		exit(0);
	}


	//enter to socket data struct.
	mc_port_and_ip.sin_family = AF_INET;
	mc_port_and_ip.sin_addr.s_addr = htonl(INADDR_ANY);
	mc_port_and_ip.sin_port = htons(double_arg.multic_port);
	mc_struct_size = sizeof(mc_port_and_ip);

	setsockopt(myUDPsocket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_ver, sizeof(reuse_ver)); //reuse udp sock.
	//bind
	if(bind(myUDPsocket, (struct sockaddr *) &mc_port_and_ip, sizeof(mc_port_and_ip))==-1)
	{
		perror("problem in binding udp socket");
		close(myUDPsocket);
		pthread_exit(&tid);
		exit(0);
	}

	mc_req.imr_multiaddr.s_addr = inet_addr(inet_ntoa(first_mc_addr));
	mc_req.imr_interface.s_addr = htonl(INADDR_ANY);//any host
	setsockopt(myUDPsocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mc_req, sizeof(mc_req)); //joining mc group
	fp = popen("play -t mp3 -> /dev/null 2>&1", "w");//opening pipe
	//waiting for data in socket:
	while(curr_stat != OFF )
	{
		//change station:
		if(other_station_flag)
		{
			setsockopt(myUDPsocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mc_req, sizeof(mc_req));
			curr_mc_channel.s_addr = (uint32_t)ntohl((htonl((first_mc_addr.s_addr))) + listening_station);
			mc_req.imr_multiaddr.s_addr = inet_addr(inet_ntoa(curr_mc_channel));
			setsockopt(myUDPsocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mc_req, sizeof(mc_req));
			other_station_flag = 0;
		}
		FD_ZERO(&fdr); //reset file descriptor reader
		FD_SET(0, &fdr);
		FD_SET(myUDPsocket, &fdr);
		//TO:
		timev.tv_sec = 2;
		timev.tv_usec = 0;
		if(select(myUDPsocket+1, &fdr, NULL, NULL, &timev) <0)
		{
			perror("udp select failed\n");
			curr_stat = OFF;
			break;
		}
		udp_len = recvfrom(myUDPsocket,buff,sizeof(buff),0,(struct sockaddr *) &mc_port_and_ip,&mc_struct_size); //receive song
		if(udp_len > 0)
		{
			fwrite (buff , sizeof(char), buffer_size, fp); //playing the song
		}
		if (udp_len < 0)
		{
			perror("problem reading udp socket\n");
			curr_stat = OFF;
		}
		if(udp_len == 0)// server closed connection
		{
			printf("\nserver closed UDP connection\n");
			curr_stat = OFF;
		}
	}
	setsockopt(myUDPsocket,IPPROTO_IP,IP_DROP_MEMBERSHIP,&mc_req,sizeof(mc_req)); //leave mc group
	printf("Bye, hope you enjoyed the music!\n");
	//free resources:
	close(myUDPsocket);
	pclose(fp);
	pthread_exit(&tid);
}

void TCP_And_Interface()
{
	int select_return;

	printf("\nEnter station number you would like to listen to or 's' to upload a song or 'q' to exit program\n"); //menu.
	while(curr_stat != OFF)
	{
		FD_ZERO(&fdr); //reset fdr
		FD_SET(0, &fdr);
		FD_SET(myTCPsocket, &fdr);
		//timeout:
		timev.tv_sec = 0;
		timev.tv_usec = 100000; //100 ms

		select_return = select(myTCPsocket+1, &fdr, NULL, NULL, &timev); //wait for input from tcp sock
		if (select_return == -1) //error in select
		{
			perror("error in tcp select");
		}
		else if(select_return) //some file descriptor has data
		{
			if(upload_flag == 0  && FD_ISSET(0, &fdr) ) //there is data to read from STDIN and we arent in upload state
			{
				fflush(stdin); //clean input buff
				gets(stdin_user); //get user input.
				input_handler();
				memset(stdin_user,'0', sizeof(stdin_user)); //init user input with 0.

			}
			if(FD_ISSET(myTCPsocket, &fdr))
			{
				if(send_succ_flag == 1)
					send_succ_flag = 0; //got response before timeout.
				TCP_messages(); //was myTCPsocket in ()
				if (curr_stat != OFF) //still online
				{
					printf("\nEnter station number you would like to listen to or 's' to upload a song or 'q' to exit program\n");
				}
			}

		}
		if(select_return==0 && send_succ_flag == 1) //sent message to server and he didnt respond
		{
			printf ("server didnt respond in time.\n");
			curr_stat = OFF;
		}
	}
	if (curr_stat == OFF)
		exit(0);
}

int input_handler()
{
	int i,sname_size,song_bytes,real_size;
	char* up_song_buff;
	char ask_song_buff[3] = {1,0,0}; //1 for command number and 00 for station number
	FILE * sp; //song pointer
	int stdin_int;
	if(stdin_user[1] == 0) //fix bugs like entering "qbasshsg" string from user and program would go off because of the starting 'q' in the string
		stdin_int = (int)stdin_user[0] ;
	else
	{
		stdin_user[0] = 128; //128 is just a random wrong value.
		stdin_int = (int)stdin_user[0];
	}
	switch(stdin_int)
	{
	case 115: //'s' =115 in ascii
		printf("Enter song name to upload\n");
		fflush(stdin);
		gets(stdin_user);

		sp = fopen(stdin_user,"r");
		if (sp != 0)  //check entered file
		{
			//preparing to upload the song:
			sname_size = strlen(stdin_user);
			//song size:
			fseek(sp, 0, SEEK_END);//get pointer to the end of the file
			song_bytes = ftell(sp); //tells the value of the pointer position aka the size since the pointer is in the end of the file.
			rewind(sp); //returns pointer to the beginning of the file.
			if(song_bytes >= TWO_KILO_BYTE && song_bytes <= TEN_MEGA_BYTE) //check that the size is good
			{
				real_size = sname_size+6; //6 bytes = command_type(1 byte) + song_size(4 byte) + song_name_size(1 byte)
				printf("uploading file with size:%d bytes\n",song_bytes);
				up_song_buff = (char*)malloc(real_size);

				//filling buff to send:
				up_song_buff[0] = 2;
				up_song_buff[1] = song_bytes>>8*3;
				up_song_buff[2] = song_bytes>>8*2;
				up_song_buff[3]	= song_bytes>>8;
				up_song_buff[4] = song_bytes;
				up_song_buff[5] = (uint8_t)sname_size;
				for (i=0; i<sname_size; i++) //song name
				{
					up_song_buff[i+6] = stdin_user[i];
				}
				//sending upsong message:
				if(send(myTCPsocket,up_song_buff,real_size,0) == -1)
				{
					perror("\nproblem sending upsong");

				}
				else
				{
					send_succ_flag = 1;
				}
				free(up_song_buff);
				curr_stat = WAIT_PERMIT;

				//saving song name to be global so we could use it in other functions.
				song_name = (char*)malloc(sname_size);
				for (i=0; i<sname_size; i++)
				{
					song_name[i] = stdin_user[i];
				}

				up_iter = song_bytes/buffer_size +1; //number of packets that we need in order to send full song.
				last_data = song_bytes-(up_iter-1)*buffer_size; // sent only if not 0
				fclose(sp);
				return 1;
			}
			else //problem in file size
			{
				printf("song size is wrong, please upload song with the right size");
				fclose(sp);

			}
		}
		else
		{
			perror("problem opening song,might not exist.");
			printf("\nEnter station number you would like to listen to or 's' to upload a song or 'q' to exit program\n");
			return 1;
		}
		break;
	case 113://'q' = 113 in ascii
		curr_stat = OFF;
		printf("Bye, hope you enjoyed the music!\n");
		return 1;
		break;
	case  0 ... MAX_STATIONS-1:
		if ((uint16_t)atoi(stdin_user) < station_number)//entered a good number
		{
			*(uint16_t*)&ask_song_buff[1] = htons(atoi(stdin_user)); //insert requested station number to upSong msg
			listening_station = (int)atoi(stdin_user);
			if(send(myTCPsocket,ask_song_buff,sizeof(ask_song_buff),0) == -1)
			{
				perror("");
				printf("\nError in sending askSong message");
			}
			else
				send_succ_flag = 1; //flag for timeout
			curr_stat = WAIT_ANNOUNCE;
		}
		else
		{
			printf("The station you entered doesn't exist, please try again.\n");
		}

		break;
	default:
		if(atoi(stdin_user) == 0 && strcmp(stdin_user, "0") != 0) // input is wrong
		{
					printf("wrong input,please try again\n");
		}
	}
	return 1;

}

int TCP_messages()
{
	int arr_len=0,i,Reply = 0,permit_flag=0, packets_send=0;
	char *announce_info_arr; //for  announce
	char *Invalid_command_arr; //for invalid command
	char buff[buffer_size];

	packets_send = recv(myTCPsocket, buff, buffer_size , 0); // reading socket data
	if(packets_send > 0) //there is a message
	{
		Reply = (uint8_t)buff[0];
		switch(Reply) //depends on what ReplyType the server sent.
		{
		case 0: //invalid Welcome message
			printf("welcome message was received in the wrong stage.\n");
			curr_stat = OFF;
			return 0;

		case 1: //Announce
			if(curr_stat == WAIT_ANNOUNCE && packets_send >=2)
			{
				printf("Announce message received");
				arr_len = (uint8_t)buff[1]; //song name size
				if (arr_len > 0)
				{
					announce_info_arr = (char*)malloc(arr_len);
					for(i=0; i<arr_len; i++) //copying song name
					{
						if(buff[i+2] != '\0') //copying without the "/0"
						{
							announce_info_arr[i] = buff[i+2];
						}
					}

					printf(" song currently playing is: %s\n",announce_info_arr); //new station
					other_station_flag = 1; //change station in play_song thread
					curr_stat = ESTABLISHED;
					free(announce_info_arr);
					return 1;

				}
				else
				{
					printf("Error: Announce message had song size 0.\n");
					curr_stat = OFF;
					return 0;
				}
			}
			else
			{
				printf ("We didn't want to get Announce message\n");
				curr_stat = OFF;
				return 0;
			}
		case 2://permit msg
			if(curr_stat == WAIT_PERMIT && packets_send == 2)
			{
				printf("got permit message ");
				permit_flag = buff[1];
				if(permit_flag == 1)
				{
					curr_stat = UPLOAD;//switched to upload state
					upload_flag = 1;
					if(upsong() == 1) //check if uploaded.
					{
						upload_flag = 0;
						fflush(stdin); //flush input
						curr_stat = ESTABLISHED; //back to Established mode

					}
					else
					{
						printf("error in uploading a song\n");
						fflush(stdin); //flush input
						curr_stat = OFF; //exit program
					}
				}
				else if(permit_flag == 0)
				{
					printf("song isn't permitted to be uploaded(maybe already played in one of the stations)\n");
					return 1;
				}
			}
			else
			{
				printf ("We didn't want to receive PermitSong message\n");
				curr_stat = OFF;
				return 0;
			}
			break;

		case 3: //Invalid_command
			if(packets_send >=2)
			{
				arr_len = (uint8_t)buff[1];
				Invalid_command_arr = (char*)malloc(arr_len);
				for(i=0; i<arr_len; i++)
				{
					Invalid_command_arr[i] = buff[i+2];
				}
				printf("invalid message : %s",Invalid_command_arr);
				curr_stat = OFF;
				free(Invalid_command_arr);
				return 0;
			}
			else
			{
				printf("problem with invalid message .\n");
				curr_stat = OFF;
				return 0;
			}

		case 4: //New stations
			if(packets_send == 3)
			{
				station_number = ntohs(((uint16_t*)(buff+1))[0]);
				printf("there is new station, we have %d stations now \n",station_number);
				curr_stat = ESTABLISHED;//return to Established state.
			}
			else
			{
				printf("problem with new station message :(\n");
				curr_stat = OFF;
				return 0;
			}
			break;

		default :
			printf("wrong message from server.\n");
			curr_stat = OFF;
			return 0;
		}
	}
	else if (packets_send == -1)//failure
	{
		perror("something went wrong when reading from TCP message.\n");
	}
	else if (packets_send == 0)
	{
		printf("\nServer closed TCP connection");
		curr_stat = OFF;
		return 0;
	}
	return 1;
}

int upsong()
{
	int select_return=0,packets_send = 0,recv_data;
	float percent = 0;
	char sending_buff[buffer_size];
	char buff[buffer_size];
	uint8_t Reply = 0;
	FILE * song_fp;
	useconds_t usec_delay = 11000;


	song_fp = fopen(song_name,"r"); //open file in read mode.
	if(song_fp == 0)  //problem opening
	{
		perror("song file won't open.\n");
		free(song_name);
		curr_stat = OFF;
		return 0;
	}

	while(fscanf(song_fp,"%1024c", sending_buff) != EOF  && packets_send < up_iter && curr_stat != OFF)
	{
		printf("Song  status: %.0f%% \r",percent); //percentage display
		fflush(stdout);


		if(packets_send != up_iter - 1 || last_data == 0) //uploading other parts
		{
			recv_data = send(myTCPsocket,sending_buff,buffer_size,0);
			counter +=recv_data;
			if(recv_data == -1)
			{
				perror("problem occured in uploading song.");
				curr_stat = OFF;
				break;
			}
			else if (recv_data == 0)
			{
				printf("\n\nthe server has closed connection");
				curr_stat = OFF;
				free(song_name);
				fclose(song_fp);
				break;
			}
			percent = (packets_send++)*100/up_iter; //percentage count
			usleep(8000);//usec_delay); //delay in sending todo
		}
		else //last part
		{
			recv_data = send(myTCPsocket,sending_buff,last_data,0);
			counter +=recv_data;

			if(recv_data == 0)
			{
				printf("\nserver has ended the connection!!\n");
				curr_stat = OFF;
				free(song_name);
				fclose(song_fp);
				break;
			}
			else if(recv_data == -1)
			{
				printf("couldn't upload last packet\n");
				curr_stat = OFF;
			}
			else
			{
				printf("\n\nsong uploaded! name: %s!\n" , song_name);
			}
		}
	}
	fclose(song_fp);
	if((fscanf(song_fp, "%1024c", sending_buff))==EOF)  //waiting for new station message
	{
		curr_stat = ESTABLISHED;//WAIT_NEW_STATIONS;
		FD_ZERO(&fdr);
		FD_SET(myTCPsocket, &fdr);
		//timeout:
		timev.tv_sec = 2;
		timev.tv_usec = 0;

		select_return = select(myTCPsocket+1, &fdr, NULL, NULL, &timev); // block for 2 sec or till get newstation message
		if (select_return == -1)
		{
			perror("problem in select() in uploading song");
			curr_stat = OFF;
		}
		else if(select_return != 0 && (FD_ISSET(myTCPsocket, &fdr)))
		{
			recv_data = recv(myTCPsocket, buff, buffer_size , 0); // read newstation data
			if(recv_data > 0)
			{
				Reply = (uint8_t)buff[0];
				if(Reply == 4 && recv_data == 3)  //we got new station message
				{
					station_number = ntohs(((uint16_t*)(buff+1))[0]);
					printf(" there are  %d stations now ",station_number);
					free(song_name);
					return 1;
				}
				else
				{
					printf("wrong newstation message was received\n");
					curr_stat = OFF;
				}
			}
			else
				perror("problem recieving new station message (recv data is less then 0)");

		}
		else if(select_return == 0)
		{
			printf("timeout reached to 0 while waiting for new station message :(\n");
			curr_stat = OFF;
			return 0;
		}
	}

	printf("\nProblem in uploading the song, we didn't got to the end of the file");
	free(song_name);
	return 0;
}




