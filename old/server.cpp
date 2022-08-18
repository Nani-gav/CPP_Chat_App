#include <iostream>
#include "tcp.h"
#include "errno.h"
#include <stdio.h>
#include <errno.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <string.h>

using std::cout;

void delete_client(int arr[], int value, int* len){
  int index;
  for(int i = 0; i < *len; i++){
    if(arr[i]==value){
      index = i;
      break;
    }
  }
  arr[index] = arr[--(*len)];
}

char buf[64];

int main(){
  struct ifaddrs *myaddrs, *ifa;
  void *in_addr;

  if(getifaddrs(&myaddrs) != 0){
    perror("getifaddrs error");
    exit(1);
  }
  for(ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next){
    if (ifa->ifa_addr == NULL)
        continue;
    if (!(ifa->ifa_flags & IFF_UP))
        continue;
    switch (ifa->ifa_addr->sa_family)
    {
      case AF_INET:{
        struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
        in_addr = &s4->sin_addr;
        break;
      }
      // case AF_INET6:
      default:
        continue;
    }
    if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf))){
      printf("%s: inet_ntop failed!\n", ifa->ifa_name);
      return 1;
    }
    else{
      if(strcmp(ifa->ifa_name, "en0")==0){
        break;
      }
    }
  }
  freeifaddrs(myaddrs);
  cout << "Server_id: " << buf << std::endl;

  // printf("IP addresses for %s:\n\n", argv[1]);
  // char *ipad; // IP ADdress
  // struct hostent *host_entry;
  // int hostname;
  // if((hostname = gethostname(host, sizeof(host))) != 0){
  //   cout << host << std::endl;
  //   return 0;
  // }
  // cout << "L'Host: " << host << "\n";
  // if((host_entry = gethostbyname(host)) == NULL){
  //   perror("Error");
  //   cout << h_errno << std::endl;
  //   return 0;
  // }
  // if((ipad = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]))) == NULL){
  //   cout << "err2" << std::endl;
  //   return 0;
  // }
  // cout << "Server id: " << ipad << "\n";

  jsntServer server(JSNT_USEIPV4, 3000);
  server.startServer();
  cout << "started\n";
  struct jsnt_client_info clonf;
  int loop = 1;
  char bufr[1025];
  int clients[20];
  int clilen = 0;
  while(loop){
    memset(bufr, 0, sizeof(bufr));
    server.waitForMesg(&clonf);
    if(clonf.action == JSNT_USEIPV4 || clonf.action == JSNT_USEIPV6){
      cout << "Client " << clonf.sfd << " joined the server.\n";
      clients[clilen++] = clonf.sfd;
    }else if(clonf.action == JSNT_CLIENT_MESG){
      int lol = server.readMesg(clonf.sfd, bufr, 1024, 0);
      for (int i = 0; i < clilen; i++){
        // if(clients[i] != clonf.sfd){
        // because even the sender needs to see their messaege
          server.writeMesg(clients[i], bufr, lol);
        // }
      }
    }else if(clonf.action == JSNT_CLIENT_LEFT){
      delete_client(clients, clonf.sfd, &clilen);
      cout << "Client " << clonf.sfd << " left the server.\n";
    }
  }
  return 0;
}
