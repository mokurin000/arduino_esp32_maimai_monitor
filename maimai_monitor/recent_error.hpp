#include <atomic>

#define BITS_OF_STATUS 3

typedef uint32_t recenterror_t;

extern std::atomic<recenterror_t> RecentError;

const recenterror_t EMPTY = 0;
const recenterror_t STATUS_MASK = 0b111;
const recenterror_t REQUEST_SUCCEED = STATUS_MASK;
const recenterror_t REQUEST_FAILED = 0b001;
const recenterror_t REQUEST_TIMEOUT = 0b010;
const recenterror_t REQUEST_TIMEOUT_LONG = 0b011;
const recenterror_t REQUEST_TIMEOUT_MESSY = 0b100;
