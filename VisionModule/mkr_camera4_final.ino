/*
   Here I'll try to read the image and then send to the server

*/
#include <Wire.h>                    // I2C library, used to 
#include <SPI.h>                     // SPI library, used to control camera
#include <ArduCAM.h>                 // Camera library, creates a camera object and define image configuration
#include "memorysaver.h" // File with OV5642's reg value
#include <WiFi101.h>

//Wifi
#define UNI 1
//Is this a testing sketch?
#define TESTING 1
//High quality
#define HIGH_QUALITY 0
//Enable the use of IoT wifi
#define IOT_WIFI 1
//Seconds to wait before making a pic
#define SECOND_PAUSE 200
#define TIME_TO_PIC 3
#define RANGE_MIN 4
#define RANGE_MAX 25

// set pin 0 as the slave select for the digital pot:
#define CS_CAM 0
#define LED_UNSORTED 1
#define LED_PLASTIC 2
#define LED_PAPER 3
#define LED_DISTANCE_OK 4
#define LED_DISTANCE_NO 5
#define ECHO_PIN 6
#define TRIG_PIN 7
#if IOT_WIFI == 0
  //MQTT info
  #define host ""
  #define api_page "/image_api.php"

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
  #define host "192.168.0.105"
  #define api_page "/image_api.php"
  #define wifi_ssid "VodafoneMobileWiFi-ECE395"
 #define wifi_password "3890107988"
  // #define wifi_ssid "IoTPolimi"
  //#define wifi_password "ZpvYs=gT-p3DK3wb"
#endif




#if HIGH_QUALITY == 1
//It uses more space than 20kb - Partial image
#define IMAGE_QUALITY OV5642_640x480
#else
#define IMAGE_QUALITY OV5642_320x240
#endif

#define BUFFER 20000
#define SMALL_BUF 512

//The string before the selected bin
#define BIN_STRING "BIN: "

//Inizialize cam
ArduCAM myCam( OV5642, CS_CAM );
static uint8_t read_fifo_burst(ArduCAM myCAM, String CAM_NAME);

int secondOk = 0;
bool ledKo_ON = false;
bool ledOk_ON = false;

//Wifi Client
WiFiClient client;

void setup() {
  uint8_t vid, pid;
  uint8_t temp;

  Wire.begin();
  delay(3000);

  test_led();
  
  Serial.println(F("ACK CMD ArduCAM Start!"));
  //Set the pin as output
  pinMode(CS_CAM, OUTPUT);
  
  // initialize SPI:
  SPI.begin();
  delay(1000);

  //Check if the ArduCAM SPI bus is OK
  myCam.write_reg(ARDUCHIP_TEST1, 0x55);
  delay(1000);
  temp = myCam.read_reg(ARDUCHIP_TEST1);

  myCam.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
  myCam.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
  while ((vid != 0x56) || (pid != 0x42) || (temp != 0x55)) {
    myCam.write_reg(ARDUCHIP_TEST1, 0x55);
    delay(3000);
    temp = myCam.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55) {
      Serial.println("SPI1 interface Error!");
    } else {
      Serial.println("SPI1 OK");
    }

    // Check if Normal CAM is connected
    myCam.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCam.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);

    if ((vid != 0x56) || (pid != 0x42)) {
      Serial.println("Can't find OV5642 module!");
      Serial.println("vid=" + ToString(vid) + " - pid=" + ToString(pid));
    } else {
      delay(1000);
    }
  }
  // Serial.println("CAM1 detected.");
  // Configure JPEG format
  myCam.set_format(JPEG);
  myCam.InitCAM();

  //Setting the quality of image at minimum
  myCam.OV5642_set_JPEG_size(IMAGE_QUALITY);
  myCam.set_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
  myCam.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power
  myCam.clear_fifo_flag();
  myCam.write_reg(ARDUCHIP_FRAMES, 0x00);
  Serial.println("Setup OK!");
}

void loop() {
  //Check if it is time to make a pic
  if (secondOk >= TIME_TO_PIC) {
    secondOk = 0;
    ledOn(LED_DISTANCE_NO);
    ledOn(LED_DISTANCE_OK);
    capture_normalImage();
    ledOff(LED_DISTANCE_NO);
    ledOff(LED_DISTANCE_OK);
    ledKo_ON = false;
    ledOk_ON = false;
  }

  //check the status of the object
  if (inRange(RANGE_MIN, RANGE_MAX)) {
    //In range!
    if ( ! (!ledKo_ON && ledOk_ON) ) {
      ledOff(LED_DISTANCE_NO);
      ledKo_ON = false;
      
      ledOn(LED_DISTANCE_OK);
      ledOk_ON = true;
      Serial.println("Switch OK");
    }
    
    secondOk++;
  } else {
    //Not in range
    if ( ! (ledKo_ON && !ledOk_ON) ) {
      ledOff(LED_DISTANCE_OK);
      ledOk_ON = false;
      
      ledOn(LED_DISTANCE_NO);
      ledKo_ON = true;
      Serial.println("Switch KO");
    }
    secondOk = 0;
  }

  //Wait a second before next measeure 
  if (secondOk != 0) {
    delay(SECOND_PAUSE);
  }
  
}

