#include <stdint.h>
#include <chainloader.h>
#include <hardware.h>
#include <stdbool.h>

extern uintptr_t* _end;

#define logf(fmt, ...) printf("[BRINGUP:%s]: " fmt, __FUNCTION__, ##__VA_ARGS__);

static void heap_init() {
	void* start_of_heap = (void*)MEM_HEAP_START;
	size_t hs = MEM_HEAP_SIZE;

	logf("Initializing heap at 0x%x with size 0x%x\n", start_of_heap, hs);

	init_memory_pool(hs, start_of_heap);
}

void main(bool security_supported) {
	/* wait for peripheral access */
	while(ARM_ID != ARM_IDVAL);
	udelay(500);

	logf("Started on ARM, continuing boot from here ...\n");

	logf("Firmware data: SDRAM_SIZE=%d, VPU_CPUID=0x%X\n",
	     g_FirmwareData.sdram_size,
	     g_FirmwareData.vpu_cpuid);

	if (security_supported) {
		logf("Security extensions are supported!\n");
	}

	heap_init();

	/* c++ runtime */
	__cxx_init();

	panic("Nothing else to do!");
}
