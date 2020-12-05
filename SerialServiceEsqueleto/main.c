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
#define SERIAL_PORT	3 // depende del puerto en la placa
#define SERIAL_BAUDRATE 115200
//#define BUFFER_SIZE 10
#define PUERTO_TCP	10000
#define ADDR_IP		"127.0.0.1"
#define MSG_TYPE ">OUT:1,0\r\n"
#define BUFFER_SIZE strlen(MSG_TYPE)

//*********** Variablos globales ***********
char Buffer[BUFFER_SIZE];
char Buffer2[BUFFER_SIZE];
int ret;

pthread_t h_thread,h_thread1;
void* ret;
volatile sig_atomic_t flag_fin;

// Variables que se usan para la conexión tcp
socklen_t addr_len;
struct sockaddr_in clientaddr;
struct sockaddr_in serveraddr;
char buffer_tx[128],buffer_rx[128];
char data_send[]=MSG_SERIAL_SND;
int newfd;
int n;
int fd_s;



// ************* Main ***************
int main(void)
{
	ret = serial_open(SERIAL_PORT, SERIAL_BAUDRATE);
    if ( ret!=0 )

    {
        printf("Error opening serial port1: %d\n", ret);
        exit(1);
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
