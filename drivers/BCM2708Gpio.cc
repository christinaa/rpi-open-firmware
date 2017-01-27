#include <drivers/BCM2708Gpio.hpp>

void BCM2708Gpio::setFunction(uint32_t pin_num, BCM2708PinmuxSetting function) {
	uint32_t* fsel = reinterpret_cast<uint32_t*>(
		                     reinterpret_cast<uint32_t>(&GP_FSEL0) + (0x4 * (pin_num/10))
		                 );
	uint32_t pin_shift = (pin_num % 10) * 3;

	*fsel = (*fsel & (0x7 << pin_shift)) | function;
}

void BCM2708Gpio::init() {
	setTag('GPIO');
}

IODriverCreateSingletonInstance(BCM2708Gpio);