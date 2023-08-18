/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <bits/stdc++.h>

#include "http_server.cpp"

#define qsize 4096

pthread_t thread_pool[16];
int q[qsize], front = -1, rear = -1;
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
int sockfd, flag = 0;

void sig_handler(int signum)
{
    if(getpid() == gettid()) {
        close(sockfd);
        flag = 1;
        pthread_cond_broadcast(&cond1);
        for (int i = 0; i < 16; i++)
        {
            pthread_join(thread_pool[i], NULL);
        }
        exit(1);
    }
    flag = 1;
}

void error(char *msg)
{
    perror(msg);
}

void *start_thread(void *arg)
{
    signal(SIGINT, sig_handler);
    while (flag == 0)
    {
        pthread_mutex_lock(&lock1);
        while (front == -1 && flag == 0)
            pthread_cond_wait(&cond1, &lock1);
        int newsockfd = q[front];
        if (front == rear)
        {
            rear = -1;
            front = -1;
        }
        else
            front = (front + 1) % qsize;
        pthread_mutex_unlock(&lock1);
        char buffer[1024];
        int n;
        if(flag == 1)
            break;

        /* read message from client */
        bzero(buffer, 1024);
        n = read(newsockfd, buffer, 1023);
        if (n < 0){
            error("ERROR reading from socket");
            close(newsockfd);
            continue;
        }
        if (n == 0)
        {
            close(newsockfd);
            continue;
        }
        string str1(buffer);
        HTTP_Response *resp = handle_request(str1);
        string s = resp->get_string();
        const char *str = s.c_str();

        /* send reply to client */
        int m = strlen(str);
        n = write(newsockfd, str, m);
        if (n < 0){
            error("ERROR writing to socket");
            close(newsockfd);
            continue;
        }
        delete resp;
        close(newsockfd);
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    struct sigaction psa;
    psa.sa_flags = SA_RESTART;
    psa.sa_handler = sig_handler;
    sigaction(SIGINT, &psa, NULL);
    int newsockfd, portno, n_clients = 0;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    /* create socket */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        error("ERROR opening socket");
        exit(1);
    }
    /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
     */

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* bind socket to this port number on this machine */

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        error("ERROR on binding");
        exit(1);
    }
    for (int i = 0; i < 16; i++)
    {
        pthread_create((thread_pool + i), NULL, &start_thread, NULL);
    }

    listen(sockfd, 256);
    while (flag == 0)
    {
        /* listen for incoming connection requests */

        clilen = sizeof(cli_addr);

        /* accept a new request, create a newsockfd */
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0){
            error("ERROR on accept");
            continue;
        }
        pthread_mutex_lock(&lock1);
        if ((rear + 1) % qsize == front)
        {
            printf("queue is full\nDropping Packets\n");
            pthread_mutex_unlock(&lock1);
            continue;
        }
        if (front == -1)
            front = 0;
        q[(rear + 1) % qsize] = newsockfd;
        rear = (rear + 1) % qsize;
        pthread_cond_signal(&cond1);
        pthread_mutex_unlock(&lock1);
    }
    close(sockfd);
    return 0;
}
