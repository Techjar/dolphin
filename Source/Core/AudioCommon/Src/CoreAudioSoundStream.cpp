// Copyright (C) 2003 Dolphin Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official SVN repository and contact information can be found at
// http://code.google.com/p/dolphin-emu/

#include "CoreAudioSoundStream.h"

OSStatus callback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags,
			const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
			UInt32 inNumberFrames, AudioBufferList *ioData)
{
	for (UInt32 i = 0; i < ioData->mNumberBuffers; i++)
	{
		((CoreAudioSound *)inRefCon)-> \
			RenderSamples(ioData->mBuffers[i].mData,
					ioData->mBuffers[i].mDataByteSize);
	}

	return noErr;
}

void CoreAudioSound::RenderSamples(void *target, UInt32 size)
{
	m_mixer->Mix((short *)target, size / 4);
}

CoreAudioSound::CoreAudioSound(CMixer *mixer) : SoundStream(mixer)
{
}

CoreAudioSound::~CoreAudioSound()
{
}

bool CoreAudioSound::Start()
{
	OSStatus err;
	UInt32 enableIO;
	AURenderCallbackStruct callback_struct;
	AudioStreamBasicDescription format;

	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_HALOutput;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;

	Component component = FindNextComponent(NULL, &desc);
	if (component == NULL) {
		printf("error finding audio component\n");
		return false;
	}

	err = OpenAComponent(component, &audioUnit);
	if (err != noErr) {
		printf("error opening audio component\n");
		return false;
	}

	enableIO = 1;
	AudioUnitSetProperty(audioUnit,
				kAudioOutputUnitProperty_EnableIO,
				kAudioUnitScope_Output, 0, &enableIO,
				sizeof enableIO);

	FillOutASBDForLPCM(format, m_mixer->GetSampleRate(),
				2, 16, 16, false, false, false);
	err = AudioUnitSetProperty(audioUnit,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Input, 0, &format,
				sizeof(AudioStreamBasicDescription));
	if (err != noErr) {
		printf("error setting audio format\n");
		return false;
	}

	callback_struct.inputProc = callback;
	callback_struct.inputProcRefCon = this;
	err = AudioUnitSetProperty(audioUnit,
				kAudioUnitProperty_SetRenderCallback,
				kAudioUnitScope_Input, 0, &callback_struct,
				sizeof callback_struct);
	if (err != noErr) {
		printf("error setting audio callback\n");
		return false;
	}

	err = AudioUnitInitialize(audioUnit);
	if (err != noErr) {
		printf("error initializing audiounit\n");
		return false;
	}

	err = AudioOutputUnitStart(audioUnit);
	if (err != noErr) {
		printf("error starting audiounit\n");
		return false;
	}

	return true;
}

void CoreAudioSound::SoundLoop()
{
}

void CoreAudioSound::Stop()
{
	OSStatus err;

	err = AudioOutputUnitStop(audioUnit);
	if (err != noErr) {
		printf("error stopping audiounit\n");
		return;
	}

	err = AudioUnitUninitialize(audioUnit);
	if (err != noErr) {
		printf("error uninitializing audiounit\n");
		return;
	}

	err = CloseComponent(audioUnit);
	if (err != noErr) {
		printf("error while closing audio component\n");
		return;
	}
}

void CoreAudioSound::Update()
{
}
