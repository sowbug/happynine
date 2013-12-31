#include "types.h"

class RNG {
 public:
  RNG();
  virtual ~RNG();

  // Returns num "cryptographically strong pseudo-random bytes" using
  // OpenSSL's RAND_bytes(). If the underlying method fails, returns
  // a zero-length vector instead.
  bytes_t GetRandomBytes(size_t num);
};
