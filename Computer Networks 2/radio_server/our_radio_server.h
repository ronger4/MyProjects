/*
 * our_radio_server.h
 *
 *  Created on: Jan 6, 2019
 *      Author: Ron and Matan
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
struct udp_info
{
	int udp_sock;
	int station_num;//the number of the playing station
	char *song_name;//name
	pthread_t station_id;//id of the thread
	struct in_addr ip;
	struct sockaddr_in addr;
	int port;
	struct udp_info *next;//next in the list
};

struct tcp_info
{
	int tcp_sock;
	char tcp_ip[15];
	int serial_number;
	pthread_t thread_id;
	int connected_flag;
	int station;

};

//global vars:
struct tcp_info all_clients[100];
int stations_tot=0;
int upload_status=0; //0- no song is uploading, 1- a song is uploading, 2 - a song has uploaded
int portU=0;
int exit_flag=0;
char *first_mc_addr;
struct udp_info *list_head;

int create_tcp(int port);
void create_client(int client_num,struct sockaddr_in server);
int free_all(struct udp_info *root_station);
void* client_thread(void* client_number);
void open_udp_socket(struct udp_info *station_p, char *song_name);
void* station_thread(void* data);
void send_new_stations(void);
int create_station(char* name);
int DL_song(char* name,int  len,int sock);
int send_reply(int tcp_sock,int reply_num,int len,char* message);
int start_tcp(struct tcp_info my_data);
int send_invalid(int sock,int len,char* message);
int send_welcome(int sock);
int send_announce(int sock,char* message,int len);
int send_permit(int sock,int len);
int send_new_stations_msg(int sock,int len);
void prints(void);
void init_clients();

int create_tcp(int port)
{
	int wel_sock,reuse_sock = 1;
	struct sockaddr_in serv_sock_info ;
	wel_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (wel_sock == -1)
	{
		perror("problem creating welcome socket");
		close(wel_sock);
		exit(0);
	}

	//socket struct:
	serv_sock_info.sin_family = AF_INET; //ipv4
	serv_sock_info.sin_port = htons(port);
	serv_sock_info.sin_addr.s_addr = htonl(INADDR_ANY); //setting so every one can connect. INADDR_ANY = 0
	memset(serv_sock_info.sin_zero, '\0', sizeof serv_sock_info.sin_zero); 	//reset unused field.

	//binding ip address:
	setsockopt(wel_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_sock, sizeof(reuse_sock)); //reuse option
	if (bind(wel_sock, (struct sockaddr *) &serv_sock_info, sizeof(serv_sock_info)) == -1)
	{
		perror("\nproblem in bind");
		close(wel_sock);
		exit(0);
	}
	return wel_sock;
}
void create_client(int client_num,struct sockaddr_in client_sockaddr)
{
	struct in_addr client_ipv4_addr;
	struct sockaddr_in* client_data_pointer;

	client_data_pointer = (struct sockaddr_in*)&client_sockaddr;
	client_ipv4_addr = client_data_pointer->sin_addr;
	inet_ntop( AF_INET, &client_ipv4_addr.s_addr,all_clients[client_num].tcp_ip,16);//converts to textual address
	all_clients[client_num].serial_number=client_num;
}
void init_clients()
{
	int i;
	for(i=0;i<100;i++) //up to 100 clients can connect the server
		{
			all_clients[i].serial_number=0;
			all_clients[i].connected_flag=0;
			all_clients[i].station=0;
			memset(all_clients[i].tcp_ip, '\0', sizeof(all_clients[i].tcp_ip));
		}
}
//opens udp socket and plays the song
void* station_thread(void* data)
{
	struct udp_info *station_p=(struct udp_info*)data;
	FILE * song_file;
	int sent_counter;
	char send_buff[Buffer_size] = {0};

	//open file in order to send:
	song_file = fopen(station_p->song_name , "r");
	if(!song_file)
	{
		printf("error opening song file");
		close(station_p->udp_sock);
		pthread_exit(NULL);
		exit(0);
	}
	printf("$$$$$$$ new song: %s at station %d $$$$$$$\n",station_p->song_name,station_p->station_num);

	//sending the song:
	while (exit_flag==0)
	{
		usleep(62500);//delay
		if((fscanf(song_file, "%1024c", send_buff))==EOF)
		{
			rewind(song_file);//restart song
		}
		sent_counter =  sendto(station_p->udp_sock, send_buff, sizeof(send_buff), 0, (struct sockaddr *) &station_p->addr, sizeof(station_p->addr));
		if(sent_counter == -1)
		{
			perror(NULL);
			printf("\nproblem in sending file\n");
		}
	}
	//free resources
	fclose(song_file);
	close(station_p->udp_sock);
	free(station_p->song_name);
	pthread_exit((void *) station_p->station_id);
}

void open_udp_socket(struct udp_info *station_p, char *song_name)
{
	int ttl = 16;
	struct in_addr bin_addr;
	int reuse_ver = 1;


	inet_aton(first_mc_addr,&bin_addr);
	station_p->ip.s_addr=(uint32_t)ntohl(htonl(bin_addr.s_addr)+stations_tot);//adding mc address of the station
	station_p->station_num=stations_tot;//station number
	station_p->port=portU; //udp port
	station_p->song_name=song_name;//name of the song that the user wrote.

	//try to open the udp sock to send the data on
	station_p->udp_sock = socket(AF_INET, SOCK_DGRAM, 0);//opening udp ipv4 socket
	if (station_p->udp_sock == -1)
	{
		perror("\ncan't create udp socket");
		close(station_p->udp_sock);
		pthread_exit(NULL);
		exit(0);
	}
	station_p->addr.sin_family = AF_INET;
	station_p->addr.sin_addr.s_addr = inet_addr(first_mc_addr);
	station_p->addr.sin_addr.s_addr = ntohl(htonl(station_p->addr.sin_addr.s_addr)+stations_tot);
	station_p->addr.sin_port = htons(portU);


	//bind
	if(bind(station_p->udp_sock, (struct sockaddr *) &station_p->addr, sizeof(station_p->addr))== -1)
	{
		perror("\ncant  bind udp socket");
		pthread_exit(NULL);
		close(station_p->udp_sock);
		exit(0);
	}

	//setting options:
	setsockopt(station_p->udp_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_ver, sizeof(reuse_ver));//so we can reuse
	setsockopt(station_p->udp_sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)); //ttl to 16
}

/*
 * A function to close all the connections
 * the function receives a udp_info struct linked-list
 * the function will close all the UDP stations threads and Multicast
 * and then close all TCP sockets and client threads
 * the function will return 0 if successful, or -1 if failed
 */
