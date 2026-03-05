// ESP32C3FN4 SuperMini Board
// ===============================================================
// Arduino IDE settings:
//   - Board: ESP32C3 Dev Module
//   - ESP CDC On Boot: Enabled
//   - CPU Frequency: 80MHz (WiFi)
//   - Core Debug Level: None
//   - Erase All Flash Before Sketch Upload: Disabled
//   - Flash frequency: 80Mhz
//   - Flash Mode: QIO
//   - Flash Size: 4MB (32Mb)
//   - JTAG Adapter: Disabled
//   - Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
//   - Upload Speed: 921600
//   - Zigbee Mode: Disabled
//   - Programmer: Esptool
// ===============================================================

#include <HomeSpan.h>
#include <PubSubClient.h>
#include <WiFi.h>


/**************************************************************************************/
/*                                        Pins                                        */

// HomeSpan status LED pin.
#define STATUS_LED_PIN                  GPIO_NUM_8
// HomeSpan control button pin.
#define CONTROL_PIN                     GPIO_NUM_9

// The doorbell button input pin (radio signal)
#define BELL_BUTTON_PIN                 GPIO_NUM_3
// The doorbell signal pin (wired to the sound chip)
#define BELL_SIGNAL_PIN                 GPIO_NUM_10

/**************************************************************************************/


/**************************************************************************************/
/*                                  Firmware defines                                  */

// The doorbell button signal duration in milliseconds.
#define BELL_BUTTON_SIGNAL_DURATION     500
// The doorbell play signal duration in milliseconds.
#define BELL_SIGNAL_DURATION            250

/**************************************************************************************/


/**************************************************************************************/
/*                                   MQTT  settings                                   */

// MQTT server port.
const uint16_t MQTT_PORT = 1883;
// MQTT server address.
const char* const MQTT_SERVER = "192.168.1.13";
// MQTT client ID.
const char* const MQTT_CLIENT_ID = "DroneTales Doorbell";
// MQTT server user name.
const char* const MQTT_USER_NAME = "dronetales";
// MQTT server password.
const char* const MQTT_PASSWORD = "dronetales";
// Camera UI MQTT doorbell topic.
const char* const MQTT_DOORBELL_TOPIC = "doorcam/bell";
// Camera UI MQTT doorbell ring message.
const char* const MQTT_DOORBELL_MESSAGE = "RING";

/**************************************************************************************/


/**************************************************************************************/
/*                                  Global variables                                  */

// The doorbell state. True if the doorbell is enabled and should play a sound.
// False if the doorbell is disabled and should not play any sound.
static bool BellEnabled = false;
// Indicates when the ring button was pressed.
static volatile bool BellRing = false;

// The WiFi client instalce.
static WiFiClient NetClient;
// The MQTT instance.
PubSubClient MqttClient(NetClient);

/**************************************************************************************/


/**************************************************************************************/
/*                              Virtual  Doorbell switch                              */

struct DoorbellSwitch : Service::Switch
{
    SpanCharacteristic* Power;
    
    DoorbellSwitch() : Service::Switch()
    {
        // Default is false (bell is turned off) and we store current value in NVS.
        Power = new Characteristic::On(false, true);
        // Get current state.
        BellEnabled = Power->getVal();
    }
    
    bool update()
    {
        BellEnabled = Power->getNewVal();
        return true;
    }
};

/**************************************************************************************/


/**************************************************************************************/
/*                              Doorbell signal interrup                              */

void IRAM_ATTR RingInterrupt()
{
    static bool WasHigh = false;
    static uint32_t LastMillis = 0;

    uint32_t Level = digitalRead(BELL_BUTTON_PIN);
    if (Level == HIGH && !WasHigh)
    {
        WasHigh = true;
        LastMillis = millis();
        return;
    }

    if (Level == LOW && WasHigh)
    {
        WasHigh = false;

        uint32_t CurrentMillis = millis();
        if (CurrentMillis - LastMillis >= BELL_BUTTON_SIGNAL_DURATION)
            BellRing = true;
    }
}

/**************************************************************************************/


/**************************************************************************************/
/*                                  Arduino routines                                  */

// Arduino initialization routine.
void setup()
{
    // Initialize debug serial port.
    Serial.begin(115200);
    
    // Initialize pins door bell pins.
    pinMode(BELL_SIGNAL_PIN, OUTPUT);
    pinMode(BELL_BUTTON_PIN, INPUT_PULLDOWN);
    digitalWrite(BELL_SIGNAL_PIN, LOW);
    
    // Initialize HomeSpan pins
    pinMode(CONTROL_PIN, INPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);
    
    // Initialize HomeSpan.
    homeSpan.setControlPin(CONTROL_PIN);
    homeSpan.setStatusPin(STATUS_LED_PIN);
    homeSpan.setPairingCode("63005612");
    homeSpan.begin(Category::Other, "DroneTales Doorbell");

    // Build device's serial number.
    char Sn[24];
    snprintf(Sn, 24, "DRONETALES-%llX", ESP.getEfuseMac());

    // Configure doorbell switch.
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Manufacturer("DroneTales");
    new Characteristic::SerialNumber(Sn);
    new Characteristic::Model("DroneTales Doorbell Switch");
    new Characteristic::FirmwareRevision("1.0.0.0");
    new DoorbellSwitch();

    // Configure MQTT client.
    MqttClient.setServer(MQTT_SERVER, MQTT_PORT);

    // Attached the interrupt to the ring signal pin.
    attachInterrupt(BELL_BUTTON_PIN, RingInterrupt, CHANGE);
}

// Arduino main loop.
void loop()
{
    // Every loop try to connect to MQTT server.
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!MqttClient.connected())
            MqttClient.connect(MQTT_CLIENT_ID, MQTT_USER_NAME, MQTT_PASSWORD);
    }

    bool DoRing = false;

    if (BellRing)
    {
        BellRing = false;
        DoRing = BellEnabled;
    }

    if (DoRing)
    {
        // If MQTT connected simple send notification.
        if (MqttClient.connected())
            MqttClient.publish(MQTT_DOORBELL_TOPIC, MQTT_DOORBELL_MESSAGE);
    }

    homeSpan.poll();

    if (DoRing)
    {
        digitalWrite(BELL_SIGNAL_PIN, HIGH);
        delay(BELL_SIGNAL_DURATION);
        digitalWrite(BELL_SIGNAL_PIN, LOW);
    }
}

/**************************************************************************************/
