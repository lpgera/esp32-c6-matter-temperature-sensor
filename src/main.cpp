#include <Arduino.h>
#include <Matter.h>
#include <Adafruit_BME280.h>
#include <Ticker.h>

Adafruit_BME280 bme;
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();
sensors_event_t temperatureEvent, humidityEvent, pressureEvent;

MatterTemperatureSensor temperatureSensor;
MatterHumiditySensor humiditySensor;
MatterPressureSensor pressureSensor;
uint32_t buttonTimestamp = 0;
bool buttonState = false;
uint32_t externalButtonTimestamp = 0;
bool externalButtonState = false;

bool commissioningState = true;
bool ledState = false;
void toggleLED() {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
}
Ticker ledTicker(toggleLED, 500);

void getLatestMeasurements() {
    bme_temp->getEvent(&temperatureEvent);
    bme_pressure->getEvent(&pressureEvent);
    bme_humidity->getEvent(&humidityEvent);

    Serial.printf("Temperature: %f C, Humidity: %f %%, Pressure: %f hPa \n", temperatureEvent.temperature, humidityEvent.relative_humidity, pressureEvent.pressure);

    temperatureSensor.setTemperature(temperatureEvent.temperature);
    humiditySensor.setHumidity(humidityEvent.relative_humidity);
    pressureSensor.setPressure(pressureEvent.pressure);
}
Ticker measurementTicker(getLatestMeasurements, 30000);

void setup() {
    Serial.begin(115200);
    pinMode(BOOT_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(GPIO_NUM_20, INPUT_PULLDOWN);
    bme.begin(0x76);

    bme_temp->getEvent(&temperatureEvent);
    bme_pressure->getEvent(&pressureEvent);
    bme_humidity->getEvent(&humidityEvent);

    temperatureSensor.begin(temperatureEvent.temperature);
    humiditySensor.begin(humidityEvent.relative_humidity);
    pressureSensor.begin(pressureEvent.pressure);
    ArduinoMatter::begin();

    measurementTicker.start();

    if (!ArduinoMatter::isDeviceCommissioned()) {
        Serial.println("Device is not commissioned. Waiting for setup...");
        Serial.print("Manual pairing code: ");
        Serial.println(ArduinoMatter::getManualPairingCode());
        Serial.print("QR code URL: ");
        Serial.println(ArduinoMatter::getOnboardingQRCodeUrl());
    } else {
        Serial.println("Device is already commissioned.");
    }
}

void loop() {
    ledTicker.update();
    measurementTicker.update();

    if (ArduinoMatter::isDeviceCommissioned() != commissioningState) {
        commissioningState = ArduinoMatter::isDeviceCommissioned();
        if (commissioningState) {
            ledTicker.stop();
            digitalWrite(LED_BUILTIN, HIGH);
        } else {
            ledTicker.start();
        }
    }

    if (digitalRead(BOOT_PIN) == LOW && !buttonState) {
        buttonTimestamp = millis();
        buttonState = true;
    }

    bool newExternalButtonState = digitalRead(GPIO_NUM_20);
    if (externalButtonState != newExternalButtonState && millis() - externalButtonTimestamp > 100) {
        externalButtonState = newExternalButtonState;
        externalButtonTimestamp = millis();
        if (newExternalButtonState) {
            Serial.println("Button pressed");
        }
    }

    if (buttonState && millis() - buttonTimestamp > 5000) {
        Serial.println("Decommissioning...");
        ArduinoMatter::decommission();
        buttonTimestamp = millis();
    }
}