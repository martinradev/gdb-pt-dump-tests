#include <stdint.h>
#include <stdbool.h>
#include "../x86_tests/common.h"

volatile unsigned int * const UART0_DR = (unsigned int *) 0x09000000;

typedef enum 
{
	Granularity_T0_4K = 0U,
	Granularity_T0_16K = 2U,
	Granularity_T0_64K = 1U,
} Granularity_T0;

typedef enum 
{
	Granularity_T1_4K = 2U,
	Granularity_T1_16K = 1U,
	Granularity_T1_64K = 3U,
} Granularity_T1;

typedef enum
{
	Descriptor_Invalid = 0U,
	Descriptor_Table = 3U,
	Descriptor_TableEntry = 3U,
	Descriptor_Block = 1U,
} Descriptor;

static void save_start_state()
{

}

static void notify_done()
{

}

static uint64_t get_t0sz(uint8_t sz)
{
	return (uint64_t)sz;
}

static uint64_t get_t1sz(uint8_t sz)
{
	return ((uint64_t)sz << 16U);
}

static uint64_t get_tg0(Granularity_T0 gran)
{
	return (uint64_t)gran << 14U;
}

static uint64_t get_tg1(Granularity_T1 gran)
{
	return (uint64_t)gran << 30U;
}

static uint64_t construct_tcr(uint8_t t0sz, uint8_t t1sz, Granularity_T0 gran0, Granularity_T1 gran1)
{
	return get_t0sz(t0sz) | get_t1sz(t1sz) | get_tg0(gran0) | get_tg1(gran1);
}

static uint64_t construct_desc(Descriptor desc, uint64_t phys, uint8_t ap, bool pxn, bool xn)
{
	uint64_t desc_entry = (uint64_t)desc;
	desc_entry |= (phys & ~0xFFFUL);
	desc_entry |= (1ULL << 10u); // AF
	desc_entry |= (((uint64_t)ap & 0x3U) << 6U);
	desc_entry |= ((uint64_t)pxn << 53U);
	desc_entry  |= ((uint64_t)xn << 54U);
	return desc_entry;
}

static void print(const char *s) {
	while(*s != '\0') {
		*UART0_DR = (unsigned int)(*s++);
	}
}

static void switch_page_tables(uint32_t ttbr0_el1, uint32_t ttbr1_el1)
{
	asm volatile(
		"msr ttbr0_el1, %0\n\t"
		"msr ttbr1_el1, %1\n\t"
		"dsb ish\n\t"
		"isb\n\t"
		:
		: "r"(ttbr0_el1), "r"(ttbr1_el1)
		:
	);
}

static void write_to_paging_regs(uint64_t tcr_el1, uint32_t ttbr0_el1, uint32_t ttbr1_el1)
{
	asm volatile(
		// Update addressing and page table state
		"msr tcr_el1, %0\n\t" 
		"msr ttbr0_el1, %1\n\t"
		"msr ttbr1_el1, %2\n\t"

		// Update MAIR
		"mov x0, 0x000004FF\n\t"
		"msr mair_el1, x0\n\t"
		"dsb ish\n\t"
		"isb\n\t"

		// Enable MMU
		"mrs x0, sctlr_el1\n\t"
		"orr x0, x0, #0x1\n\t"
		"msr sctlr_el1, x0\n\t"
		:
		: "r"(tcr_el1), "r"(ttbr0_el1), "r"(ttbr1_el1)
		: "x0"
	);
}

static void setup_initial_pt()
{
	// Map first 512GiB.
	{
		const uint64_t base = 0x40100000;
		volatile uint64_t *ptr = base;
		memset(ptr, 0, 0x2000);
		ptr[0] = construct_desc(Descriptor_Block, 0, 0, false, false);
		const uint64_t tcr = construct_tcr(16, 16, Granularity_T0_4K, Granularity_T1_4K);
		write_to_paging_regs(tcr, base, base);
	}

	// Map only first 2GiB.
	{
		const uint64_t base = 0x40200000;
		volatile uint64_t *ptr = base;
		memset(ptr, 0, 0x2000);
		ptr[0] = construct_desc(Descriptor_Table, base + 0x1000ULL, 0, false, false);
		ptr[512] = construct_desc(Descriptor_Block, 0, 0, false, false);
		ptr[513] = construct_desc(Descriptor_Block, 1024 * 1024 * 1024, 0, false, false);
		switch_page_tables(base, base);
	}
}

static const uint64_t test_base = 0x40300000ULL;

