#include "instruction.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "dictionary.h"
#include "error.h"
#include "memory_stream.h"

#define SMPS2ASM_VERSION 1
#define PSG_DELTA 12

size_t file_offset;
unsigned int target_driver;

static unsigned int source_driver;
static unsigned int target_smps2asm_version;
static size_t song_start_address;
static unsigned int current_voice;

static const char *notes[] = {
	"nRst", "nC0", "nCs0", "nD0", "nEb0", "nE0", "nF0", "nFs0", "nG0", "nAb0", "nA0", "nBb0", "nB0", "nC1", "nCs1", "nD1",
	"nEb1", "nE1", "nF1", "nFs1", "nG1", "nAb1", "nA1", "nBb1", "nB1", "nC2", "nCs2", "nD2", "nEb2", "nE2", "nF2", "nFs2",
	"nG2", "nAb2", "nA2", "nBb2", "nB2", "nC3", "nCs3", "nD3", "nEb3", "nE3", "nF3", "nFs3", "nG3", "nAb3", "nA3", "nBb3",
	"nB3", "nC4", "nCs4", "nD4", "nEb4", "nE4", "nF4", "nFs4", "nG4", "nAb4", "nA4", "nBb4", "nB4", "nC5", "nCs5", "nD5",
	"nEb5", "nE5", "nF5", "nFs5", "nG5", "nAb5", "nA5", "nBb5", "nB5", "nC6", "nCs6", "nD6", "nEb6", "nE6", "nF6", "nFs6",
	"nG6", "nAb6", "nA6", "nBb6", "nB6", "nC7", "nCs7", "nD7", "nEb7", "nE7", "nF7", "nFs7", "nG7", "nAb7", "nA7", "nBb7"
};

static const struct {char *name; unsigned int value;} octave_pitches[] = {
	{"smpsPitch10lo", 0x88},
	{"smpsPitch09lo", 0x94},
	{"smpsPitch08lo", 0xA0},
	{"smpsPitch07lo", 0xAC},
	{"smpsPitch06lo", 0xB8},
	{"smpsPitch05lo", 0xC4},
	{"smpsPitch04lo", 0xD0},
	{"smpsPitch03lo", 0xDC},
	{"smpsPitch02lo", 0xE8},
	{"smpsPitch01lo", 0xF4},
	{"smpsPitch00",   0x00},
	{"smpsPitch01hi", 0x0C},
	{"smpsPitch02hi", 0x18},
	{"smpsPitch03hi", 0x24},
	{"smpsPitch04hi", 0x30},
	{"smpsPitch05hi", 0x3C},
	{"smpsPitch06hi", 0x48},
	{"smpsPitch07hi", 0x54},
	{"smpsPitch08hi", 0x60},
	{"smpsPitch09hi", 0x6C},
	{"smpsPitch10hi", 0x78}
};

static const char *psg_envelopes_s1[] = {
	"fTone_01", "fTone_02", "fTone_03", "fTone_04", "fTone_05", "fTone_06",
	"fTone_07", "fTone_08", "fTone_09"
};

static const char *psg_envelopes_s2[] = {
	"fTone_01", "fTone_02", "fTone_03", "fTone_04", "fTone_05", "fTone_06",
	"fTone_07", "fTone_08", "fTone_09", "fTone_0A", "fTone_0B", "fTone_0C",
	"fTone_0D"
};

static const char *psg_envelopes_s3k[] = {
	"sTone_01", "sTone_02", "sTone_03", "sTone_04", "sTone_05", "sTone_06",
	"sTone_07", "sTone_08", "sTone_09", "sTone_0A", "sTone_0B", "sTone_0C",
	"sTone_0D", "sTone_0E", "sTone_0F", "sTone_10", "sTone_11", "sTone_12",
	"sTone_13", "sTone_14", "sTone_15", "sTone_16", "sTone_17", "sTone_18",
	"sTone_19", "sTone_1A", "sTone_1B", "sTone_1C", "sTone_1D", "sTone_1E",
	"sTone_1F", "sTone_20", "sTone_21", "sTone_22", "sTone_23", "sTone_24",
	"sTone_25", "sTone_26", "sTone_27"
};

static const char *dac_samples_s2[] = {
	"dKick", "dSnare", "dClap", "dScratch", "dTimpani", "dHiTom", "dVLowClap", "dHiTimpani", "dMidTimpani",
	"dLowTimpani", "dVLowTimpani", "dMidTom", "dLowTom", "dFloorTom", "dHiClap",
	"dMidClap", "dLowClap"
};

static const char *dac_samples_s3_sk_s3d_common[] = {
	"dSnareS3", "dHighTom", "dMidTomS3", "dLowTomS3", "dFloorTomS3", "dKickS3", "dMuffledSnare",
	"dCrashCymbal", "dRideCymbal", "dLowMetalHit", "dMetalHit", "dHighMetalHit",
	"dHigherMetalHit", "dMidMetalHit", "dClapS3", "dElectricHighTom",
	"dElectricMidTom", "dElectricLowTom", "dElectricFloorTom",
	"dTightSnare", "dMidpitchSnare", "dLooseSnare", "dLooserSnare",
	"dHiTimpaniS3", "dLowTimpaniS3", "dMidTimpaniS3", "dQuickLooseSnare",
	"dClick", "dPowerKick", "dQuickGlassCrash"
};

static const char *dac_samples_s3_sk_common[] = {
	"dGlassCrashSnare", "dGlassCrash", "dGlassCrashKick", "dQuietGlassCrash",
	"dOddSnareKick", "dKickExtraBass", "dComeOn", "dDanceSnare", "dLooseKick",
	"dModLooseKick", "dWoo", "dGo", "dSnareGo", "dPowerTom", "dHiWoodBlock", "dLowWoodBlock",
	"dHiHitDrum", "dLowHitDrum", "dMetalCrashHit", "dEchoedClapHit",
	"dLowerEchoedClapHit", "dHipHopHitKick", "dHipHopHitPowerKick",
	"dBassHey", "dDanceStyleKick", "dHipHopHitKick2", "dHipHopHitKick3",
	"dReverseFadingWind", "dScratchS3", "dLooseSnareNoise", "dPowerKick2",
	"dCrashingNoiseWoo", "dQuickHit", "dKickHey", "dPowerKickHit",
	"dLowPowerKickHit", "dLowerPowerKickHit", "dLowestPowerKickHit"
};

