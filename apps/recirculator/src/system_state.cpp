// system_state.cpp
// System State Management Module
// Purpose: Orchestrates system-wide state transitions and task lifecycle management
// Architecture: Event-driven FSM with FreeRTOS task coordination
// Thread-Safety: Uses mutexes for state access, task notifications for events
// Dependencies: All system modules (WiFi, MQTT, sensors, drivers, etc.)

#include "system_state.h"

// Project headers (alphabetically)
#include "button_manager.h"
#include "config.h"
#include "device_id.h"
#include "display_manager.h"
#include "eeprom_config.h"
#include "led_manager.h"
#include "mqtt_handler.h"
#include "ota_manager.h"
#include "relay_controller.h"
#include "temperature_sensor.h"
#include "wifi_config_mode.h"
#include "wifi_connect.h"

// Third-party libraries
#include <Arduino.h>
#include <Log.h>
#include <UtcClock.h>

// System headers
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h> 

// Internal Variables
volatile SystemState g_systemState = SYSTEM_STATE_CONNECTING; // Initial system state
static SemaphoreHandle_t g_stateMutex = NULL;                 // Mutex to protect the system state
static TaskHandle_t g_stateManagerTaskHandle = NULL;          // Handle for the state management task
static SystemState lastLoggedState = SYSTEM_STATE_ERROR;      // Last logged state

// Task Handles
static TaskHandle_t g_wifiConnectTaskHandle = NULL;    // WiFi connection task
static TaskHandle_t g_wifiConfigTaskHandle = NULL;     // WiFi configuration mode task
static TaskHandle_t g_mqttConnectTaskHandle = NULL;    // MQTT connection task (AWS Credencials)
static TaskHandle_t g_mqttTaskHandle = NULL;           // MQTT task
static TaskHandle_t g_ledTaskHandle = NULL;            // LED management task
static TaskHandle_t g_buttonTaskHandle = NULL;         // Button management task
static TaskHandle_t g_otaTaskHandle = NULL;            // OTA update task
static TaskHandle_t g_displayManagerTaskHandle = NULL; // Display manager task
static TaskHandle_t g_temperatureSensorTaskHandle = NULL; // Temperature sensor task
static TaskHandle_t g_relayTaskHandle = NULL;          // Relay controller task

void setOtaTaskHandle(TaskHandle_t handle) {
    g_otaTaskHandle = handle;
}

// Internal Function Declarations
static void stateManagementTask(void *pvParameters);   // Main state management task
static void handleStateTransitions();                  // Handles state transitions based on events
static void handleStateActions();                      // Executes actions corresponding to the current state
static bool initializeLogSystem();                     // Initializes the logging system
static void logTask(void *pvParameters);               // Task that processes log messages

