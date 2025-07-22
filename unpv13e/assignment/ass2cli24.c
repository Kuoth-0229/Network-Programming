#include	"unp.h"
#include 	<string.h>

void xchg_data(FILE *fp, int sockfd){

	int			maxfdp1;
    int         count = 0;
	fd_set		rset;
	char		sendline[MAXLINE], recvline[MAXLINE];
    char student_id[30] = "11550105";
    char local_ip[30];
    char id_ip[30];

    struct sockaddr_in local_addr;
    socklen_t local_addrlen;
    
    //get local addr
    local_addrlen = sizeof(local_addr);
    getsockname(sockfd , (SA*) &local_addr , &local_addrlen);
    char * ip_str = inet_ntoa(local_addr.sin_addr);

    //print id + ip
    strcpy (local_ip , ip_str);
    snprintf(id_ip , sizeof(id_ip), "%s %s\n", student_id , local_ip);
    printf("sent : %s" , id_ip);
    Writen(sockfd , id_ip , strlen(id_ip));


    FD_ZERO(&rset);
	for ( ; ; ) {
		FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		Select(maxfdp1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset)) {	/* socket is readable */
            Read(sockfd, recvline, MAXLINE);
            strcat(recvline,"\0");
			printf ("recv : %s\n" ,recvline);

            if (strncmp(recvline, "bad", 4) == 0)  {
                printf ("something wrong! try again.\n");
                count --;
            }
            else if (strncmp(recvline, "stop" , 5) == 0) {
                snprintf(sendline, sizeof(sendline), "%d %s\n",count , local_ip );
                Writen(sockfd, sendline, strlen(sendline));
                printf("sent line count & IP addr: %s", sendline);
            }
            else if (strncmp(recvline, "ok" , 3) == 0) {
                printf("success!\n");
                return;
            }
            else if (strncmp(recvline, "nak", 3) == 0) {
                // Server rejected the data, terminate with an error
                printf("Server rejected the data. Exiting with error.\n");
                return;
            }
            memset(recvline,0,sizeof(recvline));
		}

		if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
			if (Fgets(sendline, MAXLINE, fp) == NULL)
				return;		/* all done */
			Writen(sockfd, sendline, strlen(sendline));
            count ++;
		}
	}
}


int main(int argc, char **argv){
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 2)
		err_quit("usage: tcpcli <IPaddress>");

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET; //IPV 4
	servaddr.sin_port = htons(SERV_PORT + 1);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	xchg_data(stdin, sockfd);		/* do it all */

	exit(0);
}
