# Changelog - 28 November 2025

## Summary
Major refactoring to centralize relay control, eliminate code duplication, improve error handling, and add real-time MQTT telemetry for relay timer status.

---

## 🔧 Major Changes

### 1. Centralized Relay Control
**Problem**: Relay state was managed in multiple places (system_state.cpp and relay_controller.cpp), causing duplication and potential inconsistencies.

**Solution**: Created centralized control functions in `relay_controller.cpp`:
- `activateRelay()` - Single function to turn relay ON
- `deactivateRelay(reason)` - Single function to turn relay OFF  
- `isRelayActive()` - Query current relay state

**Benefits**:
- Single source of truth for relay state
- All GPIO and MQTT publishing in one place
- No duplicate state variables
- Easier to maintain and debug

**Files Modified**:
- `src/relay_controller.cpp` - Added centralized functions
- `src/relay_controller.h` - Public API declarations
- `src/system_state.cpp` - Removed duplicate state management
- `src/system_state.h` - Removed relay function declarations

**Commit**: `39c3092` - Add centralized relay control functions

---

### 2. Generic Button Manager
**Problem**: Button manager had device-specific logic (checking relay state, sending ON/OFF events).

**Solution**: Made button_manager completely generic:
- Short press → sends `EVENT_SHORT_PRESS_BUTTON` (generic event)
- Long press → sends `EVENT_LONG_PRESS_BUTTON` (config mode)
- `system_state.cpp` interprets events and decides action

**Benefits**:
- Button manager is now reusable across MICA projects
- No device-specific knowledge in button handler
- Maintains compatibility with MICA Gateway architecture

**Files Modified**:
- `src/button_manager.cpp` - Removed relay-specific logic
- `src/system_state.h` - Added `EVENT_SHORT_PRESS_BUTTON`
- `src/system_state.cpp` - Added button event handler

**Commits**: 
- `631db2d` - Add EVENT_SHORT_PRESS_BUTTON event
- `e9f2dfa` - Refactor button_manager to be generic

---

### 3. MQTT State Publishing
**Problem**: Relay state changes were not being published to MQTT.

**Solution**: Integrated MQTT publishing into centralized relay functions:
- `activateRelay()` → immediately publishes "ON" to MQTT
- `deactivateRelay()` → immediately publishes "OFF" to MQTT
- Removed duplicate publishing from `mqtt_handler.cpp`

**Payload**:
```json
{
  "deviceId": "A0B1C2D3E4F5",
  "state": "ON",
  "timestamp": 123456
}
```

**Topic**: `mica/dev/telemetry/recirculator/{deviceId}/power-state`

**Files Modified**:
- `src/relay_controller.cpp` - Added publishPowerState() calls
- `src/mqtt_handler.cpp` - Removed duplicate publishing

**Commits**:
- `39c3092` - Add centralized relay control functions
- `88dfc2a` - Remove duplicate MQTT publishing

---

### 4. Relay Timer Telemetry (NEW)
**Problem**: No way to monitor relay progress remotely.

**Solution**: Added new MQTT topic that publishes timer status every 5 seconds while relay is active.

**Payload**:
```json
{
  "deviceId": "A0B1C2D3E4F5",
  "elapsed": 45,
  "remaining": 75,
  "maxTime": 120,
  "timestamp": 123456
}
```

**Topic**: `mica/dev/telemetry/recirculator/{deviceId}/relay-timer`

**Behavior**:
- ✅ Only publishes while relay is ON
- ✅ Publishes every 5 seconds (aligned with log interval)
- ✅ Automatically stops when relay turns OFF
- ✅ Allows backend to track progress in real-time

**Files Modified**:
- `src/mqtt_handler.h` - Added publishRelayTimer() declaration
- `src/mqtt_handler.cpp` - Implemented publishRelayTimer()
- `src/relay_controller.cpp` - Call publishRelayTimer() every 5s

**Commit**: `b38ff4b` - Add MQTT publishing for relay timer

---

