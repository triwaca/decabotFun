/*
 * Decabot MVP
 * Platform Wemos D1 Mini
 * Daniel Almeida Chagas
 * 
 * HR8833 I2C Motor Shield - https://github.com/wemos/LOLIN_I2C_MOTOR_Library
 * Matrix Led Shield - https://github.com/wemos/WEMOS_Matrix_LED_Shield_Arduino_Library
 * Battery Shield - https://www.wemos.cc/en/latest/d1_mini_shield/battery.html
 * Decaboard Model T
 * 
 * PORTS                             _____________
 *                                  /             \
 *                             RST |*  ---------  *| TX - GPIO1 - LED Red
 *           battery ---------- A0 |* |         | *| RX - GOIO3 - LED Green
 *            SERVO2 - GPIO16 - D0 |* |  ESP    | *| D1 --------- SCL
 *    led matrix CLK ---------- D5 |* |  8266   | *| D2 --------- SDA
 *          LED Blue - GPIO12 - D6 |* |         | *| D3 - GPIO0 - SERVO1
 *    led matrix DIN ---------- D7 |* |         | *| D4 - GPIO2 - LED WiFi
 *            buzzer - GPIO15 - D8 |*  ---------  *| GND
 *                             3V3 |*             *| 5V
 *                                  \              |
 *                            reset  |      D1mini |
 *                                    \___________/
 * SERIAL
 * Decabot will always boot with serial working on 115200bps, so it's possible to see important
 * compiled data, such as ID, IP, compilation date and time, and working modules. To continue
 * using serial as debug, must turn off RGBleds (bool rgbLED = false).
 * 
 * 
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
#include <FS.h>
#include <MPU6050_tockn.h>
//#include "SPIFFS.h"
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRsend.h>

//setup
const bool rgbLED = true;
const bool motors = true;
const bool servo = true;
const bool rfid = false;
const bool oled = false;
const bool matrixLED = true;
const bool wifi = true;
const bool gyro = false;
const bool infrared = true;
const bool gun = true;
const bool invertLeftRight = true;
const bool invertLeftMotor = false;
const bool invertRightMotor = true;

//Constantes
const char compile_date[] = __DATE__ " " __TIME__;
#define rgbRedPin 1
#define rgbGreenPin 3
#define rgbBluePin 2
//#define wifiLEDPin 12
#define irPin 12
#define buzzerPin 15
#define servo1Pin 0
#define servo2Pin 16
#define gunPin 16
bool batteryAllarm = false;

// Objects
MLED matrix(1); //set intensity=7 (maximum)
//const char* ssid = "lapin";
//const char* password = "dpdil@p1N";
//const char* ssid = "brisa-2425163";
//const char* password = "6mjkisuz";
const char* ssid = "NET_2G15E89F";
const char* password = "E215E89F";
LOLIN_I2C_MOTOR motor; //I2C address 0x30
Servo servo1;
Servo servo2;
AsyncWebServer server(80);
MPU6050 mpu6050(Wire); //I2C address 0x68
IRrecv irrecv(irPin);
decode_results results;
IRsend irsend(gunPin); 

//Variables
String recebido = "0";
const char* PARAM_INPUT = "value";
//control calibration vars
int centralXlimits = 15;
int centralYlimits = 15;
int maximumXY = 155;

//position Variables
int decabotAngle = 0;
int tempTimer = 0;

//sounds
const int decabotMusic[4][3]{{494,2,3},{554,2,4},{440,2,5},{880,1,6}};

//bitmaps
static const uint8_t PROGMEM eyes_bmp[6][8] = {
  {B00000000,B00000000,B10100101,B01000010,B10100101,B00000000,B00000000,B00000000},  //dead
  {B00000000,B00000000,B00001000,B00011000,B00001000,B00001000,B00011100,B00000000},  //1
  {B00000000,B00000000,B00111000,B00000100,B00011000,B00100000,B00111100,B00000000},  //2
  {B00000000,B00000000,B00111000,B00000100,B00011000,B00000100,B00111000,B00000000},  //3
  {B00000000,B00000000,B00010100,B00100100,B00111100,B00000100,B00000100,B00000000},  //4
  {B00000000,B00000000,B00111100,B00100000,B00111000,B00000100,B00111000,B00000000}  //5
};


//game
int healTime = 2000;
int healTimer = 0;
bool shooting = false;
int decabotTeamColor = 6;
int life = 5;
char strIr[32];
String dadoIr = "";

//emotion variables
int eyeTypeVar = 3; //select between 0 to 4
int eyePosLR = 0; //-1, 0 or 1
int eyePosUD = 0; //-1, 0 or 1
int eyeBrowPosUD = 0; //0 to 5
int eyeBrowEmotion = 0; //-1, 0 or 1
int matrixUpdate = 0;

int temp = 0;
int positionLoadLine = 0;

void setup(){
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  delay(1000);
  decabotMessage("Copyright (C) Decano Robotics - www.decabot.com");
  decabotMessage("Decabot Version: 0.9.3.015");
  decabotMessage(compile_date);
  decabotMessage("Robot ID " + decabotUniqueID());

  if(wifi){
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    decabotMessage("Connecting to WiFi " + (String)ssid);
    int tempCon = 0;
    while (WiFi.status() != WL_CONNECTED){
      tempCon++;
      if(tempCon%2==0){
        //digitalWrite(wifiLEDPin, LOW);
      } else {
        //digitalWrite(wifiLEDPin, HIGH);
      }
      delay(100);
      Serial.print(".");
      decabotLoadingLine(3);
      decabotDoneLoadingLine(0);
      if(tempCon > 160){
        Serial.println("");
        decabotMessage("Are you sure WiFi " + (String)ssid + " is OK?!");
        tempCon = 0;
      }
    }
    decabotMessage("OK!");
    Serial.println(WiFi.localIP());
    //digitalWrite(wifiLEDPin, HIGH);
    
    // Camimho da pagina raiz / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", String(), false, decabotWebProcessor);
    });
    
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/status.html", String(), false, decabotWebProcessor);
    });

    //Caminhos para as imagens:
    server.on("/joy_red", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/joy_red.png", "image/png");
    });
    
    server.on("/joy_blue", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/joy_blue.png", "image/png");
    });
    
    server.on("/joy_base", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/joy_base.png", "image/png");
    });
    
    server.on("/laser_red", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/laser_red.png", "image/png");
    });
    
    
    // Send a GET request to <ESP_IP>/joystick?value=<inputMessage>
    
    server.on("/joystick", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String inputMessage;
      
      // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
      if (request->hasParam(PARAM_INPUT)){
        
        inputMessage = request->getParam(PARAM_INPUT)->value();
        recebido = inputMessage;
        Serial.println("mensagem: " + String(recebido));
        
        maquina_de_estados(recebido);

        //Serial.println("posY: " + String(pY) + "posX: " + String(pX));
        //Serial.println("Index do | : " + String(index_split));
        //analogWrite(output, recebido.toInt());
        //myservo.write(map(recebido.toInt(),0,1023,0,180)); 
 
      } else {
        inputMessage = "No message sent";
      }
      request->send(200, "text/plain", "OK");
    });
    
    // Start server
    server.begin();
    decabotDoneLoadingLine(3);
  } else {
    decabotMessage("No WI-FI connection setup!");
    //digitalWrite(wifiLEDPin, HIGH);
  }

  if(infrared){
    decabotMessage("Setup INFRARED...");
    irrecv.enableIRIn(); 
    irsend.begin();
    decabotMessage("OK!");
  } else {
    decabotMessage("No INFRARED installed!");
  }

  if(gun){
    decabotMessage("Setup LASER GUN...");
    pinMode(gunPin, OUTPUT);
    digitalWrite(gunPin, LOW);
    decabotMessage("OK!");
  } else {
    decabotMessage("No LASER GUN installed!");
  }

  if(gyro){
    decabotMessage("Setup GYROSCOPE...");
    Wire.begin();
    mpu6050.begin();
    mpu6050.calcGyroOffsets(true);
    decabotMessage("OK!");
  } else {
    decabotMessage("No GYROSCOPE installed!");
  }
  
  decabotMessage("Initializing SPIFFS...");
  if(SPIFFS.begin()){
    decabotMessage("OK!");
  } else {
    decabotMessage("Erro! falha de SPIFFS");
  }

  decabotExpressions();
  
  //Servo Setup
  if(servo){
    decabotMessage("Setup SERVOS...");
    servo1.attach(servo1Pin);
    servo2.attach(servo2Pin);
    servo1.write(45); //posição inicial traseira
    servo2.write(45); //posição inicial frontal
    decabotMessage("OK!");
  } else {
    decabotMessage("No servo installed!");
  }

  //Motor setup
  if(motors){
    decabotMessage("Setup HR8822 I2C motors...");
    int tempMotor = 0;
    while (motor.PRODUCT_ID != PRODUCT_ID_I2C_MOTOR) {
      motor.getInfo();
      tempMotor++;
      if(tempMotor%2==0){
        decabotColor(7,1);
      }else {
        decabotColor(0,1);
      }
      Serial.print(".");
      decabotLoadingLine(0);
      delay(100);
      if(tempMotor > 160){
        Serial.println("");
        decabotMessage("Are you sure motor shield is OK?!");
        tempMotor = 0;
      }
    }
    decabotDoneLoadingLine(0);
    motor.changeFreq(MOTOR_CH_BOTH, 1000);
    decabotMessage("OK!");
  } else {
    decabotMessage("No MOTOR driver installed!");
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
  
  decabotThemeMusic();
  decabotExpressions();
  decabotMessage("Decabot Ready!");
  
  //RGB LED Setup
  if(rgbLED){
    decabotMessage("Setup RGB LEDS...");
    Serial.end();
    pinMode(rgbRedPin, OUTPUT);
    pinMode(rgbGreenPin, OUTPUT);
    pinMode(rgbBluePin, OUTPUT);
    digitalWrite(rgbRedPin, HIGH);
    digitalWrite(rgbGreenPin, LOW);
    digitalWrite(rgbBluePin, LOW);
    decabotMessage("OK!");
    decabotColor(decabotTeamColor,1);
  } else {
    decabotMessage("No RGB LEDS installed!");
  }
  
  digitalWrite(gunPin, LOW);
}

void loop(){
  //receive shots

  /*
  if (irrecv.decode(&results)) {
    numberToHexStr(strIr, (unsigned char*) &results.value, sizeof(results.value));
    dadoIr =  strIr;
    String adversario = dadoIr.substring(9,13);
    if(!shooting) {
      if(dadoIr.substring(9,10)=="F"){
        decabotHurt();
      }
    }
    irrecv.resume();  // Receive the next value
  }
  */
  
  shooting = false;
  decabotExpressions();
}