static const char *dac_samples_s3d[] = {
	"dFinalFightMetalCrash", "dIntroKick"
};

static const char *dac_samples_s3[] = {
	"dEchoedClapHit_S3", "dLowerEchoedClapHit_S3"
};

static const unsigned char smpsNoAttack = 0xE7;

static void WriteByte(unsigned char value)
{
	MemoryStream_WriteByte(output_stream, value);
}

static void WriteShort(unsigned short value)
{
	if (target_driver >= 2)
	{
		WriteByte(value & 0xFF);
		WriteByte(value >> 8);
	}
	else
	{
		WriteByte(value >> 8);
		WriteByte(value & 0xFF);
	}
}

static size_t GetLogicalAddress(void)
{
	return MemoryStream_GetPosition(output_stream) + file_offset;
}

static unsigned int conv0To256(unsigned int n)
{
	return ((n == 0) << 8) | n;
}

static unsigned int s2TempotoS1(unsigned int n)
{
	return (((768 - n) >> 1) / (256 - n)) & 0xFF;
}

static unsigned int s2TempotoS3(unsigned int n)
{
	return (0x100 - ((n == 0) | n)) & 0xFF;
}

static unsigned int s1TempotoS2(unsigned int n)
{
	return ((((conv0To256(n) - 1) << 8) + (conv0To256(n) >> 1)) / conv0To256(n)) & 0xFF;
}

static unsigned int s1TempotoS3(unsigned int n)
{
	return s2TempotoS3(s1TempotoS2(n));
}

static unsigned int s3TempotoS1(unsigned int n)
{
	return s2TempotoS1(s2TempotoS3(n));
}

static unsigned int s3TempotoS2(unsigned int n)
{
	return s2TempotoS3(n);
}

static unsigned int convertMainTempoMod(unsigned int mod)
{
	unsigned int result;

	if ((source_driver >= 3 && target_driver >= 3) || source_driver == target_driver)
	{
		result = mod;
	}
	else if (source_driver == 1)
	{
		if (mod == 1)
			PrintError("Error: Invalid main tempo of 1 in song from Sonic 1\n");

		if (target_driver == 2)
			result = s1TempotoS2(mod);
		else //if (target_driver >= 3)
			result = s1TempotoS3(mod);
	}
	else if (source_driver == 2)
	{
		if (mod == 0)
			PrintError("Error: Invalid main tempo of 0 in song from Sonic 2\n");

		if (target_driver == 1)
			result = s2TempotoS1(mod);
		else //if (target_driver >= 3)
			result = s2TempotoS3(mod);
	}
	else// if (source_driver >= 3)
	{
		if (mod == 0)
			printf("Warning: Performing approximate conversion of Sonic 3 main tempo modifier of 0\n");

		if (target_driver == 1)
			result = s3TempotoS1(mod);
		else //if (target_driver == 2)
			result = s3TempotoS2(mod);
	}

	return result;
}

static void CheckedChannelPointer(unsigned int loc)
{
	if (target_driver >= 2)
	{
		WriteShort(loc);
	}
	else
	{
		if (loc < song_start_address)
			PrintError("Error: Tracks for Sonic 1 songs must come after the start of the song\n");

		WriteShort(loc - song_start_address);
	}
}

static unsigned int PSGPitchConvert(unsigned int pitch)
{
	unsigned int value;

	if (target_driver >= 3 && source_driver < 3)
		value = (pitch + PSG_DELTA) & 0xFF;
	else if (target_driver < 3 && source_driver >= 3)
		value = (pitch - PSG_DELTA) & 0xFF;
	else
		value = pitch;

	return value;
}

