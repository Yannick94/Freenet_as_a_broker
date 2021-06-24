#include <rBase64.h>
#include <types.h>
#include <uECC.h>
#include <uECC_vli.h>
#include <AESLib.h>
#include <WiFiNINA.h>
#include <DHT.h>
#include "fcpClient.h"
#include "sdCard.h"
#include "arduino_secrets.h" 

//Variables for Wifi - arduino_secrets contains credentials for Connection
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;

//Constants for TempSensor
#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

//Variables for Freenet
IPAddress server(192,168,1,56);
String freenetRequestUSKURI = "USK@jPUqN3uiMLil1jOHssBzgUZTlKa7QJ6QdC7twajT83Q,db8UCXm-utxTMevNn4KVkwEpVnsFO~6HKCDFjl7M9qA,AQACAAE/temp/";
String freenetUploadUSKURI = "USK@UKFP9LwXBm1G8CAM~9QuZ~WXEVWvtV6voNQmRjeOHSg,db8UCXm-utxTMevNn4KVkwEpVnsFO~6HKCDFjl7M9qA,AQECAAE/temp/";
String freenetReceiverURI = "";
String freenetUploadReceiverURI = "";
String freenetArduinoUploadURI = "";
String freenetArduinoReceiverURI = "";
String freenetPrivReceiverURI = "";

int requestCounter = 0;
int responseCounter = 0;

//Variables for ECDH
const struct uECC_Curve_t * curve = uECC_secp256r1();
  uint8_t prvKey[32];
  uint8_t pubKey[64];
  uint8_t sharedSecret[32];

//Current Status Boolean
bool initConnection = true;
bool getReceiverUri = true;
bool sendPubKeySender = false;
bool receivePubKeyReceiver = false;
bool generateNewUri = false;
bool sendNewUriEncrypted = false;
bool receiveNewUriEncrypted = false;
bool didReceiveSensorData = true;
  
//Initialisation of FreenetClient and SD Card
fcpClient fcpClient(server,9481);
sdCard sdCard(10);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!Serial){
    ;
  }
  //Init Sensor
  dht.begin();

  //Set RNG of the ECC
  uECC_set_rng(&RNG);

  //Check Wifi Status Connection
  if (WiFi.status() == WL_NO_MODULE){
    Serial.println("Communcation with WiFi module failed!");
    while (true);
  }
  
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  printWifiStatus();
  
  fcpClient.connectClient();
}

void loop() {
  fcpClient.doClientHello();
  if(fcpClient.isConnected){
    if(initConnection){
      if(getReceiverUri){
        Serial.println("Start new ConnectionRequest");
        getNewConnectionRequest();
        Serial.println("End new ConnectionRequest");
      }
      if(sendPubKeySender){
        Serial.println("Start Upload PublicECCKey");
        sendPublicKeySender();
        Serial.println("End Upload PublicECCKey");
      }
      if(receivePubKeyReceiver){
        Serial.println("Start Receive PublicECCKey");
        getPublicKeyReceiver();
        Serial.println("End Receive PublicECCKey");
      }
      if(generateNewUri){
        Serial.println("Start Generate newURIs");
        getNewFreenetURI();
        Serial.println("End Generate newURIs");
      }
      if(sendNewUriEncrypted){
        Serial.println("Start sendUriCrypted");
        sendNewUriCrypted();
        Serial.println("End sendUriCrypted");
      }
      if(receiveNewUriEncrypted){
        Serial.println("Start receiveUriCrypted");
        receiveNewUriCrypted();
        Serial.println("End receiveUriCrypted");
      }      
    }else{
      if(didReceiveSensorData){
        Serial.println("Start sendSensorData");
        sendSensorDataCrypted();
        Serial.println("End sendSensorData");
      }else{
        Serial.println("Start receiveConfirmation");
        checkForReceiveConfirmation();
        Serial.println("End receiveConfirmation");
      }
    }
  }else{
    fcpClient.connectClient();
  }
  delay(1000);
  Serial.println("Repeat");
}

void getNewConnectionRequest(){
  String data = fcpClient.doClientGet(freenetRequestUSKURI + requestCounter, "ArduinoTemp");
  freenetReceiverURI = data.substring(data.indexOf("Uri:")).substring(4);
  if(freenetReceiverURI != ""){
    getReceiverUri = false;
    sendPubKeySender = true;
    requestCounter++;
  }
}

void sendPublicKeySender(){
  GenerateECDHKeys();
  rbase64.encode(pubKey, sizeof(pubKey));
  String pubBase64 = rbase64.result();
  fcpClient.doClientPut(freenetReceiverURI  + responseCounter, "TempArduino", "Pub:"+String(pubBase64.length()), pubBase64);
  delay(500);
  String data = doReadFcp();
  sendPubKeySender = false;
  receivePubKeyReceiver = true;
  responseCounter++;
}

