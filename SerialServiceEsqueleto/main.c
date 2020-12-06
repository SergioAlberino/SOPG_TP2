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

//************** Definiciones *************
#define SERIAL_PORT	1 // depende del puerto en la placa
#define SERIAL_BAUDRATE 115200
#define BUFFER_SIZE 10

#define MSG_S_RECEIVE_MODEL ">SW:1,1\r\n"
#define MSG_S_SEND_MODEL ">OUT:1,1\r\n"
#define MSG_S_SEND_SIZE 11
//#define MSG_S_SEND_SIZE strlen(MSG_S_SEND_MODEL)
#define MSG_S_RECEIVE_SIZE 10
//#define MSG_S_RECEIVE_SIZE strlen(MSG_S_RECEIVE_MODEL)
#define UWAIT	100000	//10 ms

#define TCP_PORT	10000
#define ADDR_IP		"127.0.0.1"
#define BACKCLOG	5


//*********** Variablos globales ***********
char Buffer[BUFFER_SIZE];
char Buffer2[BUFFER_SIZE];
int ret;
int ret2;

pthread_t h_thread,h_thread1;
volatile sig_atomic_t EOP;

socklen_t addr_len;
struct sockaddr_in clientaddr;
struct sockaddr_in serveraddr;
char buffer_tx[128];
char buffer_rx[128];
char data_send[]=MSG_S_SEND_MODEL;
char data_receive[MSG_S_RECEIVE_SIZE];
int len_data;
int newfd;
int s;


// **************************** 
void* newTCP(void* message)
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
		
		buffer_tx[ret2]=0x00;

		if(ret2 == MSG_S_RECEIVE_SIZE){
			serial_send(buffer_tx,ret2);
			printf("%d bytes recibidos del socket %s\n",ret,buffer_tx);
		}

		while(ret2!=-1 && ret2!=0){
			ret2 = read(newfd,buffer_tx,128);
			if(ret2==MSG_S_RECEIVE_SIZE){
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


// ********** Manejo de señales *********
void signal_receive(int signal)
{
	write(1,"Capturando las señales...\n",23);
	EOP=1;  // indica terminacion del programa en forma controlada
}

void blockSign(void)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void unBlockSign(void)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

// ***************************************************************+
int main(void)
{

printf("Inicio Serial Service\r\n");

	// Se inica com serie con la placa
	ret = serial_open(SERIAL_PORT, SERIAL_BAUDRATE);
    if ( ret!=0 )
    {
        printf("Error abriendo serial port: %d\n", ret);
        exit(1);
    }

	// Configuracion de señales 
    struct sigaction sa;
    sa.sa_handler = signal_receive;
    sa.sa_flags =0; 			//SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGINT,&sa,NULL)==-1){
		perror("Error de sigaction: SIGINT");
        	exit(1);
        }
    if(sigaction(SIGTERM,&sa,NULL)==-1){
		perror("Error de sigaction: SIGTERM");
        	exit(1);
        }

	// Se configura la conexión TCP
	s = socket(AF_INET,SOCK_STREAM, 0);
	if(s==-1){
		printf("No se pudo crear el socket\n");
		return -1;
	}
	// Se cargan datos IP:PORT del server
    bzero((char*) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(TCP_PORT);
    //serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(inet_pton(AF_INET, ADDR_IP, &(serveraddr.sin_addr))<=0){
        	fprintf(stderr,"ERROR invalid server IP\r\n");
        	return 1;
    }
	// Se abre el puerto con bind()
	if (bind(s, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1) {
		close(s);
		perror("Error al asociar IP:PUERTO");
		return 1;
	}
	// Se setea socket en modo Listening 
	if (listen (s, BACKCLOG) == -1) 
  	{
    	perror("error en listen");
    	exit(1);
  	}

	// Se crea thread para la conexión del TCP
	blockSign(); // enmascara todas las señales para que no las herede el thread
	if (pthread_create (&h_thread, NULL, newTCP,NULL)!=0) {
		perror("Error al crear el thread newTCP");
		return -1;
	}
	// detached del newTCP
	pthread_detach(h_thread);
	unBlockSign(); // reestablece la mascara de señales 

	while(1){
		
		if((len_data=serial_receive(data_receive,MSG_S_RECEIVE_SIZE))!=0){
			printf("Se recibieron %d bytes: %s",len_data,data_receive);
			strcpy(buffer_rx,data_receive);
			// printf("test %s ",buffer_rx);
			// Se envia mensaje a cliente
    		if (write (newfd, buffer_rx, MSG_S_RECEIVE_SIZE) == -1){
      			perror("Error escribiendo mensaje en socket");
      			exit (1);
    		}
		}
		usleep(UWAIT); // espera 10ms

		if(EOP==1){ // flag de señal de cierre ordenado
			printf("\n fin del programa \n");
			// Se cierran todas las conexiones
    		close(newfd);
    		close(s);			
			serial_close();
			return 0;
		}	
	}

	// El programa no debería llegar aquí
	return 0;
}






// // ************* Main ***************
// int main(void)
// {
// 	ret = serial_open(SERIAL_PORT, SERIAL_BAUDRATE);
//     if ( ret!=0 )

//     {
//         printf("Error opening serial port1: %d\n", ret);
//         exit(1);
//     }


// 	printf("Inicio Serial Service\r\n");
// 	printf("prende LED1 \r\n");
// 	serial_send(">OUT:1,1\r\n" ,BUFFER_SIZE);
// 	sleep(1);
// 	printf("prende LED2 \r\n");
// 	serial_send(">OUT:2,1\r\n" ,BUFFER_SIZE);
// 	sleep(1);
// 	printf("apaga LED1 \r\n");
// 	serial_send(">OUT:1,0\r\n" ,BUFFER_SIZE);

// 	printf("leyendo\r\n");

// ret=0;
// while(1)
// 	{
// 		usleep(1000);
		

// 		ret=serial_receive(Buffer2,BUFFER_SIZE);
// 		if (ret>>0) {

// 			printf(")se recibieron  %d  char \n", ret);
// 			printf("se recibio %s\n",  Buffer2);
// 			ret=0;//strcpy(Buffer2,"");
// 		}
		

// 	} // fin while
// 	// serial_close();
// 	exit(EXIT_SUCCESS);
// 	return 0;
// }
