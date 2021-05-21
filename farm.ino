#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
#include <Servo.h>
#include <TimeLib.h>  
#define BLYNK_PRINT Serial    
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>
#include <NTPtimeESP.h>
#include <FirebaseArduino.h> 
#define FIREBASE_HOST "iotfarm-dt2501-default-rtdb.firebaseio.com"                         
#define FIREBASE_AUTH "ya6yJURyKF6aauKCUYp6rRAvQd2mv99GwEketjoL"  
NTPtime NTPch("ch.pool.ntp.org");
strDateTime dateTime;


WidgetLED PUMP(V0);  // Đèn trạng thái bơm
WidgetLED LAMP(V1);  // Đèn trạng thái đèn sưởi
WidgetRTC rtc;
 
#include "DHT.h" //thư viện Sensor
#include <Servo.h> //thư viện servo
Servo servo;


#define SOIL_MOIST A0 // Chân PE4 nối với cảm biến độ ẩm
// Relay, nút nhấn
#define PUMP_ON_BUTTON 14   //Nút điều khiển bằng tay bơm
#define LAMP_ON_BUTTON 12   //Nút điều khiển đèn bằng tay
#define SENSORS_READ_BUTTON 2 //Nút lấy dữ liệu tức thời
#define PUMP_PIN 13   //Bom
#define LAMP_PIN 15   //Den

#define DHTPIN 0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

///* Auto */
//#define DRY           10
//#define WET_SOIL      85
//#define COLD          20
//#define HOT_TEMP      22
//#define TIME_PUMP_ON  15
//#define TIME_LAMP_ON  15

/* TIMER */
#define READ_BUTTONS_TM   1L  // Tương ứng với giây
#define READ_SOIL_HUM_TM  10L //Đọc cảm biến ẩm đất
#define READ_AIR_DATA_TM  2L  //Đọc DHT
#define DISPLAY_DATA_TM   10L //Gửi dữ liệu lên terminal
#define SEND_UP_DATA_TM   5L //Gửi dữ liệu lên blynk
#define LCD      1L //Chế độ tư động


/* HEN GIO*/
bool timeOnOff      = false;
bool oldtimeOnOff;
bool isFirstConnect;
unsigned int TimeStart, TimeStop ;
byte dayStartSelect = 0;
byte dayStopSelect = 0 ;
int oldSecond, nowSecond;

#define timeShow V9
#define timeInput V7
#define PinOut    13


WidgetLED LED(PUMP);



//Token Blynk và wifi
char auth[] = "MH3ljbP7KA257l2zw24g8XN3EtFsKYos "; // Blynk token
char ssid[] = "Matwifiroi"; //Tên wifi
char pass[] = "kiduynhat"; //Mật khẩu

// Biến lưu các giá trị cảm biến
float humDHT = 0;
float tempDHT = 0;
//int lumen;
int Soil = 0;
// Biến lưu trạng thái bơm
boolean pumpStatus = 0;
boolean lampStatus = 0;


int timePumpOn = 10; // Thời gian bật bơm nước
// Biến cho timer
long sampleTimingSeconds = 1000; // ==> Thời gian đọc cảm biến (s)
long startTiming = 0;
long elapsedTime = 0;
// Khởi tạo timer
SimpleTimer timer;
byte thermometro[8] = //icon for termometer
{
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};


byte igrasia[8] = //icon for water droplet
{
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B10001,
  B10001,
  B01110,
};


void setup() {
      lcd.begin();
      lcd.backlight();
      dht.begin();
      lcd.createChar(1, thermometro);
      lcd.createChar(2, igrasia);
      lcd.setCursor(3,0);
      lcd.print("SMARTFARM");
      lcd.setCursor(1,1);
      lcd.print("DESIGN BY DTN");
      delay(2000);
      lcd.clear();
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LAMP_PIN, OUTPUT);
    pinMode(PUMP_ON_BUTTON, INPUT_PULLUP); //chân button
    pinMode(LAMP_ON_BUTTON, INPUT_PULLUP);
    pinMode(SENSORS_READ_BUTTON, INPUT_PULLUP);
    pinMode(PinOut,OUTPUT);
    digitalWrite(PinOut, timeOnOff);
    servo.attach(16);
    // Khởi tạo cổng serial baud 115200
    Serial.begin(115200);
    Blynk.begin(auth, ssid, pass);   
    Serial.println(WiFi.localIP());
    lcd.setCursor(0,0);
    lcd.print(WiFi.localIP());
    lcd.clear();
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    aplyCmd();
    Blynk.syncAll();
    rtc.begin();
    PUMP.off();
    LAMP.off();
    startTimers();
    aplyCmd();
}