//------------------------------------------------------------------------------
// System Initialization
//------------------------------------------------------------------------------
bool initializeSystemState() {
    // Create mutex to protect the system state
    g_stateMutex = xSemaphoreCreateMutex();
    if (g_stateMutex == NULL) {
        Log::error("Failed to create g_stateMutex.");
        return false;
    }

    // Relay state mutex is managed by relay_controller.cpp

    if (!eepromInitialize()) {
        return false;
    }

    if (!initializeLogSystem()) {
        Log::error("Failed to initialize log system.");
        return false;
    }

    initializeLedManager();
    initializeButtonManager();

    if (!initializeWiFiConnection()) {
        return false;
    }

    if (!initializeDisplayManager()) {
        Log::error("Failed to initialize Display Manager.");
        return false;
    } // Inicialitza la display OLED

    initializeOTAManager();

    if (!initializeTemperatureSensor()) {
        Log::error("Failed to initialize Temperature Sensor.");
        return false;
    }

    // Create State Management Task FIRST to avoid NULL handle errors
    if (xTaskCreate(stateManagementTask, "State Management Task", 4096, NULL, 3, &g_stateManagerTaskHandle) != pdPASS) {
        Log::error("Failed to create State Management Task.");
        return false;
    }
    
    // Give state manager time to initialize
    vTaskDelay(pdMS_TO_TICKS(100));

    // Create System Tasks with logs and verify their creation
    if (xTaskCreate(wifiConnectTask, "WiFi Connect Task", 4096, NULL, 2, &g_wifiConnectTaskHandle) != pdPASS) {
        Log::error("Failed to create WiFi Connect Task.");
        return false;
    }

    if (xTaskCreate(wifiConfigModeTask, "WiFi Config Mode Task", 4096, NULL, 2, &g_wifiConfigTaskHandle) != pdPASS) {
        Log::error("Failed to create WiFi Config Mode Task.");
        return false;
    }

    if (xTaskCreate(mqttConnectTask, "WiFi Config Mode Task", 4096, NULL, 2, &g_mqttConnectTaskHandle) != pdPASS) {
        Log::error("Failed to create WiFi Config Mode Task.");
        return false;
    }

    if (xTaskCreate(mqttPublishTask, "MQTT Task", 10000, NULL, 2, &g_mqttTaskHandle) != pdPASS) {
        Log::error("Failed to create MQTT Task.");
        return false;
    }

    if (xTaskCreate(temperatureSensorTask, "Temperature Sensor Task", 4096, NULL, 1, &g_temperatureSensorTaskHandle) != pdPASS) {
        Log::error("Failed to create Temperature Sensor Task.");
        return false;
    }

    if (xTaskCreate(displayManagerTask, "Display Manager Task", 4096, NULL, 1, &g_displayManagerTaskHandle) != pdPASS) {
        Log::error("Failed to create Display Manager Task.");
        return false;
    }

    if (xTaskCreate(ledTask, "LED Task", 2048, NULL, 1, &g_ledTaskHandle) != pdPASS) {
        Log::error("Failed to create LED Task.");
        return false;
    }

    if (xTaskCreate(buttonTask, "Button Task", 2048, NULL, 1, &g_buttonTaskHandle) != pdPASS) {
        Log::error("Failed to create Button Task.");
        return false;
    }

    // Create relay controller task (always active)
    if (xTaskCreate(relayControllerTask, "Relay Task", 2048, NULL, 1, &g_relayTaskHandle) != pdPASS) {
        Log::error("Failed to create Relay Task.");
        return false;
    }

    Log::info("System Initialization completed successfully.\n");
    return true;
}

// System State Management
void setSystemState(SystemState state) {
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY)) {
        g_systemState = state;
        Log::info("System state updated to: %d", state);
        xSemaphoreGive(g_stateMutex);
    }
}

SystemState getSystemState() {
    SystemState state;
    if (xSemaphoreTake(g_stateMutex, portMAX_DELAY)) {
        state = g_systemState;
        xSemaphoreGive(g_stateMutex);
    } else {
        Log::error("Failed to acquire mutex in getSystemState.");
        state = SYSTEM_STATE_ERROR;
    }
    return state;
}

// Relay state functions removed - use activateRelay()/deactivateRelay() from relay_controller.h directly

// Log System
static bool initializeLogSystem() {
    if (!Log::init()) {
        Log::error("Failed to init log system.");
        return false;
    }

    if (xTaskCreate(logTask, "Log Task", 2048, NULL, 1, NULL) != pdPASS) {
        Log::error("Failed to create Log Task.");
        return false;
    }
    return true;
}

static void logTask(void *pvParameters) {
    while (true) {
        Log::process(&Serial);
    }
}

