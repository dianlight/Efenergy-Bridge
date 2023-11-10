#include <Arduino.h>
#include "log.h"

#include <ArduinoJson.h>
#include <rtl_433_ESP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>  
#include <WebSerial.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Ticker.h>
#include <PubSubClient.h>


//#define LOCAL_DEBUG       true  //false

AsyncWebServer server(80);
DNSServer dns;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Ticker mqttReconnectTimer;



const char *hostname = "efergybridge";
const char *datapath = "%s/sensors/%s/state";
const char *discoverypath = "homeassistant/sensor/%s/%s/config";

#define DATAPATH(buffer, id)                                     \
  char buffer[strlen(datapath) + strlen(hostname) + strlen(id)]; \
  sprintf(buffer, datapath, hostname, id);
#define DISCOVERYPATH(buffer,id)  char buffer[strlen(discoverypath)+strlen(hostname)+strlen(id)]; sprintf(buffer,discoverypath,hostname,id); 

#ifndef RF_MODULE_FREQUENCY
#  define RF_MODULE_FREQUENCY 433.54 // 433.92 
#endif

#define JSON_MSG_BUFFER 512

#define MQTT_HOST IPAddress(192, 168, 0, 250)
#define MQTT_PORT 1883
#define MQTT_USER "efergy"
#define MQTT_PASSWORD "ef2023"



char messageBuffer[JSON_MSG_BUFFER];

rtl_433_ESP rf; // use -1 to disable transmitter


void connectToMqtt() {
  if(!mqttClient.connected()){
    WebSerial.println("Connecting to MQTT...");
    if(!mqttClient.connect(hostname,MQTT_USER,MQTT_PASSWORD)){
      WebSerial.printf("E:Connectiong %d\n", mqttClient.state());
    }
    mqttReconnectTimer.once(2, connectToMqtt);
  } else {
    mqttReconnectTimer.once(10, connectToMqtt);
  }
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len){
  //WebSerial.print("Received Data... ");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  //WebSerial.println(d);

  switch(data[0]){
    case 'h':
      WebSerial.printf("* Help %u\n",COMPILE_TIME);
      WebSerial.println("i = info");
      WebSerial.println("r = restart");
      WebSerial.println("c = reset MQTT connection");
    break;
    case 'i':
    case 'I':
      WebSerial.println("* Software");
      WebSerial.printf("Version %u\n",COMPILE_TIME);

      WebSerial.println("* Network");
      WebSerial.printf("IP %s\n",WiFi.localIP().toString());
      WebSerial.printf("RSSI %d\n",WiFi.RSSI());

      WebSerial.println("* SX1278");
      WebSerial.printf("Modulation: %s\n", rf.ookModulation ? "OOK" : "FSK");
      WebSerial.printf("Frequency: %.2fMhz\n", RF_MODULE_FREQUENCY);
      WebSerial.printf("Signal RSSI: %d\n", rf.signalRssi);
      WebSerial.printf("MessageCount: %d\n", rf.messageCount);
      WebSerial.printf("TotalSignals: %d\n", rf.totalSignals);
      WebSerial.printf("IgnoredSignals: %d\n", rf.ignoredSignals);
      WebSerial.printf("UnparsedSignals: %d\n", rf.unparsedSignals);
      WebSerial.printf("CurrentRssi: %d\n", rf.currentRssi);
      WebSerial.printf("RssiThreshold: %d\n", rf.rssiThreshold);

      WebSerial.println("* MQTT");
      WebSerial.printf("Status: %s\n",mqttClient.connected()?"Connected":"Disconnected");
      WebSerial.printf("State: %d\n",mqttClient.state());

       WebSerial.println();
    break;
    case 'r':
    case 'R':
      WebSerial.println("Restart in 5sec!");
      delay(5000);
      ESP.restart();
      break;
    case 'c':
    case 'C':
      mqttClient.disconnect();
      break;
    default:
      WebSerial.printf("Unknown cmd '%c'\n",char(data[0]));
    break;
  }

  //rf.getModuleStatus();
}

void publishJson(JsonObject& jsondata,char *topic) {
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  char JSONmessageBuffer[jsondata.measureLength() + 1];
#else
  char JSONmessageBuffer[JSON_MSG_BUFFER];
#endif
  jsondata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
 // Log.notice(F("Received message : %s" CR), JSONmessageBuffer);
  WebSerial.printf("\n%s : %s\n",topic, JSONmessageBuffer);
  if(!mqttClient.publish(topic, JSONmessageBuffer, true)){
    WebSerial.println("E: MQTT");
  };
}

/**
 {
  "model":"Efergy-e2CT",
  "id":14374,
  "battery_ok":1,
  "current":9.29932,
  "interval":6,
  "learn":"NO",
  "mic":"CHECKSUM",
  "protocol":"Efergy e2 classic",
  "rssi":-102,
  "duration":18000}
*/
void rtl_433_Callback(char* message) {
  DynamicJsonBuffer jsonBuffer2(JSON_MSG_BUFFER);
  JsonObject& RFrtl_433_ESPdata = jsonBuffer2.parseObject(message);
  if(strcmp(RFrtl_433_ESPdata["model"],"undecoded signal") == 0){
    //WebSerial.print(".");
    DATAPATH(topic,"failed");
    publishJson(RFrtl_433_ESPdata, topic);
  } else {
    WebSerial.print("+");
    if(strcmp(RFrtl_433_ESPdata["protocol"],"Efergy e2 classic") == 0) {
      if(RFrtl_433_ESPdata["current"].is<double>()) RFrtl_433_ESPdata["watts"] = RFrtl_433_ESPdata["current"].as<double>() * 220.0; 
    }
    DATAPATH(topic,RFrtl_433_ESPdata["id"].as<String>().c_str());
    publishJson(RFrtl_433_ESPdata, topic);
  }
}


void WiFiEvent(WiFiEvent_t event){
  //  Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            // WebSerial is accessible at "<IP Address>/webserial" in browser
            WebSerial.begin(&server);
            // MQTT
            connectToMqtt();
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
            break;
          default: break;
    }
}

void setup() {
  //Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  AsyncWiFiManager wifiManager(&server,&dns);
  wifiManager.autoConnect();
  
  /* Attach Message Callback */
  WebSerial.msgCallback(recvMsg);

  /* OTA */
// Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(hostname);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      //Serial.println("Start updating " + type);
      WebSerial.println("Start updating " + type);
      rf.disableReceiver();
      mqttReconnectTimer.detach(); })
      .onEnd([]()
             {
     // Serial.println("\nEnd");
      WebSerial.println("\nEND"); })
      .onProgress([](unsigned int progress, unsigned int total)
              {
        static uint8_t prct = 0xFF;
        if( (progress / (total / 100)) != prct ){
          prct = progress / (total / 100);
          if(prct % 5 == 0)WebSerial.printf("Progress: %u%%\r",prct); 
        }
              })
      .onError([](ota_error_t error)
               {
      //Serial.printf("Error[%u]: ", error);
      WebSerial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) WebSerial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) WebSerial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) WebSerial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) WebSerial.println("Receive Failed");
      else if (error == OTA_END_ERROR) WebSerial.println("End Failed"); });

  ArduinoOTA.begin();  /* Start WebServer */
  server.begin();  

  rf.initReceiver(RF_MODULE_RECEIVER_GPIO, RF_MODULE_FREQUENCY);
  rf.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);

  rf.enableReceiver();

  rf.getModuleStatus();
}

void loop() {
  rf.loop();
  ArduinoOTA.handle();
  mqttClient.loop();
}