# GitHub Copilot Instructions for MICA Recirculator

> **🎓 Architecture by gaesca04**  
> This project follows professional software engineering standards recommended by **gaesca04** (computer engineer, software architecture expert). All development follows monorepo best practices learned in advanced academic coursework.

## General Guidelines

### Work Approach
- **Follow instructions precisely**: Do not add creative suggestions or extra features unless explicitly requested
- **Ask before applying**: When you have suggestions for improvements, present them for validation before implementing
- **Be conservative**: Stick to the task at hand, avoid scope creep
- **Professional standard**: All work must meet gaesca04's software engineering standards

### Standard Workflow (Step-by-Step)

**For ANY code change, follow this exact process:**

1. **📋 Plan**: Identify the issue/task from `ISSUES.md`
2. **🔍 Analyze**: Read relevant files, understand current implementation
3. **✏️ Implement**: Make code changes (edit, create, refactor)
4. **⚙️ Compile**: Run `~/.platformio/penv/bin/platformio run` to verify no errors
5. **✅ Test**: If system is functional, test compilation + basic functionality
6. **📝 Update Docs**: Update `ISSUES.md` to mark issue as completed
7. **💾 Commit**: Create commit with clear message following format
8. **🔄 Sync**: Push changes to GitHub (`git push`)

**For changes to working/production code:**
1. **🌿 Create Branch**: `git checkout -b feature/descriptive-name`
2. **✏️ Implement**: Make changes following steps 3-5 above
3. **💾 Commit**: Commit changes to branch
4. **🔄 Push Branch**: `git push origin feature/descriptive-name`
5. **🔀 Pull Request**: Create PR on GitHub for review
6. **✅ Review & Merge**: After approval, merge to main

**Golden Rules:**
- ✅ **Always compile before committing**
- ✅ **Test functional changes if hardware available**
- ✅ **Update ISSUES.md with progress**
- ✅ **One logical change per commit**
- ✅ **Use branches for non-trivial changes to working code**
- ✅ **Clear commit messages (English, following format)**

## Code Standards and Conventions

### Language Policy
- **All code**: English only (variables, functions, classes, comments)
- **All documentation**: English only (README, architecture, comments)
- **All commit messages**: English only
- **Exception**: User-facing strings can be localized (e.g., OLED display text in Catalan)

**Rationale**: International collaboration, consistency, professional standards

### Naming Conventions

#### Files
```cpp
// C++ source files
relay_controller.cpp      // snake_case for implementation files
mqtt_handler.cpp
system_state.cpp

// C++ header files
relay_controller.h        // snake_case for headers
mqtt_handler.h
system_state.h

// Configuration files
config.h                  // lowercase for config/secrets
secrets.h
platformio.ini
```

#### Variables
```cpp
// Global variables (avoid when possible)
g_systemState             // g_ prefix + camelCase
g_relayTaskHandle

// Local variables
bool isRelayActive        // camelCase
float currentTemperature
uint32_t maxTimeSeconds

// Constants
#define RELAY_PIN 8       // UPPER_SNAKE_CASE for macros
#define MAX_TEMP_ADDR 200

constexpr char AP_SSID[]  // PascalCase for constexpr
constexpr int MQTT_PORT   // UPPER_CASE for const integers
```

#### Functions
```cpp
// Public API - camelCase
void initializeMQTTHandler(const char* deviceType, const char* deviceId);
bool activateRelay();
float getLatestTemperature();

// Private/Static functions - camelCase with descriptive names
static void handleStateTransitions();
static bool loadDeviceCredentials();
static void mqttMessageCallback(char* topic, byte* payload, unsigned int length);
```

#### Classes and Types
```cpp
// Enums - PascalCase for type, UPPER_SNAKE_CASE for values
typedef enum {
    SYSTEM_STATE_CONNECTING,
    SYSTEM_STATE_CONNECTED_WIFI,
    SYSTEM_STATE_CONNECTED_MQTT
} SystemState;

typedef enum {
    EVENT_WIFI_CONNECTED,
    EVENT_MQTT_CONNECTED,
    EVENT_RELAY_ON
} TaskNotificationEvent;

// Structs - PascalCase
struct SensorReading {
    float temperature;
    uint32_t timestamp;
};
```

