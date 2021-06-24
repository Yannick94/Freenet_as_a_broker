#ifndef fcpClient_h
#define fcpClient_h

#include "Arduino.h"
#include <WiFiNINA.h>

class fcpClient{
  public:
  fcpClient(IPAddress server, int port);
  void connectClient();
  void doClientHello();
  void doClientPut(String uri, String identifier, String dataLength, String data);
  String doRead();
  String doClientGet(String uri, String identifier);
  String doGenerateSSK(String identifier);
  bool isConnected;
  private:
  IPAddress _server;
  int _port; 
  WiFiClient _client;
  String _dataReceived;
};

#endif
