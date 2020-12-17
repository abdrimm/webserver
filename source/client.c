#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_CONNECT
};
 
int init_socket(const char *ip, int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent *host = gethostbyname(ip);
    struct sockaddr_in server_address;
 
    //open socket, result is socket descriptor
    if (server_socket < 0) {
        perror("Fail: open socket");
        exit(ERR_SOCKET);
    }
 
    //prepare server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    memcpy(&server_address.sin_addr, host -> h_addr_list[0],
           (socklen_t) sizeof server_address.sin_addr);
 
    //connection
    if (connect(server_socket, (struct sockaddr*) &server_address,
        (socklen_t) sizeof server_address) < 0) {
        perror("Fail: connect");
        exit(ERR_CONNECT);
    }
    return server_socket;
}
 
int read_string(int fd, char **string) {
    char *str = NULL;
    char ch;
    int i = 0, value;
    while (1) {
        value = read(fd, &ch, sizeof(char)); 
        if(value <= 0) {
            break;
        }
        if(ch == '\r') {
            break;
        }
        if(ch == '\n') {
            continue;
        }
        str = realloc(str, (i + 1) * sizeof(char));
        str[i] = ch;
        i++;
    }
    str = realloc(str, (i + 1) * sizeof(char));
    str[i] = '\0';
    *string = str;
    return value;
} 

void write_string(int fd, char *string) {
    int i = 0;
    char ch;
    while (string[i] != '\0') {
        write(fd, &string[i], 1);
        i++;
    }
    write(fd, &string[i], 1);
}
 
//  http://127.0.0.1:5000/resource/file.html

int main(void) {
    char ch;
    int i = 0;
    int server;
    char request[256];
    char *arr[6] = {"GET", " ", "HTTP/1.1\r", "Host:", " ", "\r"};
    char *str[6] = {NULL};
    int len = 0;
    char  *strtok_ans = NULL;
    char *str1 = "HTTP/1.1 404\n";
    char *str2 = "HTTP/1.1\n";
    char *string = NULL;
    char *str3 = "http://";
    char *request_array[4];
    fgets(request, 256, stdin);
    while (strcmp(request, "exit\n") != 0) {
        i = 0;
        while(strstr(request, str3) == NULL && (strcmp(request, "exit\n") != 0)) {
            puts("Enter correct request");
            fgets(request, 256, stdin);
        }
        strtok_ans = strtok(request, ":/");
        while((strtok_ans != NULL) && (i != 4)) {
            request_array[i] = strtok_ans;
            
            if (i == 2) {
                strtok_ans = strtok(NULL, "\n");
            } else {
                strtok_ans = strtok(NULL, ":/");
            }
            i++;
        }
        arr[1] = request_array[3];
        arr[4] = request_array[1];
        server = init_socket(request_array[1], atoi(request_array[2]));
        for(int i = 0; i < 6; i++) {
            write_string(server, arr[i]);
            puts(arr[i]);
        }
        read_string(server, &str[0]);
        if ((strcmp(str[0], str1) != 0) && (strcmp(str[0], str2) != 0)) {
            read_string(server, &str[1]);
            read_string(server, &str[2]);
            read_string(server, &str[3]);
            read_string(server, &str[4]);
            read(server, &len, sizeof(int));
            read_string(server, &str[5]);
            read_string(server, &string);
            printf("%s%s%s%s%s%d%s%s\n", str[0], str[1], str[2], str[3], str[4], len, str[5], string);
        } else {
            printf("%s\n", str[0]);
        }
        arr[1] = NULL;
        arr[4] = NULL;
        fgets(request, 256, stdin);
    }
    server = init_socket(request_array[1], atoi(request_array[2]));
    close(server);
    return OK;
}
