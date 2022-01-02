/* ----- BEGIN WIFI SETTINGS ----- */
String HOSTNAME = "ESP32_LORA";
const char* ssid     = "ssid_here";
const char* password = "password_here";
const char *mqttServer = "mqtt_server_here";
/* ----- END WIFI SETTINGS ----- */

/* ----- BEGIN MQTT SETTINGS ----- */
const int mqttPort = 1883;
long lastReconnectAttempt = 0;
/* ----- END MQTT SETTINGS ----- */

/* ----- BEGIN PACKET INFORMATION AND DEFINITIONS ----- */
//              Message parts and sizes diagram
// |  src  |  type  |  seq num  | agg. bit + length |  data  |
// |  2B   |  1B    |    1B     |          1B       |  0-64B |
typedef struct packet {
  byte src[2];
  byte agg_src[2];
  byte type;
  byte seq_num;
  bool aggregated_message;
  unsigned int message_length;
  int *node_data_fields;
  int *node_data;
  long data_value;
} Packet;

/* Questions:
 *  Do we want to send the type too?
 *  Do we want to send the seq num too?
 */
/* ----- END PACKET INFORMATION AND DEFINITIONS ----- */


/* ----- BEGIN SERIAL SETTINGS ----- */
// The maximum size of a message - this is calculated from the 
// message metadata plus data max size. See diagram above.
#define MAX_PAYLOAD_SIZE 5+64 
byte input_byte_array[3*MAX_PAYLOAD_SIZE]; // Extra large just in case

unsigned int input_byte_array_index = 0;
bool byte_array_complete = false;

// Define the pins for the Arduino serial port
#define RXD2 16
#define TXD2 17

/* ----- END SERIAL SETTINGS ----- */

/* ----- BEGIN TIME VARIABLES ----- */
/* Timer related defines */
#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

#define SLEEP_TIMER_SECONDS 8 // Amount of time to wait until trying to go into hibernation mode
#define RTC_GPIO_WAKEUP_PIN GPIO_NUM_4
#define ARDUINO_SIGNAL_READY_PIN GPIO_NUM_5


time_t now;
char strftime_buf[64];
struct tm timeinfo;
/* ----- END TIME VARIABLES ----- */


// Define sensor types
#define SENSOR_TEMPERATURE 1
#define SENSOR_HUMIDITY 2
#define SENSOR_RAINFALL 3



void hibernate_mode();
void print_wakeup_reason();
static void initialize_sleep_timer(unsigned int timer_interval_sec);
static void sleep_timer_isr_callback(void* args);


WiFiClient espClient;
PubSubClient mqttClient(espClient);
