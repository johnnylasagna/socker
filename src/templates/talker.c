#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SERVERPORT "4950"

int main(int argc, char *argv[]) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;

    if (argc!=3) {
        fprintf(stderr, "Usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

    for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
		    -1) {
			perror("talker: socket");
			continue;
		}
        break;
    }

    if (p == NULL) {
		fprintf(stderr, "Talker: failed to bind socket\n");
		exit(1);
	}

    if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
    }

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
    close(sockfd);

    return 0;
}