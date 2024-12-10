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

RemoteIO::RemoteIO()
{
  _appPort = 5000;

  server = new AsyncWebServer(80);
  deviceConfig = new Preferences();

  appBaseUrl = "https://api.nodeiot.app.br/api";
  appVerifyUrl = appBaseUrl + "/devices/verify";
  appPostData = appBaseUrl + "/broker/data/";
  
  state = "";
  token = "";
    
  configurations = configurationDocument.to<JsonArray>();
  setIO = configurations.createNestedObject();

  Connected = false;
  Socketed = 0;
  messageTimestamp = 0;

  connection_state = INICIALIZATION;
  next_state = INICIALIZATION;

  start_debounce_time = 0;
  start_reconnect_time = 0;

  reconnect_counter = 0;
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

  _companyName = deviceConfig->getString("companyName", "");
  _deviceId = deviceConfig->getString("deviceId", "");
  _ssid = deviceConfig->getString("ssid", "");
  _password = deviceConfig->getString("password", "");

  getPCBModel();
  deviceConfig->end();

  startAccessPoint();
  openLocalServer();

  if ((_ssid != "") && (_ssid != "null") && (_password != "") && (_password != "null"))
  {
    nodeIotConnection(userCallbackFunction);
  }
}

void callback(int offset, int totallength)
{
  Serial.printf("Updating %d of %d (%02d%%)...\n", offset, totallength, 100 * offset / totallength);
}

void waiting_function(unsigned long waiting_time_ms)
{
  unsigned long start_time = millis();

  while (millis() - start_time >= waiting_time_ms)
  {
    // do nothing, just wait
  }
}

void RemoteIO::getPCBModel()
{
  File file = SPIFFS.open("/model.json", "r");
  
  if (!file)
  {
    Serial.println("[getPCBModel] Failed to open file for reading");
  }

  JsonDocument document;
  deserializeJson(document, file);
  file.close();
  
  if ((document["model"].as<String>() == "") || (document.isNull()))
  {
    deviceConfig->putString("model", "ESP_32");
    _model = "ESP_32";
  }
  else 
  {
    deviceConfig->putString("model", document["model"].as<String>());
    _model = document["model"].as<String>();
  }
  Serial.printf("[getPCBModel] model: %s\n", _model);
}

void RemoteIO::startAccessPoint()
{
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

void RemoteIO::openLocalServer()
{
  Serial.println("[openLocalServer] Opening local http endpoints");

  server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", page_setup);
  });

  AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/device-config", [this](AsyncWebServerRequest *request, JsonVariant &json) {
    
    JsonDocument doc;

    if (json.is<JsonArray>()) doc = json.as<JsonArray>();

    else if (json.is<JsonObject>()) doc = json.as<JsonObject>();
    
    Serial.print("[/device-config] doc: ");
    serializeJson(doc, Serial);
    Serial.println("");

    if (doc.isNull())
    {
      request->send(400, "text/html", "<script>alert('Parâmetros ausentes!');</script>");
      waiting_function(2500);
      return;
    }

    String arg_ssid = doc["ssid"].as<String>();
    String arg_password = doc["password"].as<String>();
    String arg_companyName = doc["companyName"].as<String>();
    String arg_deviceId = doc["deviceId"].as<String>();

    if (connection_state == INICIALIZATION)
    {
      Serial.print("[device-config] Trying connection on ");
      Serial.println(arg_ssid);

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
        delay(500); 
        esp_task_wdt_reset(); 
      }

      if (status != WL_CONNECTED)
      {
        request->send(400, "text/html", "<script>alert('Falha na conexão com a rede informada. Verifique a informação inserida e tente novamente.');</script>");
        waiting_function(2500);
      }
      else if (status == WL_CONNECTED)
      {
        deviceConfig->begin("deviceConfig", false);

        deviceConfig->putString("deviceId", arg_deviceId);
        deviceConfig->putString("companyName", arg_companyName);
        deviceConfig->putString("ssid", arg_ssid);
        deviceConfig->putString("password", arg_password);

        deviceConfig->end();

        request->send(200, "text/html", "<script>alert('Wi-Fi conectado com sucesso. Verifique a autenticação do dispositivo na plataforma NodeIoT.');</script>");
        waiting_function(2500);
        ESP.restart();
      }
    }
    else
    {
      Serial.println("[/device-config] Salvando informações e reiniciando!");
      deviceConfig->begin("deviceConfig", false);

      deviceConfig->putString("deviceId", arg_deviceId);
      deviceConfig->putString("companyName", arg_companyName);
      deviceConfig->putString("ssid", arg_ssid);
      deviceConfig->putString("password", arg_password);

      deviceConfig->end();

      waiting_function(2500);
      ESP.restart();
    }
  });
  
  MDNS.addService("http", "tcp", 80);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  server->addHandler(handler);
  server->onNotFound(std::bind(&RemoteIO::notFound, this, std::placeholders::_1));
  server->begin(); 
}

