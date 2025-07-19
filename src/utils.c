#include "tty.h"

int32_t utf8decode(const char *s, uint32_t *out_cp) {
  unsigned char c = s[0];
  if (c < 0x80) {
    *out_cp = c;
    return 1;
  } else if ((c >> 5) == 0x6) {
    *out_cp = ((c & 0x1F) << 6) | (s[1] & 0x3F);
    return 2;
  } else if ((c >> 4) == 0xE) {
    *out_cp = ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    return 3;
  } else if ((c >> 3) == 0x1E) {
    *out_cp = ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
              ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
    return 4;
  }
  return 0;
}
