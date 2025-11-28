#include "mqtt_handler.h"

#include "config.h"
#include "device_id.h"
#include "eeprom_config.h"
#include "secrets.h"
#include "system_state.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Log.h>

const int MQTT_MAX_MESSAGE_SIZE = 8192;
const int MAX_MQTT_SUBSCRIPTIONS = 10;

WiFiClientSecure net;
PubSubClient mqttClient(net);

// Device certificate selection based on MAC address
static String deviceCert;
static String devicePrivateKey;

// Queue for outgoing MQTT messages (thread-safe publish)
QueueHandle_t mqttPublishQueue = NULL;

// Mutex for subscription registration (thread-safe subscribe)
SemaphoreHandle_t subscriptionMutex = NULL;

// Callback registration system
struct MqttSubscription {
    String topic;
    MqttMessageHandler handler;
};

static MqttSubscription subscriptions[MAX_MQTT_SUBSCRIPTIONS];
static int subscriptionCount = 0;

// Internal: load cert and key from flash
bool loadDeviceCredentialsFromFlash()
{
    Preferences prefs;
    prefs.begin("iot-secrets", true);
    deviceCert = prefs.getString("certificatePem", "");
    devicePrivateKey = prefs.getString("privateKey", "");
    prefs.end();
    Serial.println("DEVICE CERTIFICATE AWS: " + String(deviceCert.length()));
    Serial.println("DEVICE PRIVATE KEY AWS: " + String(devicePrivateKey.length()));
    return (deviceCert.length() > 0 && devicePrivateKey.length() > 0);
}

// Internal: request new cert and key from API
bool requestDeviceCredentialsFromAPI()
{
    HTTPClient https;
    https.begin(IOT_API_ENDPOINT + String("/register-device"));
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", IOT_API_KEY);
    String macAddress = getDeviceId();
    String body = "{\"deviceName\": \"" + macAddress + "\"}";
    int httpCode = https.POST(body);
    if (httpCode == 200)
    {
        String payload = https.getString();
        DynamicJsonDocument doc(4096);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error)
        {
            deviceCert = doc["certificatePem"].as<String>();
            devicePrivateKey = doc["privateKey"].as<String>();
            Preferences prefs;
            prefs.begin("iot-secrets", false); // write mode
            prefs.putString("certificatePem", deviceCert);
            prefs.putString("privateKey", devicePrivateKey);
            prefs.end();
            Serial.println("[Provisioning] Device credentials provisioned and saved.");
            https.end();
            return true;
        }
        else
        {
            Serial.println("[Provisioning] JSON parse error.");
        }
    }
    else
    {
        Serial.println("[Provisioning] HTTP error during device registration: " + String(httpCode));
    }
    https.end();
    return false;
}

// Topics for MQTT communication - keep only OTA (temporary) and healthcheck
String OTA_TOPIC;
String HEALTH_CHECK_TOPIC;

void initializeMQTTHandler(const char* deviceType, const char* deviceId)
{
    String devType = String(deviceType);
    String devId = String(deviceId);
    
    // Only initialize system-level topics (healthcheck, OTA)
    HEALTH_CHECK_TOPIC = "mica/dev/status/" + devType + "/" + devId + "/healthcheck";
    OTA_TOPIC = "mica/dev/command/" + devType + "/" + devId + "/ota";
    
    // Create publish queue (if not already created)
    if (mqttPublishQueue == NULL) {
        mqttPublishQueue = xQueueCreate(MQTT_PUBLISH_QUEUE_SIZE, sizeof(MqttPublishMessage));
        if (mqttPublishQueue == NULL) {
            Log::error("Failed to create MQTT publish queue");
        } else {
            Log::info("MQTT publish queue created (size: %d)", MQTT_PUBLISH_QUEUE_SIZE);
        }
    }
    
    // Create subscription mutex (if not already created)
    if (subscriptionMutex == NULL) {
        subscriptionMutex = xSemaphoreCreateMutex();
        if (subscriptionMutex == NULL) {
            Log::error("Failed to create subscription mutex");
        }
    }
    
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(deviceCert.c_str());
    net.setPrivateKey(devicePrivateKey.c_str());
    mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);
    mqttClient.setCallback(mqttMessageCallback);
    mqttClient.setBufferSize(MQTT_MAX_MESSAGE_SIZE);
    
    // Reset subscription list
    subscriptionCount = 0;
    
    Log::info("MQTT Handler initialized for device type '%s' with ID: %s", deviceType, deviceId);
}

