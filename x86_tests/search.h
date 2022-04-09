#ifndef SEARCH_TESTS_H
#define SEARCH_TESTS_H

#include "common.h"
#include "common_x86.h"

void search_string_test()
{
	WRITE_MSG("SEARCH_STR~");

	map_all(512 * 1024 * 1024);

	const void* addresses[] = 
	{
		0x111111, 0x222222, 0x12000, 0x900000,
		0xaaaaaa, 0x180000 - 3
	};

	volatile char *str = "abracadabra";
	int l = strlen(str);

	write_to_addresses(addresses, sizeof(addresses) / sizeof(addresses[0]), str, l);

	memset(str, 0, l);
}

void search_bytes_8_test()
{
	WRITE_MSG("SEARCH_8b~");
	map_all(512 * 1024 * 1024);
	const void* addresses[] = 
	{
		0x312911, 0xabcdef, 0x888888, 0x99999,
		0xa383838, 0x19abde1 - 3, 0x89abde1 - 0x5,
		0x412918, 0x99999a, 0x8b8000 - 1
	};

	volatile uint64_t value = 0x11aa22bb33ccdd55;

	write_to_addresses(addresses, sizeof(addresses) / sizeof(addresses[0]), &value, sizeof(value));

	memset(&value, 0, sizeof(value));
}

void search_bytes_4_test()
{
	WRITE_MSG("SEARCH_4b~");
	map_all(512 * 1024 * 1024);
	const void* addresses[] = 
	{
		0x913911, 0xcbeae9, 0x333333, 0x99999,
	};

	volatile uint32_t value = 0x8833aacc;

	write_to_addresses(addresses, sizeof(addresses) / sizeof(addresses[0]), &value, sizeof(value));
	memset(&value, 0, sizeof(value));
}

void search_bytes_2_test()
{
	WRITE_MSG("SEARCH_2b~");
	map_all(256 * 1024 * 1024);
	const void* addresses[] = 
	{
		0x343434, 0x353535, 0x800000-1, 0x120000-2,
	};

	volatile uint16_t value = 0xaaee;

	write_to_addresses(addresses, sizeof(addresses) / sizeof(addresses[0]), &value, sizeof(value));

	memset(&value, 0, sizeof(value));
}

#endif
