long lastReconnectAttempt = 0;

bool setupAndConnectToMqtt() {
  Serial.println("[INFO] Getting MQTT server credentials");
  MQTTCredentials credentials;
  if (getMQTTCredentials(&credentials)) {
    Serial.println("[SETUP] Checking if MQTT server exists");
    bool ping_success = true;//WiFi.ping(credentials.server_ip); //Ping 3 times
    mqtt_app_start(&credentials);

  }
  return false;
}

static void mqtt_app_start(MQTTCredentials *credentials)
{
    esp_mqtt_client_config_t mqtt_cfg;
    
    
//    mqtt_cfg.event_handle = mqtt_event_handler;/
//    mqtt_cfg.host = credentials->server_ip;
//    mqtt_cfg.port = strtol(credentials->port, NULL, 10);
//    mqtt_cfg.username = credentials->username;
//    mqtt_cfg.password = credentials->password;
    //mqtt_cfg.host = "142.150.235.12";
    mqtt_cfg.uri = "mqtt://student:ece1528@142.150.235.12:1883";
//    mqtt_cfg.host = NULL;/
    //mqtt_cfg.port = 1883;
    //mqtt_cfg.username = "student";
    //mqtt_cfg.password = "ece1528";

    //ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    Serial.println("Setting up client");
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    Serial.println("Starting client");
    esp_mqtt_client_start(client);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


void setupMqtt(char *mqttServer, char *mqttPort)
{
    mqttClient.setServer(mqttServer, atoi(mqttPort));
    mqttClient.setCallback(callbackMqtt);
    mqttClient.setBufferSize(1024);   // MAKE SURE TO CHECK THAT THIS IS BIG ENOUGH
    mqttClient.setSocketTimeout(5);
}

boolean connectToMqtt(char *username, char *password)
{
    char hostname_char[HOSTNAME.length() + 1];
    HOSTNAME.toCharArray(hostname_char, HOSTNAME.length() + 1);

    Serial.println("[SETUP] Connecting to MQTT...");
    if (mqttClient.connected()) {
      Serial.println("[INFO] MQTT Client is already connected!");
    }else if (mqttClient.connect(hostname_char, username, password))
    { //, mqttUser, mqttPassword )) {
        Serial.print("[INFO] MAC: ");
        Serial.println(WiFi.macAddress());
        Serial.println("[SETUP] Connected to MQTT!\n");
        mqttSubscribeToTopics(&mqttClient);
        return true;
    }
    else
    {
        Serial.print("[SETUP] Connection failed with state ");
        Serial.println(mqttClient.state());
        delay(200);
        return false;
    }
}


void loop_and_check_mqtt_connection()
{
    if (!mqttClient.connected())
    {
        long now_mqtt = millis();
        if (now_mqtt - lastReconnectAttempt > 10000L)
        {
            lastReconnectAttempt = now_mqtt;
            // Attempt to reconnect
            mqtt_connection_made = setupAndConnectToMqtt();
            if (mqtt_connection_made)
            {
                lastReconnectAttempt = 0;
            }
        }
    }
    else
    {
        mqttClient.loop();
    }
}



bool setMQTTCredentials(MQTTCredentials *credentials) {
    // Creating new file
    File configFile = SPIFFS.open("/mqtt_config.json", "w");
    
    if (configFile) {
      Serial.println("[INFO] Opened MQTT config file");
      DynamicJsonDocument json(1024);
      json["server_ip"] = credentials->server_ip;
      json["port"] = credentials->port;
      json["username"] = credentials->username;
      json["password"] = credentials->password;
      
      //serializeJson(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      return true;
    }
  Serial.println("Returning false");
  return false; 
}

bool getMQTTCredentials(MQTTCredentials *credentials) {
  if (SPIFFS.exists("/mqtt_config.json")) {  
    //File exists, reading and loading
    File configFile = SPIFFS.open("/mqtt_config.json", "r");
    
    if (configFile) {
      Serial.println("[INFO] Opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      //serializeJson(json, Serial);
      if ( ! deserializeError ) {
        strcpy(credentials->server_ip, json["server_ip"]);
        strcpy(credentials->port, json["port"]);
        strcpy(credentials->username, json["username"]);
        strcpy(credentials->password, json["password"]);
        
        configFile.close();
        Serial.print("[INFO] Server IP: ");
        Serial.println(credentials -> server_ip);
        Serial.print("[INFO] Port: ");
        Serial.println(credentials -> port);
        Serial.print("[INFO] Username: ");
        Serial.println(credentials -> username);
        Serial.print("[INFO] Password: ");
        Serial.println(credentials -> password);
        return true;
      } else {
        Serial.println("[INFO] Failed to load JSON config");
        configFile.close();
        return false;
      }
    }
    Serial.println("[INFO] No config file found");
  }
  return false;
}

void mqttSubscribeToTopics(PubSubClient *mqttClient)
{
    mqttClient->subscribe("ping");

    // All subscriptions should be of the form {HOSTNAME/endpoint}
    mqttClient->subscribe("ESP32_LORA/test");    // Subscribe function only takes char*, not String
}

void callbackMqtt(char *topic, byte *payload, unsigned int length)
{

    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    String topic_string = String(topic);

    Serial.print("Message:");
    String message = "";
    for (int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    Serial.println(message);

    if (topic_string.equals("ping"))
    {
        Serial.println("Calling ping response");
        return_ping();
    }
    else if (topic_string.equals("test"))
    {
        Serial.println("Message received in test topic");
    }

    Serial.println();
    Serial.println("-----------------------");
}

// Returns a ping with information about the system
void return_ping()
{
    String message;
    const size_t capacity = JSON_OBJECT_SIZE(8);
    DynamicJsonDocument doc(capacity);

    doc["ip_address"] = WiFi.localIP().toString();
    doc["mac_address"] = WiFi.macAddress();
    doc["hostname"] = HOSTNAME;
    doc["current_ssid"] = WiFi.SSID();

    serializeJson(doc, message);

    String topic = "ping/response/" + HOSTNAME;

    char topic_output[topic.length() + 1];
    char message_output[message.length() + 1];

    topic.toCharArray(topic_output, topic.length() + 1);
    message.toCharArray(message_output, message.length() + 1);

    Serial.print("Replying with: ");
    Serial.println(message);
    mqttClient.publish(topic_output, message_output);

    yield();
}

void publish_arduino_data(String arduino_data) {
    String message;
    const size_t capacity = JSON_OBJECT_SIZE(4);
    DynamicJsonDocument doc(capacity);

    doc["data_1"] = arduino_data.substring(0, arduino_data.indexOf(','));
    doc["data_2"] = arduino_data.substring(arduino_data.indexOf(',')+1, arduino_data.indexOf('\r'));

    serializeJson(doc, message);

    String topic = "data/" + HOSTNAME;

    char topic_char_array[topic.length() + 1];
    char message_char_array[message.length() + 1];

    topic.toCharArray(topic_char_array, topic.length() + 1);
    message.toCharArray(message_char_array, message.length() + 1);

    Serial.print("Publishing to MQTT: ");
    Serial.println(message);
    mqttClient.publish(topic_char_array, message_char_array);

    yield();
}

void publish_test_data() {
    String payload = "Test message";
    String message;
    // 2 static fields + two arrays of size message_length/2
    // Need 4 space for the time field
    const int capacity = JSON_OBJECT_SIZE(6) + 2*JSON_ARRAY_SIZE(payload.length()/2);
    DynamicJsonDocument doc(capacity);

    update_current_local_time();
    String src = "0x99";
    doc["src"] = src;

    update_current_local_time();
    doc["time"] = now;
    doc["data"] = payload;
    
    serializeJson(doc, message);
    String topic = "data/" + HOSTNAME;
    char topic_char_array[topic.length() + 1];
    char message_char_array[message.length() + 1];

    topic.toCharArray(topic_char_array, topic.length() + 1);
    message.toCharArray(message_char_array, message.length() + 1);
    Serial.print("Publishing to MQTT topic ' ");
    Serial.print(topic);
    Serial.print(" ':");
    Serial.println(message);
    Serial.println();
    mqttClient.publish(topic_char_array, message_char_array);
    
    yield();
}
