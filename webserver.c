//order is now socket, bind, listen, accept, recv, send, close
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>

#define LOCAL_PORT 3490;

char* get_date();
void get_message(int port);
void append_newline(char *time_string);
void append_return(char *append_string);
void concatenate_string(char *first, char *second);
/*
 * Print error messages and exit
 */
extern void die0(const char *msg);
extern void diee(const char *msg);
extern void die1(const char *format, const char *msg);

extern int udp_open(int port);
extern struct sockaddr *get_sockaddr(const char *s, int port, struct sockaddr *saddr);
extern char *format_sockaddr(struct sockaddr *saddr, char *buffer);

/*
 * Connect to the given port on the given host, returning the connected
 * socket.  If there are any errors, print an error message and exit.
 */
int tcp_connect(int port);

int main(int argc, char *argv[]) {
	int port = atoi(argv[1]);
	printf("Port number: %d \n", port);
	get_message(port);
	return 1;
}

char *format_sockaddr(struct sockaddr *saddr, char *buffer) {
	struct sockaddr_in *sin = (struct sockaddr_in *) saddr;
	int ip, port;
	ip = ntohl(sin->sin_addr.s_addr);
	port = ntohs(sin->sin_port);
	sprintf(buffer, "<%u.%u.%u.%u, port %d>", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0XFF, ip & 0XFF, port);
	return buffer;
}

/*
 * Convert a DNS name or numeric IP address into an integer value
 * (in network byte order).  This is more general-purpose than
 * inet_addr() which maps dotted pair notation to uint.
 */

