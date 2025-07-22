#include "unp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Signal handler for SIGCHLD to clean up terminated child processes
void handle_sigchld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0); // Reap zombie processes
    return;
}

void xchg_data(int client1_fd, int client2_fd, char *client1_name, char *client2_name,
               struct sockaddr_in *client1_addr, struct sockaddr_in *client2_addr) {
    fd_set read_fds;
    char buffer[MAXLINE], recv_buffer[MAXLINE], send_buffer[MAXLINE], temp_buffer[MAXLINE];
    int maxfdp1, client1_disconnected = 0, client2_disconnected = 0, bytes_read;
    time_t current_time;

    current_time = time(NULL);
    printf("\n%.24s: connected from %s, port %d and from %s, port %d.\n\n",
           ctime(&current_time),
           Inet_ntop(AF_INET, &client1_addr->sin_addr, buffer, sizeof(buffer)),
           ntohs(client1_addr->sin_port),
           Inet_ntop(AF_INET, &client2_addr->sin_addr, buffer, sizeof(buffer)),
           ntohs(client2_addr->sin_port));

    FD_ZERO(&read_fds);

    for (;;) {
        FD_SET(client1_fd, &read_fds);
        FD_SET(client2_fd, &read_fds);
        maxfdp1 = max(client1_fd, client2_fd) + 1;
        Select(maxfdp1, &read_fds, NULL, NULL, NULL);

        // Handle messages from Client 1
        if (FD_ISSET(client1_fd, &read_fds)) {
            bytes_read = Readline(client1_fd, recv_buffer, MAXLINE);
            if (bytes_read <= 0) { // Client 1 disconnected
                printf("Client 1 disconnected.\n");
                client1_disconnected = 1;
            } else {
                memset(send_buffer, 0, sizeof(send_buffer));
                strcpy(send_buffer, "(");
                strcat(send_buffer, client1_name);
                strcat(send_buffer, ") ");
                strcat(send_buffer, recv_buffer);
                strcat(send_buffer, "\n");

                memset(temp_buffer, 0, sizeof(temp_buffer));
                snprintf(temp_buffer, sizeof(temp_buffer), "(%s) \n\n", client1_name);

                if (strcmp(send_buffer, temp_buffer) != 0) {
                    Writen(client2_fd, send_buffer, strlen(send_buffer));
                    printf("Sent client1's message to client2.\n");
                }
            }
        }

        // Handle messages from Client 2
        if (FD_ISSET(client2_fd, &read_fds)) {
            bytes_read = Readline(client2_fd, recv_buffer, MAXLINE);
            if (bytes_read <= 0) { // Client 2 disconnected
                printf("Client 2 disconnected.\n");
                client2_disconnected = 1;
            } else {
                memset(send_buffer, 0, sizeof(send_buffer));
                strcpy(send_buffer, "(");
                strcat(send_buffer, client2_name);
                strcat(send_buffer, ") ");
                strcat(send_buffer, recv_buffer);
                strcat(send_buffer, "\n");

                memset(temp_buffer, 0, sizeof(temp_buffer));
                snprintf(temp_buffer, sizeof(temp_buffer), "(%s) \n\n", client2_name);

                if (strcmp(send_buffer, temp_buffer) != 0) {
                    Writen(client1_fd, send_buffer, strlen(send_buffer));
                    printf("Sent client2's message to client1.\n");
                }
            }
        }

        // Handle Client 1 disconnection
        if (client1_disconnected) {
            memset(send_buffer, 0, sizeof(send_buffer));
            strcpy(send_buffer, "(");
            strcat(send_buffer, client1_name);
            strcat(send_buffer, " left the room. Press Ctrl + D to leave.)\n");
            Writen(client2_fd, send_buffer, strlen(send_buffer));

            shutdown(client2_fd, SHUT_WR);
            Readn(client2_fd, recv_buffer, MAXLINE);

            memset(send_buffer, 0, sizeof(send_buffer));
            strcpy(send_buffer, "(");
            strcat(send_buffer, client2_name);
            strcat(send_buffer, " left the room.)\n");
            Writen(client1_fd, send_buffer, strlen(send_buffer));

            shutdown(client1_fd, SHUT_WR); // send FIN to client 1
            Close(client2_fd);
            Close(client1_fd);
            break;
        }

        // Handle Client 2 disconnection
        if (client2_disconnected) {
            memset(send_buffer, 0, sizeof(send_buffer));
            strcpy(send_buffer, "(");
            strcat(send_buffer, client2_name);
            strcat(send_buffer, " left the room. Press Ctrl + D to leave.)\n");
            Writen(client1_fd, send_buffer, strlen(send_buffer));

            shutdown(client1_fd, SHUT_WR);
            Readn(client1_fd, recv_buffer, MAXLINE);

            memset(send_buffer, 0, sizeof(send_buffer));
            strcpy(send_buffer, "(");
            strcat(send_buffer, client1_name);
            strcat(send_buffer, " left the room.)\n");
            Writen(client2_fd, send_buffer, strlen(send_buffer));

            shutdown(client2_fd, SHUT_WR);
            Close(client2_fd);
            Close(client1_fd);
            break;
        }
    }
}

