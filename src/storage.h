#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include "config.h"
#include "events.h"

#define EVENTS_FILE "/events.bin"

Event    eventLog[MAX_EVENTS];
uint16_t eventCount   = 0;
uint32_t nextEventId  = 1;

inline bool storageInit() {
  if (!LittleFS.begin(true)) {
    Serial.println("[storage] LittleFS mount failed");
    return false;
  }
  return true;
}

void saveEvents() {
  File f = LittleFS.open(EVENTS_FILE, FILE_WRITE);
  if (!f) {
    Serial.println("[storage] save: open failed");
    return;
  }
  f.write((uint8_t*)&eventCount,  sizeof(eventCount));
  f.write((uint8_t*)&nextEventId, sizeof(nextEventId));
  if (eventCount) {
    f.write((uint8_t*)eventLog, sizeof(Event) * eventCount);
  }
  f.close();
}

bool loadEvents() {
  if (!LittleFS.exists(EVENTS_FILE)) return false;
  File f = LittleFS.open(EVENTS_FILE, FILE_READ);
  if (!f) return false;
  f.read((uint8_t*)&eventCount,  sizeof(eventCount));
  f.read((uint8_t*)&nextEventId, sizeof(nextEventId));
  if (eventCount > MAX_EVENTS) eventCount = MAX_EVENTS;
  if (eventCount) {
    f.read((uint8_t*)eventLog, sizeof(Event) * eventCount);
  }
  f.close();
  return true;
}

bool pruneOldEvents() {
  uint32_t cutoff = dayStartOffset(HISTORY_DAYS);
  uint16_t write = 0;
  for (uint16_t i = 0; i < eventCount; i++) {
    if (eventLog[i].timestamp >= cutoff) {
      if (write != i) eventLog[write] = eventLog[i];
      write++;
    }
  }
  if (write == eventCount) return false;
  eventCount = write;
  return true;
}
