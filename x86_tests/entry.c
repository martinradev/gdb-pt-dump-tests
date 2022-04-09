
#include <stdint.h>
#include <stddef.h>

#define WIDTH 80
#define HEIGHT 64

#include "search.h"
#include "common.h"
#include "common_x86.h"

void setup_2mb_page_table_simple();
void setup_4mb_page_table_simple();
void setup_4k_page_table_simple();
void setup_4k_page_table_complex();
void setup_pte_and_pde_in_iova();

void rest();
void entry() __attribute__((section (".text.entry")));

#define NUM_ENTRIES_PER_PAGE (4096 / sizeof(size_t))

_Static_assert(sizeof(size_t) == 4);

void entry() {
	rest();
}

void rest() {
	WRITE_MSG("ENTER~");
	BLOCK();

	typedef void (*func)(void);
	func arr[] = {
		&search_string_test,
		&search_bytes_8_test,
		&search_bytes_4_test,
		&setup_pte_and_pde_in_iova,
		&setup_4mb_page_table_simple,
		&setup_2mb_page_table_simple,
		&setup_4k_page_table_complex,
		&setup_4k_page_table_simple,
	};

	map_all(1024 * 1024 * 1024);

	for (size_t i = 0; i < sizeof(arr) / sizeof(arr[0]); ++i) {
		func f = arr[i];
		f();
		NOTIFY_TEST();
		BLOCK();
		map_all(1024 * 1024 * 1024);
	}

	WRITE_MSG("DONE~");
	NOTIFY_TEST();

	while(1) {
		volatile register int a = 0xdead;
	}
}

// 2MiB, -PSE, -PAE
void setup_2mb_page_table_simple()
{
	WRITE_MSG("2MiB,-PSE,-PAE~");
	uint32_t *pd = 0x10000;
	pd[0] = PDE(0x0, PDE_PRESENT | PDE_PS);
	pd[1] = PDE(TWO_MB, PDE_RW | PDE_PRESENT | PDE_PS);
	pd[2] = PDE(TWO_MB * 2, PDE_US | PDE_PRESENT | PDE_PS);
	pd[3] = PDE(TWO_MB * 3, PDE_PWT | PDE_PRESENT | PDE_PS);
	pd[4] = PDE(TWO_MB * 4, PDE_PCD | PDE_PRESENT | PDE_PS);
	pd[5] = PDE(TWO_MB * 5, PDE_ACCESSED | PDE_PRESENT | PDE_PS);
	pd[6] = PDE(TWO_MB * 6, PDE_AVL | PDE_PRESENT | PDE_PS);
	wr_cr3(pd);
}

// 4K, -PSE, -PAE
void setup_4k_page_table_simple()
{
	WRITE_MSG("4K,-PSE,-PAE~");
	uint32_t *pd = 0x40000;
	pd[0] = PDE(0x0, PDE_PRESENT | PDE_PS);
	pd[1] = PDE(TWO_MB, PDE_RW | PDE_PRESENT | PDE_PS);
	pd[2] = PDE(TWO_MB * 2, PDE_US | PDE_PRESENT | PDE_PS);
	pd[3] = PDE(TWO_MB * 3, PDE_PWT | PDE_PRESENT | PDE_PS);
	pd[4] = PDE(TWO_MB * 4, PDE_PCD | PDE_PRESENT | PDE_PS);
	pd[5] = PDE(TWO_MB * 5, PDE_ACCESSED | PDE_PRESENT | PDE_PS);
	pd[6] = PDE(TWO_MB * 6, PDE_AVL | PDE_PRESENT | PDE_PS);

	uint32_t *pt = 0x30000;
	memset(pt, 0, 0x1000);
	pd[7] = PDE(pt, PDE_PRESENT);
	pt[0] = PT(0, PT_PRESENT);
	pt[2] = PT(0x1000, PT_PRESENT);
	pt[1023] = PT(0x200000, PT_PRESENT | PT_RW);

	wr_cr3(pd);
}

