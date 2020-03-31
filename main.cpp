#include "main.h"
#include "configuration.h"
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <set>
#include <WiFiClient.h>
#include "html.h"

ESP8266WebServer http_server(HTTP_REST_PORT);

std::vector<int> analogData;
std::set<String> listeners;
unsigned int target;
bool denialMode = false;
bool enabled = false;
Mode mode = TARGET;
unsigned long lastNotification = 0;
unsigned long lastRead = 0;
bool pulseMode = false;
unsigned int pulseInterval = 1000;
unsigned int pulseTarget = 0;
unsigned long nextPulse = 0;

void setup()
{
  setTarget(100);

  pinMode(LED_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(VALVE_PIN, OUTPUT);

  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(VALVE_PIN, LOW);

  Serial.begin(115200);

  setupWifi();
  setupRESTServer();

  digitalWrite(LED_PIN, LOW);

  Serial.println("ready!");

  for (unsigned int i = 0; i < 3; i++)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
    delay(250);
  }
}

void loop()
{
  if (!WiFi.isConnected())
  {
    Serial.println("disconnected!");
    MDNS.end();
    setupWifi();
  }

  MDNS.update();
  http_server.handleClient();

  if (enabled && lastRead + 10 < millis())
  {
    int p = analogRead(A0);
    lastRead = millis();

    analogData.insert(analogData.begin(), p);
    while (analogData.size() > samples)
    {
      analogData.pop_back();
    }

    analyzeData();
  }
}

//*********************************************************************
void analyzeData()
{
  double avg = average(0, samples);
  double diff = avg - target;

  Serial.println(String(avg) + "/" + String(target));

  if (denialMode)
  {
    if (diff > denialDelta)
    {
      notifyListeners();
    }
  }
  else
  {
    if (pulseMode && millis() > nextPulse)
    {
      nextPulse = millis() + pulseInterval;
      if (target > 0)
      {
        target = 0;
      }
      else
      {
        target = pulseTarget;
      }
    }
    switch (mode)
    {
    case OK:
      if (diff > targetDelta)
      {
        deflate();
      }
      else if (-diff > targetDelta)
      {
        inflate();
      }
      break;
    case INFLATE:
      if (avg > target)
      {
        stop();
      }
      break;
    case DEFLATE:
      if (avg < target)
      {
        stop();
      }
      break;
    }
  }
}

void stop()
{
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(VALVE_PIN, LOW);
  mode = TARGET;
}

void inflate()
{
  digitalWrite(PUMP_PIN, HIGH);
  digitalWrite(VALVE_PIN, LOW);
  mode = INFLATE;
}

void deflate()
{
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(VALVE_PIN, HIGH);
  mode = DEFLATE;
}

void notifyListeners()
{
  if (millis() - lastNotification > 1000)
  {
    Serial.println("DENIAL!!!");
    digitalWrite(LED_PIN, LOW);
    lastNotification = millis();

    for (std::set<String>::iterator it = listeners.begin(); it != listeners.end(); ++it)
    {
      digitalWrite(LED_PIN, HIGH);
      WiFiClient client;
      HTTPClient http;
      http.begin(client, String(*it));
      int status = http.GET();
      http.end();
      Serial.println(String(*it) + ": " + String(status));
      yield();
      digitalWrite(LED_PIN, LOW);
    }

    delay(250);
    digitalWrite(LED_PIN, enabled ? HIGH : LOW);
  }
}

void setTarget(int t)
{
  target = constrain(t, minTarget, maxTarget);
  targetDelta = target * 0.1;
}

double average(unsigned int startIncl, unsigned int endExcl)
{
  double avg = 0;
  int n = 0;

  for (unsigned int i = 0; i < endExcl && i < analogData.size(); i++)
  {
    avg += analogData.at(i);
    n++;
  }

  if (n > 0)
    avg /= n;

  return avg;
}

void setupRESTServer()
{
  http_server.on("/", HTTP_ANY, sendWebpage);
  http_server.on("/api", HTTP_ANY, handleRequest);
  http_server.begin();
  Serial.println("HTTP REST Server Started");
}

