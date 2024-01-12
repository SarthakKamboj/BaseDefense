#include "audio.h"

#include <iostream>
#include <unordered_map>
#include <string>

#include "sndfile.h"

#include "constants.h"
#include "utils/io.h"

#define NUM_SOURCES_PER_FORMAT 5

static bool sound_working = true;

static sound_device_t sound_device;

static audio_source_t background_source;
static audio_source_t mono_sources[NUM_SOURCES_PER_FORMAT];
static audio_source_t stereo_sources[NUM_SOURCES_PER_FORMAT];

static std::unordered_map<std::string, sound_t> sounds;

static std::string bck_sound_name;

void init_source_pools() {
    for (int i = 0; i < NUM_SOURCES_PER_FORMAT; i++) {
        mono_sources[i] = create_audio_source();
        stereo_sources[i] = create_audio_source();
    }
}

audio_source_t* get_source_from_pool(audio_source_t* sources) {
    for (int i = 0; i < NUM_SOURCES_PER_FORMAT; i++) {
        audio_source_t& source = sources[i];
        ALint state;
        alGetSourcei(source.al_handle, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING && state != AL_PAUSED) return &source;
    }
    return NULL;
}

void read_wav_sound(const char* name, const char *filename, bool bck_sound) {
    char resources_folder[256]{};
    get_resources_folder_path(resources_folder);

    char full_file_path[256]{};
    sprintf(full_file_path, "%s\\%s\\%s", resources_folder, AUDIO_FOLDER, filename);

    SF_INFO sound_file_info{};
    SNDFILE* sound_file = sf_open(full_file_path, SFM_READ, &sound_file_info);
    int major_type = sound_file_info.format & SF_FORMAT_TYPEMASK;
    int minor_type = sound_file_info.format & SF_FORMAT_SUBMASK;
    int endianess = sound_file_info.format & SF_FORMAT_ENDMASK;

    game_assert(major_type == SF_FORMAT_WAV);
    game_assert(sound_file_info.channels <= 2);

    ALenum format;
    if (sound_file_info.channels == 1) {
        format = AL_FORMAT_MONO16;
    } else if (sound_file_info.channels == 2) {
        format = AL_FORMAT_STEREO16;
    }

    int bytes_per_sample = sound_file_info.channels * 2;
    ALsizei num_bytes = bytes_per_sample * sound_file_info.frames;
    short* data = static_cast<short*>(malloc(num_bytes));

    sf_count_t count = sf_readf_short(sound_file, data, sound_file_info.frames);
    game_assert(count > 0);

    ALuint sound_al_handle;
    alGenBuffers(1, &sound_al_handle);
    alBufferData(sound_al_handle, format, data, num_bytes, sound_file_info.samplerate);
    free(data);

    game_assert(!detect_al_error());

    sound_t sound;
    sound.al_buffer_handle = sound_al_handle;
    sound.num_channels = sound_file_info.channels;
    sound.sample_rate = sound_file_info.samplerate;

    sounds[name] = sound;

    if (bck_sound) {
        bck_sound_name = name;
    }
}

void mute_bck_sound() {
    alSourcef(background_source.al_handle, AL_GAIN, 0.f);
}

void unmute_bck_sound() {
    alSourcef(background_source.al_handle, AL_GAIN, 0.5f);
}

void mute_sounds() {
    for (int i = 0; i < NUM_SOURCES_PER_FORMAT; i++) {
        alSourcef(mono_sources[i].al_handle, AL_GAIN, 0.f);
        alSourcef(stereo_sources[i].al_handle, AL_GAIN, 0.f);
    }
}

void unmute_sounds() {
    for (int i = 0; i < NUM_SOURCES_PER_FORMAT; i++) {
        alSourcef(mono_sources[i].al_handle, AL_GAIN, 0.5f);
        alSourcef(stereo_sources[i].al_handle, AL_GAIN, 0.5f);
    }
}

audio_source_t create_audio_source() {
    audio_source_t audio_source;
    alGenSources(1, &audio_source.al_handle);
    game_assert(!detect_al_error());
    return audio_source;
}

void play_bck_sound() {
    game_assert(bck_sound_name[0] != 0);
    sound_t& bck_sound = sounds[bck_sound_name];
    alSourcef(background_source.al_handle, AL_MIN_GAIN, 0);
    alSourcef(background_source.al_handle, AL_GAIN, 0.5f);
    alSourcei(background_source.al_handle, AL_BUFFER, bck_sound.al_buffer_handle);
    alSourcei(background_source.al_handle, AL_LOOPING, 1);
    game_assert(!detect_al_error());
    alSourcePlay(background_source.al_handle);
    game_assert(!detect_al_error());
}

