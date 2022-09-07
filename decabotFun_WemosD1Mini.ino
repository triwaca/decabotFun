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
 *            SERVO2 - GPIO16 - D0 |* |  ESP    | *| D1 --------- SCL
 *    led matrix CLK ---------- D5 |* |  8266   | *| D2 --------- SDA
 *          LED Blue - GPIO12 - D6 |* |         | *| D3 - GPIO0 - SERVO1
 *    led matrix DIN ---------- D7 |* |         | *| D4 - GPIO2 - LED WiFi
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
#include <FS.h>
//#include "SPIFFS.h"

//setup
const bool rgbLED = true;
const bool motors = true;
const bool servo = false;
const bool rfid = false;
const bool oled = false;
const bool matrixLED = false;
const bool wifi = true;
// Replace with your network credentials
//const char* ssid = "lapin";
//const char* password = "dpdil@p1N";
//const char* ssid = "brisa-2425163";
//const char* password = "6mjkisuz";
const char* ssid = "NET_2G15E89F";
const char* password = "E215E89F";

// Objects
MLED matrix(7); //set intensity=7 (maximum)
LOLIN_I2C_MOTOR motor; //I2C address 0x30
Servo servo1;
Servo servo2;
AsyncWebServer server(80);

//Cons

//Variables
const char compile_date[] = __DATE__ " " __TIME__;
#define rgbRedPin 1
#define rgbGreenPin 3
#define rgbBluePin 12
#define wifiLEDPin 2
#define buzzerPin 15
#define servo1Pin 0
#define servo2Pin 12
bool batteryAllarm = false;

