//SERVER


#include <stdio.h>			//per funzioni di I/O (printf, ecc...)
#include <pthread.h>		//per implementare multithreading
#include <sys/socket.h>		//per funzioni su socket
#include <netinet/in.h>		//per indirizzi socket
#include <stdlib.h>			//per funzioni quali malloc, free, ecc.
#include <string.h>			//per funzioni di manipolazione stringhe
#include <signal.h>			//per manipolare segnali
#include <errno.h>			//per usare variabile errno
#include <sys/types.h>		//per semafori
#include <sys/ipc.h>		// "     "
#include <sys/sem.h>		// "     "


#include "stack.h"			//header relativo a struttura dati "stack"


#define SERVERPORT 5000		//porta fissa del server
#define BACKLOG 100			//numero di connessioni pendenti
#define MAX 1000			//numero massimo di utenti
#define NOME 10				//taglia massima del buffer per l'inserimento del nome utente
#define TIMEOUT 300			//secondi che la socket attende prima di chiudere la comunicazione (5 min)
#define CHIAVESEMAFORO 50	//chiave del semaforo
#define RISCONTRO 3 		//dimensione massima del riscontro server
#define VOCEUTENTE 16		//numero di caratteri occupati da una voce nella lista utenti
#define CODUTENTE 6			//taglia del buffer che deve contenere il codice dell'utente in lista
#define MESSAGGIO 1002		//taglia del buffer che deve contenere il messaggio


typedef struct client_attr {
	//attributi di un client

	int socket;					//descrittore socket di comunicazione con server
	unsigned key;				//chiave univoca per il client (0 non è ammessa)
	char nome[NOME];			//nome del client
	char disp;					//flag di disponibilità
} client_attr;


typedef struct thread_chat_arg {
	//argomenti da passare al thread della chat

	int mitt;
	int dest;
}thread_chat_arg;


int listeningSocket, acceptSocket;		//descrittori sockets
struct sockaddr_in server;				//indirizzo server
struct sockaddr_in client;				//indirizzo client
int clientLength = sizeof(client);


client_attr utentiConnessi[MAX + 1];	//lista utenti connessi
stack* chiaviDisponibili;				//stack delle chiavi libere
int limiteListaUtenti = 0;
int semaforo;


struct timeval timeout;					//timeout per setsockopt()


struct sigaction sig_act;				//per definire handlers dei segnali


unsigned chiaviMittenti[MAX + 1];		//array tampone per lo scambio delle chiavi degli utenti


char flag_attesa_chat[MAX + 1];			/*array di flag: il flag i-esimo indica se il client i-esimo
										  è in attesa o impegnato in una chat*/


void dealloca_strutture() {

	//elimina stack
	stack_delete(chiaviDisponibili);

	//elimina semaforo
	if(semctl(semaforo, 0, IPC_RMID, 0) == -1) {
		//con IPC_RMID gli altri parametri vengono ignorati (vd dispensa)
		printf("Errore nella rimozione del semaforo!\n");
	}
}


void chiusura(char * stringa){

	dealloca_strutture();
	printf("%s", stringa);
	exit(0);
}


void chiusura_thread(char * stringa, int socket, void* status, char* nome) {

	if (strcmp(nome, "") == 0)
		printf("%s\n", stringa);
	else
		printf("%s %s!\n", stringa, nome);

	close(socket);
	pthread_exit(status);
}


void inizializza_strutture() {

	//crea (init) stack
	chiaviDisponibili = stack_new();
	int i;
	for(i = MAX; i > 0; i--) {
		stack_push(chiaviDisponibili, i);
	}

	//crea (init) semaforo
	semaforo = semget(CHIAVESEMAFORO, 1,IPC_CREAT | IPC_EXCL  | 0666);	//può dare problemi con interruzione anomala
	if(semaforo == -1) {
		chiusura("Errore nella creazione del semaforo!\n");
	}
	if(semctl(semaforo, 0, SETVAL, 1)) {								//inserisce un token nel semaforo
		chiusura("Errore nell'inizializzazione del semaforo!\n");
	}

}