void FillDefaultDictionary(void)
{
	for (unsigned int i = 0; i < sizeof(notes) / sizeof(notes[0]); ++i)
		AddDictionaryEntry(notes[i], 0x80 + i);

	for (unsigned int i = 0; i < sizeof(octave_pitches) / sizeof(octave_pitches[0]); ++i)
		AddDictionaryEntry(octave_pitches[i].name, octave_pitches[i].value);

	AddDictionaryEntry("smpsNoAttack", smpsNoAttack);

	if (target_driver > 2)
	{
		AddDictionaryEntry("nMaxPSG", LookupDictionary("nBb6") - PSG_DELTA);
		AddDictionaryEntry("nMaxPSG1", LookupDictionary("nBb6"));
		AddDictionaryEntry("nMaxPSG2", LookupDictionary("nB6"));
	}
	else
	{
		AddDictionaryEntry("nMaxPSG", LookupDictionary("nA5"));
		AddDictionaryEntry("nMaxPSG1", LookupDictionary("nA5") + PSG_DELTA);
		AddDictionaryEntry("nMaxPSG2", LookupDictionary("nA5") + PSG_DELTA);
	}

	if (target_driver == 1)
	{
		for (unsigned int i = 0; i < sizeof(psg_envelopes_s1) / sizeof(psg_envelopes_s1[0]); ++i)
			AddDictionaryEntry(psg_envelopes_s1[i], 1 + i);
	}
	else if (target_driver == 2)
	{
		for (unsigned int i = 0; i < sizeof(psg_envelopes_s2) / sizeof(psg_envelopes_s2[0]); ++i)
			AddDictionaryEntry(psg_envelopes_s2[i], 1 + i);
	}
	else
	{
		unsigned int current_id = 1;

		for (unsigned int i = 0; i < sizeof(psg_envelopes_s3k) / sizeof(psg_envelopes_s3k[0]); ++i)
			AddDictionaryEntry(psg_envelopes_s3k[i], current_id++);

		for (unsigned int i = 0; i < sizeof(psg_envelopes_s2) / sizeof(psg_envelopes_s2[0]); ++i)
			AddDictionaryEntry(psg_envelopes_s2[i], current_id++);
	}

	if (target_driver == 1)
	{
		AddDictionaryEntry("dKick", 0x81);
		AddDictionaryEntry("dSnare", 0x82);
		AddDictionaryEntry("dTimpani", 0x83);
		AddDictionaryEntry("dHiTimpani", 0x88);
		AddDictionaryEntry("dMidTimpani", 0x89);
		AddDictionaryEntry("dLowTimpani", 0x8A);
		AddDictionaryEntry("dVLowTimpani", 0x8B);
	}
	else if (target_driver == 2)
	{
		for (unsigned int i = 0; i < sizeof(dac_samples_s2) / sizeof(dac_samples_s2[0]); ++i)
			AddDictionaryEntry(dac_samples_s2[i], 0x81 + i);
	}
	else if (target_driver == 3)
	{
		unsigned int current_id = 0x81;

		for (unsigned int i = 0; i < sizeof(dac_samples_s3_sk_s3d_common) / sizeof(dac_samples_s3_sk_s3d_common[0]); ++i)
			AddDictionaryEntry(dac_samples_s3_sk_s3d_common[i], current_id++);

		for (unsigned int i = 0; i < sizeof(dac_samples_s3_sk_common) / sizeof(dac_samples_s3_sk_common[0]); ++i)
			AddDictionaryEntry(dac_samples_s3_sk_common[i], current_id++);

		for (unsigned int i = 0; i < sizeof(dac_samples_s3) / sizeof(dac_samples_s3[0]); ++i)
			AddDictionaryEntry(dac_samples_s3[i], current_id++);
	}
	else if (target_driver == 4)
	{
		unsigned int current_id = 0x81;

		for (unsigned int i = 0; i < sizeof(dac_samples_s3_sk_s3d_common) / sizeof(dac_samples_s3_sk_s3d_common[0]); ++i)
			AddDictionaryEntry(dac_samples_s3_sk_s3d_common[i], current_id++);

		for (unsigned int i = 0; i < sizeof(dac_samples_s3_sk_common) / sizeof(dac_samples_s3_sk_common[0]); ++i)
			AddDictionaryEntry(dac_samples_s3_sk_common[i], current_id++);
	}
	else //if (target_driver == 5)
	{
		unsigned int current_id = 0x81;

		for (unsigned int i = 0; i < sizeof(dac_samples_s3_sk_s3d_common) / sizeof(dac_samples_s3_sk_s3d_common[0]); ++i)
			AddDictionaryEntry(dac_samples_s3_sk_s3d_common[i], current_id++);

		for (unsigned int i = 0; i < sizeof(dac_samples_s3_sk_common) / sizeof(dac_samples_s3_sk_common[0]); ++i)
			AddDictionaryEntry(dac_samples_s3_sk_common[i], current_id++);

		for (unsigned int i = 0; i < sizeof(dac_samples_s2) / sizeof(dac_samples_s2[0]); ++i)
			AddDictionaryEntry(dac_samples_s2[i], current_id++);

		for (unsigned int i = 0; i < sizeof(dac_samples_s3d) / sizeof(dac_samples_s3d[0]); ++i)
			AddDictionaryEntry(dac_samples_s3d[i], current_id++);

		for (unsigned int i = 0; i < sizeof(dac_samples_s3) / sizeof(dac_samples_s3[0]); ++i)
			AddDictionaryEntry(dac_samples_s3[i], current_id++);
	}

	AddDictionaryEntry("panNone", 0x00);
	AddDictionaryEntry("panRight", 0x40);
	AddDictionaryEntry("panLeft", 0x80);
	AddDictionaryEntry("panCentre", 0xC0);
	AddDictionaryEntry("panCenter", 0xC0);

	AddDictionaryEntry("cPSG1", 0x80);
	AddDictionaryEntry("cPSG2", 0xA0);
	AddDictionaryEntry("cPSG3", 0xC0);
	AddDictionaryEntry("cNoise", 0xE0);
	AddDictionaryEntry("cFM3", 0x02);
	AddDictionaryEntry("cFM4", 0x04);
	AddDictionaryEntry("cFM5", 0x05);
	AddDictionaryEntry("cFM6", 0x06);
}

void HandleLabel(char *label)
{
	AddDictionaryEntry(label, GetLogicalAddress());
}

static void Macro_smpsStop(unsigned int arg_count, unsigned int arg_array[]);

static void Macro_smpsHeaderStartSong(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	song_start_address = GetLogicalAddress();
	current_voice = 0;

	source_driver = arg_array[0];
	target_smps2asm_version = (arg_count >= 2) ? arg_array[1] : 0;

	if (target_smps2asm_version > SMPS2ASM_VERSION)
		PrintError("Error: Song targets a newer version of SMPS2ASM than what this tool supports (it wants version %d)\n", target_smps2asm_version);

	if (undefined_symbol)
		PrintError("Error: smpsHeaderStartSong must be evaluable on first pass\n");
}

static void Macro_smpsHeaderVoice(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (song_start_address != GetLogicalAddress())
		PrintError("Error: Missing smpsHeaderStartSong\n");

	if (target_driver >= 2)
		WriteShort(arg_array[0]);
	else
		WriteShort(arg_array[0] - song_start_address);
}

static void Macro_smpsHeaderVoiceNull(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	if (song_start_address != GetLogicalAddress())
		PrintError("Error: Missing smpsHeaderStartSong\n");

	WriteShort(0);
}

static void Macro_smpsHeaderVoiceUVB(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	if (song_start_address != GetLogicalAddress())
		PrintError("Error: Missing smpsHeaderStartSong\n");

	if (target_driver == 3 || target_driver == 4)
		WriteShort(0x17D8);
	else if (target_driver == 5)
		PrintError("Error: smpsHeaderVoiceUVB not supported in Flamewing's driver yet\n");
	else
		PrintError("Error: smpsHeaderVoiceUVB not supported in S1/S2's drivers\n");
}

static void Macro_smpsHeaderChan(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	WriteByte(arg_array[0]);	// DAC+FM channel count
	WriteByte(arg_array[1]);	// PSG channel count
}

static void Macro_smpsHeaderTempo(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	WriteByte(arg_array[0]);
	WriteByte(convertMainTempoMod(arg_array[1]));
}

static void Macro_smpsHeaderDAC(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	CheckedChannelPointer(arg_array[0]);		// Location
	WriteByte((arg_count >= 2) ? arg_array[1] : 0);	// Pitch
	WriteByte((arg_count >= 3) ? arg_array[2] : 0);	// Volume
}

static void Macro_smpsHeaderFM(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 3);

	CheckedChannelPointer(arg_array[0]);	// Location
	WriteByte(arg_array[1]);		// Pitch
	WriteByte(arg_array[2]);		// Volume
}

