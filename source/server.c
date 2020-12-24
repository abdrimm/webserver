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
#define MAX_CLIENTS_NUM 5

char *error400 = "<html><h1>Error 404<h1><html>\r\n"; 
char *http400 = "HTTP/1.1 404\r\n\r\n";
char *header_http = "HTTP/1.1";
char *http200 = "HTTP/1.1 200\r\n";
char *content_type = "Сontent-type: ";
char *content_length = "Сontent-length: ";
char *str3 = "text/html";
char *str4 = "image/png";
char *str5 = "image/jpg";
char *str6 = "text/css";

enum errors {
    OK,
    ERR_INCORRECT_ARGS,
    ERR_SOCKET,
    ERR_SETSOCKETOPT,
    ERR_BIND,
    ERR_LISTEN,
};

typedef struct {
 char *ext;
 char *mediatype;
} extn;

int init_socket(int port);
int read_string(int fd, char **string);
int file_size(int fd);
int run_binary(char *path, int client_socket, char **argv);
char** divide(char *str, char *delimiter);
void header_send(int client_socket, char *type, int len);
void its_error(int client_socket);
char** get_argv(char *arr[], int client_socket, char *post_str);
char* get_type(char *filename);
int get_answer(char *arr[], int client_socket, char *post_str);
void for_client(int client_socket);


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
    int clients_number = 0;
    pid_t pids[MAX_CLIENTS_NUM] = {0}, pid;
    int client_socket;
    while (1) {
        for (clients_number = 0; clients_number < MAX_CLIENTS_NUM; clients_number++) {
            if (pids[clients_number] == 0) {
                break;
            }
        }
        if (pids[clients_number] == 0) {
            pids[clients_number] = fork();
            if (pids[clients_number] == 0) {
                client_socket = accept(
                    server_socket,
                    (struct sockaddr *) &client_address,
                    &size);
                printf("client connected, slot №: %d\n", clients_number);
                for_client(client_socket);
                sleep(20);
                return OK;
            }
        } else {
            pid = wait(NULL);
            for (int i = 0; i < MAX_CLIENTS_NUM; i++) {
                if (pids[i] == pid) {
                    printf("slot № %d is free\n", i);
                    pids[i] = 0;
                    break;
                }
            }
            for (int i = 0; i < MAX_CLIENTS_NUM; i++) {
                if (waitpid(pids[i], NULL, WNOHANG) > 0) {
                    printf("slot № %d is free\n", i);
                    pids[i] = 0;
                    break;
                }
            }
        }
    }
    return OK;
}

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

