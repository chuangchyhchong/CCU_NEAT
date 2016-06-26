/******************** LoRa  mode**************************/
//Error Coding rate (CR)setting
#define CR_4_5 0
#define CR  0x01  // 4/5
//CRC Enable
#define CRC_EN
#define CRC 0x01  //CRC Enable
//RFM98 Internal registers Address
/********************Lroa mode***************************/
// Common settings
#define LR_RegOpMode  0x01
#define LR_RegFrMsb 0x06
// Tx settings
#define LR_RegPaConfig  0x09
#define LR_RegOcp 0x0B
// Rx settings
#define LR_RegLna 0x0C
#define LR_RegFifoAddrPtr 0x0D
#define LR_RegFifoTxBaseAddr  0x0E
#define LR_RegFifoRxBaseAddr  0x0F
#define LR_RegFifoRxCurrentaddr 0x10
#define LR_RegIrqFlagsMask  0x11
#define LR_RegIrqFlags  0x12
#define LR_RegRxNbBytes 0x13
#define LR_RegModemStat 0x18
#define LR_RegPktRssiValue 0x1A
#define LR_RegModemConfig1  0x1D
#define LR_RegModemConfig2  0x1E
#define LR_RegSymbTimeoutLsb  0x1F
#define LR_RegPreambleMsb 0x20
#define LR_RegPreambleLsb 0x21
#define LR_RegPayloadLength 0x22
#define LR_RegHopPeriod 0x24
// I/O settings
#define REG_LR_DIOMAPPING1  0x40
#define REG_LR_DIOMAPPING2  0x41
// Additional settings
#define REG_LR_PADAC  0x4D

/*coap packet type*/
#define ASK_SENSOR_DATA                 0x0B //11
#define ACK_SENSOR_DATA                 0x0C //12
#define LORA_PARAMETER_CHANGE           0x15 //21
#define LORA_PARAMETER_CHANGE_ACK       0x16 //22  
#define LORA_PARAMETER_COMMIT           0x17 //23
#define LORA_PARAMETER_COMMIT_ACK       0x18 //24
#define GATEWAY_BEACON                  0x1F //31
#define END_DEVICE_JOIN                 0x20 //32
#define GATE_ACCEPT_JOIN                0x21 //33

/*AT Command*/
#define AT_TYPE_FRE     0x01
#define AT_TYPE_POW     0x02
#define AT_TYPE_SF      0x03
#define AT_TYPE_BW      0x04
#define AT_TYPE_TIMEOUT 0x05
#define AT_TYPE_RST     0x06
#define AT_COMMAND_COMMIT_TMDA 0x02

#define TMDA_TIME 0x03

/*default lora parameter*/
#define def_mode          0x01
#define def_Freq_Sel      0x00
#define def_Power_Sel     0x00
#define def_Lora_Rate_Sel 0x06
#define def_BandWide_Sel  0x09
/*********Pinout*****************************************/
int dio0 = 7;
int nsel = 6;
int sck  = 5;
int mosi = 4;
int miso = 3;
int analog_random_pin = A0;
/* sx1276_7_8Data
   00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19
   40 01 00 00 ff ta id sr id ty pa pa pa pa pa pa pa
*/
unsigned char sx1276_7_8Data[20] = {0};
unsigned char CoAP_Get_Header[5] = {0x40, 0x01, 0x00, 0x00, 0xFF};
unsigned char CoAP_Post_Header[5] = {0x40, 0x01, 0x00, 0x00, 0xFF};
unsigned char RxData[30];
unsigned char Child_table[10];
unsigned long Change_Parameter_Commit_Timeout;
unsigned long no_child_every_10s_send_beacon;
uint16_t Device_ID = 0;                   //0: gateway。1~65534:node。65535:all node。
uint16_t ID ;                             //send packet to this ID
uint8_t Child_Number = 0;
unsigned long RxPacket_timeout;// for listen some lora packet ack,wait some time
uint16_t Message_ID;                     //in coap packet header message id
String LoRaRSSI;                         //the lora rssi when gateway recvice
String parameter_order;
String AT_Command;
int i, j;

