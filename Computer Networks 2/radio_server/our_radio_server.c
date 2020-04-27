/*
 ============================================================================
 Name        : our_radio_server.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "our_radio_server.h"


int main(int argc, char** argv)
{
	int wel_sock,portT,i,select_ver,menu_print_flag=0;
	struct sockaddr_in client_data_sock;
	char user_input,*song_name;
	//struct in_addr bin_addr;
	socklen_t sock_info_size;
	struct udp_info *station_p;
	struct udp_info *temp;
	fd_set fdr;

	if(argc < 5)
	{
		printf("wrong number of arguments given, please try again");
		exit(0);
	}
	init_clients();//init array of structs




	//get input
	portT = atoi(argv[1]); //for the tcp port
	first_mc_addr = argv[2]; ///beginning of mc addresses
	portU = atoi(argv[3]); //for the udp port




	//first node:
	station_p=(struct udp_info*)malloc(sizeof(struct udp_info)); //allocate the station udp for each udp multicast
	list_head=station_p; //head of the list
	//opening thread for every station to play the song
	for(stations_tot=0;stations_tot<(argc-4);stations_tot++) //argc -4 = number of stations in the beginning
	{
		if(stations_tot!=0)//first node was already allocated
		{
			temp=(struct udp_info*)malloc(sizeof(struct udp_info)); //allocate a new station
			station_p->next= temp;
			station_p=station_p->next;
		}
		song_name = (char*) argv[stations_tot+4];
		open_udp_socket(station_p,song_name);//opening udp socket for this node
		if( pthread_create( &station_p->station_id , NULL ,  &station_thread , (void*)station_p) < 0) //handling the station
		{
			perror("problem creating thread");
			exit(0);
		}
	}

	//welcome socket:
	wel_sock = create_tcp(portT);//creating TCP socket



	//listen to socket: (1 socket can be queued at a given time)
	if(listen(wel_sock,1)==0)
		printf("\n\nwaiting for new clients requests\n");
	else
	{
		perror("problem listening to welcome socket");
		close(wel_sock);
		exit(0);
	}

	//new socket created for a new connection.
	sock_info_size = sizeof(client_data_sock);
	while(exit_flag==0)//while we don't need to exit
	{
		if(menu_print_flag==0)//check if already printed or not.
		{
			menu_print_flag=1;
			printf("\nto print server data please enter 'p' , press 'q' to exit\n");
		}
		FD_ZERO(&fdr);
		FD_SET(wel_sock, &fdr);
		FD_SET(STDIN_FILENO, &fdr); //STDIN_FILENO = 0 (STDIN FILE DESCRIPTOR)
		select_ver = select(wel_sock+1, &fdr, NULL, NULL, NULL); //block till there is input in the stdin or data on the socket
		if(FD_ISSET(wel_sock, &fdr))//there is new tcp socket waiting to connect
		{
			for(i=0;i<100;i++) //find first empty spot
				if(all_clients[i].connected_flag==0)
					break;
			if(i==100)
			{
				printf("server is full.");
			}
			else
			{
				all_clients[i].tcp_sock = accept(wel_sock, (struct sockaddr *)&client_data_sock, &sock_info_size);
				if (select_ver == -1) //error in select
				{
					perror("error in select(while entering new client)");
					continue;
				}

				if (all_clients[i].tcp_sock == -1)
				{
					perror("probelm in accepting new client");
					close(all_clients[i].tcp_sock);
					break;
				}
				else
				{
					printf("\nThere is new client :) \n");
				}

				create_client(i,client_data_sock);//creating new client
				if( pthread_create(&all_clients[i].thread_id,NULL,&client_thread,(void*)i) < 0)
				{
					perror("could not create thread");
					close(all_clients[i].tcp_sock);
				}
			}
		}

		else if(FD_ISSET(STDIN_FILENO,&fdr))//interupt from user.
		{
			scanf("%c",&user_input);
			menu_print_flag=0;
			switch (user_input)
			{
			case 'p':
				prints();
				break;
			case 'q':
				exit_flag=1;
				break;
			default:
				printf("wrong input!!  press 'p' or 'q' only.");
				break;
			}
		}

	}
	free_all(list_head);



	return 0;
}
