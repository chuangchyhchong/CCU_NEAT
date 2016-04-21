//for Arduino UNO
/*Pinout:
   DHT11 ，S->13，+->5V，- ->GND
   Esp8266，TX->RX1，RX->TX1，VCC->3.3V，CH_EN->5V，GND->GND
   PMS3003，由下往上，VCC->5V，GND->GND，SET->3.3V，RX->TX2，TX->RX2
   LCD，GND->GND，VCC->5V，SDA->SDA，SCL->SCL
*/
#include "DHT.h"
#define _baudrate   9600
#define _dhtpin     13
#define _dhttype    DHT11
DHT dht11( _dhtpin, _dhttype );

#include <SoftwareSerial.h>
SoftwareSerial PMS(4, 5);
SoftwareSerial esp( 3 , 2);//RX,TX
unsigned char pms3003[2];

#include <LiquidCrystal_I2C.h>
// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // 設定 LCD I2C 位址

//*-- IoT Information
String SSID = "WIFI_SSID";
String PASS = "WIFI_PASS";
String IP = "184.106.153.149"; // ThingSpeak IP Address: 184.106.153.149/*/ 144.212.80.10
String GET = "GET /update?key=[THINGSPEAK_KEY]";// 使用 GET 傳送資料的格式  GET /update?key=[THINGSPEAK_KEY]&field1=[data 1]&filed2=[data 2]...;
long start_time;

void sendtoesp(String cmd) {
  Serial.print("SEND: ");
  Serial.println(cmd);
  cmd += "\r\n";
  esp.print(cmd);
}
void LCD_Print(String W1, int a, int b) {
  lcd.setCursor(a, b);
  lcd.print(W1);
}
void esp_at_test() {
  lcd.clear();
  LCD_Print("Wait ESP AT", 0, 0);
  start_time=millis();
  sendtoesp("AT");
  while ( !(esp.available()) || !(esp.find("OK")) ) {
    if(millis()-start_time>10000){
      start_time=millis();
      sendtoesp("AT");
    }
  }
  Serial.println("AT OK");

  lcd.clear();
  sendtoesp("AT+CIPMUX=0");
}
boolean ConnectWiFi() {
  LCD_Print("Connecting WiFi ", 0, 0);
  sendtoesp("AT+CWJAP_CUR=\"" + SSID + "\",\"" + PASS + "\"");
  start_time=millis();
  while (!( esp.available() && esp.find("OK") )) {
    if(millis()-start_time>15000){
      start_time=millis();
      sendtoesp("AT+CWJAP_CUR=\"" + SSID + "\",\"" + PASS + "\"");
    }
  }
  Serial.println("RECEIVED: OK");
  LCD_Print("Connect WiFi OK ", 0, 0);
}
String do_pms() {
  PMS.begin(_baudrate);//Arduino to PMS3003
  int i;
  while (1) {
    PMS.readBytes(pms3003, 2);
    if (pms3003[0] == 0x42 || pms3003[1] == 0x4d) { //Start Character1,2
      for (i = 0; i < 5; i++)
        PMS.readBytes(pms3003, 2);
      Serial.print("atmosphere, PM2.5= ");
      Serial.print(pms3003[1]);
      Serial.println("ug/m3");
      break;
    }
  }
  esp.begin(115200);//Arduino to ESP8266  
  return (String)pms3003[1];
  }
void UpdateToTP() {
  //建立TCP
  LCD_Print("Building TCP... ", 0, 0);
  sendtoesp("AT+CIPSTART=\"TCP\",\"" + IP + "\",80");
  start_time=millis();
  /*while (   esp.available() > 0   &&   (!esp.find("CONNECT"))     ) {
    delay(1000);
    sendtoesp("AT+CIPSTART=\"TCP\",\"" + IP + "\",80");
  }*/
  while (   esp.available() > 0   &&   (!esp.find("CONNECT"))     ) {
    if(millis()-start_time>15000){
      start_time=millis();
      sendtoesp("AT+CWJAP_CUR=\"" + SSID + "\",\"" + PASS + "\"");
    }
  }

  Serial.println("RECV: TCP CONNECTED");

  //向ESP8266宣告要送幾個byte個字元
  LCD_Print("CIPSEND         ", 0, 0);
  String Update_command = GET + "&field1=" + (String)dht11.readTemperature() + "&field2=" + (String)dht11.readHumidity() + "&field3=" + (String)do_pms() + "\r\n";
  sendtoesp("AT+CIPSEND=" + (String)Update_command.length());
  start_time=millis();
  while (   esp.available() > 0   &&   (!esp.find(">"))     ) {
    if(millis()-start_time>15000){
      start_time=millis();
      sendtoesp("AT+CIPSEND=" + (String)Update_command.length());
    }
  }

  //送出指令
  Serial.print(">" + Update_command);
  esp.print(Update_command);
  start_time=millis();
  while (   esp.available() > 0   &&   (!esp.find("OK"))     ) {
    if(millis()-start_time>15000){
      start_time=millis();
      esp.print(Update_command);
    }
  }
  Serial.println( "UpdateSuccessful" );
  LCD_Print("UpdateSuccessful", 0, 0);

  //關閉TCP 連線
  sendtoesp("AT+CIPCLOSE");
}

void setup() {
  Serial.begin( _baudrate );//Arduino to computer
  esp.begin(115200);//Arduino to ESP8266  
  lcd.begin(16, 2);//表示16*2個字元

  lcd.backlight();//背光開//lcd.noBacklight(); //背光關。
  lcd.clear();
  LCD_Print("Hello! Junhan", 0, 0);
  LCD_Print("Starting...", 0, 1);
  for (int i = 0; i < 5; i++)//開機等五秒，因為esp8266剛上電會吐垃圾訊息
  {
    LCD_Print((String)(5 - i), 11, 1);
    delay(1000);
  }
  esp_at_test();//測試ESP8266有無反應
  dht11.begin();// DHT11 start
}
void loop() {
  //連上WiFi
  ConnectWiFi();
  //Update to ThingSpeak
  UpdateToTP();
  //顯示結果在LCD螢幕上
  lcd.clear();
  String PM_LCD = "PM2.5:" + (String)do_pms() + "ug/m3 ";
  LCD_Print(PM_LCD, 0, 0);
  String w = "Temp:" + (String)dht11.readTemperature();
  LCD_Print(w, 0, 1);
  w = "  Hmui:" + (String)dht11.readHumidity();
  LCD_Print(w, 7, 1);
  
  //斷開WiFi連線
  sendtoesp("AT+CWQAP");
  //while (!(esp.available() && esp.find("OK"))) {}
  Serial.println("Wait 60s\n");
  delay(60000);// 六十秒上傳一次
}
