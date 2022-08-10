#include <iostream>
#include <string>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include "client.h"
#include "stdlib.h"

using std::cin;
using std::cout;

// https://gamedev.stackexchange.com/questions/146256/how-do-i-get-getchar-to-not-block-the-input

int main(int argc, char** argv){
  if(argc < 2){
    cout << "enter port no\n";
    return 0;
  }
  int portnog = atoi(argv[1]);
  if(portnog < 3069 || portnog > 3079){
    cout << "port in wron range\n";
    return 0;
  }
  jsntClient client(JSNT_USEIPV4, "127.0.0.1", portnog);
  client.startClient();
  cout << "started\n\n";
  struct jsnt_server_struct clonf;
  std::string bufr = "";
  char redbufr[1025] = "";
  char mesgbufr[1025] = "";
  while(true){
    client.waitForMesg(&clonf);
    if(clonf.action == JSNT_CLIENT_LEFT){ // server
      return 0;
    }
    if(clonf.action == JSNT_CLIENT_MESG){ // cin
      int len = client.readMesg(redbufr, 1024, 0);
      redbufr[len] = 0;
      cout << redbufr << "\n" << std::endl;
    }
  }

  // cin >> bufr;
  // client.writeMesg(bufr, strlen(bufr));
  // cout << "done\n";
  return 0;
}