void loop() {
    dateTime = NTPch.getNTPtime(7.0, 0);
    String hum = String(humDHT) + String("%") ; 
    String temp = String(tempDHT) + String("°C"); 
    String soil = String(Soil) + String("%");
     Firebase.setString("/DHT11/Nhiet do",temp+ " time " + String(dateTime.hour) + ":" + String(dateTime.minute) + ":" + String(dateTime.second)+ " "+ String(dateTime.day) + "/" + String(dateTime.month));
     Firebase.setString("/DHT11/Do am",hum+ " time " + String(dateTime.hour) + ":" + String(dateTime.minute) + ":" + String(dateTime.second)+ " "+ String(dateTime.day) + "/" + String(dateTime.month));
     Firebase.setString("/DHT11/Do am dat",soil+ " time " + String(dateTime.hour) + ":" + String(dateTime.minute) + ":" + String(dateTime.second)+ " "+ String(dateTime.day) + "/" + String(dateTime.month)) ;
   
    timer.run(); // Bắt đầu SimpleTimer
    Blynk.run();
    showTime();
    TimeAuto();
    printLCD();
  
  

}


/****************************************************************
* Hàm điều khiển nhận tín hiệu từ blynk
****************************************************************/


BLYNK_WRITE(5) // Điều khiển cửa 
{
     servo.write(param.asInt());
     delay (10);
}

BLYNK_WRITE(3) // Điều khiển bơm
{
    int i = param.asInt();
    if (i == 1)
    {
      pumpStatus = !pumpStatus;
      aplyCmd();
    }
}

BLYNK_WRITE(4) // Điều khiển đèn
{
    int i = param.asInt();
    if (i == 1)
    {
      lampStatus = !lampStatus;
      if (lampStatus == 1)
  {
    
    digitalWrite(LAMP_PIN, LOW);
    LAMP.off();
  }
  else
  {
    digitalWrite(LAMP_PIN, HIGH);
    LAMP.on();
  }
    }
}

BLYNK_WRITE(timeInput) { 
  Serial.println("Time Input"); 
  TimeInputParam t(param); //blynk-library/src/WidgetTimeInput.h 
  // Process start time 
  if (t.hasStartTime())
  { 
    TimeStart = t.getStartHour()*60 + t.getStartMinute();
  }
  else TimeStart = 0;

  if (t.hasStopTime()) { 
    TimeStop = t.getStopHour()*60 + t.getStopMinute(); 
  }
  else TimeStop = 0;

  dayStartSelect = 0;
  dayStopSelect  = 0;
  for (int i = 1; i <= 7; i++) 
    if (t.isWeekdaySelected(i)) 
      if (i == 7) { 
        bitWrite(dayStartSelect, 0, 1); 
        bitWrite(dayStopSelect, 1, 1); 
      }
      else { 
        bitWrite(dayStartSelect, i, 1);
      bitWrite(dayStopSelect, i+1, 1); 
      } 
  }
  String twoDigits(int digits) {
  if(digits < 10) return "0" + String(digits); 
else return String(digits); }
void TimeAuto() { 
  
  unsigned int times = hour()*60 + minute();
byte today = weekday(); //the weekday now (Sunday is day 1, Monday is day 2) 
if (TimeStart == TimeStop) { timeOnOff = false; } 
else if (TimeStart < TimeStop)
if (bitRead(dayStartSelect, today - 1))
if ((TimeStart <= times) && (times < TimeStop)) timeOnOff = true; 
    else timeOnOff = false; 
  else timeOnOff = false; 
else if (TimeStart > TimeStop) 
{ 
  if ((TimeStop <= times) && (times < TimeStart)) timeOnOff = false;
  else if ((TimeStart <= times) && bitRead(dayStartSelect, today - 1)) timeOnOff = true; 
  else if ((TimeStop > times) && bitRead(dayStopSelect, today)) timeOnOff = true; } }



 void showTime() { 
 nowSecond = second(); 
 if (oldSecond != nowSecond) 
 { 
  oldSecond = nowSecond; 
  String currentTime;
 if (isPM()) currentTime = twoDigits(hourFormat12()) + ":" + twoDigits(minute()) + ":" + twoDigits(second()) + " PM";
 else  currentTime = twoDigits(hourFormat12()) + ":" + twoDigits(minute()) + ":" + twoDigits(second()) + " AM";
   String currentDate = String(day()) + "/" + month() + "/" + year(); 
 Blynk.virtualWrite(timeShow, currentTime);
  Blynk.virtualWrite(V6, currentDate);
 

  if (oldtimeOnOff != timeOnOff) 
{ 
  if (timeOnOff) { 
    LED.on();
    digitalWrite(PinOut, timeOnOff); 
  Serial.println("Time schedule is ON"); } 
  else { LED.off(); 
  digitalWrite(PinOut, timeOnOff); 
  Serial.println("Time schedule is OFF"); 
}  
    oldtimeOnOff = timeOnOff;
    }
  }
}

void getSoilHum(void) //hàm đọc cảm biến
{
    int i = 0;
    Soil = 0;
    for (i = 0; i < 10; i++)  //
    { 
      Soil += analogRead(SOIL_MOIST); //Đọc giá trị cảm biến độ ẩm đất
      delay(50);   // Đợi đọc giá trị ADC
    }
    Soil = Soil / (i);
    Soil = map(Soil, 1023, 0, 0, 100); //Ít nước:0%  ==> Nhiều nước 100%
    
}

