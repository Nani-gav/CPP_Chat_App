#ifndef CLIENT_KEY_H
#define CLIENT_KEY_H

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/event.h> //evport for solaris, epoll for linux, kqueue for macos and free bsd
#include <arpa/inet.h>
#include <pthread.h>

#define JSNT_DONTCARE -1 //JSNT stands for JSiNc Tcp (JavaScript IN C)

#define JSNT_USEIPV4 2
#define JSNT_USEIPV6 AF_INET6

//I dont know what the below macros are for

#define JSNT_MESGNONE 0
#define JSNT_MESGADD 1
#define JSNT_MESGSEND 2
#define JSNT_MESGDEL 3

//Sys poll type

#define KQUEUE 0
#define EPOLL 1
#define EVPORT 2
#define IOCTL 3

//Error codes

#define JSNT_NO_ERR 0 //Do not use
#define JSNT_FUNCARG_ERR 1 //Wrong arguments passed
#define JSNT_SCREATE_ERR 2
#define JSNT_SBIND_ERR 3
#define JSNT_SSOPT_ERR 4
#define JSNT_SCONNECT_ERR 13
#define JSNT_SLISTEN_ERR 5
#define JSNT_SACCEPT_ERR 6
#define JSNT_SEND_ERR -7
#define JSNT_RECV_ERR -8
#define JSNT_SCLOSE_ERR 9
#define JSNT_SYSPOLLCREATE_ERR 10
#define JSNT_SYSPOLL_ERR 11
#define JSNT_FCNTL_ERR 12

//Success codes (Do not use)

#define JSNT_SERVER_START 10
#define JSNT_CLIENT_ADDED 11
#define JSNT_CLIENT_REMOVED 12
#define JSNT_CLIENT_KICKED 13
#define JSNT_MESG_SENT 14
#define JSNT_MESG_RECV 15

#define JSNT_CLIENT_IPV4 JSNT_USEIPV4
#define JSNT_CLIENT_IPV6 JSNT_USEIPV6
#define JSNT_CLIENT_LEFT -1
#define JSNT_CLIENT_MESG -2

#define JSNT_CONN int //for socket file descriptors

struct jsnt_client_struct{
  int ipv;
  JSNT_CONN sfd; //socket file descriptors
  int spfd; //sys poll file descriptor
  int sptype; //sys poll type
  short int port;
  int backlog;
  char ipaddr[46];
};

struct jsnt_server_struct{
  int action;
  JSNT_CONN sfd;
  char ipaddr[46]; //replace with dynamic string when done with string.h
};

class jsntClient {
  public:

  struct jsnt_client_struct info;

  jsntClient(int ipv, char ipar[], short int port){
    info.ipv = ipv;
    strcpy(info.ipaddr, ipar);
    info.port = port;
  }

  int startClient(/*struct jsnt_server_info* info, */ ){
    // merge with constructor
    if(info.ipv != JSNT_USEIPV4 && info.ipv != JSNT_USEIPV6){
      return JSNT_FUNCARG_ERR;
    }
    if((info.sfd = socket(info.ipv, 1, 6)) < 0){ //AF_INET, SOCK_STREAM, SOL_TCP
      return JSNT_SCREATE_ERR;
    }
    if(info.ipv == JSNT_USEIPV4){
      struct sockaddr_in my_addr;
      memset(&my_addr, 0, sizeof(my_addr));
      my_addr.sin_family = JSNT_USEIPV4;
      my_addr.sin_port = htons(info.port);
      inet_pton(AF_INET, info.ipaddr, &(my_addr.sin_addr));
      if(connect(info.sfd, (sockaddr*)&my_addr, sizeof(my_addr)) < 0){
        close(info.sfd);
        return JSNT_SCONNECT_ERR;
      }
    }else{
      struct sockaddr_in6 my_addr;
      memset(&my_addr, 0, sizeof(my_addr));
      my_addr.sin6_family = JSNT_USEIPV6;
      my_addr.sin6_port = htons(info.port);
      inet_pton(AF_INET6, info.ipaddr, &(my_addr.sin6_addr));
      if(connect(info.sfd, (sockaddr*)&my_addr, sizeof(my_addr)) < 0){
        close(info.sfd);
        return JSNT_SCONNECT_ERR;
      }
    }
    info.sptype = KQUEUE;
    if((info.spfd = kqueue()) < 0){
      close(info.sfd);
      return JSNT_SYSPOLLCREATE_ERR;
    }
    struct kevent msockstruct; //main socket struct
    EV_SET(&msockstruct, info.sfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if(kevent(info.spfd, &msockstruct, 1, NULL, 0, NULL) < 0){
      return JSNT_SYSPOLL_ERR;
    }
    return 0;
  }

  int waitForMesg(/*struct jsnt_server_info* info,*/ struct jsnt_server_struct* clinfo){ //maybe
    //you should replace this with a dynamic string
    struct kevent event;
    if(kevent(info.spfd, NULL, 0, &event, 1, NULL) < 0){
      return JSNT_SYSPOLL_ERR;
    }
    int retsfd = (int)event.ident;
    if(event.flags & EV_EOF){
      clinfo->sfd = retsfd;
      clinfo->action = JSNT_CLIENT_LEFT;
    }else if(event.filter & EVFILT_READ){
      clinfo->sfd = retsfd;
      clinfo->action = JSNT_CLIENT_MESG;
    }
    return 0;
  }

  int readMesg(/*int sfd,*/char* buffer, size_t len, int count){
    int bytes;
    char* bufptr = buffer;
    int readpt = 0;
    if(!count){
      if((bytes = read(info.sfd, bufptr, len)) < 0){
        // cout << strerror(errno) <<'\n';
        return JSNT_RECV_ERR;
      }
    }else{
      while((bytes = read(info.sfd, bufptr, len-readpt)) < 0){
        if(bytes < 0){
          return JSNT_SEND_ERR;
        }
        readpt += bytes;
        if(readpt >= len){
          break;
        }
        bufptr += bytes;
      }
    }
    return bytes;
  }

  int writeMesg(/*int sfd,*/ char* buffer, size_t len){
    int bytes;
    char* bufptr = buffer;
    int written = 0;
    while((bytes = write(info.sfd, bufptr, len-written))){
      if(bytes < 0){
        return JSNT_SEND_ERR;
      }
      written += bytes;
      if(written >= len){
        break;
      }
      bufptr += bytes;
    }
    return 0;
  }

  int closeConn(int conn_id){
    if(close(conn_id)){
      return JSNT_SCLOSE_ERR;
    }
    return 0;
  }

  ~jsntClient(/*struct jsnt_server_info* info*/){
    // if(close(info.spfd)){
    //   return JSNT_SCLOSE_ERR;
    // }
    if(close(info.sfd)){
      // theres been an error, print error message
    }
  }
};

#endif
