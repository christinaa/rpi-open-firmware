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
	void* start_of_heap = (void*)MEM_HEAP_START;
	size_t hs = MEM_HEAP_SIZE;

	logf("Initializing heap at 0x%x with size 0x%x\n", start_of_heap, hs);

	init_memory_pool(hs, start_of_heap);
}

void main() {
	/* wait for peripheral access */
	while(ARM_ID != ARM_IDVAL);

	logf("Started on ARM, continuing boot from here ...\n");

	heap_init();

	/* c++ runtime */
	cxx_init();

	panic("Nothing else to do!");

#if 0
	printf("Done ");
	for(;;) {
		printf(".");
		udelay(1000000);
	}
#endif
}
