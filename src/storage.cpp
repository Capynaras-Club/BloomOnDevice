#include "storage.h"

#include <LittleFS.h>
#include "config.h"

#define EVENTS_FILE   "/events.bin"
#define EVENTS_MAGIC  0x004D4C42UL    // "BLM\0" (little-endian)
#define EVENTS_VERSION  1

// Event log state — definitions live here so adding a second .cpp file
// in the future doesn't trip a multiple-definition link error.
Event    eventLog[MAX_EVENTS];
uint16_t eventCount  = 0;
uint32_t nextEventId = 1;

bool storageInit() {
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

  const uint32_t magic    = EVENTS_MAGIC;
  const uint8_t  version  = EVENTS_VERSION;
  const uint8_t  reserved = 0;

  size_t w = 0;
  w += f.write((const uint8_t*)&magic,       sizeof(magic));
  w += f.write((const uint8_t*)&version,     sizeof(version));
  w += f.write((const uint8_t*)&eventCount,  sizeof(eventCount));
  w += f.write((const uint8_t*)&nextEventId, sizeof(nextEventId));
  w += f.write((const uint8_t*)&reserved,    sizeof(reserved));
  if (eventCount) {
    w += f.write((const uint8_t*)eventLog, sizeof(Event) * eventCount);
  }
  f.close();

  const size_t expected = sizeof(magic) + sizeof(version) + sizeof(eventCount)
                        + sizeof(nextEventId) + sizeof(reserved)
                        + sizeof(Event) * eventCount;
  if (w != expected) {
    Serial.printf("[storage] save: short write %u/%u\n",
                  (unsigned)w, (unsigned)expected);
  }
}

bool loadEvents() {
  if (!LittleFS.exists(EVENTS_FILE)) return false;
  File f = LittleFS.open(EVENTS_FILE, FILE_READ);
  if (!f) return false;

  uint32_t magic    = 0;
  uint8_t  version  = 0;
  uint16_t count    = 0;
  uint32_t nextId   = 1;
  uint8_t  reserved = 0;

  if (f.read((uint8_t*)&magic,    sizeof(magic))    != sizeof(magic) ||
      f.read((uint8_t*)&version,  sizeof(version))  != sizeof(version) ||
      f.read((uint8_t*)&count,    sizeof(count))    != sizeof(count) ||
      f.read((uint8_t*)&nextId,   sizeof(nextId))   != sizeof(nextId) ||
      f.read((uint8_t*)&reserved, sizeof(reserved)) != sizeof(reserved)) {
    Serial.println("[storage] load: header truncated, ignoring file");
    f.close();
    return false;
  }

  if (magic != EVENTS_MAGIC || version != EVENTS_VERSION) {
    Serial.printf("[storage] load: bad magic/version (%08lX v%u), ignoring\n",
                  (unsigned long)magic, (unsigned)version);
    f.close();
    return false;
  }

  if (count > MAX_EVENTS) {
    Serial.printf("[storage] load: count %u > MAX_EVENTS %u, clamping\n",
                  (unsigned)count, (unsigned)MAX_EVENTS);
    count = MAX_EVENTS;
  }

  size_t want = sizeof(Event) * count;
  size_t got  = count ? f.read((uint8_t*)eventLog, want) : 0;
  f.close();

  if (got != want) {
    Serial.printf("[storage] load: body short %u/%u, refusing\n",
                  (unsigned)got, (unsigned)want);
    return false;
  }

  eventCount  = count;
  nextEventId = nextId;
  return true;
}
