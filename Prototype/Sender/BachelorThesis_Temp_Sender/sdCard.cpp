#include "Arduino.h"
#include "sdCard.h"
#include <SPI.h>
#include <SD.h>
#include "AESLib.h"
#include <EEPROM.h>


sdCard::sdCard(int pin){
  if (!SD.begin(pin)){
    Serial.println("Initialization failed!");
    while(5);
  }else{
    filename = "connections.txt";
    reqCounter = 0;
    recCounter = 0;
    recFreenetAdr = "";
    sharedSecret = "";
  }
}


void sdCard::readFromSD(){
  String content = "";
  myFile = SD.open(filename);
  if(myFile){
    while (myFile.available()){
      content = content + (char)myFile.read();
    }
   myFile.close(); 
  }
  if(content != ""){
    recCounter = getValue(content,';',3).toInt();
    reqCounter = getValue(content,';',2).toInt();
    recFreenetAdr = getValue(content,';',1);
    sharedSecret = getValue(content,';',0);
  }
}

void sdCard::writeToSD(int reqCtr, int recCtr,  String adr, String secret){
  myFile = SD.open(filename);
  if(myFile){
    myFile.println(secret + ";" + adr + ";" + reqCtr + ";" + recCtr);
    myFile.close();
  }
}

void sdCard::resetFile(){
  SD.remove(filename);
}

String sdCard::getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
