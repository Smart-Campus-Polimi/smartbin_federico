/*
 * TODO:
 *  - Set the topic of height and weight as: id/height and id/weight
 *  - Setup the deep sleep
 *  - Change server and wifi
 */
#include <WiFi101.h>
#include <PubSubClient.h>
#include <HX711_ADC.h>
#include <FlashStorage.h>
#include <FlashAsEEPROM.h>
#include <RTCZero.h>

//MKR1000 pin
#define D0    0 //TRIGGER - White
#define D1    1 //ECHO - Violet
#define D2    2
#define D3    3
#define D4    4 //CLK - Green
#define D5    5 //DAT - Blue
#define D6    6
#define D7    7

//Wifi 
#define UNI 1
//Is this a testing sketch?
#define TESTING 0
//Are you using iot wifi line?
#define IOT_WIFI 1

#define DEEP_SLEEP true

#if TESTING == 1
  //Define the time that passes before next sampling
  #define SLEEPHH 0
  #define SLEEPMIN 0
  #define SLEEPSEC 25
  //Threshold to send the measure
  #define THR_HEIGHT 2 //Difference in percentage befere send new measure
  #define THR_MIN_HEIGHT 1 //The min number of cm that must change to ensure that the height has changed
  #define THR_WEIGHT 5 //Difference in grams befere send new measure
  #define MAX_TIME_NO_SEND 1 //Maximum number of iteration without sending a message
#else
  #define SLEEPHH 1
  #define SLEEPMIN 0
  #define SLEEPSEC 0
  #define THR_HEIGHT 5 //Difference in percentage befere send new measure
  #define THR_MIN_HEIGHT 1 //The min number of cm that must change to ensure that the height has changed
  #define THR_WEIGHT 5 //Difference in grams befere send new measure
  //Send at least every hour
  #define MAX_TIME_NO_SEND 6
#endif

#if IOT_WIFI == 0
  //MQTT info
  #define mqtt_server ""
  #define mqtt_user ""
  #define mqtt_password ""

  #if UNI == 1
    //Wifi lab
    #define wifi_ssid ""
    #define wifi_password ""
  #else
    //Wifi casa
    #define wifi_ssid ""
    #define wifi_password ""
  #endif
#else
  //MQTT info
  #define mqtt_server "10.172.0.11"
  #define mqtt_user ""
  #define mqtt_password ""

  #define wifi_ssid "IoTPolimi"
  #define wifi_password "ZpvYs=gT-p3DK3wb"
#endif


#define weight_topic "/weight"
#define height_topic "/height"

//Bin fixed data
#define STABILISING_TIME 3000
#define OFFSET 135292655
#define SCALE_FACTOR -206.53

//Instantiate cell
HX711_ADC LoadCell(D5, D4);
//MQTT client
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
//Timer
RTCZero rtc;

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 00;
const byte hours = 9;

/* Change these values to set the current initial date */
const byte day = 31;
const byte month = 5;
const byte year = 18;

//Values
long height = 2;
long weight = 3;
int successive_boot = -1;
long old_height = -100;
long old_weight = -100;
int passed_cycles = 0;
unsigned long id = 0;

//Tare value
long bin_height;
long offset;
byte mac[6];

FlashStorage(first_boot_storage, int);
FlashStorage(height_tare_storage, long);
FlashStorage(weight_tare_storage, long);

void setup() {
  delay(5000);
  if ( ! TESTING ) {
    Serial.println("Unplug before starting calibration");
    delay(3000);
  }
  
  Serial.println("Start tare");
  //Tare everything at startup
  tare();
  //get id
  id = getId();
  Serial.println("ID: "+String(id));

  
  Serial.println("Setting timer");
  rtc.begin();
  rtc.setTime(hours, minutes, seconds); //set time
  rtc.setDate(day, month, year); //set date
}


void loop() {
  Serial.println("_____START_____");

  action();

  if (successive_boot == 1) {
    Serial.println("This was not first boot");
  } else if (successive_boot == 0) {
    Serial.println("First boot!");
  } else {
    Serial.println("Storage problem");
  }
  delayFunction();
}



