// mqtt_handler.h
#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>

// MQTT Handler Module
// Purpose: Generic MQTT communication layer for AWS IoT Core
// Architecture: Device-agnostic transport layer with FreeRTOS queue-based messaging
// Thread-Safety: Uses FreeRTOS queues for publish, mutex for subscriptions
// 
// Usage Pattern:
// 1. Modules construct their own topics (e.g., "mica/dev/telemetry/{deviceType}/{deviceId}/temperature")
// 2. Modules construct their own JSON payloads
// 3. Modules call mqttPublish(topic, payload, retain) → enqueues message
// 4. mqttPublishTask() dequeues and publishes to broker
// 5. Modules register callbacks via mqttSubscribe(topic, handler) to receive commands
// 6. mqttMessageCallback() routes incoming messages to registered handlers

// Maximum message sizes
#define MQTT_TOPIC_MAX_LENGTH 128
#define MQTT_PAYLOAD_MAX_LENGTH 512
#define MQTT_PUBLISH_QUEUE_SIZE 20

/**
 * @brief Structure for queued MQTT publish messages
 */
typedef struct {
    char topic[MQTT_TOPIC_MAX_LENGTH];
    char payload[MQTT_PAYLOAD_MAX_LENGTH];
    bool retain;
} MqttPublishMessage;

/**
 * @brief Callback function type for MQTT message handlers
 * @param topic The message topic
 * @param payload The message payload (null-terminated string)
 * @param length The payload length
 */
typedef void (*MqttMessageHandler)(const char* topic, const char* payload, unsigned int length);

/**
 * @brief Initializes the MQTT handler with secure client settings for AWS IoT.
 * @param deviceType Type of device (e.g., "recirculator", "gateway", "sensor")
 * @param deviceId Unique device identifier (MAC address)
 */
void initializeMQTTHandler(const char* deviceType, const char* deviceId);

/**
 * @brief Connects to the MQTT broker with retries, notifying system events.
 * @return true if connection succeeds, false otherwise.
 */
bool connectMQTT();

/**
 * @brief Callback for processing incoming MQTT messages.
 * @param topic The message topic.
 * @param payload The message content.
 * @param length The payload length.
 */
void mqttMessageCallback(char* topic, byte* payload, unsigned int length);

void mqttConnectTask(void *pvParameters);

/**
 * @brief FreeRTOS task that publishes messages to an MQTT topic.
 * @param pvParameters Task parameters (not used).
 */
void mqttPublishTask(void *pvParameters);

/**
 * @brief Generic MQTT publish function - publishes any topic/payload
 * @param topic Full MQTT topic string (e.g., "mica/dev/telemetry/recirculator/AABBCC/temperature")
 * @param payload Message payload (JSON string or plain text)
 * @param retain Whether to retain the message on the broker
 * @return true if publishing succeeds, false otherwise
 * 
 * @note Thread-safe: Can be called from any task
 * @note Caller is responsible for constructing topic and payload
 */
bool mqttPublish(const char* topic, const char* payload, bool retain = false);

/**
 * @brief Subscribe to an MQTT topic with a callback handler
 * @param topic Full MQTT topic string to subscribe to (exact match, no wildcards)
 * @param handler Callback function invoked when message received on this topic
 * @return true if subscription succeeds, false otherwise
 * 
 * @note Must be called after MQTT connection is established
 * @note Maximum 10 topic subscriptions supported
 */
bool mqttSubscribe(const char* topic, MqttMessageHandler handler);

/**
 * @brief Check if MQTT client is connected
 * @return true if connected, false otherwise
 */
bool isMqttConnected();

/**
 * @brief Publishes system health check message to MQTT
 * @param uptime Device uptime in milliseconds
 * @return true if publishing succeeds, false otherwise
 * 
 * @note This is a system-level function (not device-specific)
 * @note Topic: mica/dev/status/{deviceType}/{deviceId}/healthcheck
 */
bool publishHealthCheck(uint64_t uptime);

#endif