#  include <ESP8266WiFi.h>
#include "DHT.h"
//#include "config.h"

#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht (DHTPIN, DHTTYPE);

//BLYNK API
#define BLYNK_TEMPLATE_ID "TMPL6dnpx1eNE"
#define BLYNK_TEMPLATE_NAME "Latihan"
#define BLYNK_AUTH_TOKEN "q-bZX9w1wIhH_OxYtz-4_pnNwsRjWCii"


//hotspot
char ssid[] = "SSID name";
char pass[] = "SSID password";

// Blynk cloud server
const char* host = "blynk.cloud";
unsigned int port = 80;

WiFiClient client;

int Status;

#define Rled D9
#define Gled D11
#define Bled D10


#define pot A0
#define tombol D2
#define lampuIndikator D12
#define lampuBiru D13

unsigned int potensio;
float suhu, humid;
bool Bstatus, Gstatus, Rstatus;

// Start the WiFi connection
void connectNetwork() {
  digitalWrite(lampuIndikator, 1);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(lampuIndikator, 0);
  Serial.println();
  Serial.println("WiFi connected");
}

bool httpRequest(const String& method,
                 const String& url,
                 const String& request,
                 String&       response)
{
  Serial.print(F("Connecting to "));
  Serial.print(host);
  Serial.print(":");
  Serial.print(port);
  Serial.print("... ");
  if (client.connect(host, port)) {
    Serial.println("OK");
  } else {
    Serial.println("failed");
    return false;
  }

  Serial.print(method); Serial.print(" "); Serial.println(url);

  client.print(method); client.print(" ");
  client.print(url); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(host);
  client.println(F("Connection: Keep-Alive"));
  if (request.length()) {
    client.println(F("Content-Type: application/json"));
    client.print(F("Content-Length: ")); client.println(request.length());
    client.println();
    client.print(request);
  } else {
    client.println();
  }

  //Serial.println("Waiting response");
  int timeout = millis() + 5000;
  while (client.available() == 0) {
    if (timeout - millis() < 0) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  //Serial.println("Reading response");
  int contentLength = -1;
  while (client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    line.toLowerCase();
    if (line.startsWith("content-length:")) {
      contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
    } else if (line.length() == 0) {
      break;
    }
  }

  //Serial.println("Reading response body");
  response = "";
  response.reserve(contentLength + 1);
  while (response.length() < contentLength) {
    if (client.available()) {
      char c = client.read();
      response += c;
    } else if (!client.connected()) {
      break;
    }
  }
  client.stop();
  return true;
}

void setup() {
  Serial.begin(9600);
  initPin();
  delay(10);
  Serial.println();
  Serial.println();

  dht.begin();
  connectNetwork();
}

void loop() {
  String response;
  humid = dht.readHumidity(); //Variabel penampung nilai suhu
  suhu = dht.readTemperature(); //Variabel penampung nilai kelembaban
  if (isnan(humid)) {
    humid = -99;
  }
  if (isnan(suhu)) {
    suhu = -99;
  }

  potensio = analogRead(A0);
  ledOff();
  lampuLED();

  // Send value to the cloud
  // similar to Blynk.virtualWrite()

  //tampil data
  Serial.println("\r\n--------------");
  Serial.print("Humidity: ");  Serial.print(humid);  Serial.print(" % \t");
  Serial.print("Temperature: ");  Serial.print(suhu);  Serial.println(" *C ");
  Serial.print("Potensio: ");  Serial.print(potensio);
  Serial.print(", Rstatus: ");  Serial.print(Rstatus);
  Serial.print(", Gstatus: ");  Serial.print(Gstatus);
  Serial.print(", Bstatus: ");  Serial.println(Bstatus);
  //SEND BATCH DATA OR MULTIPLE DATA
  String Data = "/external/api/batch/update?token=" + String(BLYNK_AUTH_TOKEN) + "&V0=" + String(humid, 1);
  Data = Data + "&V1=" + String(suhu, 1);
  Data = Data + "&V3=" + String(potensio);
  Data = Data + "&V4=" + String(Rstatus);
  Data = Data + "&V5=" + String(Gstatus);
  Data = Data + "&V6=" + String(Bstatus);

  if (httpRequest("GET", Data , "", response)) {
    if (response.length() != 0) {
      Serial.print("WARNING: ");
      Serial.println(response);
    }
  }

  //  if (httpRequest("GET", String("/external/api/update?token=") + BLYNK_AUTH_TOKEN + "&pin=V3&value=" + value, "", response)) {
  //    if (response.length() != 0) {
  //      Serial.print("WARNING: ");
  //      Serial.println(response);
  //    }
  //  }

  // Read the value
  Serial.println("Reading value");
  if (httpRequest("GET", String("/external/api/get?token=") + BLYNK_AUTH_TOKEN + "&pin=V2", "", response)) {
    Serial.print("Value from server: ");
    Serial.println(response);
    Status = response.toInt();
    digitalWrite(lampuBiru, Status);
  }

  if (httpRequest("GET", String("/external/api/update?token=") + BLYNK_AUTH_TOKEN + "&pin=V7&value=" + Status, "", response)) {
    if (response.length() != 0) {
      Serial.print("WARNING: ");
      Serial.println(response);
    }
  }

  delay(1000L);
}

void lampuLED() {
  if (potensio < 150) {
    digitalWrite(Rled, 1);    Rstatus = 1;
  }
  else if (potensio >= 150 && potensio < 300) {
    digitalWrite(Rled, 1);    Rstatus = 1;
    digitalWrite(Gled, 1);    Gstatus = 1;
  }
  else if (potensio >= 300 && potensio < 450) {
    digitalWrite(Gled, 1);    Gstatus = 1;
  }
  else if (potensio >= 450 && potensio < 600) {
    digitalWrite(Bled, 1);    Bstatus = 1;
    digitalWrite(Gled, 1);    Gstatus = 1;
  }
  else if (potensio >= 600 && potensio < 750) {
    digitalWrite(Rled, 1);    Rstatus = 1;
    digitalWrite(Bled, 1);    Bstatus = 1;
  }
  else if (potensio >= 750 && potensio < 900) {
    digitalWrite(Bled, 1);    Bstatus = 1;
  }
  else {
    digitalWrite(Rled, 1);    Rstatus = 1;
    digitalWrite(Gled, 1);    Gstatus = 1;
    digitalWrite(Bled, 1);    Bstatus = 1;
  }
}

void ledOff() {
  digitalWrite(Rled, 0);    Rstatus = 0;
  digitalWrite(Gled, 0);    Gstatus = 0;
  digitalWrite(Bled, 0);    Bstatus = 0;
}

void initPin() {
  pinMode(tombol, INPUT_PULLUP);
  pinMode(Rled, OUTPUT);
  pinMode(Gled, OUTPUT);
  pinMode(Bled, OUTPUT);
  pinMode(lampuIndikator, OUTPUT);
  pinMode(lampuBiru, OUTPUT);
  digitalWrite(Rled, 0);
  digitalWrite(Gled, 0);
  digitalWrite(Bled, 0);
  digitalWrite(lampuBiru, 0);
  digitalWrite(lampuIndikator, 0);
}
