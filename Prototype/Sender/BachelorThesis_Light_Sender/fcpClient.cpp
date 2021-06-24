#include "Arduino.h"
#include "fcpClient.h"
#include <WiFiNINA.h>

fcpClient::fcpClient(IPAddress server, int port){
  _server = server;
  _port = port;
  isConnected = false;
}

void fcpClient::connectClient(){
  if (_client.connect(_server, _port)) {
    Serial.println("connected to server");
  }
}

void fcpClient::doClientHello(){
    _client.println("ClientHello");
    _client.println("Name=ArduinoGenerateUSKExample");
    _client.println("ExpectedVersion=2.0");
    _client.println("EndMessage");
  delay(1000);
  while (_client.available()) {
    _dataReceived = _client.readString(); 
  }
  if(_dataReceived.indexOf("NodeHello" > 0)){
    isConnected = true;
  }else{
    isConnected = false;
  }
}

String fcpClient::doRead(){
  while (_client.available()) {
    _dataReceived = _client.readString(); 
  }
  _client.flush();
  return _dataReceived;
}

void fcpClient::doClientPut(String uri, String identifier, String dataLength, String data){
  _client.println("ClientPut");
  _client.println("URI="+uri);
  _client.println("Metadata.ContentType=text/plain");
  _client.println("PriorityClass=0");
  _client.println("Identifier="+identifier);
  _client.println("DataLength="+dataLength);
  _client.println("Data");
  _client.println(data);
  Serial.println("Client put sended!");
}

String fcpClient::doClientGet(String uri, String identifier){
  _client.println("ClientGet");
  _client.println("URI="+uri);
  _client.println("Identifier="+identifier);
  _client.println("ReturnType=direct");
  _client.println("PriorityClass=0");
  _client.println("EndMessage");
  delay(1000);
  while (_client.available()) {
    _dataReceived = _client.readString(); 
  }
  return _dataReceived;
}

String fcpClient::doGenerateSSK(String identifier){
  _client.println("GenerateSSK");
  _client.println("Identifier="+identifier);
  _client.println("EndMessage");
  delay(1000);
  while (_client.available()) {
    _dataReceived = _client.readString(); 
  }
  return _dataReceived;
}