//Functions Decabot

void decabotExpressions(){
  //exibits faces and colors
  if(millis()>matrixUpdate){
    decabotColor(decabotTeamColor,0);
    decabotEye(eyeTypeVar,eyePosLR,eyePosUD);
    if(eyeBrowPosUD!=0) decabotEyeBrow(eyeBrowEmotion,eyeBrowPosUD);
    matrix.writeDisplay();
    matrixUpdate = millis() + 250;
  }
  
  if(millis()%60000==0){
    decabotShowBatLevel();
    matrixUpdate = millis() + 1000;
  }
  
}

void maquina_de_estados(String msg){
  char chave = msg[0];
  String posX, posY;
  int index_split, pX, pY;
  switch(chave){
    case 'M':
      index_split = recebido.indexOf('|');
      posX = recebido.substring(1, index_split);
      posY = recebido.substring(index_split+1);
      pX = posX.toInt();
      pY = posY.toInt();
      
      decabotMove(map(pX,0,64,0,100), map(pY,0,64,0,100));
      break;
      
    case 'B':
      decabotGunShot(2);
      break;

    case 'S':
      //index_split = recebido.indexOf('|');
      posX = recebido.substring(1);
      //posY = recebido.substring(index_split+1);
      pX = posX.toInt();
      //pY = posY.toInt();
      decabotServo2move(pX);
      //decabotServo2move(pY);
      break;

    case 'T':
      //index_split = recebido.indexOf('|');
      posX = recebido.substring(1);
      //posY = recebido.substring(index_split+1);
      pX = posX.toInt();
      //pY = posY.toInt();
      decabotServo1move(pY);
      //decabotServo2move(pY);
      break;
  }
}