#### FreeRTOS Tasks
```cpp
// Task functions - descriptive + Task suffix
void wifiConnectTask(void* pvParameters);
void mqttPublishTask(void* pvParameters);
void relayControllerTask(void* pvParameters);
void temperatureSensorTask(void* pvParameters);

// Task handles - g_ prefix + descriptive + TaskHandle suffix
TaskHandle_t g_wifiConnectTaskHandle;
TaskHandle_t g_mqttTaskHandle;
TaskHandle_t g_relayTaskHandle;
```

#### Mutexes and Semaphores
```cpp
// Mutexes - descriptive + Mutex suffix
SemaphoreHandle_t eepromMutex;
SemaphoreHandle_t g_stateMutex;
SemaphoreHandle_t temperatureMutex;
```

### Code Style

#### Indentation and Formatting
```cpp
// Use 4 spaces for indentation (no tabs)
void functionName() {
    if (condition) {
        // Code here
    } else {
        // Code here
    }
}

// Braces on same line for functions and control structures
if (isRelayActive()) {
    deactivateRelay("timeout");
}

// Line length: Max 120 characters (prefer 100)
```

#### Comments
```cpp
// Single-line comments for brief explanations
float temperature = readSensor(); // Read DS18B20 temperature

/**
 * @brief Multi-line Doxygen comments for functions
 * @param deviceType Type of device (e.g., "recirculator", "gateway")
 * @param deviceId Unique device identifier (MAC address)
 * @return true if initialization successful, false otherwise
 */
bool initializeMQTTHandler(const char* deviceType, const char* deviceId);

// Section separators for major code blocks
//------------------------------------------------------------------------------
// System Initialization
//------------------------------------------------------------------------------

// Module headers with purpose
// WiFi Connect Module
// Purpose: Manages WiFi connection with auto-reconnect and exponential backoff
```

#### Header Guards
```cpp
// Use #ifndef guards (not #pragma once for portability)
#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

// Header content here

#endif // MQTT_HANDLER_H
```

#### Include Order
```cpp
// 1. Related header
#include "mqtt_handler.h"

// 2. Project headers (alphabetically)
#include "device_id.h"
#include "eeprom_config.h"
#include "secrets.h"
#include "system_state.h"

// 3. Third-party libraries (alphabetically)
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// 4. System headers (alphabetically)
#include <stdint.h>
#include <string.h>
```

### Architecture Patterns

#### Module Structure
```
module_name/
├── module_name.h        # Public API + documentation
└── module_name.cpp      # Implementation

// Header contains:
// 1. Module purpose comment
// 2. Public constants/enums
// 3. Public function declarations with Doxygen comments
// 4. No implementation (except inline)

// Implementation contains:
// 1. Includes
// 2. Private constants/variables
// 3. Private function declarations (static)
// 4. Public function implementations
// 5. Private function implementations
```

#### Dependency Injection
```cpp
// BAD: Hardcoded strings
void initMQTT() {
    topic = "mica/dev/telemetry/recirculator/" + deviceId;
}

// GOOD: Parameterized
void initMQTT(const char* deviceType, const char* deviceId) {
    snprintf(topic, sizeof(topic), "mica/dev/telemetry/%s/%s", deviceType, deviceId);
}
```

#### Error Handling
```cpp
// Always check return values
bool result = connectWiFi();
if (!result) {
    Log::error("WiFi connection failed");
    notifySystemState(EVENT_WIFI_FAIL_CONNECT);
    return false;
}

// Use early returns for error cases
if (temperature == -127.0f) {
    Log::error("Temperature sensor error");
    return false;
}

// Log errors with context
Log::error("Failed to save temperature %.2f to EEPROM", temperature);
```

### Documentation Standards

#### File Headers
```cpp
// mqtt_handler.cpp
// MQTT Handler Module
// Purpose: Manages secure MQTT communication with AWS IoT Core
// Architecture: Event-driven, publishes telemetry and subscribes to commands
// Thread-Safety: Uses mqttClient internal mutex, publishes from dedicated task
// Dependencies: WiFiClientSecure, PubSubClient, system_state
```