static void Macro_smpsHeaderPSG(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 5);

	CheckedChannelPointer(arg_array[0]);		// Location
	WriteByte(PSGPitchConvert(arg_array[1]));	// Pitch
	WriteByte(arg_array[2]);			// Volume
	WriteByte(arg_array[3]);			// Modulation
	WriteByte(arg_array[4]);			// Instrument
}

static void Macro_smpsHeaderTempoSFX(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(arg_array[0]);	// Tempo
}

static void Macro_smpsHeaderChanSFX(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(arg_array[0]);	// Channel count
}

static void Macro_smpsHeaderSFXChannel(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	if (target_driver >= 3 && arg_array[0] == LookupDictionary("cNoise"))
		PrintError("Error: Using channel ID of cNoise ($E0) in Sonic 3 driver is dangerous. Fix the song so that it turns into a noise channel instead.\n");
	else if (target_driver < 3 && arg_array[0] == LookupDictionary("cFM6"))
		PrintError("Error: Using channel ID of FM6 ($06) in Sonic 1 or Sonic 2 drivers is unsupported. Change it to another channel.\n");

	WriteByte(0x80);			// Playback-control
	WriteByte(arg_array[0]);		// Channel ID
	CheckedChannelPointer(arg_array[1]);	// Location
	WriteByte((arg_array[0] & 0x80) ? PSGPitchConvert(arg_array[2]) : arg_array[2]);	// Pitch
	WriteByte(arg_array[3]);		// Volume
}

static void Macro_smpsPan(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	WriteByte(0xE0);
	WriteByte(arg_array[0] + arg_array[1]);	// Direction + amsfms
}

static void Macro_smpsDetune(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(0xE1);
	WriteByte(arg_array[0]);
}

static void Macro_smpsNop(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 3)
	{
		WriteByte(0xE2);
		WriteByte(arg_array[0]);
	}
}

static void Macro_smpsReturn(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	WriteByte((target_driver >= 3) ? 0xF9 : 0xE3);
}

static void Macro_smpsFade(unsigned int arg_count, unsigned int arg_array[])
{
	if (target_driver >= 3)
	{
		WriteByte(0xE2);

		if (arg_count >= 1)
			WriteByte(arg_array[0]);

		if (source_driver < 3)
			Macro_smpsStop(arg_count, arg_array);
	}
	else if (source_driver >= 3 && arg_count >= 1 && arg_array[0] != 0xFF)
	{
		// We should ignore these (they're actually smpsNop commands)
	}
	else
	{
		WriteByte(0xE4);
	}
}

static void Macro_smpsChanTempoDiv(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver >= 5)
	{
		WriteByte(0xFF);
		WriteByte(0x08);
		WriteByte(arg_array[0]);
	}
	else if (target_driver >= 3)
	{
		PrintError("Error: Coord. Flag to set tempo divider of a single channel does not exist in S3 driver. Use Flamewing's modified S&K sound driver instead.\n");
	}
	else
	{
		WriteByte(0xE5);
		WriteByte(arg_array[0]);
	}
}

static void Macro_smpsAlterVol(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(0xE6);
	WriteByte(arg_array[0]);
}

static void Macro_smpsNoteFill(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver >= 5 && source_driver < 3)
	{
		WriteByte(0xFF);
		WriteByte(0x0A);
		WriteByte(arg_array[0]);
	}
	else
	{
		if (target_driver >= 3 && source_driver < 3)
			PrintError("Note fill will not work as intended unless you divide the fill value by the tempo divider or complain to Flamewing to add an appropriate coordination flag for it.\n");
		else if (target_driver < 3 && source_driver >= 3)
			PrintError("Note fill will not work as intended unless you multiply the fill value by the tempo divider or complain to Flamewing to add an appropriate coordination flag for it.\n");
	}

	WriteByte(0xE8);
	WriteByte(arg_array[0]);
}

static void Macro_smpsChangeTransposition(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte((target_driver >= 3) ? 0xFB : 0xE9);
	WriteByte(arg_array[0]);
}

static void Macro_smpsSetTempoMod(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver >= 3)
	{
		WriteByte(0xFF);
		WriteByte(0x00);
	}
	else
	{
		WriteByte(0xEA);
	}

	WriteByte(convertMainTempoMod(arg_array[0]));
}

static void Macro_smpsSetTempoDiv(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver >= 3)
	{
		WriteByte(0xFF);
		WriteByte(0x04);
	}
	else
	{
		WriteByte(0xEB);
	}

	WriteByte(arg_array[0]);
}

static void Macro_smpsSetVol(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver >= 3)
	{
		WriteByte(0xE4);
		WriteByte(arg_array[0]);
	}
	else
	{
		PrintError("Error: Coord. Flag to set volume (instead of volume attenuation) does not exist in S1 or S2 drivers. Complain to Flamewing to add it.\n");
	}
}

static void Macro_smpsPSGAlterVol(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(0xEC);
	WriteByte(arg_array[0]);
}

static void Macro_smpsClearPush(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	if (target_driver == 1)
		WriteByte(0xED);
	else
		PrintError("Coord. Flag to clear S1 push block flag does not exist in S2 or S3 drivers. Complain to Flamewing to add it.\n");
}

static void Macro_smpsStopSpecial(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	if (target_driver == 1)
	{
		WriteByte(0xEE);
	}
	else
	{
		printf("Warning: Coord. Flag to stop special SFX does not exist in S2 or S3 drivers. Complain to Flamewing to add it. With adequate caution, smpsStop can do this job.\n");
		Macro_smpsStop(arg_count, arg_array);
	}
}

static void Macro_smpsFMvoice(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(0xEF);

	if (target_driver >= 3 && arg_count >= 2)
	{
		WriteByte(arg_array[0] | 0x80);	// Instrument
		WriteByte(arg_array[1] + 0x81);	// ID of the song containing the instrument
	}
	else
	{
		WriteByte(arg_array[0]);	// Instrument
	}
}

