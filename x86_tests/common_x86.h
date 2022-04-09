#ifndef COMMON_X86_H
#define COMMON_X86_H

#include <stdint.h>
#include <stddef.h>


#define PDE_PRESENT (1u << 0)
#define PDE_RW (1u << 1)
#define PDE_US (1u << 2)
#define PDE_PWT (1u << 3)
#define PDE_PCD (1u << 4)
#define PDE_ACCESSED (1u << 5)
#define PDE_AVL (1u << 6)
#define PDE_PS (1u << 7)
#define PDE(ADDR, FLAGS) ((((unsigned)ADDR) & ~0xfffu) | ((FLAGS) & 0xFFFu))

#define PT_PRESENT (1u << 0)
#define PT_RW (1u << 1)
#define PT_US (1u << 2)
#define PT_PWT (1u << 3)
#define PT_PCD (1u << 4)
#define PT_ACCESSED (1u << 5)
#define PT_D (1u << 6)
#define PT_PAT (1u << 7)
#define PT_G (1u << 8)
#define PT(ADDR, FLAGS) (((unsigned)ADDR & ~0xfffu) | ((FLAGS) & 0xFFFu))
#define ONE_KB (1U * 1024U)
#define FOUR_KB (ONE_KB * 4U)
#define TWO_MB (2U * 1024U * 1024U)
#define FOUR_MB (4U * 1024U * 1024U)

#define VIDMEM_ADDR 0xb8000

void write_to_screen(const char *const msg, unsigned msg_len)
{
	volatile uint16_t *vidmem = VIDMEM_ADDR;
	for (unsigned i = 0; i < msg_len; ++i)
	{
		uint16_t value = (uint8_t)msg[i];
		value |= 0x0f00;
		vidmem[i] = value;
	}
}

#define WRITE_MSG(MSG) write_to_screen(MSG, sizeof(MSG) - 1);

void wr_cr3(unsigned long new_cr3) {
	asm volatile(
		"movl %0, %%cr3\n\t"
		:
		: "r"(new_cr3)
		: "memory"
	);
}

unsigned long rd_cr3() {
	unsigned long value;
	asm volatile(
		"movl %%cr3, %0\n\t"
		: "=r"(value)
		:
		: "memory"
	);
	return value;
}

void wr_cr0(unsigned long new_cr0) {
	asm volatile(
		"movl %0, %%cr0\n\t"
		:
		: "r"(new_cr0)
		: "memory"
	);
}

unsigned long rd_cr0() {
	unsigned long value;
	asm volatile(
		"movl %%cr0, %0\n\t"
		: "=r"(value)
		:
		: "memory"
	);
	return value;
}

void wr_cr4(unsigned long new_cr4) {
	asm volatile(
		"movl %0, %%cr4\n\t"
		:
		: "r"(new_cr4)
		: "memory"
	);
}

unsigned long rd_cr4() {
	unsigned long value;
	asm volatile(
		"movl %%cr4, %0\n\t"
		: "=r"(value)
		:
		: "memory"
	);
	return value;
}

void map_all(size_t size) {
	unsigned pse = 1u << 4; // use 4 MiB-long page.
	unsigned cr4 = rd_cr4() | pse;
	wr_cr4(cr4);
	volatile uint32_t *pd = 0x20000;
	for (unsigned i = 0; i < size / FOUR_MB; ++i)
	{
		pd[i] = PDE((FOUR_MB * i), PDE_PRESENT | PDE_RW | PDE_PS);
	}
	wr_cr3(pd);
}

#endif 