int free_all(struct udp_info *root_station) //todo back to here
{
	struct udp_info *temp;
	int Check,index=0;
	struct timeval timev;
	timev.tv_sec = 0; //to reset the sockets
	timev.tv_usec = 0;

	/********Close all tcp sockets and threads*********/
	for(index=0;index<100;index++)
		if(all_clients[index].serial_number!=0)
		{
			setsockopt(all_clients[index].tcp_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timev,sizeof(struct timeval));
			printf("closing socket %d\n",all_clients[index].serial_number);
			Check = pthread_join((pthread_t) all_clients[index].thread_id, NULL);
			//wait for the threads to end
			if (Check != 0)
			{
				perror("failed to close thread");
				exit(0);
			}
			all_clients[index].connected_flag=0;
		}
	index =0;
	while(root_station->next!=NULL)
	{
		Check = pthread_join(root_station->station_id, NULL);//waits for the termination of the thread
		if (Check != 0)
		{
			perror("failed to close thread");
			exit(0);
		}
		temp=root_station->next;
		if(root_station!=NULL)
		{
			free(root_station);
			printf("Station %d free'ed\n",index);
			index++;
		}
		root_station=temp;
	}
	if(root_station!=NULL){
		free(root_station);
		printf("Station %d free'ed\n",index);
	}
	printf("\n------------All Stations and clients Free'ed---------\n");
	printf("                      Server closed                    \n");
	return 1;
}