/**
 * @brief Internal callback for PubSubClient - routes messages to registered handlers
 * @param topic Topic of the received message
 * @param payload Payload of the received message
 * @param length Length of the payload
 */
void mqttMessageCallback(char *topic, byte *payload, unsigned int length)
{
    Log::debug("Message received on topic %s:", topic);

    // Convert payload to null-terminated string
    char message[length + 1];
    for (unsigned int i = 0; i < length; i++)
    {
        message[i] = (char)payload[i];
    }
    message[length] = '\0';
    Log::debug("Payload: %s", (const char *)message);

    // Handle OTA topic (temporary - will be moved to ota_manager later)
    if (strcmp(topic, OTA_TOPIC.c_str()) == 0)
    {
        Log::info("OTA update command received via dedicated topic.");
        StaticJsonDocument<2048> doc;
        DeserializationError err = deserializeJson(doc, message);
        if (err)
        {
            Log::error("Failed to parse OTA JSON: %s", err.c_str());
            return;
        }
        String jsonPretty;
        serializeJsonPretty(doc, jsonPretty);
        Log::debug("Full parsed JSON (pretty):\n%s", jsonPretty.c_str());
        const char *firmwareUrl = doc["firmwareUrl"];
        if (!firmwareUrl || strlen(firmwareUrl) == 0)
        {
            Log::error("No firmwareUrl in OTA message.");
            return;
        }
        Preferences preferences;
        preferences.begin("ota", false);
        preferences.putString("url", firmwareUrl);
        preferences.end();
        Log::info("Firmware URL length: %d", strlen(firmwareUrl));
        Log::info("Stored firmwareUrl to EEPROM: %s", firmwareUrl);
        notifySystemState(EVENT_OTA_UPDATE);
        return;
    }

    // Route to registered handlers
    bool handlerFound = false;
    for (int i = 0; i < subscriptionCount; i++)
    {
        if (strcmp(topic, subscriptions[i].topic.c_str()) == 0)
        {
            subscriptions[i].handler(topic, message, length);
            handlerFound = true;
            break;
        }
    }

    if (!handlerFound)
    {
        Log::warn("No handler registered for topic: %s", topic);
    }
}

bool connectMQTTClient(String deviceId)
{
    initializeMQTTHandler("recirculator", deviceId.c_str());
    if (mqttClient.connect(deviceId.c_str()))
    {
        // Subscribe to OTA topic (temporary - will be moved to ota_manager)
        if (mqttClient.subscribe(OTA_TOPIC.c_str()))
        {
            Log::info("Subscribed to OTA topic: %s", OTA_TOPIC.c_str());
        }
        else
        {
            Log::error("Failed to subscribe to OTA topic: %s", OTA_TOPIC.c_str());
        }
        
        // Subscribe to all registered topics
        for (int i = 0; i < subscriptionCount; i++)
        {
            if (mqttClient.subscribe(subscriptions[i].topic.c_str()))
            {
                Log::info("Subscribed to topic: %s", subscriptions[i].topic.c_str());
            }
            else
            {
                Log::error("Failed to subscribe to topic: %s", subscriptions[i].topic.c_str());
            }
        }
        
        return true;
    }

    return false;
}

/**
 * @brief Connects to the MQTT broker with the device ID.
 * @return true if the connection is successful, false otherwise.
 */
