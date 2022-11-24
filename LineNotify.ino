void Line_Notify(String message);
#include <SPI.h>
//#include <WiFiClientSecure.h>
#include "SevenSegmentTM1637.h"
#include "max6675.h"
#include <time.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureAxTLS.h>
// Config connect WiFi
#define WIFI_SSID "XXXX" // แก้ชื่อ ssid
#define WIFI_PASSWORD "XXXX" // แก้รหัสผ่าน 
// Line config
#define LINE_TOKEN "XXXX" // แก้ LINE TOKEN
String message = "Hello NodeMCU and Line";
//String message2 = "%E0%B8%AA%E0%B8%A7%E0%B8%B1%E0%B8%AA%E0%B8%94%E0%B8%B5%20Line%20and%20NodeMCU%20%E0%B8%A2%E0%B8%B4%E0%B8%99%E0%B8%94%E0%B8%B5%E0%B8%97%E0%B8%B5%E0%B9%88%E0%B9%84%E0%B8%94%E0%B9%89%E0%B8%A3%E0%B8%B9%E0%B9%89%E0%B8%88%E0%B8%B1%E0%B8%81";
String message2 = "ลองภาษาไทยไปด้วยเลยแล้วกัน";
//Line Notify ยังไม่รองรับภาษาไทย ดังนั้นเราสามารถแปลงข้อความเป็น utf-8 เพื่อส่งเป็นภาษาไทยได้ จาก http://meyerweb.com/eric/tools/dencoder/
//-----------------------------------------------------------
char ntp_server1[20] = "pool.ntp.org";
char ntp_server2[20] = "time.nist.gov";
char ntp_server3[20] = "time.uni.net.th";
int timezone = 7 * 3600;
int dst = 0;
//-----------------------------------------------------------
int ktcSO = 12;// D6
int ktcCS = 13;// D7
int ktcCLK = 14;// D5

int inputSignal = D0;    // D3 รับสัญญาณ เซ็นเซอร์ไฟดับ

float temp_chk = 0.0;
float temp_val = 0.0;
int min_max = 2;
int loop1;     
int loop_time;    
int loop_time2;    
int warning_flg; 
String celsius = "";
String alert = "";
String electricFall = "";
String electricTrue = "";
String check_date = "";
String cur_date = "";
MAX6675 thermocouple(ktcCLK, ktcCS, ktcSO);
const byte PIN_CLK = 5;   // D1 - define CLK pin (any digital pin)
const byte PIN_DIO = 4;   // D2 - define DIO pin (any digital pin)
SevenSegmentTM1637    display(PIN_CLK, PIN_DIO);
int temperature = 0;
int electricity = 0;
int chkElectricity = 0;
String showDisplay = "C";
//------------------------------------------------------------------------
void setup() {
      //--
      loop1 = 0;
      Serial.begin(115200);      
      Serial.println("Config start");
      WiFi.mode(WIFI_STA);
      // connect to wifi.
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      Serial.print("connecting");
      display.begin();            // initializes the display
      display.setBacklight(70);  // set the brightness to 100 %
      display.print("INIT");      // display INIT on the display
      Serial.print("MAC address: ");
      Serial.println(WiFi.macAddress());
      while (WiFi.status() != WL_CONNECTED) {
        if(loop1 == 20){
          loop1 = 0 ;
          Serial.println("Wait Wifi Connect!");
          
        }
        Serial.print(".");
        delay(500);
        loop1++;
      }
      //CC:50:E3:C6:DE:EC
      
   
      Serial.println(""); 
      Serial.println("WiFi connected");   //แสดงข้อความเชื่อมต่อสำเร็จ  
      Serial.print(" IP address : "); 
      Serial.println(WiFi.localIP()); //แสดงหมายเลข IP ของ ESP8266
      DisplayTemperature(0000);
      //-------------------------------------------------------------------
      configTime(timezone, dst, ntp_server1, ntp_server2, ntp_server3);
      Serial.println("\nWaiting for Loading time server...");
      while (!time(nullptr)) {
        Serial.print(".");
        delay(1000);
      }
      
      //check_date = p_tm->tm_year  +  std::string(2 - p_tm->tm_mon.length(), '0') + p_tm->tm_mon  +   std::string(2 - p_tm->tm_mday.length(), '0') + p_tm->tm_mday ;
      temp_chk = 0;
      /*celsius = String(thermocouple.readCelsius());
      alert = "อุณหภูมิห้อง = "+celsius+" C"; 
      Line_Notify(alert);*/
      //--------CHK ไฟฟ้า
      warning_flg = 1;
      pinMode(inputSignal, INPUT);// กำหนดขาทำหน้าที่ให้ขา D0 เป็น INPUT รับค่าจากสวิตช์
      
      chkElectricity = 1; // default เช็คไฟฟ้าดับ = 1 
      loop_time2 = 6 * 5;// loop เช็คไฟฟ้าดับ 5 นาที
      electricity = digitalRead(inputSignal); // อ่านค่าสถานะ;
      electricFall = "!!! กระแสไฟฟ้าห้อง Server room ดับ โปรดตรวจสอบ !!! ";
      electricTrue = "กระแสไฟฟ้าห้อง Server room ปกติ ";
      //--------CHK ไฟฟ้า
      time_t now = time(nullptr);      
      struct tm* p_tm = localtime(&now);        
      String y = String(p_tm->tm_year+1900)+"";
      String m = String(p_tm->tm_mon+1)+"";
      String d = String(p_tm->tm_mday)+"";
      String zero = "00";
      check_date = y+zero.substring(0,m.length())+m+zero.substring(0,d.length())+d;
      //Serial.println("--->"+check_date);
      //check_date = "20190902";
}