struct sockaddr* get_sockaddr(const char *s, int port, struct sockaddr *saddr) {
	struct sockaddr_in *sin = (struct sockaddr_in *) saddr;
	int ip;
	if (isdigit(*s)) {
		ip = (unsigned int) inet_addr(s);
	} else {
		struct hostent *hp = gethostbyname(s);
		if (hp == 0)
			die1("Can't translate %s to an address", s);
		ip = *((unsigned int **) hp->h_addr_list)[0];
	}
	memset((char *) sin, 0, sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
	sin->sin_addr.s_addr = ip;
	return saddr;
}

void die0(const char *msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

void die1(const char *format, const char *msg) {
	fprintf(stderr, format, msg);
	fprintf(stderr, "\n");
	exit(1);
}

void diee(const char *msg) {
	perror(msg);
	exit(1);
}

//order is now socket, bind, listen, accept, recv, send, close

/*
 * Connect to the given port on the given host, returning the connected
 * socket.  If there are any errors, print an error message and exit.
 */
int tcp_connect(int port) {

	struct sockaddr_in my_sock;
	struct sockaddr_in client_sock;

	int connected_socket = socket(AF_INET, SOCK_STREAM, 0);
	my_sock.sin_family = AF_INET;
	my_sock.sin_port = htons(port);
	my_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(my_sock.sin_zero), 8);

	int bind_check = bind(connected_socket, (struct sockaddr *) &my_sock, sizeof(struct sockaddr));
	int checkListen = listen(connected_socket, 10); // 10 concurrent users

	return connected_socket;
}

void get_message(int port) {
	int success;
	int size_recv;
	int total_size;
	int msg_flag = 1;
	char initial_response[1024];
	char continuous_response[1024];
	int soc_connected = tcp_connect(port);

	struct sockaddr_in client_sock;
	int document_type = 2; //0 for index.html, 1 for calculate.html

	char sendmsg[2048] = "HTTP/1.0 200 OK \r\n";
	char badrequest[2048] = "HTTP/1.0 400 Bad Request \r\n";
	char *the_time = get_date();
	char date[512];
	char server_header[512] = "Server: This is Stephen's server for CPSC 261 at UBC.\r\n";
	char content_header[512] = "Content-Type: text/html\r\n\r\n";
	char error_html[2048] =
			"<!DOCTYPE html>\n<html>\n<head>\n<title>Stephen's CPSC261 Server</title>\n</head>\n<body>\nError: This page cannot be found.\n</body>\n</html>";
	append_return(error_html);
	append_newline(error_html);
	append_return(error_html);
	append_newline(error_html);
	char index_html[2048] =
			"<!DOCTYPE html>\n<html>\n<head>\n<title>Stephen's CPSC261 Server</title>\n</head>\n<body>\nThis is a server created by Stephen Hu for CPSC 261 at UBC. \nCheck out the sick ASCII art below!\n  _____\n |_   _|\n   | |\n   | |\n  _| |\n |_____|\n          .-.  .-.\n         |   \\/   |\n         \\        /\n          `\\    /`\n            `\\/`\n                    //\"\"--.._\n                   ||  (x) o  \"-_\n                   || o  _    (x) '-.\n                   ||   (x)  o _..-'\n                    \\__..--\"\"\"\n\n</body>\n</html>";
	append_return(index_html);
	append_newline(index_html);
	append_return(index_html);
	append_newline(index_html);
	sprintf(date, "Date: %s", the_time);
	double first = 0;
	double second = 0;
	double result;
	char calculation[2048];
	while (1) {
		int size = sizeof(struct sockaddr_in);
		int new_soc;
		if ((new_soc = accept(soc_connected, (struct sockaddr *) &client_sock, &size)) == -1) {
			printf("Could not accept \n");
			continue;
		}
		while (1) {

			printf("Server: Connected to %s \n", inet_ntoa(client_sock.sin_addr));

			label: memset(initial_response, 0, 1024);  //clear the variable
			size_recv = recv(new_soc, initial_response, 50, 0);

			if (size_recv > 0) {
				total_size += size_recv;
				if (msg_flag == 1) {
					printf("Received message: \n"); // perform only once
				}
				msg_flag = 0;
				printf("%s", initial_response);
				if ((initial_response[14] == 'l') && (initial_response[13] == 'm') && (initial_response[12] == 't')
						&& (initial_response[11] == 'h') && (initial_response[10] == '.') && (initial_response[9] == 'x')
						&& (initial_response[8] == 'e') && (initial_response[7] == 'd') && (initial_response[6] == 'n')
						&& (initial_response[5] == 'i')) {
					success = 1;
					document_type = 0;

					while (size_recv != 0) {
						memset(initial_response, 0, 1024);
						size_recv = recv(new_soc, initial_response, 1, 0);

						printf("%s", initial_response);
						total_size += size_recv;
					}

				} // figure out how to check if calculate later
				else if ((initial_response[5] == 'c') && (initial_response[6] == 'a') && (initial_response[7] == 'l')
						&& (initial_response[8] == 'c') && (initial_response[9] == 'u') && (initial_response[10] == 'l')
						&& (initial_response[11] == 'a') && (initial_response[12] == 't') && (initial_response[13] == 'e')
						&& (initial_response[14] == '.') && (initial_response[15] == 'h') && (initial_response[16] == 't')
						&& (initial_response[17] == 'm') && (initial_response[18] == 'l') && (initial_response[19] == '?')) {
					success = 1;
					document_type = 1;
					int i = 1;
					int j = 0;

					while (size_recv != 0) {
						memset(continuous_response, 0, 1024);
						size_recv = recv(new_soc, continuous_response, 1, 0);

						printf("%s", continuous_response);
						total_size += size_recv;
					}

					double first = atof(&initial_response[20]);

					while ( isdigit(initial_response[19+i]) && ((19 + i) < 25)) {

						i++;
					}

					char operator = initial_response[19 + i];
					i++; // skips operator

					double second = atof(&initial_response[19 + i]);

					if (operator == '/') {
						result = first / second;
					} else if (operator == '*') {
						result = first * second;
					} else if (operator == '+') {
						result = first + second;
					} else if (operator == '-') {
						result = first - second;
					}

					sprintf(calculation,
							"<!DOCTYPE html>\n<html>\n<head>\n<title>Stephen's CPSC261 Server</title>\n</head>\n<body>\nCalculation: %.2lf %c %.2lf = %.2lf\n</body>\n</html>",
							first, operator, second, result);
					append_return(calculation);
					append_newline(calculation);
					append_return(calculation);
					append_newline(calculation);
					document_type = 1;
					goto label;

				} else {
					success = 0;
					document_type = -1;
					goto label2;
				}

			} else if (size_recv == 0) {
				goto label2;
			}

		}

		label2: while (1) {
			int size = sizeof(struct sockaddr_in);
			if (!fork()) {

				printf("\nMessage sent:\n");
				if (success == 1) {
					if (send(new_soc, sendmsg, strlen(sendmsg), 0) == -1)
						printf("Error encountered with sending.\n");
					printf("%s", sendmsg);
				} else if (success == 0) {
					if (send(new_soc, badrequest, strlen(badrequest), 0) == -1)
						printf("Error encountered with sending.\n");
					printf("%s", badrequest);
				}

				if (send(new_soc, date, strlen(date), 0) == -1)
					printf("Error encountered with sending.\n");
				printf("%s", date);

				if (send(new_soc, server_header, strlen(server_header), 0) == -1)
					printf("Error encountered with sending.\n");
				printf("%s", server_header);

				if (send(new_soc, content_header, strlen(content_header), 0) == -1)
					printf("Error encountered with sending.\n");
				printf("%s", content_header);

				if (document_type == 0) {
					if (send(new_soc, index_html, strlen(index_html), 0) == -1)
						printf("Error encountered with sending.\n");
					printf("%s", index_html);
				}

				if (document_type == 1) {
					if (send(new_soc, calculation, strlen(calculation), 0) == -1)
						printf("Error encountered with sending.\n");
					printf("%s", calculation);
				}

				if (document_type == -1) {
					if (send(new_soc, error_html, strlen(error_html), 0) == -1)
						printf("Error encountered with sending.\n");
					printf("%s", error_html);
				}

				close(new_soc);
				exit(0);
			}
			close(new_soc);
			while (waitpid(-1, NULL, WNOHANG) > 0)
				;
			break;
		}
		break;
	}

}

char* get_date() {
	time_t time_raw;
	struct tm* time_local;
	time(&time_raw);
	time_local = localtime(&time_raw);

	return asctime(time_local);
}

void append_return(char *time_string) {
	while (*time_string) {
		time_string++; // reach end of index for original string
	}

	*time_string = '\r';
	time_string++;
	*time_string = '\0';
}

void append_newline(char *time_string) {
	while (*time_string) {
		time_string++; // reach end of index for original string
	}

	*time_string = '\n';
	time_string++;
	*time_string = '\0';
}
