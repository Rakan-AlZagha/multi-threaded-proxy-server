/*
 * File name: proxyserver.c
 * Project: HTML Server Assignment 8
 * CPSC 333
 * Author: @Rakan AlZagha
 * Comments: Used telnet format for client input!
 */

#define CLIENT_PORT 9555 //fixed port error
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
#include <dirent.h>
#include <pthread.h>
#include <sys/file.h>

void handle_ctrlc(int sig);
void *get(void *file_descriptor);
int flock(int fd, int operation);
int control_socket, data_socket, fileDescriptor;
char* client_command;

/*
 *   Function: main()
 *   ----------------------------
 *   Purpose: wait for connection from client and return appropriate messages from original server
 *
 */

int main(int argc, char *argv[]) {
    struct sockaddr_in server_control_address; //socket address for server
    struct sockaddr_in client_address; //socket address for client
    struct hostent *hostConvert;       //convert hostname to IP
    struct sockaddr_in server_data_address;
    struct dirent *directories;
    char data_buffer[BUFFERSIZE];            //buffer of size 8192
    int i, n, file, bind_override = 1, address_size, error_check;
    int *thread_sock;
    int threadCount = 0, validCommand = -1;
    char command[50];

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
    server_control_address.sin_port = htons(CLIENT_PORT);
    server_control_address.sin_addr.s_addr = htonl(INADDR_ANY);

    //assign the address specified by addr to the socket referred to by the file descriptor
    error_check = bind(control_socket, (struct sockaddr *)&server_control_address, sizeof(server_control_address));
    if(error_check < 0){ //error checking
        perror("ERROR at bind function!\n");
        exit(1);
    }

    error_check = listen(control_socket, 100); //listen, specified 100 calls (requests)
    if(error_check < 0){ //error checking
        perror("ERROR at listen function!\n");
        exit(1);
    }

    while(fileDescriptor = accept(control_socket, (struct sockaddr *)&client_address, &address_size)){ //keep accepting commands/requests
        pthread_t t;
        threadCount = threadCount + 1; //how many threads/requests
        printf("Accepted connection from client.\n.\n.\n.");
        bzero(command, sizeof(command)); //set command to null
        read(fileDescriptor, command, sizeof(command)); //read the command from the client
        client_command = strtok(command, " "); //extract the command (first token)

        if(strcmp(client_command, "GET") == 0){ //get COMMAND
            validCommand = 0;
            thread_sock = malloc(sizeof(int));
            *thread_sock = fileDescriptor;
            if(pthread_create(&t, NULL, get, thread_sock) < 0){ //create a thread for each process/request
                perror("ERROR at thread!\n");
                continue;
            } //thread for each incoming request
            printf("\nThread Number: %d\n", threadCount);
        }

        else{
            printf("\nCommand unknown, please try again!\n");
            continue;
        }

        if(fileDescriptor < 0){
            perror("ERROR AT ACCEPT\n");
        }
        pthread_join(t, NULL); //to not terminate pre-maturely (middle of request)
    }
}

/*
 *   Function: handle_ctrlc()
 *   ----------------------------
 *   Purpose: handle ctrl-c command
 *
 *   int sig
 *
 */

void handle_ctrlc(int sig) {
   fprintf(stderr, "\nCtrl-C pressed\n");
   close(data_socket);
   close(control_socket);
   close(fileDescriptor);
   exit(0);
}

/*
 *   Function: get()
 *   ----------------------------
 *   Purpose: handle a GET command from client
 *
 *   void *file_descriptor
 *
 */

