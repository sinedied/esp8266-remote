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

const uint8_t UP_PIN = 1;
const uint8_t DOWN_PIN = 2;
const char *STATUS_UNKNOWN = "unknown";

AsyncWebServer server(80);
ESPmanager settings(server, SPIFFS);
const char *status = STATUS_UNKNOWN;

void sendStatus(AsyncWebServerRequest *request);
void activateSwitch(uint8_t pin);

void setup() {
	Serial.begin(115200);
	SPIFFS.begin();

	Serial.println("");
	Serial.println(F("ESPRemote started"));

	// Setup GPIO
	pinMode(UP_PIN, OUTPUT);
	pinMode(DOWN_PIN, OUTPUT);

	// Flash light
	pinMode(LED_BUILTIN, OUTPUT);
	for (int i = 0; i < 5; i++) {
		digitalWrite(LED_BUILTIN, HIGH);
		delay(100);
		digitalWrite(LED_BUILTIN, LOW);
		delay(100);
	}

	settings.begin();
	settings.disablePortal();

	// This rewrite is active when the captive portal is working, and redirects the root / to the setup wizard. 
	// This has to go in the main sketch to allow your project to control the root when using ESPManager. 
	// server.rewrite("/", "/espman/setup.htm").setFilter([](AsyncWebServerRequest *request) {
	// 	return settings.portal();
	// });

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
}

void loop() {
	settings.handle();
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
	digitalWrite(pin, HIGH);
	delay(100);
	digitalWrite(pin, LOW);
}
