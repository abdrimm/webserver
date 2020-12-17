#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/sendfile.h>
 
enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_SETSOCKETOPT,
    ERR_BIND,
    ERR_LISTEN,
};

 
int init_socket(int port) {
    int server_socket, socket_option = 1;
    struct sockaddr_in server_address;
 
    //open socket, return socket descriptor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Fail: open socket");
        exit(ERR_SOCKET);
    }
 
    //set socket option
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_option, (socklen_t) sizeof socket_option);
    if (server_socket < 0) {
        perror("Fail: set socket options");
        exit(ERR_SETSOCKETOPT);
    }
 
    //set socket address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *) &server_address, (socklen_t) sizeof server_address) < 0) {
        perror("Fail: bind socket address");
        exit(ERR_BIND);
    }
 
    //listen mode start
    if (listen(server_socket, 5) < 0) {
        perror("Fail: bind socket address");
        exit(ERR_LISTEN);
    }
    return server_socket;
}
 
int isEndLine(char ch) {
    return ch == '\r';
}

int isEmptyChar(char ch) {
    return ch == '\n';
}

int readStr(int fd, char **string) {
    char *str = NULL;
    char ch;
    int i = 0, status;
    while (1) {
        status = read(fd, &ch, sizeof(char)); 
        if(status <= 0) {
            break;
        }
        if(isEndLine(ch)) {
            break;
        }
        if(isEmptyChar(ch)) {
            continue;
        }
        str = realloc(str, (i + 1) * sizeof(char));
        str[i] = ch;
        i++;
    }
    str = realloc(str, (i + 1) * sizeof(char));
    str[i] = '\0';
    *string = str;
    return status;
} 

int get_file_size(int fd) {
    struct stat stat_struct;
    if (fstat(fd, &stat_struct) == -1)
        return (1);
    return (int) stat_struct.st_size;
}

int run_binary(char *path, int client_socket, char **argv) {
    if (fork() == 0) {
        dup2(client_socket, 1);
        close(client_socket);
        if(execv(path, argv) < 0) {
            perror("exec error");
            exit(1);
        }
        exit(0);
    }
    wait(NULL);
    return 0;
}

char** split(char *str, char *delimiter) {
    int i = 0;
    char  *strtokAnswer = NULL;
    char **words = NULL;
    strtokAnswer = strtok(str, delimiter);
    while(strtokAnswer != NULL) {
        words = realloc(words, (i + 1) * sizeof(char *));
        words[i] = strtokAnswer;
        puts(words[i]);
        strtokAnswer = strtok(NULL, delimiter);
        i++;
    }
    words = realloc(words, (i + 1) * sizeof(char *));
    words[i] = NULL;
    return words;
}

void sendHeader(int client_socket, char *type, int len) {
    char *str3 = "HTTP/1.1 200\r\n";
    char *str1 = "Сontent-type: ";
    char *str2 = "Сontent-length: ";
    char str4[10];
    snprintf(str4, 10, "%d", len);
    write(client_socket, str3, strlen(str3) * sizeof(char));
    write(client_socket, str1, strlen(str1) * sizeof(char));   
    write(client_socket, type, strlen(type) * sizeof(char));
    write(client_socket, "\r\n", 2 * sizeof(char));
    write(client_socket, str2, strlen(str2) * sizeof(char));
    write(client_socket, str4, strlen(str4) * sizeof(char));
    write(client_socket, "\r\n\r\n", 4 * sizeof(char));
}

