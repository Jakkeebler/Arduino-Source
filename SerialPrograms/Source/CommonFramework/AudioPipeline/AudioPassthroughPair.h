/*  Audio Passthrough Pair
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_AudioPipeline_AudioPassthroughPair_H
#define PokemonAutomation_AudioPipeline_AudioPassthroughPair_H

#include "AudioInfo.h"

namespace PokemonAutomation{

struct FFTListener;


//  Represents an audio source/sink pair where audio is passed through from
//  source -> sink with minimal latency.
//
//  If desired, listeners can be attached to receive the FFT spectrums.
//
//  This class is fully thread-safe. Both source and sink can be changed
//  asynchronously. Listeners can be attached/detached asynchronously.
//
class AudioPassthroughPair{
public:
    virtual void add_listener(FFTListener& listener) = 0;
    virtual void remove_listener(FFTListener& listener) = 0;

public:
    virtual ~AudioPassthroughPair() = default;

    virtual void clear_audio_source() = 0;
    virtual void set_audio_source(const std::string& file) = 0;
    virtual void set_audio_source(const AudioDeviceInfo& device, AudioChannelFormat format) = 0;

    virtual void clear_audio_sink() = 0;
    virtual void set_audio_sink(const AudioDeviceInfo& device, float volume) = 0;

    virtual void set_sink_volume(float volume) = 0;
};



}
#endif