int sem_wait(int sem_descr, int sem_num) {

	struct sembuf operazioni[1] = {{ sem_num, -1, 0}};
	if(semop(sem_descr, operazioni, 1) == -1) {
		chiusura("Errore nella sem_wait!\n");
		return -1;
	}
	return 0;
}


int sem_signal(int sem_descr, int sem_num) {

	struct sembuf operazioni[1] = {{ sem_num, +1, 0}};
	if(semop(sem_descr, operazioni, 1) == -1) {
		chiusura("Errore nella sem_signal!\n");
		return -1;
	}
	return 0;
}


void gestione_accesso_illegale() {
	printf("\nACCESSO NON CONSENTITO\n");
	raise(SIGINT);
}


void gestione_interruzione() {
	chiusura("\nINTERRUZIONE INASPETTATA DEL SERVIZIO\n");
}


void* thread_chat(void* arg) {

	char buff[MESSAGGIO];
	memset(buff, 0, MESSAGGIO);

	while(1) {
		//legge da arg.mitt
		if(read(  ((thread_chat_arg *) arg)->mitt, buff, MESSAGGIO) == -1) {
			printf("\nErrore nella lettura dal mittente!\n");

			//chiude thread
			pthread_exit(NULL);
			break;
		}

		//scrive su arg.dest
		if(write( ((thread_chat_arg *) arg)->dest, buff, MESSAGGIO) == -1) {
			printf("\nErrore nell'inoltro al destinatario!\n");

			//chiude thread
			pthread_exit(NULL);
			break;
		}

		if(strcmp(buff, "quit\n") == 0) {
			//chiude thread
			pthread_exit(NULL);
			break;
		}

		memset(buff, 0, MESSAGGIO);
	}
	return NULL;
}


int aggiorna_lista_utenti(int socket){

	char buff[VOCEUTENTE * MAX + 1] = "";
	int i, j = 1;	/*i-->scorrimento lista utenti
					  j-->spiazzamento buffer	  */

	for(i = 1; i <= limiteListaUtenti; i++) {
		client_attr aux = utentiConnessi[i];

		if(aux.key != 0) {

			int k = (j - 1) * VOCEUTENTE;
			snprintf(&(buff[k]), 5, "%04d", aux.key);
			snprintf(&(buff[k+4]), 10, "-%-8s", aux.nome);

			if(aux.disp == 0)
				snprintf(&(buff[k+13]), 3, "%2s", "X");
			else
				snprintf(&(buff[k+13]), 3, "%2s", "V");

			snprintf(&(buff[k+15]), 2, "\n");
			j++;

		}
	}

	if(write(socket, buff, sizeof(buff)) == -1) {
		return -1;
	}
	return 0;
}


int avvia_chat(int socketMitt, unsigned key, char* nome){

	char codiceUtente[CODUTENTE];
	unsigned cod;
	int socketDest;
	pthread_t mitt_tid, dest_tid;			//tids dei threads
	thread_chat_arg mitt_arg, dest_arg;		//struttura per gli argomenti da passare ai thread

	/*################################################################################*/
	//interazione iniziale con il mittente
	//verifica della disponibilita dell'utente selezionato


	//legge il codice dell'utente desiderato
	if(read(socketMitt, codiceUtente, CODUTENTE) == -1) {
		printf("\nErrore nella lettura del codice utente!\n");
		return -1;
	}

	cod = atoi(codiceUtente);

	if(utentiConnessi[cod].key == 0 || utentiConnessi[cod].disp == 0 || cod == key) {
		//riscontro negativo
		if(write(socketMitt, "ko", NOME) == -1) {
			printf("\nErrore nell'invio del riscontro negativo!\n");
			return -1;
		}

		return 1;
	}
	else {
		//riscontro positivo
		if(write(socketMitt, utentiConnessi[cod].nome, NOME) == -1) {
			printf("\nErrore nell'invio del nome del destinatario!\n");
			return -1;
		}

		//rende disponibile la chiave del mittente al destinatario
		chiaviMittenti[cod] = key;


	}
	/*################################################################################*/


	/*################################################################################*/
	//interazione con il destinatario

	//il server deve contattare il client che ha key = codiceUtente (contatto tramite socket)
	socketDest = utentiConnessi[cod].socket;
	if(write(socketDest, nome, NOME) == -1) {
		printf("\nErrore nell'invio del nome del mittente!\n");
		return -1;
	}


	//impostazione della disponibilità del destinatario a 0
	utentiConnessi[cod].disp = 0;

	/*################################################################################*/


	mitt_arg.mitt = socketMitt;
	mitt_arg.dest = socketDest;

	if(pthread_create(&mitt_tid, NULL, thread_chat, &mitt_arg) != 0) {
		printf("\nErrore nella creazione del thread del mittente!\n");
	}

	dest_arg.mitt = socketDest;
	dest_arg.dest = socketMitt;

	if(pthread_create(&dest_tid, NULL, thread_chat, &dest_arg) != 0) {
		printf("\nErrore nella creazione del thread del destinatario!\n");
	}

	//attesa che i thread terminino
	while(1) {
		sleep(1);
		if(pthread_kill(mitt_tid, 0) != 0) {
			pthread_cancel(dest_tid);
			break;
		}

		if(pthread_kill(dest_tid, 0) != 0) {
			pthread_cancel(mitt_tid);
			break;
		}
	}

	//setta il flag di attesa del destinatario
	flag_attesa_chat[cod] = 0;
	return 0;
}


