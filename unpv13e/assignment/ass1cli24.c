#include	"unp.h"
#include 	<string.h>

int gcd(int a , int b);
void xchg_data(FILE *fp, int sockfd);

// Function to implement the Euclidean algorithm
int gcd(int a, int b) {
    // Base case: if b is 0, gcd is a
    if (b == 0)
        return a;
    
    // Recursively call gcd with b and a % b
    return gcd(b, a % b);
}

void xchg_data(FILE *fp, int sockfd){
    char recvline[MAXLINE];
    char student_id[10] = "11550105\n";
    char *nums[MAXLINE];
    int a , b , result, n;
    char result_str[MAXLINE];
    
    //First send student id
    printf("sent : %s",student_id);
    Writen(sockfd, student_id, strlen(student_id));

    for (;;){
        if ((n = Readline(sockfd, recvline, MAXLINE)) == 0){
            err_quit("str_cli: server terminated prematurely\n");
        }
        //parse string
        char *argu = strtok(recvline," ");
		int i = 0;
        while (argu != NULL && i < 10) {
            nums[i++] = argu;
            argu = strtok(NULL, " ");
        }
        //parse number
        sscanf(nums[0], "%d" , &a);
        sscanf(nums[1], "%d" , &b);

        //print received numbers
        printf("recv : %d %d\n\n", a , b);

        //calculate result
        result = gcd( a , b);
        printf("sent : %d\n\n" , result);
        sprintf(result_str, "%d\n", result);
        Writen(sockfd, result_str, strlen(result_str));

        //Read ACK
        Readline(sockfd, recvline, MAXLINE);
        
        // Handle acknowledgment
        if (strcmp(recvline, "nak\n") == 0) {
			printf("recv : nak!\n");
            continue; 
        } else if (strcmp(recvline, "ok\n") == 0) {
			printf("recv : ok!\n");
            break; 
        }
        else {
			printf("\n=============error=============\n");
            continue;
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
	servaddr.sin_port = htons(SERV_PORT);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	xchg_data(stdin, sockfd);		/* do it all */

	exit(0);
}