void loop(){      
      temp_val = thermocouple.readCelsius() - 1 ;//calibate on current temperature      
      DisplayTemperature(temp_val);
      celsius = String(temp_val);      
      electricity = digitalRead(inputSignal); // อ่านค่าสถานะ;
      //----------------------
      if(electricity == 1){//เช็คกระแสไฟฟ้าดับ
          chkElectricity = electricity;
          int time_limit  = 6 * 5; // =  5 min
          loop_time2++;
          if(loop_time2 > time_limit){
              Line_Notify(electricFall);
              loop_time2 = 0;
          }
          //electricFall electricTrue
      }else if(electricity == 0 && chkElectricity == 1){
          chkElectricity = electricity;
          Line_Notify(electricTrue);
          loop_time2 = 6 * 5;
      }
      //---------------------      
      alert = "!!** อุณหภูมิห้อง Server = "+celsius+" C"; 
      if(temp_val > 28.00){//เช็คถ้าอุณหภูมิเกินให้ส่ง Line
          if((temp_chk-min_max) > temp_val || (temp_chk+min_max) < temp_val){
              Line_Notify(alert);
              loop_time = 0;
              temp_chk = temp_val;   
              warning_flg = 1;           
          }
          int time_limit  = 6 * 15; // = 15 min
          if(loop_time > time_limit){
              Line_Notify(alert);
              loop_time = 0;
              temp_chk = temp_val;
              warning_flg = 1;
          }        
          loop_time++;
      }else if(temp_val < 19.00){//เช็คถ้าอุณหภูมิต่ำกว่าให้ส่ง Line
          if((temp_chk-min_max) > temp_val || (temp_chk+min_max) < temp_val){
              Line_Notify(alert);
              loop_time = 0;
              temp_chk = temp_val;   
              warning_flg = 1;           
          }
          int time_limit  = 6 * 15; // = 15 min
          if(loop_time > time_limit){
              Line_Notify(alert);
              loop_time = 0;
              temp_chk = temp_val;
              warning_flg = 1;
          }        
          loop_time++;
      
      }else if(warning_flg == 1){
          //temp_chk = temp_val;   
          Line_Notify(alert);
          warning_flg = 0;
          temp_chk = 0;
      }
      //----
      time_t now = time(nullptr);      
      struct tm* p_tm = localtime(&now);        
      String y = String(p_tm->tm_year+1900)+"";
      String m = String(p_tm->tm_mon+1)+"";
      String d = String(p_tm->tm_mday)+"";
      String zero = "00";
      cur_date = y+zero.substring(0,m.length())+m+zero.substring(0,d.length())+d;
      if(check_date != cur_date && (p_tm->tm_hour) > 7 ){
          check_date = cur_date;
          Line_Notify("อุณหภูมิห้อง Server เช้านี้ = "+celsius+ " C ระบบจะแจ้งเตือนเมื่ออุณหภฺมิมากกว่า 28.00 C และ น้อยกว่า 19.00 C");
          
      }
      //----      
      
      Serial.print("Check Electric : ");
      Serial.println(electricity);
      Serial.println(alert);
      
      Serial.print("CHECK DATE:");
      Serial.print(check_date);
      Serial.print("<---> CURRENT DATE : ");
      Serial.println(cur_date);
      Serial.print(" HOUR : ");
      Serial.println(p_tm->tm_hour);      
      delay(1000*10);//delay * 10 sec
}

void Line_Notify(String message) {
      axTLS::WiFiClientSecure client; // กรณีขึ้น Error ให้ลบ axTLS:: ข้างหน้าทิ้ง     
      if (!client.connect("notify-api.line.me", 443)) {
          Serial.println("connection failed");
          return;
      }  
      String req = "";
      req += "POST /api/notify HTTP/1.1\r\n";
      req += "Host: notify-api.line.me\r\n";
      req += "Authorization: Bearer " + String(LINE_TOKEN) + "\r\n";
      req += "Cache-Control: no-cache\r\n";
      req += "User-Agent: ESP8266\r\n";
      req += "Content-Type: application/x-www-form-urlencoded\r\n";
      req += "Content-Length: " + String(String("message=" + message).length()) + "\r\n";
      req += "\r\n";
      req += "message=" + message;
      Serial.println(req);
      client.print(req);
      delay(100);      
      Serial.println("-------------");
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
        Serial.println(line);
      }
      Serial.println("-------------");
}
void DisplayTemperature(float Temp) {
    temperature = int(Temp);
    showDisplay = String(temperature) + " C";
    display.clear();                      // clear the display
    display.print(showDisplay);
}
