//CLIENT


#include <stdio.h>			//per funzioni di I/O (printf, ecc...)
#include <sys/socket.h>		//per funzioni su socket
#include <stdlib.h>			//per funzioni quali malloc, free, ecc.
#include <netinet/in.h>		//per indirizzi socket
#include <netdb.h> 			//per struct hostent
#include <string.h>			//per funzioni di manipolazione stringhe
#include <signal.h>			//per manipolare segnali
#include <errno.h>			//per usare variabile errno


#define SERVERPORT 5000		//porta fissa del server
#define RISCONTRO 3 		//dimensione massima del riscontro server
#define NOME 10				//taglia massima del buffer per l'inserimento del nome utente
#define COMANDO 4 			//taglia del buffer che deve contenere il comando preso in input (1 char)
#define VOCEUTENTE 16		//numero di caratteri occupati da una voce nella lista utenti
#define MAX 1000			//numero massimo di utenti
#define CODUTENTE 6			//taglia del buffer che deve contenere il codice dell'utente in lista
#define MESSAGGIO 1002		//taglia del buffer che deve contenere il messaggio
#define DIM_IP 19			//lunghezza massima di un indirizzo ip
#define TIMEOUT 60			//secondi che un utente può passare in attesa di una richiesta di chat

//#define CUSTOM_IP
#define DEBUG


typedef struct thread_arg {
	//argomenti da passare ai thread della chat

	char* nome;
}thread_arg;


int clientSocket;							//descrittori sockets
struct sockaddr_in server_addr;				//indirizzo server
struct hostent* hp;


int comando;								//comando inserito
char command[COMANDO];
char nomeClient[NOME];


struct sigaction sig_act;					//per definire handlers dei segnali


void chiusura(char * stringa){
	printf("%s", stringa);
	close(clientSocket);
	exit(0);
}


int acquisisci_stringa_dimensionata_anche_vuota(char* buff, int buffSize) {

	//il numero di caratteri digitati dall'utente può essere al massimo buffSize - 2
	//buff deve essere dichiarato come array

	strcpy(buff, "");

	char* contenuto = NULL;
	size_t dimensione = 0;
	int caratteri;
	caratteri = getline(&contenuto, &dimensione, stdin);

	//con getline, in caso di inserimento stringa vuota, caratteri = 1 (considera comunque \n)

	if (caratteri > buffSize - 1) {
		printf("Errore: superato il limite massimo!\n");
		free(contenuto);
		return -1;
	}

	if (strcmp(contenuto, "\n") != 0) {
		strcpy(buff, contenuto);
	}

	buff[caratteri-1] = '\0';
	free(contenuto);
	return 0;

}


int acquisisci_stringa_dimensionata(char* buff, int buffSize) {

	//il numero di caratteri digitati dall'utente può essere al massimo buffSize - 2
	//buff deve essere dichiarato come array

	while(1) {
		strcpy(buff, "");

		char* contenuto = NULL;
		size_t dimensione = 0;
		int caratteri;
		caratteri = getline(&contenuto, &dimensione, stdin);

		if (caratteri > buffSize - 1) {
			printf("Errore: superato il limite massimo! Riprova:\n");
			free(contenuto);
		}
		else if (strcmp(contenuto, "\n") == 0) {
			printf("Errore: inserire almeno un carattere! Riprova:\n");
			free(contenuto);
		}
		else{
			strcpy(buff, contenuto);
			buff[caratteri-1] = '\0';
			free(contenuto);
			return 0;
		}

	}
	return -1;
}


void gestione_rottura_canale() {

	chiusura("\nINTERRUZIONE DELLA COMUNICAZIONE CON IL SERVER\n");
}


void gestione_accesso_illegale() {

	printf("\nACCESSO NON CONSENTITO\n");
	raise(SIGINT);
}


void gestione_interruzione() {

	chiusura("\nINTERRUZIONE INASPETTATA DEL SERVIZIO\n");
}