### 5. Temperature Sensor Error Handling
**Problem**: 
- Publishing every 1 second (too frequent)
- No clear error indication in logs or display
- Backend couldn't detect sensor failures

**Solution**:
1. **Error Logging**: Log prominent error once when sensor fails
2. **Reduced Frequency**: Changed from 1s to 5s interval
3. **Display Feedback**: OLED shows "T: ERROR" instead of "-127.0C"
4. **MQTT Publishing**: Still publishes -127°C so backend can detect failures

**Why publish errors?**
- Backend needs to know about hardware failures
- Allows triggering maintenance alerts
- App can show "sensor disconnected" status
- Historical data shows when failures occurred

**Files Modified**:
- `src/temperature_sensor.cpp` - Added error logging, kept MQTT publishing
- `src/displayManager.cpp` - Show "ERROR" on display

**Commit**: `1df36b8` - Improve temperature sensor error handling

---

### 6. Removed Unnecessary EEPROM Usage
**Problem**: Relay state was being saved to EEPROM, which is unnecessary and potentially unsafe.

**Solution**: 
- Removed `getPowerState()` function (unused)
- Removed relay state saving in `setPowerState()`
- Relay **always starts OFF** on boot for safety

**Rationale**:
- Configuration (max temp, max time) → ✅ Save to EEPROM
- Temporary runtime state (relay ON/OFF) → ❌ Don't save

**Files Modified**:
- `src/mqtt_handler.cpp` - Removed EEPROM save
- `src/mqtt_handler.h` - Removed getPowerState() declaration

**Commit**: `4d3b802` - Remove unnecessary relay state persistence

---

## 📊 Current MQTT Topics

### Telemetry (Publish)
| Topic | Frequency | Payload | Description |
|-------|-----------|---------|-------------|
| `.../temperature` | Every 5s | `{deviceId, temperature, uptime}` | Temperature readings (only if valid) |
| `.../power-state` | On change | `{deviceId, state, timestamp}` | Relay state changes (ON/OFF) |
| `.../relay-timer` | Every 5s | `{deviceId, elapsed, remaining, maxTime, timestamp}` | Timer status (only when ON) |
| `.../healthcheck` | On demand | `{uptime, freeHeap}` | Device health |

### Commands (Subscribe)
| Topic | Payload | Range | Description |
|-------|---------|-------|-------------|
| `.../power-state` | "ON" / "OFF" | - | Control relay manually |
| `.../max-temperature` | Float | -55 to 125°C | Set temperature threshold |
| `.../max-time` | Integer | 1-3600s | Set max runtime (default: 120s) |
| `.../ota` | JSON | - | Firmware update URL |

---

## 🔍 Architecture Improvements

### Before
```
system_state.cpp
├─ g_relayState (duplicate)
├─ g_relayStateMutex
├─ setRelayState()
├─ getRelayState()
└─ isRelayActive()

relay_controller.cpp
├─ Internal relay state (duplicate)
└─ Direct GPIO manipulation
```

### After
```
relay_controller.cpp (SINGLE SOURCE OF TRUTH)
├─ isRelayPhysicallyOn (internal state)
├─ activateRelay() → GPIO + MQTT
├─ deactivateRelay(reason) → GPIO + MQTT
└─ isRelayActive() → query state

system_state.cpp
└─ Calls relay functions directly
```

**Result**: No duplication, clear ownership, automatic MQTT publishing

---

## 🐛 Bug Fixes

1. **Relay state not published to MQTT** - Fixed by integrating publishing into centralized functions
2. **Temperature sensor errors spamming MQTT** - Added validation to skip invalid readings
3. **Display showing -127°C on sensor failure** - Now shows "ERROR" for clarity
4. **Potential state inconsistency** - Eliminated by centralizing relay control
5. **Unnecessary EEPROM writes** - Removed relay state persistence (safety improvement)

---

## 📝 Code Quality Improvements

