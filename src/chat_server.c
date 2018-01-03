/*
 ============================================================================
 Name        : chat_server.c
 Author      : 
 Version     :
 Copyright   : 
 Description : Simple message server
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/time.h>

#define PORT 8080

int main(int argc, char const *argv[]) {
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */

	//Arbitrary limit to 30 clients
	int server_socket, new_socket, valread, max_sockets=30, sd, max_sd,
			activity, client;
	struct sockaddr_in address;
	//???
	int addrlen;
	int opt = 1;
	char buffer[1024] = {0};
	char *hello = "hello from server";
	fd_set readfds;

	//initialize client sockets
	int client_socket[max_sockets];
	for (int i=0; i < max_sockets; i++) {
		client_socket[i] = 0;
	}

	// Sets the protocol for communications
	// This is a pointer to the socket (a la file pointers)
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	//This can be called multiple times, each time setting a new option or set ot options
	// First argument is the socket
	// Second and third arguments set the option. The first is the level of option, and second are the options themselves
	// The last two are what you are setting it to and the size of what you are setting
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		perror("setsocketopt");
		exit(EXIT_FAILURE);
	}

	//This is the address struct
	//family of address IPv4, Ipv6,...
	address.sin_family = AF_INET;
	// this is a IP host address, host interface address in network byte order
	// this is localhost
	address.sin_addr.s_addr = INADDR_ANY;
	// This is just a conversion between host and network byte order
	address.sin_port = htons(PORT);

	//Binds to the specified address
	// In this case we are casting the pointer of address to a sockaddr struct so we can buk,d it as a sockaddr_in
	if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	// Puts the server in a passive listen mode
	// Second number is max size of queue when accepting connections
	if (listen(server_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	addrlen = sizeof(address);
	puts("waiting for connections");

	//Set up loop to run from here on
	while(1) {
		//clear the FD socket set
		// TODO Why do I have to make this anew on each loop?
		FD_ZERO(&readfds);

		//add known sockets to fd_set
		FD_SET(server_socket, &readfds);

		max_sd = server_socket;
		for (int i=0; i < max_sockets; i++) {
			//socket_descriptor
			sd = client_socket[i];

			//if valid socket descriptor then add to fd set
			if (sd > 0){
				FD_SET(sd, &readfds);
			}

			if (sd > max_sd) {
				max_sd = sd;
			}
		}

		// wait for activity on the sockets
		// if null, wait indefinitely
		// TODO what is the difference between select() and accept()?
		puts("stop loop");
		activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		// TODO what does this error code mean?
		if ((activity < 0) && (errno != EINTR)) {
			puts("select error");
		}

		// If something happened on the server socket...

		//it is an incoming connection
		// after running selct, to see if something has input, then we run fd_isset on it after running select
		if (FD_ISSET(server_socket, &readfds)) {
			//All these casts are important, but not sure why they are nessisary...
			if ((new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}

			//gives the server user the socket info
			printf("New Connection, socket fd is %d , ip is : %s , port : %d \n",
					new_socket,
					inet_ntoa(address.sin_addr),
					ntohs(address.sin_port));

			// send new connection greeting
			if (send(new_socket, hello, strlen(hello), 0) != 0) {
				perror("failed welcome send");
			}

			puts("sent hello");

			//add new socket
			for (int i = 0; i < max_sockets; i++) {
				//check if empty
				if (client_socket[i] ==0) {
					client_socket[i] = new_socket;
					printf("added to list at index %d\n", i);
				}
			}

		}

		// it is an existing connection
		for (int i = 0; i < max_sockets; i++) {
			client = client_socket[i];

			if (FD_ISSET(client, &readfds)){
				// Check to see if someone was disconnecting, otherwise read the message
				if ((valread= read(sd, buffer, 1024)) == 0) {
					//if it fails
					getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
					printf("Host disconnected , ip %s , port %d \n",
							inet_ntoa(address.sin_addr),
							ntohs(address.sin_port));
					// if you don't do this then it just contantly has disconnects

					//close the socket and mark as 0 for reuse
					close(sd);
					client_socket[i] = 0;
				// Should be echoing the message that just came in
				} else {
					//set the strung terminating NULL byte on the end of the data read
					buffer[valread] = '\0';
					send(sd, buffer, strlen(buffer), 0);
				}
			}
		}
	}

	return EXIT_SUCCESS;
}