void sendWebpage()
{
  parseGetRequest();

  unsigned int s = sizeof(PAGE) + 100;
  char buffer[s];

  snprintf(buffer, s, PAGE,
           enabled ? "\"checked\"" : "",
           minTarget,
           maxTarget,
           target,
           target,
           pulseMode ? "\"checked\"" : "",
           pulseInterval,
           pulseInterval,
           denialMode ? "\"checked\"" : "",
           0,
           500,
           denialDelta,
           denialDelta);

  yield();
  http_server.send(200, "text/html", buffer);
}

void parseGetRequest()
{
  for (int i = 0; i < http_server.args(); i++)
  {
    String name = http_server.argName(i);
    String value = http_server.arg(i);

    Serial.println(name + " = " + value);

    if (name.equalsIgnoreCase("target"))
    {
      setTarget(value.toInt());
    }
    else if (name.equals("denial"))
    {
      denialMode = value.equalsIgnoreCase("true");
      if (denialMode)
      {
        stop();
      }
    }
    else if (name.equalsIgnoreCase("add_listener"))
    {
      listeners.insert(value);
      Serial.println("new listener: " + value);
    }
    else if (name.equalsIgnoreCase("rm_listener"))
    {
      listeners.erase(value);
      Serial.println("removed listener: " + value);
    }
    else if (name.equalsIgnoreCase("enable"))
    {
      enabled = value.equalsIgnoreCase("true");
      if (!enabled)
      {
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(VALVE_PIN, LOW);
      }
      digitalWrite(LED_PIN, enabled ? HIGH : LOW);
    }
    else if (name.equalsIgnoreCase("mintarget"))
    {
      minTarget = value.toInt();
    }
    else if (name.equalsIgnoreCase("maxtarget"))
    {
      maxTarget = value.toInt();
    }
    else if (name.equalsIgnoreCase("targetdelta"))
    {
      targetDelta = value.toInt();
    }
    else if (name.equalsIgnoreCase("denialdelta"))
    {
      denialDelta = value.toInt();
    }
    else if (name.equalsIgnoreCase("samples"))
    {
      samples = value.toInt();
    }
    else if (name.equalsIgnoreCase("pulse"))
    {
      pulseMode = value.equalsIgnoreCase("true");
      if (pulseMode)
      {
        pulseTarget = target;
      }
      else if (target == 0 && pulseTarget > 0)
      {
        target = pulseTarget;
      }
    }
    else if (name.equalsIgnoreCase("interval"))
    {
      pulseInterval = value.toInt();
    }
  }
}

void handleRequest()
{
  Serial.println("parsing request");
  parseGetRequest();
  Serial.println("sending status");
  sendStatus();
  Serial.println("OK");
}

void sendStatus()
{
  StaticJsonDocument<512> doc;
  doc["id"] = "SCS";
  doc["name"] = HOSTNAME;
  doc["avg_pressure"] = average(0, samples);
  doc["denialMode"] = denialMode;
  doc["enabled"] = enabled;
  doc["target"] = target;
  doc["targetDelta"] = targetDelta;
  doc["denialDelta"] = denialDelta;
  doc["minTarget"] = minTarget;
  doc["maxTarget"] = maxTarget;
  doc["samples"] = samples;
  //doc["sampleDelay"] = sampleDelay;

  JsonArray array = doc.createNestedArray("listeners");
  for (std::set<String>::iterator it = listeners.begin(); it != listeners.end(); ++it)
  {
    array.add(*it);
  }

  char json[512];
  serializeJson(doc, json);
  http_server.send(200, "application/json", json);
}

void setupWifi()
{
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(SSID, WIFI_PWD);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(HOSTNAME))
  {
    Serial.println("Error setting up MDNS responder!");
  }
  else
  {
    Serial.println("mDNS responder started for " + WiFi.hostname());
    MDNS.addService("http", "tcp", HTTP_REST_PORT);
    MDNS.addServiceTxt("http", "tcp", "service", "SCS");
    MDNS.addServiceTxt("http", "tcp", "type", "buttplug");
    MDNS.addServiceTxt("http", "tcp", "hostname", WiFi.hostname());
    MDNS.announce();
  }

  digitalWrite(LED_BUILTIN, HIGH);
}