//thread that handles clients messages
void* client_thread(void* client_number)
{

	int len,close_sock=0, init_connection_flag,Done_flag,already_play_flag=1,same=1,end=0,i;
	char *inv_msg,*name;
	char buffer[Buffer_size];
	struct udp_info *temp_pointer = list_head;
	uint8_t command,name_length;
	uint16_t rec_data;
	uint32_t song_len;
	int client_inx = (int)client_number;
	fd_set set;

	init_connection_flag=start_tcp(all_clients[client_inx]);
	if(init_connection_flag==-1)
		close_sock=1;
	else
	{
		all_clients[client_inx].connected_flag=1;
	}
	while(exit_flag==0 && close_sock==0)
	{
		FD_ZERO(&set);
		FD_SET(all_clients[client_inx].tcp_sock, &set);
		FD_SET(STDIN_FILENO, &set); //STDIN_FILENO = 0
		select(all_clients[client_inx].tcp_sock+1, &set, NULL, NULL, NULL); // block till there is data in socket or stdin.
		if(FD_ISSET(all_clients[client_inx].tcp_sock, &set))
		{
			len= recv(all_clients[client_inx].tcp_sock, buffer, Buffer_size , 0);
			if(len>0)
			{
				command = buffer[0];
				switch (command)
				{
				case HELLO:
					//send an invalid command to the client
					inv_msg = "You sent a wrong Hello message";
					send_reply(all_clients[client_inx].tcp_sock, INVALID_MSG,strlen(inv_msg),inv_msg);
					close_sock = 1;//there is a error with the message - close connection
					break;
				case ASKSONG:
					if(len==3)
					{
						rec_data=(uint16_t )((buffer[1]<<8)||buffer[2]);

						//check that an Hello message was received
						if(init_connection_flag==1)
						{
							//check that the client asked for a station that exist
							if((rec_data>=0 && rec_data<stations_tot))
							{
								//search for the requested station
								while(rec_data!=(temp_pointer->station_num))
								{
									if(temp_pointer->next!=NULL)
									{
										temp_pointer=temp_pointer->next;
									}
								}
								if(rec_data==temp_pointer->station_num)
								{
									name=temp_pointer->song_name;
								}
								temp_pointer=list_head;
								if(send_reply(all_clients[client_inx].tcp_sock,ANNOUNCE,strlen(name),name)==-1)
								{
									close_sock = 1;
									//free
								}
								else
									all_clients[client_inx].station = rec_data; //change station number
							}
							else
							{
								inv_msg = "requested station number doesn't exist";
								send_reply(all_clients[client_inx].tcp_sock, INVALID_MSG,strlen(inv_msg),inv_msg);
								close_sock = 1;//error
							}
						}
						else
						{
							inv_msg ="didn't get Hello message";
							send_reply(all_clients[client_inx].tcp_sock, INVALID_MSG,strlen(inv_msg),inv_msg);
							close_sock = 1;//there is a error with the message - close connection
						}
					}
					break;
				case UPSONG:
					song_len=ntohl(*(uint32_t*)&buffer[1]);//copies 4 bytes
					//check song len
					if((song_len < TWO_KILO_BYTE || song_len > TEN_MEGA_BYTE))
					{
						inv_msg = "wrong song size";
						send_reply(all_clients[client_inx].tcp_sock, INVALID_MSG,strlen(inv_msg),inv_msg);
						close_sock = 1;
					}
					else//size is valid
					{
						name_length = buffer[5];
						name = (char *)malloc(sizeof(char));
						for (i = 0; i < name_length; i++)
						{
							name[i] = buffer[6 + i];
						}
						//check whether song is already playing
						if(temp_pointer!=NULL)
						{
							while(same!=0 && end==0)
							{
								same=strcmp(temp_pointer->song_name,name);
								if(temp_pointer->next!=NULL)
									temp_pointer=temp_pointer->next;
								else
									end=1;
							}
							temp_pointer = list_head;
							end = 0;
						}
						if(same!=0)
						{
							already_play_flag = 0 ;
						}
						same = 1;

						//already_play_flag=check_new_song(name,name_length); //checks whether  this song is already played
						if(already_play_flag==0  && !upload_status)
						{
							already_play_flag = 1;
							send_reply(all_clients[client_inx].tcp_sock, PERMIT,1,NULL); //permit the client to upload the song
							//start downloading the song.
							Done_flag = DL_song(name, song_len, all_clients[client_inx].tcp_sock);
							if (Done_flag == -1)
							{
								inv_msg = "upload song has failed";
								send_reply(all_clients[client_inx].tcp_sock, INVALID_MSG,strlen(inv_msg), inv_msg);
								close_sock = 1;
							}
							else
								create_station(name);
						}
						else
						{
							send_reply(all_clients[client_inx].tcp_sock, PERMIT, 0, NULL);
						}
					}
					//free(name);


					break;
				default:
					inv_msg = "wrong command type";
					send_reply(all_clients[client_inx].tcp_sock, INVALID_MSG,strlen(inv_msg),inv_msg);
					close_sock = 1;
					break;
				}
			}
			else
			{
				close_sock=1;
			}
		}

	}
	//closing thread
	all_clients[client_inx].connected_flag=0;
	pthread_exit(&(all_clients[client_inx].thread_id));

}
//responsible for welcome connection(welcome socket)
int start_tcp(struct tcp_info tcp_data)
{
	int len=0, ret=0;
	char buff[Buffer_size+1];
	char *inv_msg;
	uint8_t command = 0;
	fd_set set;
	uint16_t rec_data;
	struct timeval timev;
	//100 milisec:
	timev.tv_sec = 0;
	timev.tv_usec = 100000;
	FD_ZERO(&set);
	FD_SET(tcp_data.tcp_sock, &set);
	select(tcp_data.tcp_sock+1, &set, NULL, NULL, &timev); // block till data in socket
	if(FD_ISSET(tcp_data.tcp_sock,&set))
	{
		len = (int) recv(tcp_data.tcp_sock, buff, Buffer_size , 0);
	}
	if(len==3)
	{
		command=buff[0];
		if(command==0)
		{
			rec_data=(uint16_t )((buff[1]<<8)||buff[2]);
			if(rec_data==0)
			{
				//send welcome
				send_reply(tcp_data.tcp_sock, WELCOME,0,NULL); //send welcome
				ret=1;
			}
			else
			{
				//send invalid
				inv_msg = "wrong command number";
				send_reply(tcp_data.tcp_sock, INVALID_MSG,strlen(inv_msg),inv_msg);
				ret = -1;
			}
		}
	}
	else
	{
		//send invalid
		inv_msg = "Not hello message";
		send_reply(tcp_data.tcp_sock, INVALID_MSG,strlen(inv_msg),inv_msg);
		ret = -1;
	}
	return ret;
}