static void Macro_smpsModSet(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	WriteByte(0xF0);

	if (target_driver >= 3 && source_driver < 3)
	{
		WriteByte(arg_array[0] + 1);	// Wait
		WriteByte(arg_array[1]);	// Speed
		WriteByte(arg_array[2]);	// Change
		WriteByte(((arg_array[3] + 1) * arg_array[1]) & 0xFF);	// Step
	}
	else if (target_driver < 3 && source_driver >= 3)
	{
		WriteByte(arg_array[0] - 1);	// Wait
		WriteByte(arg_array[1]);	// Speed
		WriteByte(arg_array[2]);	// Change
		WriteByte(conv0To256(arg_array[3]) / conv0To256(arg_array[1]) - 1);	// Step
	}
	else
	{
		WriteByte(arg_array[0]);	// Wait
		WriteByte(arg_array[1]);	// Speed
		WriteByte(arg_array[2]);	// Change
		WriteByte(arg_array[3]);	// Step
	}
}

static void Macro_smpsModOn(unsigned int arg_count, unsigned int arg_array[])
{
	if (target_driver >= 3)
	{
		WriteByte(0xF4);

		if (arg_count >= 1)
			WriteByte(arg_array[0]);
		else
			WriteByte(0x80);
	}
	else
	{
		if (arg_count >= 1)
			printf("Warning: Modulation envelopes are not supported in Sonic 1 or Sonic 2 drivers. smpsModOn flag won't work properly.\n");
		else
			WriteByte(0xF1);
	}
}

static void Macro_smpsStop(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	WriteByte(0xF2);
}

static void Macro_smpsPSGform(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(0xF3);
	WriteByte(arg_array[0]);
}

static void Macro_smpsModOff(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	WriteByte((target_driver >= 3) ? 0xFA : 0xF4);
}

static void Macro_smpsPSGvoice(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(0xF5);
	WriteByte(arg_array[0]);
}

static void Macro_smpsJump(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(0xF6);

	if (target_driver >= 2)
		WriteShort(arg_array[0]);
	else
		WriteShort(arg_array[0] - GetLogicalAddress() - 1);
}

static void Macro_smpsLoop(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 3);

	WriteByte(0xF7);
	WriteByte(arg_array[0]);	// Index
	WriteByte(arg_array[1]);	// Loops

	if (target_driver >= 2)
		WriteShort(arg_array[2]);	// Location
	else
		WriteShort(arg_array[2] - GetLogicalAddress() - 1);	// Location
}

static void Macro_smpsCall(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	WriteByte(0xF8);

	if (target_driver >= 2)
		WriteShort(arg_array[0]);
	else
		WriteShort(arg_array[0] - GetLogicalAddress() - 1);
}

static void Macro_smpsFMAlterVol(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (arg_count >= 2)
	{
		if (target_driver >= 3)
		{
			WriteByte(0xE5);
			WriteByte(arg_array[0]);	// PSG volume delta (ignored in S3/S&K/S3D's driver)
			WriteByte(arg_array[1]);	// FM volume delta
		}
		else
		{
			WriteByte(0xE6);
			WriteByte(arg_array[1]);	// FM volume delta
		}
	}
	else
	{
		WriteByte(0xE6);
		WriteByte(arg_array[0]);	// FM volume delta
	}
}

