/**************************************************************
 *
 * This script tries to auto-detect the baud rate
 * and allows direct AT commands access
 *
 * TinyGSM Getting Started guide:
 *   https://tiny.cc/tinygsm-readme
 *
 **************************************************************/


// Select your modem:
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>

// Set serial for debug console (to the Serial Monitor, speed 115200)
#define SerialMon Serial
#define SerialAT Serial2
#define TINY_GSM_DEBUG SerialMon

#define MAX_BUFFER_SIZE 10
String input_string_array[MAX_BUFFER_SIZE]; // Extra large just in case

unsigned int input_string_array_index = 0;
unsigned int parsing_string_array_index = 0;
bool string_array_complete = false;

String commandList [] = {
  "AT+CGSOCKCONT=1,\"IP\",\"iot.truphone.com\"",
  "AT+CMQTTSTART",
  "AT+CMQTTACCQ=0,\"client\"",
  "AT+CMQTTCONNECT=0,\"tcp://142.150.235.12:1883\",60,1,\"student\",\"ece1528\"",
  "AT+CMQTTTOPIC=0,12",
  "publish_test",
  "AT+CMQTTPAYLOAD=0,29",
  "this is a test, hello world 1",
  "AT+CMQTTPUB=0,1,60"
};
enum states {
  APN_SOCKET_CREATE,
  MQTT_START,
  MQTT_ACC,
  MQTT_CONNECT,
  MQTT_TOPIC,
  MQTT_PAYLOAD,
  MQTT_PUBLISH  
};


void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(500);
  SerialAT.begin(115200);
  delay(500);
  
  // Access AT commands from Serial Monitor
  SerialMon.println(F("***********************************************************"));
  SerialMon.println(F(" You can now send AT commands"));
  SerialMon.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
  SerialMon.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
  SerialMon.println(F("***********************************************************"));
}


int current_command = APN_SOCKET_CREATE;
bool current_command_complete = true;

void loop() {
  switch(current_command) {
    case APN_SOCKET_CREATE:
      SerialAT.println("AT+CGSOCKCONT=1,\"IP\",\"iot.truphone.com\"");
      delay(500);
      current_command = MQTT_START;
      break;

    case MQTT_START:
      SerialAT.println("AT+CMQTTSTART");
      delay(500);
      current_command = MQTT_ACC;
      break;
    
    case MQTT_ACC:
      SerialAT.println("AT+CMQTTACCQ=0,\"client\"");
      delay(500);
      current_command = MQTT_CONNECT;
      break;
    
    case MQTT_CONNECT:
      SerialAT.println("AT+CMQTTCONNECT=0,\"tcp://142.150.235.12:1883\",60,1,\"student\",\"ece1528\"");
      delay(500);
      current_command = MQTT_TOPIC;
      break;
    
    case MQTT_TOPIC:
      SerialAT.println("AT+CMQTTTOPIC=0,12");
      delay(500);
      SerialAT.println("publish_test");
      delay(500);
      current_command = MQTT_PAYLOAD;
      break;
    
    case MQTT_PAYLOAD:
      SerialAT.println("AT+CMQTTPAYLOAD=0,29");
      delay(500);
      SerialAT.println("this is a test, hello world 1");
      delay(500);
      current_command = MQTT_PUBLISH;
      break;
    
    case MQTT_PUBLISH:
      SerialAT.println("AT+CMQTTPUB=0,1,60");
      delay(500);
      current_command = -1;
      break;
    
    default:
      break;
  }

  /*
  if(current_command_complete) {
    SerialAT.println(commandList[current_command]);
    //delay(100);
    if (commandList[current_command].substring(0,13) == "AT+CMQTTTOPIC"){
      current_command +=1;
      SerialAT.println(commandList[current_command]);
    }
    if (commandList[current_command].substring(0,15) == "AT+CMQTTPAYLOAD"){
      current_command +=1;
      SerialAT.println(commandList[current_command]);
      
    }
    //SerialMon.println(commandList[current_command]);
    current_command_complete = false;
  }*/

/*
  if (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
*/  
  // Send data coming in from SerialMon (PC) to the cellular board
  if (SerialMon.available()) {
    SerialAT.write(SerialMon.read());
  }

  // Check if we received a complete string through SerialArduino
  if (string_array_complete)
  {
    // Parse the string and publish to MQTT
   // parse_input_string(&input_string_array[parsing_string_array_index]);

    // Clear the byte array: reset the index pointer
    parsing_string_array_index = (parsing_string_array_index + 1) % MAX_BUFFER_SIZE;
    string_array_complete = false;
  }

  // Check if there's anything in the SerialArduino buffer
  while (SerialAT.available())
  {
    // Get the new byte
    byte inByte = (byte)SerialAT.read();
    SerialMon.write(inByte);

    // Add it to the inputString:
    input_string_array[input_string_array_index] += (char)inByte;
    

    // If the incoming character is a newline, set a flag so 
    // the main loop can do something about it:
    if (inByte == '\n')
    {
      input_string_array_index = (input_string_array_index + 1) % MAX_BUFFER_SIZE;
      input_string_array[input_string_array_index] = "";
      string_array_complete = true;
    }
  }
}


void parse_input_string(String *input_string) {
  //SerialMon.print("[INFO] ");
  SerialMon.println(*input_string);
  if((*input_string).substring(0,2) == "OK"){
    current_command_complete = true;
    //current_command +=1;
  }
  if((*input_string).substring(0,5) == "ERROR"){
    current_command_complete = true;
    //current_command +=1;
  }
  if (current_command == 5 || current_command == 7){
    current_command_complete = true;
    //current_command +=1;
  }
  //SerialMon.print("Current command is: ");
  //SerialMon.println(current_command);
}


void connectToCellularMQTT(){
  SerialAT.println("AT+CGSOCKCONT=1,\"IP\",\"iot.truphone.com\"");
  delay(1000);
  SerialAT.println("AT+CMQTTSTART");
  delay(1000);
  SerialAT.println("AT+CMQTTACCQ=0,\"client\"");
  delay(1000);
  SerialAT.println("AT+CMQTTCONNECT=0,\"tcp://142.150.235.12:1883\",60,1,\"student\",\"ece1528\"");
  delay(1000);
  SerialAT.println("AT+CMQTTTOPIC=0,12");
  delay(1000);
  SerialAT.println("publish_test");
  delay(1000);
  SerialAT.println("AT+CMQTTPAYLOAD=0,29");
  delay(1000);
  SerialAT.println("this is a test, hello world 1");
  delay(1000);
  SerialAT.println("AT+CMQTTPUB=0,1,60");
}


/* AT COMMANDS REQUIRED

// Setup of MQTT connection
AT+CGSOCKCONT=1,"IP","iot.truphone.com"
AT+CMQTTSTART
AT+CMQTTACCQ=0,"client"
AT+CMQTTCONNECT=0,"tcp://142.150.235.12:1883",60,1,"student","ece1528"

// Publish a message to a topic
// Set the topic first
AT+CMQTTTOPIC=0,12
publish_test
// Set the payload for the PUBLISH message
AT+CMQTTPAYLOAD=0,27
this is a test, hello world
// Publish a message
AT+CMQTTPUB=0,1,60

// Subscribe to a topic
AT+CMQTTSUB=0,9,1\nTOPIC

*/
