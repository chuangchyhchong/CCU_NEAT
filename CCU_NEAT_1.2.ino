/*By CCU NEAT Junhan-Lin 
 * 
 * Pinout:
 * DHT11:S->13,+ ->5V,- ->GND
 * ESP-01:TX->RX1，RX->TX1，VCC->3.3V，CH_EN->5V，GND->GND
  *PMS3003:VCC->5V，GND->GND，SET->3.3V，RX->TX2，TX->RX2
  *LCD:GND->GND，VCC->5V，SDA->SDA，SCL->SCL
 */

#include "DHT.h"
#define _dhtpin         13
#define _dhttype       DHT11
DHT dht11(_dhtpin,_dhttype);

#include <SoftwareSerial.h>//這個library 同時只能begin一個serial port
SoftwareSerial esp(3,2); //Digital_3 as Rx ,Digital_2 as Tx
SoftwareSerial pms(4,5);

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);//addr,en,rw,rs,d4,d5,d6,d7,bl, blpol

#define pms_baudrate  9600
unsigned char pms3003[2];//store pms3003 data

#define esp_baudrate 115200
String SSID = "WIFI SSID";
String PASS = "WIFI PASS";
String IP = "184.106.153.149"; //ThingSpeak IP
String GET = "GET /update?key=_______________"; //ThingSpeak update key
// 使用 GET 傳送資料的格式  GET /update?key=[THINGSPEAK_KEY]&field1=[data 1]&filed2=[data 2]...;
long start_time;//for esp command retry
int i;
int esp_com_timeout;//esp8266 command timeout

String do_pms(){//get pms3003 data
  pms.begin(pms_baudrate);
  while(1){
    pms.readBytes(pms3003,2);
    if(pms3003[0]==0x42 || pms3003[1]==0x4d){//尋找每段資料的開頭
      for(i=0;i<5;i++)
        pms.readBytes(pms3003,2);      
      break;
    }
  }
  esp.begin(esp_baudrate);
  return (String)pms3003[1];
}
void lcd_print(String Str,int column,int row){//print word on lcd,column 0~15,row 0~1
  lcd.setCursor(column,row);
  lcd.print(Str);
}
void lcd_start_information(){
  lcd.backlight();//背光開 //lcd.noBacllight(); //背光關
  lcd.clear();
  lcd_print("Hello!",0,0);
  lcd_print("Starting...",0,1);
  for(i=0;i<5;i++){
    lcd_print((String)(5-i),11,1);
    delay(1000);
  }
}
void sendtoesp(String cmd){//send cmd to esp8266
  Serial.println("SEND: "+cmd);
  esp.print(cmd+"\r\n");
}
void test_esp(){
  lcd.clear();
  lcd_print("Wait ESP AT RESP",0,0);
  start_time=0;
  do{
    if(millis()-start_time>10000 || start_time==0){
      start_time=millis();
      sendtoesp("AT");
    }
  }while(  esp.available()>0  &&  (!esp.find("OK")) );
  Serial.println("AT OK");
}
void ConnectWiFi(){
  lcd.clear();
  lcd_print("Connecting WiFi ",0,0);
  
  sendtoesp("AT+CWJAP_CUR=\""+SSID+"\",\""+PASS+"\"");
  start_time=millis();
  while(1){
    if(millis()-start_time>=10000){//timeout 10s
      start_time=millis();
      sendtoesp("AT+CWJAP_CUR=\""+SSID+"\",\""+PASS+"\"");
    }
    if(esp.find("OK"))
      break;
  }
  Serial.println("RECV: Connect WiFi OK");
}
void build_TCP(){
  lcd_print("Building TCP    ",0,0);
  start_time=0;
  do{
    if(millis()-start_time>=15000 || start_time==0){
      start_time=millis();
      sendtoesp("AT+CIPSTART=\"TCP\",\"" +IP+ "\",80");
    }
  }while ( esp.available()>0 &&(!esp.find("CONNECT")));
  Serial.println("RECV:TCP CONNECTED");  
}
void send_update_command(){
  lcd_print("Send data length",0,0);
  String Update_command = GET+"&field1="+(String)dht11.readTemperature()+
                              "&field2="+(String)dht11.readHumidity()+
                              "&field3="+(String)do_pms()+"\r\n";
  i=1;
  do{
    start_time=millis();
    if(millis()-start_time>=15000 || i--)
      sendtoesp("AT+CIPSEND="+(String)Update_command.length());
  }while (esp.available()>0 &&(!esp.find(">")));

  //send Update_command
  Serial.print(">" +Update_command);
  i=1;
  do{
    start_time=millis();
    if(millis()-start_time>=15000 || i--)
      esp.print(Update_command);    
  }while ( (esp.available()>0) &&(!esp.find("OK")));
  Serial.println("RECV: UpdateSuccessful" );
}
void UpdateToTP(){  //Update to thingspeak
  build_TCP();//Build tcp
  send_update_command();//Tell to TCP server how many bytes to send  
}
void lcd_sensor_value(){
  lcd.clear();
  String PM_LCD = "PM2.5: " + (String)do_pms() + "ug/m3";
  lcd_print(PM_LCD,0,0);
  String TEMP_HUMI = "Temp:" + (String)dht11.readTemperature();
  lcd_print(TEMP_HUMI,0,1);
  TEMP_HUMI = "  Hmui:"+(String)dht11.readHumidity();
  lcd_print(TEMP_HUMI,7,1);
}
void setup(){
  Serial.begin(9600);//arduino to PC
  esp.begin(esp_baudrate);
  lcd.begin(16,2);

  lcd_start_information();//print start information on lcd
  test_esp();
  dht11.begin();
}
void loop(){
  ConnectWiFi();//Connect WiFi  
  UpdateToTP();//Update to TP
  lcd_sensor_value();//print sensor value on lcd  
  delay(60000);//every 60s to update data
}

