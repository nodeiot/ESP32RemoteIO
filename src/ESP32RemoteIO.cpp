/*
######################################################################
##      Integração das tecnologias da REMOTE IO com Node IOT        ##
##                          Version 1.0                             ##
##   Código base para implementação de projetos de digitalização de ##
##   processos, automação, coleta de dados e envio de comandos com  ##
##   controle embarcado e na nuvem.                                 ##
##                                                                  ##
######################################################################
*/

#include "ESP32RemoteIO.h"
#include "index_html.h"

void callback(int offset, int totallength);

typedef struct interrupt_data 
{
  RemoteIO* remoteio_pointer;
  String ref_arg;
} interrupt_data;

typedef struct event_data
{
  RemoteIO* remoteio_pointer;
  JsonDocument* actions_arg;
} event_data;

DynamicJsonDocument post_data_queue(1024);
unsigned long last_queue_sent_time = 0;

volatile bool timer_expired;

RemoteIO::RemoteIO()
{
  _appPort = 5000;

  server = new AsyncWebServer(80);
  deviceConfig = new Preferences();

  anchor_route = "http://anchor_IP/post-message";
  anchored_route = "http://anchored_IP/post-message";
  appBaseUrl = "https://api.nodeiot.app.br/api";
  appVerifyUrl = appBaseUrl + "/devices/verify";
  appPostData = appBaseUrl + "/broker/data/";
  appPostMultiData = appBaseUrl + "/broker/multidata";
  appSideDoor = appBaseUrl + "/devices/devicedisconnected";
  appPostDataFromAnchored = appBaseUrl + "/broker/ahamdata";

  timer_expired = false;

  state = "";
  token = "";
    
  configurations = configurationDocument.to<JsonArray>();
  setIO = configurations.createNestedObject();

  event_array = event_doc.to<JsonArray>();

  local_mode = false;
  Connected = false;
  Socketed = 0;
  messageTimestamp = 0;

  connection_state = INICIALIZATION;
  next_state = INICIALIZATION;

  start_debounce_time = 0;
  start_browsing_time = 0;
  start_reconnect_time = 0;
  start_reset_time = 0;

  lastIP_index = -1;
  anchored = false;
  anchoring = false;
  reconnect_counter = 0;
}

