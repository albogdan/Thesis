void handleCottonCandyGateway(void *params){
  Serial.print("[INFO] handleCottonCandyGateway() running on core ");
  Serial.println(xPortGetCoreID());
  input_byte_array_index = 0;
  byte_array_complete = false;
  while(1) 
  {
    // Check if we received a complete string through SerialArduino
    if (byte_array_complete)
    {
      // Publish the string to MQTT
      Serial.println("[INFO] Parsing input string");
      parse_input_string(input_byte_array);
  
      // Clear the byte array: reset the index pointer
      input_byte_array_index = 0;
      byte_array_complete = false;
    }
  
    // Check if there's anything in the SerialArduino buffer
    /*while (SerialArduino.available())
    {
      // Get the new byte
      byte inByte = (byte)SerialArduino.read();
  
      // Add it to the inputString:
      input_byte_array[input_byte_array_index] = inByte;
      input_byte_array_index += 1;
  
      // If the incoming character is a newline, set a flag so 
      // the main loop can do something about it:
      if (inByte == '\n')
      {
        byte_array_complete = true;
      }
      delay(2);
    }*/

    if (SerialArduino.available()){
      // Read the src
      input_byte_array[input_byte_array_index] = (byte)SerialArduino.read();
      input_byte_array[input_byte_array_index+1] = (byte)SerialArduino.read();

      // Read the length
      input_byte_array[input_byte_array_index + 2] = (byte)SerialArduino.read();

      input_byte_array_index += 3;

      Serial.print("| 0: 0x");
      Serial.print(input_byte_array[0], HEX);
      Serial.print("| 1: 0x");
      Serial.print(input_byte_array[1], HEX);
      Serial.print("| 2: 0x");
      Serial.print(input_byte_array[2], HEX);

    } else {
      continue;
    }

    unsigned int len = input_byte_array[input_byte_array_index-1];
    for (int i=0; i <len; i ++) {
      input_byte_array[input_byte_array_index + i] = (byte)SerialArduino.read();
    }
    Serial.print("| 3: 0x");
    Serial.print(input_byte_array[3], HEX);
    Serial.print("| 4: 0x");
    Serial.print(input_byte_array[4], HEX);
    Serial.print("| 5: 0x");
    Serial.print(input_byte_array[5], HEX);
    Serial.print("| 6: 0x");
    Serial.println(input_byte_array[6], HEX);

    yield();
    delay(10);
  }
  
}
