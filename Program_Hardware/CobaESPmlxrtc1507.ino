#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <PZEM004Tv30.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <ArduinoJson.h>

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
LiquidCrystal_I2C lcd(0x27, 16, 2);
int GPSBaud = 9600;
int RXPin = 32;
int TXPin = 33;
TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXPin, TXPin);
#define RXD2 16
#define TXD2 17
PZEM004Tv30 pzem_r(&Serial2);

HardwareSerial Receiver3(1);
//HardwareSerial Receiver(1);
#define Receiver3_Txd_pin 25
#define Receiver3_Rxd_pin 26
//#define Receiver_Txd_pin 32
//#define Receiver_Rxd_pin 33
//int enablePin = 13;
const int Enable =  4;
unsigned char returnBuffer[8], counter = 0;
int yawPitchRoll[3];
float kalibgy, kalibgy2, datagy1, datamlx1;

struct gy25 {
  char buffer[50];
  int counter;
  float heading;
} cmps;

unsigned long interval = 1000;
unsigned long waktuAwal = 0;
int range = 0;
int Max = 5;
int ButtonMenu = 15;
int ButtonStateMenu = 0;
bool bPress = false;
float kalibmlx, gy;
int tang, bul, tah, jam, men, det;
float aku, baik, mlx_a, mlx_b;

#define SD_CS 5
float vr;
float ir;
float freq;
float pf_r;
float energy;
float power;

const char* ssid = "AEF";
const char* password = "kamubaik21";
const char* mqtt_server = "broker.hivemq.com";
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
const int id_train = 001;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(250);
  pinMode(Enable, OUTPUT);
  digitalWrite(Enable, HIGH);
  lcd.init();                      // initialize the lcd
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Semangat TA nya");

  lcd.setCursor(0, 1);
  lcd.print("I Love U");
  lcd.clear();
  if (! rtc.begin()) {
    Serial.println("RTC tidak terbaca");
    while (1);
  }
  if (rtc.lostPower()) {
    //atur waktu sesuai waktu pada komputer
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    //atur waktu secara manual
    //January 21, 2019 jam 10:30:00
    //rtc.adjust(DateTime(2021, 4, 2, 23, 47, 0));
  }
  gpsSerial.begin(GPSBaud);
  mlx.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Receiver3.begin(115200, SERIAL_8N2, Receiver3_Txd_pin, Receiver3_Rxd_pin);
  //    Receiver.begin(9600, SERIAL_8N2, Receiver_Txd_pin, Receiver_Rxd_pin);
  //  pinMode(enablePin, OUTPUT);
  //  delay(10);
  //  digitalWrite(enablePin, LOW);
  SD.begin(SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("Gagal Memuat Kartu SD");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("Tidak Ada Kartu SD");
    return;
  }
  Serial.println("Menginisialisasi kartu SD...");
  //  Serial.print("Connecting to ");
  //  Serial.println(ssid);
}

