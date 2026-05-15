#pragma once

#include <Arduino.h>
#include <string.h>
#include "config.h"

struct Event {
  uint32_t id;
  uint32_t timestamp;
  uint8_t  type;
  uint16_t duration;
};

extern Event    eventLog[MAX_EVENTS];
extern uint16_t eventCount;
extern uint32_t nextEventId;

void     saveEvents();
bool     loadEvents();
// Returns true iff any events were dropped. Does NOT save — caller decides.
bool     pruneOldEvents();

inline uint32_t dayStartOffset(uint8_t daysBack) {
  time_t now = time(nullptr);
  struct tm t = *localtime(&now);
  t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
  time_t midnight = mktime(&t);
  return (uint32_t)midnight - (uint32_t)daysBack * 86400UL;
}

inline uint32_t logEvent(uint8_t type, uint16_t duration = 0) {
  // Prune stale events first so we never write the new event into flash
  // alongside ones we're about to drop on the next boot. One flash write
  // per logged event, regardless of whether pruning happened.
  pruneOldEvents();

  if (eventCount >= MAX_EVENTS) {
    memmove(&eventLog[0], &eventLog[1], (MAX_EVENTS - 1) * sizeof(Event));
    eventCount = MAX_EVENTS - 1;
  }
  Event& e = eventLog[eventCount++];
  e.id        = nextEventId++;
  e.timestamp = (uint32_t)time(nullptr);
  e.type      = type;
  e.duration  = duration;
  saveEvents();
  return e.id;
}

inline bool deleteEvent(uint32_t id) {
  for (uint16_t i = 0; i < eventCount; i++) {
    if (eventLog[i].id == id) {
      memmove(&eventLog[i], &eventLog[i + 1],
              (eventCount - i - 1) * sizeof(Event));
      eventCount--;
      saveEvents();
      return true;
    }
  }
  return false;
}

inline const Event* getLastOfType(uint8_t type) {
  for (int i = eventCount - 1; i >= 0; i--) {
    if (eventLog[i].type == type) return &eventLog[i];
  }
  return nullptr;
}

inline uint16_t getEventsForDay(uint8_t daysBack, const Event** out, uint16_t maxOut) {
  uint32_t start = dayStartOffset(daysBack);
  uint32_t end   = start + 86400UL;
  uint16_t n = 0;
  for (int i = eventCount - 1; i >= 0 && n < maxOut; i--) {
    if (eventLog[i].timestamp >= start && eventLog[i].timestamp < end) {
      out[n++] = &eventLog[i];
    }
  }
  return n;
}

inline uint16_t countEventsForDay(uint8_t daysBack, uint8_t type) {
  uint32_t start = dayStartOffset(daysBack);
  uint32_t end   = start + 86400UL;
  uint16_t n = 0;
  for (uint16_t i = 0; i < eventCount; i++) {
    if (eventLog[i].type == type &&
        eventLog[i].timestamp >= start &&
        eventLog[i].timestamp < end) n++;
  }
  return n;
}
