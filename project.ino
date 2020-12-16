// RemoteXY select connection mode and include library
#define REMOTEXY_MODE__SOFTSERIAL
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include "imagenes.h"

#include <SoftwareSerial.h>
#define SoftwareSerial_h
#include <RemoteXY.h>

// RemoteXY connection settings
#define REMOTEXY_SERIAL_RX D4
#define REMOTEXY_SERIAL_TX D3
#define REMOTEXY_SERIAL_SPEED 9600

// Conectar al wifi
const char* ssid = "Casa-Mari (~^•^)~";        // Poner aquí la SSID de la WiFi
const char* password = "luckyximonicojulieta99";  // Poner aquí el passwd de la WiFi
const char* mqtt_server = "47.63.0.49";
//const char* mqtt_server = "sergioi7pc.duckdns.org";

// cliente MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// variables
volatile int modo = 0;
volatile int modoAux;
volatile double temp;
volatile double humd;
String humedad_string;
String temperatura_string;
volatile int pintar = 0;
// LED
int led_b = D8;
int led_g = D7;
int led_r = D6;
//boton
const int boton = 14;
const int bluetooth = D4;
// Pantalla
#define ANCHO_PANTALLA 128 // ancho pantalla OLED
#define ALTO_PANTALLA 64 // alto pantalla OLED
Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);

//______________________________________REMOTE_XY______________________________________
// RemoteXY configurate
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =
{ 255, 3, 0, 4, 0, 99, 0, 10, 24, 1,
  1, 1, 17, 41, 30, 10, 177, 31, 84, 101,
  109, 112, 101, 114, 97, 116, 117, 114, 97, 0,
  1, 1, 17, 56, 30, 10, 177, 31, 72, 117,
  109, 101, 100, 97, 100, 0, 1, 1, 17, 27,
  30, 10, 177, 31, 73, 110, 105, 99, 105, 111,
  0, 65, 4, 12, 15, 9, 9, 65, 7, 42,
  15, 9, 9, 129, 0, 9, 9, 13, 4, 17,
  65, 108, 97, 114, 109, 97, 0, 129, 0, 35,
  9, 23, 4, 17, 84, 101, 109, 112, 101, 114,
  97, 116, 117, 114, 97, 0
};

// this structure defines all the variables and events of your control interface
struct {

  // input variables
  uint8_t btn_tmp; // =1 if button pressed, else =0
  uint8_t btn_hmd; // =1 if button pressed, else =0
  uint8_t btn_ini; // =1 if button pressed, else =0
  //uint8_t btn_alrm; // =1 if button pressed, else =0

  // output variables
  uint8_t led_alrm_r; // =0..255 LED Red brightness
  uint8_t led_1_r; // =0..255 LED Red brightness
  uint8_t led_1_g; // =0..255 LED Green brightness
  uint8_t led_1_b; // =0..255 LED Blue brightness

  // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0

} RemoteXY;
#pragma pack(pop)


//______________________________________ISR______________________________________
void ICACHE_RAM_ATTR apagarAlarma() {
  Serial.println("boton");
  if (modo == 3) {
    modo = modoAux;
    RemoteXY.led_1_r = 0;
  }
}

//______________________________________MQTT______________________________________

// recibir de los diferentesa topicos
void callback(char* topic, byte* payload, unsigned int length) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.println("callback");
  display.display();

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    char receivedChar = (char)payload[i];
    Serial.print(receivedChar);
  }
  Serial.println();
  //actualizamos los datos
  String string_topic(topic);
  String dato;
  for (int i = 0; i < length; i++) {
    char receivedChar = (char)payload[i];
    dato.concat(receivedChar);
  }

  if ((string_topic == "wemosd1mini/binary_sensor/botn_del_pnico/state" || string_topic == "wemosd1mini/binary_sensor/muelle/state") && dato == "ON") {
    modo = 3;
  } else if (string_topic == "wemosd1mini/sensor/temperatura_dht11/state") {
    temp = dato.toDouble();
    temperatura_string = "";
    temperatura_string.concat(dato);
    temperatura_string.concat("C");

  } else if (string_topic == "wemosd1mini/sensor/humedad_dht11/state") {
    humd = dato.toDouble();
  }
  pintarPantalla();
}

//suscripción a los topicos
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 30);
  display.print("Connecting to ");
  display.setCursor(10, 40);
  display.println(ssid);
  display.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  display.clearDisplay();
  display.setCursor(10, 30);
  display.println("WiFi connected");
  display.setCursor(10, 40);
  display.println("IP address: ");
  display.setCursor(10, 50);
  display.println(WiFi.localIP());
  display.display();
  delay(1000);


}

