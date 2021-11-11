// Message type definitions
#define MESSAGE_JOIN              1
#define MESSAGE_JOIN_ACK          2
#define MESSAGE_JOIN_CFM          3
#define MESSAGE_CHECK_ALIVE       4
#define MESSAGE_REPLY_ALIVE       5
#define MESSAGE_GATEWAY_REQ       6
#define MESSAGE_NODE_REPLY        7


// Define sensor types
#define SENSOR_TEMPERATURE 1
#define SENSOR_HUMIDITY 2
#define SENSOR_RAINFALL 3

// Arrays for data received with no-aggregation
// |  src  |  type  |  seq num  | agg. bit + length |  data  |
// |  2B   |  1B    |    1B     |          1B         |  0-64B |
// Note: agg. bit defines whether data is aggregated. In the layout
// above, there is no aggregation, so the bit is 0.
// Note: type = node_reply, etc

// src[0], src[1], type, seq_num, agg. bit + length, data[0:length]
byte payload_non_agg_0[7] = {0x70,                  // src[0]
                       0xA0,                  // src[1]
                       MESSAGE_NODE_REPLY,    // type
                       0x01,                  // seq_num
                       0x00 | 2,              // agg. bit 0 + length
                       SENSOR_TEMPERATURE, 0x1A              // Temperature, 26
                      };
//Payload sent: 1121607128426

byte payload_non_agg_1[7] = {0x70,            // src[0]
                       0xA1,                  // src[1]
                       MESSAGE_NODE_REPLY,    // type
                       0x02,                  // seq_num
                       0x00 | 2,              // agg. bit 0 + length
                       SENSOR_HUMIDITY, 0x4F              // Humidity, 79
                      };
//Payload sent: 1121617227279

byte payload_non_agg_2[9] = {0x70,            // src[0]
                       0xA2,                  // src[1]
                       MESSAGE_NODE_REPLY,    // type
                       0x03,                  // seq_num
                       0x00 | 4,              // agg. bit 0 + length
                       SENSOR_HUMIDITY, 0x4E, SENSOR_TEMPERATURE, 0x1B   // Humidity, 78, Temperature, 27
                      };
//Payload sent: 11216273472788427

byte payload_non_agg_3[11] = {0x70,            // src[0]
                       0xA3,                  // src[1]
                       MESSAGE_NODE_REPLY,    // type
                       0x04,                  // seq_num
                       0x00 | 6,              // agg. bit 0 + length
                       SENSOR_RAINFALL, 0x80, SENSOR_HUMIDITY, 0x40, SENSOR_TEMPERATURE, 0x1F   // Rainfall, 128mm, Humidity, 64, Temperature, 31
                      };
//Payload sent: 1121637468212872648431

// Arrays for data received with no-aggregation
// |  src_0  |  type  |  seq num  | agg. bit + length   |  src_0  |  agg. bit + length   |  data  |
// |    2B   |  1B    |    1B     |          1B         |    2B   |           1B         |  0-61B |
// Note: agg. bit defines whether data is aggregated. In the layout
// above, there is aggregation, so the bit is 1.
// Note: type = node_reply, etc

// src[0], src[1], type, seq_num, agg. bit + length, data[0:length]
byte payload_agg_0[10] = {0x70,                  // src[0]
                         0xA4,                  // src[1]
                         MESSAGE_NODE_REPLY,    // type
                         0x01,                  // seq_num
                         0x80 | 5,              // agg. bit 1 + length
                            // Begin mini-packet
                            0x70,                 // src[0]
                            0xB1,                 // src[1]
                            0x00 | 2,              // agg. bit 0 + length
                            SENSOR_TEMPERATURE, 0x1A              // Temperature, 26
                      };
byte payload_agg_1[10] = {0x70,                  // src[0]
                         0xA5,                  // src[1]
                         MESSAGE_NODE_REPLY,    // type
                         0x02,                  // seq_num
                         0x80 | 5,              // agg. bit 1 + length
                            // Begin mini-packet
                            0x70,                 // src[0]
                            0xB2,                 // src[1]
                            0x00 | 2,              // agg. bit 0 + length
                            SENSOR_HUMIDITY, 0x3B              // Humidity, 59
                      };
byte payload_agg_2[12] = {0x70,                  // src[0]
                         0xA6,                  // src[1]
                         MESSAGE_NODE_REPLY,    // type
                         0x03,                  // seq_num
                         0x80 | 7,              // agg. bit 1 + length
                            // Begin mini-packet
                            0x70,                 // src[0]
                            0xB3,                 // src[1]
                            0x00 | 4,              // agg. bit 0 + length
                            SENSOR_HUMIDITY, 0x3F, SENSOR_TEMPERATURE, 0x1B   // Humidity, 63, Temperature, 27
                      };
byte payload_agg_3[14] = {0x70,                  // src[0]
                         0xA7,                  // src[1]
                         MESSAGE_NODE_REPLY,    // type
                         0x04,                  // seq_num
                         0x80 | 9,              // agg. bit 1 + length
                            // Begin mini-packet
                            0x70,                 // src[0]
                            0xB4,                 // src[1]
                            0x00 | 6,              // agg. bit 0 + length
                            SENSOR_RAINFALL, 0x30, SENSOR_HUMIDITY, 0x10, SENSOR_TEMPERATURE, 0x0F   // Rainfall, 48mm, Humidity, 16, Temperature, 15
                      };



#define EXAMPLE_PAYLOAD_NON_AGG_COUNT 4  // Number of payloads in the non-aggregated array
byte* payloads_non_agg[EXAMPLE_PAYLOAD_NON_AGG_COUNT] = {payload_non_agg_0, payload_non_agg_1, payload_non_agg_2, payload_non_agg_3};

#define EXAMPLE_PAYLOAD_AGG_COUNT 4  // Number of payloads in the non-aggregated array
byte* payloads_agg[EXAMPLE_PAYLOAD_AGG_COUNT] = {payload_agg_0, payload_agg_1, payload_agg_2, payload_agg_3};



/*
// For Gateway only: The first bit of the address has to be 1
byte myAddr[2] = {0x80, 0xA0};
// For Node: 
byte myAddr[2] = {0x70, 0xA0};
*/
