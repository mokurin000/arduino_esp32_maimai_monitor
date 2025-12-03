#include <atomic>

#include <HTTPClient.h>
#include <WiFiClientSecure.h>

void maimai_check_setup();

void spawn_maimai_check();

extern WiFiClientSecure *client;
extern HTTPClient https;

extern std::atomic<long> Elapsed;
extern std::atomic<uint32_t> SuccCount, TimeoutCount, ErrCount;

const long VALUE_MISSING = -1;