#### Function Documentation
```cpp
/**
 * @brief Activates the relay and starts the safety timer
 * 
 * Turns on GPIO relay pin, loads max time/temperature from EEPROM,
 * starts countdown timer, and publishes power state to MQTT.
 * 
 * @return true if relay activated successfully
 * @return false if relay already active or activation failed
 * 
 * @note Thread-safe: Can be called from any task
 * @note Publishes MQTT message automatically
 * @warning Does not persist state to EEPROM (safety by design)
 */
bool activateRelay();
```

#### Complex Logic Comments
```cpp
// Check timeout condition: elapsed time >= max configured time
// This prevents pump running indefinitely if temperature never reached
if (elapsedSeconds >= maxTimeSeconds) {
    Log::info("Relay timeout reached: %lu/%lu seconds", elapsedSeconds, maxTimeSeconds);
    playGameOverMelody();  // Audio feedback for user
    deactivateRelay("timeout");
    notifySystemState(EVENT_RELAY_STOPPED);
}
```

### Git Workflow

#### Branch Naming
```bash
# Feature branches
feature/mqtt-parametrize-device-type
feature/add-pressure-sensor

# Bugfix branches
fix/relay-timeout-not-working
fix/temperature-sensor-error-handling

# Refactoring branches
refactor/eliminate-includes-h
refactor/consolidate-config

# Documentation branches
docs/update-architecture-monorepo
docs/add-api-reference
```

#### Commit Message Format
```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code restructuring (no functionality change)
- `docs`: Documentation changes
- `test`: Adding/updating tests
- `chore`: Maintenance (dependencies, config, etc.)
- `perf`: Performance improvements

**Example**:
```
refactor(mqtt): Parametrize deviceType for multi-device support

- Add deviceType parameter to initializeMQTTHandler()
- Make MQTT topics dynamic based on device type
- Update main.cpp to pass "recirculator" as deviceType
- Maintain backward compatibility

Preparation for mica-ecosystem monorepo architecture.
Recommended by: gaesca04 (monorepo architecture expert)
```

### Testing Standards

#### Pre-Commit Testing
```bash
# 1. Compilation
platformio run

# 2. Static analysis (if available)
platformio check

# 3. Upload to hardware (if possible)
platformio run --target upload

# 4. Manual functional testing
# - Test changed functionality
# - Test related functionality
# - Check for regressions
```

#### Testing Checklist Template
```markdown
## Testing Checklist - [Feature Name]

### Compilation
- [ ] Compiles without errors
- [ ] Compiles without warnings
- [ ] Build size reasonable (< 1.5MB for ESP32)

### Functionality
- [ ] Primary feature works as expected
- [ ] Edge cases handled
- [ ] Error conditions handled gracefully

### Integration
- [ ] No regressions in existing features
- [ ] MQTT messages correct
- [ ] Display updates correctly
- [ ] LED indicators correct

### Performance
- [ ] No memory leaks (check free heap)
- [ ] Task execution time reasonable
- [ ] WiFi/MQTT reconnect works
```

## PlatformIO Environment

### PlatformIO Command
**IMPORTANT**: Always use the full path to PlatformIO executable:
```bash
~/.platformio/penv/bin/platformio run
```

**Common PlatformIO commands**:
```bash
# Compile
~/.platformio/penv/bin/platformio run

# Upload to device
~/.platformio/penv/bin/platformio run --target upload

# Clean build
~/.platformio/penv/bin/platformio run --target clean