void logTaskStatus() {
    static char lastStatus[256] = ""; // Almacena el último estado de tareas para evitar duplicados
    char currentStatus[256]; // Estado actual de las tareas

    snprintf(currentStatus, sizeof(currentStatus),
        "WiFi Connect Task: %s\n"
        "WiFi Config Mode Task: %s\n"
        "MQTT Connect Task: %s\n"
        "MQTT Task: %s\n"
        "Temperature Sensor Task: %s\n"
        "Display Manager Task: %s\n"
        "LED Task: %s\n"
        "Button Task: %s\n",
        (g_wifiConnectTaskHandle && eTaskGetState(g_wifiConnectTaskHandle) == eSuspended) ? "SUSPENDED" : "ACTIVE",
        (g_wifiConfigTaskHandle && eTaskGetState(g_wifiConfigTaskHandle) == eSuspended) ? "SUSPENDED" : "ACTIVE",
        (g_mqttConnectTaskHandle && eTaskGetState(g_mqttConnectTaskHandle) == eSuspended) ? "SUSPENDED" : "ACTIVE",
        (g_mqttTaskHandle && eTaskGetState(g_mqttTaskHandle) == eSuspended) ? "SUSPENDED" : "ACTIVE",
        (g_temperatureSensorTaskHandle && eTaskGetState(g_temperatureSensorTaskHandle) == eSuspended) ? "SUSPENDED" : "ACTIVE",
        (g_displayManagerTaskHandle && eTaskGetState(g_displayManagerTaskHandle) == eSuspended) ? "SUSPENDED" : "ACTIVE",
        (g_ledTaskHandle ? "ACTIVE" : "ERROR (Not Created)"),
        (g_buttonTaskHandle ? "ACTIVE" : "ERROR (Not Created)")
    );

    // Solo imprimir si hay cambios en el estado de las tareas
    if (strcmp(currentStatus, lastStatus) != 0) {
        Serial.println("\n===== Task Status =====");
        Serial.println(currentStatus);
        strcpy(lastStatus, currentStatus); // Update the last registered state
    }
}

// Event Handling and Transitions
void notifySystemState(TaskNotificationEvent event) {
    if (g_stateManagerTaskHandle != NULL) {
        xTaskNotify(g_stateManagerTaskHandle, event, eSetBits);
    } else {
        Log::error("notifySystemState State Manager Task Handle is NULL.");
    }
}

/** @brief Waits to receive event notifications and returns the received event.
 * @param waitTime Time to wait for notifications in ticks.
 * @return TaskNotificationEvent The received event notification, or 0 if no notification was received.
 */
static TaskNotificationEvent receiveSystemStateNotification(TickType_t waitTime) {
    uint32_t receivedBits;
    if (xTaskNotifyWait(0, 0xFFFFFFFF, &receivedBits, waitTime) == pdPASS) {
        return (TaskNotificationEvent)receivedBits;
    }
    return (TaskNotificationEvent)0;
}

/** @brief Handles system state transitions when an event is received.
 */
static void handleStateTransitions() {
    TaskNotificationEvent event = receiveSystemStateNotification(pdMS_TO_TICKS(50));

    if (event == 0) return;

    // Handle button events globally, in any state
    if (event & EVENT_LONG_PRESS_BUTTON) {
        Log::info("Long press button event received. Transitioning to CONFIG_MODE.");
        setSystemState(SYSTEM_STATE_CONFIG_MODE);
        return; // Process this event exclusively
    }
    
    if (event & EVENT_SHORT_PRESS_BUTTON) {
        Log::info("Short press button event received. Toggling relay.");
        // Toggle relay: if active, deactivate; if inactive, activate
        if (isRelayActive()) {
            deactivateRelay("button");
        } else {
            activateRelay();
        }
    }

    // Handle relay events globally, in any state
    if (event & EVENT_RELAY_ON) {
        Log::info("EVENT_RELAY_ON received. Activating relay.");
        activateRelay(); // Call centralized function directly
    }
    
    if (event & EVENT_RELAY_OFF) {
        Log::info("EVENT_RELAY_OFF received. Deactivating relay.");
        deactivateRelay("command"); // Call centralized function directly
    }
    
    if (event & EVENT_RELAY_STOPPED) {
        Log::info("EVENT_RELAY_STOPPED received. Relay stopped automatically.");
        // Could publish MQTT notification here if needed
    }

    SystemState currentState = getSystemState();
    switch (currentState) {
        case SYSTEM_STATE_CONNECTING:
            if (event & EVENT_WIFI_CONNECTED) {
                Log::info("WiFi connected. Transitioning to CONNECTED_WIFI.");
                setSystemState(SYSTEM_STATE_CONFIG_MQTT);
            }
            if (event & EVENT_NO_PARAMETERS_EEPROM) {
                Log::warn("No WiFi parameters in EEPROM. Transitioning to CONFIG_MODE.");
                setSystemState(SYSTEM_STATE_CONFIG_MODE);
            }
            if (event & EVENT_WIFI_FAIL_CONNECT) {
                Log::error("WiFi connection failed. Trying again...");
            }
            break;

        case SYSTEM_STATE_CONFIG_MQTT:
            if (event & EVENT_MQTT_AWS_CREDENTIALS) {
                Log::info("AWS credentials accquired.");
                setSystemState(SYSTEM_STATE_CONNECTED_WIFI);
            }
            break;

        case SYSTEM_STATE_CONNECTED_WIFI:
            if (event & EVENT_MQTT_CONNECTED) {
                Log::info("MQTT connected. Transitioning to CONNECTED_MQTT.");
                setSystemState(SYSTEM_STATE_CONNECTED_MQTT);
                // Initialize relay controller MQTT subscriptions after connection
                initializeRelayController();
            }
            break;

        case SYSTEM_STATE_CONNECTED_MQTT:
            if (event & EVENT_MQTT_DISCONNECTED) {
                Log::warn("MQTT disconnected. Downgrading to CONNECTED_WIFI.");
                setSystemState(SYSTEM_STATE_CONFIG_MQTT);
            }
            if (event & EVENT_WIFI_DISCONNECTED) {
                Log::warn("WiFi disconnected. Downgrading to CONNECTING.");
                setSystemState(SYSTEM_STATE_CONNECTING);
            }
            if (event & EVENT_OTA_UPDATE) {
                Log::info("OTA update event received. Transitioning to OTA_UPDATE state.");
                setSystemState(SYSTEM_STATE_OTA_UPDATE);
            }
            break;

        case SYSTEM_STATE_CONFIG_MODE:
            if (event & EVENT_WIFI_CONNECTED) {
                Log::info("Connected to wifi while in SYSTEM_STATE_CONFIG_MODE.");
                setSystemState(SYSTEM_STATE_CONFIG_MQTT);
            }
            // NOTE: A long press in this mode will just re-trigger the same state.
            break;

        case SYSTEM_STATE_ERROR:
            Log::error("Critical system error detected. Restarting device in 5 seconds...");
            break;

        default:
            Log::error("Unknown system state: %d", currentState);
            break;
    }
}

