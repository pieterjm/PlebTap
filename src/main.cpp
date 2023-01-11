#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include "TFT_eSPI.h"
#include <ESP32Servo.h>
#include <OneButton.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <qrcode.h>

#define PIN_BUTTON_1                 0
#define PIN_BUTTON_2                 14
#define QRCODE_PIXEL_SIZE 3
#define QRCODE_X_OFFSET 10
#define QRCODE_Y_OFFSET 10


Servo servo;
TFT_eSPI tft = TFT_eSPI();
OneButton button1(PIN_BUTTON_1, true);
OneButton button2(PIN_BUTTON_2, true);
fs::SPIFFSFS &FlashFS = SPIFFS;
WebSocketsClient webSocket;
bool bOperational = false;

#define FORMAT_ON_FAIL true
#define PARAM_FILE "/elements.json"

#define SERVO_PIN 16

int servoAngle = 0;

String input = "";
String eat = "";

#define CMD_CONFIG_DONE "/config-done"
#define CMD_FILE_REMOVE "/file-remove"
#define CMD_FILE_DONE   "/file-done"
#define CMD_FILE_READ   "/file-read"
#define CMD_FILE_APPEND "/file-append"

String config_ssid;
String config_wifipassword;
String config_lnurl;
String config_wshost;
String config_wspath;
int config_servoclose;
int config_servoopen;
int config_tapduration;


bool removeBeforeAppend = false;
bool bBierConfirm = false;

// display qr code on the screen
void displayQrcode()
{
  // generate QR code
  uint8_t qrcodeData[qrcode_getBufferSize(20)];
  QRCode qrcode;  
  qrcode_initText(&qrcode, qrcodeData, 6, 0, config_lnurl.c_str());
  tft.fillScreen(TFT_WHITE);
  tft.drawRect(8,8,127,127,TFT_SKYBLUE);
  tft.drawRect(7,7,129,129,TFT_SKYBLUE);
  tft.drawRect(6,6,131,131,TFT_SKYBLUE);
  tft.drawRect(5,5,133,133,TFT_SKYBLUE);
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if ( qrcode_getModule(&qrcode, x, y) ) {
        tft.fillRect(QRCODE_X_OFFSET + x * QRCODE_PIXEL_SIZE,QRCODE_Y_OFFSET + y * QRCODE_PIXEL_SIZE,QRCODE_PIXEL_SIZE,QRCODE_PIXEL_SIZE,0);
      }
    }
  }
  tft.drawString("Scan the QR",150,10,2);
  tft.drawString("for a beer",150,40,2);
  
}

void displayMessage(String msg)
{
  tft.fillScreen(TFT_WHITE);
  tft.drawString(msg,10,10,2);
}

bool readConfig() {

  // start updating config
  tft.drawString("Reloading config",10,10,2);
   
  // open file 
  File file = SPIFFS.open(PARAM_FILE, "r");
  if (! file ) {
    tft.drawString("Could not open config file",10,10,2);
    return false;
  }

  // deserialize the document and return in case of an error
  StaticJsonDocument<2000> doc;
  String content = file.readString();
  Serial.println(content);
  DeserializationError error = deserializeJson(doc, content);
  file.close();

  switch (error.code()) {
    case DeserializationError::Ok:
        break;
    case DeserializationError::InvalidInput:
        displayMessage("Invalid input in JSON file");
        return false;
    case DeserializationError::NoMemory:
        displayMessage("Not enough JSON memory");
        return false;
    default:
        displayMessage("JSON Deserialisation error");
        return false;
  }

  // iterate over config file and parse arguments
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj: arr) {
    const char *name = obj["name"];
    const char *value = obj["value"];
    if ( strncmp(name,"ssid",4) == 0 ) {
      config_ssid = String(value);
    } else if ( strncmp(name,"wifipassword",12) == 0 ) {
      config_wifipassword = String(value);
    } else if ( strncmp(name,"websocket",9) == 0 ) {
      // convert to string object
      String wsstr = value;

      // check prefixes
      if ( wsstr.startsWith("ws://") ) {
        wsstr = wsstr.substring(5);
      } else if ( wsstr.startsWith("wss://") ) {
        wsstr = wsstr.substring(6);
      } else {
        displayMessage("Incorrect websocket URL");
        return false;
      }

      int index = wsstr.indexOf('/');
      if ( index == -1 ) {
        displayMessage("No host in websocket URL");
        return false;
      }
      config_wshost = wsstr.substring(0,index);
      config_wspath = wsstr.substring(index);
    } else if ( strncmp(name,"lnurl",5) == 0 ) {
      config_lnurl = String(value);
    } else if ( strncmp(name,"servoclose",10) == 0 ) {
      config_servoclose = String(value).toInt();
    } else if ( strncmp(name,"servoopen",9) == 0 ) {
      config_servoopen = String(value).toInt();
    } else if ( strncmp(name,"tapduration",11) == 0 ) {
      config_tapduration = String(value).toInt();
    }
  }

  displayMessage("Done reading config");

  servo.write(config_servoclose);

  displayMessage("Connecting to Wi-Fi");
  WiFi.begin(config_ssid.c_str(), config_wifipassword.c_str());
  int count = 0;
  while (WiFi.status() != WL_CONNECTED ) {
    delay(500);      
    if ( count++ > 20 ) {
      displayMessage("Failed Wi-Fi connection");
      return false;
    }
  }
  displayMessage("Wi-Fi connected");
  webSocket.beginSSL(config_wshost, 443, config_wspath);
  bOperational = true;

  return true;

}

