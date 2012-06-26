/*
 Smart Star - the internet controllable, high voltage Christmas decoration 
  - activates 3 relays that each control a rope light arranged in a patterm around a start shape
  - Illuminates each light according to predefined pattern sequence  
  - connects to an MQTT server over Wifi for control messages (on, off and maybe more later)
  
  Compatibly WiFly Libary can be found here https://github.com/dpslwk/WiFly
  Current WiFly Libary only supports DHCP out the box
  
*/

#include <SPI.h>
#include <WiFly.h>
#include <PubSubClient.h>

#include "Credentials.h"

#define MQTT_CLIENT "SmartStar"
#define IN_TOPIC "smartstar/in"
#define OUT_TOPIC "smartstar/out"

#define STEPS 32

// Update these with values suitable for your network.
byte mqtt_server[] = { 192, 168, 0, 12 };
int mqtt_port = 1883;

const int lights[] = {5, 6, 7}; // Arduino pins that will control the light relays

boolean active = true; // primary switch to set lights on or off 
unsigned long light_alarm;  // time of next light pattern change, usually around a second after last change
unsigned long connect_alarm; // time of next connection check;
boolean wifi_connected = false;

// Pattern definition 
int step = 0;

boolean patterns[STEPS][3] = {
  {false, false, false}, 
  {true, false, false},
  {false, true, false},
  {false, false, true},
  {false, false, false},
  {false, false, true},
  {false, true, false},
  {true, false, false},
  {false, false, false},
  {true, false, false},
  {true, true, false},
  {false, true, false},
  {false, true, true},  
  {false, false, true},
  {false, false, false},
  {false, false, true},
  {false, true, true},
  {false, true, false},
  {true, true, false},
  {true, false, false},
  {false, false, false},
  {true, false, false},
  {true, true, false},
  {true, true, true},
  {false, true, true},
  {false, false, true},
  {false, false, false},
  {false, false, true},
  {false, true, true},
  {true, true, true},
  {true, true, false},
  {true, false, false}};


// The IP network client we will be using 
WiFlyClient wiFlyClient;

/*
 * callback method invoked when pubsubclient receives a message from the MQTT server
 *
 */
void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print(length);
  Serial.print(" Message received from topic ");
  Serial.println(topic);
  
  //Serial.write(payload, length);
  
  // convert the byte array received into a String
  char buffer[length];
  for(int x = 0; x < length; x++) {
    buffer[x] = payload[x];
  }
  
  String message = String(buffer);
  
  Serial.print(">");
  Serial.println(message);
  
  // decide what action to take based on message received
  if (message.startsWith("on")) {
    step = 0;
    active = true;
  } else if (message.startsWith("off")) {
    light_pattern(0);
    active = false;
  }  
}

// Initialise the MQTT pubsubclient
PubSubClient client(mqtt_server, mqtt_port, callback, wiFlyClient);

/*
 * setup method invoked once at Arduino startup
 *
 */
void setup() {
  Serial.begin(9600);   // Start hardware Serial for the RN-XV
  Serial.println("Booting up...");
  
  // initialise the alarm to current time so that it will be triggered immediately
  light_alarm = millis();
  
  // initialize the digital pins as an output.
  for(int x = 0; x < 3; x++) {
    pinMode(lights[x], OUTPUT);
  }

  WiFly.begin();
    
  ensure_connected();
}

/*
 * main control method invoked continuously by Arduino
 *
 */
void loop()
{
  // Check if the system is "on" and the alarm has been triggered
  if (active && millis() > light_alarm) {
    update_lights();
  }
  
  if (millis() > connect_alarm) {
    ensure_connected(); 
  }
  
  client.loop();
}

/*
 * method to make sure we have a connection to the Wifi and MQTT server
 *
 */
void ensure_connected() {
  connect_alarm = millis() + 30000;
  
  if (!client.connected()) {
    
    light_pattern(0); // turn lights off temporarily just in case this hangs
    
//    if (!wiFlyClient.connected()) {
    if (!wifi_connected) {
      Serial.print("Reset any previous Wifi connection...");
      wiFlyClient.stop();
      Serial.println("OK");
  
      Serial.print("Connecting to WiFi...");
      WiFly.begin();
  
      // Join the WiFi network
      if (!WiFly.join(ssid, passphrase, mode)) {
        Serial.println(" Failed");
        wifi_connected = false;  
        return;    } 
  
      wifi_connected = true;  
      Serial.println(" OK");
    }

    mqtt_connect();    
  }
}

void mqtt_connect() {
    Serial.print("Reset any previous mqtt connection...");
    //client.disconnect();
    Serial.println("OK");
    Serial.print("Connecting to MQTT Broker...");
    if (client.connect(MQTT_CLIENT)) {
      Serial.println("OK");
      client.publish(OUT_TOPIC,"hello world");
      client.subscribe(IN_TOPIC);
    } else {
      Serial.println("Failed");
    }
}

/*
 * update_lights method displays the current light pattern and then sets the alarm for the next pattern
 * Setting the alarm here means we could vary the duration of each pattern step with just a little extra code.
 */
void update_lights() {
  light_pattern(step);  
  step++;
  if(step >= STEPS) {
    step = 0;
  }
  light_alarm = millis() + 600; // How long until the next pattern change
}

/*
 * light_pattern method switches on the individual relays that display the light pattern
 *
 */
void light_pattern(int step) {
    for(int x = 0; x < 3; x++) {
    // Serial.print(patterns[step][x]);
    if(patterns[step][x]) {
      digitalWrite(lights[x], HIGH);    // turn the Light on by making the voltage HIGH
    } else {
      digitalWrite(lights[x], LOW);   // turn the Light off (LOW is the voltage level)
    }
  }
  //Serial.println("");
}
