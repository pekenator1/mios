#include <string.h>
#include <inttypes.h>


int
atoi(const char *s)
{
  int m = 1;
  int r = 0;
  while(*s && *s <= 32)
    s++;

  if(!*s)
    return 0;

  if(*s == '-') {
    m = -1;
    s++;
  }
  while(*s >= '0' && *s <= '9') {
    r = r * 10 + *s - '0';
    s++;
  }
  return r * m;
}


int
conv_hex_to_nibble(char c)
{
  switch(c) {
  case '0' ... '9':
    return c - '0';
  case 'A' ... 'F':
    return c - 'A' + 10;
  case 'a' ... 'f':
    return c - 'a' + 10;
  default:
    return -1;
  }
}



static unsigned int
atoi_hex(const char *s)
{
  unsigned int r = 0;

  while(1) {
    int v = conv_hex_to_nibble(*s);
    if(v == -1)
      return r;
    r = r * 16 + v;
    s++;
  }
}


unsigned int
atoix(const char *s)
{
  while(*s && *s <= 32)
    s++;

  if(s[0] == '0' && s[1] == 'x') {
    return atoi_hex(s + 2);
  }
  return atoi(s);
}