/*
   Do a tare only if this is the first boot
*/
void tare() {
  //Initialize load cell
  LoadCell.powerUp();
  LoadCell.begin();
  //Check this is the first boot
  successive_boot = first_boot_storage.read();
  if (successive_boot == 1) {
    //Load old values from memory
    bin_height = height_tare_storage.read();
    offset = weight_tare_storage.read();
    LoadCell.startUsingOldTare(STABILISING_TIME, offset);
  } else if (successive_boot == 0) {
    //Tare and store values to memory
    Serial.println("Storing bin height");
    bin_height = tare_height();
    height_tare_storage.write(bin_height);
    Serial.println("Storing bin weight");
    LoadCell.start(STABILISING_TIME);
    offset = LoadCell.getTareOffset();
    weight_tare_storage.write(offset);

    first_boot_storage.write(1);
  } else {
    Serial.print("Unexpected value as successive_boot: ");
    Serial.println(successive_boot);
  }

  //Finish set-up loadcell calibration
  LoadCell.setCalFactor(SCALE_FACTOR);
  LoadCell.powerDown();
}



void action() {
  //Get new data
  float temp_h = getHeightAIO();
  Serial.print("Altezza misurata: ");
  Serial.println(temp_h);
  height = bin_height - temp_h;
  Serial.print("Height: ");
  Serial.print(height);
  Serial.print("cm su ");
  Serial.println(bin_height);
  weight = getWeightAIO();
  Serial.print("Weight: ");
  Serial.println(weight);

  //Compare data
  if (changed_weight() || changed_height() || elapsed_max_time()) {
    Serial.println("Threshold passed, send data!");
    //Update old_values only after a message
    old_height = height;
    old_weight = weight;
    passed_cycles = 0;
    //Send data to server
    sendData();
  } else {
    Serial.println("No send");
  }
  //increment the number ofX cycles passed
  passed_cycles += 1;
}

//Tells if the weight threshold is passed
bool changed_weight() {
  float difference = weight - old_weight;
  difference = abs(difference);

  return difference > THR_WEIGHT;
}

//Tells if the height threshold is passed
bool changed_height() {
  float difference = 100 * (height - old_height);
  difference = abs(difference);
  float height_thr = THR_HEIGHT * bin_height;
  return (difference > height_thr) && THR_MIN_HEIGHT * 100 < difference;
}

//Tells if the max number of iteration without a message is passed
bool elapsed_max_time() {
  return passed_cycles >= MAX_TIME_NO_SEND;
}