void *get(void *file_descriptor){
    struct dirent *directories;
    struct hostent *hostConvert;       //convert hostname to IP
    struct sockaddr_in server_data_address;
    char filename[50], filename_temp[50], command[50];
    char fileNameTemp[255];
    char data_buffer[BUFFERSIZE];            //buffer of size 8192
    int i, n, file, lock, error_check, fd, port, data_address_size, fileInCache = 0;
    char* success_message = "HTTP/1.1 200 OK\n\n";
    char* error_message = "HTTP/1.1 404 Not Found\n\n<html><head></head><html><body><h1>404 Not Found</h1></body></html>\n";
    char* command_error = "COMMAND NOT FOUND\n";
    char* ip;
    char* hostname;
    char* file_token;
    char* host_token;
    char* port_token;

    fd = *(int *) file_descriptor;
    free(file_descriptor); //free the fd (old, made copy of it after casting to int)

    bzero(filename, sizeof(filename)); //set filename to null

    printf("\nCommand accepted.\n");
    host_token = strtok(NULL, "/:"); //extract the name of the host (2nd token)
    port_token = strtok(NULL, ":/"); //extract the port of the host (3rd token)
    file_token = strtok(NULL, "/"); //extract the name of the file (4th token)
    strcpy(filename_temp, file_token); //copy into char array from char * context
    for(i = 0; i < sizeof(filename_temp); i++){ //removing random characters and null from end of filename
        if(i == strlen(filename_temp) - 2) //there are 2 random character at the end of the filename
            break;
        filename[i] = filename_temp[i]; //copy temp to real filename
    }
    
    DIR *directory_stream = opendir("."); //"." signifies the current directory

    if (directory_stream != NULL) {
        while ((directories = readdir(directory_stream)) != NULL){ //while there are more filenames to read
            strncpy(fileNameTemp, directories->d_name, 254); //copy the name of the file into the struc
            fileNameTemp[254] = '\0'; //nullify the filename (clear)
            if(strncmp(fileNameTemp, filename, sizeof(fileNameTemp)) == 0){ //if we encounter the filename
                fileInCache = 1; //return 1 (true)
                break;
            }
            else{
                fileInCache = 0; //return 0 (false)
            }
        }
        closedir(directory_stream); //close the directory that was opened
    }

    else { // directory_stream == NULL
        printf("Could not open current directory" );
        return 0;
    }

    if(fileInCache == 1) { //file found in cache
        printf("File found in cache.\n");
        if((file = open(filename, O_RDONLY, S_IRUSR)) < 0){ //open local file to read
            write(fd, error_message, strlen(error_message)); //return error if file not found
            perror("ERROR in opening\n");
            close(fd);
        }

        //lock the file once open
        lock = flock(file, LOCK_EX);
        if(lock != 0){
            perror("ERROR at LOCK!\n");
        }

        //write the success message to the client
        write(fd, success_message, strlen(success_message));

        // write the contents of the file
        while((n = read(file, data_buffer, BUFFERSIZE)) > 0){ //read local file
            data_buffer[n] = '\0'; //set buffer to NULL prior to writing
            if(write(fd, data_buffer, n) < n){ //write date over to the client
                perror("ERROR in writing\n");
                exit(1);
            }
            else{
                printf("HTML File sent successfully!\n");
            }
        }

        //unlock file once done
        lock = flock(file, LOCK_UN);
        if(lock != 0){
            perror("ERROR at LOCK!\n");
        }

        close(fd);
        close(file); //close the file after extracting data
        printf("\nClosing connection...\n");
        return NULL;
    }

    else {
        bzero(&server_data_address, sizeof(server_data_address));
        data_address_size = sizeof(server_data_address);
        hostname = host_token; //extracting the name of the host we are connecting to (first argument)

        hostConvert = gethostbyname(hostname); //converting name to host address
        if(hostConvert == NULL){ //error checking that host is valid (ex. syslab001... would error)
            printf("Not a valid hostname. Please try again.\n");
            exit(1);
        }

        //ip info
        ip = inet_ntoa(*(struct in_addr *) *hostConvert->h_addr_list); //converts host address to a string in dot notation

        //create socket for server data transfer
        data_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(data_socket < 0){ //error checking
            perror("Failed to create data socket, please try again!\n");
            exit(1);
        }

        port = atoi(port_token);

        //connecting to original server specifications
        server_data_address.sin_family = AF_INET;
        server_data_address.sin_port = htons(port);
        server_data_address.sin_addr.s_addr = inet_addr(ip);

        //connect to server
        error_check = connect(data_socket, (struct sockaddr *)&server_data_address , data_address_size);
        if (error_check != 0){ //error checking
            perror("ERROR in connect function\n");
            exit(1);
        }
        else{
            printf("Successfully connected to original server.\n");
        }

        //combining the client command and filename
        char* server_command = (char *) malloc(1 + strlen(client_command) + strlen(file_token) + strlen(" /"));
        strcpy(server_command, client_command);
        strcat(server_command, " /");
        strcat(server_command, file_token);

        //writing to server the command
        write(data_socket, server_command, sizeof(command));

        //open the file and create it
        if((file = open(filename, O_WRONLY|O_CREAT, S_IRWXU)) < 0){ //create and or write privelages
            perror("ERROR in opening\n");
            exit(1);
        }

        //lock the file once open
        lock = flock(file, LOCK_EX);
        if(lock != 0){
            perror("ERROR at LOCK!\n");
        }

        //read the file and copy it and send it
        while((n = read(data_socket, data_buffer, BUFFERSIZE)) > 0){ //read data
            data_buffer[n] = '\0';
            write(file, data_buffer, n); //write to the file
            write(fd, data_buffer, n); //write to the socket (fd)
        }

        //unlock the file once done
        lock = flock(file, LOCK_UN);
        if(lock != 0){
            perror("ERROR at LOCK!\n");
        }

        printf("\nClosing connection...\n\n");
        close(fd);
        close(file); //close the file after extracting data
        close(data_socket); //close data socket
        return NULL;
    }
}
