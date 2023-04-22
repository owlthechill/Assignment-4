#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <cstring>
#include <iostream>

#define PORT 8007

void handle_connect(int newsockfd);

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        /* error */
        std::cerr << "Failed to create socket" << std::endl;
    }
    bzero((void *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
    { /* error */
        std::cerr << "Failed to bind server to socket" << std::endl;
    }
    listen(sockfd, 5);
    for (;;)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd,
                           (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
        { /* error */
        std::cerr << "Failed to accept socket" << std::endl;
        }
        handle_connect(newsockfd);
    }
}


void handle_connect(int newsockfd)
{
    FILE *fp = fdopen(newsockfd, "r+");
    char msg[1024];
    fgets(msg, 1024, fp); /* should check for error here */
    fprintf(fp, "%s", msg);
    fclose(fp);
}