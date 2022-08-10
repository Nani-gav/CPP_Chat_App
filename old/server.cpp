#include <iostream>
#include "tcp.h"

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

int main(){
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