void resume_bck_sound() {
    ALint state;
    alGetSourcei(background_source.al_handle, AL_SOURCE_STATE, &state);
    if (state == AL_PAUSED) {
        alSourcePlay(background_source.al_handle);
    }
    game_assert(!detect_al_error());
}

void pause_bck_sound() {
    alSourcePause(background_source.al_handle);
    game_assert(!detect_al_error());
}

void stop_bck_sound() {
    sound_t& bck_sound = sounds[bck_sound_name];
    alSourceStop(background_source.al_handle);
    game_assert(!detect_al_error());
}

void clear_sounds() {
    for (int i = 0; i < NUM_SOURCES_PER_FORMAT; i++) {
        alSourceStop(mono_sources[i].al_handle);
        alSourceStop(stereo_sources[i].al_handle);
    }
}

void play_sound(const char* sound_name, bool overrule) {
    if (overrule) {
        clear_sounds();
    }

    audio_source_t* pool = NULL;
    sound_t& sound = sounds[sound_name];
    if (sound.num_channels == 1) pool = mono_sources;
    else pool = stereo_sources;
    audio_source_t* source = get_source_from_pool(pool);
    if (!source) return;
    alSourcei(source->al_handle, AL_BUFFER, sound.al_buffer_handle);
    game_assert(!detect_al_error());
    alSourcePlay(source->al_handle);
    game_assert(!detect_al_error());
}

bool sound_finished_playing(const char* sound_name) {
    sound_t& sound = sounds[sound_name];
    ALuint buffer_al_handle = sound.al_buffer_handle;
    audio_source_t* pool = NULL;
    if (sound.num_channels == 1) {
        pool = mono_sources;
    } else {
        pool = stereo_sources;
    }

    for (int i = 0; i < NUM_SOURCES_PER_FORMAT; i++) {
        ALint cur_buffer;
        alGetSourcei(pool[i].al_handle, AL_BUFFER, &cur_buffer);
        if (cur_buffer == buffer_al_handle) {
            ALint state;
            alGetSourcei(pool[i].al_handle, AL_SOURCE_STATE, &state);
            return state == AL_STOPPED || state == AL_INITIAL;
        }
    }

    return true;
}

bool detect_al_error() {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        switch (error) {
            case AL_OUT_OF_MEMORY: {
                std::cout << "AL_OUT_OF_MEMORY error." << std::endl;
                break;
            }
            case AL_INVALID_VALUE: {
                std::cout << "AL_INVALID_VALUE error." << std::endl;
                break;
            }
            case AL_INVALID_ENUM: {
                std::cout << "AL_INVALID_ENUM error." << std::endl;
                break;
            }
            case AL_INVALID_OPERATION: {
                std::cout << "AL_INVALID_OPERATION error." << std::endl;
                break;
            }
            case AL_INVALID_NAME: {
                std::cout << "AL_INVALID_NAME error." << std::endl;
                break;
            }
            default: {
                std::cout << "error has occurred" << std::endl;
            }
        }
        return true;
    }
    return false;
}

void init_audio() {
    sound_device.device = alcOpenDevice(NULL);
    if (!sound_device.device) {
        sound_working = false;
        std::cout << "could not make device" << std::endl;
        return;
    }
    sound_device.context = alcCreateContext(sound_device.device, NULL);
    if (!sound_device.context) {
        alcCloseDevice(sound_device.device);
        sound_working = false;
        std::cout << "could not make context" << std::endl;
        return;
    }
    if (alcMakeContextCurrent(sound_device.context) == ALC_FALSE) {
        sound_working = false;
        std::cout << "could not make context current context" << std::endl;
        alcCloseDevice(sound_device.device);
        alcDestroyContext(sound_device.context);
        return;
    }

    background_source = create_audio_source();
    init_source_pools();

#if 0
    read_wav_sound("bck", BACKGROUND_MUSIC, true);
    read_wav_sound("jump", JUMP_SOUND_EFFECT);
    read_wav_sound("stomp", STOMP_SOUND_EFFECT);
    read_wav_sound("level_finish", LEVEL_FINISH_SOUND_EFFECT);

    play_bck_sound();
#endif

    std::cout << "sound initialized successfully" << std::endl;
}