void decabotDead(){
  decabotColor(1,0);
  decabotBitmap(0);
  delay(2000);
}

void decabotHurt(){
  life--;
  if(life<=0){
    decabotDead();
    decabotSpaw();
  } else {
    decabotColor(1,0);
    eyePosLR = 0;
    eyePosUD = 1;
    eyeBrowPosUD = 1;
    eyeBrowEmotion = -1;
    decabotExpressions();
    delay(500);
    decabotBitmap(life+1);
    delay(200);
    decabotBitmap(life);
    matrixUpdate = millis() + 1000;
  }
}

void decabotSpaw(){
  decabotThemeMusic();
  eyePosLR = 0;
  eyeBrowPosUD = 0;
  decabotExpressions();
  life = 5;
}

void decabotServo1move(int porc){
  servo1.write(map(porc,0,100,180,45));
}

void decabotServo2move(int porc){
  servo2.write(map(porc,0,100,0,180));
}

void decabotGunShot(int qual){
  eyeBrowPosUD = 2;
  eyeBrowEmotion = 1;
  decabotExpressions();
  bool stp = false;
  switch(qual){
    case 1:
      
      //delay(3000);
      decabotMoveForward(50);
      
      for(int i=800;i>500;i--)
      { //tirinho 800 --> 500
        //if(i == 780) decabotMoveForward(50);
        if(i < 745 && stp == false)
        {
          decabotMoveStop();
          stp = true;
        }
        digitalWrite(gunPin, HIGH);
        tone(buzzerPin,i);
        //delay(1);
        delayMicroseconds(400);
        digitalWrite(gunPin, LOW);
        
      }
      decabotColor(5,1);
      stp = false;
      noTone(buzzerPin);
      break;
    case 2:
      shooting = true;
      //decabotShotCode(decabotTeamColor);
      digitalWrite(gunPin, HIGH);
      for(int i=1000;i>500;i=i-5){
        tone(buzzerPin,i);
        delay(1);
      }
      noTone(buzzerPin);
      digitalWrite(gunPin, LOW);
      break;
  }
}

