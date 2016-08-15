#include <stdint.h>
#include <chainloader.h>
#include <hardware.h>

extern uintptr_t* __init_array_start;
extern uintptr_t* __init_array_end;
extern uintptr_t* _end;

#define logf(fmt, ...) printf("[BRINGUP:%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

static void cxx_init() {
	unsigned ctor_count = (unsigned)(&__init_array_end - &__init_array_start);
	void (*static_ctor)();

	logf("calling %d static constructors (0x%X - 0x%X) ...\n", ctor_count, &__init_array_start, &__init_array_end);

	for (unsigned i = 0; i < ctor_count; i++) {
		uintptr_t* ptr = (((uintptr_t*)&__init_array_start) + i);
		static_ctor = (void*)*ptr;
		static_ctor();
	}
}

static void heap_init() {
	void* start_of_heap = (void*)&_end;
	size_t hs = 0x100000;

	logf("Initializing heap at 0x%x with size 0x%x\n", start_of_heap, hs);

	init_memory_pool(hs, start_of_heap);
}

void main() {
	logf("Started on ARM, continuing boot from here ...\n", __FUNCTION__);

	heap_init();

	/* c++ runtime */
	//cxx_init();

        /* poke the VPU */
        printf("Poking the VPU..\n");
        while( (mmio_read32(0x2000B898) & 0x80000000) != 0);
        mmio_write32(0x2000B8A0, 0xA1755A01);

	panic("Nothing else to do!");

#if 0
	printf("Done ");
	for(;;) {
		printf(".");
		udelay(1000000);
	}
#endif
}
