/*
 * Decabot Fun
 * Platform Wemos D1 Mini
 * Daniel Almeida Chagas
 * 
 * HR8833 I2C Motor Shield - https://github.com/wemos/LOLIN_I2C_MOTOR_Library
 * Matrix Led Shield - https://github.com/wemos/WEMOS_Matrix_LED_Shield_Arduino_Library
 * Battery Shield - https://www.wemos.cc/en/latest/d1_mini_shield/battery.html
 * Decabot Mini Shield
 * 
 * PORTS                             _____________
 *                                  /             \
 *                             RST |*  ---------  *| TX - GPIO1 - LED Red
 *           battery ---------- A0 |* |         | *| RX - GOIO3 - LED Green
 *                     GPIO16 - D0 |* |  ESP    | *| D1 --------- SCL
 *    led matrix CLK ---------- D5 |* |  8266   | *| D2 --------- SDA
 *          LED Blue - GPIO12 - D6 |* |         | *| D3 - GPIO0 - SERVO1
 *    led matrix DIN ---------- D7 |* |         | *| D4 - GPIO2 - SERVO2
 *            buzzer - GPIO15 - D8 |*  ---------  *| GND
 *                             3V3 |*             *| 5V
 *                                  \              |
 *                            reset  |      D1mini |
 *                                    \___________/
 */

// Libs
#include <Adafruit_GFX.h>
#include <WEMOS_Matrix_GFX.h>
#include <Servo.h>
#include <Wire.h>
#include <LOLIN_I2C_MOTOR.h>
#include <ArduinoUniqueID.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//setup
const bool rgbLED = true;
const bool motors = true;
const bool servo = true;
const bool rfid = false;
const bool oled = false;
const bool matrixLED = true;
const bool wifi = false;
// Replace with your network credentials
const char* ssid = "lapin";
const char* password = "dpdil@p1N";

// Objects
MLED matrix(7); //set intensity=7 (maximum)
LOLIN_I2C_MOTOR motor; //I2C address 0x30
Servo servo1;
Servo servo2;
AsyncWebServer server(80);

//Variables
const char compile_date[] = __DATE__ " " __TIME__;
const int rgbRedPin = 1;
const int rgbGreenPin = 3;
const int rgbBluePin = 12;
const int buzzerPin = 15;
const int servo1Pin = 0;
const int servo2Pin = 2;
bool batteryAllarm = false;
String sliderValue = "0";
const char* PARAM_INPUT = "value";

//sounds
const int decabotMusic[4][3]{{494,2,3},{554,2,4},{440,2,5},{880,1,6}};

//bitmaps
static const uint8_t PROGMEM eyes_bmp[4][8] = {
  {B10000001,B10000001,B10000001,B10000001,B10000001,B10000001,B10000001,B10000001},  //open eyes
  {B10000001,B10000001,B10000001,B10000001,B10000001,B10000001,B11000011,B00000000},  //closing eyes 1
  {B10000001,B10000001,B11000011,B00000000,B00000000,B00000000,B00000000,B00000000},  //closing eyes 2
  {B00000000,B11100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000}   //losed eyes
};

//webpages
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Decabot Web Server</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
  </style>
</head>
<body>
  <h2>Decabot Web Server</h2>
  <p><span id="textSliderValue">%SLIDERVALUE%</span></p>
  <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="1023" value="%SLIDERVALUE%" step="1" class="slider"></p>
