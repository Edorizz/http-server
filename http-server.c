/* C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* POSIX */
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZ		1024

#define DEFAULT_PAGE	"index.html"
#define NOT_FOUND	"404.html"

const char resp_fmt[] = { "HTTP/1.1 %s\r\nContent-Type: text/html\r\n\r\n" };

int
parse_req(int client_sock, char *buf, int buf_siz)
{
	char *nl, *status_code, *path_beg, *path_end, *time_str;
	int resource_fd, ret;
	time_t request_time;

	if ((nl = strchr(buf, '\n')) - buf <= buf_siz) {
		*nl = '\0';
		request_time = time(NULL);
		time_str = ctime(&request_time);
		nl = strchr(time_str, '\n');
		*nl = '\0';

		fprintf(stderr, "[%s]: %s\n", time_str, buf);

		if (strncmp(buf, "GET ", 4) == 0) {
			path_beg = strchr(buf, '/') + 1;
			path_end = strchr(path_beg, ' ');
			*path_end = '\0';

			if (strlen(path_beg) == 0) {
				path_beg = DEFAULT_PAGE;
			}

			status_code = "200 OK";

			if ((resource_fd = open(path_beg, O_RDONLY)) < 0) {
				status_code = "404 Not Found";
				resource_fd = open(NOT_FOUND, O_RDONLY);
			}
			
			sprintf(buf, resp_fmt, status_code);
			send(client_sock, buf, strlen(buf), 0);
			
			while ((ret = read(resource_fd, buf, buf_siz)) > 0) {
				send(client_sock, buf, ret, 0);
			}

			close(resource_fd);
		}

		return 0;
	}

	return -1;
}

int
main(int argc, char **argv)
{
	struct addrinfo *res, hints;
	int server_sock, client_sock, ret, buf_siz, buf_off;
	char *buf, *tmp;

	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((ret = getaddrinfo(NULL, "80", &hints, &res))) {
		fprintf(stderr, "%s\n", gai_strerror(ret));
		return 1;
	}

	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 2;
	}

	ret = 1;
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof ret) < 0) {
		perror("setsockopt");
		return 3;
	}

	if (bind(server_sock, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		return 4;
	}

	if (listen(server_sock, 5) < 0) {
		perror("listen");
		return 5;
	}

	chdir("web/");

	buf_siz = BUF_SIZ;
	buf = malloc(buf_siz);
	memset(buf, 0, buf_siz);

	while ((client_sock = accept(server_sock, NULL, NULL)) >= 0) {
		recv(client_sock, buf, buf_siz, 0);

		while ((buf_off = parse_req(client_sock, buf, buf_siz)) < 0) {
			tmp = malloc(buf_siz * 2);
			memcpy(tmp, buf, buf_siz);
			free(buf);
			buf = tmp;
		}

		close(client_sock);
	}

	return 0;
}