# Monitor serial output
~/.platformio/penv/bin/platformio device monitor
```

**Do NOT use** `platformio` directly as it's not in PATH.

## Pre-Work Checklist

### Before Starting Any Changes
1. **Check for Remote Changes**
   ```bash
   git fetch origin
   git status
   ```
   - If branch is behind: `git pull --rebase origin main`
   - If diverged: Resolve conflicts before starting work
   - Always ensure local is up-to-date with remote before making changes

2. **Verify Clean Working Tree**
   ```bash
   git status
   ```
   - Ensure no uncommitted changes before starting new work
   - Commit or stash any pending work

3. **Check Compilation**
   ```bash
   ~/.platformio/penv/bin/platformio run
   ```
   - Verify code compiles before making changes
   - Establish baseline to identify new issues

## Development Guidelines

### Code Organization
- **Centralized Control**: Relay state managed only in `relay_controller.cpp`
- **Generic Components**: `button_manager` must remain generic (no device-specific logic)
- **Event-Driven**: All modules communicate via events through `system_state`
- **Single Source of Truth**: No duplicate state variables across modules

### Architecture Compatibility
- Maintain compatibility with **MICA Gateway** for common modules:
  - WiFi Connect
  - WiFi Config Mode
  - MQTT Handler
  - Button Manager
  - LED Manager
  - System State
- See `src/gateway_specs.md` for reference architecture

### MQTT Topics
- **Telemetry** (publish): Use `mica/dev/telemetry/recirculator/{deviceId}/...`
- **Commands** (subscribe): Use `mica/dev/command/recirculator/{deviceId}/...`
- **Status** (publish): Use `mica/dev/status/recirculator/{deviceId}/...`
- Always include `deviceId` in payload
- Use `retain` flag for state messages (power-state, configuration)

### EEPROM Persistence Policy
✅ **Save to EEPROM**:
- WiFi credentials
- Configuration (max temperature, max time)
- User preferences

❌ **Do NOT save to EEPROM**:
- Runtime state (relay ON/OFF)
- Temporary data
- Sensor readings

**Reason**: Safety - relay must always start OFF on boot

### Error Handling
- **Temperature Sensor Errors** (-127°C):
  - Log error prominently once
  - Display "ERROR" on OLED
  - **Still publish to MQTT** (backend needs visibility)
  - Continue monitoring for auto-recovery

### Commit Guidelines
1. **One logical change per commit**
2. **Separate unrelated changes into different commits**:
   - ❌ BAD: Combining ISSUES.md update + workflow changes in one commit
   - ✅ GOOD: Commit 1 for ISSUES.md, Commit 2 for workflow changes
   - Each commit should have a single, clear purpose
   - Makes git history readable and revert easier if needed

3. **Clear commit messages**:
   ```
   type(scope): subject
   
   - Detail 1
   - Detail 2
   - Reason/benefit
   ```

4. **Test before committing**:
   - Compilation passes
   - No new errors
   - Functionality verified

5. **Group related changes**:
   - If change affects multiple files for same feature, commit together
   - If unrelated topics, make separate commits
   - Example: Bug fix + documentation update = 2 commits

6. **Close issues with commits**:
   - Use `Closes #N` or `Fixes #N` in commit message footer
   - GitHub will automatically close the issue when commit is pushed
   - Example:
     ```
     refactor(mqtt): Make mqtt_handler generic
     
     - Add deviceType parameter
     - Implement FreeRTOS queues
     
     Closes #7
     ```

### Documentation Updates
- Update `architecture.md` when changing system architecture
- Create `CHANGELOG-YYYY-MM-DD.md` for major refactoring sessions
- Keep README.md updated with new features
- Document all MQTT topics and payloads
- **All documentation in English** (following gaesca04 professional standards)
- Use Doxygen-style comments for public APIs
- Include code examples in documentation

### GitHub Issues Management
**Updating issues via command line** (requires GitHub CLI `gh`):

1. **Edit issue description**:
   ```bash
   gh issue edit <number> --body "New description with markdown"
   ```

2. **Add comment to issue**:
   ```bash
   gh issue comment <number> --body "Comment text"
   ```

3. **Close issue manually**:
   ```bash
   gh issue close <number>
   ```

4. **Reopen issue**:
   ```bash
   gh issue reopen <number>
   ```

5. **Update issue with multiline description**:
   ```bash
   gh issue edit 7 --body "## Title
   Description paragraph
   
   ### Section
   - List item
   - List item"
   ```

**Best practices**:
- Update issue descriptions when implementation differs significantly from original plan
- Add code examples and technical details in issue body
- Use markdown formatting for readability
- Close issues automatically with `Closes #N` in commit messages
- Only use manual commands when automatic closing doesn't work

