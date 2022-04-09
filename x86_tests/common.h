#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define BLOCK() \
{ \
	volatile unsigned char *done = (volatile unsigned char *)0x13370; \
	while ((*done) == 0); \
	done = 0; \
}

#define NOTIFY_TEST() \
{ \
	volatile unsigned char *notify = (volatile unsigned char *)0x13378; \
	*notify = 1; \
}

void memset(volatile unsigned char *buf, int v, size_t sz)
{
	for (size_t i = 0; i < sz; ++i) {
		buf[i] = v;
	}
}

void memcpy(volatile unsigned char *buf, volatile unsigned char *src, size_t sz)
{
	for (size_t i = 0; i < sz; ++i) {
		buf[i] = src[i];
	}
}

size_t strlen(volatile unsigned char *buf)
{
	size_t i = 0;
	while (buf[i++]);
	return i - 1;
}

void write_to_addresses(void *addresses[], size_t n, void *value, size_t value_size)
{
	for (size_t i = 0; i < n; ++i) {
		volatile void *ptr = addresses[i];
		memcpy(ptr, value, value_size);
	}
	memset(value, 0, value_size);
}

#endif 