<script>
function updateSliderPWM(element) {
  var sliderValue = document.getElementById("pwmSlider").value;
  document.getElementById("textSliderValue").innerHTML = sliderValue;
  console.log(sliderValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  delay(1000);
  decabotMessage("Copyright (C) Decano Robotics - www.decabot.com");
  decabotMessage("Decabot Version: 0.7");
  decabotMessage(compile_date);
  decabotMessage("Robot ID " + decabotUniqueID());
  
  //RGB LED Setup
  if(rgbLED){
    decabotMessage("Setup RGB LEDS...");
    //pinMode(rgbRedPin, OUTPUT);
    //pinMode(rgbGreenPin, OUTPUT);
    pinMode(rgbBluePin, OUTPUT);
    //digitalWrite(rgbRedPin, HIGH);
    //digitalWrite(rgbGreenPin, LOW);
    digitalWrite(rgbBluePin, LOW);
    decabotMessage("OK!");
  } else {
    decabotMessage("No RGB LEDS installed!");
  }

  decabotFace(3);
  
  //Servo Setup
  if(servo){
    decabotMessage("Setup SERVOS...");
    servo1.attach(servo1Pin);
    servo2.attach(servo2Pin);
    decabotMessage("OK!");
  } else {
    decabotMessage("No servo installed!");
  }

  //Motor setup
  if(motors){
    decabotMessage("Setup HR8822 I2C motors...");
    while (motor.PRODUCT_ID != PRODUCT_ID_I2C_MOTOR) //wait motor shield ready.
    {
      decabotMessage("OK!");
      decabotMessage((String) motor.getInfo());
      motor.changeFreq(MOTOR_CH_BOTH, 1000);
    }
  } else {
    decabotMessage("No motor driver installed!");
  }

  if(rfid){
    //nothing here
  } else {
    decabotMessage("No RFID installed!");
  }

  if(oled){
    //nothing here
  } else {
    decabotMessage("No OLED screen installed!");
  }

  if(wifi){
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    decabotMessage("Connecting to WiFi " + (String)ssid);
    int tempCon = 0;
    while (WiFi.status() != WL_CONNECTED) {
      tempCon++;
      delay(1000);
      Serial.print(".");
      if(tempCon > 40) {
        Serial.println("");
        decabotMessage("Are you sure WiFi " + (String)ssid + " is OK?!");
        tempCon = 0;
      }
    }
    Serial.println("");
    Serial.println(WiFi.localIP());
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, decabotWebProcessor);
    });
  
    // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
    server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String inputMessage;
      // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
      if (request->hasParam(PARAM_INPUT)) {
        inputMessage = request->getParam(PARAM_INPUT)->value();
        sliderValue = inputMessage;
        //analogWrite(output, sliderValue.toInt());
        //myservo.write(map(sliderValue.toInt(),0,1023,0,180)); 
        decabotMoveForward(map(sliderValue.toInt(),0,1023,0,100));
      }
      else {
        inputMessage = "No message sent";
      }
      Serial.println(inputMessage);
      request->send(200, "text/plain", "OK");
    });
    
    // Start server
    server.begin();
  } else {
    decabotMessage("No WI-FI connection setup!");
  }
  
  decabotThemeMusic();
  decabotFace(0);
  decabotColor(5,1);
  decabotMessage("Decabot Ready!");
}

void loop() {
  
  /*
  decabotMoveForward(100);
  delay(3000);
  decabotMoveRotate(1,100);
  delay(1500);
  decabotMoveLeft(100);
  delay(500);
  decabotMoveStop();
  delay(2000);
  decabotMoveBackward(50);
  delay(3000);
  */
}

//Functions Decabot

void decabotThemeMusic(){
  decabotMessage("Playing Decabot Music Theme...");
  for(int i=0;i<4;i++){
    tone(buzzerPin, decabotMusic[i][0], decabotMusic[i][1]*200);
    decabotColor(decabotMusic[i][2], 0);
    delay(decabotMusic[i][1]*200);
    noTone(buzzerPin);
  }
  decabotMessage("OK!");
}

void decabotMessage(String text){
  if(text == "OK!"){
    Serial.println(text);
  } else {
    Serial.print(millis());
    Serial.print(" - "); 
    if(text.endsWith("...")){
      Serial.print(text);
    } else {
      Serial.println(text);
    } 
  }
}

