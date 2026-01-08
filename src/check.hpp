#include <atomic>

#define BITS_OF_STATUS 3

typedef uint64_t recenterror_t;

#define MAIMAI_VERSION "1.53"

const long VALUE_MISSING = -1;
const recenterror_t EMPTY = 0;
const recenterror_t STATUS_MASK = 0b111;
const recenterror_t REQUEST_SUCCEED = STATUS_MASK;
const recenterror_t REQUEST_FAILED = 0b001;
const recenterror_t REQUEST_TIMEOUT = 0b010;
const recenterror_t REQUEST_TIMEOUT_LONG = 0b011;
const recenterror_t REQUEST_TIMEOUT_MESSY = 0b100;

void spawn_maimai_check();

extern std::atomic<long> Elapsed;
extern std::atomic<uint32_t> SuccCount, TimeoutCount, ErrCount;
extern std::atomic<recenterror_t> RecentError;