void decabotThemeMusic(){
  decabotMessage("Playing Decabot Music Theme...");
  eyePosLR = 0;
  eyePosUD = 0;
  eyeBrowEmotion = 0;
  for(int i=0;i<4;i++){
    eyeBrowPosUD = i+1;
    decabotExpressions();
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
  if(rgbLED){
    if(!mute) decabotMessage("Set color to " + (String) color);
    switch(color){
      case 0:
        //no color
        digitalWrite(rgbRedPin, LOW);
        digitalWrite(rgbGreenPin, LOW);
        digitalWrite(rgbBluePin, LOW);
        break;
      case 1:
        //red
        digitalWrite(rgbRedPin, HIGH);
        digitalWrite(rgbGreenPin, LOW);
        digitalWrite(rgbBluePin, LOW);
        break;
      case 2:
        //green
        digitalWrite(rgbRedPin, LOW);
        digitalWrite(rgbGreenPin, HIGH);
        digitalWrite(rgbBluePin, LOW);
        break;
      case 3:
        //yellow
        digitalWrite(rgbRedPin, HIGH);
        digitalWrite(rgbGreenPin, HIGH);
        digitalWrite(rgbBluePin, LOW);
        break;
      case 4:
        //blue
        digitalWrite(rgbRedPin, LOW);
        digitalWrite(rgbGreenPin, LOW);
        digitalWrite(rgbBluePin, HIGH);
        break;
      case 5:
        //purple
        digitalWrite(rgbRedPin, HIGH);
        digitalWrite(rgbGreenPin, LOW);
        digitalWrite(rgbBluePin, HIGH);
        break;
      case 6:
        //cyan
        digitalWrite(rgbRedPin, LOW);
        digitalWrite(rgbGreenPin, HIGH);
        digitalWrite(rgbBluePin, HIGH);
        break;
      case 7:
        //white
        digitalWrite(rgbRedPin, HIGH);
        digitalWrite(rgbGreenPin, HIGH);
        digitalWrite(rgbBluePin, HIGH);
        break;
      default:
        //no color
        digitalWrite(rgbRedPin, LOW);
        digitalWrite(rgbGreenPin, LOW);
        digitalWrite(rgbBluePin, LOW);
        break;
    } 
  }
}

void decabotShotCode(int color){
  switch(color){
    case 0:
      //no color
      irsend.sendNEC(0x00000FF0UL);
      break;
    case 1:
      //red
      irsend.sendNEC(0x0FF0000FUL);
      break;
    case 2:
      //green
      irsend.sendNEC(0x0F0F000FUL);
      break;
    case 3:
      //yellow);
      irsend.sendNEC(0x0FFF000FUL);
      break;
    case 4:
      //blue
      irsend.sendNEC(0x0F00F00FUL);
      break;
    case 5:
      //purple
      irsend.sendNEC(0x0FF0F00FUL);
      break;
    case 6:
      //cyan
      irsend.sendNEC(0x0F0FF00FUL);
      break;
    case 7:
      //white
      irsend.sendNEC(0x0FFFF00FUL);
      break;
    default:
      //no color
      irsend.sendNEC(0x00000FF0UL);
      break;
  }
}

void decabotBitmap(int draw){
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

void decabotMotorRight(int power, bool fwd){
  if(power>100) power = 100;
  if(invertRightMotor) fwd = !fwd;
  
  if(fwd){
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CCW);
  } else {
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CW);
  }
  
  if(power<=15) {
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_SHORT_BRAKE);
  } else {
    motor.changeDuty(MOTOR_CH_A, power);
  }
}

