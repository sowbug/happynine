#if !defined __TYPES_H__
#define __TYPES_H__

#include <cctype>
#include <stdint.h>
#include <string>
#include <vector>

typedef std::vector<unsigned char> bytes_t;

std::string to_hex(const bytes_t bytes);

int to_int(int c);

template<class InputIterator, class OutputIterator> int
  unhexlify(InputIterator first, InputIterator last, OutputIterator ascii) {
  while (first != last) {
    int top = to_int(*first++);
    int bot = to_int(*first++);
    if (top == -1 or bot == -1)
      return -1; // error
    *ascii++ = (top << 4) + bot;
  }
  return 0;
}

#endif  // #if !defined __TYPES_H__
