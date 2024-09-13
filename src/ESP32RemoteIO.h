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

#define VERSION "1.2.6"
#define OTA_JSON_URL "https://nodeiot-firmware.s3.amazonaws.com/bin/esp32/esp32remoteio.json" // this is where you'll post your JSON filter file

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
#include <time.h>
#include <ESP32OTAPull.h>

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
    static void IRAM_ATTR interruptCallback(void* arg);
    static void inputTimerCallback(void* arg);
    static void outputTimerCallback(void* arg);
    void notFound(AsyncWebServerRequest *request);
    void localHttpUpdateMsg(String ref, String value);
    void tryAuthenticate();    
    void fetchLatestData();
    void browseService(const char* service, const char* proto);
    void sendDataFromQueue();
    void switchState();
    void stateLogic();
    void socketIOConnect();
    void nodeIotConnection();
    void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length);
    void extractIPAddress(String url);
    void startAccessPoint();
    void checkResetting(long timeInterval);
    void updateEventArray();
    void getEvents();
    void setTimer();
    int espPOST(JsonDocument arrayDoc);
    int espPOST(String Router, String variable, String value);

    ESP32OTAPull ota;

    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args;

    StaticJsonDocument<JSON_DOCUMENT_CAPACITY> configurationDocument;
    JsonArray configurations;
    
    JsonDocument event_doc;
    JsonArray event_array;

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

    const char* ntp_server1 = "pool.ntp.org";
    const char* ntp_server2 = "time.nist.gov";

    long start_debounce_time;
    long start_browsing_time;
    long start_reconnect_time;
    long start_config_time; 
    long start_reset_time;
    
    long gmtOffset_sec;
    int daylightOffset_sec;
    struct tm timeinfo;

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