void RemoteIO::openLocalServer()
{
  Serial.println("[openLocalServer] Opening local http endpoints");

  server->on("/network", HTTP_GET, [this](AsyncWebServerRequest *request) {
    deviceConfig->begin("deviceConfig", false);
    request->send_P(200, "text/html", page_network); // page_network
  });
  
  server->on("/submit-network", HTTP_GET, [this](AsyncWebServerRequest *request) {

    if (request->hasParam("ssid") && request->hasParam("password"))
    {
      String arg_ssid = request->getParam("ssid")->value();
      String arg_password = request->getParam("password")->value();

      Serial.printf("[/submit-network] Trying connection on %s\n", arg_ssid);
      WiFi.begin(arg_ssid, arg_password);
      
      unsigned long startTime = millis();
      const unsigned long timeout = 3000;
      bool connected = false;
      wl_status_t status;

      while (millis() - startTime < timeout) 
      {
        status = WiFi.status();
        if (status == WL_CONNECTED) 
        {
          connected = true;
          break;
        } 
        delay(500); // Ajuste o atraso para 500ms
        esp_task_wdt_reset(); // Alimenta o watchdog timer
      }

      if (status == WL_NO_SSID_AVAIL || status == WL_CONNECT_FAILED || status == WL_CONNECTION_LOST || status == WL_DISCONNECTED)
      {
        Serial.println("[/submit-network] Failed connecting...");
        request->send_P(200, "text/html", page_network_fail); // tentar informar qual foi o erro ocorrido
        delay(2000);
      }
      else if (status == WL_CONNECTED)
      {
        String wifi_array_doc;
        JsonDocument array;
        JsonDocument obj;

        deserializeJson(array, deviceConfig->getString("wifi_array_doc", ""));
        obj["ssid"] = arg_ssid;
        obj["password"] = arg_password;

        _ssid = arg_ssid;
        _password = arg_password;
        
        if (array.size() >= 5) array.remove(0);
        array.add(obj);
        serializeJson(array, wifi_array_doc);

        deviceConfig->putString("wifi_array_doc", wifi_array_doc);
        deviceConfig->end();

        array.clear();
        obj.clear();
        wifi_array_doc.clear();

        Serial.println("[/submit-network] Success connecting...");
        Serial.printf("[/submit-network] WiFi connected %s\n", WiFi.localIP().toString().c_str());

        request->send_P(200, "text/html", page_network_success);
        delay(2000);
      }
    }
    else
    {
      request->send(400, "text/plain", "Parâmetros ausentes");
      delay(1000);
    }
  });

  server->on("/account", HTTP_GET, [this](AsyncWebServerRequest *request) {
    deviceConfig->begin("deviceConfig", false);
    request->send_P(200, "text/html", page_account); // page_account
  });

  server->on("/submit-account", HTTP_GET, [this](AsyncWebServerRequest *request) {

    if (request->hasParam("companyName")) 
    {
      String arg_deviceId;
      String arg_companyName = request->getParam("companyName")->value();
      _companyName = arg_companyName;

      if (request->hasParam("deviceId")) 
      {
        arg_deviceId = request->getParam("deviceId")->value();
        deviceConfig->putString("deviceId", arg_deviceId);
        _deviceId = arg_deviceId;
      }

      deviceConfig->putString("companyName", arg_companyName);
      deviceConfig->end();
      Serial.println("[/submit-account] Informações de conta recebidas!");
      request->send_P(200, "text/html", page_account_success);

      time_t now = millis();
      while (millis() - now <= 2000)
      {
        //Serial.print("."); delay...
      }
      ESP.restart();
    }
    else
    {
      Serial.println("[/submit-account] Informações de conta ausentes!");
      request->send_P(200, "text/html", page_account_fail);
    }
  });

  server->on("/monitor-data", HTTP_GET, [this](AsyncWebServerRequest *request) {

    JsonDocument monitor_doc;
    String wifi_state; 
    uint32_t flashSize = 0;
    esp_flash_get_size(NULL, &flashSize);
    flashSize = flashSize / 1024;

    if (WiFi.status() == WL_CONNECTED) wifi_state = "Conectado";
    else wifi_state = "Desconectado";

    if (state == "accepted") monitor_doc["NodeIoT"]["authentication"] = "Verificado";
    else monitor_doc["NodeIoT"]["authentication"] = "Não verificado";

    if (Connected) monitor_doc["NodeIoT"]["connection"] = "Conectado";
    else monitor_doc["NodeIoT"]["connection"] = "Desconectado";

    char timeString[64];
    if (!getLocalTime(&timeinfo))
    {
      sprintf(timeString, "Desconectado");
    }
    else
    {
      strftime(timeString, sizeof(timeString), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    }
    
    monitor_doc["Wi-Fi"]["ssid"] = _ssid;
    monitor_doc["Wi-Fi"]["ipLocal"] = WiFi.localIP().toString();
    monitor_doc["Wi-Fi"]["state"] = wifi_state;
    monitor_doc["Wi-Fi"]["rssi"] = WiFi.RSSI();
    monitor_doc["RemoteIO"]["model"] = _model;
    monitor_doc["RemoteIO"]["memory"] = flashSize;
    monitor_doc["RemoteIO"]["version"] = VERSION;
    monitor_doc["RemoteIO"]["localTime"] = String(timeString);
    monitor_doc["NodeIoT"]["companyName"] = _companyName;
    monitor_doc["NodeIoT"]["deviceId"] = _deviceId;

    String monitor_info;
    serializeJson(monitor_doc, monitor_info);
    monitor_doc.clear();
    
    request->send(200, "application/json", monitor_info);
  });

  server->on("/monitor", HTTP_GET, [this](AsyncWebServerRequest *request) {
      
    request->send_P(200, "text/html", page_monitor);
  });

  server->on("/monitor-reset", HTTP_GET, [this](AsyncWebServerRequest *request) {
      
    deviceConfig->begin("deviceConfig", false);
    deviceConfig->clear();
    deviceConfig->end();
    request->send(200, "text/plain", "Reset de credenciais efetuado com sucesso! Reiniciando dispositivo...");
    delay(1000);
    ESP.restart();
  });

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/post-message", [this](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<250> data;
    String response;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }
    
    WebSerial.print("[AsyncCallback]: ");
    WebSerial.println(data.as<String>());

    if (data.containsKey("status"))
    {
      if (connection_state == CONNECTED)
      {
        data.remove("status");
        data["ipAddress"] = request->client()->remoteIP().toString();

        serializeJson(data, send_to_niot_buffer);

        if (espPOST(appSideDoor, "", send_to_niot_buffer) == HTTP_CODE_OK)
        {
          send_to_niot_buffer.clear();
          data.clear();
          
          if (anchoring) 
          {
            data["msg"] = "ok";
            anchoring = false;
          }
          else data["msg"] = "received";

          serializeJson(data, response);
          request->send(200, "application/json", response);
        }
        else 
        {
          send_to_niot_buffer.clear();
          data.clear();
          data["msg"] = "post to niot failed";
          serializeJson(data, response);
          request->send(500, "application/json", response);
        }
      }
      else 
      {
        if (request->client()->remoteIP().toString() == anchor_IP) 
        {
          anchor_IP.clear();
          anchored = false;
        }

        data.clear();
        data["msg"] = "disconnected";
        serializeJson(data, response);
        request->send(500, "application/json", response);
      }
    }
    else if ((data.containsKey("ref")) && (!data.containsKey("deviceId")) && (connection_state != CONNECTED))
    {
      anchor_IP.clear();
      anchor_IP = request->client()->remoteIP().toString();
      anchored = true;
      
      String ref = data["ref"].as<String>();
      setIO[ref]["value"] = data["value"];
      
      if (setIO[ref]["type"] == "OUTPUT")
      {
        updatePinOutput(ref);
      }

      data.clear();
      data["msg"] = "ok";
      serializeJson(data, response);
      request->send(200, "application/json", response);

      if (ref == "restart") ESP.restart();
      else if (ref == "reset")
      {
        deviceConfig->begin("deviceConfig", false);
        deviceConfig->clear();
        deviceConfig->end();
        delay(1000);
        ESP.restart();
      }
    }
    else if (data.containsKey("deviceId"))
    {
      if (connection_state == CONNECTED)
      {
        serializeJson(data, send_to_niot_buffer);
        if (espPOST(appPostDataFromAnchored, "", send_to_niot_buffer) == HTTP_CODE_OK)
        {
          send_to_niot_buffer.clear();
          data.clear();
          data["msg"] = "ok";
          serializeJson(data, response);
          request->send(200, "application/json", response);
        }
        else 
        {
          send_to_niot_buffer.clear();
          data.clear();
          data["msg"] = "post to niot failed";
          serializeJson(data, response);
          request->send(500, "application/json", response);
        }
      }
      else 
      {
        data.clear();
        data["msg"] = "disconnected";
        serializeJson(data, response);
        request->send(500, "application/json", response);
      }
    }
    else 
    {
      data.clear();
      data["msg"] = "unhandled message";
      serializeJson(data, response);
      request->send(500, "application/json", response);
    }
  });

  WebSerial.begin(server);
  
  WebSerial.onMessage([this](uint8_t *data, size_t len) {
    Serial.printf("Received %lu bytes from WebSerial: ", len);
    Serial.write(data, len);
    Serial.println();
    WebSerial.println("Received Data...");
    String d = "";
    
    for(size_t i = 0; i < len; i++)
    {
      d += char(data[i]);
    }
    WebSerial.println(d);

    if (d == "restart") rebootDevice();
    else if (d == "reset") eraseDeviceSettings();
  });

  WebSerial.setAuthentication("remoteio", "12345678");
  MDNS.addService("http", "tcp", 80);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  server->addHandler(handler);
  server->onNotFound(std::bind(&RemoteIO::notFound, this, std::placeholders::_1));
  server->begin(); 
}