void test_led() {
   ledOn(LED_DISTANCE_NO);
   delay(500);
   ledOn(LED_DISTANCE_OK);
   delay(500);
   ledOn(LED_UNSORTED);
   delay(500);
   ledOn(LED_PLASTIC);
   delay(500);
   ledOn(LED_PAPER);
   delay(500);
   
   ledOff(LED_DISTANCE_NO);
   delay(500);
   ledOff(LED_DISTANCE_OK);
   delay(500);
   ledOff(LED_UNSORTED);
   delay(500);
   ledOff(LED_PLASTIC);
   delay(500);
   ledOff(LED_PAPER);
   delay(500);
   
}

bool inRange(int min, int max) {
  //Min must be the smaller value
  if (min > max) {
    int temp = min;
    min = max;
    max = temp;
  }
  int dist = getDistance();
  Serial.println("Distance: "+String(dist));
  bool in_range = (dist <=max && dist >= min);
  return in_range;
}

void capture_normalImage() {
  uint64_t temp = 0;
  uint64_t temp_last = 0;
  uint64_t start_capture = 0;
  
  //Power up Camera
  myCam.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  delay(500);
  myCam.flush_fifo();
  myCam.clear_fifo_flag();
  myCam.start_capture();
  while (!myCam.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  myCam.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK); //enable low power

  Serial.println("Normal Capture Done!");
  read_fifo_burst(myCam, "NORM");

  //Clear the capture done flag
  myCam.clear_fifo_flag();
  //delay(500);
}

/*
   Leggo l'immagine dalla memoria
*/
uint8_t read_fifo_burst(ArduCAM myCAM, String CAM_NAME) {
  //Two variable to store last read byte
  uint64_t temp = 0;
  uint64_t temp_last = 0;
  //Counter used to measure the size of the image
  int image_len = 0;
  byte buf[BUFFER];
  byte small_buf[SMALL_BUF];
  int buf_index = 0;
  size_t length = myCAM.read_fifo_length();
  Serial.println("Expected image of: " + String(length) + "bytes");
  //Read length of image captured
  if (length >= 524288) {
    //512 kb -> Too big image
    Serial.println("Not found the end.");
    return 0;
  }

  //get the image
  Serial.print("Storing image...");
  myCAM.CS_LOW();
  delay(500);
  myCAM.set_fifo_burst();
  while ( (temp != 0xD9) | (temp_last != 0xFF)) {
    temp_last = temp;
    temp = SPI.transfer(0x00);

    //Print the read byte
    if (image_len == 0 || image_len == 1) {
      Serial.println("Byte #" + String(image_len) + ": " + ToString(temp));
    }

    //Write image data to buffer if not full
    if (image_len < BUFFER) {
      buf[image_len] = temp;
      image_len++;
    } else {
      Serial.println("ERROR: Buffer too small!");
      break;
    }
  }
  Serial.println("DONE!");

  //connect to wifi
  setup_wifi();

  // Prepare request
  String start_request = "";
  String end_request = "";
  start_request = start_request + "-----------------------------172051402316064980891449920408\r\nContent-Disposition: form-data; name=\"bin_id\"\r\n\r\n" + String(getId()) + "\r\n" +
                  "-----------------------------172051402316064980891449920408\r\nContent-Disposition: form-data; name=\"bin_image\"; filename=\"Schermata123.jpg\"\r\nContent-Type: image/jpeg\r\nContent-Transfer-Encoding: binary\r\n\r\n";
  end_request = end_request + "\r\n" + "-----------------------------172051402316064980891449920408--" + "\r\n";
  //calculate the extra length of the packet
  long extra_length;
  extra_length = start_request.length() + end_request.length();
  long len = image_len + extra_length;

  Serial.print("Starting connection to server...");

  client.connect(host, 80);
  Serial.println("OK");
  client.print("POST ");
  client.print(String(api_page));
  client.print(" HTTP/1.1\r\n");
  client.print("Host: " + String(host) + "\r\n");
  client.print("Content-Type: multipart/form-data; boundary=---------------------------172051402316064980891449920408\r\n");
  client.print("Content-Length: " + String(len) + "\r\n");
  client.print("\r\n");
  //Write the request body
  client.print(start_request);
  //Read the image in chunks, store into a small buff and send it
  for (int i = 0; i < image_len; i++, buf_index++) {
    //Buff full - Send
    if (buf_index == SMALL_BUF) {
      //restart buffer count
      buf_index = 0;
      //Send the whole buffer
      client.write(small_buf, SMALL_BUF);
      //Serial.println("Buffer sent!");
    }
    //Move value to buffer
    small_buf[buf_index] = buf[i];
  }
  //send remaining bytes
  if (buf_index > 0) {
    client.write(small_buf, buf_index);
  }

  client.print(end_request);
  Serial.println("Transmission over");


  String response = getResponse();
  if (response == "UNSORTED") {
      blinkLed(LED_UNSORTED);
  } else if (response == "PLASTIC" || response == "ALUMINIUM") {
      blinkLed(LED_PLASTIC);
  } else if (response == "PAPER") {
      blinkLed(LED_PAPER);
  } else {
      Serial.println("Cannot recognise the string");
  }
  Serial.println("Stringa letta: "+response);
  client.stop();
  //WiFi.disconnect();
  //WiFi.end();
}

