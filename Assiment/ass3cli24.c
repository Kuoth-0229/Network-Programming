#include	"unp.h"
#include 	<string.h>

void xchg_data(FILE *fp, int sockfd){

	char		sendline[MAXLINE], recvline[MAXLINE];
    char student_id[30] = "111550105";
    char local_ip[30];
    char id_ip[30];

    struct sockaddr_in local_addr;
    socklen_t local_addrlen;
    
    memset(recvline,0,sizeof(recvline));
    //get local addr
    local_addrlen = sizeof(local_addr);
    getsockname(sockfd , (SA*) &local_addr , &local_addrlen);
    char * ip_str = inet_ntoa(local_addr.sin_addr);

    //print id + ip
    strcpy (local_ip , ip_str);
    snprintf(id_ip , sizeof(id_ip), "%s %s\n", student_id , local_ip);
    printf("sent : %s" , id_ip);
    Writen(sockfd , id_ip , strlen(id_ip));
    Read(sockfd, recvline, MAXLINE);
    strcat(recvline,"\0");
	printf ("recv : %s\n" ,recvline);

    struct hostent *hptr;
    char **pptr , str[INET6_ADDRSTRLEN];

    hptr = gethostbyname(recvline);
    pptr = hptr->h_addr_list;
    printf ("sent : %s\n",Inet_ntop(hptr->h_addrtype,*pptr,str,sizeof(str)));
    memset(recvline,0,sizeof(recvline));
    strcpy(sendline, str);
    Writen(sockfd , sendline , strlen(sendline));
    memset(recvline,0,sizeof(recvline));
    Read(sockfd, recvline, MAXLINE);
    strcat(recvline,"\0");
	printf ("recv : %s\n" ,recvline);

    if (strncmp(recvline, "bad", 4) == 0)  {
        printf ("something wrong! wrong addr \n");
        return;
    }
    else if (strncmp(recvline, "good", 5) == 0)  {
        printf("sent : %s" , id_ip);
        Writen(sockfd , id_ip , strlen(id_ip));
    }
    memset(recvline,0,sizeof(recvline));
    Read(sockfd, recvline, MAXLINE);
    strcat(recvline,"\0");
	printf ("recv : %s\n" ,recvline);
    if (strncmp(recvline, "ok" , 3) == 0){
        return;
    }
    else {
        printf("wrong here");
        return;
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
	servaddr.sin_port = htons(SERV_PORT + 2);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	xchg_data(stdin, sockfd);		/* do it all */

	exit(0);
}
