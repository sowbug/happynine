#include "rng.h"

#include <openssl/rand.h>

RNG::RNG() {
}

RNG::~RNG() {
}

bool RNG::GetRandomBytes(bytes_t& bytes) {
  return RAND_bytes(&bytes[0], bytes.capacity()) == 1;
}
