//  http协议解析封装
//(消息体不用解析)
#ifndef _http11_common_h
#define _http11_common_h

#include <sys/types.h>

typedef void (*element_cb)(void *data, const char *at, size_t length);
typedef void (*field_cb)(void *data, const char *field, size_t flen, const char *value, size_t vlen);

#endif