String recebido = "0";
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
<!DOCTYPE html>
<meta name="viewport" content="width=device-width, initial-scale=0.25">
<html>
  <meta name="viewport" content="width=device-width, initial-scale=0.25">
  <head>
    <title>Joystick Controls</title>
    <style>
    body
    {
      font-family: Courier, monospaced;
      font-size: 20px;
      font-weight: bold;
    }
    </style>
  </head>

  <body>

    Joystick Controls.
    <hr>
      <div id="status1" style="color: red;">Joystick 1</div>
    <hr>
    <div style = "align-items: center ;position: absolute; left: 166px; top:150px; ">
      <button onclick="apertou()" type="button"> <img src="/laser_red" height ="64" width="64" /></button>
    </div>
    <div style="border: 1px solid red; width: 128px; position: absolute; left:150px; top:300px;">

      <div style="transform: scale(1,1);">
        <!-- <img src="images/joy_base.png"/> -->
        <img src="/joy_base"/>
      </div>

      <div id="stick1" style="transform: scale(1,1);position: absolute; left:32px; top:32px;">
        <div style="cursor: grab; ">
          <!-- <img src="images/joy_red.png"/> -->
          <img src="/joy_red"/>
        </div>
      </div>
    </div>

    <script>
      var xhr = new XMLHttpRequest();

      class JoystickController
      {
        // stickID: ID of HTML element (representing joystick) that will be dragged
        // maxDistance: maximum amount joystick can move in any direction
        // deadzone: joystick must move at least this amount from origin to register value change
        constructor( stickID, maxDistance, deadzone )
        {
          this.id = stickID;
          let stick = document.getElementById(stickID);

          // location from which drag begins, used to calculate offsets
          this.dragStart = null;

          // track touch identifier in case multiple joysticks present
          this.touchId = null;
          
          this.active = false;
          this.value = { x: 0, y: 0 }; 

          let self = this;

          function handleDown(event)
          {
            self.active = true;

            // all drag movements are instantaneous
            stick.style.transition = '0s';

            // touch event fired before mouse event; prevent redundant mouse event from firing
            event.preventDefault();

            if (event.changedTouches)
              self.dragStart = { x: event.changedTouches[0].clientX, y: event.changedTouches[0].clientY };
            else
              self.dragStart = { x: event.clientX, y: event.clientY };

            // if this is a touch event, keep track of which one
            if (event.changedTouches)
              self.touchId = event.changedTouches[0].identifier;
          }
          
          function handleMove(event) 
          {
            if ( !self.active ) return;

            // if this is a touch event, make sure it is the right one
            // also handle multiple simultaneous touchmove events
            let touchmoveId = null;
            if (event.changedTouches)
            {
              for (let i = 0; i < event.changedTouches.length; i++)
              {
                if (self.touchId == event.changedTouches[i].identifier)
                {
                  touchmoveId = i;
                  event.clientX = event.changedTouches[i].clientX;
                  event.clientY = event.changedTouches[i].clientY;
                }
              }

              if (touchmoveId == null) return;
            }

            const xDiff = event.clientX - self.dragStart.x;
            const yDiff = event.clientY - self.dragStart.y;
            const angle = Math.atan2(yDiff, xDiff);
            const distance = Math.min(maxDistance, Math.hypot(xDiff, yDiff));
            const xPosition = distance * Math.cos(angle);
            const yPosition = distance * Math.sin(angle);

            // move stick image to new position
            stick.style.transform = `translate3d(${xPosition}px, ${yPosition}px, 0px)`;

            // deadzone adjustment
            const distance2 = (distance < deadzone) ? 0 : maxDistance / (maxDistance - deadzone) * (distance - deadzone);
            const xPosition2 = distance2 * Math.cos(angle);
            const yPosition2 = distance2 * Math.sin(angle);
            const xPercent = parseFloat((xPosition2 / maxDistance).toFixed(4));
            const yPercent = parseFloat((yPosition2 / maxDistance).toFixed(4));
            
            self.value = { x: parseInt(xPosition), y: parseInt(yPosition) };

            let joy_y = joystick1.value.y;
            let joy_x = joystick1.value.x;
            let joy_xy = 'M' + joy_x + '|' + joy_y;
            
            xhr.open("GET", "/joystick?value="+joy_xy, true)
            xhr.send();
            console.log("M" + joy_xy);
          }

          function handleUp(event) 
          {
            if ( !self.active ) return;

            // if this is a touch event, make sure it is the right one
            if (event.changedTouches && self.touchId != event.changedTouches[0].identifier) return;

            // transição do joystick para o centro
            stick.style.transition = '.2s';
            stick.style.transform = `translate3d(0px, 0px, 0px)`;

            // zera tudo
            self.value = { x: 0, y: 0 };

            for (let i = 0; i < 3; i++) //evitar falhas ao enviar (0,0)
            {
              xhr.open("GET", "/joystick?value="+"M00|00", true)
              xhr.send();
              console.log("Soltou --> 0");
            }
            

            self.touchId = null;
            self.active = false;
          }

          stick.addEventListener('mousedown', handleDown);
          stick.addEventListener('touchstart', handleDown);
          document.addEventListener('mousemove', handleMove, {passive: false});
          document.addEventListener('touchmove', handleMove, {passive: false});
          document.addEventListener('mouseup', handleUp);
          document.addEventListener('touchend', handleUp);
        }
      }

      let joystick1 = new JoystickController("stick1", 100, 6);
      //let joystick2 = new JoystickController("stick2", 64, 8);
      function apertou()
      {
        xhr.open("GET", "/joystick?value="+"B1", true)
        xhr.send();
        console.log("Apertou");
      }
      function update()
      {
        document.getElementById("status1").innerText = "Joystick 1: " + JSON.stringify(joystick1.value);
      }

      function loop()
      {
        requestAnimationFrame(loop);
        update();
      }

      loop();

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
  
  decabotMessage("Inicializando SPIFFS...");
  if(SPIFFS.begin())
  {
    decabotMessage("OK!");
  }
  else
  {
    decabotMessage("Erro! falha de SPIFFS");
  }
  
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
    decabotColor(7,1);
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
      delay(500);
      if(tempMotor > 40){
        Serial.println("");
        decabotMessage("Are you sure Motor Shield is OK?!");
        tempMotor = 0;
      }
    }
    motor.changeFreq(MOTOR_CH_BOTH, 1000);
    decabotMessage("OK!");
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

  pinMode(wifiLEDPin,OUTPUT);
  digitalWrite(wifiLEDPin, HIGH);
  if(wifi)
  {
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    decabotMessage("Connecting to WiFi " + (String)ssid);
    int tempCon = 0;
    while (WiFi.status() != WL_CONNECTED) 
    {
      tempCon++;
      if(tempCon%2==0){
        digitalWrite(wifiLEDPin, LOW);
      }else {
        digitalWrite(wifiLEDPin, HIGH);
      }
      delay(500);
      Serial.print(".");
      if(tempCon > 40){
        Serial.println("");
        decabotMessage("Are you sure WiFi " + (String)ssid + " is OK?!");
        tempCon = 0;
      }
    }
    Serial.println("OK!");
    Serial.println(WiFi.localIP());
    digitalWrite(wifiLEDPin, LOW);
    
    // Camimho da pagina raiz / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, decabotWebProcessor);
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
    
    server.on("/joystick", HTTP_GET, [] (AsyncWebServerRequest *request) 
    {
      String inputMessage;
      
      // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
      if (request->hasParam(PARAM_INPUT)) 
      {
        
        inputMessage = request->getParam(PARAM_INPUT)->value();
        recebido = inputMessage;
        Serial.println("mensagem: " + String(recebido));
        
        maquina_de_estados(recebido);

        //Serial.println("posY: " + String(pY) + "posX: " + String(pX));
        //Serial.println("Index do | : " + String(index_split));
        //analogWrite(output, recebido.toInt());
        //myservo.write(map(recebido.toInt(),0,1023,0,180)); 
 
      }
      else 
      {
        inputMessage = "No message sent";
      }
      //Serial.println(inputMessage);
      request->send(200, "text/plain", "OK");
    });
    
    // Start server
    server.begin();
  } 
  else 
  {
    decabotMessage("No WI-FI connection setup!");
    digitalWrite(wifiLEDPin, HIGH);
  }
  
  decabotThemeMusic();
  decabotFace(0);
  decabotColor(7,0);
  decabotMessage("Decabot Ready!");

}

