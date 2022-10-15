#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "apu.h"
#include "platform.h"
#include "platform_shim.h"

bool N3DS_soundFillBlock;
ndspWaveBuf N3DS_audioBuf[2];
u8*	N3DS_audioData;
u32	N3DS_bufferSize, N3DS_sampleSize;

void N3DS_SoundCallback(void* dud)
{
	if (N3DS_audioBuf[N3DS_soundFillBlock].status == NDSP_WBUF_DONE) {
        w4_apuWriteSamples(N3DS_audioBuf[N3DS_soundFillBlock].data_pcm16, N3DS_bufferSize);
		DSP_FlushDataCache(N3DS_audioBuf[N3DS_soundFillBlock].data_pcm8, N3DS_bufferSize * N3DS_sampleSize);

		ndspChnWaveBufAdd(0, &N3DS_audioBuf[N3DS_soundFillBlock]);
		N3DS_soundFillBlock = !N3DS_soundFillBlock;
	}
}

void N3DS_ClearAudioData(void)
{
	memset(N3DS_audioData, 0, N3DS_bufferSize * 2 * N3DS_sampleSize);
    DSP_FlushDataCache(N3DS_audioData, N3DS_bufferSize * 2 * N3DS_sampleSize);
	N3DS_soundFillBlock = false;
}

void platform_apu_init(void) {
	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = mix[1] = 1.0f;

	N3DS_bufferSize = PLATFORM_APU_SAMPLE_RATE / 30;
	N3DS_sampleSize = 4;
	N3DS_audioData = (u8*) linearAlloc(N3DS_bufferSize * 2 * N3DS_sampleSize);

	ndspInit();

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);

	ndspChnReset(0);
	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, PLATFORM_APU_SAMPLE_RATE);
	ndspChnSetFormat(0, NDSP_CHANNELS(2) | NDSP_ENCODING(NDSP_ENCODING_PCM16));
	ndspChnSetMix(0, mix);

	ndspSetOutputCount(1);
	ndspSetMasterVol(1.0f);
	ndspSetCallback(N3DS_SoundCallback, N3DS_audioBuf);

	memset(N3DS_audioBuf, 0, sizeof(N3DS_audioBuf));
	N3DS_audioBuf[0].data_vaddr = &N3DS_audioData[0];
	N3DS_audioBuf[0].nsamples = N3DS_bufferSize;
	N3DS_audioBuf[1].data_vaddr = &N3DS_audioData[N3DS_bufferSize * N3DS_sampleSize];
	N3DS_audioBuf[1].nsamples = N3DS_bufferSize;

	N3DS_ClearAudioData();

	ndspChnWaveBufAdd(0, &N3DS_audioBuf[0]);
	ndspChnWaveBufAdd(0, &N3DS_audioBuf[1]);
}

void platform_apu_deinit(void) {
	linearFree(N3DS_audioData);
	ndspExit();
}