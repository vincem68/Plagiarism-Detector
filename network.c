#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>
#include <signal.h>
#include "ht.c"

struct arg {
	struct connection *con;
	Hashtable *table;
	
};

struct connection {
	struct sockaddr_storage addr;
	socklen_t addr_len;
	int fd;
};
int running = 1;

int server(char *port, Hashtable *h);
void *update(void *args);

void handler(int signal){
	running = 0;
}

void *update(void *args){
	
	int run = 1;
	char host[100], port[10];
	struct arg *argptr = args;
	struct connection *c = (struct connection *) argptr->con;
	struct Hashtable *h = (struct Hashtable *) argptr->table;
	int error;
	error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
	while (run){
		char *command = malloc(1);
		if (command == NULL){
			write(c->fd, "ERR\nSRV\n", 8);
			run = 0;
			continue;
		}
		char* number = malloc(sizeof(char));
		if (number == NULL){
			write(c->fd, "ERR\nSRV\n", 8);
			run = 0;
			continue;
		}
		int bytes;
		int i = 0;
		while ((bytes = read(c->fd, &command[i], 1)) > 0){
			if (command[i] == '\0'){
				write(c->fd, "ERR\nBAD\n", 8);
				free(command); 	
				free(number);
				run = 0;
				break;
			}
			if (command[i] == '\n'){
				command[i] = '\0';
				break;
			}
			command = realloc(command, i + 2);
			i++;
		}
		if (run == 0){
			continue;
		}
	
		i = 0;
		if (!(strcmp(command, "GET") == 0 || strcmp(command, "SET") == 0 || strcmp(command, "DEL") == 0)){ //we had \n here
			write(c->fd, "ERR\nBAD\n", 8);
			free(number);
			free(command);
			run = 0;
			continue;
		}
		while ((bytes = read(c->fd, &number[i], 1)) > 0){
			if (number[i] == '\n'){
				number[i] = '\0';
				break;
			}
			if (!isdigit(number[i])){
				write(c->fd, "ERR\nBAD\n", 8);
				free(number);
				free(command);
				run = 0;
				//continue;
				break;
			} 
			number = realloc(number, i + 2);
			i++;
		}
		if (run == 0){
			continue;
		}
		int length = atoi(number);
		if (length < 1){
			free(command);
			free(number);
			run = 0;
			continue;
		}
		char *message = malloc(sizeof(char) * length + 1); 
		if (message == NULL){
			write(c->fd, "ERR\nSRV\n", 8);
			run = 0;
			continue;
		}
		int x = 0;
		int early = 0;
		while (x < length){
			bytes = read(c->fd, &message[x], 1);
			if (message[x] == '\n'){
				early++;
			}
			if (early == 2 && x != length - 1){
				early++;
				write(c->fd, "ERR\nLEN\n", 8);
				run = 0;
				free(message);
				free(command);
				free(number);
				break;
			}
			x++;
		}
		if (early == 3){
			continue;
		}
		if (message[length - 1] != '\n'){
			write(c->fd, "ERR\nLEN\n", 8);
			run = 0;
			free(message);
			free(command);
			free(number);
			continue;
		}
		message[x] = '\0';

		for (int j = 0; j < length; j++){
			if (message[j] == '\0'){
				free(message);
				write(c->fd, "ERR\nBAD\n", 8);
				free(command);
				free(number);
				run = 0;
				break;
			}
			if (j == length - 1 && message[j] != '\n'){
				free(message);
				write(c->fd, "ERR\nBAD\n", 8);
				free(command);
				free(number);
				run = 0;
				break;
			}
			if (j != length - 1 && message[j] == '\n' && command[0] != 'S'){
				free(message);
				write(c->fd, "ERR\nBAD\n", 8);
				free(command);
				free(number);
				run = 0;
				break;
			}
		}
		if (run == 0){
			continue;
		}
		if (strcmp(command, "GET") == 0){
			int mark_get = 0;
			for (int k = 0; k <= length; ++k){
				if (message[k] == '\n'){
					mark_get = k;
					break;
				}
			}
			char *key = malloc(sizeof(char) * (mark_get + 1)); 
			for (int k = 0; k < mark_get; ++k){
				key[k] = message[k];
			}
			key[mark_get] = '\0';

			pthread_mutex_lock(&h->lock);
			Node *ptr = get(h, key, mark_get, c->fd);
			if (ptr != NULL){
				write(c->fd, "OKG\n", 4);
				printValDetails(ptr, c->fd);
			}
			else {
				printKNF(c->fd);
			}
			pthread_mutex_unlock(&h->lock);
			free(key);	
		}
		else if (strcmp(command, "DEL") == 0){
			int mark_del = 0;
			for (int p = 0; p <= length; ++p) {
				if (message[p] == '\n') {
					mark_del = p;
					break;
				}
			}
			char *key = malloc(sizeof(char) * (mark_del + 1));
			for (int p = 0; p < mark_del; ++p) {
				key[p] = message[p];
			}
			key[mark_del] = '\0';
			pthread_mutex_lock(&h->lock);
			del(h, key, mark_del, c->fd);
			pthread_mutex_unlock(&h->lock);
			free(key);
		}
		else if (strcmp(command, "SET") == 0){
			int mark;
			for (int j = 0; j <= length; j++){
				if (message[j] == '\n'){
					mark = j;
					break;
				}
			}
			if (mark == length - 1){
				write(c->fd, "ERR\nBAD\n", 8);
				free(message);
				free(number);
				free(command);
				run = 0;
				continue;
			}
			char *key = malloc(sizeof(char) * (mark + 1)); 
			char *value = malloc(sizeof(char) * (length - mark - 1));
			for (int j = 0; j < mark; j++){
				key[j] = message[j];
			}
			key[mark] = '\0';
			for (int j = mark + 1; j < length; j++){
				value[j - (mark + 1)] = message[j];
			}
			value[length - (mark + 2)] = '\0';
			pthread_mutex_lock(&h->lock);
			set(h, key, value, mark, (length - (mark + 2)), c->fd);
			write(c->fd, "OKS\n", 4);
			pthread_mutex_unlock(&h->lock);
			
		}
		free(message); free(number); free(command);
	}

		close(c->fd);
		free(argptr->con);
		free(argptr);


	return NULL;
}