void RemoteIO::loop()
{
  switchState();
  stateLogic();
}

void RemoteIO::switchState()
{
  switch (connection_state)
  {
    case INICIALIZATION:
      if ((WiFi.status() == WL_CONNECTED) && (Connected == true))
      {
        Serial.println("");
        Serial.println("[INICIALIZATION] Goes to CONNECTED state");
        Serial.println("[INICIALIZATION] Closing access point");

        WiFi.mode(WIFI_STA); 

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
        Serial.println("");
        Serial.println("[CONNECTED] Goes to NO_WIFI state");
        startAccessPoint();
        openLocalServer();
        next_state = NO_WIFI;
      }
      else if (!Connected)
      {
        Serial.println("");
        Serial.println("[CONNECTED] Goes to DISCONNECTED state");
        startAccessPoint();
        openLocalServer();
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
        Serial.println("");
        Serial.println("[NO_WIFI] Goes to DISCONNECTED state");
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
        Serial.println("");
        reconnect_counter = 0;
        Serial.println("[DISCONNECTED] Goes to CONNECTED state");
        Serial.println("[DISCONNECTED] Closing access point");
        WiFi.mode(WIFI_STA); 
        next_state = CONNECTED;
      }
      else if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("");
        Serial.println("[DISCONNECTED] Goes to NO_WIFI state"); 
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
      break;
      
    case NO_WIFI:

      if (millis() - start_reconnect_time >= 10000)
      {
        start_reconnect_time = millis();
        start_debounce_time = millis();
        Serial.println("[NO-WIFI] Calling nodeIotConnection");
        nodeIotConnection(storedCallbackFunction); 
      }
      break;

    case DISCONNECTED:
      
      socketIO.loop();
      socketIOConnect();
      
      if (millis() - start_reconnect_time >= 60000)
      {
        if (reconnect_counter >= 3) ESP.restart();
        else reconnect_counter++;
        start_reconnect_time = millis();
        start_debounce_time = millis();
        Serial.println("");
        Serial.println("[DISCONNECTED] Trying connection...");
        nodeIotConnection(storedCallbackFunction); 
      }
      break;
  }
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
  Serial.printf("\nErasing device configurations saved in non-volatile storage\n");
  delay(1000);
  ESP.restart();
}

void RemoteIO::infoUpdatedEventHandler(JsonDocument payload_doc)
{
  String function = payload_doc[1]["function"].as<String>();
  
  if (function == "restart") rebootDevice();
  else if (function == "reset") eraseDeviceSettings();
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

      Serial.printf("[socketIOEvent] get event: %s id: %d\n", payload, id);
      
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
        String ref = doc[1]["ref"];
        String value = doc[1]["value"];

        if (ref == "restart") rebootDevice();
        else if (ref == "reset") eraseDeviceSettings();

        setIO[ref]["value"] = value;

        if (setIO[ref]["type"].as<String>() == "OUTPUT")
        {
          updatePinOutput(ref);
        }
        
        doc.clear();
        break;
      }
  }
}

void RemoteIO::tryWiFiConnection()
{
  Connected = false;

  if ((_ssid == "") || (_ssid == "null") || (_password == "") || (_password == "null"))
  {
    Serial.println("[tryWiFiConnection] No Wi-Fi network info available...");
    return;
  }

  if (_deviceId != "" && _deviceId != "null")
  {
    String hostname = String("niot-") + String(_deviceId);
    hostname.toLowerCase();
    WiFi.setHostname(hostname.c_str());
  }

  Serial.printf("\n[tryWiFi] Trying connection on %s...\n", _ssid);
  WiFi.begin(_ssid, _password);

  while (WiFi.status() != WL_CONNECTED)
  {
    if ((start_debounce_time != 0) && (millis() - start_debounce_time >= 2000) && (connection_state == NO_WIFI))
    {
      WiFi.disconnect();
      return;
    }
  }
}

