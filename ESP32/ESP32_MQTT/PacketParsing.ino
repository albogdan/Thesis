
void parse_input_string(byte *input_byte_array) {
    Packet packet;
    packet.src[0] = input_byte_array[0];
    packet.src[1] = input_byte_array[1];
    packet.type = input_byte_array[2];
    packet.seq_num = input_byte_array[3];
    packet.aggregated_message = (input_byte_array[4] & 0x80)>> 7;
    
    // Parse non-aggregated messages here
    if(!packet.aggregated_message) {
        // Get the data
        Serial.println("Non-aggregated packet acquired");
        packet.message_length = input_byte_array[4] & 0x7F;
        packet.node_data_fields = (int*)malloc(packet.message_length*sizeof(int));
        packet.node_data = (int*)malloc(packet.message_length*sizeof(int));
        
        for(int i=0; i<packet.message_length/2; i+=1) {
          /* Uncomment if want to convert to chars
          if (input_byte_array[5+2*i] == SENSOR_TEMPERATURE) {
            packet.node_data_fields[i] = 'T';
          } else if (input_byte_array[5+2*i] == SENSOR_HUMIDITY) {
            packet.node_data_fields[i] = 'H';
          } else if(input_byte_array[5+2*i] == SENSOR_RAINFALL) {
            packet.node_data_fields[i] = 'R';
          } */
          packet.node_data_fields[i] = (int)input_byte_array[5+2*i];
          packet.node_data[i] = (int)input_byte_array[5+2*i+1];
        }
        
        print_packet(packet);
        publish_packet_to_mqtt(packet);
        free(packet.node_data_fields);
        free(packet.node_data);
        
    // Parse aggregated messages here
    } else {
        Serial.println("Aggregated packet acquired");
        packet.agg_src[0] = input_byte_array[5];
        packet.agg_src[1] = input_byte_array[6];
        packet.message_length = input_byte_array[7] & 0x7F;

        packet.node_data_fields = (int*)malloc(packet.message_length*sizeof(int));
        packet.node_data = (int*)malloc(packet.message_length*sizeof(int));
        
        for(int i=0; i<packet.message_length/2; i+=1) {
            packet.node_data_fields[i] = (int)input_byte_array[8+2*i];
            packet.node_data[i] = (int)input_byte_array[8+2*i+1];
        }
        
        print_packet(packet);
        publish_packet_to_mqtt(packet);
        free(packet.node_data_fields);
        free(packet.node_data);
    }
}

void print_packet(Packet payload){
    Serial.print("SRC: 0x");
    Serial.print(payload.src[0], HEX);
    Serial.print(" + 0x");
    Serial.print(payload.src[1], HEX);
    Serial.print(" | TYPE: 0x");
    Serial.print(payload.type, HEX);
    Serial.print(" | SEQ_NUM: 0x");
    Serial.print(payload.seq_num, HEX);
    Serial.print(" | AGG_BOOL: ");
    Serial.print(payload.aggregated_message, DEC);
    Serial.print(" | LEN: ");
    Serial.print(payload.message_length);
    Serial.print(" | DATA: { | ");
    for(int j=0; j<payload.message_length/2; j++) {
        Serial.print(payload.node_data_fields[j]);
        Serial.print(":");
        Serial.print(payload.node_data[j]);
        Serial.print(" | ");
    }
    Serial.println("}");  
}

void publish_packet_to_mqtt(Packet payload) {
    String message;
    // 2 static fields + two arrays of size message_length/2
    // Need 4 space for the time field
    const int capacity = JSON_OBJECT_SIZE(6) + 2*JSON_ARRAY_SIZE(payload.message_length/2);
    DynamicJsonDocument doc(capacity);
        
    update_current_local_time();
    doc["time"] = now;
    
    if (payload.aggregated_message) {
        doc["src_1"] = payload.agg_src[0];
        doc["src_2"] = payload.agg_src[1];
    } else {
        doc["src_1"] = payload.src[0];
        doc["src_2"] = payload.src[1];
    }
    JsonArray node_fields = doc.createNestedArray("node_fields");
    JsonArray node_data = doc.createNestedArray("node_data");
    for(int i=0; i< payload.message_length/2; i++){
        node_fields.add(payload.node_data_fields[i]);
        node_data.add(payload.node_data[i]);
    }
    serializeJson(doc, message);
    String topic = "data/" + HOSTNAME;
    char topic_char_array[topic.length() + 1];
    char message_char_array[message.length() + 1];

    topic.toCharArray(topic_char_array, topic.length() + 1);
    message.toCharArray(message_char_array, message.length() + 1);
    Serial.print("Publishing to MQTT: ");
    Serial.println(message);
    Serial.println();
    mqttClient.publish(topic_char_array, message_char_array);

    yield();
}