bool connectMQTT()
{
    if (mqttClient.connected())
    {
        Log::info("MQTT is already connected. Skipping connection attempt.");
        return true;
    }

    // Ensure credentials are loaded before attempting connection
    if (deviceCert.length() == 0 || devicePrivateKey.length() == 0)
    {
        Log::error("Device credentials not loaded. Cannot connect to MQTT.");
        return false;
    }

    Log::warn("Attempting to connect to MQTT...");
    String deviceId = getDeviceId();
    int maxRetries = 3;
    for (int attempt = 1; attempt <= maxRetries; attempt++)
    {
        Log::info("MQTT Connection Attempt %d/%d", attempt, maxRetries);
        if (connectMQTTClient(deviceId))
        {
            Log::info("Successfully connected to MQTT with client ID: %s. Notifying EVENT_MQTT_CONNECTED.", deviceId.c_str());
            notifySystemState(EVENT_MQTT_CONNECTED);
            return true;
        }
        Log::warn("MQTT connection failed. Retrying in 2 seconds...");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    Log::error("Failed to connect to MQTT after %d attempts. Notifying EVENT_MQTT_DISCONNECTED.", maxRetries);
    notifySystemState(EVENT_MQTT_DISCONNECTED);
    return false;
}

void mqttConnectTask(void *pvParameters)
{
    while (true)
    {
        // Wait for WiFi to be connected before attempting to get credentials
        if (WiFi.status() != WL_CONNECTED)
        {
            Log::warn("WiFi disconnected or inactive. Notifying EVENT_WIFI_DISCONNECTED.");
            notifySystemState(EVENT_WIFI_DISCONNECTED);
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (loadDeviceCredentialsFromFlash())
        {
            Log::info("Credentials loaded from flash.");
            notifySystemState(EVENT_MQTT_AWS_CREDENTIALS);
        }
        else
        {
            Log::info("No credentials found, requesting from API...");
            if (requestDeviceCredentialsFromAPI())
            {
                Log::info("AWS Credentials obtained successfully!");
                notifySystemState(EVENT_MQTT_AWS_CREDENTIALS);
            }
            else
            {
                Log::warn("Failed to obtain AWS credentials");
            }
        }
        // Short delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void mqttPublishTask(void *pvParameters)
{
    MqttPublishMessage msg;
    TickType_t lastHealthCheck = 0;
    const TickType_t healthCheckInterval = pdMS_TO_TICKS(60000); // 60 seconds

    while (true)
    {
        SystemState currentState = getSystemState();
        bool mqttConnected = mqttClient.connected();

        // Process incoming MQTT messages
        mqttClient.loop();

        // Case 1: MQTT connected but system state not updated
        if (mqttConnected && currentState != SYSTEM_STATE_CONNECTED_MQTT)
        {
            Log::info("MQTT connected but state incorrect. Notifying EVENT_MQTT_CONNECTED.");
            notifySystemState(EVENT_MQTT_CONNECTED);
        }

        // Case 2: MQTT not connected, attempt reconnection if WiFi is active
        if (!mqttConnected)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                Log::info("WiFi active. Attempting MQTT connection...");
                connectMQTT();
            }
            else
            {
                Log::error("WiFi disconnected or inactive. Notifying EVENT_WIFI_DISCONNECTED.");
                notifySystemState(EVENT_WIFI_DISCONNECTED);
            }
            
            // Delay before retry
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Case 3: Process queued publish messages
        if (xQueueReceive(mqttPublishQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (mqttClient.connected())
            {
                bool published = mqttClient.publish(msg.topic, msg.payload, msg.retain);
                if (published)
                {
                    Log::debug("Published to %s: %s", msg.topic, msg.payload);
                }
                else
                {
                    Log::error("Failed to publish to %s. MQTT State: %d", msg.topic, mqttClient.state());
                    // TODO: Consider re-queuing or storing failed messages
                }
            }
            else
            {
                Log::warn("MQTT disconnected while processing queue. Message to %s dropped.", msg.topic);
            }
        }

        // Case 4: Publish periodic health check
        TickType_t now = xTaskGetTickCount();
        if (mqttConnected && (now - lastHealthCheck >= healthCheckInterval))
        {
            publishHealthCheck(millis());
            lastHealthCheck = now;
        }

        // Small delay to prevent task starvation (only if queue was empty)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief Publishes a health check message to MQTT.
 * @param uptime Device uptime in milliseconds
 * @return true if publishing succeeds, false otherwise.
 */
bool publishHealthCheck(uint64_t uptime)
{
    const size_t capacity = 256;
    DynamicJsonDocument doc(capacity);
    doc["uptime"] = uptime;
    doc["freeHeap"] = ESP.getFreeHeap();
    String jsonString;
    serializeJson(doc, jsonString);

    // Use generic publish function (goes through queue)
    return mqttPublish(HEALTH_CHECK_TOPIC.c_str(), jsonString.c_str(), false);
}

//------------------------------------------------------------------------------
// Generic MQTT API - Device-agnostic publish/subscribe functions
//------------------------------------------------------------------------------

bool mqttPublish(const char* topic, const char* payload, bool retain)
{
    if (mqttPublishQueue == NULL)
    {
        Log::error("MQTT publish queue not initialized");
        return false;
    }

    // Validate input sizes
    if (strlen(topic) >= MQTT_TOPIC_MAX_LENGTH)
    {
        Log::error("Topic too long (%d bytes): %s", strlen(topic), topic);
        return false;
    }
    if (strlen(payload) >= MQTT_PAYLOAD_MAX_LENGTH)
    {
        Log::error("Payload too long (%d bytes) for topic: %s", strlen(payload), topic);
        return false;
    }

    // Create message structure
    MqttPublishMessage msg;
    strncpy(msg.topic, topic, MQTT_TOPIC_MAX_LENGTH - 1);
    msg.topic[MQTT_TOPIC_MAX_LENGTH - 1] = '\0';
    strncpy(msg.payload, payload, MQTT_PAYLOAD_MAX_LENGTH - 1);
    msg.payload[MQTT_PAYLOAD_MAX_LENGTH - 1] = '\0';
    msg.retain = retain;

    // Enqueue message (wait up to 100ms if queue is full)
    if (xQueueSend(mqttPublishQueue, &msg, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        Log::debug("Enqueued MQTT message to %s", topic);
        return true;
    }
    else
    {
        Log::error("MQTT publish queue full! Dropped message to: %s", topic);
        return false;
    }
}

bool mqttSubscribe(const char* topic, MqttMessageHandler handler)
{
    if (subscriptionMutex == NULL)
    {
        Log::error("Subscription mutex not initialized");
        return false;
    }

    // Acquire mutex for thread-safe subscription registration
    if (xSemaphoreTake(subscriptionMutex, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        Log::error("Failed to acquire subscription mutex");
        return false;
    }

    bool result = false;

    if (subscriptionCount >= MAX_MQTT_SUBSCRIPTIONS)
    {
        Log::error("Maximum MQTT subscriptions (%d) reached. Cannot subscribe to: %s", 
                   MAX_MQTT_SUBSCRIPTIONS, topic);
        goto cleanup;
    }

    // Check if already subscribed
    for (int i = 0; i < subscriptionCount; i++)
    {
        if (subscriptions[i].topic == topic)
        {
            Log::warn("Already subscribed to: %s", topic);
            result = true;
            goto cleanup;
        }
    }

    // Register subscription
    subscriptions[subscriptionCount].topic = String(topic);
    subscriptions[subscriptionCount].handler = handler;
    subscriptionCount++;

    Log::info("Registered subscription %d/%d for topic: %s", subscriptionCount, MAX_MQTT_SUBSCRIPTIONS, topic);

    // If already connected, subscribe immediately
    if (mqttClient.connected())
    {
        if (mqttClient.subscribe(topic))
        {
            Log::info("Subscribed to topic: %s", topic);
            result = true;
        }
        else
        {
            Log::error("Failed to subscribe to topic: %s", topic);
            result = false;
        }
    }
    else
    {
        // If not connected, subscription will happen when connection is established
        result = true;
    }

cleanup:
    xSemaphoreGive(subscriptionMutex);
    return result;
}

bool isMqttConnected()
{
    return mqttClient.connected();
}