#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "FirebaseESP32.h"

FirebaseData firebaseData;

WiFiClient esp32Client;

const char* wifi   = "ifce-alunos";
const char* wsenha = "ifce4lun0s";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800,60000);

// Variables to save date and time
String daystamp;
String timestamp;

const int led1 = 22; //dispositivoFechado
const int led2 = 21; //internet
const int led = 23; //dispositivoAberto


int scanTime = 2;
int nivelRSSI = -80;

String dispositivosAutorizados;
String nome;
bool dispositivoPresente = false;
String path = "/Permissao";
String path2 = "/Code";
String path3 = "/User";
int codigo = 0;
const char* result;
unsigned long ultimoTempoMedido = 0;
const long intervaloPublicacao = 20000;
static BLEAddress *pServerAddress;
BLEScan* pBLEScan;
BLEClient*  pClient;

const char* Broker = "https://controle-residencial-fae98.firebaseio.com";
const char* firebase_key = "o41PKeyROoVbLoChQvv0Prjopx2NtfzdR6KbhL0Y";

void setup() {
  
  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT);
  pinMode(led,OUTPUT);
  Serial.begin(115200);
  digitalWrite(led1, HIGH);
  connectWifi();
  Firebase.begin(Broker, firebase_key);
  BLEDevice::init("");
  
  timeClient.begin();
  timeClient.setTimeOffset(-10800);}

void connectWifi(){
    WiFi.begin(wifi, wsenha);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //piscar();        
        }
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.print("IP: ");Serial.println(WiFi.localIP());
    digitalWrite(led2, HIGH);
}

void searchUser(){
    if (Firebase.getInt(firebaseData, path2+"/number")) {
      if  (firebaseData.dataType() == "int") {
        codigo = firebaseData.intData();
        if (Firebase.getString(firebaseData, path3 + "/"+codigo+"/mac")) {
          if  (firebaseData.dataType() == "string") {
            dispositivosAutorizados = firebaseData.stringData();
          }
        }

        if (Firebase.getString(firebaseData, path3 + "/"+codigo+"/nome")) {
          if  (firebaseData.dataType() == "string") {
            nome = firebaseData.stringData();
          }
        }
      }
    }
}

void loop() {
    Serial.println("BUSCANDO DISPOSITIVO... ");
    searchUser();
    scanBLE();
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      String dispositivosEncontrados = advertisedDevice.getAddress().toString().c_str();
      Serial.println(advertisedDevice.toString().c_str());
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      if ( dispositivosEncontrados == dispositivosAutorizados  && advertisedDevice.getRSSI() < nivelRSSI) {
        Serial.println("DISPOSITIVO PROXIMO!");
        Serial.print("RSSI: ");Serial.println(advertisedDevice.getRSSI());
        piscarProximo();
      }
      if ( dispositivosEncontrados == dispositivosAutorizados  && advertisedDevice.getRSSI() > nivelRSSI) {
        digitalWrite(led1, LOW);
        Serial.println("DISPOSITIVO ENCONTRADO!");
        Serial.print("RSSI: ");Serial.println(advertisedDevice.getRSSI());
        dispositivoPresente = true;
        ultimoTempoMedido = millis();
        Serial.println("PERMITIDO O ACESSO!");
        publisherAccess();
      }
      else{
        dispositivoPresente = false;
      }
    }
};

void scanBLE(){
  // Scanner do BLE
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  BLEScanResults foundDevices = pBLEScan->start(scanTime);}

void publisherAccess(){
  // Print código
  Serial.print("Código: ");Serial.println(codigo);

  timeClient.update();
  // Extract date
  timestamp = timeClient.getFormattedTime();
  Serial.print("Hora: ");Serial.println(timestamp);
  
  String formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  daystamp = formattedDate.substring(0, splitT);
  Serial.print("Data: ");Serial.println(daystamp);

  // Armazenando no Firebase
  Firebase.setString(firebaseData, path + "/"+codigo+"/nome", nome);
  Firebase.setString(firebaseData, path + "/"+codigo+"/data", daystamp);
  Firebase.setString(firebaseData, path + "/"+codigo+"/hora", timestamp);
  Firebase.setBool(firebaseData, path + "/"+codigo+"/acesso", dispositivoPresente);

  digitalWrite(led, HIGH);
  delay(5000);
  digitalWrite(led, LOW);
  delay(200);
  digitalWrite(led1, HIGH);
}

void piscarProximo()//piscar do led quando menor do que o RSSI -80
  {
  digitalWrite(led, HIGH);   
  delay(200);
  digitalWrite(led, LOW);   
  delay(200);  
  digitalWrite(led, HIGH);   
  delay(200);
  digitalWrite(led, LOW);
}