void send_new_stations(void)
{
	int i;
	stations_tot++;
	for(i=0;i<100;i++) //publish the new station
		if(all_clients[i].connected_flag==1)
		{
			send_reply(all_clients[i].tcp_sock,NEW_STATION,stations_tot,NULL);
		}
	printf("sent new station message to all stations\n");
	upload_status=0;
}

//function used send messages to the clients
int send_reply(int tcp_sock,int reply_num,int len,char* message)
{
	int check;
	switch (reply_num)
	{
	case WELCOME:
		check = send_welcome(tcp_sock);
		break;
	case ANNOUNCE:
		check = send_announce(tcp_sock,message,len);
		break;
	case PERMIT:
		check = send_permit(tcp_sock,len);
		break;
	case NEW_STATION:
		check = send_new_stations_msg(tcp_sock,len);
		break;
	case INVALID_MSG:
		check = send_invalid(tcp_sock,len,message);
		break;
	}
	if(check == 0)
	{
		printf("problem in sending");
		return -1;
	}
	return 0;
}

int send_invalid(int sock,int len,char* message)
{
	char buffer[100]; //all invalid messages aren't bigger then size 100.
	int i,m_len;
	buffer[0]=3; //command number(reply number)
	buffer[1]=(uint8_t)len; //reply size
	for (i = 0; i < len+2; i++)//reply (len bytes)
	{
		buffer[2+i]=message[i];
	}
	m_len=2+len;
	if(send(sock,buffer,m_len,0) == -1)
	{
		printf("problem in sending invalid message\n");
		return 0;
	}
	return 1;
}

int send_welcome(int sock)
{
	int m_len;
	char buffer[9];
	buffer[0]=0; ////command type(reply)
	*(uint16_t*)&buffer[1]=htons(stations_tot);//stations
	*(uint32_t*)&buffer[3]=htonl(inet_addr(first_mc_addr));//mc address
	*(uint16_t*)&buffer[7]=htons(portU);//port
	m_len=9;
	if(send(sock,buffer,m_len,0) == -1)
	{
		printf("problem in sending welcome message to the client\n");
		return 0;
	}
	return 1;
}

int send_announce(int sock,char* message,int len)
{
	int i,m_len;
	char buffer[100];//there is no song name bigger then 100
	buffer[0]=1; //command type(reply)
	buffer[1]=(uint8_t)len; //song name size
	for (i = 0; i < len; i++)//song name(len bytes)
		buffer[2+i]=message[i];
	m_len=2+len;
	if(send(sock,buffer,m_len,0) == -1)
	{
		printf("problem in sending announce message\n");
		return 0;
	}
	return 1;
}

int send_permit(int sock,int len)
{
	int m_len;
	char buffer[2];
	buffer[0]= 2;//command number(reply)
	buffer[1]=(uint8_t)len;//in permit case len is for permit.
	m_len=2;
	if(send(sock,buffer,m_len,0) == -1)
	{
		printf("problem in sending permit message to the client\n");
		return 0;
	}
	return 1;
}

