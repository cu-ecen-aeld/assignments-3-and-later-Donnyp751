#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <syslog.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "linkedlist.h"
#include "aesd_ioctl.h"

#define SOCKET_PORT "9000"
#define BUFFER_SIZE 1024
#define SEEKTO_PREFIX "AESDCHAR_IOCSEEKTO:"

#define USE_AESD_CHAR_DEVICE 1

#ifdef USE_AESD_CHAR_DEVICE
#define SOCKET_RECV_FILE "/dev/aesdchar"
#else
#define SOCKET_RECV_FILE "/var/tmp/aesdsocketdata"
#endif

volatile sig_atomic_t b_shutdown = 0;

void signal_handler(int signum)
{
    if (signum == SIGINT || signum == SIGTERM)
    {
        b_shutdown = 1;
    }
}

typedef struct
{
    FILE *rx_file; // logging fd
    pthread_mutex_t mutex;
} server_info_t;

typedef struct
{
    server_info_t server_info;
    int client_fd;
    struct sockaddr client_addr;
    socklen_t client_addr_len;
} thread_info_t;

int write_data_to_file(FILE *file, pthread_mutex_t *mutex, const char *data, size_t len)
{
    pthread_mutex_lock(mutex);
#ifdef USE_AESD_CHAR_DEVICE
    /* For char device, open/write/close each time to avoid holding reference */
    (void)file; /* unused when using char device */
    int fd = open(SOCKET_RECV_FILE, O_WRONLY | O_APPEND);
    if (fd < 0) {
        pthread_mutex_unlock(mutex);
        return -1;
    }
    ssize_t written = write(fd, data, len);
    close(fd);
#else
    size_t written = fwrite(data, 1, len, file);
    fflush(file);
#endif
    pthread_mutex_unlock(mutex);
    if ((size_t)written != len)
    {
        return -1;
    }
    return 0;
}

