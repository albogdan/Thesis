typedef union{
  float f;
  byte b[4];
}doubleConverter;

void parse_input_string(byte *input_byte_array) {
    Packet packet;
    packet.src[0] = input_byte_array[0];
    packet.src[1] = input_byte_array[1];
    packet.message_length = input_byte_array[2];
    // input_byte_array[3] = gateway_addr[0]
    // input_byte_array[4] = gateway_addr[1]
    // input_byte_array[5] = 0
    // input_byte_array[6] = 0
    // input_byte_array[7] = data start
    /*Serial.print("0: 0x");
    Serial.println(input_byte_array[0], HEX);
    Serial.print("1: 0x");
    Serial.println(input_byte_array[1], HEX);
    Serial.print("2: 0x");
    Serial.println(input_byte_array[2], HEX);
    Serial.print("3: 0x");
    Serial.println(input_byte_array[3], HEX);
    Serial.print("4: 0x");
    Serial.println(input_byte_array[4], HEX);
    Serial.print("5: 0x");
    Serial.println(input_byte_array[5], HEX);
    Serial.print("6: 0x");
    Serial.println(input_byte_array[6], HEX);
    Serial.print("7: 0x");
    Serial.println(input_byte_array[7], HEX);
    Serial.print("8: 0x");
    Serial.println(input_byte_array[8], HEX);
    Serial.print("9: 0x");
    Serial.println(input_byte_array[9], HEX);
    Serial.print("10: 0x");
    Serial.println(input_byte_array[10], HEX);
    Serial.print("11: 0x");
    Serial.println(input_byte_array[11], HEX);
    Serial.print("12: 0x");
    Serial.println(input_byte_array[12], HEX);*/
    if (packet.src[0] == 0 && packet.src[1] == 0) {
      Serial.println("Initial packets, ignoring");
    } else {
      /*Serial.print("Packet src: 0x");
      Serial.print(packet.src[0], HEX);
      Serial.println(packet.src[1], HEX);
      Serial.print("Packet length: ");
      Serial.println(packet.message_length);*/
      byte float_data[4] = {input_byte_array[5], input_byte_array[6], input_byte_array[7], input_byte_array[8]};

      packet.data_value = *( (float*) float_data );
      /*Serial.print("Message: ");
      Serial.println(packet.data_value);
      Serial.println(packet.data_value, HEX);*/
      publish_packet_to_mqtt(packet);
      //print_packet(packet);
    }
}

void print_packet(Packet payload){
    Serial.print("SRC: 0x");
    Serial.print(payload.src[0], HEX);
    Serial.print(" + 0x");
    Serial.print(payload.src[1], HEX);
    Serial.print(" | LEN: ");
    Serial.print(payload.message_length);
    Serial.print(" | DATA: { | ");
    Serial.print(payload.data_value);
    Serial.println("}");  
}

void publish_packet_to_mqtt(Packet payload) {
    String message;
    // 2 static fields + two arrays of size message_length/2
    // Need 4 space for the time field
    const int capacity = JSON_OBJECT_SIZE(6) + 2*JSON_ARRAY_SIZE(payload.message_length/2);
    DynamicJsonDocument doc(capacity);

    update_current_local_time();
    String src = "0x" + String(0 | (payload.src[0] << 8) | payload.src[1], HEX);
    doc["src"] = src;

    update_current_local_time();
    doc["time"] = now;
    doc["data"] = payload.data_value;
    
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
    //digitalWrite(ARDUINO_SIGNAL_READY_PIN, LOW);
    
    yield();
}
