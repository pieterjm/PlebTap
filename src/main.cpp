#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <WebSocketsClient.h>
#include "TFT_eSPI.h"
#include <ESP32Servo.h>
#include <qrcode.h>


#define WIFI_SSID "plebtab"
#define WS_HOST   "legend.lnbits.com"
#define WS_PATH   "/api/v1/ws/AfWPcwLraoDY4RDAzVcz3H"
#define PAYMENT_LNURL "LNURL1DP68GURN8GHJ7MR9VAJKUEPWD3HXY6T5WVHXXMMD9AKXUATJD3JX2ANFVDJJ7CTSDYHHVVF0D3H82UNV9AQKV46SVDM5CUNPDAZ9JDZJG3QH54NR0GE5S0M8WP5K70F3YEC8YMMXD96R6VPWXQCJVCTDDA6KUAPAX5CRQVQTCVSX9"
#define SERVO_PIN  2
#define SERVO_OFF  10
#define SERVO_ON   20
#define DELAY_BEFORE_BEER 5000
#define DELAY_DURING_BEER 5000
#define DELAY_AFTER_BEER  5000
#define SERVO_MIN_US  1000
#define SERVO_MAX_US  2000
#define BOOT_DELAY  5000 
#define QRCODE_PIXEL_SIZE 3
#define QRCODE_X_OFFSET 10
#define QRCODE_Y_OFFSET 10

WebServer webserver;
AutoConnect       acportal(webserver);
AutoConnectConfig acconfig;       // Enable autoReconnect supported on v0.9.4
WebSocketsClient webSocket;
Servo servo;
TFT_eSPI tft = TFT_eSPI();

// display the QR code on the screen   
void displayQrcode()
{
  // generate QR code
  uint8_t qrcodeData[qrcode_getBufferSize(20)];
  QRCode qrcode;  
  qrcode_initText(&qrcode, qrcodeData, 6, 0, PAYMENT_LNURL);
  tft.fillScreen(TFT_WHITE);
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if ( qrcode_getModule(&qrcode, x, y) ) {
        tft.fillRect(QRCODE_X_OFFSET + x * QRCODE_PIXEL_SIZE,QRCODE_Y_OFFSET + y * QRCODE_PIXEL_SIZE,QRCODE_PIXEL_SIZE,QRCODE_PIXEL_SIZE,0);
      }
    }
  }
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString(WiFi.localIP().toString(), 10, 140, 2);
}

// pour a beer
void beer()
{      
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_BLACK, TFT_BLUE);
  tft.drawString("Waiting to pour beer", 0, 0, 2);
  delay(DELAY_BEFORE_BEER);

  tft.fillScreen(TFT_GOLD);
  tft.setTextColor(TFT_BLACK, TFT_GOLD);
  tft.drawString("Pouring beer", 0, 0, 2);
  servo.write(SERVO_ON);
  delay(DELAY_DURING_BEER);
  servo.write(SERVO_OFF);
  
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_BLACK, TFT_BLUE);
  tft.drawString("Waiting after beer", 0, 0, 2);
  delay(DELAY_AFTER_BEER);

  displayQrcode();
}

// handle websocket events
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
    Serial.printf("[WSc] Disconnected!\n");
    break;
  case WStype_CONNECTED:
    Serial.printf("[WSc] Connected to url: %s\n",  payload);
    webSocket.sendTXT("Connected");
    break;
  case WStype_TEXT:
    Serial.printf("[WSc] Got text: %s\n",  payload);
    beer();
    break;
  case WStype_ERROR:			
	case WStype_FRAGMENT_TEXT_START:
	case WStype_FRAGMENT_BIN_START:
	case WStype_FRAGMENT:
	case WStype_FRAGMENT_FIN:
		break;
  }
}

// root page of webserver
void rootPage() {
  String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>"
    "<h2 align=\"center\" style=\"color:blue;margin:20px;\">Hello, world</h2>"
    "<p><form action=\"/testbeer\"><input type=\"submit\" value=\"Test\"></form></p>"
    "<p style=\"padding-top:15px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";

  webserver.send(200, "text/html", content);
}

// clicking on the test beer button pours a beer
void testbeerPage() {
  webserver.sendHeader("Location", String("http://") + webserver.client().localIP().toString() + String("/"));
  webserver.send(302, "text/plain", "");
  webserver.client().flush();
  webserver.client().stop();
  beer();
}

// start page
void startPage() {
  webserver.sendHeader("Location", String("http://") + webserver.client().localIP().toString() + String("/"));
  webserver.send(302, "text/plain", "");
  webserver.client().flush();
  webserver.client().stop();
}


void setup() {
  // Initialise serial port for debugging output
  Serial.begin(115200);
  

  // initialise TFT
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString("Initializing",10,0,2);
  
  // wait boot delay to see all data on serial port
  delay(BOOT_DELAY);





  // Initialize Wi-Fi configuration portal
  acconfig.autoReconnect = true;
  acconfig.apid = WIFI_SSID;
  acportal.config(acconfig);

  // Behavior a root path of the webserver
  webserver.on("/", rootPage);
  webserver.on("/start", startPage); 
  webserver.on("/testbeer", testbeerPage); 

  // Establish a connection with an autoReconnect option.
  if (acportal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    tft.drawString("Wi-Fi connected",10,0,2);
  } else {
    tft.drawString("Wi-Fi not connected",10,0,2);
  }

  // attach servo
  servo.attach(SERVO_PIN);

  // initialize websocket
  delay(5000);
  webSocket.beginSSL(WS_HOST, 443, WS_PATH);
  webSocket.onEvent(webSocketEvent);

  displayQrcode();

}

void loop() {
  acportal.handleClient();
  webSocket.loop();
}