/*Serial38266 parameter*/
//#include <SoftwareSerial.h>//這個library 同時只能begin一個serial port
//SoftwareSerial esp(3, 2);

#define Serial_baudrate 9600
#define esp_baudrate 115200
String SSID = "6s";//"NEAT_2.4G"; // ;"HAPPY541"
String PASS = "88888888";//"221b23251"; //"coveryou" ;
String IP = "184.106.153.149";//ThingSpeak IP
String GET ; //GET /update?key=[THINGSPEAK_KEY]&field1=[data 1]&filed2=[data 2]...;
String Update_command;
unsigned long esp_com_timeout = 10 * 1000; //Unit:(ms) esp8266 command timeout
int esp_com_error_times;//esp8266 command timeout
unsigned long every_60s_send_to_Thingspeak;

/*********Parameter table define**************************/
unsigned char mode;//lora--1/FSK--0
unsigned char Freq_Sel;
unsigned char Power_Sel;
unsigned char Lora_Rate_Sel;
unsigned char BandWide_Sel;
unsigned char Fsk_Rate_Sel;
unsigned char sx1276_7_8FreqTbl[2][3] = {
  {0x6B, 0x00, 0x00}, {0x00, 0x00, 0x00}, //428MHz
};
unsigned char sx1276_7_8PowerTbl[4] = {
  0xFF, 0xFC, 0xF9, 0xF6, //20dbm,17dbm,14dbm,11dbm
};
unsigned char sx1276_7_8SpreadFactorTbl[7] = {
  6, 7, 8, 9, 10, 11, 12
};
unsigned char sx1276_7_8LoRaBwTbl[10] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9 //7.8,10.4,15.6,20.8,31.2,41.7,62.5,125,250,500KHz
};