void RemoteIO::begin(void (*userCallbackFunction)(String ref, String value))
{
  storedCallbackFunction = userCallbackFunction;
  Serial.begin(115200);
  
  deviceConfig->begin("deviceConfig", false);
  if (!SPIFFS.begin(true))
  {
    Serial.println("[begin] An error has occurred while mounting SPIFFS");
    return;
  }

  String NVS_WIFI_ARRAY = deviceConfig->getString("wifi_array_doc", "");
  _companyName = deviceConfig->getString("companyName", "");
  _deviceId = deviceConfig->getString("deviceId", "");

  getPCBModel();
  deviceConfig->end();

  startAccessPoint();
  openLocalServer();

  if (NVS_WIFI_ARRAY.length() > 0)
  {
    nodeIotConnection(userCallbackFunction);
  }
}

void callback(int offset, int totallength)
{
  Serial.printf("Updating %d of %d (%02d%%)...\n", offset, totallength, 100 * offset / totallength);
}

void RemoteIO::checkResetting(long timeInterval)
{
  if (digitalRead(setIO["reset"]["pin"].as<int>()) == LOW)
  {
    if (start_reset_time == 0) start_reset_time = millis();
    else if (millis() - start_reset_time >= timeInterval)
    {
      deviceConfig->begin("deviceConfig", false);
      deviceConfig->clear();
      deviceConfig->end();
      delay(1000);
      ESP.restart();
    }
  }
  else start_reset_time = 0;
}

void RemoteIO::getPCBModel()
{
  File file = SPIFFS.open("/model.json", "r");
  
  if (!file)
  {
    WebSerial.println("[getPCBModel] Failed to open file for reading");
  }

  JsonDocument document;
  deserializeJson(document, file);
  file.close();
  
  if (document["model"].as<String>() == "") 
  {
    deviceConfig->putString("model", "ESP_32");
    _model = "ESP_32";
  }
  else 
  {
    deviceConfig->putString("model", document["model"].as<String>());
    _model = document["model"].as<String>();
  }
}

void RemoteIO::startAccessPoint()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);

  IPAddress apIP(192, 168, 4, 1);

  bool result = WiFi.softAP("RemoteIO");
  if (!result) 
  {
    Serial.println("Erro ao configurar o ponto de acesso");
    ESP.restart();
  }
  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  IPAddress IP = WiFi.softAPIP();
  Serial.print("[startAccessPoint] IP: ");
  Serial.println(IP);

  String LOCAL_DOMAIN = String("remoteio-device");

  if (!MDNS.begin(LOCAL_DOMAIN)) 
  {
    Serial.println("Erro ao configurar o mDNS");
  }
}

void RemoteIO::sendDataFromQueue()
{
  if (post_data_queue.size() >= 1)
  {
    last_queue_sent_time = millis();
    espPOST(post_data_queue);
    post_data_queue.clear();
  }
}

void RemoteIO::loop()
{
  ArduinoOTA.handle();
  WebSerial.loop();
  switchState();
  stateLogic();
  //checkResetting(5000); // millisegundos
  updateEventArray();
  sendDataFromQueue();
}

void RemoteIO::browseService(const char * service, const char * proto)
{
  int n = MDNS.queryService(service, proto);
  if (n == 0) 
  {
    lastIP_index = -1;
  } 
  else 
  {
    for (int i = 0; i < n; i++) 
    {
      if ((MDNS.hostname(i).indexOf("niot") != -1) || (MDNS.hostname(i).indexOf("esp32") != -1) || (MDNS.hostname(i).indexOf("esp8266") != -1))
      {
        if (i > lastIP_index)
        {
          lastIP_index = i;
          anchor_IP = MDNS.IP(i).toString();
          return;
        }
        else lastIP_index = -1;
      }
    }
  }
  Serial.println();
}

void RemoteIO::switchState()
{
  switch (connection_state)
  {
    case INICIALIZATION:
      if ((WiFi.status() == WL_CONNECTED) && (Connected == true))
      {
        WebSerial.println("[INIC.] vai pro CONNECTED");
        Serial.println("[INIC.] vai pro CONNECTED");
        Serial.println("[INIC.] fechando access point");

        WiFi.mode(WIFI_STA); 

        next_state = CONNECTED;
      }
      else if (local_mode) 
      {
        WebSerial.println("[INIC.] vai pro DISCONNECTED");
        Serial.println("[INIC.] vai pro DISCONNECTED");
        next_state = DISCONNECTED;
      }
      else
      {
        next_state = INICIALIZATION;
      }
      break;
      
    case CONNECTED:
      if (WiFi.status() != WL_CONNECTED)
      {
        WebSerial.println("[CONNECTED] vai pro NO_WIFI");
        Serial.println("[CONNECTED] vai pro NO_WIFI");
        next_state = NO_WIFI;
      }
      else if (!Connected)
      {
        WebSerial.println("[CONNECTED] vai pro DISCONNECTED");
        Serial.println("[CONNECTED] vai pro DISCONNECTED");
        next_state = DISCONNECTED;
      }
      else 
      {
        next_state = CONNECTED;
      }
      break;
      
    case NO_WIFI:
      if (WiFi.status() == WL_CONNECTED)
      {
        WebSerial.println("[NO_WIFI] vai pro DISCONNECTED");
        Serial.println("[NO_WIFI] vai pro DISCONNECTED");
        start_debounce_time = 0;
        next_state = DISCONNECTED;
      }
      else 
      {
        next_state = NO_WIFI;
      }
      break;
      
    case DISCONNECTED:
      if (Connected)
      {
        reconnect_counter = 0;
        WebSerial.println("[DISCONNECTED] vai pro CONNECTED");
        Serial.println("[DISCONNECTED] vai pro CONNECTED");
        next_state = CONNECTED;
      }
      else if (WiFi.status() != WL_CONNECTED)
      {
        WebSerial.println("[DISCONNECTED] vai pro NO_WIFI");  
        Serial.println("[DISCONNECTED] vai pro NO_WIFI"); 
        next_state = NO_WIFI;
      }
      else 
      {
        next_state = DISCONNECTED;
      }
      break;
  }
  // updates current_state
  connection_state = next_state;
}