int send_new_stations_msg(int sock,int len)
{
	int m_len;
	char buffer[3];
	buffer[0]=4;//command number(reply)
	*(uint16_t*)&buffer[1] = htons(len);//len is number of stations
	m_len=3;
	if(send(sock,buffer,m_len,0) == -1)
	{
		printf("problem in sending new stations message\n");
		return 0;
	}
	return 1;
}
//function to handle song download
int DL_song(char* name,int len,int sock)//todo change stuff
{
	int recv_count=1;
	int ret=0,pack_count=0,byte_counter=0;
	FILE* s_ptr;
	char buffer[Buffer_size];
	fd_set set;
	//3 sec time out
	struct timeval timev;
	timev.tv_sec = 3;
	timev.tv_usec = 0;
	//wait 3 seconds for data in socket
	FD_ZERO(&set);
	FD_SET(sock, &set);
	FD_SET(STDIN_FILENO, &set); //STDIN_FILENO = 0
	select(sock+1, &set, NULL, NULL, &timev); // block till there is data in socket.
	s_ptr=fopen(name,"w"); //open file to write to
	if(s_ptr<0)
	{
		perror("cant open a file");
		ret = -1;
	}
	//check that there are no other download already
	if(upload_status==0)
	{
		upload_status=1;
	}

	else
	{
		perror("someone is already downloading a song.");
		ret= -1;
	}
	if(ret>= 0)
	{
		printf("downloading : %s\n",name);
	}
	//start downloading:
	while(recv_count>0 && ret>= 0)
	{
		if((recv_count=recv(sock,buffer, sizeof(buffer),0))>0)
		{
			byte_counter += recv_count;
			pack_count++;
			fwrite(buffer,sizeof(char),recv_count,s_ptr);//write song to file
		}
		if(recv_count!=Buffer_size && byte_counter == len)
		{
			recv_count=0;
		}
	}
	if(recv_count<0 && pack_count==0)
	{
		perror("timeout in DL_song function");
		upload_status=0;  //not uploading anymore
		ret= -1;
	}
	else if(byte_counter==len)//download was successfull
	{
		printf("Done downloading!\n");
	}
	else
	{
		perror("failed to download song\n");
		upload_status=0;
		ret= -1;
	}
	fclose(s_ptr);
	return ret;
}


//new station thread
int create_station(char* name)
{
	struct udp_info *temp_pointer=list_head;
	while(temp_pointer->next!=NULL)
	{
		temp_pointer=temp_pointer->next;
	}
	temp_pointer->next=(struct udp_info*)malloc(sizeof(struct udp_info));
	temp_pointer=temp_pointer->next;
	open_udp_socket(temp_pointer, name);
	if( pthread_create( &temp_pointer->station_id , NULL ,  &station_thread , (void*)temp_pointer) < 0)
	{
		perror("problem creating thread in function create_station");
		return -1;
	}
	send_new_stations();
	upload_status=0;
	return 0;
}


void prints(void)
{
	int client_flag=0,i,j;
	struct udp_info *temp_pointer=list_head;
		printf(	"\nStations:\n");
		for(i=0;i<stations_tot;i++)
		{
			printf("\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
			printf("\n station  %d has mc %s , song: %s\n",i,inet_ntoa(temp_pointer->ip),temp_pointer->song_name);
			printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n\n\n");
			if(temp_pointer->next!=NULL)
				temp_pointer=temp_pointer->next;
		}
		printf("Clients:\n");
		for(i=0;i<100;i++)
		{
			if (all_clients[i].connected_flag == 1)
				printf("Client %d : %s\n",all_clients[i].serial_number,all_clients[i].tcp_ip);
		}
		printf("\n********************************************\n");
		printf("********************************************\n");
		printf("\n\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
		printf("station list:\n");
		for (i = 0; i < stations_tot; i++)
		{
			printf("\n station  %d has mc %s , song: %s\n",i,inet_ntoa(temp_pointer->ip),temp_pointer->song_name);
			printf("Clients on this stations:\n");
			client_flag=0;
			for (j = 0; j < 100; j++)
			{
				if (all_clients[j].connected_flag == 1 && all_clients[j].station == i)
				{
					client_flag = 1;
					printf("Client %d : %s\n", all_clients[j].serial_number, all_clients[j].tcp_ip);
				}
			}
			if (client_flag == 0)
				printf("there are no clients that are listening this station \n");
			printf("\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
			if(temp_pointer->next!=NULL)
				temp_pointer=temp_pointer->next;
		}

}



