#define LOAD_FONT2
#define LOAD_FONT4
#include <M5Stack.h>
#include "utility/MPU9250.h"
#include <WiFi.h>
#include <WiFiUdp.h>
//
// original code from https://qiita.com/Mitsu-Murakita/items/b86ad79d3590adb3b5b9
//
// TELLOのSSID
const char* TELLO_SSID = "TELLO-EE817F";  // 自分のTelloのWi-Fi SSIDを入力
// TELLOのIP
const char* TELLO_IP = "192.168.10.1";
// TELLO_PORT
const int PORT = 8889;
// UDPまわり
WiFiUDP Udp;
char packetBuffer[255];
String message = "";

MPU9250 IMU;

float x;
float y;
char msgx[6];
char msgy[6];
String status_msg;
void setup() {
  //M5Stackの初期設定
  M5.begin();
  M5.Power.begin();
  //画面表示
  //---タイトル
  M5.Lcd.fillRect(0,0,320,30,TFT_BLUE);
  M5.Lcd.drawCentreString("Tello Controller",160,2,4);
  //---X, Yの表示
  M5.Lcd.setTextColor(TFT_YELLOW,TFT_BLACK);
  M5.Lcd.drawCentreString(" Accel-X : ",20,30,2);
  sprintf(msgx,"%-2.2f",x);
  M5.Lcd.drawCentreString(msgx,88,30,2);
  //---X, Y値の表示
  M5.Lcd.drawCentreString(" Accel-Y : ",240,30,2);
  sprintf(msgy,"%-2.2f",y);
  M5.Lcd.drawCentreString(msgy,294,30,2);
  //---ボタンエリア
  M5.Lcd.fillRect(0,217,320,20,TFT_LIGHTGREY);
  //---ボタン文字
  M5.Lcd.setTextColor(TFT_BLACK,TFT_YELLOW);
  M5.Lcd.drawCentreString(" TAKE OFF ",64,220,2);
  M5.Lcd.setTextColor(TFT_BLACK,TFT_RED);
  M5.Lcd.drawCentreString(" LANDING  ",250,220,2);
  M5.Lcd.setTextColor(TFT_BLACK,TFT_CYAN);
  M5.Lcd.drawCentreString(" ROTATION ",160,220,2);
  //---方向矢印
  M5.Lcd.fillTriangle(159,40,189,60,129,60,TFT_GREEN);
  M5.Lcd.fillTriangle(159,160,189,140,129,140,TFT_GREEN);
  M5.Lcd.fillTriangle(269,100,220,80,220,120,TFT_GREEN);
  M5.Lcd.fillTriangle(98,80,98,120,49,100,TFT_GREEN);
  //---方向の文字
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.drawCentreString("FORWARD",160,64,2);
  M5.Lcd.drawCentreString("BACK",160,120,2);
  M5.Lcd.drawCentreString("LEFT",120,92,2);
  M5.Lcd.drawCentreString("RIGHT",200,92,2);
  //---FLIP文字
  M5.Lcd.setTextColor(TFT_YELLOW,TFT_BLACK);
  M5.Lcd.drawCentreString("FLIP: ",60,140,1);
  M5.Lcd.drawCentreString("ButtonA + Direction",50,150,1);
  //---UP/DOWN文字
  M5.Lcd.setTextColor(TFT_YELLOW,TFT_BLACK);
  M5.Lcd.drawCentreString("UP/DOWN: ",280,140,1);
  M5.Lcd.drawCentreString("ButtonC + FORWARD",280,150,1);
  M5.Lcd.drawCentreString("Up/Down",300,160,1);
  //---メッセージ領域
  M5.Lcd.drawRoundRect(0,180,319,30,4,TFT_WHITE);
  //---メッセージのタイトル文字
  M5.Lcd.setTextColor(TFT_WHITE,TFT_DARKGREEN);
  M5.Lcd.drawCentreString("<Message>",38,170,1);
  read_batt();
  //---メッセージの文字
  //M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  //M5.Lcd.drawString(msg,4,190,1);
  //初期設定
  //Wireライブラリを初期化
  Wire.begin();
  //MPU9250を初期化
  IMU.initMPU9250();
  //WiFi通信の開始
  WiFi.begin(TELLO_SSID, "");
  //WiFi接続　接続するまでループ
  while (WiFi.status() != WL_CONNECTED) {
        print_msg("Now, WiFi Connecting......");
        delay(500);
  }
  print_msg("WiFi Connected.");
  // UDP
  Udp.begin(PORT);
  //Telloへ”command”送信
  print_msg("sendMessage command");
  tello_command_exec("command");
  tello_command_exec("battery?");
}