int log_time(server_info_t *server_info)
{

    while (!b_shutdown)
    {
        for(int delay = 0; delay < 10 && !b_shutdown; delay++)
        {
            sleep(1);
        }
        time_t now = time(NULL);
        if (now == (time_t)-1)
        {
            return -1;
        }

        struct tm *tm_info = localtime(&now);
        if (!tm_info)
        {
            return -1;
        }

        char time_buffer[64];
        size_t time_len = strftime(time_buffer, sizeof(time_buffer), "timestamp: %Y-%m-%d %H:%M:%S\n", tm_info);
        if (time_len == 0)
        {
            return -1;
        }

        if (write_data_to_file(server_info->rx_file, &server_info->mutex, time_buffer, time_len) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int on_connect(thread_info_t *thread_info)
{
    char client_ip[INET_ADDRSTRLEN];
    char buffer[BUFFER_SIZE];
    char *recv_buffer = NULL;
    size_t recv_buffer_size = 0;
    struct sockaddr_in *s = (struct sockaddr_in *)&thread_info->client_addr;

    inet_ntop(AF_INET, &s->sin_addr, client_ip, sizeof(client_ip));
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);

    ssize_t recvd;
    int found_newline = 0;

    /* Accumulate received data until newline */
    do
    {
        recvd = recv(thread_info->client_fd, buffer, BUFFER_SIZE, 0);
        if (recvd > 0)
        {
            char *new_recv_buffer = realloc(recv_buffer, recv_buffer_size + recvd + 1);
            if (!new_recv_buffer)
            {
                perror("realloc failed");
                free(recv_buffer);
                close(thread_info->client_fd);
                free(thread_info);
                return -1;
            }
            recv_buffer = new_recv_buffer;
            memcpy(recv_buffer + recv_buffer_size, buffer, recvd);
            recv_buffer_size += recvd;
            recv_buffer[recv_buffer_size] = '\0';

            if (memchr(buffer, '\n', recvd))
            {
                found_newline = 1;
                break;
            }
        }
    } while (recvd > 0);

    if (!found_newline || recv_buffer_size == 0)
    {
        free(recv_buffer);
        close(thread_info->client_fd);
        free(thread_info);
        return 0;
    }

    /* Check if this is an IOCSEEKTO command */
    syslog(LOG_DEBUG, "Checking command (len=%zu): first 30 chars='%.30s'", recv_buffer_size, recv_buffer);
    if (strncmp(recv_buffer, SEEKTO_PREFIX, strlen(SEEKTO_PREFIX)) == 0)
    {

        uint32_t write_cmd, write_cmd_offset;
        syslog(LOG_DEBUG, "Detected IOCSEEKTO command: %s", recv_buffer);
        if (sscanf(recv_buffer + strlen(SEEKTO_PREFIX), "%u,%u", &write_cmd, &write_cmd_offset) == 2)
        {
            syslog(LOG_DEBUG, "Parsed IOCSEEKTO: cmd=%u, offset=%u", write_cmd, write_cmd_offset);

            int fd = open(SOCKET_RECV_FILE, O_RDWR);
            if (fd >= 0)
            {
                struct aesd_seekto seekto = { .write_cmd = write_cmd, .write_cmd_offset = write_cmd_offset };
                int ioctl_ret = ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto);
                syslog(LOG_DEBUG, "ioctl returned %d, errno=%d", ioctl_ret, errno);
                if (ioctl_ret == 0)
                {
                    /* Check position after ioctl */
                    off_t pos = lseek(fd, 0, SEEK_CUR);
                    syslog(LOG_DEBUG, "Position after ioctl: %ld", (long)pos);

                    /* Read from seeked position and send back */
                    ssize_t bytes_read;
                    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
                    {
                        syslog(LOG_DEBUG, "Read %zd bytes", bytes_read);
                        send(thread_info->client_fd, buffer, bytes_read, 0);
                    }
                }
                close(fd);
            }
            else
            {
                syslog(LOG_ERR, "Failed to open %s for ioctl: %s", SOCKET_RECV_FILE, strerror(errno));
            }
        }
        else
        {
            syslog(LOG_ERR, "Failed to parse IOCSEEKTO parameters from: %s", recv_buffer);
        }
    }
    else
    {

        if (write_data_to_file(thread_info->server_info.rx_file, &thread_info->server_info.mutex, recv_buffer, recv_buffer_size) != 0)
        {
            perror("write to file failed");
            free(recv_buffer);
            close(thread_info->client_fd);
            free(thread_info);
            return -1;
        }


        FILE *fp = fopen(SOCKET_RECV_FILE, "r");
        if (!fp)
        {
            perror("fopen read");
            free(recv_buffer);
            close(thread_info->client_fd);
            free(thread_info);
            return -1;
        }

        while ((recvd = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
        {
            send(thread_info->client_fd, buffer, recvd, 0);
        }

        fclose(fp);
    }

    free(recv_buffer);
    close(thread_info->client_fd);
    free(thread_info);
    return 0;
}

int main(int argc, char *argv[])
{
    int sockfd, new_fd;
    struct addrinfo hints = {0}, *res;

    int daemon_mode = 0;

    // Open syslog
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "Starting server...");

    // Set up signal handlers
    struct sigaction sa = {.sa_handler = signal_handler};
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Check for daemon flag
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        daemon_mode = 1;
    }

    // Set up address info
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, SOCKET_PORT, &hints, &res) != 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // Create socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow socket reuse
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    int bind_ret = 0;
    for (int attempts = 0; attempts < 5; attempts++)
    {
        if ((bind_ret = bind(sockfd, res->ai_addr, res->ai_addrlen)) == 0)
        {
            break;
        }
        else
        {
            syslog(LOG_ERR, "Bind attempt %d failed: %s", attempts + 1, strerror(errno));
            sleep(1);
        }
    }

    if (bind_ret != 0)
    {
        perror("bind");
        close(sockfd);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    // Daemonize if requested
    if (daemon_mode && fork() != 0)
    {
        freeaddrinfo(res);
        close(sockfd);
        exit(EXIT_SUCCESS);
    }

    listen(sockfd, 10);
    syslog(LOG_INFO, "Listening on port %s", SOCKET_PORT);

    server_info_t server_info = {0};
    pthread_mutex_init(&server_info.mutex, NULL);


    linked_list_t *thread_list = ll_create();
    if (!thread_list)
    {
        perror("linked list creation failed");
        return -1;
    }


    while (!b_shutdown)
    {
        thread_info_t *thread_info = malloc(sizeof(thread_info_t));
        if (!thread_info)
        {
            perror("malloc failed");
            break;
        }
        thread_info->server_info = server_info;

        thread_info->client_addr_len = sizeof(thread_info->client_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&thread_info->client_addr, &thread_info->client_addr_len);
        if (new_fd == -1)
        {
            free(thread_info);
            if (errno == EINTR)
                continue; // Interrupted by signal
            perror("accept");
            break;
        }

        thread_info->client_fd = new_fd;
        pthread_t thread_id = 0;
        int ret = pthread_create(&thread_id, NULL, (void *(*)(void *))on_connect, (void *)thread_info);
        if (ret != 0)
        {
            perror("pthread_create failed");
            free(thread_info);
            break;
        }

        pthread_t *thread_id_ptr = malloc(sizeof(pthread_t));
        if (!thread_id_ptr)
        {
            perror("malloc failed");
            break;
        }
        *thread_id_ptr = thread_id;
        ll_push_back(thread_list, thread_id_ptr);
    }

    // Close all threads
    while (ll_size(thread_list) > 0)
    {
        pthread_t *thread_id = ll_pop_front(thread_list);
        pthread_join(*thread_id, NULL);
        free(thread_id);
    }

    
    // Cleanup
    syslog(LOG_INFO, "Shutting down server...");
    ll_destroy(thread_list, free);

    pthread_mutex_destroy(&server_info.mutex);
    close(sockfd);
    freeaddrinfo(res);

    closelog();

    return 0;
}