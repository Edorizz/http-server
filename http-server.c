/* C library */
#include <stdio.h>
#include <string.h>
/* POSIX */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

const char resp_fmt[] = { "HTTP/1.0 %s\r\nContent-type: text/html\r\n\r\n" };

int
main(int argc, char **argv)
{
	struct addrinfo *res, hints;
	int sock_fd, client_fd, req_fd, ret;
	char buf[1024], *str_beg, *str_end, *req_nam;

	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((ret = getaddrinfo(NULL, "80", &hints, &res))) {
		fprintf(stderr, "%s\n", gai_strerror(ret));
		return 1;
	}

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("sock_fd");
		return 2;
	}

	ret = 1;
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof ret) < 0) {
		perror("setsockopt");
		return 3;
	}

	if (bind(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		return 4;
	}

	if (listen(sock_fd, 5) < 0) {
		perror("listen");
		return 5;
	}

	memset(buf, 0, sizeof buf);
	while ((client_fd = accept(sock_fd, NULL, NULL)) >= 0) {
		while ((ret = recv(client_fd, buf, sizeof buf - 1, 0)) > 0) {
			if ((str_end = strchr(buf, ' '))) {
				*str_end = '\0';
				if (strcmp(buf, "GET") == 0) {
					*str_end = ' ';

					req_nam = "index.html";
					str_beg = strchr(buf, '/') - 1;
					str_end = strchr(str_beg, ' ');

					*str_end = '\0';

					if (strlen(str_beg) > 0) {
						req_nam = str_beg;
					}

					fprintf(stderr, "requested for file: %s\n", req_nam);

					if ((req_fd = open(req_nam, O_RDONLY)) < 0) {
						perror("open");
						return 6;
					}

					sprintf(buf, resp_fmt, "200 OK");
					send(client_fd, buf, strlen(buf), 0);

					while ((ret = read(req_fd, buf, sizeof buf))) {
						send(client_fd, buf, ret, 0);
					}

				} else {
					fprintf(stderr, "unknown request: %s\n", buf);
				}
			}
		}
	}

	return 0;
}

