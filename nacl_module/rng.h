#include "types.h"

class RNG {
 public:
  RNG();
  virtual ~RNG();

  // Fills the given vector with "cryptographically strong
  // pseudo-random bytes" using OpenSSL's RAND_bytes(). If the
  // underlying method fails, returns false.
  static bool GetRandomBytes(bytes_t& bytes);
};
