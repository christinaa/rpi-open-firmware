#pragma once

#include <hardware.h>

static inline void __attribute__((noreturn)) hang_cpu() {
	/* disable interrupts and enter WFI state */
	__asm__ __volatile__ (
		"di\n"
		"sleep\n"
	);

	/* in case the above fails */
	for (;;) {
		__asm__ __volatile__ ("nop");
	}
}