void decabotColor(int color, bool mute){
  if(mute) decabotMessage("Set color to " + (String) color);
  switch(color){
    case 1:
      digitalWrite(rgbBluePin, HIGH);
      digitalWrite(rgbRedPin, LOW);
      digitalWrite(rgbGreenPin, LOW);
      break;
    case 2:
      digitalWrite(rgbBluePin, HIGH);
      digitalWrite(rgbRedPin, HIGH);
      digitalWrite(rgbGreenPin, LOW);
      break;
    case 3:
      digitalWrite(rgbBluePin, LOW);
      digitalWrite(rgbRedPin, HIGH);
      digitalWrite(rgbGreenPin, LOW);
      break;
    case 4:
      digitalWrite(rgbBluePin, LOW);
      digitalWrite(rgbRedPin, HIGH);
      digitalWrite(rgbGreenPin, HIGH);
      break;
    case 5:
      digitalWrite(rgbBluePin, LOW);
      digitalWrite(rgbRedPin, LOW);
      digitalWrite(rgbGreenPin, HIGH);
      break;
    case 6:
      digitalWrite(rgbBluePin, HIGH);
      digitalWrite(rgbRedPin, LOW);
      digitalWrite(rgbGreenPin, HIGH);
      break;
    case 7:
      digitalWrite(rgbBluePin, HIGH);
      digitalWrite(rgbRedPin, HIGH);
      digitalWrite(rgbGreenPin, HIGH);
      break;
    default:
      digitalWrite(rgbBluePin, LOW);
      digitalWrite(rgbRedPin, LOW);
      digitalWrite(rgbGreenPin, LOW);
      break;
  }
}

void decabotFace(int draw){
  if(matrixLED){
    decabotMessage("Change face to " + (String) draw);
    matrix.clear();
    matrix.drawBitmap(0, 0, eyes_bmp[draw], 8, 8, LED_ON);
    matrix.writeDisplay();  
  } else {
    decabotMessage("No LED Matrix installed!");
  }
}

int readBatteryLevel(){
  int reading = map(analogRead(A0),704,955,0,100);
  if (reading<0) reading = 0;
  if (reading>100) reading = 100;
  if (reading<10){
    matrix.drawLine(1,0,6,7, LED_ON);
    matrix.writeDisplay();
    if(batteryAllarm){
      batteryAllarm = false;
      noTone(buzzerPin);
      decabotColor(0,1);
    } else {
      batteryAllarm = true;
      tone(buzzerPin, 494, 200);
      decabotColor(3,1);
    }
  }
  return reading;
}

void decabotShowBatLevel(){
  matrix.clear();
  matrix.drawRect(0,2, 7,4, LED_ON);
  matrix.drawLine(7,3,7,4, LED_ON);
  matrix.fillRect(1,3, map(readBatteryLevel(),0,100,0,5),2, LED_ON);
  matrix.writeDisplay();
  //decabotMessage(readBatteryLevel().toString());
}

void decabotMotorLeft(int power, bool fwd){
  if(fwd){
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CW);
  } else {
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CCW);
  }
  if(power<=15){
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_SHORT_BRAKE);
  } else {
    motor.changeDuty(MOTOR_CH_A, power);
  }
}

void decabotMotorRight(int power, bool fwd){
  if(fwd){
    motor.changeStatus(MOTOR_CH_B, MOTOR_STATUS_CW);
  } else {
    motor.changeStatus(MOTOR_CH_B, MOTOR_STATUS_CCW);
  }
  if(power<=15){
    motor.changeStatus(MOTOR_CH_B, MOTOR_STATUS_SHORT_BRAKE);
  } else {
    motor.changeDuty(MOTOR_CH_B, power);
  }
}

void decabotMoveRotate(bool cw, int pot){
  if(cw){
    //clock-wise
    decabotMotorLeft(pot,1);
    decabotMotorRight(pot,0);
  } else {
    //couter clock-wise
    decabotMotorLeft(pot,0);
    decabotMotorRight(pot,1);
  }
}

void decabotMoveStop(){
  decabotMotorRight(0, 1);
  decabotMotorLeft(0, 1);
}

void decabotMoveForward(int pot){
  decabotMotorRight(pot, 1);
  decabotMotorLeft(pot, 1);
}

void decabotMoveBackward(int pot){
  decabotMotorRight(pot, 0);
  decabotMotorLeft(pot, 0);
}

void decabotMoveLeft(int pot){
  decabotMotorRight(pot, 0);
  decabotMotorLeft(pot, 1);
}

void decabotMoveRight(int pot){
  decabotMotorRight(pot, 1);
  decabotMotorLeft(pot, 0);
}

String decabotUniqueID(){
  String id = "";
  for (size_t i = 0; i < UniqueIDsize; i++){
    if (UniqueID[i] < 0x10) id = id + "0";
    id = id + UniqueID[i];
  }
  return id;
}

String decabotWebProcessor(const String& var){
  if (var == "SLIDERVALUE"){
    return sliderValue;
  }
  return String();
}
