#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>



#define PORT 2000


int main(){

	int client_fd;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	
	
	client_fd = socket(AF_INET, SOCK_STREAM, 0);  	// AF_INET tells the os to use ipv4, SOCK_STREAM means tcp, no need to explicitly fill the 3rd param after that
							// AF_INET6 means ipv6, 3rd param we can explicitly say IPPROTO_TCP	
	
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	client_addr.sin_port = htons(8000);  // htons is host to network short
	client_addr.sin_family = AF_INET;   // address family
	inet_pton(AF_INET,"127.0.0.1",&client_addr.sin_addr.s_addr);  // pton means pattern to network
	
	bind(client_fd, (struct sockaddr*)(&client_addr), sizeof(client_addr));
	
	
	
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_port = htons(PORT);  // htons is host to network short
	server_addr.sin_family = AF_INET;   // address family
	inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr.s_addr);  // pton means pattern to network
	
	
	
	
	if(client_fd == -1){
		printf("error making the file descriptor for client\n");
		return 1;
	}
	
	int connect_ret = connect(client_fd, (struct sockaddr*)(&server_addr) , sizeof(struct sockaddr_in));
	if(connect_ret==-1){
		printf("somer error in connecting to the server\n");
		close(client_fd);
		return 2;
	}
	
	printf("connection made\n");
	
	
	// as the client will both send and receive, we will have to add select here too
	fd_set readfds;
	int max_fd = client_fd > 0 ? client_fd : 0;
	
	while(1){
		
		FD_ZERO(&readfds);
		FD_SET(client_fd, &readfds);
		FD_SET(0, &readfds);
	
		int select_ret = select(max_fd+1, &readfds, NULL, NULL, NULL);
		if(select_ret==-1){
			perror("error: ");
		}
		
		if(FD_ISSET(0, &readfds)){
		
			char* buf=NULL;
			size_t size=0;
			ssize_t bytes_read= getline(&buf, &size, stdin);
			if(bytes_read<=0){
				printf("somer error while reading from stdin\n");
				continue;
			}
			buf[strcspn(buf,"\n")]='\0';
			
			int bytes_sent=send(client_fd, buf, strlen(buf), 0);
			if(bytes_sent==-1){
				printf("some error sending data\n");
				continue;
			}
			
			if(strcmp(buf,"close")==0){
				printf("close detected.. closing\n");
				break;
			}
			
			
		}
		if(FD_ISSET(client_fd, &readfds)){
			
			char recv_buffer[160]={0};
			recv(client_fd, recv_buffer, sizeof(recv_buffer), 0);
			printf("%s\n", recv_buffer);
			
		}
		
		
	}
	
	close(client_fd);
	printf("closed\n");

	return 0;
}