// returns the string until the first space from input and truncates the input
void eatinput() {
  int index = input.indexOf(' ');
  if ( index == -1 ) {
    eat = input;
    input = "";
  } else {
    eat = input.substring(0,index);
    input = input.substring(index + 1);
  }  
}

// Parse commands from the serial input
void parseInput() {
  //Serial.println("parsing input: " + input);

  if ( input.length() == 0 ) {
    return;
  }

  bOperational = false;

  eatinput();

  if ( eat == CMD_CONFIG_DONE ) {
    Serial.println(CMD_CONFIG_DONE);
    displayMessage("Config loaded");
    readConfig();
    return;  
  } 

  if ( eat == CMD_FILE_REMOVE ) {
    eatinput();
    Serial.println("removeFile: " + eat);
    SPIFFS.remove(PARAM_FILE);
    removeBeforeAppend = true;
    displayMessage("Writing config");    
    return;
  }

  if ( eat == CMD_FILE_APPEND ) {
    eatinput();
    
    Serial.println("appendToFile: " + eat);

    // make sure that file is removed before appended
    if ( removeBeforeAppend == false ) {
      SPIFFS.remove(PARAM_FILE);
      removeBeforeAppend = true;
      displayMessage("Writing config");    
    }

    File file = SPIFFS.open(PARAM_FILE, FILE_APPEND);
    if (!file) {
      file = SPIFFS.open(PARAM_FILE, FILE_WRITE);
    }
    if (file) {
      file.println(input);
      file.close();
    }
    
  }


  if ( eat == CMD_FILE_READ ) {
    eatinput();
    Serial.println("readFile: " + eat);
    File file = SPIFFS.open(PARAM_FILE);
    if (!file) {
      Serial.println("Could not open file for reading");
      return;
    }
    
    while (file.available()) {
      String line = file.readStringUntil('\n');
      Serial.println("/file-read " + line);
    }
    file.close();
    Serial.println("");
    Serial.println("/file-done");
  }
}

// reads characters from the Serial interfaces and calls parser when a new line is detected
void handleSerial()
{
  while ( Serial.available() ) {
    char c = Serial.read();
    if ( c == '\n' ) {
      parseInput();      
      input = "";
    } else {
      input += c;
    }
  }
}

void bier()
{
  displayMessage("Paid! Press the button.");
  bBierConfirm = true;
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      displayMessage("WebSocket disconnected");
      break;
    case WStype_CONNECTED:
      displayMessage("WebSocket connected");
      displayQrcode();
			webSocket.sendTXT("Connected");
      break;
    case WStype_TEXT:
      displayMessage("Received WS text");
      bier();
      //payloadStr = (char*)payload;
      //paid = true;
      break;
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
			break;
  }
}

void setup() {
  // Initialise serial port for debugging output
  Serial.begin(115200);
  delay(2000);

  // initialise TFT
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  if(!SPIFFS.begin(FORMAT_ON_FAIL)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }

 
  servo.attach(SERVO_PIN);
 
  button1.attachClick([]() {
    if ( bBierConfirm == true ) {
      bBierConfirm = false;    
      displayMessage("Opening tap");
      servo.write(config_servoopen);
      displayMessage("Tap open");
      delay(config_tapduration);
      displayMessage("Closing tap");  
      servo.write(config_servoclose);
      displayQrcode();
    }
  });

  webSocket.onEvent(webSocketEvent);


  readConfig();     
}


void loop() {
  if ( bOperational ) {
    button1.tick();
    button2.tick();
    webSocket.loop();
  }

  handleSerial();  
}