void decabotMotorLeft(int power, bool fwd){
  if(power>100) power = 100;
  if(invertLeftMotor) fwd = !fwd;
  
  if(fwd){
    motor.changeStatus(MOTOR_CH_B, MOTOR_STATUS_CCW);
  } else {
    motor.changeStatus(MOTOR_CH_B, MOTOR_STATUS_CW);
  }
  
  if(power<=15) {
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
  decabotMessage("[move:stop]");
}

void decabotMoveForward(int pot){
  decabotMotorRight(pot, 1);
  decabotMotorLeft(pot, 1);
}

void decabotMove(int x, int y){
  //x indica movimentos para esquerda(-) e direita(+)
  //y indica movimetnos para frente(-) e para trás(+)
  //valores entre -99 e 99

  //direção olhos
  if(x>-20&&x<20){
    eyePosLR = 0;
  } else {
    if(x>=20) eyePosLR = -1;
    if(x<=-20) eyePosLR = 1;
  }
  if(y<0){
    eyeBrowPosUD = map(y,-30,-100,3,1);
    if(y<-50){
      eyeBrowEmotion = 1;
    } else {
      eyeBrowEmotion = 0;
    }
  } else {
    if(y>40){
      eyeBrowPosUD = 5;
      eyeBrowEmotion = -1;
    } else {
      eyeBrowPosUD = 0;
    }
  }
  byte velD, velE;

  /*
  if((x>-centralXlimits)&&(x<centralXlimits)) //frente
  {
    if(y<-centralYlimits)
    {
      decabotMoveForward(-y);
    } else if(y>centralYlimits)
    {
      decabotMoveBackward(y);
    } else {
      decabotMoveStop();
    }
  }
  */
  //checa ajustes dos motores
  if(invertLeftRight) x = x * (-1);
  //if(invertRightMotor) y = y * (-1);

  if(y <= 0 && x >= 0){ //Frente Direita
    velD = constrain(abs(y)+ x, 0, 100);
    velE = constrain(abs(y)-(x/3), 0, 100);
    
    Serial.print("Frente Direita ");
    Serial.println(" x: " + String(x) + " y: " + String(y));
    Serial.println("                E: " + String(velE) + " D: " + String(velD));
    
    decabotMotorLeft(velE, 0);
    decabotMotorRight(velD, 0);
  } else if(y <= 0 && x <= 0){//Frente Esquerda 
    velD = constrain(abs(y)- (abs(x)/3), 0, 100);
    velE = constrain(abs(y)+ abs(x), 0, 100);
    
    Serial.print("Frente Esquerda");
    Serial.println(" x: " + String(x) + " y: " + String(y));
    Serial.println("                E: " + String(velE) + " D: " + String(velD));
    
    decabotMotorLeft(velE, 0);
    decabotMotorRight(velD, 0);
  } else if(y >= 0 && x >= 0){//Tras Direita
    velD = constrain(abs(y)+ abs(x), 0, 100);
    velE = constrain(abs(y)- (abs(x)/3), 0, 100);
    
    Serial.print("Tras Direita   ");
    Serial.println(" x: " + String(x) + " y: " + String(y));
    Serial.println("                E: " + String(velE) + " D: " + String(velD));

    decabotMotorRight(velD, 1);
    decabotMotorLeft(velE, 1);
  } else { //Tras Esquerda
    velD = constrain(abs(y)- (abs(x)/3), 0, 100);
    velE = constrain(abs(y)+ abs(x), 0, 100);
    
    Serial.print("Tras Esquerda  ");
    Serial.println(" x: " + String(x) + " y: " + String(y));
    Serial.println("                E: " + String(velE) + " D: " + String(velD));
    
    decabotMotorRight(velD, 1);
    decabotMotorLeft(velE, 1);
  }
  
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
  if (var == "recebido"){
    return recebido;
  }
  return String();
}

void decabotLoadingLine(int high){
  matrix.clear();
  matrix.drawLine(positionLoadLine-3,high,positionLoadLine,high, LED_ON);
  matrix.drawLine(positionLoadLine-3,high+1,positionLoadLine,high+1, LED_ON);
  matrix.writeDisplay();
  positionLoadLine++;
  if(positionLoadLine>11) positionLoadLine = 0;
}

void decabotDoneLoadingLine(int high){
  matrix.drawLine(0,high,7,high, LED_ON);
  matrix.drawLine(0,high+1,7,high+1, LED_ON);
  matrix.writeDisplay();
}

char* numberToHexStr(char* out, unsigned char* in, size_t length){
        char* ptr = out;
        for (int i = length-1; i >= 0 ; i--)
            ptr += sprintf(ptr, "%02X", in[i]);
        return ptr;
}

void decabotEye(int eyeType,int eyeLrPosition, int eyeUdPosition){
  matrix.clear();
  int lx = 1+eyeLrPosition;
  int rx = 6+eyeLrPosition;
  int ly = 5-eyeUdPosition;
  int ry = 5-eyeUdPosition;
  if(eyeType==0){
    matrix.drawPixel(lx,ly, LED_ON);
    matrix.drawPixel(rx,ry, LED_ON);
  }
  if(eyeType==1){
    matrix.drawLine(lx,ly,lx,ly-3, LED_ON);
    matrix.drawLine(rx,ry,rx,ry-3, LED_ON);
  }
  if(eyeType==2){
    matrix.drawLine(lx,ly+1,lx,ly-1, LED_ON);
    matrix.drawLine(lx+1,ly+1,lx+1,ly-1, LED_ON);
    
    matrix.drawLine(rx,ry+1,rx,ry-1, LED_ON);
    matrix.drawLine(rx-1,ry+1,rx-1,ry-1, LED_ON);
  }
  if(eyeType==3){
    matrix.drawLine(1,6,1,3, LED_ON);
    matrix.drawLine(2,6,2,3, LED_ON);
    matrix.drawLine(lx+1,6,lx+1,3, LED_ON);
    matrix.drawPixel(lx+1,ly, LED_OFF);
    matrix.drawLine(lx-1,6,lx-1,3, LED_OFF);
    
    matrix.drawLine(5,6,5,3, LED_ON);
    matrix.drawLine(6,6,6,3, LED_ON);
    matrix.drawLine(rx-1,6,rx-1,3, LED_ON);
    matrix.drawPixel(rx-1,ry, LED_OFF);
    matrix.drawLine(rx+1,6,rx+1,3, LED_OFF);
  }
  if(eyeType==4){
    matrix.drawLine(lx,ly+1,lx,ly, LED_ON);
    matrix.drawLine(lx+1,ly+1,lx+1,ly, LED_ON);
    if(lx<1){
      matrix.drawPixel(lx+2,ly-1, LED_ON);
    } else {
      matrix.drawPixel(lx-1,ly-1, LED_ON);
    }
    
    
    matrix.drawLine(rx,ry+1,rx,ry, LED_ON);
    matrix.drawLine(rx-1,ry+1,rx-1,ry, LED_ON);
    if(rx>6){
      matrix.drawPixel(rx-2,ry-1, LED_ON);
    } else {
      matrix.drawPixel(rx+1,ry-1, LED_ON);
    }
  }
}

void decabotEyeBrow(int emotion,int high){
  decabotEyeBrowElement(0,5-(high*1)-emotion);
  decabotEyeBrowElement(1,5-(high*1));
  decabotEyeBrowElement(2,5-(high*1)+emotion);
  matrix.drawLine(3,0,3,5-(high*1)+emotion, LED_OFF);
  matrix.drawLine(4,0,4,5-(high*1)+emotion, LED_OFF);
  decabotEyeBrowElement(5,5-(high*1)+emotion);
  decabotEyeBrowElement(6,5-(high*1));
  decabotEyeBrowElement(7,5-(high*1)-emotion);
}

void decabotEyeBrowElement(int colum,int heigh){
  matrix.drawLine(colum,0,colum,heigh, LED_OFF);
  matrix.drawPixel(colum,heigh, LED_ON);
}
