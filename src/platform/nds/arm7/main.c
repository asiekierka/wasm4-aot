/*---------------------------------------------------------------------------------

	Derived from the default ARM7 core

		Copyright (C) 2005 - 2010
		Michael Noland (joat)
		Jason Rogers (dovoto)
		Dave Murphy (WinterMute)

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.

	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.

	3.	This notice may not be removed or altered from any source
		distribution.

---------------------------------------------------------------------------------*/
#include <nds.h>
#include <dswifi7.h>
#include <maxmod7.h>
#include <nds/arm7/audio.h>
#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include "platform_shim.h"
#ifdef PLATFORM_HAS_APU
#include "apu.h"
#endif

//---------------------------------------------------------------------------------
void VblankHandler(void) {
//---------------------------------------------------------------------------------

}


//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	inputGetAndSend();
}

volatile bool exitflag = false;

//---------------------------------------------------------------------------------
void powerButtonCB() {
//---------------------------------------------------------------------------------
	exitflag = true;
}

#ifdef PLATFORM_HAS_APU
static uint32_t w4_to_nds_duty(float duty) {
	if (duty == 0.125f) {
		return (1 << 24);
	} else if (duty == 0.5f) {
		return (4 << 24);
	} else {
		return (2 << 24);
	}
}
static uint32_t w4_to_nds_pan(uint8_t pan) {
	if (pan == 1) {
		return (0 << 16);
	} else if (pan == 2) {
		return (127 << 16);
	} else {
		return (64 << 16);
	}
}

extern apu_time_t apu_time;
apu_initial_data_t *apu_initial_data = NULL;

__attribute__((optimize("unroll-loops")))
void apu_tick(uint32_t value, void * userdata) {
	apu_time = value;
	const apu_channel *chn = apu_initial_data->channels;
	
	for (int i = 0; i <= 1; i++) {
		int16_t volume = w4_apuGetCurrentVolume(&chn[i]) >> 7;
		if (volume > 0) {
			uint16_t freq = w4_apuGetCurrentFrequency(&chn[i]);
			if (volume > 127) volume = 127;
			SCHANNEL_TIMER(8+i) = -(2094624/freq);
			SCHANNEL_CR(8+i) = SCHANNEL_ENABLE | SOUND_REPEAT | SOUND_FORMAT_PSG
				| w4_to_nds_duty(chn->pulse.dutyCycle)
				| w4_to_nds_pan(chn->pan)
				| SOUND_VOL(volume);
		} else {
			SCHANNEL_CR(8+i) = 0;
		}
	}

	for (int i = 2; i <= 2; i++) {
		int16_t volume = w4_apuGetCurrentVolume(&chn[i]) >> 7;
		if (volume > 0) {
			uint16_t freq = w4_apuGetCurrentFrequency(&chn[i]);
			if (volume > 127) volume = 127;
			freq *= 32;
			SCHANNEL_TIMER(10) = -(16756991/freq);
			SCHANNEL_CR(10) = SCHANNEL_ENABLE | SOUND_REPEAT | SOUND_FORMAT_8BIT
				| w4_to_nds_pan(chn->pan)
				| SOUND_VOL(volume);
		} else {
			SCHANNEL_CR(10) = 0;
		}
	}
	for (int i = 3; i <= 3; i++) {
		int16_t volume = w4_apuGetCurrentVolume(&chn[i]) >> 7;
		if (volume > 0) {
			uint16_t freq = w4_apuGetCurrentFrequency(&chn[i]);
			if (volume > 127) volume = 127;
			uint32_t freq_real = ((uint64_t) (freq * freq) * 44100) / 1000000;
			SCHANNEL_TIMER(15) = -(16756991/freq_real);
			SCHANNEL_CR(15) = SCHANNEL_ENABLE | SOUND_REPEAT | SOUND_FORMAT_PSG
				| w4_to_nds_pan(chn->pan)
				| SOUND_VOL(volume);
		} else {
			SCHANNEL_CR(15) = 0;
		}
	}
}

void apu_receive_data(void * address, void * userdata) {
	apu_initial_data = address;

	REG_SOUNDCNT = SOUND_ENABLE;
	REG_MASTER_VOLUME = 127;

	SCHANNEL_SOURCE(10) = apu_initial_data->sample_triangle;
	SCHANNEL_REPEAT_POINT(10) = 0;
	SCHANNEL_LENGTH(10) = 8;
}
#endif

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	// clear sound registers
	dmaFillWords(0, (void*)0x04000400, 0x100);

#ifdef PLATFORM_HAS_APU
	REG_SOUNDCNT |= SOUND_ENABLE;
	writePowerManagement(PM_CONTROL_REG, ( readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE ) | PM_SOUND_AMP );
	powerOn(POWER_SOUND);
#endif

	readUserSettings();
	ledBlink(0);

	irqInit();
	initClockIRQ();
	fifoInit();
	touchInit();

	SetYtrigger(80);

#ifdef PLATFORM_HAS_APU
	fifoSetAddressHandler(FIFO_USER_01, apu_receive_data, NULL);
	fifoSetValue32Handler(FIFO_USER_01, apu_tick, NULL);
#endif
	installSystemFIFO();

	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);

	setPowerButtonCB(powerButtonCB);
	// Keep the ARM7 mostly idle
	while (!exitflag) {
		if ( 0 == (REG_KEYINPUT & (KEY_SELECT | KEY_START | KEY_L | KEY_R))) {
			exitflag = true;
		}
		swiWaitForVBlank();
	}
	return 0;
}