// 4MiB, +PSE, -PAE
void setup_4mb_page_table_simple()
{
	WRITE_MSG("4MiB,+PSE,-PAE~");
	unsigned pse = 1u << 4; // use 4 MiB-long page.
	unsigned cr4 = rd_cr4() | pse;
	wr_cr4(cr4);
	uint32_t *pd = 0x30000;
	pd[0] = PDE(0x0, PDE_PRESENT | PDE_PS);
	pd[1] = PDE(FOUR_MB, PDE_RW | PDE_PRESENT | PDE_PS);
	pd[2] = PDE(FOUR_MB * 2, PDE_US | PDE_PRESENT | PDE_PS);
	pd[3] = PDE(FOUR_MB * 3, PDE_PWT | PDE_PRESENT | PDE_PS);
	pd[4] = PDE(FOUR_MB * 4, PDE_PCD | PDE_PRESENT | PDE_PS);
	pd[5] = PDE(FOUR_MB * 5, PDE_ACCESSED | PDE_PRESENT | PDE_PS);
	pd[6] = PDE(FOUR_MB * 6, PDE_AVL | PDE_PRESENT | PDE_PS);
	pd[7] = PDE(FOUR_MB * 7, PDE_AVL | PDE_PRESENT | PDE_PS);
	pd[8] = PDE(FOUR_MB * 8, PDE_AVL | PDE_PRESENT | PDE_PS);
	pd[9] = PDE(FOUR_MB * 9, PDE_US | PDE_RW | PDE_PRESENT | PDE_PS);
	wr_cr3(pd);
}

void setup_4k_page_table_complex()
{
	WRITE_MSG("complex_test~");
	volatile uint32_t *pd = 0x100000;
	volatile uint32_t *pt = 0x101000;

	// Map two gigs.
	for (unsigned i = 0; i < 128; ++i)
	{
		pd[i] = PDE(&pt[NUM_ENTRIES_PER_PAGE * i], PDE_PRESENT | PDE_RW);
		for (unsigned j = 0; j < NUM_ENTRIES_PER_PAGE; ++j)
		{
			pt[i * NUM_ENTRIES_PER_PAGE + j] = PT(i * FOUR_MB + j * FOUR_KB, PT_PRESENT);
		}
	}

	pt[129] &= ~PT_PRESENT;
	pt[220] |= PT_US;
	pt[444] |= PT_RW;
	pt[320] &= ~PT_RW;
	pt[330] &= ~PT_PRESENT;
	pt[500] |= PT_RW;
	pt[NUM_ENTRIES_PER_PAGE - 1] &= ~PT_PRESENT;
	pt[NUM_ENTRIES_PER_PAGE] |= PT_US;

	wr_cr3(pd);
}

void setup_pte_and_pde_in_iova()
{
	const unsigned device_address_space = 0x80fc0000U;
	WRITE_MSG("device_iova_space~");
	uint32_t *pd = 0x40000;
	pd[0] = PDE(0x0, PDE_PRESENT | PDE_PS);
	pd[1] = PDE(TWO_MB, PDE_RW | PDE_PRESENT | PDE_PS);
	pd[2] = PDE(TWO_MB * 2, PDE_US | PDE_PRESENT | PDE_PS);
	pd[3] = PDE(TWO_MB * 3, PDE_PWT | PDE_PRESENT | PDE_PS);
	pd[4] = PDE(TWO_MB * 4, PDE_PCD | PDE_PRESENT | PDE_PS);
	pd[5] = PDE(device_address_space, PDE_ACCESSED | PDE_PRESENT | PDE_PS);
	pd[6] = PDE(device_address_space, PDE_AVL | PDE_PRESENT);

	uint32_t *pt = 0x30000;
	memset(pt, 0, 0x1000);
	pd[7] = PDE(device_address_space, PDE_PRESENT);

	wr_cr3(pd);
}

// set one of the entries not in ram 0xa0000
