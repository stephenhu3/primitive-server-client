#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include "tcp.h"
char* get_date();
void send_message(char *hostname, int port);
void append_newline(char *time_string);

#define LOCAL_PORT 8291;

int main(int argc, char *argv[]) {
	char *host = argv[1];
	int port = atoi(argv[2]);
	printf("Host name: %s| Port number: %d \n", host, port);

	send_message(host, port);
	return 1;
}

// given hostname and port number, creates a socket, binds it to a local port, and connects to the given machine and port
// returns the connected socket
// use tcp.h and tcp.c files to use their relevant functions from sample program talked about in class
// message to send looks like this:
// receive message
// send another message that was reply - 1, a space, and then string My name is Stephen Hu \n
// shutdown, pass argument SHUT_WR
void send_message(char *hostname, int port) {
	unsigned long long int *received = malloc(sizeof(long long));
	char second_received[256 * 1024];
	int soc_connected = tcp_connect(hostname, port);
	char *s = get_date();
	char second_string[256];
	printf("Message sent: %s", s);
	send(soc_connected, s, strlen(s), 0);

	recv(soc_connected, received, strlen(s), 0);
	printf("Received message: %llu\n", received);
	received--;
	sprintf(second_string, "%llu My name is Stephen Hu\n", received);
	printf("Message sent: %s", second_string);

	send(soc_connected, second_string, strlen(second_string), 0);
	shutdown(soc_connected, SHUT_WR); // disallows any more sending

	int size_recv;
	int total_size;
	int msg_flag = 1;

	while (1) {
		memset(second_received, 0, 256 * 1024);  //clear the variable
		if ((size_recv = recv(soc_connected, second_received, 1, 0)) < 1) {
			printf("End of message!\n");
			break;

		} else {
			total_size += size_recv;
			if (msg_flag == 1) {
				printf("Received message: "); // perform only once
			}
			msg_flag = 0;
			if (second_received[0] == '5') {
				printf("%llu", second_received);
			} else {

				printf("%s", second_received);
			}
		}
	}
}

char* get_date() {
	time_t time_raw;
	struct tm* time_local;
	time(&time_raw);
	time_local = localtime(&time_raw);

	return asctime(time_local);
}

void append_newline(char *time_string) {
	while (*time_string) {
		time_string++; // reach end of index for original string
	}

	*time_string = '\n';
	time_string++;
	*time_string = '\0';
}