void RemoteIO::stateLogic()
{
  switch (connection_state)
  {
    case INICIALIZATION:
      
      socketIO.loop(); 
      if (Connected == false)
      {
        socketIOConnect();
      }
      break;
      
    case CONNECTED:
      
      socketIO.loop();
      if (setIO["disconnect"]["value"] == "1")
      {
        socketIO.sendEVENT("disconnect");
      }
      break;
      
    case NO_WIFI:

      if (millis() - start_reconnect_time >= 10000)
      {
        start_reconnect_time = millis();
        start_debounce_time = millis();
        nodeIotConnection(storedCallbackFunction); 
      }
      break;

    case DISCONNECTED:
      
      if (setIO["disconnect"]["value"].as<String>() == "0")
      {
        socketIO.loop();
        socketIOConnect();
      }
      
      // procura um âncora a cada 5 segundos
      if ((!anchored) && (millis() - start_browsing_time >= 5000))
      {
        browseService("http", "tcp");
        start_browsing_time = millis();

        if (anchor_IP.length() > 0)
        {
          StaticJsonDocument<250> doc;
          doc["status"] = "disconnected";
          doc["mac"] = WiFi.macAddress();
          send_to_anchor_buffer.clear();
          serializeJson(doc, send_to_anchor_buffer);
          doc.clear();

          espPOST(anchor_route, "", send_to_anchor_buffer);          
        }
      }
      
      if (millis() - start_reconnect_time >= 60000)
      {
        if (reconnect_counter >= 3) ESP.restart();
        else reconnect_counter++;
        start_reconnect_time = millis();
        start_debounce_time = millis();
        Serial.println("[stateLogic][DISCONNECTED] Trying connection...");
        nodeIotConnection(storedCallbackFunction); 
      }
      break;
  }
}

int RemoteIO::updateFirmwareOTA(String device_model, String firmware_version, String build_id)
{
  String base_url = "https://nodeiot-firmware.s3.us-east-1.amazonaws.com/bin/esp32";
  String ota_url = base_url + "/" + device_model + "/" + firmware_version + "/" + build_id;

  WebSerial.print("[updateFirmwareOTA] Tentando obter firmware na URL: ");
  WebSerial.println(ota_url);
  int ret = ota.DoOTAUpdate(ota_url.c_str(), ota.UPDATE_AND_BOOT);
  return ret;
}

void RemoteIO::rebootDevice()
{
  ESP.restart();
}

void RemoteIO::eraseDeviceSettings()
{
  deviceConfig->begin("deviceConfig", false);
  deviceConfig->clear();
  deviceConfig->end();
  WebSerial.printf("\nApagando configurações salvas na memória não volátil...\n");
  delay(1000);
  ESP.restart();
}

void RemoteIO::addWifiAccessPoint(String ssid, String password)
{
  JsonDocument array_document;
  JsonDocument accessPoint;
  
  deviceConfig->begin("deviceConfig", false); 
  String array_string = deviceConfig->getString("wifi_array_doc", "");
  deserializeJson(array_document, array_string);

  accessPoint["ssid"] = ssid;
  accessPoint["password"] = password;

  if (array_document.size() >= 5) array_document.remove(0);
  array_document.add(accessPoint);
  accessPoint.clear();

  array_string.clear();
  serializeJson(array_document, array_string);
  deviceConfig->putString("wifi_array_doc", array_string);
  array_document.clear();
  array_string.clear();

  deviceConfig->end();
}

void RemoteIO::infoUpdatedEventHandler(JsonDocument payload_doc)
{
  String function = payload_doc[1]["function"].as<String>();
  
  if (function == "restart") rebootDevice();
  else if (function == "reset") eraseDeviceSettings();
  else if (function == "otaUpdate") 
  {
    String device_model;
    String firmware_version = payload_doc[1]["data"]["firmware_version"].as<String>();
    String build_id = payload_doc[1]["data"]["build_id"].as<String>();

    if (payload_doc[1]["data"].containsKey("deviceModel"))
    {
      device_model = payload_doc[1]["data"]["deviceModel"].as<String>();
    }
    else 
    {
      device_model = _model;
    }

    updateFirmwareOTA(device_model, firmware_version, build_id);
  }
  else if (function == "addWifi")
  {
    String ssid = payload_doc[1]["data"]["ssid"].as<String>();
    String password = payload_doc[1]["data"]["password"].as<String>();

    addWifiAccessPoint(ssid, password);
  }
}