void loop() {
//  decabotFace(0);
//  delay(3000);
//  decabotFace(1);
//  delay(50);
//  decabotFace(2);
//  delay(50);
//  decabotFace(3);
//  delay(60);
  //Loop pra que? se a gente usa interrupts?
  //decabotSpaceGun(1000);
}

//Functions Decabot

void maquina_de_estados(String msg)
{
  char chave = msg[0];
  String posX, posY;
  int index_split, pX, pY;
  switch(chave)
  {
    case 'M':
      
      index_split = recebido.indexOf('|');
      posX = recebido.substring(1, index_split);
      posY = recebido.substring(index_split+1);
      pX = posX.toInt();
      pY = posY.toInt();
      
      decabotMove(map(pX,0,64,0,100), map(pY,0,64,0,100));
      break;
    case 'B':
      decabotGunShot(1);
      break;
  }
  
}

void decabotGunShot(int qual)
{
  bool stp = false;
  switch(qual)
  {
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
        tone(buzzerPin,i);
        //delay(1);
        delayMicroseconds(700);
        
      }
      decabotColor(5,1);
      stp = false;
      noTone(buzzerPin);
      break;
    case 2:
      for(int i=1000;i>700;i--)
      {
        tone(buzzerPin,i);
        delay(1);
      }
      noTone(buzzerPin);
      break;
  }
}

void decabotThemeMusic()
{
  decabotMessage("Playing Decabot Music Theme...");
  for(int i=0;i<4;i++)
  {
    tone(buzzerPin, decabotMusic[i][0], decabotMusic[i][1]*200);
    decabotColor(decabotMusic[i][2], 1);
    delay(decabotMusic[i][1]*200);
    noTone(buzzerPin);
  }
  decabotMessage("OK!");
}

void decabotMessage(String text)
{
  if(text == "OK!")
  {
    Serial.println(text);
  } 
  else 
  {
    Serial.print(millis());
    Serial.print(" - "); 
    if(text.endsWith("..."))
    {
      Serial.print(text);
    } 
    else 
    {
      Serial.println(text);
    } 
  }
}

