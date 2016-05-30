#include <stdint.h>
#include <chainloader.h>
#include <hardware.h>

extern uintptr_t* __init_array_start;
extern uintptr_t* __init_array_end;

#define logf(fmt, ...) printf("[startup::%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

void cxx_init() {
	unsigned ctor_count = (unsigned)(&__init_array_end - &__init_array_start);
	void (*static_ctor)();

	logf("calling %d static constructors (0x%X - 0x%X) ...\n", ctor_count, &__init_array_start, &__init_array_end);

	for (unsigned i = 0; i < ctor_count; i++) {
		uintptr_t* ptr = (((uintptr_t*)&__init_array_start) + i);
		static_ctor = (void*)*ptr;
		static_ctor();
	}
}

void main() {
	logf("started on ARM, continuing boot from here ...\n", __FUNCTION__);

	/* c++ runtime */
	cxx_init();

	printf("Done ");
	for(;;) {
		printf(".");
		udelay(1000000);
	}
}