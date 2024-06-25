#ifndef DC801_EMBEDDED
#include "EngineAudio.h"
#include <cstring>
#include <iostream>

/*

	The Audio Player is implemented as a single linked list. This could have been done in a more
	C++ way (ie with a std::vector), however this implementation more or less reflects how EasyDMA
	works on the NRF52840.

	The SDL audio subsytem has you load a file, then unpause audio. From there, a callback is called
	asking you to feed the configured number of bytes (in this case, (n_channels * samples) 8192 bytes)
	each iteration. The audio subsystem then plays that audio through the configured device.

	The EasyDMA subsystem will ask you to configure a buffer then trigger a transfer. During that transfer,
	a callback function is called asking for more data based on the configured size. This design works
	and will make it easier to port to the embedded system.


 */
uint32_t AudioPlayer::soundCount = 0;

// Constructor:
//  - Initialize SDL audio
//  - Configure the SDL device structure
//  - Setup the root to the list of audio samples
//  - Unpause SDL audio (callbacks start now)
AudioPlayer::AudioPlayer()
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		std::cout << "Audio Player: Failed to init SDL audio" << std::endl;
		return;
	}

	const auto useDefaultDevice = (const char*)nullptr;
	const int outputOnly = 0;
	const int allowNoChanges = 0; //SDL_AUDIO_ALLOW_ANY_CHANGE
	SDL_OpenAudioDevice(useDefaultDevice, outputOnly, &device.spec, NULL, allowNoChanges);

	if (device.id == 0)
	{
		std::cout << "Audio Player: Failed to open audio device" << std::endl;
		std::cout << "    " << SDL_GetError() << std::endl;
		return;
	}

	device.enabled = true;

	// Initialize cmixer
	cm_init(AUDIO_FREQUENCY);
	//cm_set_lock(&mutex);
	cm_set_master_gain(1.0);

	SDL_PauseAudioDevice(device.id, 0);
}

// Destructor:
// - Pause the Audio device
// - Free and delete all audio samples
// - Close the audio device

// Whether the SDL calls here are actually necessary remains to be seen.
// main() calls SDL_Quit() before this destructor is called, which should
// be cleaning up everything SDL, including pausing an closing.
AudioPlayer::~AudioPlayer()
{
	if (device.enabled)
	{
		// Pause SDL audio
		SDL_PauseAudioDevice(device.id, 1);
		// Free and delete all samples
		head.reset(nullptr);
		// Close SDL audio device
		SDL_CloseAudioDevice(device.id);
	}
}

// Callbacks issued by SDL
//  Here we:
//   - Silence the SDL buffer (done by cmixer)
//   - Walk the tree, mixing all samples in progress
//   - Loop audio samples, fading the volume when necessary
//   - Free finished samples
void AudioPlayer::callback(nrfx_i2s_buffers_t* stream, uint32_t len)
{
	// Capture the root of the list
	auto previous = head.get();
	if (previous) {
		// Pull the first audio sample
		auto audio = previous->next.get();

		//memset(stream, 0, len);
		// Walk the list
		while (audio != NULL)
		{
			cm_Source* source = audio->source;

			// If this sample has remaining data to play
			if (audio->end == false)
			{
				// And the audio is being faded
				if (audio->fade == 1)
				{
					// Fade until gain is zero, then truncate sample to zero length
					if (source->gain > 0.0)
					{
						cm_set_gain(source, source->gain - 0.1);
					}
					else
					{
						audio->end = true;
					}
				}

				// Mix with cmixer
				cm_process(reinterpret_cast<cm_Int16*>(stream), len / sizeof(cm_Int16));

				// Continue to the next item
				previous = audio;
			}
			// Otherwise, this sample is finished
			else
			{
				// Offset our sample count for non-looped samples
				if (source->loop == 0)
				{
					soundCount -= 1;
				}
				previous->next = std::move(audio->next);
			}

			// Iterate to the next sample
			audio = previous->next.get();
		}
	}
}

// Indicate that an audio sample should be faded out and removed
// The current driver only allows one looped audio sample
void AudioPlayer::fadeAudio()
{
	// Walk the tree
	for (auto audio =head.get();
		audio && audio->source;
		audio = audio->next.get())
	{
		// Find any looped samples and fade them
		if (audio->source->loop == 1)
		{
			audio->source->loop = 0;
			audio->fade = 1;
		}
	}
}

// Add an audio sample to the end of the list
void AudioPlayer::addAudio(Audio* audio)
{
	auto root = head.get();
	// Sanity check
	if (!root)
	{
		return;
	}

	// Walk the tree to the end
	while (root->next.get())
	{
		root = root->next.get();
	}

	// Link the new item to the end
	root->next = std::unique_ptr<Audio>{ audio };
}

// Loads a wave file and adds it to the list of samples to be played
void AudioPlayer::playAudio(const char* filename, bool loop, double gain)
{
	// Sanity check
	if (filename == NULL)
	{
		return;
	}

	// If the audio player isn't enabled
	if (device.enabled == false)
	{
		return;
	}

	// If we've reached the maximum number of audio samples
	if (soundCount >= AUDIO_MAX_SOUNDS)
	{
		return;
	}

	// Increment sample counts
	soundCount += 1;

	// Allocate a new audio object
	auto audio = std::make_unique<Audio>(filename);

	if (audio->source == NULL)
	{
		return;
	}

	cm_set_gain(audio->source, gain);

	if (loop == true)
	{
		audio->source->loop = 1;
	}
	else
	{
		audio->source->loop = 0;
	}

	// Tell cmixer to play the audio source
	cm_play(audio->source);

	// Sample is loaded, it's good to free
	audio->free = 1;

	// The callback is issued for the head of the list, and the head only.
	// We don't need a callback for each individual sample, since the head
	// callback walks the entire tree and mixes all samples.
	audio->spec.callback = NULL;
	audio->spec.userdata = NULL;

	// Temporarily stop the audio callback
	SDL_LockAudioDevice(device.id);

	// If the sample we're adding is looped, fade any and all existing looped samples
	if (loop == true)
	{
		//reinterpret_cast<Audio*>(device.spec.userdata)
		fadeAudio();
	}

	// Append the sample to the end of the list
	addAudio(reinterpret_cast<Audio*>(device.spec.userdata));

	// Resume audio callback
	SDL_UnlockAudioDevice(device.id);
}

// Public interface: Play an audio sound
void AudioPlayer::play(const char* name, double gain)
{
	playAudio(name, false, gain);
}

// Public interface: Play a looped sound
void AudioPlayer::loop(const char* name, double gain)
{
	playAudio(name, true, gain);
}

void AudioPlayer::stop_loop()
{
	for (auto audio = head.get();
		audio && audio->source;
		audio = audio->next.get())
	{
		// Find any looped samples and end them
		if (audio->source->loop != 0)
		{
			audio->end = true;
		}
	}
}
#endif //DC801_EMBEDDED
