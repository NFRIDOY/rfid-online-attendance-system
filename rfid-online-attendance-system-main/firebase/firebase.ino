#include <ESP8266WiFi.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h> 
#include <RFID.h>
#include "FirebaseESP8266.h"  // Install Firebase ESP8266 library
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h> 
int servoPin = 0; //D3
Servo Servo1; 
#define BUZ_PIN D4 
// getFormattedDate 
// Card found
// FAILED
#define FIREBASE_HOST "rfidu-e0ebd-default-rtdb.firebaseio.com" //Without http:// or https:// schemes
#define FIREBASE_AUTH "8OW4YsFLt9XB6IdCLrxL2Ig5KT605fXJTY6wzzCT"
RFID rfid(D8, D0);       //D10:pin of tag reader SDA. D9:pin of tag reader RST 
unsigned char str[MAX_LEN]; //MAX_LEN is 16: size of the array 
LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 21600; //(UTC+6)
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char ssid[] = "TP-Link_6104";
const char pass[] = "nfridoy12345";

String uidPath= "/";
FirebaseJson json;
//Define FirebaseESP8266 data object
FirebaseData firebaseData;

unsigned long lastMillis = 0;
//const int red = D4;
//const int green = D3;
String alertMsg;
String device_id="Gate101";
boolean checkIn = true;
int star=0;

void connect() {
  Serial.print("\nChecking wifi...");
  lcd.setCursor(0,0); 
  lcd.print("CHECKING WIFI...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    lcd.clear();
    lcd.setCursor(1,0); 
    lcd.print("CHECKING WIFI..");
    lcd.setCursor(star,1); 
    lcd.print("*");
    delay(200);
    star=star+1;
  }

  lcd.clear();
  Serial.println("\n Connected!");
  lcd.setCursor(1,0); 
  lcd.print("CHECKING WIFI...");
  lcd.setCursor(1,1); 
  lcd.print("CONNECTED!");
  delay(3000);
}
/*
void connect() {
  Serial.print("\nChecking wifi...");
  lcd.setCursor(1,0); 
  lcd.print("CHECKING WIFI...");
  while (WiFi.status() != WL_CONNECTED) {
    lcd.setCursor(1,0); 
    Serial.print(".");
    lcd.print("*");
    delay(1000);
  }

  Serial.println("\n Connected!");
  lcd.setCursor(1,0); 
  lcd.print("CONNECTED!");
}
*/
void sarvox() {
  Serial.print("\nGate Opening");
  Servo1.write(180);
  digitalWrite(BUZ_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZ_PIN, LOW);
  delay(4000);
  Servo1.write(0);
  Serial.print("\nGate Closing");
  delay(500);
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  
  Servo1.attach(servoPin); 
  Servo1.write(0);

  pinMode(BUZ_PIN, OUTPUT);

  

  //pinMode(red, OUTPUT);
  //pinMode(green, OUTPUT);
  lcd.init();                      // initialize the lcd 
  lcd.clear();
  lcd.backlight();

  SPI.begin();
  rfid.init();
  
  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
  connect();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}
void checkAccess (String temp)    //Function to check if an identified tag is registered to allow access
{
    lcd.setCursor(1,0);   
    lcd.print("SCAN YOUR RFID");

    if(Firebase.getInt(firebaseData, uidPath+"/users/"+temp)){
      
      if (firebaseData.intData() == 0)         //If firebaseData.intData() == checkIn
      {  
        
          
          alertMsg="CHECKING IN";
          lcd.setCursor(2,1);   
          lcd.print(alertMsg);
          delay(2000);
          sarvox();
          
          //delay(4000);

          //json.add("time", String(timeClient.getFormattedDate()));
          json.add("time", String(timeClient.getFormattedTime()));
          json.add("id", device_id);
          json.add("uid", temp);
          json.add("status",1);

          Firebase.setInt(firebaseData, uidPath+"/users/"+temp,1);
          
          if (Firebase.pushJSON(firebaseData, uidPath+ "/attendence", json)) {
            Serial.println(firebaseData.dataPath() + firebaseData.pushName()); 
          } else {
            Serial.println(firebaseData.errorReason());
          }
      }
      else if (firebaseData.intData() == 1)   //If the lock is open then close it
      { 
          sarvox();
          alertMsg="CHECKING OUT";

          lcd.setCursor(2,1);   
          lcd.print(alertMsg);
          delay(2000);

          //delay(4000);

          Firebase.setInt(firebaseData, uidPath+"/users/"+temp,0);
          
          json.add("time", String(timeClient.getFormattedTime()));
          json.add("id", device_id);
          json.add("uid", temp);
          json.add("status",0);
          if (Firebase.pushJSON(firebaseData, uidPath+ "/attendence", json)) {
            Serial.println(firebaseData.dataPath() + firebaseData.pushName()); 
          } else {
            Serial.println(firebaseData.errorReason());
          }
      }
 
    }
    else
    {
      Serial.println("FAILED");
      lcd.setCursor(1,1);   
      lcd.print("FAILED");
      lcd.setCursor(2,1);   
      lcd.print("UNAUTHORIZED");
      digitalWrite(BUZ_PIN, HIGH);  // turn the LED on (HIGH is the voltage level)
      delay(300); 
      digitalWrite(BUZ_PIN, LOW);   // turn the LED off by making the voltage LOW
      delay(300); 
      digitalWrite(BUZ_PIN, HIGH);  // turn the LED on (HIGH is the voltage level)
      delay(300); 
      digitalWrite(BUZ_PIN, LOW);   // turn the LED off by making the voltage LOW
      delay(300); 
      lcd.setCursor(1,1);   
      lcd.print("FAILED");
      lcd.setCursor(2,1);   
      lcd.print("UNAUTHORIZED");
      delay(4000);                      // wait for a second
      digitalWrite(BUZ_PIN, LOW);   // turn the LED off by making the voltage LOW
      delay(500);  
      lcd.clear();
      //Servo1.write(0);
      Serial.println("REASON: " + firebaseData.errorReason());
    }
}
void loop() {
  //Servo1.write(0);
  timeClient.update();
  if (rfid.findCard(PICC_REQIDL, str) == MI_OK)   //Wait for a tag to be placed near the reader
  { 
    Serial.println("Card found"); 
    /*lcd.clear();
    lcd.setCursor(1,0); 
    lcd.print("CARD FOUND!!!");
    delay(1000);*/
    
    String temp = "";                             //Temporary variable to store the read RFID number
    if (rfid.anticoll(str) == MI_OK)              //Anti-collision detection, read tag serial number 
    { 
      Serial.print("The card's ID number is : "); 
      for (int i = 0; i < 4; i++)                 //Record and display the tag serial number 
      { 
        temp = temp + (0x0F & (str[i] >> 4)); 
        temp = temp + (0x0F & str[i]); 
      } 
      Serial.println (temp);
      checkAccess (temp);     //Check if the identified tag is an allowed to open tag
    } 
    rfid.selectTag(str); //Lock card to prevent a redundant read, removing the line will make the sketch read cards continually
  }
  rfid.halt();

  lcd.setCursor(1,0);   
  lcd.print("SCAN YOUR RFID");
  lcd.setCursor(2,1);   
  lcd.print("GATE CLOSE");
  //Servo1.write(0); 
  lcd.clear();
  delay(500);
}
