#include <stdint.h>
#include <string>
#include <vector>

typedef std::vector<unsigned char> bytes_t;

std::string to_hex(const bytes_t bytes);