int main(int argc, char **argv) {
    int server_fd, client1_fd, client2_fd, bytes_read, connected_clients;
    socklen_t client1_addr_len, client2_addr_len;
    struct sockaddr_in client1_addr, client2_addr, server_addr;
    char client1_name[20], client2_name[20];
    char message_to_client1[100] = "You are the 1st user. Wait for the second one!\n";
    char message_to_client2[100] = "You are the 2nd user.\n";

    // Set up TCP server
    server_fd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERV_PORT + 4);
    Bind(server_fd, (SA *) &server_addr, sizeof(server_addr));
    Listen(server_fd, LISTENQ);
    Signal(SIGCHLD, handle_sigchld); // Register signal handler

    for (;;) {
        // Wait for two clients to connect
        connected_clients = 0;
        while (connected_clients != 2) {
            if (connected_clients == 0) { // First client
                client1_addr_len = sizeof(client1_addr);
                if ((client1_fd = accept(server_fd, (SA *) &client1_addr, &client1_addr_len)) < 0) {
                    if (errno == EINTR) continue; // Interrupted, try again
                    else err_sys("accept error");
                }
                printf("Client1 connected.\n");
                bytes_read = Read(client1_fd, client1_name, 20);
                client1_name[bytes_read] = '\0'; // Null-terminate the name
                printf("Received: %s\n", client1_name);
                Writen(client1_fd, message_to_client1, strlen(message_to_client1));
                connected_clients++;
            } else if (connected_clients == 1) { // Second client
                client2_addr_len = sizeof(client2_addr);
                if ((client2_fd = accept(server_fd, (SA *) &client2_addr, &client2_addr_len)) < 0) {
                    if (errno == EINTR) continue; // Interrupted, try again
                    else err_sys("accept error");
                }
                printf("Client2 connected.\n");
                bytes_read = Read(client2_fd, client2_name, 20);
                client2_name[bytes_read] = '\0'; // Null-terminate the name
                printf("Received: %s\n", client2_name);
                Writen(client2_fd, message_to_client2, strlen(message_to_client2));
                connected_clients++;
            }
        }

        // Inform both clients about the other
        char buffer[MAXLINE], send_buffer[MAXLINE];
        snprintf(send_buffer, sizeof(send_buffer), "The second user is %s from %s\n", client2_name,
                 Inet_ntop(AF_INET, (SA *) &client2_addr.sin_addr, buffer, sizeof(buffer)));
        Writen(client1_fd, send_buffer, strlen(send_buffer));

        snprintf(send_buffer, sizeof(send_buffer), "The first user is %s from %s\n", client1_name,
                 Inet_ntop(AF_INET, (SA *) &client1_addr.sin_addr, buffer, sizeof(buffer)));
        Writen(client2_fd, send_buffer, strlen(send_buffer));

        // Handle communication between clients using xchg_data
        if (Fork() == 0) { // Child process
            Close(server_fd); // Close the listening socket in the child
            xchg_data(client1_fd, client2_fd, client1_name, client2_name, &client1_addr, &client2_addr);
            exit(0);
        }

        Close(client1_fd); // Parent closes connected sockets
        Close(client2_fd);
    }

    return 0;
}
