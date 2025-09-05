#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>


// macros
#define PORT 2000
#define MAX_CLIENTS 5
#define BACKLOG MAX_CLIENTS+10
#define BUF 256


// global varibales
struct sockaddr_in server_addr;
int server_fd;
int current_amount_of_clients=0;

// mutex
pthread_mutex_t client_mutex;



// sigint handler
void sigintHandler(int signum){
	if(signum==SIGINT){
		printf("closing the server\n");
		close(server_fd);
		abort();
	}

}


void* talk(void* arg){

	int client_fd =*((int*)(arg));
	free(arg);
	arg=NULL;
	
	char buf[BUF];
	
	while(1){
	
		memset(buf, 0, BUF);
		ssize_t bytes_recv=recv(client_fd, buf, BUF-1 /*to prevent buffer overflow*/, 0);
		
		if(bytes_recv<0){
			printf("error receiving.. continuing..\n");
			continue;
		}
		else if(bytes_recv==0){
			printf("client called an unconditonal close\n");
			break;
		}
		
		buf[bytes_recv]='\0';
		
		printf("msg : %s\n", buf);
		
		if(strcmp(buf, "close")==0){
			printf("close detected\nclosing connection with this client\n");
			break;
		}
		
	}
	
	//lets decrement this client and release the lock
	
	pthread_mutex_lock(&client_mutex);
	current_amount_of_clients--;
	pthread_mutex_unlock(&client_mutex);
	
	
	close(client_fd);	// closing this clients filedes
	return NULL;

}

void* accept_new_clients(void* arg){
		
		while(1){
		
			int* client_fd = (int*)malloc(sizeof(int)*1);
			
			struct sockaddr_in client_addr;
			socklen_t client_len=sizeof(client_addr);
			*client_fd=accept(server_fd, (struct sockaddr*)(&client_addr), &client_len);
			
			if(*client_fd==-1){
				perror("err in accept, continuing: ");
				free(client_fd);
				client_fd=NULL;
				continue;
			}
			
			
			
			// using mutex to track the number of clients
			pthread_mutex_lock(&client_mutex);
			if(current_amount_of_clients>=MAX_CLIENTS){
				printf("too much traffic, could not accept new client\n");
				close(*client_fd);
				free(client_fd);
				client_fd=NULL;
				pthread_mutex_unlock(&client_mutex);
				continue;
			}
			current_amount_of_clients++;
			pthread_mutex_unlock(&client_mutex);
			
			
			char ip[16];
			inet_ntop(AF_INET,&client_addr.sin_addr, ip, 16);
			printf("new connection made with client\nip: %s and port: %d\n", ip, ntohs(client_addr.sin_port));
			
			pthread_t client_thread;		// new client thread, that will execute the talk function
									// lets make this thread self detachable
			
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			
			int ret = pthread_create(&client_thread, &attr, &talk, (void*)(client_fd));
			if(ret!=0){
				printf("error creating the thread for the client\n");
				
				close(*client_fd);
				free(client_fd);
				client_fd=NULL;
				
				pthread_mutex_lock(&client_mutex);
				current_amount_of_clients--;
				pthread_mutex_unlock(&client_mutex);
				
			}
			
			pthread_attr_destroy(&attr);
			
		}
		
		return NULL;
	
}

int main(){

	// signal handlign for ctrl+c will be the first thing
	
	signal(SIGINT, sigintHandler);
	
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family= AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port = htons(PORT);   
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd==-1){
		perror("server fd err: ");
		return 1;
	}
	
	int bind_ret = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in));
	if(bind_ret==-1){
		perror("err in bind: ");
		close(server_fd);
		return 2;
	}
	
	printf("bind done\n");
	
	
	int listen_ret = listen(server_fd, BACKLOG);
	if(listen_ret==-1){
		perror("err in listen: ");
		close(server_fd);
		return 2;
	}
	
	printf("listening\n");
	
	
	pthread_mutex_init(&client_mutex,NULL);  // inititalizing the mutex
									// sole goal is to limit the no of simultaneous connections to MAX_CLIENTS
	
	
	
	pthread_t acceptor_thread;	// sole purpose of this thread is to accept new connections
	
	int ret=pthread_create(&acceptor_thread, NULL/*&attr*/, &accept_new_clients, NULL);  // calling the accpt function in here
	if (ret != 0) {
	    perror("pthread_create failed");
	    close(server_fd);

	    perror("acceptor thread: ");
	    return 3;
	}
	pthread_join(acceptor_thread, NULL);
	
	
	pthread_mutex_destroy(&client_mutex);	// destroying the mutex
	
	
	close(server_fd);
	printf("server closed\n");

	return 0;
}


