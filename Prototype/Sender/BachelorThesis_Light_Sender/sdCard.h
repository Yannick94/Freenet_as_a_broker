#ifndef sdCard_h
#define sdCard_h

#include "Arduino.h"
#include <SPI.h>
#include <SD.h>
#include "AESLib.h"
#include <EEPROM.h>

class sdCard{
  public:
  int reqCounter;
  int recCounter;
  String recFreenetAdr;
  String sharedSecret;
  sdCard(int pin);
  void writeToSD(int reqCtr, int recCtr,  String adr, String secret);
  void resetFile();
  void readFromSD();
  private:
  String filename;
  File myFile;
  String getValue(String data, char separator, int index);
};

#endif