void RemoteIO::nodeIotConnection(void (*userCallbackFunction)(String ref, String value))
{
  if ((connection_state == INICIALIZATION || connection_state == NO_WIFI))
  {
    Serial.println("[nodeIotConnection] chamando tryWiFi");
    tryWiFiConnection();
  }

  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.printf("[nodeIotConnection] WiFi connected %s\n", WiFi.localIP().toString().c_str());
  }

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
    }
    
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
    if (Connected) Serial.println("[socketIOConnect] Connected");
    //else Serial.println("[socketIOConnect] Failed connecting");
  }
}

void RemoteIO::setIOs(JsonDocument document)
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
    String ref = document["gpio"][i]["ref"].as<String>();
    int pin = document["gpio"][i]["pin"].as<int>();
    String type = document["gpio"][i]["type"].as<String>();
    String mode = document["gpio"][i]["mode"].as<String>(); 

    if (type == "INPUT" || type == "INPUT_ANALOG")
    {
      setIO[ref]["pin"] = pin;
      setIO[ref]["type"] = type;
      setIO[ref]["mode"] = mode;
      pinMode(pin, INPUT);
    }
    else if (type == "INPUT_PULLUP")
    {
      setIO[ref]["pin"] = pin;
      setIO[ref]["type"] = type;
      setIO[ref]["mode"] = mode;
      pinMode(pin, INPUT_PULLUP);
    }
    else if (type == "INPUT_PULLDOWN")
    {
      setIO[ref]["pin"] = pin;
      setIO[ref]["type"] = type;
      setIO[ref]["mode"] = mode;
      pinMode(pin, INPUT_PULLDOWN);
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
  document.clear();
  
  Serial.print("[tryAuthenticate] Request: ");
  Serial.println(request);

  https.begin(client, appVerifyUrl);
  https.addHeader("Content-Type", "application/json");

  int statusCode = https.POST(request);
  
  String response = https.getString(); 
  
  deserializeJson(document, response);
  
  Serial.print("[tryAuthenticate] Response: ");
  Serial.println(response);

  if (statusCode == HTTP_CODE_OK)
  {
    Serial.println("[tryAuth] HTTP_CODE 200");
    state = document["state"].as<String>();
    
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
      Serial.println("");
      Serial.println("[begin] An error occurred while configuring mDNS");
    }
    
    document.clear();
    deserializeJson(document, deviceSettings);
    
    setIOs(document);
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
    Serial.println("[fetchLatest] HTTP_CODE 200"); 

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
    Serial.print("[fetchLatest] Failed. HTTP_CODE: "); 
    Serial.println(statusCode);
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
    }
    else if (typeRef == "INPUT_ANALOG")
    {
      float valueRef = analogRead(pinRef);
      if (connection_state == CONNECTED) espPOST(ref, String(valueRef));
    }
  }
}

void RemoteIO::notFound(AsyncWebServerRequest *request)
{
  request->send(404, "application/json", "{\"message\":\"Not found\"}");
}

int RemoteIO::espPOST(String variable, String value)
{
    return espPOST(appPostData, variable, value);
}

int RemoteIO::espPOST(String Router, String variable, String value)
{
  if ((WiFi.status() == WL_CONNECTED))
  {
    String route = Router;

    WiFiClientSecure client;
    HTTPClient https;
    JsonDocument document;
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
    else request = value; 
    
    https.begin(client, route); // HTTP
    https.addHeader("Content-Type", "application/json");
    https.addHeader("authorization", "Bearer " + token);

    Serial.print("[espPOST] Request: ");
    Serial.println(request);
    
    int httpCode = https.POST(request);

    String response = https.getString(); 
    Serial.print("[espPOST] Response: ");
    Serial.println(response);
    
    document.clear();
    https.end();
    return httpCode;
  }
  return 0;
}