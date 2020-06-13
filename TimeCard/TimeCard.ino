#include <M5Atom.h>
#include <BleKeyboard.h>
#include <RCS620S.h>
#include <VL53L0X.h>
#include <Wire.h>

#define TIMEOUT 400
#define POLLING_INTERVAL 1000
#define CALIBRATION 5.8       //体温校正値

BleKeyboard bleKeyboard;  //BLEキーボード
VL53L0X distSensor;       //距離センサー
RCS620S rcs620s;          //NFCリーダー

uint16_t result;
uint16_t dist;
float temperature;

void setup() {
  int ret;

  //M5Atom初期化
  M5.begin(true,false,true);  //シリアル有効,I2C無効（内部IMU無効）,ディスプレイ有効
  
  //シリアルモニター初期化
  Serial.begin(115200);

  //BLEキーボード初期化
  Serial.println();
  Serial.println("Starting BLE work!");
  bleKeyboard.begin();

  //NFCリーダー用UART初期化（21->RXD,25->TXD）
  Serial1.begin(115200,SERIAL_8N1,21,25);

  //NFCリーダー初期化
  ret=rcs620s.initDevice();

  delay(10);
  
  Serial.println("Init RCS620s ");
  
  while(!ret) {
    delay(100);
    Serial.print(".");
  };
  
  Serial.println(".");
  Serial.println("Connect RCS620S");
  rcs620s.timeout=TIMEOUT;

  //I2C初期化（Groveコネクタ）
  Wire.begin(26,32);

  //距離センサー初期化
  distSensor.init();
  distSensor.setTimeout(1000);
  distSensor.startContinuous(10);

  //LED初期化
  for (int i=0;i<25;i++){
    M5.dis.drawpix(i,0x000000);
  }

  delay(3000);

}

void loop() {
  String uid;

  //NFC読取 TypeAカードとFeliCaに対応
  if (rcs620s.polling() || rcs620s.polling_typeA()) {
    Serial.print("ID: ");
    
    for ( int i=0;i<rcs620s.idLength;i++) {
      uid=uid+String(rcs620s.idm[i],HEX);
    }
    
    Serial.println(uid);

    //IDをキー入力として送出
    if(bleKeyboard.isConnected()) {
      bleKeyboard.println(uid);
      delay(500);
    }
  }
  
  rcs620s.rfOff();
  //delay(POLLING_INTERVAL);

  //距離測定
  dist=distSensor.readRangeSingleMillimeters();

  //距離によってLEDの色を変更
  if (dist<100) {
    //100mm未満は赤
    for (int i=0;i<25;i++){
      M5.dis.drawpix(i,0x00ff00);
    }
  }
  else if (dist>=100 && dist<120){
    //100mmから120㎜は青
    for (int i=0;i<25;i++){
      M5.dis.drawpix(i,0x0000ff);
    }
    //温度を測定
    Wire.beginTransmission(0x5A);
    Wire.write(0x07);
    Wire.endTransmission(false);
    Wire.requestFrom(0x5A,2);

    result=Wire.read();
    result |=Wire.read()<<8;

    temperature=(result*0.02-273.15)+CALIBRATION;

    Serial.print("  Temp: ");
    Serial.println(temperature);

    //体温をキー入力として送出
    if(bleKeyboard.isConnected()) {
      bleKeyboard.println(temperature);
    }
    delay(800);
  }
  else if (dist>=120 && dist<200) {
    //120mmから200㎜は黄
    for (int i=0;i<25;i++){
      M5.dis.drawpix(i,0xffff00);
    }
  }
  else {
    //それ以上はLEDをOFF
    for (int i=0;i<25;i++){
      M5.dis.drawpix(i,0x000000);
    }
  }

  Serial.print("Distance: ");
  Serial.println(dist);

  delay(POLLING_INTERVAL);
}
