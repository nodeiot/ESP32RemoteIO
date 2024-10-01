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

  state = "";
  token = "";
    
  configurations = configurationDocument.to<JsonArray>();
  setIO = configurations.createNestedObject();

  event_array = event_doc.to<JsonArray>();

  Connected = false;
  Socketed = 0;
  messageTimestamp = 0;

  connection_state = INICIALIZATION;
  next_state = INICIALIZATION;

  start_debounce_time = 0;
  start_browsing_time = 0;
  start_reconnect_time = 0;
  start_config_time = 0;
  start_reset_time = 0;

  lastIP_index = -1;
  anchored = false;
  anchoring = false;
  reconnect_counter = 0;
}

void RemoteIO::begin()
{
  Serial.begin(115200);
  deviceConfig->begin("deviceConfig", false);

  if (!SPIFFS.begin(true))
  {
    //Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  String NVS_SSID = deviceConfig->getString("ssid", "");
  String NVS_PASSWORD = deviceConfig->getString("password", "");
  String NVS_COMPANYNAME = deviceConfig->getString("companyName", "");
  String NVS_DEVICEID = deviceConfig->getString("deviceId", "");
  String NVS_MODEL = deviceConfig->getString("model", "");
  String NVS_IOSETTINGS = deviceConfig->getString("ioSettings", "");

  server->on("/monitor-data", HTTP_GET, [this](AsyncWebServerRequest *request) {
      
      StaticJsonDocument<1024> monitor_doc;
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

  if (NVS_SSID.length() == 0 || NVS_PASSWORD.length() == 0 || NVS_COMPANYNAME.length() == 0 || NVS_DEVICEID.length() == 0)
  {
    startAccessPoint();
  }
  else
  {
    deviceConfig->end();

    _ssid = NVS_SSID;
    _password = NVS_PASSWORD;
    _companyName = NVS_COMPANYNAME;
    _deviceId = NVS_DEVICEID;
    _model = NVS_MODEL;

    appBaseUrl = "https://api-dev.orlaguaiba.com.br/api"; //"https://api.nodeiot.app.br/api"; // teste no dev
    appVerifyUrl = appBaseUrl + "/devices/verify";
    appPostData = appBaseUrl + "/broker/data/";
    appSideDoor = appBaseUrl + "/devices/devicedisconnected";
    appPostDataFromAnchored = appBaseUrl + "/broker/ahamdata";
    appLastDataUrl = appBaseUrl + "/devices/getdata/" + _companyName + "/" + _deviceId;

    timer_expired = false;
    nodeIotConnection();

    String LOCAL_DOMAIN = String("niot-") + String(_deviceId);
    LOCAL_DOMAIN.toLowerCase();

    if (!MDNS.begin(LOCAL_DOMAIN)) 
    {
      Serial.println("Erro ao configurar o mDNS");
    }

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
      
      //Serial.print("[AsyncCallback]: ");
      //Serial.println(data.as<String>());

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
      else if (data.containsKey("ref") && !data.containsKey("deviceId") && connection_state == DISCONNECTED)
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
    server->addHandler(handler);
  }

  MDNS.addService("http", "tcp", 80);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  server->onNotFound(std::bind(&RemoteIO::notFound, this, std::placeholders::_1));

  ArduinoOTA.begin();
  server->begin(); 
  ota.SetCallback(callback);
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

void RemoteIO::startAccessPoint()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);

  File file = SPIFFS.open("/model.json", "r");
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    ESP.restart();
  }

  StaticJsonDocument<100> modelDoc;
  deserializeJson(modelDoc, file);
  file.close();
  deviceConfig->putString("model", modelDoc["model"].as<String>());

  IPAddress apIP(192, 168, 4, 1);

  bool result = WiFi.softAP("RemoteIO");
  if (!result) 
  {
    //Serial.println("Erro ao configurar o ponto de acesso");
    ESP.restart();
  }
  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  //Serial.println("Ponto de acesso iniciado");

  IPAddress IP = WiFi.softAPIP();
  //Serial.print("IP do ponto de acesso: ");
  Serial.println(IP);

  String LOCAL_DOMAIN = String("remoteiodevice");

  if (!MDNS.begin(LOCAL_DOMAIN)) 
  {
    Serial.println("Erro ao configurar o mDNS");
  }

  start_config_time = millis();
  
  server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", page_setup);
  });
  
  server->on("/get", HTTP_GET, [this](AsyncWebServerRequest *request) {

    if (request->hasParam("ssid") && request->hasParam("password") && request->hasParam("companyName") && request->hasParam("deviceId")) 
    {
      String arg_ssid = request->getParam("ssid")->value();
      String arg_password = request->getParam("password")->value();
      String arg_companyName = request->getParam("companyName")->value();
      String arg_deviceId = request->getParam("deviceId")->value();

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
        request->send_P(200, "text/html", page_setup_fail);
      }
      else if (status == WL_CONNECTED)
      {
        deviceConfig->putString("ssid", arg_ssid);
        deviceConfig->putString("password", arg_password);
        deviceConfig->putString("companyName", arg_companyName);
        deviceConfig->putString("deviceId", arg_deviceId);
        deviceConfig->end();

        request->send_P(200, "text/html", page_setup_success);
        delay(1000);
        ESP.restart();
      }   
    } 
    else 
    {
      request->send(400, "text/plain", "Parâmetros ausentes");
    }
  });
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
  switchState();
  stateLogic();
  checkResetting(5000); // millisegundos
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
        Serial.println("[INICIALIZATION] vai pro CONNECTED");
        next_state = CONNECTED;
      }
      else
      {
        next_state = INICIALIZATION;
      }
      break;
      
    case CONNECTED:
      if (WiFi.status() != WL_CONNECTED)
      {
        //Serial.println("[CONNECTED] vai pro NO_WIFI");
        next_state = NO_WIFI;
      }
      else if (!Connected)
      {
        //Serial.println("[CONNECTED] vai pro DISCONNECTED");
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
        //Serial.println("[NO_WIFI] vai pro DISCONNECTED");
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
        //Serial.println("[DISCONNECTED] vai pro CONNECTED");
        next_state = CONNECTED;
      }
      else if (WiFi.status() != WL_CONNECTED)
      {
        //Serial.println("[DISCONNECTED] vai pro NO_WIFI");  
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
        nodeIotConnection(); 
      }
      break;

    case DISCONNECTED:
      
      if (setIO["disconnect"]["value"] == "0")
      {
        socketIO.loop();
        socketIOConnect();
      }
      
      // procura um âncora a cada 5 segundos
      if ((!anchored) && (millis() - start_browsing_time >= 5000))
      {
        browseService("http", "tcp");
        start_browsing_time = millis();

        // se encontrou possível âncora, tenta comunicação
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
        nodeIotConnection(); 
      }
      break;
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
      
      if (id)
      {
        payload = (uint8_t *)sptr;
      }

      StaticJsonDocument<1024> doc;
      StaticJsonDocument<250> doc2;
      DeserializationError error = deserializeJson(doc, payload, length);

      if (error)
      {
        //Serial.print(F("[IOc]: deserializeJson() failed: "));
        //Serial.println(error.c_str());
        return;
      }

      String eventName = doc[0];

      if (doc[1].containsKey("ipdest")) // modo âncora
      {
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

        if (ref == "restart") ESP.restart();
        else if (ref == "reset")
        {
          deviceConfig->begin("deviceConfig", false);
          deviceConfig->clear();
          deviceConfig->end();
          delay(1000);
          ESP.restart();
        }
        else if (ref == "otaUpdate")
        {
          int ret = ota.CheckForOTAUpdate(OTA_JSON_URL, "0.0.0");
        }

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

void RemoteIO::nodeIotConnection()
{
  String hostname = String("niot-") + String(_deviceId);
  hostname.toLowerCase();
  Connected = false;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);
  WiFi.waitForConnectResult();
  WiFi.setHostname(hostname.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    if ((start_debounce_time != 0) && (millis() - start_debounce_time >= 2000))
    {
      WiFi.disconnect();
      return;
    }
    delay(500);
  }
  
  Serial.printf("[nodeIotConnection] WiFi Connected %s\n", WiFi.localIP().toString().c_str());

  appVerifyUrl.replace(" ", "%20");
  appLastDataUrl.replace(" ", "%20");
  
  gmtOffset_sec = (-3) * 3600;
  daylightOffset_sec = 0;
  configTime(gmtOffset_sec, daylightOffset_sec, ntp_server1, ntp_server2);

  while (state != "accepted")
  {
    if ((start_debounce_time != 0) && (millis() - start_debounce_time >= 2000))
    {
      return;
    }
    tryAuthenticate();
  }

  String appSocketPath = "/socket.io/?token=" + token + "&EIO=4";

  fetchLatestData();

  socketIO.begin(_appHost, _appPort, appSocketPath); 
  socketIO.onEvent(std::bind(&RemoteIO::socketIOEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
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
    //if (Connected) //Serial.println("[socketIOConnect] Connected");
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

void RemoteIO::inputTimerCallback(void* arg)
{
  interrupt_data* obj = (interrupt_data*)arg;
  String ref = obj->ref_arg;
  String refType = obj->remoteio_pointer->setIO[ref]["type"].as<String>();
  int refPin = obj->remoteio_pointer->setIO[ref]["pin"].as<int>();

  JsonDocument doc;
  doc["ref"] = ref;

  if (refType == "INPUT" || refType == "INPUT_PULLUP" || refType == "INPUT_PULLDOWN")
  {
    doc["value"] = String(digitalRead(refPin));
  }
  else if (refType == "INPUT_ANALOG")
  {
    doc["value"] == String(analogRead(refPin));
  }
  
  doc["timestamp"] = String(millis());
  post_data_queue.add(doc);
  timer_expired = true; // para sinalizar ao loop principal
}

void RemoteIO::outputTimerCallback(void *arg)
{
  interrupt_data* obj = (interrupt_data*)arg;
  String ref = obj->ref_arg;
  String newValue;
  int current_value = obj->remoteio_pointer->setIO[ref]["value"].as<int>();

  if (current_value == 1) newValue = "0";
  else if (current_value == 0) newValue = "1";
  
  obj->remoteio_pointer->setIO[ref]["value"] = newValue;
  obj->remoteio_pointer->updatePinOutput(ref);

  JsonDocument doc;
  doc["ref"] = ref;
  doc["value"] = newValue;
  doc["timestamp"] = String(millis());

  post_data_queue.add(doc);
  timer_expired = true; // para sinalizar ao loop principal
}

void RemoteIO::updateEventArray()
{
  if ((timer_expired) && (event_array.size() > 0) && (event_array[0]["active"].as<bool>()))
  {
    event_array.remove(0);
    timer_expired = false;

    // set do timer para um novo evento
    setTimer();

    // obtém novos eventos da plataforma
    // getEvents();
  }
}

void RemoteIO::getEvents()
{
  // http get numa rota para pegar um array de eventos
}

void RemoteIO::setTimer()
{
  int currentHour, currentMinute, currentSecond;

  if (getLocalTime(&timeinfo) && event_array.size() > 0)
  {
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSecond = timeinfo.tm_sec;
    
    if (!event_array[0]["state"].as<bool>()) 
    {
      int delaySeconds = (event_array[0]["targetHour"].as<int>() * 3600 + event_array[0]["targetMinute"].as<int>() * 60 + event_array[0]["targetSecond"].as<int>()) - 
                          (currentHour * 3600 + currentMinute * 60 + currentSecond);
      
      // Adjust if target time is the next day
      if (delaySeconds < 0) delaySeconds += 86400;

      String ref = event_array[0]["ref"].as<String>();
      String refType = setIO[ref]["type"].as<String>();

      Serial.print(ref);
      Serial.printf(" event will trigger in %d seconds\n", delaySeconds);
      
      interrupt_data* arg = new interrupt_data();
      arg->remoteio_pointer = this;
      arg->ref_arg = ref;

      if (refType == "OUTPUT")
      {
        timer_args.callback = &outputTimerCallback;
        timer_args.arg = (void*) arg;
        timer_args.name = "outputTimer";

        esp_timer_create(&timer_args, &timer);
        esp_timer_start_once(timer, delaySeconds * 1000000);
        event_array[0]["active"] = true;
      }
      else if (refType == "INPUT" || refType == "INPUT_PULLDOWN" || refType == "INPUT_PULLUP" || refType == "INPUT_ANALOG")
      {
        timer_args.callback = &inputTimerCallback;
        timer_args.arg = (void*) arg;
        timer_args.name = "inputTimer";

        esp_timer_create(&timer_args, &timer);
        esp_timer_start_once(timer, delaySeconds * 1000000);
        event_array[0]["active"] = true;
      }
    }
  }
  else 
  {
    //Serial.println("failed getLocalTime");
  }
}

void RemoteIO::tryAuthenticate()
{
  WiFiClientSecure client;
  HTTPClient https;

  client.setInsecure();
  
  StaticJsonDocument<JSON_DOCUMENT_CAPACITY> document;
  String request;

  deviceConfig->begin("deviceConfig", false);
  String storedTimestamp = deviceConfig->getString("Timestamp", "");
  String model = deviceConfig->getString("model", "");

  document["companyName"] = _companyName;
  document["deviceId"] = _deviceId;
  document["mac"] = WiFi.macAddress();
  document["ipAddress"] = WiFi.localIP().toString();
  document["settingsTimestamp"] = storedTimestamp;
  
  if (model == "") document["model"] = "ESP_32";
  else document["model"] = model;
  
  serializeJson(document, request);

  https.begin(client, appVerifyUrl);
  https.addHeader("Content-Type", "application/json");

  int statusCode = https.POST(request);
  
  String response = https.getString(); 
  document.clear();
  deserializeJson(document, response);
  Serial.println(response);

  if (statusCode == HTTP_CODE_OK)
  {
    state = document["state"].as<String>();

    if (state != "accepted") 
    {
      document.clear();
      deviceConfig->end();
      https.end();
      return;
    }
    else if (document["settingsTimestamp"].as<String>() != storedTimestamp)
    {
      //Serial.println("[tryAuthenticate] timestamps diferentes");
      String ioSettings;
      serializeJson(document, ioSettings);
      deviceConfig->putString("ioSettings", ioSettings);
      document.clear();
      deserializeJson(document, ioSettings);
    }

    deviceConfig->end();
    token = document["token"].as<String>();
    extractIPAddress(document["serverAddr"].as<String>());

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

    //
    //getEvents();
    setTimer();
    // 
  }
  document.clear();
  https.end();
}

void RemoteIO::fetchLatestData()
{ 
  WiFiClient client;
  HTTPClient http;

  http.begin(client, appLastDataUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + token);

  int statusCode = http.GET();

  if (statusCode == HTTP_CODE_OK)
  {
    StaticJsonDocument<JSON_DOCUMENT_CAPACITY> document;
    deserializeJson(document, http.getStream());

    for (size_t i = 0; i < document.size(); i++)
    {
      String auxRef = document[i]["ref"];
      String auxValue = document[i]["data"]["value"];

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
    document.clear();
  }

  http.end();
}

void RemoteIO::extractIPAddress(String url)
{
  String new_url;
  
  if (appBaseUrl == "https://api-dev.orlaguaiba.com.br/api") 
  {
    new_url = "https://54.88.219.77:5000"; // url atual do back (dev)
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
  return espPOST("appPostDataArray", "", value);
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
      JsonDocument doc; 
      doc["ref"] = variable;
      doc["value"] = value;
      doc["timestamp"] = String(millis());
      document["dataArray"].add(doc);
      setIO[variable]["value"] = value;
      serializeJson(document, request);
    }
    else if (Router == "appPostDataArray")
    {
      route = appPostData;
      request = value;
    }
    else request = value; 
    
    https.begin(client, route); // HTTP
    https.addHeader("Content-Type", "application/json");
    https.addHeader("authorization", "Bearer " + token);

    Serial.print("[espPOST] Request: ");
    Serial.println(request);
    
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