void RemoteIO::socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
    case sIOtype_DISCONNECT:
      Connected = false;
      break;
    case sIOtype_CONNECT:
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
      char *sptr = NULL;
      int id = strtol((char *)payload, &sptr, 10);

      //Serial.printf("[IOc] get event: %s id: %d\n", payload, id);
      WebSerial.printf("[socketIOEvent] get event: %s id: %d\n", payload, id);
      
      if (id)
      {
        payload = (uint8_t *)sptr;
      }

      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload, length);

      if (error) return;
      
      String eventName = doc[0];

      if (eventName == "infoUpdated")
      {
        infoUpdatedEventHandler(doc);
      }
      else
      {
        if (doc[1].containsKey("ipdest")) // modo âncora
        {
          StaticJsonDocument<250> doc2;
          doc2["ref"] = doc[1]["ref"];
          doc2["value"] = doc[1]["value"];
          
          anchored_IP = doc[1]["ipdest"].as<String>();
          serializeJson(doc2, send_to_anchored_buffer);
          
          doc2.clear();
          
          espPOST(anchored_route, "", send_to_anchored_buffer);
          send_to_anchored_buffer.clear();
        }
        else 
        {
          String ref = doc[1]["ref"];
          String value = doc[1]["value"];

          if (ref == "restart") rebootDevice();
          else if (ref == "reset") eraseDeviceSettings();
          else if (ref == "otaUpdate") ota.DoOTAUpdate(OTA_BASE_URL, ota.UPDATE_AND_BOOT);

          setIO[ref]["value"] = value;

          if (setIO[ref]["type"] == "OUTPUT")
          {
            updatePinOutput(ref);
          }
        }
        doc.clear();
        break;
      }
  }
}

void RemoteIO::tryWiFiConnection()
{
  JsonDocument wifi_doc;
  deviceConfig->begin("deviceConfig", false);
  String wifi_doc_string = deviceConfig->getString("wifi_array_doc", "");

  if (wifi_doc_string == "")
  {
    Serial.println("[tryWiFiConnection] No Wi-Fi network info available...");
    return;
  }

  deserializeJson(wifi_doc, wifi_doc_string);
  Connected = false;

  if (_deviceId != "" && _deviceId != "null")
  {
    String hostname = String("niot-") + String(_deviceId);
    hostname.toLowerCase();
    WiFi.setHostname(hostname.c_str());
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    if ((start_debounce_time != 0) && (millis() - start_debounce_time >= 2000) && (connection_state == NO_WIFI))
    {
      WiFi.disconnect();
      return;
    }

    long start_time = 0;
    int i = (wifi_doc.size() - 1);
    
    while (i >= 0)
    {
      if (start_time == 0)
      {
        _ssid = wifi_doc[i]["ssid"].as<String>();
        _password = wifi_doc[i]["password"].as<String>();
        Serial.printf("\n[tryWiFi] Trying connection on %s ...\n", _ssid);
        start_time = millis();
        WiFi.begin(_ssid, _password);
      } 
      else if (millis() - start_time >= 5000)
      {
        WiFi.disconnect();
        start_time = 0;
        i--;
        Serial.println("[tryWiFi] Connection failed...");
      }   

      if (WiFi.status() == WL_CONNECTED)
      {
        JsonDocument wifi_info_copy = wifi_doc[i];
        JsonDocument array_document;
        String array_string = deviceConfig->getString("wifi_array_doc", "");
        
        deserializeJson(array_document, array_string);

        wifi_doc.remove(i);
        wifi_doc.add(wifi_info_copy);
        serializeJson(wifi_doc, array_string);
        deviceConfig->putString("wifi_array_doc", array_string);

        wifi_info_copy.clear();
        Serial.printf("[tryWiFi] WiFi connected %s\n", WiFi.localIP().toString().c_str());
        
        return;
      }
    }
  }
  deviceConfig->end();
}

void RemoteIO::nodeIotConnection(void (*userCallbackFunction)(String ref, String value))
{
  if (connection_state == INICIALIZATION || connection_state == NO_WIFI) 
  {
    Serial.println("[nodeIotConnection] chamando tryWiFi");
    tryWiFiConnection();
  }

  if (WiFi.status() == WL_CONNECTED) 
  {
    WebSerial.printf("[nodeIotConnection] WiFi connected %s\n", WiFi.localIP().toString().c_str());
  }
  ArduinoOTA.begin();
  ota.SetCallback(callback);
  
  gmtOffset_sec = (-3) * 3600;
  daylightOffset_sec = 0;
  configTime(gmtOffset_sec, daylightOffset_sec, ntp_server1, ntp_server2);

  if ((connection_state == INICIALIZATION || connection_state == DISCONNECTED) && (_companyName != "")) 
  {
    Serial.println("[nodeIotConnection] chamando tryAuthenticate");
    while (state != "accepted")
    {
      if ((start_debounce_time != 0) && (millis() - start_debounce_time >= 2000))
      {
        return;
      }
      tryAuthenticate();
      
      if (local_mode) 
      {
        WebSerial.println("[nodeIotConnection] Local mode");
        break;
      }
    }

    if (!local_mode)
    {
      String appSocketPath = "/socket.io/?token=" + token + "&EIO=4";
      appLastDataUrl.replace(" ", "%20");

      fetchLatestData();

      socketIO.begin(_appHost, _appPort, appSocketPath); 
      socketIO.onEvent([this, userCallbackFunction](socketIOmessageType_t type, uint8_t* payload, size_t length)
      {
        this->socketIOEvent(type, payload, length);

        if ((userCallbackFunction != nullptr) && (type == sIOtype_EVENT)) 
        {
          char *sptr = NULL;
          int id = strtol((char *)payload, &sptr, 10);

          if (id)
          {
            payload = (uint8_t *)sptr;
          }

          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, payload, length);

          if (error) return;

          String ref = doc[1]["ref"];
          String value = doc[1]["value"];

          userCallbackFunction(ref, value);
        }
      });
    }
  }
}

void RemoteIO::socketIOConnect()
{
  uint64_t now = millis();
  if (Socketed == 0)
  {
    StaticJsonDocument<JSON_DOCUMENT_CAPACITY> doc;
    JsonArray array = doc.to<JsonArray>();
    array.add("connection");
    JsonObject query = array.createNestedObject();
    query["Query"]["token"] = token;

    String output;
    serializeJson(doc, output);
    Socketed = socketIO.sendEVENT(output);
    Socketed = 1;
  }
  if ((Socketed == 1) && (now - messageTimestamp > 2000) && (Connected == 0))
  {
    StaticJsonDocument<JSON_DOCUMENT_CAPACITY> doc;
    JsonArray array = doc.to<JsonArray>();
    messageTimestamp = now;
    array.add("joinRoom");
    String output;
    serializeJson(doc, output);
    Connected = socketIO.sendEVENT(output);
    if (Connected) WebSerial.println("[socketIOConnect] Connected");
    //else Serial.println("[socketIOConnect] Failed connecting");
  }
}

