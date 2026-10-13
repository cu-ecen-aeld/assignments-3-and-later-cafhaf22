#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>


#define BACKLOG 1
#define SOCKET_PORT "9000"
#define MAX_BUFFER_SIZE 256
#define TIMEOUT_MS 1000

const char * TAG = "SOCKET_SERVER";
const char * DataFilePath = "/var/tmp/aesdsocketdata";
int DataFd = -1;
int isDaemon = 0;
volatile sig_atomic_t ShouldStop =  0;

static void signal_handler(int data) {
    ShouldStop = 1;
}

static int OpenFile(void) {
    int fd = open(DataFilePath, O_TRUNC | O_RDWR| O_CREAT | O_APPEND, 0666);
    return fd;
}

static int WriteToFile(const char * buff, size_t buff_size) {
    int ret = -1;
    //char * p; 
    
    ret = write(DataFd, buff, buff_size);
    if(ret == -1) {
        syslog(LOG_ERR, "Error on write: %d", ret);
    } else if(ret != buff_size) {
        syslog(LOG_WARNING, "Warning on write %d/%ld", ret, buff_size);
    }

    return ret;
}

static int EchoToClient(int fd) {
    ssize_t ret;
    int err;

    ret = lseek(DataFd, (off_t)0, SEEK_SET); // set the fd position at the beginning of the file
    if(ret == -1) {
        syslog(LOG_ERR, "Error on lseek to beginning of file");
        return ret;
    }

    char* read_buf = malloc(MAX_BUFFER_SIZE);
    if(read_buf == NULL) {
        syslog(LOG_ERR, "Error creating read buffer");
        return -1;
    }

    size_t len = MAX_BUFFER_SIZE;
    while((ret = read(DataFd, read_buf, MAX_BUFFER_SIZE - 1)) != 0) {
        syslog(LOG_INFO, "ret is: %zd", ret);
        if(ret == -1) {
            break;
        }

        if(send(fd, read_buf, ret, MSG_NOSIGNAL) == ret) {
            read_buf[ret] = '\0';
            syslog(LOG_INFO, "Sending %zd bytes\n%s", ret, read_buf);
        } else {
            syslog(LOG_ERR, "Error in send");
        }
        
    }
    free(read_buf);
    return ret;
}

int server(void) {
    int err;
    int yes = 1;
    // prepare the socket address to listen onto
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if(getaddrinfo(NULL, SOCKET_PORT, &hints, &res) != 0) {
        err = errno;
        syslog(LOG_ERR, "Error on getaddrinfo, errno: %d", err);
        return -1;
    }

    // Creating socket
    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sockfd == -1) {
        err = errno;
        syslog(LOG_ERR, "Error on socket open, sockfd : %d, errno:recv_buff %d",sockfd, err);
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    // binding the socket to an address
    if(bind(sockfd, res->ai_addr, res->ai_addrlen) != 0) {
        err = errno;
        syslog(LOG_ERR, "Error on bind socket, errno: %d", err);
        close(sockfd);
        return -1;
    }

    // all done with the resultt form getaddrinfo
    freeaddrinfo(res);
    if(isDaemon) {
        pid_t pid = fork();
        if(pid == -1) {
            syslog(LOG_ERR, "fork: %s", strerror(errno));
            close(sockfd);
            return -1;
        } else if(pid > 0) {
            // parent process
            _exit(0);
        }

        // child process
        if(setsid() == -1) {
            syslog(LOG_ERR, "setsid: %s", strerror(errno));
            close(sockfd);
            return -1;
        }
        chdir("/");
        int devnull = open("/dev/null", O_RDWR);
        if(devnull != -1) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if(devnull > STDERR_FILENO) {
                close(devnull);
            }
        }
    }


    

    // listen for incomming connections
    if(listen(sockfd, BACKLOG) != 0) {
        err = errno;
        syslog(LOG_ERR, "Error on listen socket, errno: %d", err);
        close(sockfd);
        return -1;
    }

    syslog(LOG_INFO, "waiting for connections...");
    
    
    // receive stuff
    char* recv_buff = malloc(MAX_BUFFER_SIZE);
    if(recv_buff == NULL) {
        err = errno;
        syslog(LOG_ERR, "Receive buffer allocation failed, errno: %d", err);
        close(sockfd);
        return -1;
    }
    
    // new connection set up
    
    struct sockaddr_storage client_addr;
    socklen_t cli_size;

    char ipstr[INET_ADDRSTRLEN];
    int new_fd;
    while(!ShouldStop) {
        // Accepting connection
        cli_size = sizeof(client_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &cli_size);
        if(new_fd == -1) {
            err = errno;
            if(err == EINTR) {
                continue;
            }
            syslog(LOG_WARNING, "accept socket, errno: %d", err);
            continue;
        }
        
        socklen_t len = sizeof(client_addr);
        getpeername(new_fd, (struct sockaddr*)&client_addr, &len);
        if(client_addr.ss_family == AF_INET) {
            struct sockaddr_in* s = (struct sockaddr_in*)&client_addr;
            inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
            syslog(LOG_INFO, "Accepted connection from: %s", ipstr);
        }

        struct pollfd ufd = {
            .fd = new_fd,
            .events = POLLIN,
        };

        // receving loop
        while(!ShouldStop) {
            err = poll(&ufd, 1, TIMEOUT_MS);
            if(err == -1) {
                if(errno == EINTR) {
                    continue;
                }
                syslog(LOG_ERR, "Error ocurred on poll");
                break; // or return
            }
            else if (err == 0) {
                syslog(LOG_WARNING, "Poll() timed out");
                continue;
            }

            if(ufd.revents & POLLIN) {
                int bytes = 0;
                while((bytes = recv(new_fd, recv_buff, MAX_BUFFER_SIZE - 1, 0)) != 0){
                    if(bytes < 0) {
                        syslog(LOG_ERR, "recv: %s", strerror(errno));
                        break;
                    }
                    syslog(LOG_INFO, "Received %d bytes of data", bytes);
                    WriteToFile(recv_buff, bytes);
                    if (recv_buff[bytes - 1] == '\n')
                    {
                        if(EchoToClient(new_fd) != 0) { // // open the file read it and sent it back
                            syslog(LOG_ERR, "Error echo to client");
                        }
                        break;
                    }
                }
                
                syslog(LOG_INFO,"Closed connection from %s", ipstr);
                break;
            }
        }
        
        close(new_fd);
        
    }
    free(recv_buff);
    syslog(LOG_INFO, "Caught signal, exiting");
    close(sockfd);



    return 0;
}


int main (int argc, const char * argv []) {
    // open syslog
    openlog(TAG, LOG_PID | LOG_NDELAY, LOG_USER);
    // register signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if(argc == 2) {
        printf("%s, %s\n", argv[0], argv[1]);
        isDaemon = (strcmp(argv[1], "-d" ) == 0) ? 1 : 0;
    }

    DataFd = OpenFile();
    if (DataFd == -1) {
        syslog(LOG_ERR, "Error opening/creating file : %s", DataFilePath);
        return -1;
    }

    server();
    close(DataFd);
    remove(DataFilePath);



    return 0;
}