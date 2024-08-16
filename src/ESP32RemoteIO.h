/*
######################################################################
##      Integração das tecnologias da REMOTE IO com Node IOT        ##
##                          Versão 1.0                              ##
##   Código base para implementação de projetos de digitalização de ##
##   processos, automação, coleta de dados e envio de comandos com  ##
##   controle embarcado e na nuvem.                                 ##
##                                                                  ##
######################################################################
*/

#ifndef ESP32RemoteIO_h
#define ESP32RemoteIO_h

#define JSON_DOCUMENT_CAPACITY 4096

#define INICIALIZATION 0    
#define CONNECTED 1         
#define NO_WIFI 2           
#define DISCONNECTED 3      

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <ArduinoOTA.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>

class RemoteIO 
{
  public:
    RemoteIO();
    void begin();
    void loop();
    void updatePinOutput(String ref);
    void updatePinInput(String ref);
    int espPOST(String variable, String value);

    JsonObject setIO;
    
  private:
    void notFound(AsyncWebServerRequest *request);
    void localHttpUpdateMsg(String ref, String value);
    void tryAuthenticate();    
    void fetchLatestData();
    void browseService(const char * service, const char * proto);
    void switchState();
    void stateLogic();
    void socketIOConnect();
    void nodeIotConnection();
    void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length);
    void extractIPAddress(String url);
    void startAccessPoint();
    void checkResetting(long timeInterval);
    int espPOST(String Router, String variable, String value);

    StaticJsonDocument<JSON_DOCUMENT_CAPACITY> configurationDocument;
    JsonArray configurations;

    Preferences* deviceConfig;
    
    SocketIOclient socketIO;
    AsyncWebServer* server;

    bool Connected;
    int Socketed;
    unsigned long messageTimestamp;

    String _ssid;
    String _password;
    String _companyName;
    String _deviceId;
    String _appHost;
    String _model;
    uint16_t _appPort;

    String anchor_route;
    String anchored_route;
    
    String appBaseUrl;
    String appVerifyUrl;
    String appLastDataUrl;
    String appSideDoor;
    String appPostData;
    String appPostDataFromAnchored;

    long start_debounce_time;
    long start_browsing_time;
    long start_reconnect_time;
    long start_config_time; 
    long start_reset_time;

    String state;
    String token;

    String anchor_IP;
    String anchored_IP;
    String send_to_niot_buffer;
    String send_to_anchor_buffer;
    String send_to_anchored_buffer;

    int connection_state;
    int next_state;

    bool anchored;
    bool anchoring;
    int lastIP_index;
    int reconnect_counter;
};

#endif // ESP32RemoteIO_h