void loop() {
  // put your main code here, to run repeatedly:
  //x,y値の取得と表示
  if (IMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01){
    IMU.readAccelData(IMU.accelCount);
    IMU.getAres();
    x = IMU.accelCount[0] * IMU.aRes;
    y = IMU.accelCount[1] * IMU.aRes;
    sprintf(msgx,"%-2.2f",x);
    M5.Lcd.drawCentreString("      ",88,30,2);
    M5.Lcd.drawCentreString(msgy,88,30,2);
    sprintf(msgy,"%-2.2f",y);
    M5.Lcd.drawCentreString("      ",294,30,2);
    M5.Lcd.drawCentreString(msgy,294,30,2);
    print_msg("Operation Start!"); 

    //ボタンA処理
    //0.3は実測値から閾値を設定した
    if(M5.BtnA.wasPressed()) {
      //Telloフリップと離陸
      if (fabs(x)> 0.3){
          if (x > 0){
            //フリップ左
            print_msg("LEFT FLIP"); 
            tello_command_exec("flip l");
          }
          if (x < 0){
            //フリップ右
            print_msg("RIGHT FLIP"); 
            tello_command_exec("flip r");
          }
      }
      if (fabs(y)> 0.3){
          if (y > 0){
            //フリップ後
            print_msg("BACK FLIP"); 
            tello_command_exec("flip b");
          }
          if (y < 0){
            //フリップ前
            print_msg("FRONT FLIP"); 
            tello_command_exec("flip f");
          }
      }else{
        //離陸
        print_msg("TAKE OFF"); 
        tello_command_exec("takeoff");
      }    
    }

    //ボタンB処理
    //時計回り45度回転
    if(M5.BtnB.wasPressed()) {
        print_msg("CW");
        tello_command_exec("cw 45");
    }

    //ボタンC処理    
    if(M5.BtnC.wasPressed()) {
      //着陸
      if (fabs(y)> 0.3){
        //上昇50cm
        if (y > 0){
          tello_command_exec("up 50");
        }
        //下降50cm
        if (y < 0){
          print_msg("DOWN");
          tello_command_exec("down 50");
        }
      }else{
        //着陸
        print_msg("LAND");
        tello_command_exec("land");
      }
    }

    //tello移動
    if (fabs(x)> 0.3){
        //左移動50cm
        if (x > 0){
          print_msg("LEFT");
          tello_command_exec("left 30");
        }
        //右移動50cm
        if (x < 0){
          print_msg("RIGHT");
          tello_command_exec("right 30");
        }
    }
    if (fabs(y)> 0.3){
        //後退50cm
        if (y > 0){
          print_msg("BACK");
          tello_command_exec("back 30");
        }
        //前進50cm
        if (y < 0){
          print_msg("FRONT");
          tello_command_exec("forward 30");
        }
    }
    delay(150);
  }
  M5.update();
  read_batt();
}

/////////////////////////////
//      ユーザ関数定義       //
/////////////////////////////

// 画面メッセージエリアへ状況メッセージ表示
void print_msg(String status_msg){
  M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
  M5.Lcd.drawString("                          ",4,190,1);
  M5.Lcd.drawString(status_msg,4,190,1);
  status_msg="";
}

// Telloへメッセージ送信＆コマンド実行
void tello_command_exec(char* tello_command){
  Udp.beginPacket(TELLO_IP, PORT);
  Udp.printf(tello_command);
  Udp.endPacket();
  Serial.println(tello_command);
  message = listenMessage();
  Serial.println("tello : " + message);
  delay(100);
}

// Telloからのメッセージ受信
String listenMessage() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    IPAddress remoteIp = Udp.remoteIP();
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
  }
  delay(100);
  return (char*) packetBuffer;
}

void read_batt()
{
  int bat_level = M5.Power.getBatteryLevel();
    if (bat_level <= 25){
            M5.Lcd.setTextColor(RED);
        }
  M5.Lcd.drawString("        ",4,7,2);
  M5.Lcd.drawString(String(bat_level) + " %",4,7,2);
}