void IRAM_ATTR RemoteIO::interruptCallback(void* arg)
{
  interrupt_data* obj = (interrupt_data*)arg;
  
  if (post_data_queue.size() <= 10)
  {
    unsigned long reading_timestamp = millis();
    int value = digitalRead(obj->remoteio_pointer->setIO[obj->ref_arg]["pin"].as<int>());

    DynamicJsonDocument doc(75);
    doc["ref"] = obj->ref_arg;
    doc["value"] = value;
    doc["timestamp"] = reading_timestamp;

    obj->remoteio_pointer->setIO[obj->ref_arg]["value"] = value;
    obj->remoteio_pointer->setIO[obj->ref_arg]["timestamp"] = reading_timestamp;

    post_data_queue.add(doc);
  }
}

void RemoteIO::timerEventCallback(void *arg)
{
  event_data* obj = (event_data*)arg;
  JsonDocument& actions = *(obj->actions_arg); //desrefenciar, lembrar disso

  for (size_t i = 0; i < actions.size(); i++)
  {
    String ref = actions[i]["ref"].as<String>();
    String refType = obj->remoteio_pointer->setIO[ref]["type"].as<String>();

    obj->remoteio_pointer->setIO[ref]["value"] = actions[i]["value"];
    if(refType == "OUTPUT") obj->remoteio_pointer->updatePinOutput(ref);

    JsonDocument doc;
    doc["ref"] = ref;
    doc["value"] = actions[i]["value"];
    doc["timestamp"] = time(nullptr); //unix time seconds, gmt-3
    
    post_data_queue.add(doc);
    timer_expired = true; // para sinalizar ao loop principal
  }
}

void RemoteIO::updateEventArray()
{
  if ((timer_expired) && (event_array.size() > 0))
  {
    for (size_t i = 0; i < event_array.size(); i++)
    {
      if (event_array[i]["active"].as<bool>())
      {
        if (event_array[i]["repeat"].as<int>() > 0)
        {
          JsonDocument obj = event_array[0];
          String targetTimestamp = obj["targetTimestamp"].as<String>();
          obj["targetTimestamp"] = ((strtol(targetTimestamp.c_str(), NULL, 10)) + ((obj["delay"].as<int>())/1000));
          obj["active"] = false;
          event_array.add(obj);
          obj.clear();
        }

        event_array.remove(i);
        timer_expired = false;
        setTimer();
        return;
      }
    }
  }
}

void RemoteIO::setTimer()
{
  if (getLocalTime(&timeinfo) && event_array.size() > 0)
  {
    int next_event_position = 0;
    String next_event_timestamp_string = event_array[0]["targetTimestamp"].as<String>();
    long next_event_timestamp = strtol(next_event_timestamp_string.c_str(), NULL, 10);

    for (size_t i = 1; i < event_array.size(); i++)
    {
      String temp_event_timestamp_string = event_array[i]["targetTimestamp"].as<String>();
      long temp_event_timestamp = strtol(next_event_timestamp_string.c_str(), NULL, 10);

      if (temp_event_timestamp < next_event_timestamp) 
      {
        next_event_position = i;
        next_event_timestamp_string = temp_event_timestamp_string;
        next_event_timestamp = temp_event_timestamp;
      }
    }

    if (!event_array[next_event_position]["active"].as<bool>())
    {
      time_t now = time(nullptr);
      String unix_time_s_string = event_array[next_event_position]["targetTimestamp"].as<String>();

      long unix_time_s = strtol(unix_time_s_string.c_str(), NULL, 10);
      int delaySeconds = unix_time_s - now;

      WebSerial.printf("\n[setTimer] Event will trigger in %d\n", delaySeconds);
      Serial.printf("\n[setTimer] Event will trigger in %d\n", delaySeconds);

      event_data* arg = new event_data();
      arg->remoteio_pointer = this;

      JsonDocument* actions_pointer = new JsonDocument();

      for (size_t i = 0; i < event_array[next_event_position]["actions"].size(); i++)
      {
        actions_pointer->add(event_array[next_event_position]["actions"][i]);
      }

      arg->actions_arg = actions_pointer;

      timer_args.callback = &timerEventCallback;
      timer_args.arg = (void*) arg;
      timer_args.name = "timerEvent";

      esp_timer_create(&timer_args, &timer);
      esp_timer_start_once(timer, delaySeconds * 1000000);
      event_array[next_event_position]["active"] = true;
    }
  }
  else if (event_array.size() > 0)
  {
    WebSerial.println("[setTimer] Got events, but couldn't sync internal clock.");
    Serial.println("[setTimer] Got events, but couldn't sync internal clock.");
  }
  else 
  {
    WebSerial.println("[setTimer] No events found.");
    Serial.println("[setTimer] No events found.");
  }
}