int server(char *port, Hashtable *h){

	struct addrinfo hint, *info, *info_list;
	struct connection *connect;
	int error, sfd;
	pthread_t threads;
	
	memset(&hint, 0, sizeof(struct addrinfo));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM; //both of these are defaults
	hint.ai_flags = AI_PASSIVE; //this is to have sockets listen
	error = getaddrinfo(NULL, port, &hint, &info_list);
	
	if (error != 0){
		perror("Couldn't connect\n");
		return -1;
	}

	for (info = info_list; info != NULL; info = info->ai_next){
		sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
		if (sfd == -1){
			continue;
		}
		if ((bind(sfd, info->ai_addr, info->ai_addrlen) == 0) && (listen(sfd, 5) == 0)){
			break;
		}
		close(sfd);
	}
	
	if (info == NULL){
		perror("Error\n");
		return -1;
	}
	
	freeaddrinfo(info_list);
	struct sigaction act;
	act.sa_handler = handler;	
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGINT, &act, NULL);
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);	

	while (running){ 
		connect = malloc(sizeof(struct connection));
		connect->addr_len = sizeof(struct sockaddr_storage); 
		connect->fd = accept(sfd, NULL, NULL);
	
		if (connect->fd == -1){
			continue;
		} 
		struct arg *args = malloc(sizeof(struct arg));
		args->con = connect;
		args->table = h;
		error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
		error = pthread_create(&threads, NULL, update, args);
		if (error != 0){
			write(connect->fd, "ERR\nSRV\n", 8);
			//free(connect); 
			free(args);
			close(connect->fd);
			free(connect);
			continue;
		}
		pthread_detach(threads);
		error = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
	}
	printf("out of the loop\n");
	freeHT(h);
	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return 0;
}


int main(int argc, char **argv){

	Hashtable *h = initialize(20);
	if (argc != 2){
		printf("Wrong number of arguments\n");
		return EXIT_FAILURE;
	}
	server(argv[1], h);
	return EXIT_SUCCESS;

}