int read_string(int fd, char **string) {
    char *str = NULL;
    char ch;
    int i = 0, status;
    while (1) {
        status = read(fd, &ch, sizeof(char)); 
        if(status <= 0) {
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
    return status;
} 

int file_size(int fd) {
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

char** divide(char *str, char *delimiter) {
    int i = 0;
    char  *strtok_ans = NULL;
    char **words = NULL;
    strtok_ans = strtok(str, delimiter);
    while(strtok_ans != NULL) {
        words = realloc(words, (i + 1) * sizeof(char *));
        words[i] = strtok_ans;
        strtok_ans = strtok(NULL, delimiter);
        i++;
    }
    words = realloc(words, (i + 1) * sizeof(char *));
    words[i] = NULL;
    return words;
}

void header_send(int client_socket, char *type, int len) {
    char str4[10];
    snprintf(str4, 10, "%d", len);
    write(client_socket, http200, strlen(http200) * sizeof(char));
    write(client_socket, content_type, strlen(content_type) * sizeof(char));   
    write(client_socket, type, strlen(type) * sizeof(char));
    write(client_socket, "\r\n", 2 * sizeof(char));
    write(client_socket, content_length, strlen(content_length) * sizeof(char));
    write(client_socket, str4, strlen(str4) * sizeof(char));
    write(client_socket, "\r\n\r\n", 4 * sizeof(char));
}

void its_error(int client_socket) {
    write(client_socket, http400, strlen(http400) * sizeof(char));
    write(client_socket, error400, strlen(error400) * sizeof(char));
    close(client_socket); 
}

char** get_argv(char *arr[], int client_socket, char *post_str) {
    char *del = "=&";
    int i = 0;
    char *array[2] = {"GET", "POST"};
    int i_argv = 1, i_query_arr = 0;
    char **argv = NULL;
    char **query_arr = NULL;
    char *query = NULL;
    if ((strcmp(arr[0], array[0])) != 0 &&
        (strcmp(arr[0], array[1]) != 0)) {
        its_error(client_socket);
        exit(1);
    } 
    argv = realloc(argv, (i_argv + 1) * sizeof(char *));
    argv[0] = arr[1];
    while (i != 2) {
        if (strcmp(arr[0], array[i]) == 0) {
            if (i == 0) {
                if ((query = strchr(arr[1], '?')) != NULL) {
                    query[0] = '\0';
                    query++;
                }
                query_arr = divide(query, del);
            }
            if (i == 1) {
                query_arr = divide(post_str, del);
            }
            while(query_arr[i_query_arr] != NULL) {
                argv = realloc(argv, (i_argv + 1) * sizeof(char *));
                argv[i_argv] = query_arr[i_query_arr];
                puts(argv[i_argv]);
                i_query_arr++;
                i_argv++;
            }
            argv = realloc(argv, (i_argv + 1) * sizeof(char *));
            argv[i_argv] = NULL;
        } 
        i++;     
    }
    return argv;
}


char* get_type(char *filename) {
    char *type = NULL;
    int i = 0;
    if ((type = strchr(filename, '.')) == NULL) {
        type = "bin";
    } else if((strstr(filename, ".png")) != NULL) {
        type = str4;
    } else if((strstr(filename, ".html")) != NULL) {
        type = str3;
    } else if((strstr(filename, ".jpg")) != NULL) {
        type = str5;
    } else if((strstr(filename, ".css")) != NULL) {
        type = str6;
    } 
    return type;
}

int get_answer(char *arr[], int client_socket, char *post_str) {
    struct stat stat;
    char **argv = NULL;
    char *type = NULL;
    char *string = NULL;
    off_t offset = 0;
    int fd = 0, len = 0, i = 0;
    char ch;
    struct stat stat_buf;
    argv = get_argv(arr, client_socket, post_str);
    type = get_type(arr[1]);
    if (type == NULL) {
        its_error(client_socket);
        return -1;
    }
    if (strcmp(arr[2], header_http) != 0) {
        its_error(client_socket);
        return -1;
    }
    if (arr[1][0] == '/') {
        arr[1]++;
    }
    fd = open(arr[1], O_RDONLY);
    if (fd < 0) {
        perror("open error");
        its_error(client_socket);
        return -1;
    }
    lstat(arr[1], &stat);
    len = stat.st_size;
    printf("%d\n", len);
    if (S_ISDIR(stat.st_mode)) {
        its_error(client_socket);
        return -1;
    }
    header_send(client_socket, type, len);
    if (strcmp(type, "bin") == 0) {
        run_binary(arr[1], client_socket, argv);
    } else if (strcmp(type, str3) == 0) {
        read_string(fd, &string);
        write(client_socket, string, strlen(string) * sizeof(char));
    } else if(strcmp(type, str4) == 0 || (strcmp(type, str5) == 0) ||
        (strcmp(type, str6) == 0)) {
        fstat (fd, &stat_buf);
        sendfile (client_socket, fd, &offset, stat_buf.st_size);
    }
    close(fd);
    close(client_socket);
    return 0;
} 

void for_client(int client_socket) {
    char **arr = NULL, **words;
    char *str = NULL;
    char *del = " ";
    char ch;
    char *post_str = NULL;
    int ret = 0, strings = 0;
    while (1) {
        while (1) {
            ret = read_string(client_socket, &str);
            if (ret <= 0) {
                puts("close");
                return;
            }
            printf("%d:", strings);
            if (strcmp(str, "") == 0) {
                read(client_socket, &ch, 1);
                fcntl(client_socket, F_SETFL, O_NONBLOCK);
                read_string(client_socket, &post_str);
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
            words = divide(arr[0] , del);
            get_answer(words, client_socket, post_str);
            for(int i = 0; i < strings; i++) {
                free(arr[i]);
            }
            free(arr);
            arr = NULL;
        }
    }
}