void getDhtData(void) 
{
    tempDHT = dht.readTemperature();
    humDHT = dht.readHumidity();
    if (isnan(humDHT)||isnan(tempDHT))   // Kiểm tra kết nối lỗi thì thông báo.
    {
      Serial.println("Chua doc duoc gia tri sensor");
      return;
    }
}
void printLCD(){
  lcd.setCursor(0, 0);
  lcd.write(2);
  lcd.print("Soilhum: ");
  lcd.print(Soil);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.write(1);
  lcd.print(tempDHT);
  lcd.print((char)223);
  lcd.print("C");
  lcd.print(" ");
  lcd.write(2);
  lcd.print(humDHT);
  lcd.print("%");
     
}
void printSerial(void)
{
    // IN thông tin ra màn hình
    Serial.print("Do am: ");
    Serial.print(humDHT);
    Serial.print(" %\t");
    Serial.print("Nhiet do: ");
    Serial.print(tempDHT);
    Serial.print(" *C\t");
    Serial.print(" %\t");
    Serial.print("Soil: ");
    Serial.print(Soil);
    Serial.println(" %");
    
    
}


/****************************************************************
  Hàm đọc trạng thái bơm và kiểm tra nút nhấn
  (Nút nhấn mặc định là mức "CAO"):
****************************************************************/
void readLocalCmd()
{
    boolean digiValue = debounce(PUMP_ON_BUTTON);
    if (!digiValue)
    {
      pumpStatus = !pumpStatus;
      aplyCmd();
    }
  
    digiValue = debounce(LAMP_ON_BUTTON);
    if (!digiValue)
    {
      lampStatus = !lampStatus;
      if (lampStatus == 1)
  {
    digitalWrite(LAMP_PIN, LOW);
    LAMP.off();
  }
  else
  {
    digitalWrite(LAMP_PIN, HIGH);
    LAMP.on();
  }
    }
  
    digiValue = debounce(SENSORS_READ_BUTTON);
    if (!digiValue)
    {
      getDhtData();
      getSoilHum();
      printSerial();
      printLCD();
    }
}

 
/***************************************************
  Thực hiện điều khiển bơm và đèn
****************************************************/
void aplyCmd()
{     
    if (pumpStatus == 1)
    {
      digitalWrite(PUMP_PIN, LOW);
      PUMP.off();
    }  
    else {
     
      digitalWrite(PUMP_PIN, HIGH);
      PUMP.on();
    }     
}

/***************************************************
  Hàm kiểm tra trạng thái phím bấm
****************************************************/
boolean debounce(int pin)
{
    boolean state;
    boolean previousState;
    const int debounceDelay = 60;
  
    previousState = digitalRead(pin);
    for (int counter = 0; counter < debounceDelay; counter++)
    {
      delay(1);
      state = digitalRead(pin);
      if (state != previousState)
      {
        counter = 0;
        previousState = state;
      }
    }
    return state;
}


/***************************************************
* Chế độ tự động
//****************************************************/
//void autoControlPlantation(void)
//{
//    if (Soil < DRY)
//    {
//  
//      Blynk.notify("bom nuoc");
//      turnPumpOn();
//    }
//  
//    if (tempDHT < COLD)
//    {
//      Blynk.notify("bat den len");
//      turnLampOn();
//    }
//}


/***************************************************
* Bật bơm trong thời gian định sẵn
****************************************************/
//void turnPumpOn()
//{
//    pumpStatus = 1;
//    aplyCmd();
//    delay (TIME_PUMP_ON * 1000);
//    pumpStatus = 0;
//    aplyCmd();
//}
//
//
///***************************************************
//* Bật đèn trong thời gian định sẵn
//****************************************************/
//void turnLampOn()
//{
//    lampStatus = 1;
//    aplyCmd();
//    delay (TIME_LAMP_ON * 1000);
//    lampStatus = 0;
//    aplyCmd();
//}


/***************************************************
  Khởi động Timers
****************************************************/
void startTimers(void)
{
    timer.setInterval(READ_BUTTONS_TM * 1000, readLocalCmd);
    timer.setInterval(READ_AIR_DATA_TM * 1000, getDhtData);
    timer.setInterval(LCD * 1000, printLCD);
    timer.setInterval(READ_SOIL_HUM_TM * 1000, getSoilHum);
    timer.setInterval(SEND_UP_DATA_TM * 1000, sendBlynk);  
    timer.setInterval(DISPLAY_DATA_TM * 1000,printSerial);
}
/***************************************************
 * Gửi dữ liệu lên Blynk
 **************************************************/
 
void sendBlynk()
{
    Blynk.virtualWrite(10, tempDHT); //Nhiệt độ với pin V10
    Blynk.virtualWrite(11, humDHT); // Độ ẩm với pin V11
    Blynk.virtualWrite(12, Soil); // Độ ẩm đất với V12
}