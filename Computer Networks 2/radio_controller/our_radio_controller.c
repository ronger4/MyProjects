/*
 ============================================================================
 Name        : our_radio_controller.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include "our_radio_controller.h"


int main(int argc, char** argv)
{
	mc_port_and_ip double_arg; //in order to send 2 arguments to thread
	int byte_counter = 0,serv_TCP_port, select_ver=0;
	char* serv_TCP_addr;
	//multicast :
	char mc_ip[15];
	uint16_t mc_port;
	uint8_t Reply_T;
	struct sockaddr_in sock_data;
	unsigned char hello_buff[3]={0,0,0};
	unsigned char welcome_buff[9];


	if(argc != 3)
	{
		printf("number of arguments received is wrong\n");
		exit(0);
	}

	printf("waiting to connect server\n");
	serv_TCP_addr = argv[1]; //getting servers ip address
	serv_TCP_port = atoi(argv[2]); //getting servers port
	myTCPsocket = socket(AF_INET, SOCK_STREAM, 0); //opening a TCP socket
	if (myTCPsocket == -1)
	{
		perror(NULL);
		close(myTCPsocket);
		exit(0);
	}

	//setting socket struct:
	sock_data.sin_family = AF_INET; //setting as ipv4
	sock_data.sin_port = htons(serv_TCP_port); 	//server's port
	sock_data.sin_addr.s_addr = inet_addr(serv_TCP_addr); //server's ip addr
	memset(sock_data.sin_zero,'\0', sizeof(sock_data.sin_zero)); // setting 0 to unused field

	//connecting socket to server:
	if(connect(myTCPsocket, (struct sockaddr*)&sock_data,sizeof(sock_data))==-1)
	{
		perror("Problem connecting the server(TCP)");
		close(myTCPsocket);
		exit(0);
	}


	if(send(myTCPsocket,hello_buff,sizeof(hello_buff),0) == -1)
	{
		perror(NULL);
		printf("\n error in hello message \n");
		exit(0);
	}
	//reseting file descriptors and timers before using them in select :
	FD_ZERO(&fdr);
	FD_SET(myTCPsocket, &fdr);
	timev.tv_sec = 0;
	timev.tv_usec = 300000;//0.3 sec

	select_ver = select(myTCPsocket+1, &fdr, NULL, NULL, &timev); // wait for input
	 if(select_ver == 0) //timeout
	{
		printf("\n took more then 0.3 to get welcome message :(\n");
		curr_stat = OFF;
		close (myTCPsocket);
		exit(0);
	}
	else if(FD_ISSET(myTCPsocket, &fdr) && select_ver) //not in upload , fdisset checks if mytcpsocket was set
	{
		curr_stat = WAIT_WELCOME;
	}
	else if(select_ver == -1)
	{
		perror("problem in select during handshake\n");
		curr_stat = OFF;
		close(myTCPsocket);
		exit(0);
	}
	byte_counter = recv(myTCPsocket, welcome_buff, sizeof(welcome_buff) , 0); //receive welcome message
	if (byte_counter == -1)
	{
		perror(NULL);
		printf("prob receiving welcome message\n");
		curr_stat = OFF;
		close(myTCPsocket);
		exit(0);
	}

	Reply_T = (uint8_t)welcome_buff[0];
	if(Reply_T == 0)
	{
		printf("\n welcome message was received \n");
		station_number = ntohs(((uint16_t*)(welcome_buff+1))[0]); //num of stations.
		sprintf(mc_ip,"%u.%u.%u.%u",welcome_buff[6],welcome_buff[5],welcome_buff[4],welcome_buff[3]);	// getting mc address and implementing ntohs
		mc_port = ntohs(((uint16_t*)(welcome_buff+7))[0]); //get port num
		inet_aton(mc_ip, &first_mc_addr); //put ip string in first_mc_addr
		curr_mc_channel = first_mc_addr;
		curr_stat = ESTABLISHED;
	}
	else
	{
		printf("message is not welcome message.\n");
		curr_stat = OFF;
		close(myTCPsocket);
		exit(0);
	}

	printf("\nconnectd to the server, info is :%d stations, multicast is: %s, multicast port is: %d.\n"
			,station_number,mc_ip,mc_port);


	//prepare struct to enter as argument to the thread;
	strcpy(double_arg.multic_ip, mc_ip);
	double_arg.multic_port = mc_port;
	if(pthread_create(&tid, NULL, play_song, &double_arg)!=0) //play_song thread
	{
		printf("\n error creating the play song thread");
		close(myTCPsocket);
		exit(0);
	}
	TCP_And_Interface();


	close(myTCPsocket); //close socket

	pthread_join(tid, NULL);
	return 1;
}