/***********************SPI Function***********************/
void SPICmd8bit(unsigned char WrPara) {
  unsigned char bitcnt;
  digitalWrite(nsel, LOW);//nSEL_L();
  digitalWrite(sck, LOW);//SCK_L();
  for (bitcnt = 8; bitcnt != 0; bitcnt--)
  {
    digitalWrite(sck, LOW);//SCK_L();
    if (WrPara & 0x80)//最高位元是否為1
      digitalWrite(mosi, HIGH);//SDI_H();
    else
      digitalWrite(mosi, LOW);//SDI_L();

    digitalWrite(sck, HIGH);//SCK_H();
    WrPara <<= 1;
  }
  digitalWrite(sck, LOW);//SCK_L();
  digitalWrite(mosi, HIGH);//SDI_H();
}
unsigned char SPIRead8bit(void) {
  unsigned char RdPara = 0;
  unsigned char bitcnt;

  digitalWrite(nsel, LOW);//nSEL_L();
  digitalWrite(mosi, HIGH);//SDI_H(); //Read one byte data from FIFO, MOSI hold to High
  for (bitcnt = 8; bitcnt != 0; bitcnt--)
  {
    digitalWrite(sck, LOW);//SCK_L();
    RdPara <<= 1;
    digitalWrite(sck, HIGH); //SCK_H();

    if (digitalRead(miso)) //if(Get_SDO())
      RdPara |= 0x01;
    else
      RdPara |= 0x00;
  }
  digitalWrite(sck, LOW);//SCK_L();
  return (RdPara);
}
unsigned char SPIRead(unsigned char adr) {
  unsigned char tmp;

  SPICmd8bit(adr);  //Send address first
  tmp = SPIRead8bit();
  digitalWrite(nsel, HIGH);//nSEL_H();
  return (tmp);
}
void SPIWrite(unsigned char adr, unsigned char WrPara) {
  digitalWrite(nsel, LOW);//nSEL_L();
  SPICmd8bit(adr | 0x80); //寫入地址，一定大於0x80
  SPICmd8bit(WrPara);//寫入數據
  digitalWrite(sck, LOW);//SCK_L();
  digitalWrite(mosi, HIGH);//SDI_H();
  digitalWrite(nsel, HIGH);//nSEL_H();
}
void SPIBurstRead(unsigned char adr, unsigned char *ptr, unsigned char leng) {
  unsigned char i;

  if (leng <= 1) //length must more than  one
    return;
  else
  {
    digitalWrite(sck, LOW); //SCK_L();
    digitalWrite(nsel, LOW);//nSEL_L();

    SPICmd8bit(adr);
    for (i = 0; i < leng; i++)
      ptr[i] = SPIRead8bit();
    digitalWrite(nsel, HIGH);//nSEL_H();
  }
}
void BurstWrite(unsigned char adr, unsigned char *ptr, unsigned char leng) {
  unsigned char i;

  if (leng <= 1) //length must more than  one
    return;
  else
  {
    digitalWrite(sck, LOW);//SCK_L();
    digitalWrite(nsel, LOW);//nSEL_L();
    SPICmd8bit(adr | 0x80);
    for (i = 0; i < leng; i++)
      SPICmd8bit(ptr[i]);
    digitalWrite(nsel, HIGH);//nSEL_H();
  }
}
/***********************LoRa mode**************************/
void sx1276_7_8_Standby(void) {
  /*   7  :0->FSK/OOK Mode，1->LoRa mode
       6-5:00->FSK，01->OOK，10->11->reserved
       4  :0->reserved
       3  :0->High 1->Low frequency mode
       2-0:000->sleep，001->standby，....*/
  SPIWrite(LR_RegOpMode, 0x09);//0000 1001
  //SPIWrite(LR_RegOpMode,0x01);//0000 0001
}
void sx1276_7_8_Sleep(void) {
  SPIWrite(LR_RegOpMode, 0x08);//0000 1000
  //SPIWrite(LR_RegOpMode,0x00);
}
void sx1276_7_8_EntryLoRa(void) {
  SPIWrite(LR_RegOpMode, 0x88); //Low Frequency Mode//1000 1000
  //SPIWrite(LR_RegOpMode,0x80);//High Frequency Mode
}
void sx1276_7_8_LoRaClearIrq(void) {
  SPIWrite(LR_RegIrqFlags, 0xFF);
}
unsigned char sx1276_7_8_LoRaReadRSSI(void) {
  int temp = 10;
  temp = SPIRead(LR_RegPktRssiValue); //Read RegRssiValue，Rssi value
  //temp = temp - 164;
  LoRaRSSI = temp - 164; //127:Max RSSI, 137: RSSI offset
}
unsigned char sx1276_7_8_LoRaEntryRx(void) {
  unsigned char addr;

  sx1276_7_8_Config();  //setting base parameter

  SPIWrite(REG_LR_PADAC, 0x84); //Normal and Rx //Higher power settings of the PA
  SPIWrite(LR_RegHopPeriod, 0xFF); //RegHopPeriod NO FHSS
  SPIWrite(REG_LR_DIOMAPPING1, 0x01);  //DIO0=00, DIO1=00, DIO2 = 00, DIO3 = 01

  SPIWrite(LR_RegIrqFlagsMask, 0x3F); //Open RxDone interrupt & Timeout 0011 1111
  sx1276_7_8_LoRaClearIrq();

  SPIWrite(LR_RegPayloadLength, 27); //RegPayloadLength 21byte(this register must difine when the data long of one byte in SF is 6)

  addr = SPIRead(LR_RegFifoRxBaseAddr); //Read RxBaseAddr
  SPIWrite(LR_RegFifoAddrPtr, addr); //RxBaseAddr -> FiFoAddrPtr
  SPIWrite(LR_RegOpMode, 0x8d); //Continuous  Rx Mode//Low Frequency Mode
  //SPIWrite(LR_RegOpMode,0x05);  //Continuous  Rx Mode//High Frequency Mode
  //SysTime = 0;
  while (1)
  {
    if ((SPIRead(LR_RegModemStat) & 0x04) == 0x04) //Rx-on going RegModemStat
      break;
  }
}
unsigned char sx1276_7_8_LoRaRxPacket(void) {
  unsigned char i;
  unsigned char addr;
  unsigned char packet_size;

  if (digitalRead(dio0)) //if(Get_NIRQ())
  {
    for (i = 0; i < 32; i++)
      RxData[i] = 0x00;

    addr = SPIRead(LR_RegFifoRxCurrentaddr);//last packet addr
    SPIWrite(LR_RegFifoAddrPtr, addr);//RxBaseAddr ->   FiFoAddrPtr
    if (sx1276_7_8SpreadFactorTbl[Lora_Rate_Sel] == 6) //When SpreadFactor is six，will used Implicit Header mode(Excluding internal packet length)
      packet_size = 21;
    else
      packet_size = SPIRead(LR_RegRxNbBytes); //Number for received bytes
    SPIBurstRead(0x00, RxData, packet_size);

    if ( RxData[0] == 0x40 || RxData[0] == 0x60) {
      Serial.print("RECV: ");
      for (i = 0; i < packet_size; i++) {
        Serial.print(RxData[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    sx1276_7_8_LoRaClearIrq();
    return (Check_ID());
  }
  else
    return (0);
}
unsigned char sx1276_7_8_LoRaEntryTx(void) {
  unsigned char addr, temp;

  sx1276_7_8_Config();  //setting base parameter

  SPIWrite(REG_LR_PADAC, 0x87); //Tx for 20dBm
  SPIWrite(LR_RegHopPeriod, 0x00); //RegHopPeriod NO FHSS
  SPIWrite(REG_LR_DIOMAPPING1, 0x41); //DIO0=01, DIO1=00, DIO2 = 00, DIO3 = 01

  sx1276_7_8_LoRaClearIrq();
  SPIWrite(LR_RegIrqFlagsMask, 0xF7); //Open TxDone interrupt
  SPIWrite(LR_RegPayloadLength, sizeof(sx1276_7_8Data)); //RegPayloadLength 21byte

  addr = SPIRead(LR_RegFifoTxBaseAddr); //RegFiFoTxBaseAddr
  SPIWrite(LR_RegFifoAddrPtr, addr);  //RegFifoAddrPtr

  //SysTime = 0;

}
unsigned char sx1276_7_8_LoRaTxPacket(void) {
  unsigned char TxFlag = 0;
  unsigned char addr;

  BurstWrite(0x00, (unsigned char *)sx1276_7_8Data, sizeof(sx1276_7_8Data));
  Serial.print("SEND: ");
  for (i = 0; i < sizeof(sx1276_7_8Data); i++) {
    Serial.print(sx1276_7_8Data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  SPIWrite(LR_RegOpMode, 0x8b); //Tx Mode
  while (1)
  {
    if (digitalRead(dio0)) //if(Get_NIRQ())     //Packet send over
    {
      SPIRead(LR_RegIrqFlags);
      sx1276_7_8_LoRaClearIrq();  //Clear irq
      sx1276_7_8_Standby(); //Entry Standby mode
      break;
    }
  }
}
void sx1276_7_8_Config(void) {
  unsigned char i;

  sx1276_7_8_Sleep(); //Change modem mode Must in Sleep mode
  for (i = 250; i != 0; i--); //Delay
  delay(15);

  //lora mode
  sx1276_7_8_EntryLoRa();
  //SPIWrite(0x5904);   //Change digital regulator form 1.6V to 1.47V: see errata note

  //setting frequency parameter
  BurstWrite(LR_RegFrMsb, sx1276_7_8FreqTbl[Freq_Sel], 3);

  //setting base parameter
  SPIWrite(LR_RegPaConfig, sx1276_7_8PowerTbl[Power_Sel]);    //Setting output power parameter

  SPIWrite(LR_RegOcp, 0x0B); //RegOcp,Close Ocp，0000 1011
  SPIWrite(LR_RegLna, 0x23);   //RegLNA,High & LNA Enable

  if (sx1276_7_8SpreadFactorTbl[Lora_Rate_Sel] == 6) //SFactor=6
  {
    unsigned char tmp;

    SPIWrite(LR_RegModemConfig1, ((sx1276_7_8LoRaBwTbl[BandWide_Sel] << 4) + (CR << 1) + 0x01));//Implicit Enable CRC Enable(0x02) & Error Coding rate 4/5(0x01), 4/6(0x02), 4/7(0x03), 4/8(0x04)
    SPIWrite(LR_RegModemConfig2, ((sx1276_7_8SpreadFactorTbl[Lora_Rate_Sel] << 4) + (CRC << 2) + 0x03));

    tmp = SPIRead(0x31);
    tmp &= 0xF8;//1111 1000
    tmp |= 0x05;//0000 0101
    SPIWrite(0x31, tmp);//0011 0001，
    SPIWrite(0x37, 0x0C);//0011 0111，0000 1100
  }
  else
  {
    SPIWrite(LR_RegModemConfig1, ((sx1276_7_8LoRaBwTbl[BandWide_Sel] << 4) + (CR << 1) + 0x00));//Explicit Enable CRC Enable(0x02) & Error Coding rate 4/5(0x01), 4/6(0x02), 4/7(0x03), 4/8(0x04)
    SPIWrite(LR_RegModemConfig2, ((sx1276_7_8SpreadFactorTbl[Lora_Rate_Sel] << 4) + (CRC << 2) + 0x03)); //SFactor & LNA gain set by the internal AGC loop
  }
  SPIWrite(LR_RegSymbTimeoutLsb, 0xFF); //RegSymbTimeoutLsb  Timeout = 0x3FF(Max)
  SPIWrite(LR_RegPreambleMsb, 0x00); //RegPreambleMsb
  SPIWrite(LR_RegPreambleLsb, 12);   //RegPreambleLsb  8 + 4 = 12byte Preamble
  SPIWrite(REG_LR_DIOMAPPING2, 0x01); //RegDioMapping2 DIO5=00, DIO4=01

  sx1276_7_8_Standby(); //Entry standby mode
}
void set_default_parameter() {
  mode          = def_mode;//lora mode
  Freq_Sel      = def_Freq_Sel;//433M
  Power_Sel     = def_Power_Sel;//
  Lora_Rate_Sel = def_Lora_Rate_Sel;//SF
  BandWide_Sel  = def_BandWide_Sel;
  sx1276_7_8_Config();
}

void sendtoesp(String cmd) { //send cmd to esp8266
  Serial.println("SEND: " + cmd);
  Serial3.print(cmd + "\r\n");
}
void test_esp() {
  esp_com_timeout = 0;
  do {
    if ((millis() - esp_com_timeout) / 1000 > 5 || esp_com_timeout == 0) {
      esp_com_timeout = millis();
      sendtoesp("AT");
    }
  } while (!Serial3.find("OK") );
  Serial.println("AT OK");
}
void Check_WiFi() {
  sendtoesp("AT+CWJAP?");
  esp_com_timeout = millis();
  esp_com_error_times = 0;

  while (1) {
    //delay(500);
    if (Serial3.available() > 0) {
      if (Serial3.find("No")) {
        Serial.println("State: No AP");
        sendtoesp("AT+CWJAP_DEF=\"" + SSID + "\",\"" + PASS + "\"");
        esp_com_timeout = millis();
        while (1) {
          if (Serial3.find("OK")) {
            Serial.println("State: Connect WiFi OK");
            break;
          }

          else if (Serial.find("FAIL")) {
            Serial.println("State: Connect WiFi Fail");
          }

          if ((millis() - esp_com_timeout) / 1000 >= 10) { //timeout 10s
            esp_com_timeout = millis();
            sendtoesp("AT+CWJAP_DEF=\"" + SSID + "\",\"" + PASS + "\"");
            esp_com_error_times++;
          }
          if (esp_com_error_times >= 5) {
            Serial.println("State: Can't connect to wifi\nNo such WiFi");
          }
        }
      }
      break;
    }
    else if ((millis() - esp_com_timeout) / 1000 >= 5) {
      sendtoesp("AT+CWJAP?");
      esp_com_error_times++;
      esp_com_timeout = millis();
    }
    if (esp_com_error_times >= 5)
      break;
  }
}
void build_TCP() {
  esp_com_error_times = 0;
  esp_com_timeout = 0;
  do {
    if ((millis() - esp_com_timeout) / 1000 >= 5 || esp_com_timeout == 0) {
      esp_com_timeout = millis();
      sendtoesp("AT+CIPSTART=\"TCP\",\"" + IP + "\",80");
      esp_com_error_times++;
    }

    if (esp_com_error_times >= 5) {
      Check_WiFi();
      esp_com_error_times = 0;
      Serial.println("State: Break check wifi");
    }
  } while (!Serial3.find("CONN"));
}
void Rxdata_to_Update_command() {
  Update_command = GET;
  switch (           (RxData[10] << 8) + RxData[11]             ) { //source_id
    case 1:
      GET = "GET /update?key=9UVXFOCNVISHFZ2O";
      break;
    case 2:
      GET = "GET /update?key=L9S8KM83QS87U8RC";
      break;
    case 3:
      GET = "GET /update?key=LJCWWW71KYAJX0T6";
      break;
    case 5:
      GET = "GET /update?key=I9PO0ECSYXHQODQU";
      break;
    case 6:
      GET = "GET /update?key=XWBRWE8W9TDN4F4O";
      break;
    case 7:
      GET = "GET /update?key=YLVQMK9J3WUMYFBS";
      break;
    case 8:
      GET = "GET /update?key=7ZE706RFQMP4836O";
      break;
  }

  Update_command += "&field1=" + (String)RxData[13];
  Update_command += "&field2=" + (String)RxData[14];
  Update_command += "&field3=" + (String)RxData[15];
  Update_command += "&field4=" + (String)LoRaRSSI;
  Update_command += "\r\n";
}
void send_update_command() {
  Serial.print(">" + Update_command);
  esp_com_timeout = 3;
  while (1) {
    sendtoesp("AT+CIPSEND=" + (String)Update_command.length());
    delay(500);
    Serial3.print(Update_command);
    delay(500);
    esp_com_timeout--;

    if (Serial3.find("IPD") || esp_com_timeout <= 0) {
      Serial.println("State: Successfully update to Thingspeak.");
      break;
    }
  }
}
void ask_every_device_data_and_update() {
  Check_WiFi();
  for (j = 0; j < Child_Number; j++) {
    ID = Child_table[j];
    Serial.println("State: Ask Node: " + (String)ID + " sensor data");
    make_coap_packet_header_and_ID(CoAP_Get_Header, ID, ASK_SENSOR_DATA);

    sx1276_7_8_LoRaEntryTx();
    sx1276_7_8_LoRaTxPacket();
    sx1276_7_8_LoRaEntryRx();
    RxPacket_timeout = millis();
    while (1) {
      if ((millis() - RxPacket_timeout) / 1000 >= 10) {
        Serial.println("State: " + (String)ID + " no response");
        break;
      }

      if (sx1276_7_8_LoRaRxPacket()) {
        switch_packet_type();
        break;
      }
    }
    Serial.println();
  }
  every_60s_send_to_Thingspeak = millis();
  Serial.println("State: Gate way ask sensor data finish.");
}

void send_parameter_order(unsigned char type, unsigned char parameter1, unsigned char parameter2) {
  make_coap_packet_header_and_ID(CoAP_Get_Header, 0xffff, LORA_PARAMETER_CHANGE);
  sx1276_7_8Data[10] = type;
  sx1276_7_8Data[11] = parameter1;
  sx1276_7_8Data[12] = parameter2;

  sx1276_7_8_LoRaEntryTx();
  sx1276_7_8_LoRaTxPacket();
  sx1276_7_8_LoRaEntryRx();
}
void commit_parameter_order() {
  sx1276_7_8Data[9] = LORA_PARAMETER_COMMIT;
  sx1276_7_8_LoRaEntryTx();
  sx1276_7_8_LoRaTxPacket();
  sx1276_7_8_LoRaEntryRx();

  RxPacket_timeout = millis();
  Serial.println("State: Commit parameter order listening");
  while ((millis() - RxPacket_timeout) / 1000 < (Child_Number + 1) * TMDA_TIME) //Commit listen timeout
    if (sx1276_7_8_LoRaRxPacket())
      switch_packet_type();
  Serial.println("State: Commit_parameter_order finish");
}
void switch_at_command_type() {
  AT_Command = Serial.readString();
  Serial.println("User key in: AT+" + AT_Command);
  if (AT_Command.substring(       0, AT_Command.indexOf('=') + 1) == "FRE=") {
    long long_fre = 100 * (AT_Command[4] - 48) + 10 * (AT_Command[5] - 48) + (AT_Command[6] - 48);
    send_parameter_order(AT_TYPE_FRE, (byte)(long_fre >> 8), (byte)long_fre);
    for (i = 0; i < 14; i++)
      long_fre *= 2;
    sx1276_7_8FreqTbl[1][2] = (byte)long_fre;
    sx1276_7_8FreqTbl[1][1] = (byte)(long_fre >> 8);
    sx1276_7_8FreqTbl[1][0] = (byte)(long_fre >> 16);
    Freq_Sel = 0x01;
  }
  else if (AT_Command.substring(  0, AT_Command.indexOf('=') + 1) == "POW=") {
    send_parameter_order(AT_TYPE_POW, AT_Command[4] - 48, 0);
    Power_Sel = AT_Command[3] - 48;
  }
  else if (AT_Command.substring(  0, AT_Command.indexOf('=') + 1) == "SF=") {
    send_parameter_order(AT_TYPE_SF, AT_Command[3] - 48, 0);
    Lora_Rate_Sel = AT_Command[3] - 48;
  }
  else if (AT_Command.substring(  0, AT_Command.indexOf('=') + 1) == "BW=") {
    send_parameter_order(AT_TYPE_BW, AT_Command[3] - 48, 0);
    BandWide_Sel = AT_Command[3] - 48;
  }
  else if (AT_Command.substring(  0, AT_Command.indexOf('=') + 1) == "RST=") {
    if (AT_Command.indexOf('\r') - AT_Command.indexOf('=') == 2) {// id length=1
      if (AT_Command[AT_Command.indexOf('=') + 1] == '0') {//gatewat restart
        asm volatile ("  jmp 0");
      }
      else {
        send_parameter_order(AT_TYPE_RST, AT_Command[AT_Command.indexOf('=') + 1] - 48, 0);
      }
    }
    else if (AT_Command.indexOf('\r') - AT_Command.indexOf('=') == 3) {// id length=2
      send_parameter_order(AT_TYPE_RST, 10 * (AT_Command[AT_Command.indexOf('=') + 1] - 48) + (AT_Command[AT_Command.indexOf('=') + 2] - 48), 0);
    }
    //send_parameter_order(AT_TYPE_RST, AT_Command[4] - 48, 0);
  }
  else if (AT_Command.substring(  0, AT_Command.indexOf('=') + 1) == "TIM=") {}
  else
    Serial.println("State: NO this command");

  /*Listen change ack*/
  Serial.println("State: Listen node ack change parameter");
  Change_Parameter_Commit_Timeout = millis();
  while ((millis() - Change_Parameter_Commit_Timeout) / 1000 < (Child_Number + 1) * TMDA_TIME)
    if (sx1276_7_8_LoRaRxPacket())
      switch_packet_type();

  /*Change LoRa Parameter*/
  Serial.println("State: Change LoRa parmeter");
  sx1276_7_8_LoRaEntryTx();
  Serial.println("State: Entry commit_parameter_order");
  commit_parameter_order();
}
void make_coap_packet_header_and_ID(unsigned char header[5], uint16_t target_id, unsigned char Packet_type) {
  int i;
  for (i = 0; i < 5; i++)//sizeof(header)
    sx1276_7_8Data[i] = header[i];
  Message_ID = analogRead(A0) * 100 ;
  sx1276_7_8Data[2] = (byte)(Message_ID >> 8);
  sx1276_7_8Data[3] = (byte)Message_ID;
  sx1276_7_8Data[5] = (byte)(target_id >> 8);
  sx1276_7_8Data[6] = (byte)target_id;
  sx1276_7_8Data[7] = (byte)(Device_ID >> 8);
  sx1276_7_8Data[8] = (byte)Device_ID;
  sx1276_7_8Data[9] = Packet_type;
}
void send_beacon() {
  Serial.println("State: Send_beacon");
  make_coap_packet_header_and_ID(CoAP_Get_Header, 0xffff, GATEWAY_BEACON);
  sx1276_7_8_LoRaEntryTx();
  sx1276_7_8_LoRaTxPacket();
  sx1276_7_8_LoRaEntryRx();
  no_child_every_10s_send_beacon = millis();
}
void accept_join_packet() {
  Serial.println("RECV: ID: " + (String)RxData[11] + " join packet");
  for (i = 0; i < Child_Number; i++) {
    if ( (RxData[10] << 8) + RxData[11] == Child_table[i]) {
      Serial.println("State: ID: " + (String)RxData[11] + " has joined");
      send_accept_join_packet(i);
      break;
    }
  }

  if (i == Child_Number) {
    Child_table[Child_Number] = (RxData[10] << 8) + RxData[11];
    send_accept_join_packet(i);
    Child_Number++;
  }
}
void send_accept_join_packet(int child_table_index) {
  make_coap_packet_header_and_ID(CoAP_Get_Header, Child_table[child_table_index], GATE_ACCEPT_JOIN);
  sx1276_7_8_LoRaEntryTx();
  sx1276_7_8_LoRaTxPacket();
  sx1276_7_8_LoRaEntryRx();
}

int Check_ID() {
  if ( ((RxData[8] << 8) + RxData[9]) == Device_ID) //Device_id 0 is gateway
    return (1);

  else {
    Serial.println("State:None of my business");
    return (0);
  }
}
void switch_packet_type() {
  switch (RxData[12]) {
    case ACK_SENSOR_DATA:
      sx1276_7_8_LoRaReadRSSI();
      Rxdata_to_Update_command();
      build_TCP();
      send_update_command();
      break;

    case LORA_PARAMETER_CHANGE_ACK://好像沒用
      break;

    case LORA_PARAMETER_COMMIT_ACK://好像沒用
      break;

    case END_DEVICE_JOIN:
      accept_join_packet();
      break;

    default:
      Serial.println("State: Packet Type= " + (String)RxData[12] + ".\tNo such type.");
      break;
  }
}
void setup() {
  for(i=22;i<54;i++){
    pinMode(i,INPUT);
    //digitalWrite(i, LOW);
  }
  
  pinMode(nsel, OUTPUT);
  pinMode(sck,  OUTPUT);
  pinMode(mosi, OUTPUT);
  pinMode(miso, INPUT);
  Serial.begin(Serial_baudrate);
  Serial3.begin(esp_baudrate);
  test_esp();
  Serial.println("State: Set_default_parameter");
  set_default_parameter();
  send_beacon();

  every_60s_send_to_Thingspeak = millis();
}
void loop() {
  if ( (millis() - no_child_every_10s_send_beacon) / 1000 >= 10 && Child_Number == 0)
    send_beacon();

  if ( ((millis() - every_60s_send_to_Thingspeak) / 1000 >= 20) && Child_Number >= 1)
    ask_every_device_data_and_update();

  if (sx1276_7_8_LoRaRxPacket())
    switch_packet_type();

  if (Serial.find("AT+"))
    switch_at_command_type();
}
