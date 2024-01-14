#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#define LISTEN_BACKLOG 20

#define MAXDATASIZE 100 // max number of bytes we can get at once

#define OUTPUT "/var/tmp/aesdsocketdata"

#define INIT_OUTPUT_BUF_SIZE 500000

int server();

void sigchld_handler(int s) {
  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}
void sigint_handler(int sig) {
  remove(OUTPUT);
  _exit(0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int open_file(int *fd) {
  int exit_value = 0;
  if ((*fd = open(OUTPUT, O_CREAT | O_APPEND | O_RDWR,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) ==
      -1) {
    int errcode = errno;
    syslog(LOG_ERR, "failed to open output file, errorcode %s",
           strerror(errcode));
    exit_value = 1;
  } else {
    exit_value = 0;
  }
  return exit_value;
}

void append_char_to_buffer(uint *cursor, char *output_send_buf,
                           char single_char_buf) {
  // static uint buffer_size = INIT_OUTPUT_BUF_SIZE;
  *cursor += 1;
  // if(*cursor >= buffer_size){
  //   buffer_size += 1024;
  //   char* new_buf = realloc(output_send_buf, buffer_size);
  //   if(!(new_buf)){
  //     syslog(LOG_ERR, "failed to reallocate old buffer address
  //     %ld",(uint64_t) output_send_buf); int errcode = errno; syslog(LOG_ERR,
  //     "errno says %s",strerror(errcode)); syslog(LOG_ERR, "failed to
  //     reallocate new buffer address %ld",(uint64_t) new_buf);
  //     free(output_send_buf);
  //   }
  //   else{
  //     syslog(LOG_ERR, "successfully allocates at %ld",(uint64_t) new_buf);
  //     output_send_buf = new_buf;
  //   }
  // }
  output_send_buf[*cursor - 1] = single_char_buf;
}

int read_from_file_and_send(int output_fd, int new_fd)
{
  char single_char_buf;
  uint cursor = 0;
  char *output_send_buf = malloc(INIT_OUTPUT_BUF_SIZE);
  if (open_file(&output_fd)) {
      return -1;
  }
  while (read(output_fd, &single_char_buf, 1)) {
    if (single_char_buf == '\n') {
      append_char_to_buffer(&cursor, output_send_buf,
                            single_char_buf);
      if (send(new_fd, output_send_buf, cursor, 0) == -1) {
        int errcode = errno;
        syslog(LOG_ERR, "failed to write buffer, error %s",
                strerror(errcode));
      }
      cursor = 0;
    } else {
      append_char_to_buffer(&cursor, output_send_buf,
                            single_char_buf);
    }
  }
  if (output_send_buf)
    free(output_send_buf);
  close(output_fd);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 2 && !strcmp(argv[1], "-d")) {
      if(daemon(0,0))
        syslog(LOG_ERR, "failed to start daemon");
      else
        syslog(LOG_INFO, "starting daemon");
      server();
      return 0;
  } else {
    return server();
  }
}

int server() {
  int status, socketfd, new_fd, yes, exit_value;
  struct addrinfo hints;
  struct addrinfo *servinfo;
  struct sigaction sa;
  struct sockaddr_storage their_addr;
  socklen_t addr_size;
  char s[INET6_ADDRSTRLEN];

  if (signal(SIGINT | SIGTERM, sigint_handler) == SIG_ERR) {
    syslog(LOG_ERR, "An error occurred while setting a signal handler.");
    return EXIT_FAILURE;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0) {
    syslog(LOG_ERR, "getaddrinfo error: %s", gai_strerror(status));
    exit_value = -1;
    goto out;
  }
  if ((socketfd = socket(servinfo->ai_family, servinfo->ai_socktype,
                         servinfo->ai_protocol)) == -1) {
    syslog(LOG_ERR, "couldn't open socket\n");
    exit_value = -1;
    goto out;
  }

  yes = 1;
  if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
    syslog(LOG_ERR, "setsockopt failed");
    exit_value = 1;
    goto out;
  }
  if (bind(socketfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    syslog(LOG_ERR, "failed to bind socket");
    exit_value = -1;
    goto out;
  }
  if (listen(socketfd, LISTEN_BACKLOG) == -1) {
    syslog(LOG_ERR, "failed to listen to socket");
  }
  freeaddrinfo(servinfo);
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    syslog(LOG_ERR, "sigaction");
    exit_value = 1;
    goto out;
  }

  while (1) {
    addr_size = sizeof their_addr;
    new_fd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_fd == -1) {
      syslog(LOG_ERR, "accept");
      continue;
    }
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    syslog(LOG_INFO, "Accepted connection from %s", s);
    if (!fork()) {
      // child process
      int numbytes, curr_size, output_fd;
      char buf[MAXDATASIZE];
      char *file_buffer;
      file_buffer = malloc(0);
      curr_size = 0;
      if (open_file(&output_fd)) {
        exit_value = 1;
        goto child_out;
      }
      while (1) {
        if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1) {
          syslog(LOG_ERR, "recv");
          exit_value = 1;
          goto child_out;
        }
        file_buffer = realloc(file_buffer, curr_size + numbytes);
        memcpy(&file_buffer[curr_size], buf, numbytes);
        curr_size += numbytes;
        if (buf[numbytes - 1] == '\n') {
          if (write(output_fd, file_buffer, curr_size) == -1) {
            syslog(LOG_ERR, "failed to write buffer");
          } else {
            close(output_fd);
            exit_value = read_from_file_and_send(output_fd, new_fd);
            close(new_fd);
            goto child_out;
          }
        }
      }
    child_out:
    syslog(LOG_INFO, "Closed connection from  %s", s);
      if (file_buffer)
        free(file_buffer);
      exit(exit_value);
    }
  }
out:
  exit(exit_value);
  close(socketfd);
  remove(OUTPUT);
  exit(0);
}