/** @brief Executes the actions associated with each system state.
 */
static void handleStateActions() {
    SystemState currentState = getSystemState();
   
    logTaskStatus(); // Print tasks status

    switch (currentState) {
        case SYSTEM_STATE_CONNECTING:
            if (g_wifiConnectTaskHandle) vTaskResume(g_wifiConnectTaskHandle);
            if (g_wifiConfigTaskHandle) vTaskSuspend(g_wifiConfigTaskHandle);
            if (g_mqttConnectTaskHandle) vTaskSuspend(g_mqttConnectTaskHandle);
            if (g_mqttTaskHandle) vTaskSuspend(g_mqttTaskHandle);
            if (g_displayManagerTaskHandle) vTaskResume(g_displayManagerTaskHandle);
            if (g_temperatureSensorTaskHandle) vTaskResume(g_temperatureSensorTaskHandle);
            if (g_buttonTaskHandle) vTaskResume(g_buttonTaskHandle);
            break;

        case SYSTEM_STATE_CONNECTED_WIFI:
            if (g_wifiConnectTaskHandle) vTaskResume(g_wifiConnectTaskHandle);
            if (g_wifiConfigTaskHandle) vTaskSuspend(g_wifiConfigTaskHandle);
            if (g_mqttConnectTaskHandle) vTaskSuspend(g_mqttConnectTaskHandle);
            if (g_mqttTaskHandle) vTaskResume(g_mqttTaskHandle);
            if (g_displayManagerTaskHandle) vTaskResume(g_displayManagerTaskHandle);
            if (g_temperatureSensorTaskHandle) vTaskResume(g_temperatureSensorTaskHandle);
            if (g_buttonTaskHandle) vTaskResume(g_buttonTaskHandle);
            break;

        case SYSTEM_STATE_CONFIG_MQTT:
            if (g_wifiConnectTaskHandle) vTaskResume(g_wifiConnectTaskHandle);
            if (g_wifiConfigTaskHandle) vTaskSuspend(g_wifiConfigTaskHandle);
            if (g_mqttConnectTaskHandle) vTaskResume(g_mqttConnectTaskHandle);
            if (g_mqttTaskHandle) vTaskSuspend(g_mqttTaskHandle);
            if (g_displayManagerTaskHandle) vTaskResume(g_displayManagerTaskHandle);
            if (g_temperatureSensorTaskHandle) vTaskResume(g_temperatureSensorTaskHandle);
            if (g_buttonTaskHandle) vTaskResume(g_buttonTaskHandle);
            break;

        case SYSTEM_STATE_CONNECTED_MQTT:
            if (g_wifiConnectTaskHandle) vTaskResume(g_wifiConnectTaskHandle);
            if (g_wifiConfigTaskHandle) vTaskSuspend(g_wifiConfigTaskHandle);
            if (g_mqttConnectTaskHandle) vTaskSuspend(g_mqttConnectTaskHandle);
            if (g_mqttTaskHandle) vTaskResume(g_mqttTaskHandle);
            if (g_displayManagerTaskHandle) vTaskResume(g_displayManagerTaskHandle);
            if (g_temperatureSensorTaskHandle) vTaskResume(g_temperatureSensorTaskHandle);
            if (g_buttonTaskHandle) vTaskResume(g_buttonTaskHandle);
            break;

        case SYSTEM_STATE_CONFIG_MODE:
            if (g_wifiConnectTaskHandle) vTaskSuspend(g_wifiConnectTaskHandle);
            if (g_wifiConfigTaskHandle) vTaskResume(g_wifiConfigTaskHandle);
            if (g_mqttConnectTaskHandle) vTaskSuspend(g_mqttConnectTaskHandle);
            if (g_mqttTaskHandle) vTaskSuspend(g_mqttTaskHandle);
            if (g_displayManagerTaskHandle) vTaskResume(g_displayManagerTaskHandle);
            if (g_temperatureSensorTaskHandle) vTaskResume(g_temperatureSensorTaskHandle);
            if (g_buttonTaskHandle) vTaskResume(g_buttonTaskHandle);
            break;

        case SYSTEM_STATE_OTA_UPDATE:
            if (g_otaTaskHandle == NULL) {
                if (g_wifiConnectTaskHandle) vTaskSuspend(g_wifiConnectTaskHandle);
                if (g_mqttConnectTaskHandle) vTaskSuspend(g_mqttConnectTaskHandle);
                if (g_mqttTaskHandle) vTaskSuspend(g_mqttTaskHandle);
                if (g_displayManagerTaskHandle) vTaskSuspend(g_displayManagerTaskHandle);
                if (g_temperatureSensorTaskHandle) vTaskSuspend(g_temperatureSensorTaskHandle);
                if (g_buttonTaskHandle) vTaskSuspend(g_buttonTaskHandle);

                if (xTaskCreate(otaTask, "OTA Task", 4096, NULL, 3, &g_otaTaskHandle) != pdPASS) {
                    Log::error("Failed to create OTA Task.");
                    setSystemState(SYSTEM_STATE_ERROR);
                }
            }
            break;

        case SYSTEM_STATE_ERROR:
            if (g_wifiConnectTaskHandle) vTaskSuspend(g_wifiConnectTaskHandle);
            if (g_wifiConfigTaskHandle) vTaskSuspend(g_wifiConfigTaskHandle);
            if (g_mqttConnectTaskHandle) vTaskSuspend(g_mqttConnectTaskHandle);
            if (g_mqttTaskHandle) vTaskSuspend(g_mqttTaskHandle);
            if (g_displayManagerTaskHandle) vTaskSuspend(g_displayManagerTaskHandle);
            if (g_temperatureSensorTaskHandle) vTaskSuspend(g_temperatureSensorTaskHandle);
            if (g_buttonTaskHandle) vTaskSuspend(g_buttonTaskHandle);

            vTaskDelay(pdMS_TO_TICKS(5000));
            ESP.restart();
            break;

        default:
            break;
    }
}

// ===================================================================================
// MAIN SYSTEM MANAGEMENT TASK
// ===================================================================================
/** @brief Main task that handles the system state.
 * @param pvParameters Parameters passed to the task (Not used).
 */
static void stateManagementTask(void *pvParameters) {
    while (true) {
        handleStateTransitions();
        handleStateActions();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