void RemoteIO::setIOsAndEvents(JsonDocument document)
{
  if (document.containsKey("token")) 
  {
    token = document["token"].as<String>();
  }
  if (document.containsKey("serverAddr"))
  {
    extractIPAddress(document["serverAddr"].as<String>());
  }

  for (size_t i = 0; i < document["gpio"].size(); i++)
  {
    String ref = document["gpio"][i]["ref"];
    int pin = document["gpio"][i]["pin"].as<int>();
    String type = document["gpio"][i]["type"];
    String mode = document["gpio"][i]["mode"]; // modo de operação. Ex. p/ INPUTs: interrupção, cíclica, em horário definido...

    if (type == "INPUT" || type == "INPUT_ANALOG")
    {
      setIO[ref]["pin"] = pin;
      setIO[ref]["type"] = type;
      setIO[ref]["mode"] = mode;
      pinMode(pin, INPUT);

      if (mode == "interrupt")
      {
        interrupt_data* arg = new interrupt_data();
        arg->remoteio_pointer = this;
        arg->ref_arg = document["gpio"][i]["ref"].as<String>();
        attachInterruptArg(digitalPinToInterrupt(pin), interruptCallback, (void*)arg, CHANGE);
      }
    }
    else if (type == "INPUT_PULLUP")
    {
      setIO[ref]["pin"] = pin;
      setIO[ref]["type"] = type;
      setIO[ref]["mode"] = mode;
      pinMode(pin, INPUT_PULLUP);

      if (mode == "interrupt")
      {
        interrupt_data* arg = new interrupt_data();
        arg->remoteio_pointer = this;
        arg->ref_arg = document["gpio"][i]["ref"].as<String>();
        attachInterruptArg(digitalPinToInterrupt(pin), interruptCallback, (void*)arg, CHANGE);
      }
    }
    else if (type == "INPUT_PULLDOWN")
    {
      setIO[ref]["pin"] = pin;
      setIO[ref]["type"] = type;
      setIO[ref]["mode"] = mode;
      pinMode(pin, INPUT_PULLDOWN);

      if (mode == "interrupt")
      {
        interrupt_data* arg = new interrupt_data();
        arg->remoteio_pointer = this;
        arg->ref_arg = document["gpio"][i]["ref"].as<String>();
        attachInterruptArg(digitalPinToInterrupt(pin), interruptCallback, (void*)arg, CHANGE);
      }
    }
    else if (type == "OUTPUT")
    {
      setIO[ref]["pin"] = pin;
      setIO[ref]["type"] = type;
      pinMode(pin, OUTPUT);
    } 
    else 
    {
      setIO[ref]["pin"] = pin;
      setIO[ref]["type"] = "N/L";
    }
  }

  for (size_t i = 0; i < document["events"].size(); i++)
  {
    document["events"][i]["active"] = false; 
    event_array.add(document["events"][i]);
  }
  
  setTimer();
}

int RemoteIO::tryAuthenticate()
{
  WiFiClientSecure client;
  HTTPClient https;

  client.setInsecure();
  
  StaticJsonDocument<JSON_DOCUMENT_CAPACITY> document;
  String request;
  
  deviceConfig->begin("deviceConfig", false);
  
  document["companyName"] = _companyName;
  if (_deviceId != "" && _deviceId != "null") document["deviceId"] = _deviceId;
  document["mac"] = WiFi.macAddress();
  document["ipAddress"] = WiFi.localIP().toString();
  document["version"] = VERSION;
  document["deviceModel"] = deviceConfig->getString("model", "ESP_32");
  
  serializeJson(document, request);
  Serial.println(request);

  https.begin(client, appVerifyUrl);
  https.addHeader("Content-Type", "application/json");

  int statusCode = https.POST(request);
  
  String response = https.getString(); 
  document.clear();
  deserializeJson(document, response);
  Serial.println(response);

  if (statusCode == HTTP_CODE_OK)
  {
    Serial.println("[tryAuth] HTTP_CODE 200");
    WebSerial.println("[tryAuth] HTTP_CODE 200");
    state = document["state"].as<String>();
    local_mode = false;
    
    if (state != "accepted") 
    {
      document.clear();
      deviceConfig->end();
      https.end();
      return 0; 
    }
    
    String deviceSettings;
    serializeJson(document, deviceSettings);
    deviceConfig->putString("deviceSettings", deviceSettings);
    deviceConfig->putString("deviceId", document["deviceId"].as<String>());
    _deviceId = document["deviceId"].as<String>();

    String LOCAL_DOMAIN = String("niot-") + document["deviceId"].as<String>();
    LOCAL_DOMAIN.toLowerCase();

    if (!MDNS.begin(LOCAL_DOMAIN)) 
    {
      Serial.println("[begin] An error occurred while configuring mDNS");
    }
    
    document.clear();
    deserializeJson(document, deviceSettings);
    
    setIOsAndEvents(document);
  }
  else
  {
    Serial.printf("[tryAuth] HTTP_CODE %d\n", statusCode);
    WebSerial.printf("[tryAuth] HTTP_CODE %d\n", statusCode);
    WebSerial.print("[tryAuth] Msg: ");
    WebSerial.println(response);

    document.clear();
    String deviceSettings = deviceConfig->getString("deviceSettings", "");

    if (deviceSettings != "")
    {
      deserializeJson(document, deviceSettings);
      setIOsAndEvents(document);
      local_mode = true;
    }
    else
    {
      File file = SPIFFS.open("/model.json", "r");
  
      if (!file)
      {
        WebSerial.println("[tryAuth] Failed to open file for reading");
        Serial.println("[tryAuth] Failed to open file for reading");
      }

      deserializeJson(document, file);
      file.close();

      if (document.containsKey("gpio"))
      {
        WebSerial.println("[tryAuth] Using model's default gpio json document..."); 
        Serial.println("[tryAuth] Using model's default gpio json document...");
        setIOsAndEvents(document);
        local_mode = true;
      }
    }
  }
  document.clear();
  deviceConfig->end();
  https.end();
  return statusCode;
}

