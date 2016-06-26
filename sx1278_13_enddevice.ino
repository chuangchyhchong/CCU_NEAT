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

#define END_DEVICE_NUMBER 0x02
#define TMDA_TIME 0x02

/*default lora parameter*/
#define def_mode          0x01
#define def_Freq_Sel      0x00
#define def_Power_Sel     0x00
#define def_Lora_Rate_Sel 0x06
#define def_BandWide_Sel  0x09
/*********Pinout*****************************************/
int dio0 = 12;
int nsel = 11;
int sck  = 10;
int mosi = 9;
int miso = 8;
int analog_random_pin = A0;

unsigned char sx1276_7_8Data[20] = {0};
unsigned char CoAp_Ack_Header[8] = {0x60, 0x45, 0x00, 0x00, 0xc2, 0x00, 0x00, 0xff};
unsigned char RxData[64];
uint8_t Device_ID = 3;//0: gateway，1: all end_device,2~: end_device
uint16_t Message_ID;
String LoRaRSSI;
long No_Any_Packet_Set_Dedault_parameter = 0;
int join_state = 0;
unsigned char parent;
int Check_ID_Result;
int i;

/*PMS5003*/
#include <SoftwareSerial.h>//這個library 同時只能begin一個serial port
SoftwareSerial pms(4, 5);//Digital_4 as Rx ,Digital_5 as Tx
#define pms_baudrate  9600
/*LCM1602*/
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);//addr,en,rw,rs,d4,d5,d6,d7,bl, blpol
/*DHT22*/
#include "DHT.h"
DHT dht(13, DHT22);//pinout,type

uint8_t int_pms5003[2];
unsigned long every_60s_update_sensor_data;