int attendi_chat(int socketDest, unsigned key) {

	//setta la disponibilità del client
	utentiConnessi[key].disp = 1;

	//setta il flag di attesa
	flag_attesa_chat[key] = 1;

	//setta le strutture necessarie per la select
	fd_set file_descr_set;
	FD_ZERO(&file_descr_set);
	FD_SET(socketDest, &file_descr_set);
	struct timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;

	int result;

	while(flag_attesa_chat[key] == 1) {
		//deve attendere la fine della chat per evitare che il thread
		//del client si metta in attesa dei comandi dell'utente

		sleep(1);
		result = select(FD_SETSIZE, &file_descr_set, &file_descr_set, NULL, &time);

		//dai test risulta che il risultato è 2 solo quando il client
		//si disconnette durante la fase di attesa
		if(result == 2) {
			flag_attesa_chat[key] = 0;
			return -1;
		}

		if(result == -1) {
			printf("Errore nella select!\n");
			flag_attesa_chat[key] = 0;
			return -1;
		}
	}
	return 0;
}


int chiudi_connessione(int socket, char* messaggio, char* nome, unsigned key){

	/*################################################################################*/
	//parte relativa alla rimozione del record dell'utente

	//INIZIO SEZIONE CRITICA
	sem_wait(semaforo, 0);

	//rende nuovamente disponibile la chiave
	stack_push(chiaviDisponibili, key);

	//azzera lo slot
	utentiConnessi[key].key = 0;
	utentiConnessi[key].socket = 0;
	strcpy(utentiConnessi[key].nome, "");

	//aggiorna il limite della lista utenti
	if(key == limiteListaUtenti) {
		int i = limiteListaUtenti;
		while(utentiConnessi[i].key == 0 && i > 0) {
			i--;
		}
		limiteListaUtenti = i;
	}

	//reinizializza lo stack se tutti gli utenti si sono disconnessi
	if(limiteListaUtenti == 0) {

		stack_delete(chiaviDisponibili);
		chiaviDisponibili = stack_new();
		int i;
		for(i = MAX; i > 0; i--) {
			stack_push(chiaviDisponibili, i);
		}

	}

	sem_signal(semaforo, 0);
	//FINE SEZIONE CRITICA
	/*################################################################################*/

	int status;
	chiusura_thread(messaggio, socket, &status, nome);

	return 0;
}