void RemoteIO::fetchLatestData()
{ 
  WiFiClientSecure client;
  HTTPClient https;
  JsonDocument document;

  client.setInsecure();
  appLastDataUrl = appBaseUrl + "/devices/getdata/" + _companyName + "/" + _deviceId;

  https.begin(client, appLastDataUrl);
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + token);

  int statusCode = https.GET();
  deserializeJson(document, https.getStream());

  if (statusCode == HTTP_CODE_OK)
  {
    WebSerial.println("[fetchLatest] HTTP_CODE 200"); 

    for (size_t i = 0; i < document.size(); i++)
    {
      String auxRef = document[i]["ref"].as<String>();
      String auxValue = document[i]["data"]["value"].as<String>();

      if (auxValue == "null")
      {
        auxValue = "0";
      }
      
      setIO[auxRef]["value"] = auxValue;
      
      if (setIO[auxRef]["type"] == "OUTPUT")
      {
        updatePinOutput(auxRef);
      }
    }
  }
  else
  {
    WebSerial.print("[fetchLatest] Failed. HTTP_CODE: "); 
    WebSerial.println(statusCode);
    WebSerial.print("[fetchLatest] URL: ");
    WebSerial.println(appLastDataUrl);
  }

  document.clear();
  https.end();
}

void RemoteIO::extractIPAddress(String url)
{
  String new_url;
  
  if (appBaseUrl == "https://api-dev.orlaguaiba.com.br/api") 
  {
    new_url = "https://54.88.219.77:5000"; 
  }
  else 
  {
    new_url = url;
  }

  int startIndex = new_url.indexOf("//") + 2; // Encontra o início do endereço IP
  int endIndex = new_url.indexOf(":", startIndex); // Encontra o fim do endereço IP

  _appHost = new_url.substring(startIndex, endIndex); // Extrai o endereço IP
}

void RemoteIO::localHttpUpdateMsg (String ref, String value)
{
  StaticJsonDocument<250> doc;
  send_to_anchor_buffer.clear();
  setIO[ref]["value"] = value;
  doc["deviceId"] = _deviceId;
  doc["ref"] = ref;
  doc["value"] = value;
  serializeJson(doc, send_to_anchor_buffer);
  doc.clear();
  espPOST(anchor_route, "", send_to_anchor_buffer);
}

void RemoteIO::updatePinOutput(String ref)
{
  int PinRef = setIO[ref]["pin"].as<int>();
  int ValueRef = setIO[ref]["value"].as<int>();
  
  digitalWrite(PinRef, ValueRef);
}

void RemoteIO::updatePinInput(String ref)
{
  int pinRef = setIO[ref]["pin"].as<int>();
  String typeRef = setIO[ref]["type"].as<String>();
  int delayTime = setIO[ref]["delay"].as<int>() * 1000; // variável de configuração sincronizada com a plataforma
  int timestamp = setIO[ref]["timestamp"].as<int>();  // variável de configuração local, dessincronizada

  // garantir pelo menos 5 seg de delay
  if (delayTime < 5000) 
  {
    delayTime = 5000; // ms
    setIO[ref]["delay"] = 5; // s
  }

  if (millis() - timestamp >= delayTime)
  {
    setIO[ref]["timestamp"] = millis();
    
    // executa ação de leitura conforme tipo de variável ou processo utilizado
    if (typeRef == "INPUT" || typeRef == "INPUT_PULLDOWN" || typeRef == "INPUT_PULLUP")
    {
      int valueRef = digitalRead(pinRef);
      if (connection_state == CONNECTED) espPOST(ref, String(valueRef));
      else if (anchored) localHttpUpdateMsg(ref, String(valueRef));
    }
    else if (typeRef == "INPUT_ANALOG")
    {
      float valueRef = analogRead(pinRef);
      if (connection_state == CONNECTED) espPOST(ref, String(valueRef));
      else if (anchored) localHttpUpdateMsg(ref, String(valueRef));
    }
  }
}

void RemoteIO::notFound(AsyncWebServerRequest *request)
{
  request->send(404, "application/json", "{\"message\":\"Not found\"}");
}

int RemoteIO::espPOST(JsonDocument arrayDoc)
{
  JsonDocument doc;
  String value;
  doc["deviceId"] = _deviceId;
  doc["dataArray"] = arrayDoc;
  serializeJson(doc, value);
  return espPOST(appPostMultiData, "", value);
}

int RemoteIO::espPOST(String variable, String value)
{
    return espPOST(appPostData, variable, value);
}

int RemoteIO::espPOST(String Router, String variable, String value)
{
  if ((WiFi.status() == WL_CONNECTED))
  {
    String route;

    if (Router == anchor_route) route = "http://" + anchor_IP + "/post-message";
    else if (Router == anchored_route) route = "http://" + anchored_IP + "/post-message";
    else route = Router;

    WiFiClientSecure client;
    HTTPClient https;
    StaticJsonDocument<1024> document;
    String request;

    client.setInsecure();

    if (Router == appPostData)
    {
      document["deviceId"] = _deviceId;
      document["ref"] = variable;
      document["value"] = value;
      document["timestamp"] = time(nullptr);
      setIO[variable]["value"] = value;
      serializeJson(document, request);
    }
    else if (Router == appPostMultiData)
    {
      route = appPostMultiData;
      request = value;
    }
    else request = value; 
    
    https.begin(client, route); // HTTP
    https.addHeader("Content-Type", "application/json");
    https.addHeader("authorization", "Bearer " + token);

    WebSerial.print("[espPOST] Request: ");
    WebSerial.println(request);
    
    int httpCode = https.POST(request);

    String response = https.getString(); 
    document.clear();
    deserializeJson(document, response);

    if (httpCode == HTTP_CODE_OK)
    {
      if (Router == appSideDoor && document.containsKey("data"))
      {
        if (document["data"]["actived"].as<bool>() == true)
        {
          anchored_IP = document["data"]["ipdest"].as<String>();
          anchoring = true; 
        } 
      }
      else if (Router == anchor_route && document["msg"] == "ok")
      {
        anchored = true;
      }
    }
    else if (httpCode != HTTP_CODE_OK) 
    {
      if (Router == anchor_route)
      {
        anchored = false;
      }
    }
    document.clear();
    https.end();
    return httpCode;
  }
  return 0;
}