// UtcClock.cpp
// UTC Clock Module
// Purpose: Manages NTP time synchronization with drift compensation
// Architecture: Periodic sync with validation, fallback to uptime estimate
// Thread-Safety: Read-only after sync (atomic operations on ESP32)
// Dependencies: esp_sntp, Arduino

#include "UtcClock.h"

// Third-party libraries
#include <Arduino.h>
#include <Log.h>

// System headers
#include <esp_sntp.h>

#define SYNC_INTERVAL 1000 * 60 * 60

UtcClock::UtcClock(const char *ntpServerMain, const char *ntpServerBackup)
    : ntpServerMain(ntpServerMain), ntpServerBackup(ntpServerBackup) {}

void UtcClock::init() {
    sntp_set_sync_interval(SYNC_INTERVAL);
    configTime(
        gmtOffsetSeconds,
        gmtOffsetSeconds,
        ntpServerMain,
        ntpServerBackup
    );
}

uint64_t UtcClock::getTime(uint64_t millisTimestamp) {
    synchronize();
    uint64_t currentMillis = (millisTimestamp > 0) ? millisTimestamp : millis();

    if (isSynchronized) {
        return lastSyncUnixMs + (currentMillis - lastSyncMillis);
    }
    Log::warn("NTP not synchronized, using uptime estimate");
    return currentMillis;
}

bool UtcClock::hasLastSyncExpired() const {
    return millis() - lastSyncMillis > SYNC_INTERVAL || !isSynchronized;
}

void UtcClock::synchronize() {
    Log::debug("Time sync: %lu", millis());
    if (!hasLastSyncExpired()) {
        return;
    }

    if (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
        Log::debug("Could not sync, sntp not completed");
        return;
    }

    time_t newTime;
    time(&newTime);
    uint32_t syncMillis = millis();

    if (newTime < 1577836800) {  // January 1, 2020 timestamp
        Log::warn("Sync time validation failed");
        return;
    }

    if (!isSynchronized) {
        Log::debug("Initial sync");
        lastSyncMillis = syncMillis;
        lastSyncUnixMs = (uint64_t)newTime * 1000L;
        Log::debug("Sync value %llu", lastSyncUnixMs);
    } else {
        uint64_t expected = lastSyncUnixMs + (syncMillis - lastSyncMillis);
        uint64_t actual = (uint64_t)newTime * 1000L;
        sntp_sync_status_t sync_status = sntp_get_sync_status();
        Log::debug("Last sync %llu", lastSyncUnixMs);
        Log::debug("Elapsed %llu", syncMillis - lastSyncMillis);
        Log::debug("Expected %llu", expected);
        Log::debug("Actual %llu", actual);
        Log::debug("Drifted %lld ms", (int64_t)actual - (int64_t)expected);
        lastSyncMillis = syncMillis;
        lastSyncUnixMs = actual;
    }
    isSynchronized = true;
}