void getPublicKeyReceiver(){
  String data = fcpClient.doClientGet(freenetRequestUSKURI + requestCounter, "ArduinoTemp");
  String base64Key = data.substring(data.indexOf("Pub:")).substring(4);
  if(base64Key != ""){
    rbase64.decode(base64Key);
    char * pubkeyRec = rbase64.result();
    int r = uECC_shared_secret((uint8_t*)pubkeyRec,prvKey,sharedSecret, curve);
    if(r > 0){
      receivePubKeyReceiver = false;
      generateNewUri = true;
      requestCounter++;
    }
  }
}

void getNewFreenetURI(){
  String data = fcpClient.doGenerateSSK("ArduinoTemp");
  if(data != ""){
    freenetArduinoUploadURI = getValue(data, '\n', 2);
    freenetArduinoReceiverURI = getValue(data, '\n', 3);
    freenetArduinoUploadURI = "USK" + freenetArduinoUploadURI.substring(13) + "temp/";
    freenetArduinoReceiverURI = "USK" + freenetArduinoReceiverURI.substring(14)+ "temp/";

    generateNewUri = false;
    sendNewUriEncrypted = true;
  }
}

void sendNewUriCrypted(){
  int str_len = freenetArduinoUploadURI.length() + 1;
  char char_array[str_len];
  freenetArduinoUploadURI.toCharArray(char_array, str_len);
  
  aes256_enc_single(sharedSecret, char_array);
  
  rbase64.encode((uint8_t*)char_array, sizeof(char_array));
  String pubBase64 = rbase64.result();
  fcpClient.doClientPut(freenetReceiverURI  + responseCounter, "TempArduino", "PrvURI:" + String(pubBase64.length()), pubBase64);
  delay(500);
  String data = doReadFcp();
  sendNewUriEncrypted = false;
  receiveNewUriEncrypted = true;
  responseCounter++;
}

void receiveNewUriCrypted(){
  requestCounter = 0;
  String data = fcpClient.doClientGet(freenetArduinoReceiverURI + requestCounter, "ArduinoTemp");
  String base64Key = data.substring(data.indexOf("PrvURI:")).substring(7);
  if(base64Key != ""){
    rbase64.decode(base64Key);
    String crypted = rbase64.result();
    int str_len = crypted.length() + 1;
    char char_array[str_len];
    crypted.toCharArray(char_array, str_len);
    aes256_dec_single(sharedSecret, char_array);
    freenetPrivReceiverURI = char_array;
    if(freenetPrivReceiverURI.indexOf("USK:" >= 0)){
      receiveNewUriEncrypted = false;
      initConnection = true;
      requestCounter++;
      responseCounter = 0;
    }
  }
}

void sendSensorDataCrypted(){
  String sensorData = "Data:" + getSensorData();
  int str_len = sensorData.length() + 1;
  char char_array[str_len];
  sensorData.toCharArray(char_array, str_len);
  aes256_enc_single(sharedSecret,char_array);
  
  rbase64.encode((uint8_t*)char_array, sizeof(char_array));
  String pubBase64 = rbase64.result();
  fcpClient.doClientPut(freenetPrivReceiverURI  + responseCounter, "TempArduino", String(pubBase64.length()), pubBase64);
  delay(500);
  String data = doReadFcp();
  didReceiveSensorData = false;
  responseCounter++;
}

void checkForReceiveConfirmation(){
  String data = fcpClient.doClientGet(freenetArduinoReceiverURI + requestCounter, "ArduinoTemp");
  if(data != ""){
    rbase64.decode(data);
    String receive = rbase64.result();
    int str_len = receive.length() + 1;
    char char_array[str_len];
    receive.toCharArray(char_array, str_len);
    aes256_dec_single(sharedSecret,char_array);
    String decryptedContent = char_array;
    if(decryptedContent.indexOf("ReceiveConfirmation" >= 0)){
      didReceiveSensorData = true;
      requestCounter++;
    }
  }
}

String getSensorData(){
  return "Data:" + (String)dht.readTemperature();
}

String doReadFcp(){
  String readValue = "";
  while (readValue == ""){
    delay(500);
    readValue = fcpClient.doRead();
    Serial.println("TryRead: "+readValue);
  }
  return readValue;
}

void GenerateECDHKeys(){
  uECC_make_key(pubKey, prvKey, curve);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

static int RNG(uint8_t *dest, unsigned size) {
  // Use the least-significant bits from the ADC for an unconnected pin (or connected to a source of 
  // random noise). This can take a long time to generate random data if the result of analogRead(0) 
  // doesn't change very frequently.
  while (size) {
    uint8_t val = 0;
    for (unsigned i = 0; i < 8; ++i) {
      int init = analogRead(0);
      int count = 0;
      while (analogRead(0) == init) {
        ++count;
      }
      
      if (count == 0) {
         val = (val << 1) | (init & 0x01);
      } else {
         val = (val << 1) | (count & 0x01);
      }
    }
    *dest = val;
    ++dest;
    --size;
  }
  // NOTE: it would be a good idea to hash the resulting random data using SHA-256 or similar.
  return 1;
}

String getValue(String data, char separator, int index)
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