int requestAnswer(char *arr[], int client_socket, char *postStr) {
    struct stat stat;
    char *del = "=&";
    char *error = "<html><h1>Error 404<h1><html>\r\n";
    char *str1 = "HTTP/1.1 404\r\n\r\n";
    char *str2 = "HTTP/1.1";
    char *str3 = "text/html";
    char *str4 = "image/png";
    char *str5 = "image/jpg";
    char *str6 = "image/gif";
    char *str7 = "text/css";
    char *str8 = "video/mp4";
    char **argv = NULL;
    char *type = NULL;
    char *string = NULL;
    char *query = NULL;
    char **queryArr = NULL;
    struct stat stat_buf;
    off_t offset = 0;
    int fd = 0, len = 0, i = 0, argvIndex = 1, queryArrIndex = 0;
    char ch;
    argv = realloc(argv, (argvIndex + 1) * sizeof(char *));
    argv[0] = arr[1];
    if (strcmp(arr[0], "POST") == 0) {
        queryArr = split(postStr, del);
        while(queryArr[queryArrIndex] != NULL) {
            argv = realloc(argv, (argvIndex + 1) * sizeof(char *));
            argv[argvIndex] = queryArr[queryArrIndex];
            queryArrIndex++;
            argvIndex++;
        }
        argv = realloc(argv, (argvIndex + 1) * sizeof(char *));
    }
    if ((strcmp(arr[0], "GET")) != 0 && (strcmp(arr[0], "POST") != 0)) {
        write(client_socket, str1, strlen(str1) * sizeof(char));
        write(client_socket, error, strlen(error) * sizeof(char));
        close(client_socket);
        return -1;
    } 
    if ((query = strchr(arr[1], '?')) != NULL) {
        query[0] = '\0';
        query++;
        queryArr = split(query, del);
        while(queryArr[queryArrIndex] != NULL) {
            argv = realloc(argv, (argvIndex + 1) * sizeof(char *));
            argv[argvIndex] = queryArr[queryArrIndex];
            queryArrIndex++;
            argvIndex++;
        }
        argv = realloc(argv, (argvIndex + 1) * sizeof(char *));
    }
    argv[argvIndex] = NULL;
    if ((type = strchr(arr[1], '.')) == NULL) {
        type = "bin";
    } else if((strstr(arr[1], ".png")) != NULL) {
        type = str4;
    } else if((strstr(arr[1], ".html")) != NULL) {
        type = str3;
    } else if((strstr(arr[1], ".jpg")) != NULL) {
        type = str5;
    } else if((strstr(arr[1], ".gif")) != NULL) {
        type = str6;
    } else if((strstr(arr[1], ".css")) != NULL) {
        type = str7;
    } else if((strstr(arr[1], ".mp4")) != NULL) {
        type = str8;
    }
    if (strcmp(arr[2], str2) != 0) {
        write(client_socket, str1, strlen(str1) * sizeof(char));
        write(client_socket, error, strlen(error) * sizeof(char));
        close(client_socket);
        return -1;
    }
    if (arr[1][0] == '/') {
        arr[1]++;
    }
    fd = open(arr[1], O_RDONLY);
    if (fd < 0) {
        perror("open error");
        write(client_socket, str1, strlen(str1) * sizeof(char));
        write(client_socket, error, strlen(error) * sizeof(char));
        close(client_socket);
        return -1;
    }
    lstat(arr[1], &stat);
    len = stat.st_size;
    if (S_ISDIR(stat.st_mode)) {
        type = "dir";
    }
    sendHeader(client_socket, type, len);
    if (strcmp(type, "bin") == 0) {
        run_binary(arr[1], client_socket, argv);
    } else if (strcmp(type, str3) == 0) {
        readStr(fd, &string);
        write(client_socket, string, strlen(string) * sizeof(char));
    } else if(strcmp(type, str4) == 0 || (strcmp(type, str5) == 0) ||
        (strcmp(type, str6) == 0) || (strcmp(type, str7) == 0 ) ||
        (strcmp(type, str8) == 0 )) {
        fstat (fd, &stat_buf);
        sendfile (client_socket, fd, &offset, stat_buf.st_size);
    }
    close(fd);
    close(client_socket);
    return 0;
} 

void doforClient(int client_socket) {
    char **arr = NULL, **words;
    char *str = NULL;
    char *del = " ";
    char ch;
    char *postStr = NULL;
    int ret = 0, strings = 0;
    while (1) {
        while (1) {
            ret = readStr(client_socket, &str);
            printf("%d:", strings);
            if(strcmp(str, "") == 0) {
                read(client_socket, &ch, 1);
                fcntl(client_socket, F_SETFL, O_NONBLOCK);
                readStr(client_socket, &postStr);
                fcntl(client_socket, F_SETFL, !O_NONBLOCK);
                break;
            }
            puts(str);
            arr = realloc(arr, (strings + 1) * sizeof(char *));
            arr[strings] = str;
            strings++;
            str = NULL;
        }
        if (arr != NULL) {
            words = split(arr[0] , del);
            requestAnswer(words, client_socket, postStr);
            for(int i = 0; i < strings; i++) {
                free(arr[i]);
            }
            free(arr);
            arr = NULL;
        }
        if (ret <= 0) {
            break;
        }
    }
    
}

int main(int argc, char** argv) {
    int port, server_socket;
    struct sockaddr_in client_address;
    socklen_t size = sizeof client_address;
    if (argc != 2) {
        puts("Incorrect args.");
        puts("./server <port>");
        puts("Example:");
        puts("./server 5000");
        return ERR_INCORRECT_ARGS;
    }
    port = atoi(argv[1]);
    server_socket = init_socket(port);
    int *client_socket = NULL;
    int clientNum = 0;
    pid_t pid = 0;
    while (1) {
        waitpid(-1, NULL, WNOHANG);
        client_socket = realloc(client_socket, (clientNum + 1) * sizeof(int));
        client_socket[clientNum] = accept
        (
            server_socket,
            (struct sockaddr *) &client_address,
            &size
        );
        pid = fork();
        if (pid == 0) {
            doforClient(client_socket[clientNum]);
            free(client_socket);
            return OK;
        } 
        close(client_socket[clientNum]);
        clientNum++;
    }
    waitpid(-1, NULL, WNOHANG);
    return OK;
}