void loop() {
  ButtonStateMenu = digitalRead(ButtonMenu);
  unsigned long waktuSekarang = millis();
  DateTime now = rtc.now();

//    Serial.print("I");
//    Serial.print("1");
//    Serial.print("L");
//    Serial.print("F");
//    Serial.flush();
//  
    digitalWrite(Enable, LOW);
    String kalib_a = Serial.readStringUntil('a');
    String kalib_b = Serial.readStringUntil('b');
    digitalWrite(Enable, HIGH);
//  
//    Serial.print("I");
//    Serial.print("2");
//    Serial.print("L");
//    Serial.print("F");
    Serial.flush();
//  
    digitalWrite(Enable, LOW);
    String kalib_c = Serial.readStringUntil('c');
    String kalib_d = Serial.readStringUntil('d');
////    Serial.print(c);
////    Serial.print(d);
//  
    digitalWrite(Enable, HIGH);



  if (waktuSekarang - waktuAwal >= interval) {
    while (gpsSerial.available() > 0)
      //Serial.write(gpsSerial.read());
      if (gps.encode(gpsSerial.read()))
        displayInfo();

    // If 5000 milliseconds pass and there are no characters coming in
    // over the software serial port, show a "No GPS detected" error
    if (millis() > 5000 && gps.charsProcessed() < 10)
    {
      Serial.println("No GPS detected");
      while (true);
    }

    while (Receiver3.available()) {
      returnBuffer[counter] = (unsigned char)Receiver3.read();
      if (counter == 0 && returnBuffer[0] != 0xAA) return;     //wait for start character
      counter++;
      if (counter == 8) {
        yawPitchRoll[0] = (returnBuffer[1] << 8 | returnBuffer[2]) / 100;
        yawPitchRoll[1] = (returnBuffer[3] << 8 | returnBuffer[4]) / 100;
        yawPitchRoll[2] = (returnBuffer[5] << 8 | returnBuffer[6]) / 100;
        counter = 0;
      }
    }
    if (yawPitchRoll[0] > 474) {
      kalibgy = yawPitchRoll[0] - 655;
      //      Serial.print("Kemiringan : ");
      //      Serial.print(kalibgy);
      //      Serial.print("*");
      //      Serial.print("\n");
    }
    if (yawPitchRoll[0] < 180) {
      kalibgy2 = yawPitchRoll[0] - 0;
      //      Serial.print("Kemiringan : ");
      //      Serial.print(kalibgy2);
      //      Serial.print("*");
      //      Serial.print("\n");
    }
    baca_pzem();
    //    Serial.print("Volt: "); Serial.print(vr, 2  ); Serial.print("V, ");
    //    Serial.print("curr: "); Serial.print(ir, 3); Serial.print("A, ");
    //    Serial.print("pf: "); Serial.print(pf_r); Serial.println("%, ");
    //    Serial.print("Power: "); Serial.print(power); Serial.print("W, ");
    //    Serial.print("Energy: "); Serial.print(energy, 3); Serial.print("kWh, ");
    //    Serial.print("freq: "); Serial.print(freq, 1); Serial.println("Hz, ");
    //    Serial.println();
    StaticJsonBuffer<300> JSONbuffer;
    JsonObject& JSONencoder = JSONbuffer.createObject();
    JsonObject& JSONencoder1 = JSONbuffer.createObject();

    JSONencoder["ta"] = "TDR";
    JSONencoder1["ta2"] = "TDR";
    JsonArray& values = JSONencoder.createNestedArray("values");
    JsonArray& datamt = JSONencoder1.createNestedArray("datamt");

    values.add(id_train);
    values.add(kalibgy);
    values.add(kalibmlx);
    values.add(vr);
    values.add(ir);
    values.add(freq);
    values.add(power);
    values.add(energy);
    values.add(gps.location.lng(), 6);
    values.add(gps.location.lat(), 6);
    values.add(gps.speed.kmph());
    datamt.add(id_train);
    datamt.add(kalibgy);
    datamt.add(mlx_a);
    datamt.add(mlx_b);

    char JSONmessageBuffer[100];
    char JSONmessageBuffer1[100];
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    JSONencoder1.printTo(JSONmessageBuffer1, sizeof(JSONmessageBuffer1));
    Serial.println("Sending message to MQTT topic..");
    Serial.println(JSONmessageBuffer);
    //String datapub = String(id_train) + "," + String(kalibgy) + "," + String(kalibmlx);
    client.publish("lrt/tdr/id/gerbongtdr", JSONmessageBuffer);
    client.publish("lrt/tdr/id/gerbongm&t", JSONmessageBuffer1);
    //  client.publish("lrt/tdr/mc1/kemiringan", String(kalibgy).c_str());
    //  client.publish("lrt/tdr/mc1/tempbear", String(kalibmlx).c_str());
    //    client.publish("lrt/tdr/mc1/elektrik/voltage", String(vr).c_str());
    //    client.publish("lrt/tdr/mc1/elektrik/arus", String(ir).c_str());
    //    client.publish("lrt/tdr/mc1/elektrik/freq", String(freq).c_str());
    //    client.publish("lrt/tdr/mc1/elektrik/daya", String(power).c_str());
    //    client.publish("lrt/tdr/mc1/elektrik/energy", String(energy).c_str());
    //    client.publish("lrt/tdr/mc1/gps/long", String(gps.location.lng(), 6).c_str());
    //    client.publish("lrt/tdr/mc1/gps/lat", String(gps.location.lat(), 6).c_str());
    //    client.publish("lrt/tdr/mc1/gps/kece", String(gps.speed.kmph()).c_str());
    //  client.publish("lrt/tdr/motor/kemiringan2", String(kalib_a).c_str());
    //  client.publish("lrt/tdr/motor/tempbear2", String(kalib_b).c_str());
    //    client.publish("lrt/tdr/trailer/kemiringan3", String(c).c_str());
    //    client.publish("lrt/tdr/trailer/tempbear3", String(d).c_str());
    // delay(5000);
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    tang = now.day(), DEC;
    bul = now.month(), DEC;
    tah = now.year(), DEC;
    jam = now.hour(), DEC;
    men = now.minute(), DEC;
    det = now.second(), DEC;
    kalibmlx = mlx.readObjectTempC() + 1;
    gy = yawPitchRoll[0];
    mlx_a = kalibmlx - 0.21;
    mlx_b = kalibmlx - 0.37;

    if (ButtonStateMenu == LOW) {
      bPress = true;
      if (ButtonStateMenu == 0) {
        range += 1;
      }
      if (range >= Max) {
        range = 0;
      }
      delay(200);
    }
    switch (range) {
      case 0:    // your hand is on the sensor
        lcd.clear();
        lcd.setCursor(6, 0);
        lcd.print("AKTIF");
        lcd.setCursor(0, 1);
        //lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
        printAngka(now.day());
        lcd.print("/");
        printAngka(now.month());
        lcd.print("/");
        printAngka(now.year());

        lcd.setCursor(11, 1);
        printAngka(now.hour());
        lcd.print(":");
        printAngka(now.minute() + 2);
        break;
      case 1:    // your hand is close to the sensor
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Temp. Bearing :");

        lcd.setCursor(4, 1);
        lcd.print (kalibmlx);
        lcd.setCursor(10, 1);
        lcd.print("C");
        break;
      case 2:    // your hand is a few inches from the sensor
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sudut Kereta :");

        lcd.setCursor(4, 1);
        lcd.print(kalibgy);
        lcd.setCursor(10, 1);
        lcd.print("*");
        break;
      case 3:    // your hand is a few inches from the sensor
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Volt :");
        lcd.setCursor(7, 0);
        lcd.print(vr, 2  );
        lcd.setCursor(14, 0);
        lcd.print("V");
        lcd.setCursor(0, 1);
        lcd.print("Arus :");
        lcd.setCursor(7, 1);
        lcd.print(ir, 3);
        lcd.setCursor(14, 1);
        lcd.print("A");
        break;
      case 4:    // your hand is a few inches from the sensor
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Daya :");
        lcd.setCursor(7, 0);
        lcd.print(power);
        lcd.setCursor(12, 0);
        lcd.print("Watt");
        lcd.setCursor(0, 1);
        lcd.print("Frek :");
        lcd.setCursor(7, 1);
        lcd.print(freq, 1);
        lcd.setCursor(12, 1);
        lcd.print("Hz");
        break;
    }
    waktuAwal = waktuSekarang;
  }

  String waktu = String(jam) + ":" + String(men) + ":" + String(det);
  String tgl = String(tang) + "/" + String(bul) + "/" + String(tah);
  String waktu_out = tgl + " " + waktu;
  String all_sensor = String (kalibgy) + "*"";" + String(kalibmlx) + "*C"";" + String(vr) + " ""V"";" + String(ir) + " ""A"";" + String(freq) + " ""F"";" + String(power) + " ""W"";" + String(energy) + " ""kWh";
  String data_gps = String (gps.location.lng(), 6) + ";" + String (gps.location.lat(), 6) + ";" + String (gps.speed.kmph()) + """km/jam";
  String data_out = waktu_out + ";" + all_sensor + ";" + data_gps;

  appendFile(SD, "/MotorCar.csv", String (data_out).c_str());
  delay(5000);
  //
  //  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);//hari
  //  Serial.print(", ");
  //  Serial.print(now.day(), DEC); //tanggal
  //  Serial.print('/');
  //  Serial.print(now.month(), DEC); //bulan
  //  Serial.print('/');
  //  Serial.print(now.year(), DEC); //tahun
  //  Serial.print(' ');
  //  Serial.print(now.hour(), DEC); //jam
  //  Serial.print(':');
  //  Serial.print(now.minute() + 5, DEC); //tanggal
  //  Serial.print(':');
  //  Serial.print(now.second(), DEC); //detik
  //  Serial.println();
  //  Serial.print("Temp Bearing : ");
  //  Serial.print(kalibmlx);
  //  Serial.println("*C");
  //  Serial.println();
  //  delay(1000);
}
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    //Serial.println("Failed to open file for appending");
    return;
  }
  if (file.println(message)) {
    //Serial.println("Message appended");
  } else {
    //Serial.println("Append failed");
  }
  file.close();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (char i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }


  // Switch on the GPIO if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(12, HIGH);   // Turn the LED on (Note that LOW is the voltage level
  }
  if ((char)payload[0] == '0') {
    digitalWrite(12, LOW);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32DEVClient")) {
      Serial.println("connected");

      client.subscribe("ESP32DEVBOARD"); //TOPIC
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void printAngka(int digits) {
  if (digits < 10) {
    lcd.print('0');
    lcd.print(digits);
  }
  else lcd.print(digits);
}

void baca_pzem() {
  vr = pzem_r.voltage();
  ir = pzem_r.current() - 0.004;
  freq = pzem_r.frequency();
  pf_r = pzem_r.pf();
  power = pzem_r.power();
  energy = pzem_r.energy() *1000;
}


void displayInfo()
{
  if (gps.location.isValid())
  {
    Serial.println("gps ok");
    //            Serial.print("Latitude: ");
    //            Serial.println(gps.location.lat(), 6);
    //            Serial.print("Longitude: ");
    //            Serial.println(gps.location.lng(), 6);
    //            Serial.print("Altitude: ");
    //            Serial.println(gps.altitude.meters());
    //            Serial.print("Kecepatan m/s = ");
    //            Serial.println(gps.speed.mps());
    //            // Speed in kilometers per hour (double)
    //            Serial.print("Kecepatan km/h = ");
    //            Serial.println(gps.speed.kmph());
  }
  else
  {
    Serial.println("Location: Not Available");
  }

  //Serial.print("Tanggal: ");
  if (gps.date.isValid())
  {
    //        Serial.print(gps.date.month());
    //        Serial.print("/");
    //        Serial.print(gps.date.day());
    //        Serial.print("/");
    //        Serial.println(gps.date.year());
  }
  else
  {
    Serial.println("Not Available");
  }
  Serial.println();
  delay(1000);
}
