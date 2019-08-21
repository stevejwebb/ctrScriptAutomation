#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
//to allow sleep function c++11 onwards
#include <chrono>
#include <thread>
//SIGNAL ALAMR FOR TIMEOUT
#include <signal.h>

//try to use this as a timeout
volatile bool failedToConnect = false;
void handle(int sig) {
    failedToConnect = true;
    exit(0);
}
//hanlde ctrl-c
void handle_ctrl_c(int sig) {
  std::cout << std::endl << "Caught ctr-c! Terminating program." << std::endl;
}

class Utilities {
  public:
  void cls()
  {
      for (int i = 0; i < 120; i++)
      {
          std::cout << std::endl;
      }
  }
  void cls(int numberOfLines)
  {
    for (int i = 0; i < numberOfLines; i++)
    {
      std::cout << std::endl;
    }
    
  }
  void debug(std::string dbgMsg)
  {
    std::cout << "DEBUG: " << dbgMsg << std::endl;
  }
  void debug(std::string dbgMsg, int num)
  {
    std::cout << "DEBUG: " << dbgMsg << num << std::endl;
  }
};

class Telnet {
public:
  Utilities utils;
  char buffer[10000];
  int bytesRead;
  int bytesSent = 0;
  int bytesSentLength = 0;
  std::string stringBuffer;
  int totalBytesRead = 0;
  int delay = 1000;
  unsigned long iMode = 1;
  const unsigned int CONNECT_DELAY = 5;
  const unsigned int ALARM_CANCEL = 0;
  bool expect(int socketFD, std::string searchString)
  {
    signal(SIGALRM, handle); //timeout
    //signal(SIGINT, handle_ctrl_c); //catch ctrl-c
    
    ioctl(socketFD, FIONBIO, &iMode); //set non-blocking
    alarm(5);
    for(;;)
    {
      bytesRead = read(socketFD, buffer, sizeof(buffer));
      stringBuffer = buffer;
      totalBytesRead += bytesRead;
      if (stringBuffer.find(searchString) != std::string::npos)
      {
        //utils.debug("Total Bytes Read: ",totalBytesRead);
        alarm(0);
        return true;
      }
      if (failedToConnect) return false;
      
    }
  }
  bool sendData(int socketFD, std::string data)
  {
    signal(SIGALRM, handle); //timeout
    //signal(SIGINT, handle_ctrl_c); //catch ctrl-c
    //hack to get data into a raw format to send
    char buffer[data.size()+1];
    data.copy(buffer,data.size()+1);
    buffer[data.size()] = '\0';

    bytesSentLength = data.length();
    bytesSent = send(socketFD, buffer, sizeof(buffer), 0);
    if (bytesSent)
    {
      return true;
    }
    return false;
  }
  bool login(int socketFD, std::string loginPrompt, std::string passwordPrompt, std::string userName, std::string password, std::string prompt)
  {
    bool result = false; //set expectation to fail
    result = expect(socketFD, loginPrompt);
    result = sendData(socketFD, userName);
    result = expect(socketFD, passwordPrompt);
    result = sendData(socketFD, password);
    //look for login prompt to show it went okay
    result = sendData(socketFD, "\n"); //prompt with CR
    result = expect(socketFD, prompt);
    if (result) return true;
    return false;
  }
  bool connectUnit(int socketFD, char *IpString)
  {
    utils.debug("Trying to connect to ");
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(IpString, "23", &hints, &res);

    alarm(CONNECT_DELAY); //set timeout
    if (connect(socketFD, res->ai_addr, res->ai_addrlen) < 0) 
    {
      utils.debug("Error connecting");
      return false;
    }
    alarm(ALARM_CANCEL);//we are okay now
    return true;
  }
};

int main(int argc, char *argv[])
{
const std::string TEST_HOST = "11.200.2.26";
const unsigned int TELNET_PORT = 23;
const unsigned int CONNECT_DELAY = 5;
const unsigned int ALARM_CANCEL = 0;

signal(SIGALRM, handle); //timeout
//signal(SIGINT, handle_ctrl_c); //catch ctrl-c

  Utilities utils;
  Telnet telnet;

  utils.cls(4);
  utils.debug("CTR Auto Scripting");
  
  int socketFD;
  socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  if (socketFD < 0)
  {
    utils.debug("Failed to open socket!");
    exit(0);
  }
  char ipString[15]  = {"11.200.2.26"};
  //------------ Start of loop to process the scripts --------------------------
  //ToDo: from here you need to process the ip file and script feel
  //create a loop and error checking to skip problem node
  //have a debug output file also
  //nice to have a time stamp on debug output and log

  //------------- main connection and login to the node -----------------------
  telnet.connectUnit(socketFD, ipString);
  bool result = telnet.login(socketFD, "login:", "assword:", "root\n", "admin123\n", "#");
  //---------------------------------------------------------------------------
  
  (result) ? utils.debug("Logged in!") : exit(0);
  
  telnet.sendData(socketFD, "show int stat\n");
  telnet.expect(socketFD, "#");

  return 0;
}