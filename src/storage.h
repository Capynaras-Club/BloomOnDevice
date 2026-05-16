#pragma once

#include <Arduino.h>
#include "events.h"

// LittleFS-backed persistence for the event log.
//
// Format on flash (`/events.bin`):
//   [4] magic       "BLM\0"
//   [1] version     1
//   [2] eventCount  uint16_t
//   [4] nextEventId uint32_t
//   [1] reserved    0
//   [N] eventLog    eventCount × Event
//
// loadEvents() refuses files with the wrong magic/version (returns false and
// leaves the in-RAM state untouched), so a corrupted or older-format file
// starts the device with an empty log rather than reading garbage.

bool storageInit();
void saveEvents();
bool loadEvents();
