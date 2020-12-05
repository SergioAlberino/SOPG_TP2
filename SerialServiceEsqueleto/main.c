//*****************************
//****** TP2 SOPG *************
//*****************************


//******* Definición de constantes ********
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SerialManager.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

//******* Definición de constantes ********
#define SERIAL_PORT	2 // depende del puerto en la placa
#define SERIAL_BAUDRATE 115200
#define BUFFER_SIZE 10

#define MSG_S_RECEIVE_MODEL ">SW:1,1\r\n"
#define MSG_S_SEND_MODEL ">OUT:1,1\r\n"
#define MSG_S_SEND_SIZE 10
//#define MSG_S_SEND_SIZE strlen(MSG_S_SEND_MODEL)
#define MSG_S_RECEIVE_SIZE 10
//#define MSG_S_RECEIVE_SIZE strlen(MSG_S_RECEIVE_MODEL)
#define UWAIT	100000	//10 ms

#define TCP_PORT	10000
#define ADDR_IP		"127.0.0.1"

//*********** Variablos globales ***********
char Buffer[BUFFER_SIZE];
char Buffer2[BUFFER_SIZE];
int ret;
int ret2;

pthread_t h_thread,h_thread1;
volatile sig_atomic_t flag_fin;

socklen_t addr_len;
struct sockaddr_in clientaddr;
struct sockaddr_in serveraddr;
char buffer_tx[128];
char buffer_rx[128];
char data_send[]=MSG_S_SEND_MODEL;
int newfd;
int s;


// *********** ***************** 
void* start_tcp(void* message)
{
	while(1){
		// se hace accept() para recibir conexiones entrantes,
		printf("Esperando por un cliente\n");
		addr_len = sizeof(struct sockaddr_in);
    	if ( (newfd = accept(s, (struct sockaddr *)&clientaddr, &addr_len)) == -1){
	 		perror("Error en accept");
	   		exit(1);
		}

		char ipClient[32];
		inet_ntop(AF_INET, &(clientaddr.sin_addr), ipClient, sizeof(ipClient));
		printf  ("Server:  %s\n",ipClient);

		if( (ret2 = read(newfd,buffer_tx,128)) == -1 ){
			perror("Error leyendo mensaje del socket");
			// exit(1);
		}
		
		buffer_tx[ret2]=0;

		if(ret2==BUFFER_SIZE){
			serial_send(buffer_tx,ret2);
			printf("%d bytes recibidos del socket %s\n",ret,buffer_tx);
		}

		while(ret2!=-1 && ret2!=0){
			ret2 = read(newfd,buffer_tx,128);
			if(ret==BUFFER_SIZE){
				strcpy(data_send,buffer_tx);
				printf ("%s",buffer_tx);
				serial_send(data_send,ret2);
				printf("%d bytes recibidos del socket %s\n",ret2,buffer_tx);
			}
		}
		printf("Error la conexión\n");
		close(newfd);
	}
}



// ************* Main ***************
int main(void)
{
	ret = serial_open(SERIAL_PORT, SERIAL_BAUDRATE);
    if ( ret!=0 )

    {
        printf("Error opening serial port1: %d\n", ret);
        exit(1);
    }


	// Cargamos datos de direccion de server
    	bzero((char *) &serveraddr, sizeof(serveraddr));
    	serveraddr.sin_family = AF_INET;
    	serveraddr.sin_port = htons(4096);
    	//serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    	if(inet_pton(AF_INET, ADDR_IP, &(serveraddr.sin_addr))<=0)
    	{
        	fprintf(stderr,"ERROR servidor invalido IP\r\n");
        	return 1;
    	}



	printf("Inicio Serial Service\r\n");
	printf("prende LED1 \r\n");
	serial_send(">OUT:1,1\r\n" ,BUFFER_SIZE);
	sleep(1);
	printf("prende LED2 \r\n");
	serial_send(">OUT:2,1\r\n" ,BUFFER_SIZE);
	sleep(1);
	printf("apaga LED1 \r\n");
	serial_send(">OUT:1,0\r\n" ,BUFFER_SIZE);

	printf("leyendo\r\n");

ret=0;
while(1)
	{
		usleep(1000);
		

		ret=serial_receive(Buffer2,BUFFER_SIZE);
		if (ret>>0) {

			printf(")se recibieron  %d  char \n", ret);
			printf("se recibio %s\n",  Buffer2);
			ret=0;//strcpy(Buffer2,"");
		}
		
		

	} // fin while
	// serial_close();
	exit(EXIT_SUCCESS);
	return 0;
}