### Testing Checklist
Before completing work:
- [ ] Code compiles without errors
- [ ] No new warnings introduced
- [ ] Tested on hardware if possible
- [ ] MQTT messages verified
- [ ] Error cases handled
- [ ] Documentation updated
- [ ] Git status clean
- [ ] Commits have clear messages
- [ ] Remote synced (`git push`)

## Common Tasks

### Adding New MQTT Topic
1. Add topic string in `mqtt_handler.cpp`
2. Initialize in `initializeMQTTHandler()`
3. Add publish/subscribe function
4. Add to documentation in `architecture.md`
5. Update MQTT topics list in this file

### Changing Relay Behavior
1. **Only modify** `relay_controller.cpp`
2. Use centralized functions: `activateRelay()`, `deactivateRelay()`
3. Publish state changes to MQTT automatically
4. Send events to `system_state` for coordination

### Adding New Sensor
1. Create sensor module (`sensor_name.cpp/h`)
2. Add task in `system_state.cpp`
3. Add MQTT publishing topic
4. Update display if needed
5. Document in `architecture.md`

## Code Review Self-Checklist

Before requesting review or merging:
- [ ] No code duplication
- [ ] Thread-safe (mutexes where needed)
- [ ] Memory leaks checked (malloc/free balanced)
- [ ] FreeRTOS best practices followed
- [ ] Error handling implemented
- [ ] Logging appropriate (not too verbose)
- [ ] Comments for complex logic
- [ ] Consistent code style
- [ ] No hardcoded values (use config.h)
- [ ] Git history clean and logical

## Emergency Procedures

### If Code Doesn't Compile After Pull
```bash
git log -5  # Check recent commits
git diff HEAD~1  # See what changed
# Fix compilation errors
# Or revert problematic commit:
git revert <commit-hash>
```

### If Remote Has Conflicts
```bash
git fetch origin
git rebase origin/main
# Resolve conflicts in files
git add <resolved-files>
git rebase --continue
```

### If Accidentally Broke Something
```bash
# Option 1: Revert last commit
git revert HEAD

# Option 2: Reset to working state (lose uncommitted changes)
git reset --hard origin/main

# Option 3: Stash and start fresh
git stash
git pull --rebase origin main
```

## Best Practices Summary

1. ✅ Always check remote before starting work
2. ✅ Compile and test before committing
3. ✅ One logical change per commit
4. ✅ Keep commits small and focused
5. ✅ Document as you go (in English)
6. ✅ Maintain architecture compatibility
7. ✅ Test error cases
8. ✅ Push regularly to avoid divergence
9. ✅ Clear commit messages (English, following format)
10. ✅ Update documentation with code changes
11. ✅ Follow naming conventions (camelCase, snake_case, UPPER_CASE)
12. ✅ Use dependency injection (avoid hardcoded values)
13. ✅ Include Doxygen comments for public APIs
14. ✅ Check code style consistency before committing

## Professional Standards Compliance

### Code Quality Metrics
- **Cyclomatic Complexity**: Keep functions under 15 complexity
- **Function Length**: Prefer functions under 50 lines
- **File Length**: Keep modules under 500 lines (split if needed)
- **Comment Ratio**: Aim for 20-30% comments in complex modules

### Architecture Principles (gaesca04 Guidelines)
1. **Single Responsibility**: Each module has one clear purpose
2. **Dependency Injection**: Parameterize instead of hardcoding
3. **Open/Closed**: Open for extension, closed for modification
4. **Interface Segregation**: Small, focused public APIs
5. **Dependency Inversion**: Depend on abstractions (events), not concrete implementations

### Monorepo Rules (gaesca04 Architecture)
- **Shared code** lives in `libs/core/` or `libs/utils/`
- **App-specific code** lives in `apps/{device_name}/`
- **Never hardcode device type** - always parameterize
- **Maintain backward compatibility** in shared modules
- **Version shared modules** carefully (breaking changes = major version)

---

*Last updated: 28 November 2025*  
*Architecture standards by: gaesca04 (computer engineer)*
