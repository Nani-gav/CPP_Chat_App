#ifndef KEY_JSYNC_TCP_H
#define KEY_JSYNC_TCP_H
//FINISH FOR WINDOWS OS TOO!!!!

// #ifdef __cplusplus
// extern "C" {
// #endif

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/event.h> //evport for solaris, epoll for linux, kqueue for macos and free bsd
#include <arpa/inet.h>
#include <pthread.h>

using std::cout;

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

struct jsnt_server_info{
  int ipv;
  JSNT_CONN sfd; //socket file descriptors
  int spfd; //sys poll file descriptor
  int sptype; //sys poll type
  short int port;
  int backlog;
};

struct jsnt_client_info{
  int action;
  JSNT_CONN sfd;
  char ipaddr[46]; //replace with dynamic string when done with string.h
};

// typedef struct jsnt_server_info jsnt_server_info;
// typedef struct jsnt_client_info jsnt_client_info;

class jsntServer {

  public:

  struct jsnt_server_info info;

  jsntServer(int ipv, short int port){
    info.ipv = ipv;
    info.port = port;
  }

  int startServer(/*struct jsnt_server_info* info*/){
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
      my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
      if(bind(info.sfd, (sockaddr*)&my_addr, sizeof(my_addr)) < 0){
        close(info.sfd);
        return JSNT_SBIND_ERR;
      }
    }else{
      struct sockaddr_in6 my_addr;
      memset(&my_addr, 0, sizeof(my_addr));
      my_addr.sin6_family = JSNT_USEIPV6;
      my_addr.sin6_port = htons(info.port);
      my_addr.sin6_addr = in6addr_any;
      if(bind(info.sfd, (sockaddr*)&my_addr, sizeof(my_addr)) < 0){
        close(info.sfd);
        return JSNT_SBIND_ERR;
      }
    }
    int yes = 1;
    if(setsockopt(info.sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
      close(info.sfd);
      return JSNT_SSOPT_ERR;
    }
    if(info.backlog < 1 || info.backlog > 10){
      info.backlog = 3;
    }
    if(listen(info.sfd, info.backlog) < 0){
      close(info.sfd);
      return JSNT_SLISTEN_ERR;
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

  int addCIN(){
    struct kevent msockstruct; //main socket struct
    EV_SET(&msockstruct, 0, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if(kevent(info.spfd, &msockstruct, 1, NULL, 0, NULL) < 0){
      return JSNT_SYSPOLL_ERR;
    }
    return 0;
  }

  int addReadSocket(int socket){
    struct kevent msockstruct; //main socket struct
    EV_SET(&msockstruct, socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if(kevent(info.spfd, &msockstruct, 1, NULL, 0, NULL) < 0){
      return JSNT_SYSPOLL_ERR;
    }
    return 0;
  }

  int waitForMesg(/*struct jsnt_server_info* info,*/ struct jsnt_client_info* clinfo){ //maybe
    //you should replace this with a dynamic string
    struct kevent event;
    if(kevent(info.spfd, NULL, 0, &event, 1, NULL) < 0){
      return JSNT_SYSPOLL_ERR;
    }
    int retsfd = (int)event.ident;
    if(retsfd == info.sfd){
      struct sockaddr_storage cliaddr;
      socklen_t len = sizeof(cliaddr);
      int new_sfd;
      if((new_sfd = accept(retsfd, (struct sockaddr*) &cliaddr, &len)) < 0){
        return JSNT_SACCEPT_ERR;
      }
      clinfo->sfd = new_sfd;
      if(cliaddr.ss_family == JSNT_USEIPV4){ //get and return client,
        //finish string.h (with antirez sds) before doing this
        struct sockaddr_in* cliaddr4 = (struct sockaddr_in*) &cliaddr;
        clinfo->action = JSNT_USEIPV4;
        inet_ntop(JSNT_USEIPV4, &(cliaddr4->sin_addr), clinfo->ipaddr, 16);
      }else{
        struct sockaddr_in6* cliaddr6 = (struct sockaddr_in6*) &cliaddr;
        clinfo->action = JSNT_USEIPV6;
        inet_ntop(JSNT_USEIPV6, &(cliaddr6->sin6_addr), clinfo->ipaddr, 46);
      }
      struct kevent clevent;
      EV_SET(&clevent, new_sfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
      if(kevent(info.spfd, &clevent, 1, NULL, 0, NULL) < 0){
        close(new_sfd);
        return JSNT_SYSPOLL_ERR;
      }
    }else if(event.flags & EV_EOF){
      clinfo->sfd = retsfd;
      clinfo->action = JSNT_CLIENT_LEFT;
      close(retsfd);
    }else if(event.filter & EVFILT_READ){
      clinfo->sfd = retsfd;
      clinfo->action = JSNT_CLIENT_MESG;
    }
    return 0;
  }

  int readMesg(/*int sfd,*/ int sfd, char* buffer, size_t len, int count){
    int bytes;
    char* bufptr = buffer;
    int readpt = 0;
    if(!count){
      if((bytes = read(sfd, bufptr, len)) < 0){
        cout << strerror(errno) <<'\n';
        return JSNT_RECV_ERR;
      }
    }else{
      while((bytes = read(sfd, bufptr, len-readpt)) < 0){
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

  int writeMesg(/*int sfd,*/ int sfd, char* buffer, size_t len){
    int bytes;
    char* bufptr = buffer;
    int written = 0;
    while((bytes = write(sfd, bufptr, len-written))){
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

  ~jsntServer(/*struct jsnt_server_info* info*/){
    if(close(info.spfd)){
      // return JSNT_SCLOSE_ERR;
    }
    if(close(info.sfd)){
      // return JSNT_SCLOSE_ERR;
    }
    // return 0;
  }

};
// #ifdef __cplusplus
// }
// #endif

#endif /* end of include guard: KEY_JSYNC_TCP_H */