String getResponse(){
  String response = "";
  bool startFound = false;
  bool endFound = false;
  char c;
  Serial.println("funzione");
  while (client.connected()) {
    while (client.available()) {
      // Read answer
      c = client.read();
      Serial.print(c);
      if ( !startFound && c == '%') {
        //Starting char found
        startFound = true;
      } else if (startFound && !endFound && c != '%') {
        //Found response
        response = response + c;
      } else if (startFound && !endFound && c == '%') {
        endFound = true;
        return response;
      } else if (startFound && endFound) {
        //Should never be here
        Serial.println("Should never be here");
      }
    }
  }
  return response;
}

bool string_found(char read_char, char* string) {
  bool found = false;
  int strlen = String(string).length();
  //Serial.println("String base lunga: "+String(strlen));
  //Add the last read char
  //Serial.print("Ho letto: ");
  for (int i = 0; i < strlen; i++) {
    int pos = strlen-i-1;
   // Serial.print(string[pos]);
    if (pos == 0) {
      string[pos] = read_char;
    } else {
      //Move forward last char
      string[pos] = string[pos-1];
    }
  }
 // Serial.println(" - FINE!");
  //Now compare the new string
  if (strcmp (string, BIN_STRING) == 0) {
    Serial.println("Stringa trovata!");
    found = true;
  } else if (read_char == '\n') {
     return true;
  }
  return found;
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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
  Serial.println("Connected");
}

String ToString(uint64_t x)
{
  boolean flag = false; // For preventing string return like this 0000123, with a lot of zeros in front.
  String str = "";      // Start with an empty string.
  uint64_t y = 10000000000000000000;
  int res;
  if (x == 0)  // if x = 0 and this is not testet, then function return a empty string.
  {
    str = "0";
    return str;  // or return "0";
  }
  while (y > 0)
  {
    res = (int)(x / y);
    if (res > 0)  // Wait for res > 0, then start adding to string.
      flag = true;
    if (flag == true)
      str = str + String(res);
    x = x - (y * (uint64_t)res);  // Subtract res times * y from x
    y = y / 10;                   // Reducer y with 10
  }
  return str;
}
byte ComBuffer[12];
int ClearComBuffer()
{ // clear com buffer for old data
  for (int i = 0; i < 12; i++)
  {
    ComBuffer[i] = 0;
  }
  return -1;
}

/*
   Get the unique id of the board using the MAC address
*/
long getId() {
  long id = 0;
  byte mac[6];
  WiFi.macAddress(mac);
  const int digits = 3;
  for (int i = 0; i < digits; i++) {
    int value = int(mac[i]);
    id += value * pow(10, i * digits);
  }
  return id;
}

void blinkLed(int led_num) {
   //Setup led
   ledOn(led_num);
   delay(5000);
   ledOff(led_num);
}

void ledOn(int led_num) {
   pinMode(led_num, OUTPUT);
   //Turn on
   digitalWrite(led_num, HIGH);
}

void ledOff(int led_num) {
   pinMode(led_num, OUTPUT);
   //Turn on
   digitalWrite(led_num, LOW);
}

long getDistance() {
  //Inizialize pin
  pinMode(TRIG_PIN, OUTPUT);
  //Echo
  pinMode(ECHO_PIN, INPUT);

  //Capture value
  long duration, distance;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) / 29.1;
  return distance;
}
