#pragma once

#include <Arduino.h>
#include <string.h>
#include <time.h>
#include "config.h"

struct Event {
  uint32_t id;
  uint32_t timestamp;
  uint8_t  type;
  uint16_t duration;
};

// Definitions live in storage.cpp.
extern Event    eventLog[MAX_EVENTS];
extern uint16_t eventCount;
extern uint32_t nextEventId;

void saveEvents();   // storage.cpp
bool loadEvents();   // storage.cpp

// =========================================================================
// Clock sanity
// =========================================================================
//
// time() returns 0 (epoch) until NTP runs. Anything before this sentinel is
// "the clock is wrong" — we refuse to log events with such timestamps,
// otherwise they end up years in the past and get pruned the next time
// pruneOldEvents() runs once the clock catches up.

#define CLOCK_VALID_AFTER_TS  1700000000UL  // 2023-11-14T22:13:20Z

inline bool clockIsValid() {
  return (uint32_t)time(nullptr) > CLOCK_VALID_AFTER_TS;
}

// =========================================================================
// Day boundary helpers
// =========================================================================

inline uint32_t dayStartOffset(uint8_t daysBack) {
  time_t now = time(nullptr);
  struct tm t = *localtime(&now);
  t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
  time_t midnight = mktime(&t);
  return (uint32_t)midnight - (uint32_t)daysBack * 86400UL;
}

// =========================================================================
// In-memory log mutation
// =========================================================================

// Returns true iff any events were dropped. Does NOT save — caller decides.
// Pure event-state, no I/O — defined here as inline rather than in
// storage.cpp so it doesn't drag flash code into callers that just want
// to filter the log in RAM.
inline bool pruneOldEvents() {
  if (!clockIsValid()) return false;  // can't decide "old" without a clock

  uint32_t cutoff = dayStartOffset(HISTORY_DAYS);
  uint16_t write  = 0;
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

// Returns the new event's id, or 0 if the clock isn't set yet (we refuse
// to log otherwise — the bogus timestamp would be pruned on the next
// clock catch-up).
inline uint32_t logEvent(uint8_t type, uint16_t duration = 0) {
  if (!clockIsValid()) {
    Serial.println("[events] refused: clock not set yet");
    return 0;
  }

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