void gestione_interruzione_in_chat() {

	if(write(clientSocket, "quit\n", MESSAGGIO) == -1) {
		printf("\nErrore nell'inoltro del messaggio di chiusura!\n");
	}
	chiusura("\nINTERRUZIONE INASPETTATA DEL SERVIZIO\n");
}


void gestione_timer() {

	printf("\nTIMEOUT SCADUTO, DISCONNESSIONE DAL SERVER\n");
	raise(SIGINT);
}


void* thread_lettura(void* arg) {

	char buff[MESSAGGIO];
	memset(buff, 0, MESSAGGIO);

	while(1) {
		//legge i messaggi in arrivo sulla socket
		if(read(clientSocket, buff, MESSAGGIO) == -1) {


#ifdef DEBUG
			printf("\nErrore nella lettura dei messaggi in arrivo!\n");
#endif

			pthread_exit(NULL);
		}

		if(strcmp(buff, "quit\n") == 0 || strlen(buff) <= 1) {

			printf("\nFINE DELLA CONVERSAZIONE\n");
			pthread_exit(NULL);
		}

		//stampa del nome prima del messaggio ricevuto
		int size_nome = strlen( ((thread_arg*) arg)->nome ) + 2;
		char nome[size_nome];
		strcpy(nome, ((thread_arg*) arg)->nome);
		strcpy(&nome[size_nome-2], "> ");

		if(write(1, nome, size_nome) == -1) {
			printf("\nErrore nella stampa del nome!\n");
		}

		//stampa del messaggio
		if(write(1, buff, MESSAGGIO) == -1) {
			printf("\nErrore nella stampa dei messaggi in arrivo!\n");
		}
		printf("\n");

		memset(buff, 0, MESSAGGIO);
	}
	return NULL;
}


