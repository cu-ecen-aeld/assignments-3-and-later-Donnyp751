#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>
#include <errno.h>
#include <arpa/inet.h>

#define SOCKET_PORT "9000"
#define SOCKET_RECV_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

volatile sig_atomic_t b_shutdown = 0;

void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        b_shutdown = 1;
    }
}

int main(int argc, char *argv[]) {
    int sockfd, new_fd;
    struct addrinfo hints = {0}, *res;
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int daemon_mode = 0;

    // Open syslog
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Starting server...");

    // Set up signal handlers
    struct sigaction sa = { .sa_handler = signal_handler };
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Check for daemon flag
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    // Set up address info
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, SOCKET_PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // Create socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow socket reuse
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Daemonize if requested
    if (daemon_mode && fork() != 0) {
        freeaddrinfo(res);
        close(sockfd);
        exit(EXIT_SUCCESS);
    }

    listen(sockfd, 10);
    syslog(LOG_INFO, "Listening on port %s", SOCKET_PORT);

    while (!b_shutdown) {
        new_fd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (new_fd == -1) {
            if (errno == EINTR) continue; // Interrupted by signal
            perror("accept");
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
        inet_ntop(AF_INET, &s->sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        // Receive data until newline
        FILE *fp = fopen(SOCKET_RECV_FILE, "a");
        if (!fp) {
            perror("fopen");
            close(new_fd);
            continue;
        }

        ssize_t recvd;
        size_t total = 0;
        do {
            recvd = recv(new_fd, buffer, BUFFER_SIZE, 0);
            if (recvd > 0) {
                fwrite(buffer, 1, recvd, fp);
                total += recvd;
                if (memchr(buffer, '\n', recvd)) break;
            }
        } while (recvd > 0);

        fclose(fp);

        // Send contents of file back to client
        fp = fopen(SOCKET_RECV_FILE, "r");
        if (!fp) {
            perror("fopen read");
            close(new_fd);
            continue;
        }

        while ((recvd = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
            send(new_fd, buffer, recvd, 0);
        }

        fclose(fp);
        close(new_fd);
    }

    // Cleanup
    syslog(LOG_INFO, "Shutting down server...");
    close(sockfd);
    freeaddrinfo(res);
    unlink(SOCKET_RECV_FILE);
    closelog();

    return 0;
}