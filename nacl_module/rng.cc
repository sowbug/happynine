#include "rng.h"

#include <openssl/rand.h>

RNG::RNG() {
}

RNG::~RNG() {
}

bytes_t RNG::GetRandomBytes(size_t num) {
  bytes_t buffer;
  buffer.resize(num);
  if (RAND_bytes(&buffer[0], num) == 1) {
    return buffer;
  }
  return bytes_t();
}