void* thread_scrittura() {

	//settare handler per gestione segnali
	sig_act.sa_handler = gestione_interruzione_in_chat;
	if (sigaction(SIGINT, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGINT\n");
	}

	int msg_len;
	char buff[MESSAGGIO];
	memset(buff, 0, MESSAGGIO);

	while(1) {
		//acquisisce messaggi da stdin
		while(1) {
			if(read(0, buff, MESSAGGIO) == -1) {

#ifdef DEBUG
				printf("\nErrore nella acquisizione dei messaggi da tastiera!\n");
#endif

				pthread_exit(NULL);
			}

			msg_len = strlen(buff);
			if(msg_len > MESSAGGIO)
				printf("Errore: superato il limite massimo! Riprova:\n");
			else if(msg_len == 1)
				printf("Errore: inserire almeno un carattere! Riprova:\n");
			else
				break;

			memset(buff, 0, MESSAGGIO);
		}

		buff[msg_len] = '\0';

		//scrive i messaggi sulla socket
		if(write(clientSocket, buff, MESSAGGIO) == -1) {

#ifdef DEBUG
			printf("Errore nella scrittura dei messaggi!");
#endif

			pthread_exit(NULL);
		}

		if(strcmp(buff, "quit\n") == 0) {
			printf("\nFINE DELLA CONVERSAZIONE\n");
			pthread_exit(NULL);
		}

		memset(buff, 0, MESSAGGIO);
	}


	return NULL;
}


int aggiorna_lista_utenti() {

	char buff[VOCEUTENTE * MAX + 1] = "";

	if(read(clientSocket, buff, sizeof(buff)) == -1) {
		printf("Errore nella lettura della lista degli utenti connessi!\n");
		return -1;
	}

	printf("\nLISTA DEGLI UTENTI CONNESSI:\n"
			"%s\n", buff);
	return 0;
}


int avvia_chat(){

	char codiceUtente[CODUTENTE] = "";		//codice utente
	char nome[NOME] = "";					//nome del destinatario
	pthread_t read_tid, write_tid;			//tids dei threads
	thread_arg arg;							//argomento da passare al thread di lettura

	printf("\nInserire il codice identificativo dell'utente desiderato:\n");
	acquisisci_stringa_dimensionata(codiceUtente, CODUTENTE);

	int cod = atoi(codiceUtente);

#ifdef DEBUG
	printf("\ncodice utente: %d\n\n", cod);
#endif

	if(cod > MAX) {
		printf("\nUtente non disponibile!\n");
		return -1;
	}
	else {

		//scrive il codice dell'utente desiderato
		if(write(clientSocket, codiceUtente, CODUTENTE) == -1) {
			printf("Errore nella comunicazione del codice utente!\n");
			return -1;
		}

		//legge il riscontro del server
		if(read(clientSocket, nome, NOME) == -1) {
			printf("Errore nella lettura del riscontro!\n");
			return -1;
		}
		if (strcmp(nome, "ko") == 0) {
			printf("\nUtente non disponibile!\n");
			return -1;
		}

		printf("\nINIZIO DELLA CHAT CON: %s\n", nome);

		//inizio threads lettura & scrittura
		if(pthread_create(&write_tid, NULL, thread_scrittura, NULL) != 0) {
			printf("Errore nella creazione del thread di scrittura!\n");
		}

		arg.nome = nome;
		if(pthread_create(&read_tid, NULL, thread_lettura, &arg) != 0) {
			printf("Errore nella creazione del thread di lettura!\n");
		}

		//attesa della terminazione dei thread
		while(1) {
			sleep(1);
			if(pthread_kill(read_tid, 0) != 0) {

				pthread_cancel(write_tid);
				break;

			}

			if(pthread_kill(write_tid, 0) != 0) {

				pthread_cancel(read_tid, SIGINT);
				break;

			}
		}
	}

	//ripristino handler segnali
	sig_act.sa_handler = gestione_interruzione;
	if (sigaction(SIGINT, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGINT\n");
	}

	return 0;
}


int attendi_chat() {

	char nome[NOME] = "";
	pthread_t read_tid, write_tid;			//tids dei threads
	thread_arg arg;							//argomento da passare al thread di lettura

	//setta timer per evitare attesa infinita
	if(alarm(TIMEOUT) == -1) {
		printf("Errore nell'impostazione del timer!\n");
	}

	//lettura della comunicazione di inizio conversazione dal server
	if(read(clientSocket, nome, NOME) == -1) {
		printf("Errore nella lettura del nome del mittente!\n");
		return -1;
	}

	//reset timer
	if(alarm(0) == -1) {
		printf("Errore nel reset del timer!\n");
	}

	printf("\nINIZIO DELLA CHAT CON: %s\n\n", nome);

	//inizio threads lettura & scrittura
	arg.nome = nome;
	if(pthread_create(&read_tid, NULL, thread_lettura, &arg) != 0) {
		printf("Errore nella creazione del thread di lettura!\n");
	}

	if(pthread_create(&write_tid, NULL, thread_scrittura, NULL) != 0) {
		printf("Errore nella creazione del thread di scrittura!\n");
	}

	//attesa della terminazione dei thread
	while(1) {
		sleep(1);
		if(pthread_kill(read_tid, 0) != 0) {

			pthread_cancel(write_tid);
			break;
		}

		if(pthread_kill(write_tid, 0) != 0) {

			pthread_cancel(read_tid);
			break;
		}
	}

	return 0;
}


int chiudi_connessione(){

	chiusura("\nTi sei disconnesso. Arrivederci!\n");
	return 0;
}


void main(){

	sigfillset(&sig_act.sa_mask);
	sig_act.sa_flags = 0;

	sig_act.sa_handler = gestione_interruzione;
	if (sigaction(SIGINT, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGINT\n");
	}
	if (sigaction(SIGHUP, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGHUP\n");
	}
	if (sigaction(SIGQUIT, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGQUIT\n");
	}
	if (sigaction(SIGTERM, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGTERM\n");
	}

	if (sigaction(SIGUSR1, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGUSR1\n");
	}

	sig_act.sa_handler = gestione_accesso_illegale;
	if (sigaction(SIGILL, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGILL\n");
	}
	if (sigaction(SIGSEGV, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGSEGV\n");
	}

	sig_act.sa_handler = gestione_rottura_canale;
	if (sigaction(SIGPIPE, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGPIPE\n");
	}

	sig_act.sa_handler = gestione_timer;
	if (sigaction(SIGALRM, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGALRM\n");
	}

	char riscontro[RISCONTRO]; //risposta del server all'invio del comando

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket==-1){
		chiusura("Errore nella creazione della server socket!\n");
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVERPORT);


#ifdef DEBUG
	printf("Porta verso il server: %hu\n", server_addr.sin_port);
#endif


#ifndef CUSTOM_IP
	hp = gethostbyname("127.0.0.1");
	memcpy(&server_addr.sin_addr,hp->h_addr,4);
#endif

#ifdef CUSTOM_IP
	char ip[DIM_IP];

	printf("\nInserire l'indirizzo ip del server (127.0.0.1 è il locale):\n");
	acquisisci_stringa_dimensionata(ip, DIM_IP);

	hp = gethostbyname(ip);
	memcpy(&server_addr.sin_addr,hp->h_addr,4);
#endif


	if(connect(clientSocket, (struct sockaddr *) &server_addr, sizeof(server_addr))==-1) {
		chiusura("Errore nella connect!\n");
	}
	else {
		printf("Connessione stabilita\n");
	}

	printf("Inserire il proprio nickname (massimo %d caratteri)\nNome: ", NOME - 2);
	acquisisci_stringa_dimensionata(nomeClient, NOME);

	//comunicazione nome al server
	if(write(clientSocket, nomeClient, NOME) == -1) {
		chiusura("Errore nella comunicazione del nome al server!\n");
	}

	printf("\nCiao %s!\n", nomeClient);

	if(read(clientSocket, riscontro, RISCONTRO) == -1) {
		chiusura("Errore nel riscontro della disponibilità\n");
	}
	if (strcmp(riscontro, "ko") == 0) {
		chiusura("Spazio non disponibile. Riprova più tardi!\n");
	}

	while(1){
		printf( "Digitare il numero del comando desiderato:\n"
				"1:AGGIORNA LA LISTA DEGLI UTENTI CONNESSI\n"
				"2:AVVIA LA CHAT CON UN UTENTE CONNESSO\n"
				"3:ATTENDI CHE UN UTENTE TI CONTATTI\n"
				"4:DISCONNETTI\n");

		acquisisci_stringa_dimensionata_anche_vuota(command, COMANDO);
		comando = atoi(command);

#ifdef DEBUG
		printf ("Hai digitato %d\n", comando);
#endif

		if(write(clientSocket, &comando, sizeof(int))== -1) {
			chiusura("Errore nell'invio del comando\n");
		}

		if(read(clientSocket, riscontro, RISCONTRO) == -1) {
			chiusura("Errore nel riscontro del comando\n");
		}

		if (strcmp(riscontro, "ok") == 0) {

			switch(comando) {

			case 0:	//nessun comando
				printf("\nINSERIRE ALMENO UN CARATTERE\n\n");
				break;

			case 1:	//aggiorna lista utenti connessi
				if(aggiorna_lista_utenti()==-1){
					printf("\nAGGIORNAMENTO DELLA LISTA UTENTI NON RIUSCITO\n\n");
				}
				break;

			case 2: //avvia chat
				if(avvia_chat()==-1){
					printf("\nAVVIO DELLA CHAT NON RIUSCITO\n\n");
				}
				break;

			case 3:	//attendi chat
				if(attendi_chat()==-1){
					printf("\nATTESA DELLA CHAT NON RIUSCITA\n\n");
				}
				break;

			case 4: //chiudi
				if(chiudi_connessione()==-1){
					printf("\nDISCONNESSIONE NON RIUSCITA\n\n");
				}
				break;

			default:
				printf("\nCOMANDO NON RICONOSCIUTO\n\n");
				break;

			}
		}

		else {
			printf("Errore nel riscontro del comando! : %s\n", riscontro);
		}
	}

	close(clientSocket);
	exit(0);
}
