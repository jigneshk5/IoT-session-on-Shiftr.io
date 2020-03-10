#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const char ssid[] = "Silver Lions Zone";
const char pass[] = "science@123";

WiFiClient net;
MQTTClient client;
Servo myservo;  // create servo object to control a servo


unsigned long lastMillis = 0;
int ldrValue=0;
int pos=0;
String ldr, distance, p;
const int red= D5;
const int green= D6;
const int trigPin = D3;
const int echoPin = D4;
const int buttonPin = D7;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("mqtt", "iot_guy123", "iot_guy123")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");
  client.subscribe("iot_guy/errors");
  client.subscribe("iot_guy/throttle");
  client.subscribe("iot_guy/feeds/red-led");
  client.subscribe("iot_guy/feeds/servo");
  client.subscribe("iot_guy/feeds/green-led");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if(topic=="iot_guy/feeds/red-led"){
    if(payload=="ON")
      digitalWrite(red,HIGH);
    else
      digitalWrite(red,LOW);
  }
  if(topic=="iot_guy/feeds/green-led"){
    if(payload=="ON")
      digitalWrite(green,HIGH);
    else
      digitalWrite(green,LOW);
  }
  if(topic=="iot_guy/feeds/servo"){
        pos = payload.toInt();
        Serial.println("SERVO POSITION: "+pos);
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
  }

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  pinMode(A0, INPUT); 
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(trigPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  pinMode(buttonPin,INPUT_PULLUP);

  myservo.attach(D8);  // attaches the servo on D8 to the servo object

  //OLED SETUP
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); /* Initialize display with address 0x3C */
  display.clearDisplay();  /* Clear display */
  display.setTextSize(2);  /* Select font size of text. Increases with size of argument. */
  display.setTextColor(WHITE);  /* Color of text*/
  
  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin("io.adafruit.com", net);
  client.onMessage(messageReceived);

  connect();
}

void loop() {
  client.loop();
  delay(5000);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  // publish a message roughly every 2 second.
  if (millis() - lastMillis > 4000) {
    lastMillis = millis();
    ldrValue = analogRead(A0);   // read the ldr input on analog pin 0
    ldr= String(ldrValue);
    distance = triggerRadar(trigPin, echoPin);
    p = String(pos);
    Serial.println("LDR: "+ldr+" || Dist: "+distance+" || Pos; "+p);
    updateOLED(ldr,distance,p);/* Update OLED Every second and display */
    if(digitalRead(buttonPin)==0){
       client.publish("iot_guy/feeds/button", "1");
    }
    client.publish("iot_guy/feeds/button", "0");
    client.publish("iot_guy/feeds/ldr", ldr);
    client.publish("iot_guy/feeds/ultrasonic", distance);
  }
}

void updateOLED(String ldr, String d, String pos){
    display.clearDisplay();
   display.setCursor(0,0);
  display.println("LDR:"+ldr);
  
  display.setCursor(0, 25);
  display.println("DIST:"+d);

//  display.setCursor(0, 30);
//  display.println("POS: "+pos);
  display.display();
}
String triggerRadar(int trigPin, int echoPin){
  String dist;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  double d = duration*0.0343/2;
  dist= String(d);
  return dist;
}