//Tells if the lid was open during the measure
bool closed_lid() {
  return height + THR_MIN_HEIGHT >= 0;
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  long start1 = millis();
  long start2 = millis();
  long elapsed = millis() - start1;
  while (WiFi.status() != WL_CONNECTED) {
    elapsed = millis() - start2;
    if (elapsed > 500) {
      start2 = millis();
      Serial.print(".");
    }
    if (start2 - start1 > 5000) {
      Serial.println("Retry connection");
      start1 = start2;
      WiFi.disconnect();
      WiFi.begin(wifi_ssid, wifi_password);
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void sendData() {
  bool published = false;
  char buf[256];
  String temp;
  Serial.print("Sending data...");
  String weight_str = "{ \"weight\":" + String(weight) + " }";
  String height_str =  "{ \"height\":" + String(height) + ", \"max_height\":" + String(bin_height) + " }";

  setup_wifi();
  Serial.println("Connesso");
  mqtt_client.setServer(mqtt_server, 1883);
  Serial.println("Server settato");
  if (!mqtt_client.connected()) {
    Serial.println("Reconnect...");
    reconnect();
  }
  mqtt_client.loop();
  Serial.println("Invio i dati");
  //Send the values
  Serial.print("New weight:");
  Serial.println(weight_str);
  temp = String(id)+weight_topic;
  temp.toCharArray(buf, 256);
  mqtt_client.publish(buf, weight_str.c_str(), true);
  Serial.print("New height:");
  Serial.println(height_str);
  if (closed_lid()) {
    temp = String(id)+height_topic;
    temp.toCharArray(buf, 256);
    published = mqtt_client.publish(buf, height_str.c_str(), true);
  }
  if (published) {
    Serial.println("Published");
  } else {
    Serial.println("Not published");
  }

  //Enter in low power mode
  mqtt_client.disconnect();
  espClient.stop();
  WiFi.disconnect();
  WiFi.end();
  return;
}

void reconnect() {
  // Loop until we're reconnected
  bool mqtt_connected = false;
  Serial.print("Check connection");
  while (!mqtt_client.connected()) {
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      setup_wifi();
    }

    Serial.print("Attempting MQTT connection...");
    if (mqtt_user == "") {
     // mqtt_connected = mqtt_client.connect("MKR1000Client");
      Serial.println("Simple connection - No user and psw" + String(mqtt_connected));
    } else {
      mqtt_connected = mqtt_client.connect("MKR1000Client", mqtt_user, mqtt_password);
    }
    if (mqtt_client.connect("MKR1000Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}

long getHeightAIO() {
  //Inizialize pin
  pinMode(D0, OUTPUT);
  //Echo
  pinMode(D1, INPUT);

  //Capture value
  long duration, distance;
  digitalWrite(D0, LOW);
  delayMicroseconds(2);

  digitalWrite(D0, HIGH);
  delayMicroseconds(10);

  digitalWrite(D0, LOW);
  duration = pulseIn(D1, HIGH);
  distance = (duration / 2) / 29.1;
  return distance;
}

long getWeightAIO() {
  LoadCell.powerUp();
  LoadCell.begin();
  long t = millis();
  int count = 0;

  while (1 == 1) {
    LoadCell.update();
    //get smoothed value from data set + current calibration factor
    if (millis() > t + STABILISING_TIME) {
      long i = (long) round(LoadCell.getData());
      count = count + 1;
      //Serial.print("W: ");
      //Serial.println(i);
      t = millis();

      if (count > 0) {
        count = 0;
        //Serial.println("Return");
        LoadCell.powerDown();
        return i;
      }
    }
  }
}

/*
 * Get 16 values of height
 * Return the mean of the values removing the bigger and the smaller
 */
long tare_height() {
  int samples = 16;
  float value = 0;
  long temp;
  long max;
  long min;
  for (int i = 0; i < samples; i++) {
    temp = getHeightAIO();
    //calcualte max and min value
    if (i == 0) {
      max = temp;
      min = temp;
    } else if (max < temp) {
      max = temp;
    } else if (min > temp) {
      min = temp;
    }
    //Calculate the sum
    value += temp;
    delay(10);
  }
  //calculate the mean
  value = (value - max - min) / (samples - 2);
  return long(round(value));
}

/*
 * Get the unique id of the board using the MAC address
 */
unsigned long getId() {
  setup_wifi();
  unsigned long id = 0;
//  printMac();
//  for (int i = 0; i < 6; i++) {
//    Serial.print(String(mac[i]));
  //  Serial.print(":");
//  }
  Serial.println("");
  WiFi.macAddress(mac);
  const int digits = 3;
  const int num_mac = 3;
  unsigned long temp = 0;
  for (int i = 0; i < num_mac; i++) {
    unsigned long value = long(mac[i]);
    temp = value * pow(10, i*digits);
    //Serial.println(temp);
    id += temp;
  }
  disconnect();
  return id;
}

void printMac() {
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
}

void alarmMatch(){
  //DO NOTHING - The execution restart from the loop where it was called
}

void delayFunction() {
  if (DEEP_SLEEP) {
    Serial.println("In Deep Sleep");
    rtc.setTime(0, 0, 00);
    rtc.setAlarmTime(SLEEPHH, SLEEPMIN, SLEEPSEC);
    rtc.enableAlarm(rtc.MATCH_HHMMSS); //set alarm
    rtc.attachInterrupt(alarmMatch); //creates an interrupt that wakes the SAMD21 which is triggered by a FTC alarm
    rtc.standbyMode(); //library call
  } else {
    Serial.println("In IDLE...");
    delay((SLEEPHH * 60 * 60 + SLEEPMIN * 60 + SLEEPSEC) * 1000);
  }
}

void disconnect() {
  WiFi.disconnect();
  WiFi.end();
}