void reconnect() {
  // Loop until we're reconnected
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 30);
  while (!client.connected()) {
    display.print("Attempting MQTT connection...");
    display.display();
    // Attempt to connect
    if (client.connect("mqtt", "mqtt", "mqtt")) {
      display.setCursor(10, 40);
      Serial.println("connected");
      // ... and subscribe to topic
      display.display();
      delay(1000);
      client.subscribe("mym");
      client.subscribe("wemosd1mini/binary_sensor/muelle/state");
      client.subscribe("wemosd1mini/binary_sensor/botn_del_pnico/state");
      client.subscribe("wemosd1mini/sensor/temperatura_dht11/state");
      client.subscribe("wemosd1mini/sensor/humedad_dht11/state");
    } else {
      display.setCursor(10, 40);
      display.print("failed, rc=");
      display.display();
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


//______________________________________setup______________________________________
void setup()
{
  RemoteXY_Init ();
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  setup_wifi();



  //LED
  pinMode(led_b, OUTPUT);
  pinMode(led_g, OUTPUT);
  pinMode(led_r, OUTPUT);

  // Interrupciones
  //interrupts();
  pinMode(boton, INPUT_PULLUP);
  pinMode(bluetooth, INPUT);
  attachInterrupt(digitalPinToInterrupt(boton), apagarAlarma, FALLING);
  //attachInterrupt(digitalPinToInterrupt(bluetooth), apagarAlarma, CHANGE);

}

//______________________________________loop______________________________________

void loop()
{
  RemoteXY_Handler ();

  // bluetooth
  if (RemoteXY.btn_tmp == 1) {
    modo = 1;
    modoAux = 1;
    pintarPantalla();
  } else if (RemoteXY.btn_hmd == 1) {
    modo = 2;
    modoAux = 2;
    pintarPantalla();
  } else if (RemoteXY.btn_ini == 1) {
    modo = 0;
    modoAux = 0;
    pintarPantalla();
  }

  if (pintar == 1) {
    Serial.println(modo);
    pintarPantalla();
    pintar = 0;
  }

  //Cambiamos el led de la temperatura
  if (temp < 16) { // encendemos de azul
    analogWrite(led_b, 255);
    analogWrite(led_g, 0);
    analogWrite(led_r, 0);
    RemoteXY.led_1_g = 0;
    RemoteXY.led_1_r = 0;
    RemoteXY.led_1_b = 255;
  } else if (temp > 16 && temp < 21) { // encendemos naranja
    analogWrite(led_b, 17);
    analogWrite(led_g, 70);
    analogWrite(led_r, 255);
    RemoteXY.led_1_b = 17;
    RemoteXY.led_1_g = 70;
    RemoteXY.led_1_r = 255;
  } else if (temp > 29) { // encendemos rojo
    analogWrite(led_b, 0);
    analogWrite(led_g, 0);
    analogWrite(led_r, 255);
    RemoteXY.led_1_b = 0;
    RemoteXY.led_1_g = 0;
    RemoteXY.led_1_r = 255;
  } else { // apagamos led
    analogWrite(led_b, 0);
    analogWrite(led_g, 0);
    analogWrite(led_r, 0);
    RemoteXY.led_1_b = 0;
    RemoteXY.led_1_g = 0;
    RemoteXY.led_1_r = 0;
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

}

void pintarPantalla() {
  analogWrite(led_b, 0);
  analogWrite(led_g, 0);
  analogWrite(led_r, 0);
  // Limpiar buffer
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  switch (modo) {
    case 0:
      display.setTextSize(2);
      display.setCursor(10, 30);
      display.println("inicio");
      break;
    case 1:
      display.drawBitmap(0, 16, imgTemp, 40, 40, SSD1306_WHITE);
      display.setCursor(52, 16);
      display.setTextSize(1);
      display.println("Temperatura ");
      display.setTextSize(2);
      display.setCursor(52, 26);
      //display.println(temp);
      display.println(temperatura_string);
      break;
    case 2:
      display.drawBitmap(0, 16, imgHumd, 40, 40, SSD1306_WHITE);
      display.setCursor(52, 16);
      display.setTextSize(1);
      display.println("Humedad ");
      display.setTextSize(2);
      display.setCursor(52, 26);
      //display.println(humd);
      humedad_string = "";
      humedad_string.concat(humd);
      humedad_string.concat("%");
      display.println(humedad_string);
      break;
    case 3:
      display.setTextSize(2);
      display.setCursor(10, 30);
      display.println("ALARMA");
      RemoteXY.led_alrm_r = 255;
      analogWrite(led_r, 255);
      delay(500);
      delay(1000);
      display.display();
      display.clearDisplay();
      delay (500);
      break;
  }
  // Enviar a pantalla
  display.display();
}