void decabotColor(int color, bool mute)
{
  if(!mute) decabotMessage("Set color to " + (String) color);
  
  switch(color)
  {
    case 0:
      digitalWrite(rgbBluePin, LOW);
      digitalWrite(rgbRedPin, LOW);
      digitalWrite(rgbGreenPin, LOW);
      break;
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

void decabotFace(int draw)
{
  if(matrixLED)
  {
    decabotMessage("Change face to " + (String) draw);
    matrix.clear();
    matrix.drawBitmap(0, 0, eyes_bmp[draw], 8, 8, LED_ON);
    matrix.writeDisplay();  
  } 
  else 
  {
    decabotMessage("No LED Matrix installed!");
  }
}

int readBatteryLevel()
{
  int reading = map(analogRead(A0),704,955,0,100);
  if (reading<0) reading = 0;
  if (reading>100) reading = 100;
  if (reading<10)
  {
    matrix.drawLine(1,0,6,7, LED_ON);
    matrix.writeDisplay();
    if(batteryAllarm)
    {
      batteryAllarm = false;
      noTone(buzzerPin);
      decabotColor(0,1);
    } 
    else 
    {
      batteryAllarm = true;
      tone(buzzerPin, 494, 200);
      decabotColor(3,1);
    }
  }
  return reading;
}

void decabotShowBatLevel()
{
  matrix.clear();
  matrix.drawRect(0,2, 7,4, LED_ON);
  matrix.drawLine(7,3,7,4, LED_ON);
  matrix.fillRect(1,3, map(readBatteryLevel(),0,100,0,5),2, LED_ON);
  matrix.writeDisplay();
  //decabotMessage(readBatteryLevel().toString());
}

void decabotMotorLeft(int power, bool fwd)
{
  if(fwd)
  {
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CW);
  } 
  else 
  {
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_CCW);
  }
  if(power<=15)
  {
    motor.changeStatus(MOTOR_CH_A, MOTOR_STATUS_SHORT_BRAKE);
  } 
  else 
  {
    motor.changeDuty(MOTOR_CH_A, power);
  }
}

void decabotMotorRight(int power, bool fwd)
{
  if(fwd)
  {
    motor.changeStatus(MOTOR_CH_B, MOTOR_STATUS_CW);
  } 
  else 
  {
    motor.changeStatus(MOTOR_CH_B, MOTOR_STATUS_CCW);
  }
  if(power<=15)
  {
    motor.changeStatus(MOTOR_CH_B, MOTOR_STATUS_SHORT_BRAKE);
  } 
  else 
  {
    motor.changeDuty(MOTOR_CH_B, power);
  }
}

void decabotMoveRotate(bool cw, int pot)
{
  if(cw)
  {
    //clock-wise
    decabotMotorLeft(pot,1);
    decabotMotorRight(pot,0);
  } 
  else 
  {
    //couter clock-wise
    decabotMotorLeft(pot,0);
    decabotMotorRight(pot,1);
  }
}

void decabotMoveStop()
{
  decabotMotorRight(0, 1);
  decabotMotorLeft(0, 1);
}

void decabotMoveForward(int pot)
{
  decabotMotorRight(pot, 1);
  decabotMotorLeft(pot, 1);
}

void decabotMove(int x, int y)
{
  byte velD, velE;
  
  if(y <= 0 && x >= 0)//Frente Direita
  {
    velD = constrain(abs(y)+ x, 0, 100);
    velE = constrain(abs(y)-(x/3), 0, 100);
    
    Serial.print("Frente Direita ");
    Serial.println(" x: " + String(x) + " y: " + String(y));
    Serial.println("                E: " + String(velE) + " D: " + String(velD));
    
    decabotMotorLeft(velE, 0);
    decabotMotorRight(velD, 0);
  }
  else if(y <= 0 && x <= 0)//Frente Esquerda
  { 
    velD = constrain(abs(y)- (abs(x)/3), 0, 100);
    velE = constrain(abs(y)+ abs(x), 0, 100);
    
    Serial.print("Frente Esquerda");
    Serial.println(" x: " + String(x) + " y: " + String(y));
    Serial.println("                E: " + String(velE) + " D: " + String(velD));
    
    decabotMotorLeft(velE, 0);
    decabotMotorRight(velD, 0);
  }
  else if(y >= 0 && x >= 0)//Tras Direita
  {
    velD = constrain(abs(y)+ abs(x), 0, 100);
    velE = constrain(abs(y)- (abs(x)/3), 0, 100);
    
    Serial.print("Tras Direita   ");
    Serial.println(" x: " + String(x) + " y: " + String(y));
    Serial.println("                E: " + String(velE) + " D: " + String(velD));

    decabotMotorRight(velD, 1);
    decabotMotorLeft(velE, 1);
  }
  else//Tras Esquerda
  {
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