static void Macro_smpsStopFM(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	if (target_driver < 3)
		PrintError("Error: smpsStopFM is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xE3);
}

static void Macro_smpsSpindashRev(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	if (target_driver < 3)
		PrintError("Error: smpsSpindashRev is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xE9);
}

static void Macro_smpsPlayDACSample(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 3)
		PrintError("Error: smpsPlayDACSample is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xEA);
	WriteByte(arg_array[0] & 0x7F);
}

static void Macro_smpsConditionalJump(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	if (target_driver < 3)
		PrintError("Error: smpsConditionalJump is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xEB);
	WriteByte(arg_array[0]);
	WriteShort(arg_array[1]);
}

static void Macro_smpsSetNote(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 3)
		PrintError("Error: smpsSetNote is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xED);
	WriteByte(arg_array[0]);
}

static void Macro_smpsFMICommand(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	if (target_driver < 3)
		PrintError("Error: smpsFMICommand is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xEE);
	WriteByte(arg_array[0]);
	WriteByte(arg_array[1]);
}

static void Macro_smpsModChange2(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	if (target_driver < 3)
		PrintError("Error: smpsModChange2 is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xF1);
	WriteByte(arg_array[0]);
	WriteByte(arg_array[1]);
}

static void Macro_smpsModChange(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 3)
		PrintError("Error: smpsModChange is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xF4);
	WriteByte(arg_array[0]);
}

static void Macro_smpsContinuousLoop(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 3)
		PrintError("Error: smpsContinuousLoop is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xFC);
	WriteShort(arg_array[0]);
}

static void Macro_smpsAlternateSMPS(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 3)
		PrintError("Error: smpsAlternateSMPS is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xFD);
	WriteByte(arg_array[0]);
}

static void Macro_smpsFM3SpecialMode(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	if (target_driver < 3)
		PrintError("Error: smpsFM3SpecialMode is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xFE);
	WriteByte(arg_array[0]);
	WriteByte(arg_array[1]);
	WriteByte(arg_array[2]);
	WriteByte(arg_array[3]);
}

static void Macro_smpsPlaySound(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 3)
		PrintError("Error: smpsPlaySound is not supported in Sonic 1 or Sonic 2's driver\n");
	else if (target_driver >= 5)
		printf("smpsPlaySound only plays SFX in Flamedriver; use smpsPlayMusic to play music or fade effects.");

	WriteByte(0xFF);
	WriteByte(0x01);
	WriteByte(arg_array[0]);
}

static void Macro_smpsHaltMusic(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 3)
		PrintError("Error: smpsHaltMusic is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xFF);
	WriteByte(0x02);
	WriteByte(arg_array[0]);
}

static void Macro_smpsCopyData(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	if (target_driver < 3)
		PrintError("Error: smpsCopyData is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xFF);
	WriteByte(0x03);
	WriteShort(arg_array[0]);
	WriteByte(arg_array[1]);
}

static void Macro_smpsSSGEG(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	if (target_driver < 3)
		PrintError("Error: smpsSSGEG is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xFF);
	WriteByte(0x05);
	WriteByte(arg_array[0]);
	WriteByte(arg_array[1]);
	WriteByte(arg_array[2]);
	WriteByte(arg_array[3]);
}

static void Macro_smpsFMVolEnv(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	if (target_driver < 3)
		PrintError("Error: smpsFMVolEnv is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xFF);
	WriteByte(0x06);
	WriteByte(arg_array[0]);
	WriteByte(arg_array[1]);
}

static void Macro_smpsResetSpindashRev(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	if (target_driver < 3)
		PrintError("Error: smpsResetSpindashRev is not supported in Sonic 1 or Sonic 2's driver\n");

	WriteByte(0xFF);
	WriteByte(0x07);
}

static void Macro_smpsChanFMCommand(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	if (target_driver < 5)
		PrintError("Error: smpsChanFMCommand is only supported in Flamewing's driver\n");

	WriteByte(0xFF);
	WriteByte(0x09);
	WriteByte(arg_array[0]);
	WriteByte(arg_array[1]);
}

static void Macro_smpsPitchSlide(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 5)
		PrintError("Error: smpsPitchSlide is only supported in Flamewing's driver\n");

	WriteByte(0xFF);
	WriteByte(0x0B);
	WriteByte(arg_array[0]);
}

static void Macro_smpsSetLFO(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 2);

	if (target_driver < 5)
		PrintError("Error: smpsSetLFO is only supported in Flamewing's driver\n");

	WriteByte(0xFF);
	WriteByte(0x0C);
	WriteByte(arg_array[0]);
	WriteByte(arg_array[1]);
}

static void Macro_smpsPlayMusic(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	if (target_driver < 5)
		PrintError("Error: smpsPlayMusic is only supported in Flamewing's driver\n");

	WriteByte(0xFF);
	WriteByte(0x0D);
	WriteByte(arg_array[0]);
}

static void Macro_smpsMaxRelRate(unsigned int arg_count, unsigned int arg_array[])
{
	(void)arg_count;
	(void)arg_array;

	if (target_driver >= 3)
	{
		Macro_smpsFMICommand(2, (unsigned int[]){0x88, 0x0F});
		Macro_smpsFMICommand(2, (unsigned int[]){0x8C, 0x0F});
	}
	else
	{
		WriteByte(0xF9);
	}
}

static void Macro_smpsAlterNote(unsigned int arg_count, unsigned int arg_array[])
{
	Macro_smpsDetune(arg_count, arg_array);
}

static void Macro_smpsAlterPitch(unsigned int arg_count, unsigned int arg_array[])
{
	Macro_smpsChangeTransposition(arg_count, arg_array);
}

static void Macro_smpsFMFlutter(unsigned int arg_count, unsigned int arg_array[])
{
	Macro_smpsFMVolEnv(arg_count, arg_array);
}

static void Macro_smpsWeirdD1LRR(unsigned int arg_count, unsigned int arg_array[])
{
	Macro_smpsMaxRelRate(arg_count, arg_array);
}

static void Macro_smpsSetvoice(unsigned int arg_count, unsigned int arg_array[])
{
	Macro_smpsFMvoice(arg_count, arg_array);
}

static unsigned int vcFeedback;
static unsigned int vcAlgorithm;
static unsigned int vcUnusedBits;
static unsigned int vcD1R1Unk;
static unsigned int vcD1R2Unk;
static unsigned int vcD1R3Unk;
static unsigned int vcD1R4Unk;
static unsigned int vcDT1;
static unsigned int vcDT2;
static unsigned int vcDT3;
static unsigned int vcDT4;
static unsigned int vcCF1;
static unsigned int vcCF2;
static unsigned int vcCF3;
static unsigned int vcCF4;
static unsigned int vcRS1;
static unsigned int vcRS2;
static unsigned int vcRS3;
static unsigned int vcRS4;
static unsigned int vcAR1;
static unsigned int vcAR2;
static unsigned int vcAR3;
static unsigned int vcAR4;
static unsigned int vcAM1;
static unsigned int vcAM2;
static unsigned int vcAM3;
static unsigned int vcAM4;
static unsigned int vcD1R1;
static unsigned int vcD1R2;
static unsigned int vcD1R3;
static unsigned int vcD1R4;
static unsigned int vcD2R1;
static unsigned int vcD2R2;
static unsigned int vcD2R3;
static unsigned int vcD2R4;
static unsigned int vcDL1;
static unsigned int vcDL2;
static unsigned int vcDL3;
static unsigned int vcDL4;
static unsigned int vcRR1;
static unsigned int vcRR2;
static unsigned int vcRR3;
static unsigned int vcRR4;
static unsigned int vcTL1;
static unsigned int vcTL2;
static unsigned int vcTL3;
static unsigned int vcTL4;
static unsigned int vcTLMask1;
static unsigned int vcTLMask2;
static unsigned int vcTLMask3;
static unsigned int vcTLMask4;

static void Macro_smpsVcFeedback(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	vcFeedback = arg_array[0];
}

static void Macro_smpsVcAlgorithm(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	vcAlgorithm = arg_array[0];
}

static void Macro_smpsVcUnusedBits(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 1);

	vcUnusedBits = arg_array[0];

	if (arg_count >= 5)
	{
		vcD1R1Unk = arg_array[1];
		vcD1R2Unk = arg_array[2];
		vcD1R3Unk = arg_array[3];
		vcD1R4Unk = arg_array[4];
	}
	else
	{
		vcD1R1Unk = 0;
		vcD1R2Unk = 0;
		vcD1R3Unk = 0;
		vcD1R4Unk = 0;
	}
}

static void Macro_smpsVcDetune(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcDT1 = arg_array[0];
	vcDT2 = arg_array[1];
	vcDT3 = arg_array[2];
	vcDT4 = arg_array[3];
}

static void Macro_smpsVcCoarseFreq(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcCF1 = arg_array[0];
	vcCF2 = arg_array[1];
	vcCF3 = arg_array[2];
	vcCF4 = arg_array[3];
}

static void Macro_smpsVcRateScale(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcRS1 = arg_array[0];
	vcRS2 = arg_array[1];
	vcRS3 = arg_array[2];
	vcRS4 = arg_array[3];
}

static void Macro_smpsVcAttackRate(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcAR1 = arg_array[0];
	vcAR2 = arg_array[1];
	vcAR3 = arg_array[2];
	vcAR4 = arg_array[3];
}

static void Macro_smpsVcAmpMod(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	if (target_smps2asm_version == 0)
	{
		vcAM1 = arg_array[0] << 5;
		vcAM2 = arg_array[1] << 5;
		vcAM3 = arg_array[2] << 5;
		vcAM4 = arg_array[3] << 5;
	}
	else
	{
		vcAM1 = arg_array[0] << 7;
		vcAM2 = arg_array[1] << 7;
		vcAM3 = arg_array[2] << 7;
		vcAM4 = arg_array[3] << 7;
	}
}

static void Macro_smpsVcDecayRate1(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcD1R1 = arg_array[0];
	vcD1R2 = arg_array[1];
	vcD1R3 = arg_array[2];
	vcD1R4 = arg_array[3];
}

static void Macro_smpsVcDecayRate2(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcD2R1 = arg_array[0];
	vcD2R2 = arg_array[1];
	vcD2R3 = arg_array[2];
	vcD2R4 = arg_array[3];
}

static void Macro_smpsVcDecayLevel(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcDL1 = arg_array[0];
	vcDL2 = arg_array[1];
	vcDL3 = arg_array[2];
	vcDL4 = arg_array[3];
}

static void Macro_smpsVcReleaseRate(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcRR1 = arg_array[0];
	vcRR2 = arg_array[1];
	vcRR3 = arg_array[2];
	vcRR4 = arg_array[3];
}

static void Macro_smpsVcTotalLevel(unsigned int arg_count, unsigned int arg_array[])
{
	assert(arg_count >= 4);

	vcTL1 = arg_array[0];
	vcTL2 = arg_array[1];
	vcTL3 = arg_array[2];
	vcTL4 = arg_array[3];

	WriteByte((vcUnusedBits << 6) + (vcFeedback << 3) + vcAlgorithm);

	if (target_smps2asm_version == 0)
	{
		vcTLMask4 = ((vcAlgorithm == 7) << 7);
		vcTLMask3 = ((vcAlgorithm >= 4) << 7);
		vcTLMask2 = ((vcAlgorithm >= 5) << 7);
		vcTLMask1 = 0x80;
	}
	else
	{
		vcTLMask4 = 0;
		vcTLMask3 = 0;
		vcTLMask2 = 0;
		vcTLMask1 = 0;
	}

	if (target_driver >= 3 && source_driver < 3)
	{
		vcTLMask4 = ((vcAlgorithm == 7) << 7);
		vcTLMask3 = ((vcAlgorithm >= 4) << 7);
		vcTLMask2 = ((vcAlgorithm >= 5) << 7);
		vcTLMask1 = 0x80;

		vcTL1 &= 0x7F;
		vcTL2 &= 0x7F;
		vcTL3 &= 0x7F;
		vcTL4 &= 0x7F;
	}
	else if (target_driver < 3 && source_driver >= 3 && ((vcTL1 & 0x80) || (vcTL2 & 0x80 && vcAlgorithm >= 5) || (vcTL3 & 0x80 && vcAlgorithm >= 4) || (vcTL4 & 0x80 && vcAlgorithm == 7)))
	{
		printf("Warning: Voice 0x%X has TL bits that do not match its algorithm setting. This voice will not work in S1/S2 drivers.\n", current_voice);
	}

	if (target_driver == 2)
	{
		WriteByte((vcDT4<<4)+vcCF4);
		WriteByte((vcDT2<<4)+vcCF2);
		WriteByte((vcDT3<<4)+vcCF3);
		WriteByte((vcDT1<<4)+vcCF1);
		WriteByte((vcRS4<<6)+vcAR4);
		WriteByte((vcRS2<<6)+vcAR2);
		WriteByte((vcRS3<<6)+vcAR3);
		WriteByte((vcRS1<<6)+vcAR1);
		WriteByte(vcAM4|vcD1R4|vcD1R4Unk);
		WriteByte(vcAM2|vcD1R2|vcD1R2Unk);
		WriteByte(vcAM3|vcD1R3|vcD1R3Unk);
		WriteByte(vcAM1|vcD1R1|vcD1R1Unk);
		WriteByte(vcD2R4);
		WriteByte(vcD2R2);
		WriteByte(vcD2R3);
		WriteByte(vcD2R1);
		WriteByte((vcDL4<<4)+vcRR4);
		WriteByte((vcDL2<<4)+vcRR2);
		WriteByte((vcDL3<<4)+vcRR3);
		WriteByte((vcDL1<<4)+vcRR1);
		WriteByte(vcTL4|vcTLMask4);
		WriteByte(vcTL2|vcTLMask2);
		WriteByte(vcTL3|vcTLMask3);
		WriteByte(vcTL1|vcTLMask1);
	}
	else
	{
		WriteByte((vcDT4<<4)+vcCF4);
		WriteByte((vcDT3<<4)+vcCF3);
		WriteByte((vcDT2<<4)+vcCF2);
		WriteByte((vcDT1<<4)+vcCF1);
		WriteByte((vcRS4<<6)+vcAR4);
		WriteByte((vcRS3<<6)+vcAR3);
		WriteByte((vcRS2<<6)+vcAR2);
		WriteByte((vcRS1<<6)+vcAR1);
		WriteByte(vcAM4|vcD1R4|vcD1R4Unk);
		WriteByte(vcAM3|vcD1R3|vcD1R3Unk);
		WriteByte(vcAM2|vcD1R2|vcD1R2Unk);
		WriteByte(vcAM1|vcD1R1|vcD1R1Unk);
		WriteByte(vcD2R4);
		WriteByte(vcD2R3);
		WriteByte(vcD2R2);
		WriteByte(vcD2R1);
		WriteByte((vcDL4<<4)+vcRR4);
		WriteByte((vcDL3<<4)+vcRR3);
		WriteByte((vcDL2<<4)+vcRR2);
		WriteByte((vcDL1<<4)+vcRR1);
		WriteByte(vcTL4|vcTLMask4);
		WriteByte(vcTL3|vcTLMask3);
		WriteByte(vcTL2|vcTLMask2);
		WriteByte(vcTL1|vcTLMask1);
	}

	++current_voice;
}

static const struct {char *symbol; void (*function)(unsigned int arg_count, unsigned int arg_array[]);} symbol_function_table[] = {
	{"smpsHeaderStartSong", Macro_smpsHeaderStartSong},
	{"smpsHeaderVoice", Macro_smpsHeaderVoice},
	{"smpsHeaderVoiceNull", Macro_smpsHeaderVoiceNull},
	{"smpsHeaderVoiceUVB", Macro_smpsHeaderVoiceUVB},
	{"smpsHeaderChan", Macro_smpsHeaderChan},
	{"smpsHeaderTempo", Macro_smpsHeaderTempo},
	{"smpsHeaderDAC", Macro_smpsHeaderDAC},
	{"smpsHeaderFM", Macro_smpsHeaderFM},
	{"smpsHeaderPSG", Macro_smpsHeaderPSG},
	{"smpsHeaderTempoSFX", Macro_smpsHeaderTempoSFX},
	{"smpsHeaderChanSFX", Macro_smpsHeaderChanSFX},
	{"smpsHeaderSFXChannel", Macro_smpsHeaderSFXChannel},
	{"smpsPan", Macro_smpsPan},
	{"smpsDetune", Macro_smpsDetune},
	{"smpsNop", Macro_smpsNop},
	{"smpsReturn", Macro_smpsReturn},
	{"smpsFade", Macro_smpsFade},
	{"smpsChanTempoDiv", Macro_smpsChanTempoDiv},
	{"smpsAlterVol", Macro_smpsAlterVol},
	{"smpsNoteFill", Macro_smpsNoteFill},
	{"smpsChangeTransposition", Macro_smpsChangeTransposition},
	{"smpsSetTempoMod", Macro_smpsSetTempoMod},
	{"smpsSetTempoDiv", Macro_smpsSetTempoDiv},
	{"smpsSetVol", Macro_smpsSetVol},
	{"smpsPSGAlterVol", Macro_smpsPSGAlterVol},
	{"smpsClearPush", Macro_smpsClearPush},
	{"smpsStopSpecial", Macro_smpsStopSpecial},
	{"smpsFMvoice", Macro_smpsFMvoice},
	{"smpsModSet", Macro_smpsModSet},
	{"smpsModOn", Macro_smpsModOn},
	{"smpsStop", Macro_smpsStop},
	{"smpsPSGform", Macro_smpsPSGform},
	{"smpsModOff", Macro_smpsModOff},
	{"smpsPSGvoice", Macro_smpsPSGvoice},
	{"smpsJump", Macro_smpsJump},
	{"smpsLoop", Macro_smpsLoop},
	{"smpsCall", Macro_smpsCall},
	{"smpsFMAlterVol", Macro_smpsFMAlterVol},
	{"smpsStopFM", Macro_smpsStopFM},
	{"smpsSpindashRev", Macro_smpsSpindashRev},
	{"smpsPlayDACSample", Macro_smpsPlayDACSample},
	{"smpsConditionalJump", Macro_smpsConditionalJump},
	{"smpsSetNote", Macro_smpsSetNote},
	{"smpsModChange2", Macro_smpsModChange2},
	{"smpsModChange", Macro_smpsModChange},
	{"smpsContinuousLoop", Macro_smpsContinuousLoop},
	{"smpsAlternateSMPS", Macro_smpsAlternateSMPS},
	{"smpsFM3SpecialMode", Macro_smpsFM3SpecialMode},
	{"smpsPlaySound", Macro_smpsPlaySound},
	{"smpsHaltMusic", Macro_smpsHaltMusic},
	{"smpsCopyData", Macro_smpsCopyData},
	{"smpsSSGEG", Macro_smpsSSGEG},
	{"smpsFMVolEnv", Macro_smpsFMVolEnv},
	{"smpsResetSpindashRev", Macro_smpsResetSpindashRev},
	{"smpsChanFMCommand", Macro_smpsChanFMCommand},
	{"smpsPitchSlide", Macro_smpsPitchSlide},
	{"smpsSetLFO", Macro_smpsSetLFO},
	{"smpsPlayMusic", Macro_smpsPlayMusic},
	{"smpsMaxRelRate", Macro_smpsMaxRelRate},
	{"smpsAlterNote", Macro_smpsAlterNote},
	{"smpsAlterPitch", Macro_smpsAlterPitch},
	{"smpsFMFlutter", Macro_smpsFMFlutter},
	{"smpsWeirdD1LRR", Macro_smpsWeirdD1LRR},
	{"smpsSetvoice", Macro_smpsSetvoice},
	{"smpsVcFeedback", Macro_smpsVcFeedback},
	{"smpsVcAlgorithm", Macro_smpsVcAlgorithm},
	{"smpsVcUnusedBits", Macro_smpsVcUnusedBits},
	{"smpsVcDetune", Macro_smpsVcDetune},
	{"smpsVcCoarseFreq", Macro_smpsVcCoarseFreq},
	{"smpsVcRateScale", Macro_smpsVcRateScale},
	{"smpsVcAttackRate", Macro_smpsVcAttackRate},
	{"smpsVcAmpMod", Macro_smpsVcAmpMod},
	{"smpsVcDecayRate1", Macro_smpsVcDecayRate1},
	{"smpsVcDecayRate2", Macro_smpsVcDecayRate2},
	{"smpsVcDecayLevel", Macro_smpsVcDecayLevel},
	{"smpsVcReleaseRate", Macro_smpsVcReleaseRate},
	{"smpsVcTotalLevel", Macro_smpsVcTotalLevel},
};

void HandleInstruction(char *opcode, unsigned int arg_count, char *arg_array[])
{
	unsigned int int_arg_array[arg_count];

	for (unsigned int i = 0; i < arg_count; ++i)
		int_arg_array[i] = LookupDictionary(arg_array[i]);

	for (unsigned int i = 0; i < sizeof(symbol_function_table) / sizeof(symbol_function_table[0]); ++i)
	{
		if (!strcmp(opcode, symbol_function_table[i].symbol))
		{
			symbol_function_table[i].function(arg_count, int_arg_array);
			return;
		}
	}

	if (!strcmp(opcode, "dc.b"))
	{
		for (unsigned int i = 0; i < arg_count; ++i)
		{
			const unsigned int value = int_arg_array[i];

			if (value > 0xFF)
				PrintError("Error: dc.b value must fit into a byte\n");

			WriteByte(value);
		}
	}
	else
	{
		PrintError("Error: Unhandled instruction: '%s'\n", opcode);
	}
}