/*********Parameter table define**************************/
unsigned char mode;//lora--1/FSK--0
unsigned char Freq_Sel;
unsigned char Power_Sel;
unsigned char Lora_Rate_Sel;
unsigned char BandWide_Sel;
unsigned char Fsk_Rate_Sel;
unsigned char sx1276_7_8FreqTbl[2][3] = {
  {0x6B, 0x00, 0x00}, {0x00, 0x00, 0x00} //430 MHz
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

  SPIWrite(LR_RegPayloadLength, 21); //RegPayloadLength 21byte(this register must difine when the data long of one byte in SF is 6)

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
  unsigned char addr;
  unsigned char packet_size;
  
  if (digitalRead(dio0)) //if(Get_NIRQ())
  {
    for (i = 0; i < sizeof(RxData); i++)
      RxData[i] = 0x00;

    addr = SPIRead(LR_RegFifoRxCurrentaddr);//last packet addr
    SPIWrite(LR_RegFifoAddrPtr, addr);//RxBaseAddr ->   FiFoAddrPtr
    if (sx1276_7_8SpreadFactorTbl[Lora_Rate_Sel] == 6) //When SpreadFactor is six，will used Implicit Header mode(Excluding internal packet length)
      packet_size = 21;
    else
      packet_size = SPIRead(LR_RegRxNbBytes); //Number for received bytes
    SPIBurstRead(0x00, RxData, packet_size);
    
    if ( RxData[0]==0x40 || RxData[0]==0x60) {
      Serial.print("RECV:");
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
  mode = def_mode;//lora mode
  Freq_Sel = def_Freq_Sel;
  Power_Sel = def_Power_Sel;
  Lora_Rate_Sel = def_Lora_Rate_Sel;
  BandWide_Sel = def_BandWide_Sel;
  sx1276_7_8_Config();
}

void lcd_print(String Str, int column, int row) { //print word on lcd,column 0~15,row 0~1
  lcd.setCursor(column, row);
  lcd.print(Str);
}
void update_sensor_data() {
  //PMS5005
  long pms_timeout = millis();
  pms.begin(pms_baudrate);
  while (millis() - pms_timeout < 60 * 1000) {
    pms.readBytes(int_pms5003, 2);
    if (int_pms5003[0] == 0x42 || int_pms5003[1] == 0x4d) { //尋找每段資料的開頭
      for (i = 0; i < 6; i++)
        pms.readBytes(int_pms5003, 2);//pms3003 is int type.
      break;
    }
  }
  pms.end();
  //TEMP & HUMI
  String print_on_Serial_Monitor = "Temp:" + (String)dht.readTemperature() + "C Humi:" + (String)dht.readHumidity() + "% PM2.5:" + (String)int_pms5003[1] + "ug/m^3";
  Serial.println(print_on_Serial_Monitor);
}
void print_to_lcd() {
  lcd.clear();
  lcd_print("PM2.5: " + (String)int_pms5003[1] + "ug/m3"         , 0, 0);
  lcd_print("Temp:"  + (String)dht.readTemperature() , 0, 1);
  lcd_print("  Hmui:" + (String)dht.readHumidity()    , 7, 1);
}
void update_coap_packet_data() {
  sx1276_7_8Data[13] = (int)dht.readTemperature();
  sx1276_7_8Data[14] = (int)dht.readHumidity();
  sx1276_7_8Data[15] = (int)int_pms5003[1];
}
void do_AT_command() {
  if (RxData[10] == AT_TYPE_FRE) {
    long long_fre = (RxData[11] << 8) + RxData[12];
    Serial.println("Frequency -> " + (String)long_fre);
    for (i = 0; i < 14; i++)
      long_fre *=  2;
    sx1276_7_8FreqTbl[1][2] = (byte)long_fre;
    sx1276_7_8FreqTbl[1][1] = (byte)(long_fre >> 8);
    sx1276_7_8FreqTbl[1][0] = (byte)(long_fre >> 16);
    Freq_Sel = 0x01;
  }
  else if (RxData[10] == AT_TYPE_POW  ) {
    Serial.println("Power_Sel -> " + (String)RxData[11]);
    Power_Sel = RxData[11];
  }
  else if (RxData[10] == AT_TYPE_SF  ) {
    Serial.println("Lora_Rate_Sel -> " + (String)RxData[11]);
    Lora_Rate_Sel = RxData[11];
  }
  else if (RxData[10] == AT_TYPE_BW  ) {
    Serial.println("BandWide_Sel -> " + (String)RxData[11]);
    BandWide_Sel = RxData[11];
  }
  else if (RxData[10] == AT_TYPE_RST  ) {
    if ((RxData[11] == 0xff && RxData[12] == 0xff) || ((RxData[11] << 8) + (RxData[12])) == Device_ID)
      asm volatile ("  jmp 0");
  }
  else if (RxData[10] == AT_TYPE_TIMEOUT  ) {}
  else
    Serial.println("No such AT ommand type.");

  Serial.println("Change LoRa parameter\nsx1278 Config");
  sx1276_7_8_LoRaEntryRx();
}
void send_join_packet() {
  make_coap_packet_header_and_ID();
  //Still change something in header
  Message_ID = analogRead(A0) * 100 ;
  sx1276_7_8Data[2] = (byte)(Message_ID >> 8);
  sx1276_7_8Data[3] = (byte)Message_ID;
  sx1276_7_8Data[12] = END_DEVICE_JOIN;
  //delay and send join_packet to gateway
  delay(analogRead(A0) * Device_ID * 5);
  Serial.println("send_join_packet");
  sx1276_7_8_LoRaEntryTx();
  sx1276_7_8_LoRaTxPacket();
  sx1276_7_8_LoRaEntryRx();
}
void make_coap_packet_header_and_ID() {
  for (i = 0; i < sizeof(CoAp_Ack_Header); i++)
    sx1276_7_8Data[i] = CoAp_Ack_Header[i];
  sx1276_7_8Data[2] = RxData[2];
  sx1276_7_8Data[3] = RxData[3];
  sx1276_7_8Data[8] = 0x00;//RxData[7];//source id -> target id
  sx1276_7_8Data[9] = 0x00;//RxData[8];
  sx1276_7_8Data[10] = (byte)(Device_ID >> 8);//Device id -> source id
  sx1276_7_8Data[11] = (byte)Device_ID;
}

int Check_ID() {
  uint16_t temp_id = (RxData[5] << 8)+RxData[6];
  uint16_t temp_header= (RxData[0]<<8)+RxData[1];
  if (    join_state == 0 && (temp_header==0x6045 || temp_header==0x4001)){
    Serial.println("From "+(String)((RxData[7] << 8)+RxData[8])+" to " +(String)temp_id);
    return (1);
  }
  else if (join_state == 1) {
    Serial.println("From "+(String)((RxData[7] << 8)+RxData[8])+" to " +(String)temp_id);
    if (temp_id == Device_ID || temp_id == 0xffff)//for me or for all
      return (2);
    else
      return (0);
  }
}
void listen_and_join_network(){
  if(join_state==0){
    switch(RxData[9]){
      case ASK_SENSOR_DATA:
        Serial.println("Recv: ASK_SENSOR_DATA packet");
        send_join_packet();
        break;
      case GATEWAY_BEACON:
        Serial.println("Recv: GATEWAY_BEACON packet");
        send_join_packet();
        break;
      case GATE_ACCEPT_JOIN:
        join_state=1;
        Serial.println("Join gateway accept");
        break;
    }
  }
}
void switch_packet_type() {
  make_coap_packet_header_and_ID();
  switch (RxData[9]) {
    case ASK_SENSOR_DATA:
      Serial.println("RECV: ASK_SENSOR_DATA packet");
      sx1276_7_8Data[12] = ACK_SENSOR_DATA;
      update_sensor_data();
      update_coap_packet_data();
      sx1276_7_8_LoRaEntryTx();
      sx1276_7_8_LoRaTxPacket();
      sx1276_7_8_LoRaEntryRx();
      print_to_lcd();
      break;
    case LORA_PARAMETER_CHANGE:
      Serial.println("RECV: LORA_PARAMETER_CHANGE packet");
      sx1276_7_8Data[12] = LORA_PARAMETER_CHANGE_ACK;
      sx1276_7_8Data[13] = RxData[10];
      sx1276_7_8Data[14] = RxData[11];
      sx1276_7_8Data[15] = RxData[12];
      delay(analogRead(A0) * Device_ID * 10);
      sx1276_7_8_LoRaEntryTx();
      sx1276_7_8_LoRaTxPacket();
      do_AT_command();
      break;
    case LORA_PARAMETER_COMMIT://0x17
      Serial.println("Recv: LORA_PARAMETER_COMMIT packet");
      delay(analogRead(A0) * Device_ID * 10);
      sx1276_7_8Data[12] = LORA_PARAMETER_COMMIT_ACK;
      sx1276_7_8_LoRaEntryTx();
      sx1276_7_8_LoRaTxPacket();
      sx1276_7_8_LoRaEntryRx();
      break;
    case GATEWAY_BEACON:
      Serial.println("Recv: GATEWAY_BEACON packet");
      send_join_packet();
      break;
    case GATE_ACCEPT_JOIN:
      join_state=1;
      Serial.println("Join gateway accept");
      break;
  }
}
void setup() {
  pinMode(nsel, OUTPUT);
  pinMode(sck,  OUTPUT);
  pinMode(mosi, OUTPUT);
  pinMode(miso, INPUT);
  Serial.begin(9600);
  dht.begin();
  lcd.begin(16, 2);

  lcd_print("Set Def Parmeter", 0, 0);
  set_default_parameter();
  lcd_print("Entry Rx       ", 0, 0);
  sx1276_7_8_LoRaEntryRx();

  Serial.println("Device_ID:" + (String)Device_ID + " ");
  lcd_print("update sensor   ", 0, 0);
  lcd_print("data            ", 0, 1);
  update_sensor_data();
  print_to_lcd();
}
void loop() {
  switch(sx1276_7_8_LoRaRxPacket()){
    case 1:
      listen_and_join_network();
      No_Any_Packet_Set_Dedault_parameter = millis();
      break;
    case 2:
      switch_packet_type();
      No_Any_Packet_Set_Dedault_parameter = millis();
      Serial.println();
      break;
  }
  if ((millis() - No_Any_Packet_Set_Dedault_parameter)/60000 >= 30)
    asm volatile ("  jmp 0");
}