1. **Extracted testBuzzer() function** - Better code organization
2. **Consistent error handling** - Temperature sensor errors logged prominently
3. **Improved logging levels** - Timer uses `Log::debug` to reduce log spam
4. **Better documentation** - Added detailed comments in relay_controller.cpp
5. **Thread safety** - Maintained mutex protection where needed

---

## 🧪 Testing Recommendations

### Relay Control
```bash
# Test ON/OFF via MQTT
mosquitto_pub -h broker -t "mica/dev/command/recirculator/{deviceId}/power-state" -m "ON"
mosquitto_pub -h broker -t "mica/dev/command/recirculator/{deviceId}/power-state" -m "OFF"

# Test max time configuration (30 seconds)
mosquitto_pub -h broker -t "mica/dev/command/recirculator/{deviceId}/max-time" -m "30"

# Test max temperature (35°C)
mosquitto_pub -h broker -t "mica/dev/command/recirculator/{deviceId}/max-temperature" -m "35.0"
```

### Monitor Telemetry
```bash
# Subscribe to all telemetry
mosquitto_sub -h broker -t "mica/dev/telemetry/recirculator/+/#" -v

# Expected output when relay is ON:
# .../power-state: {"deviceId":"...", "state":"ON", "timestamp":...}
# .../temperature: {"deviceId":"...", "temperature":28.5, "uptime":...}  (every 5s)
# .../relay-timer: {"deviceId":"...", "elapsed":10, "remaining":110, "maxTime":120, ...}  (every 5s)
```

### Temperature Sensor Error
1. Disconnect DS18B20 sensor
2. Check logs: Should see `[ERROR] Temperature sensor ERROR: -127°C`
3. Check OLED: Should display "T: ERROR"
4. Verify MQTT: **Still publishes -127°C** (backend needs to know about failures)
5. Check backend/app: Should show sensor error alert
6. Reconnect sensor: Should auto-recover and resume normal readings

---

## 📚 Documentation Updates

### Files Updated
- `architecture.md` - Comprehensive update with all changes
  - Updated system state management section
  - Added centralized relay control documentation
  - Updated MQTT topics with new relay-timer
  - Added error handling documentation
  - Updated thread safety section
  - Corrected display manager layout

---

## 🎯 Summary of Commits (9 total)

1. `631db2d` - Add EVENT_SHORT_PRESS_BUTTON event for generic button handling
2. `e9f2dfa` - Refactor button_manager to be generic without relay-specific logic
3. `39c3092` - Add centralized relay control functions
4. `b6c6b7b` - Remove duplicate relay state management from system_state
5. `88dfc2a` - Remove duplicate MQTT publishing from setPowerState
6. `1df36b8` - Improve temperature sensor error handling
7. `a65b400` - Display 'ERROR' on OLED when temperature sensor fails
8. `4d3b802` - Remove unnecessary relay state persistence in EEPROM
9. `b38ff4b` - Add MQTT publishing for relay timer (remaining time)

---

## ⚡ Performance Impact

### Before
- Temperature published every 1 second
- No error visibility in logs or display
- Duplicate state checks and updates

### After
- Temperature published every 5 seconds (80% reduction)
- **Error temperatures (-127°C) still published** (backend visibility)
- Clear error indication in logs and OLED display
- Single relay state update path
- Timer telemetry adds minimal overhead (every 5s when ON only)

**Net result**: More efficient, better error handling, backend can monitor hardware health

---

## 🔐 Safety Improvements

1. **Relay always starts OFF** - No EEPROM persistence prevents unexpected activation
2. **Centralized control** - Single point of control reduces bugs
3. **Better error visibility** - Display and logs clearly show sensor failures
4. **Validation before MQTT** - Don't publish invalid data

---

## 🚀 Next Steps

### Potential Future Improvements
- [ ] Add flow sensor integration
- [ ] Historical data logging
- [ ] Multiple temperature sensors support
- [ ] Energy consumption monitoring
- [ ] Schedule-based automation

### Monitoring
- Monitor relay-timer messages to verify backend integration
- Track sensor error frequency (should be rare in production)
- Verify MQTT message frequency matches expectations
