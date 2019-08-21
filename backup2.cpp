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
//to allow sleep function c++11 onwards
#include <chrono>
#include <thread>
//SIGNAL ALAMR FOR TIMEOUT
#include <signal.h>

//try to use this as a timeout
volatile bool failedToConnect = false;
void handle(int sig) {
    failedToConnect = true;
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
  bool expect(int socketFD, std::string searchString)
  {
    ioctl(socketFD, FIONBIO, &iMode); //set non-blocking
    for(;;)
    {
      bytesRead = read(socketFD, buffer, sizeof(buffer));
      stringBuffer = buffer;
      totalBytesRead += bytesRead;
      if (stringBuffer.find(searchString) != std::string::npos)
      {
        //utils.debug("Total Bytes Read: ",totalBytesRead);
        return true;
      }
      if (totalBytesRead < -5000000) //this will time out the search and return false
      {
        //utils.debug("giving up!");
        return false;
      }
      
    }
  }
  bool sendData(int socketFD, std::string data)
  {
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
};

int main(int argc, char *argv[])
{
const std::string TEST_HOST = "11.200.2.26";
const unsigned int TELNET_PORT = 23;
const unsigned int CONNECT_DELAY = 5;
const unsigned int ALARM_CANCEL = 0;

signal(SIGALRM, handle);

  Utilities utils;
  Telnet telnet;

  utils.cls(4);
  utils.debug("CTR Auto Scripting");
  
  int socketFD, portNo, bytesRead;
  char buffer[1000];
  portNo = 23; //why not accept the define?
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  if (socketFD < 0)
  {
    utils.debug("Failed to open socket!");
    exit(0);
  }
  utils.debug("Socket open");
  
  
  getaddrinfo("11.200.2.26", "23", &hints, &res);
  alarm(CONNECT_DELAY);
  if (connect(socketFD, res->ai_addr, res->ai_addrlen) < 0) 
  {
    utils.debug("Error connecting");
    std::cout << "Failed to connect is " << failedToConnect << std::endl;
    exit(0);
  }
  alarm(ALARM_CANCEL);
  std::cout << "Failed to connect is " << failedToConnect << std::endl;
  bool result = telnet.login(socketFD, "login:", "assword:", "root\n", "admin123\n", "#");
  (result) ? utils.debug("Logged in!") : exit(0);

  return 0;
}