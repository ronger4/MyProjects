/*
 * defines.h
 *
 *  Created on: Jan 6, 2019
 *      Author: ubu
 */



#define Buffer_size 	1024
#define 	TWO_KILO_BYTE 		2000 //2kb
#define 	TEN_MEGA_BYTE 		10485760 //10Mbs (10*1024*1024)
//states:(messages server can get)
#define 	HELLO 			0
#define 	ASKSONG			1
#define 	UPSONG  		2
//sending
#define		WELCOME		3
#define		ANNOUNCE	4
#define		PERMIT		5
#define		NEW_STATION	6
#define		INVALID_MSG	7


