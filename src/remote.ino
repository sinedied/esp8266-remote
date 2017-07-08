#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include <ESP8266mDNS.h>
#include <ESPmanager.h>
#include <Ticker.h>

const uint8_t UP_PIN = D1;
const uint8_t DOWN_PIN = D2;
const uint8_t LED = 2;
const unsigned long PUSH_TIME = 100;
const char *STATUS_UNKNOWN = "unknown";

AsyncWebServer server(80);
ESPmanager settings(server, SPIFFS);
const char *status = STATUS_UNKNOWN;
Ticker timer;

void sendStatus(AsyncWebServerRequest *request);
void activateSwitch(uint8_t pin);
void timerCallback();

void setup() {
	Serial.begin(115200);
	SPIFFS.begin();

	Serial.println("");
	Serial.println(F("ESPRemote started"));

	// Setup
	pinMode(UP_PIN, OUTPUT);
	digitalWrite(UP_PIN, LOW);
	pinMode(DOWN_PIN, OUTPUT);
	digitalWrite(DOWN_PIN, LOW);
	timer.setInterval(PUSH_TIME);
	timer.setCallback(timerCallback);

	// Flash light
	pinMode(LED, OUTPUT);
	for (int i = 0; i < 5; i++) {
		digitalWrite(LED, HIGH);
		delay(100);
		digitalWrite(LED, LOW);
		delay(100);
	}

	settings.begin();
	settings.disablePortal();

	// This rewrite is active when the captive portal is working, and redirects the root / to the setup wizard. 
	// This has to go in the main sketch to allow your project to control the root when using ESPManager. 
	// server.rewrite("/", "/espman/setup.htm").setFilter([](AsyncWebServerRequest *request) {
	// 	return settings.portal();
	// });

	// Shortcut
	server.rewrite("/setup", "/espman/setup.htm");

	// Then use this rewrite and serve static to serve your index file(s)
	server.rewrite("/", "/index.htm");
	server.serveStatic("/index.htm", SPIFFS, "/index.htm");

	server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request) {
		activateSwitch(UP_PIN);
		status = "up";
		sendStatus(request);
	});

	server.on("/down", HTTP_GET, [](AsyncWebServerRequest *request) {
		activateSwitch(DOWN_PIN);
		status = "down";
		sendStatus(request);
	});

	server.on("/status", HTTP_GET, sendStatus);

	server.begin();
	digitalWrite(LED, HIGH);
}

void loop() {
	settings.handle();
	timer.update();
}

// ---------------------------------------------------------------------------

void sendStatus(AsyncWebServerRequest *request) {
	AsyncResponseStream *response = request->beginResponseStream("text/json");
	DynamicJsonBuffer jsonBuffer;
	JsonObject &root = jsonBuffer.createObject();
	root["heap"] = ESP.getFreeHeap();
	root["status"] = status;
	root.printTo(*response);
	request->send(response);
}

void activateSwitch(uint8_t pin) {
	// Serial.println(F("activate"));
	digitalWrite(pin, HIGH);
	// delay(PUSH_TIME);
	// digitalWrite(pin, LOW);
	timer.start();	
}

void timerCallback() {
	// Serial.println(F("deactivate"));
	digitalWrite(UP_PIN, LOW);
	digitalWrite(DOWN_PIN, LOW);
}