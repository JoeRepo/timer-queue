#ifndef UTILS_H
#define UTILS_H

#define __DEBUG

#define ARRAY_SIZE(x) (sizeof(x)/(sizeof(*x)))

#if defined(__DEBUG)
#define DEBUG_WRITE(str) Serial.print(str)
#else
#define DEBUG_WRITE(str)
#endif

#endif