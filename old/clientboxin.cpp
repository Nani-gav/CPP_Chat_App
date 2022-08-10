#include <iostream>
#include <string>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include "client.h"
#include "tcp.h"

using std::cin;
using std::cout;

// https://gamedev.stackexchange.com/questions/146256/how-do-i-get-getchar-to-not-block-the-input

int main(int argc, char** argv){
  if(argc < 2){
    cout << "Input your username: E.g. EXEC_FILE username\n";
    return 0;
  }

  jsntClient client(JSNT_USEIPV4, "127.0.0.1", 3000);
  client.startClient();
  struct jsnt_client_info clonf;
  jsntServer server(JSNT_USEIPV4, 3069);
  int i = 0;
  int err;
  while((err = server.startServer())){
    if(i++ > 9){
      cout << "Too many clients have already connected\n";
      return 0;
    }
    server.info.port += 1;
  }
  cout << "Port no: " << server.info.port << "\n";

  server.waitForMesg(&clonf);
  int outbox;
  if(clonf.action == JSNT_USEIPV4 || clonf.action == JSNT_USEIPV6){
    outbox = clonf.sfd;
    cout << "Output process joined\n\n\n\n\nTYPE> " << std::flush;
  }
  server.addCIN();
  server.addReadSocket(client.info.sfd);

  std::string bufr = "";
  char redbufr[1025] = "";
  char mesgbufr[1065] = "";
  while(true){
    server.waitForMesg(&clonf);
    if(clonf.sfd == client.info.sfd){ // server
      // cout << "hi\n";
      int len = client.readMesg(redbufr, 1024, 0);
      redbufr[len] = 0;
      server.writeMesg(outbox, redbufr, len);
    }
    if(clonf.sfd == 0){ // cin
      std::getline(cin, bufr);
      bufr = argv[1]+std::string(": ")+bufr;
      // cout << std::endl;
      // cout << "\033[1A\033[KTYPE> " << std::flush;
      // if(bufr == "exit"){
      //   break;
      // }
      client.writeMesg((char*)bufr.c_str(), bufr.length());
      cout << "\r\033[1A\033[KTYPE> " << std::flush;
    }
  }

  // cin >> bufr;
  // client.writeMesg(bufr, strlen(bufr));
  // cout << "done\n";
  return 0;
}
