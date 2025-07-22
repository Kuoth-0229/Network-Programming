#include "unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

// Helper function to convert integer to string
void int_to_str(int num, char *str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    char temp[30];
    int i = 0, j = 0;
    
    while (num > 0) {
        temp[i++] = num % 10 + '0';
        num /= 10;
    }
    
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

// Handle new client connection
void handle_new_client(int *online_count, int *total_count, int connfd[], 
                      char names[][500], fd_set *master_set, int listenfd, 
                      struct sockaddr_in *cliaddr, socklen_t *clilen) {
    char sendline[MAXLINE];
    char num_str[30];
    int n;

    printf("\nClient %d enter.\n", *online_count);
    (*total_count)++;
    (*online_count)++;
    *clilen = sizeof(*cliaddr);
    connfd[*total_count] = Accept(listenfd, (SA *)cliaddr, clilen);
    FD_SET(connfd[*total_count], master_set);

    n = Read(connfd[*total_count], names[*total_count], 500);
    names[*total_count][n] = '\0';
    printf("Recv: %s\n", names[*total_count]);
    
    strcpy(sendline, "You are the #");
    int_to_str(*online_count, num_str);
    strcat(sendline, num_str);
    strcat(sendline, " user.\n");
    Writen(connfd[*total_count], sendline, strlen(sendline));
    
    strcpy(sendline, "You may now type in or wait for other users.\n");
    Writen(connfd[*total_count], sendline, strlen(sendline));
    
    printf("Sent: %s is the #%d user.\n", names[*total_count], *online_count);
}

// Broadcast new user entry to existing users
void broadcast_new_user(int total_count, int online_count, int connfd[], 
                       char names[][500], int left[]) {
    char sendline[MAXLINE];
    char num_str[30];

    for (int i = 1; i < total_count; i++) {
        if (!left[i]) {
            strcpy(sendline, "(#");
            int_to_str(online_count, num_str);
            strcat(sendline, num_str);
            strcat(sendline, " user ");
            strcat(sendline, names[total_count]);
            strcat(sendline, " enters.)\n");
            Writen(connfd[i], sendline, strlen(sendline));
        }
    }
}

// Handle client disconnect
void handle_disconnect(int client_id, int *online_count, int total_count, 
                      int connfd[], char names[][500], int left[], 
                      fd_set *master_set) {
    char sendline[MAXLINE];
    char num_str[30];

    printf("\nClient %d left.\n", client_id);
    (*online_count)--;

    strcpy(sendline, "Bye!\n");
    Writen(connfd[client_id], sendline, strlen(sendline));

    switch(*online_count) {
        case 0:
            printf("No one left.\n");
            break;
            
        case 1:
            strcpy(sendline, "(");
            strcat(sendline, names[client_id]);
            strcat(sendline, " left the room. You are the last one. ");
            strcat(sendline, "Press Ctrl+D to leave or wait for a new user.)\n");
            
            for (int j = 1; j <= total_count; j++) {
                if (j != client_id && !left[j]) 
                    Writen(connfd[j], sendline, strlen(sendline));
            }
            break;
            
        default:  // 2 or more users
            strcpy(sendline, "(");
            strcat(sendline, names[client_id]);
            strcat(sendline, " left the room. ");
            int_to_str(*online_count, num_str);
            strcat(sendline, num_str);
            strcat(sendline, " users left.)\n");
            
            for (int j = 1; j <= total_count; j++) {
                if (j != client_id && !left[j]) 
                    Writen(connfd[j], sendline, strlen(sendline));
            }
            break;
    }

    left[client_id] = 1;
    FD_CLR(connfd[client_id], master_set);
    shutdown(connfd[client_id], SHUT_WR);
    Close(connfd[client_id]);
}

// Handle client message
void handle_client_message(int client_id, int total_count, int connfd[], 
                          char names[][500], int left[], char *recvline) {
    char sendline[MAXLINE];
    
    printf("Receiving %d. connfd = %d\n", client_id, connfd[client_id]);
    strcpy(sendline, "(");
    strcat(sendline, names[client_id]);
    strcat(sendline, ") ");
    strcat(sendline, recvline);
    
    printf("Sent %d's message\n", client_id);
    for (int j = 1; j <= total_count; j++) {
        if (j != client_id && !left[j]) 
            Writen(connfd[j], sendline, strlen(sendline));
    }
}

// Signal handler for SIGCHLD
void handle_sigchld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    return;
}

int main(int argc, char **argv) {
    int listenfd, n, online_count, total_count, maxfdp1;
    int connfd[500], left[500] = {0};
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char names[500][500], recvline[MAXLINE];
    fd_set rset, master_set;

    // Setup TCP server
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT + 5);
    Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);
    Signal(SIGCHLD, handle_sigchld);

    // Initialize variables
    online_count = 0;
    total_count = 0;
    FD_ZERO(&master_set);
    FD_SET(listenfd, &master_set);
    maxfdp1 = listenfd + 1;
    
    for ( ; ; ) {
        rset = master_set;
        select(maxfdp1, &rset, NULL, NULL, NULL);
        
        if (FD_ISSET(listenfd, &rset) && online_count < 10) {
            handle_new_client(&online_count, &total_count, connfd, names, 
                            &master_set, listenfd, &cliaddr, &clilen);
            broadcast_new_user(total_count, online_count, connfd, names, left);
            
            if (connfd[total_count] + 1 > maxfdp1)
                maxfdp1 = connfd[total_count] + 1;
        }

        for (int i = 1; i <= total_count; i++) {
            if (left[i]) continue;
            if (FD_ISSET(connfd[i], &rset)) {
                n = Readline(connfd[i], recvline, MAXLINE);
                
                switch(n) {
                    case -1:
                    case 0:
                        handle_disconnect(i, &online_count, total_count, connfd, 
                                       names, left, &master_set);
                        break;
                        
                    case 1:
                        for (int j = 1; j <= total_count; j++) {
                            if (j != i && !left[j]) 
                                Writen(connfd[j], "\n", 1);
                        }
                        break;
                        
                    default:
                        handle_client_message(i, total_count, connfd, names, 
                                           left, recvline);
                        break;
                }
            }
        }
    }
    return 0;
}