void* thread_main() {

	int socket = acceptSocket;
	client_attr attr;
	int key;
	int comando;
	int status;

	strcpy(attr.nome, "");

	//procedura di inizializzazione:
	//1) richiede al client il nome
	//2) inserisce client nella lista on line (come disponibile)
	//3) si mette in attesa dei comandi


	//inserisce nome (max 10 char)
	if(read(socket, attr.nome, NOME)==-1){
		//non è ancora iniziato il processo di log-in, non serve chiamare chiudi_connessione
		chiusura_thread("Errore nella read del nome!\n", socket, &status, attr.nome);
	}

	//setta la disponibilità
	attr.disp = 0;


	/*################################################################################*/
	//parte relativa al login dell'utente (inserimento nella lista utenti connessi)

	//INIZIO SEZIONE CRITICA
	sem_wait(semaforo, 0);

	//inserisce chiave
	if(stack_top(chiaviDisponibili, &(attr.key)) == -1) {
		if(write(socket, "ko", RISCONTRO) == -1) {
			printf("Errore nella comunicazione della disponibilità al client!\n");
		}
		chiusura_thread("Tentativo di accesso di fallito, sistema al limite!\n", socket, &status, attr.nome);
	}
	else {
		if(write(socket, "ok", RISCONTRO) == -1) {
			chiusura_thread("Errore nella comunicazione della disponibilità al client!\n", socket, &status, attr.nome);
		}
	}

	stack_pop(chiaviDisponibili);

	if(attr.key > limiteListaUtenti) {
		limiteListaUtenti = attr.key;
	}

	//inserisce socket
	attr.socket = socket;

	//aggiorna la lista degli utenti
	utentiConnessi[attr.key] = attr;

	sem_signal(semaforo, 0);
	//FINE SEZIONE CRITICA
	/*################################################################################*/

	printf("\n%s si è connesso!\n", attr.nome);

	//attesa dei comandi degli utenti
	while(1) {
		if(read(socket, &comando, sizeof(comando))==-1){
			chiudi_connessione(socket, "E' avvenuta la rottura del canale di comunicazione con", attr.nome, attr.key);
		}

		if(write(socket, "ok", RISCONTRO)== -1) {
			chiudi_connessione(socket, "E' avvenuta la rottura del canale di comunicazione con", attr.nome, attr.key);
		}

		switch(comando) {

		case 1:	//aggiorna lista utenti connessi
			comando = 0;
			if(aggiorna_lista_utenti(socket)==-1){
				chiudi_connessione(socket, "E' avvenuta la rottura del canale di comunicazione con", attr.nome, attr.key);
			}
			break;

		case 2: //avvia chat
			comando = 0;
			if(avvia_chat(socket, attr.key, attr.nome)==-1){
				chiudi_connessione(socket, "E' avvenuta la rottura del canale di comunicazione con", attr.nome, attr.key);
			}
			break;

		case 3:	//attendi chat
			comando = 0;
			if(attendi_chat(socket, attr.key)==-1){
				chiudi_connessione(socket, "E' avvenuta la rottura del canale di comunicazione con", attr.nome, attr.key);
			}
			break;

		case 4: //chiudi
			comando = 0;
			if(chiudi_connessione(socket, "\nsi è disconnesso", attr.nome, attr.key)==-1){
				chiudi_connessione(socket, "E' avvenuta la rottura del canale di comunicazione con", attr.nome, attr.key);
			}
			break;

		default:
			break;

		}
	}
	return NULL;
}


int main() {

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

	sig_act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &sig_act, NULL) == -1) {
		chiusura("Errore nella gestione di SIGPIPE\n");
	}

	inizializza_strutture();

	listeningSocket=socket(AF_INET, SOCK_STREAM, 0);
	if (listeningSocket==-1){
		chiusura("Errore nella creazione della socket!\n");
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(SERVERPORT);
	server.sin_addr.s_addr = INADDR_ANY;

	if(bind(listeningSocket, (struct sockaddr *) &server, sizeof(server)) == -1) {
		chiusura("Errore nel binding della socket!\n");
	}

	if(listen(listeningSocket, BACKLOG)==-1) {
		chiusura("Errore nella listen!\n");
	}

	while(1) {
		pthread_t tid;

		acceptSocket = accept(listeningSocket, (struct sockaddr *) &client, &clientLength);
		if(acceptSocket == -1) {
			printf("Errore nella accept!\n");
		}

		//per evitare il blocco del sistema dovuto a utente inattivo
		timeout.tv_sec = TIMEOUT;
		setsockopt(acceptSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		if(pthread_create(&tid, NULL, thread_main,NULL) == -1 ) {
			printf("Errore nella creazione del thread!\n");
		}
	}
	return 0;
}
