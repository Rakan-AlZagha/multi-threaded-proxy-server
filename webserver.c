/*
 * File name: webserver.c
 * Project: Server Assignment 8
 * CPSC 333
 * Author: @Rakan AlZagha
 * Comments: Used telnet format for client input!
 */
 
#define SERVER_PORT 9111
#define BUFFERSIZE 8192
 
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/file.h>


void handle_ctrlc(int sig);
void *get(void *file_descriptor);
int flock(int fd, int operation);
int control_socket, fd;
 
 
  /*
  * Function: main()
  * ----------------------------
  * Purpose: wait for connection from client and return appropriate messages
  *
  */
 
int main(int argc, char *argv[]) {
    struct sockaddr_in server_control_address; //socket address for server
    struct sockaddr_in client_address; //socket address for client
    char data_buffer[BUFFERSIZE]; //buffer of size 8192
    int i, n, file, bind_override = 1, address_size, error_check;
    char command[BUFFERSIZE], filename[50], filename_temp[50];
    char* success_message = "HTTP/1.1 200 OK\n\n";
    char* error_message = "HTTP/1.1 404 Not Found\n\n<html><head></head><html><body><h1>404 Not Found</h1></body></html>\n";
    char* command_error = "COMMAND NOT FOUND\n";
    char* client_command;
    char* file_token;
    int validCommand;
    int fd, *thread_sock;

    signal(SIGINT, handle_ctrlc);

    address_size = sizeof(client_address); //establishing the size of the address
 
    //socket structures set to null
    bzero(&client_address, sizeof(client_address));
    bzero(&server_control_address, sizeof(server_control_address));

    //control_socket implementation
    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(control_socket < 0){ //error checking
      perror("Failed to create socket, please try again!\n");
      exit(1);
    }
    if (setsockopt(control_socket, SOL_SOCKET, SO_REUSEADDR, &bind_override, sizeof(int)) < 0) //bind kept failing after repeated use, SO_REUSEADDR fixed the problem
      perror("Reusable socket port failed...try again.\n");
 
    //server ipv4, server port, and IP address
    server_control_address.sin_family = AF_INET;
    server_control_address.sin_port = htons(SERVER_PORT);
    server_control_address.sin_addr.s_addr = htonl(INADDR_ANY);
 
    //assign the address specified by addr to the socket referred to by the file descriptor
    error_check = bind(control_socket, (struct sockaddr *)&server_control_address, sizeof(server_control_address));
    if(error_check < 0){ //error checking
      perror("ERROR at bind function!\n");
      exit(1);
    }
 
    error_check = listen(control_socket, 100); //listen
    if(error_check < 0){ //error checking
      perror("ERROR at listen function!\n");
      exit(1);
    }
 
  while(fd = accept(control_socket, (struct sockaddr *)&client_address, &address_size)){ //continue taking input
    pthread_t t;
    bzero(command, sizeof(command));
    read(fd, command, sizeof(command));
    client_command = strtok(command, " "); //extract the command (first token)

    if(strcmp(client_command, "GET") == 0){
      validCommand = 0;
      thread_sock = malloc(sizeof(int));
      *thread_sock = fd;
      pthread_create(&t, NULL, get, thread_sock);
    }
 
    }
  }
 
  /*
  * Function: handle_ctrlc()
  * ----------------------------
  * Purpose: handle ctrl-c command
  *
  * int sig
  *
  */
 
  void handle_ctrlc(int sig) {
    fprintf(stderr, "\nCtrl-C pressed\n");
    close(control_socket);
    close(fd);
    exit(0);
  }
 
  /*
  * Function: get()
  * ----------------------------
  * Purpose: handle a GET command from proxy
  *
  * void *file_descriptor
  *
  */
 
  void *get(void *file_descriptor){
    char* file_token;
    char data_buffer[BUFFERSIZE]; //buffer of size 8192
    char filename[50], filename_temp[50];
    int i, n, file, lock;
    char* success_message = "HTTP/1.1 200 OK\n\n";
    char* error_message = "HTTP/1.1 404 Not Found\n\n<html><head></head><html><body><h1>404 Not Found</h1></body></html>\n";
 
    int fd = *(int*) file_descriptor;
    free(file_descriptor);
    printf("\nCommand accepted.\n");
    file_token = strtok(NULL, "/"); //extract the name of the file (second token)
    strcpy(filename_temp, file_token); //copy into char array from char * context
 
    for(i = 0; i < sizeof(filename_temp); i++){ //removing random characters and null from end of filename
      if(i == strlen(filename_temp) - 2) //there are 2 random character at the end of the filename
        break;
      filename[i] = filename_temp[i]; //copy temp to real filename
    }

    if((file = open(filename, O_RDONLY, S_IRUSR)) < 0){ //open local file to read
      write(fd, error_message, strlen(error_message)); //return error if file not found
      perror("ERROR in opening\n");
      close(fd);
    }

    else{
      write(fd, success_message, strlen(success_message));
    }

    lock = flock(file, LOCK_EX);
    if(lock != 0){
      perror("ERROR at LOCK!\n");
    }

    while((n = read(file, data_buffer, BUFFERSIZE)) > 0){ //read data in
      printf("%s", data_buffer);
      data_buffer[n] = ’\0’; //set buffer to null
      if(write(fd, data_buffer, n) < n){ //send data to the client
        perror("ERROR in writing\n");
        exit(1);
      }
      else{
        printf("HTML File sent successfully!\n");
      }
    }

    lock = flock(file, LOCK_UN);
    if(lock != 0){
      perror("ERROR at LOCK!\n");
    }

    close(fd);
    close(file); //close the file after extracting data
    printf("Closing connection");
    return NULL;
  }