static void test_granularity_4k()
{
	print("TEST_4K_GRANULARITY");

	volatile uint64_t *ptr = test_base;
	memset(ptr, 0, 0x20000);
	const uint64_t num_entries = 512;
	const uint64_t granule_size = 0x1000UL;
	ptr[0] = construct_desc(Descriptor_Block, 0, 0, false, false); // cover first 512GB
	ptr[1] = construct_desc(Descriptor_Table, test_base + granule_size, 0, false, false);
	ptr[num_entries - 1] = construct_desc(Descriptor_Block, 0, 0, false, true);
	ptr[num_entries] = construct_desc(Descriptor_Table, test_base + granule_size * 2U, 0, false, false);
	ptr[num_entries + 8] = construct_desc(Descriptor_Table, test_base + granule_size * 4U, 0, false, false);
	ptr[num_entries * 2] = construct_desc(Descriptor_Table, test_base + granule_size * 3U, 0, false, false); 
	ptr[num_entries * 3] = construct_desc(Descriptor_TableEntry, 0, 0x1, true, true); 
	ptr[num_entries * 3 + 1] = construct_desc(Descriptor_TableEntry, 0, 0, false, false); 
	ptr[num_entries * 3 + 2] = construct_desc(Descriptor_TableEntry, 0, 0, false, false); 
	ptr[num_entries * 3 + 3] = construct_desc(Descriptor_TableEntry, 0, 0, false, true); 
	ptr[num_entries * 4] = construct_desc(Descriptor_Block, 0, 0x11, true, true); 
	ptr[num_entries * 4 + 1] = construct_desc(Descriptor_Block, 0, 0x10, true, true); 
	ptr[num_entries * 4 + 2] = construct_desc(Descriptor_Block, 0, 0x01, true, true); 
	ptr[num_entries * 4 + 3] = construct_desc(Descriptor_Block, 0, 0x01, false, true); 
	ptr[num_entries * 4 + 4] = construct_desc(Descriptor_Block, 0, 0x01, true, false); 
	ptr[num_entries * 4 + 5] = construct_desc(Descriptor_Block, 0, 0x01, true, false); 
	ptr[num_entries * 4 + 6] = construct_desc(Descriptor_Block, 0, 0x01, true, false); 
	ptr[num_entries * 4 + 7] = construct_desc(Descriptor_Block, 0, 0x01, true, false); 
	ptr[num_entries * 5 - 1] = construct_desc(Descriptor_Block, 0, 0x01, true, false); 

	const uint64_t tcr = construct_tcr(16, 16, Granularity_T0_4K, Granularity_T1_4K);
	write_to_paging_regs(tcr, test_base, test_base);

	notify_done();
}

static void test_granularity_16k()
{
	print("TEST_16K_GRANULARITY");

	const uint64_t num_entries = 512 * 4;
	const uint64_t granularity = 16 * 1024;
	volatile uint64_t *ptr = test_base;
	memset(ptr, 0, granularity * 5);
	ptr[0] = construct_desc(Descriptor_Block, 0, 0, false, false); // map 128TiB
	ptr[1] = construct_desc(Descriptor_Table, test_base + granularity, 0, false, false);
	ptr[num_entries] = construct_desc(Descriptor_Block, 0, 0, true, false); // map 64G
	ptr[num_entries + 2] = construct_desc(Descriptor_Table, test_base + granularity * 2, 0, true, false);
	ptr[num_entries * 2 + 1] = construct_desc(Descriptor_Block, 0, 0, true, false); // Map 32M
	ptr[num_entries * 2 + 4] = construct_desc(Descriptor_Table, test_base + granularity * 3, 0, true, false);
	ptr[num_entries * 3 + 8] = construct_desc(Descriptor_TableEntry, 0, 0, true, false); // map 16K

	const uint64_t tcr = construct_tcr(16, 16, Granularity_T0_16K, Granularity_T1_16K);
	write_to_paging_regs(tcr, test_base, test_base);

	notify_done();
}

static void test_granularity_64k()
{
	print("TEST_64K_GRANULARITY");

	const uint64_t num_entries = 512 * 16;
	const uint64_t granularity = 64 * 1024;
	volatile uint64_t *ptr = test_base;
	memset(ptr, 0, 0x20000);
	ptr[0] = construct_desc(Descriptor_Block, 0, 0, false, false); // Map first 4TiB
	ptr[1] = construct_desc(Descriptor_Block, 0, 0, false, false);
	ptr[4] = construct_desc(Descriptor_Table, test_base + granularity, 0, false, false);
	ptr[num_entries - 1] = construct_desc(Descriptor_Block, 0, 0, false, true);
	ptr[num_entries] = construct_desc(Descriptor_Block, 0, 0, false, false); // Map another 512MiB
	ptr[num_entries + 1] = construct_desc(Descriptor_Table, test_base + granularity * 2, 0, false, false);
	ptr[num_entries * 2] = construct_desc(Descriptor_TableEntry, 0, 0, true, true);
	ptr[num_entries * 2 + num_entries - 1] = construct_desc(Descriptor_TableEntry, 0, 0, true, true);
	ptr[num_entries * 2 + 2] = construct_desc(Descriptor_TableEntry, 0, 0, false, false);
	ptr[num_entries * 3 - 1] = construct_desc(Descriptor_TableEntry, 0, 0, false, false);

	const uint64_t tcr = construct_tcr(16, 16, Granularity_T0_64K, Granularity_T1_64K);
	write_to_paging_regs(tcr, test_base, test_base);

	notify_done();
}

static void test_pan_pxn_ap()
{

}

static void test_as1()
{

}

static void test_as2()
{

}

void entry()
{
	save_start_state();
	test_granularity_16k();
	while (1)
	{

	}
}
